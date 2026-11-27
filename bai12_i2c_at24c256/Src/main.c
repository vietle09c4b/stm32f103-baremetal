/*
 * main.c - Bài 12: EEPROM AT24C256 qua I2C1 trên STM32F103C8 (Blue Pill), mức thanh ghi
 *
 * Nối dây: PB6 -> SCL, PB7 -> SDA, 3V3 -> VCC, GND -> GND (module AT24C256: A0 = A1 = A2 = 0,
 *          WP = 0, điện trở kéo lên có sẵn trên module). Địa chỉ 7 bit 0x50.
 * Clock:   HSE 8 MHz, không PLL -> SYSCLK = HCLK = PCLK1 = 8 MHz. I2C1: CCR = 46 -> SCL khoảng 87 kHz.
 *
 * Chương trình chạy một lần sau mỗi reset, kết quả xem bằng Live Expressions:
 *   T0  dò địa chỉ 0x50
 *   T4  bộ đếm khởi động 4 byte ở 0x7FC0 (trang cuối): đọc, cộng 1, ghi lại -> tăng sau mỗi lần cấp điện
 *   T1  ghi rồi đọc lại N = 1, 2, 3, 64 byte và 100 byte vắt qua biên trang, so từng byte
 *   T2  một lệnh page write 8 byte bắt đầu ở offset 60 của trang 0x0300: 4 byte cuối quay về đầu trang
 *   T3  ACK polling: số lần NACK và thời gian ghi thực (µs) của lần ghi gần nhất / lớn nhất
 * LED PC13: sáng đứng = mọi phép thử đạt; nháy 4 Hz = có phép thử sai.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "i2c1.h"
#include "at24c256.h"

#define F_CPU_HZ        8000000UL
#define I2C_SCL_HZ      87000UL
#define LED_PIN         13U
#define BOOT_CNT_ADDR   0x7FC0U
#define T1_BASE         0x0100U
#define T2_PAGE         0x0300U

/* ---- Biến quan sát bằng Live Expressions ---- */
volatile uint32_t g_ms;
volatile uint32_t g_clock_hse;
volatile int32_t  g_probe_rc = 99;
volatile uint32_t g_boot_count;
volatile uint32_t g_t1_err[5];          /* số byte sai cho N = 1, 2, 3, 64, 100 */
volatile int32_t  g_t1_rc[5];
volatile uint8_t  g_t2_page[64];        /* nội dung trang 0x0300 sau phép thử roll-over */
volatile uint8_t  g_t2_next[4];         /* 4 byte đầu trang 0x0340 (không được bị ghi) */
volatile uint32_t g_t2_ok;
volatile uint32_t g_read64_us;          /* thời gian đọc 64 byte (gồm 2 byte địa chỉ + Sr) */
volatile uint32_t g_all_ok;

void SysTick_Handler(void)
{
    g_ms++;                              /* ngắt thật đang chạy: lý do phải che ngắt trong driver */
}

/* RM0008 7.3.1/7.3.2: chạy HSE 8 MHz; nếu không có HSE thì ở lại HSI (cũng 8 MHz) */
static uint32_t clock_use_hse(void)
{
    RCC->CR |= RCC_CR_HSEON;
    for (uint32_t t = 0; t < 200000UL; t++) {
        if (RCC->CR & RCC_CR_HSERDY) {
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;
            while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) { }
            return 1U;
        }
    }
    RCC->CR &= ~RCC_CR_HSEON;
    return 0U;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);                 /* tắt (LED nối kiểu sink) */
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13)) | GPIO_CRH_MODE13_1;
}

static void delay_ms(uint32_t ms)
{
    uint32_t t0 = g_ms;
    while ((g_ms - t0) < ms) { }
}

static uint32_t compare(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    uint32_t bad = 0U;
    for (uint32_t i = 0; i < n; i++) if (a[i] != b[i]) bad++;
    return bad;
}

static uint8_t wbuf[100];
static uint8_t rbuf[100];

static void test_boot_counter(void)
{
    uint8_t b[4];
    if (at24_read(BOOT_CNT_ADDR, b, 4U) != I2C_OK) return;
    uint32_t c = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    if (c == 0xFFFFFFFFUL) c = 0U;                  /* EEPROM mới xuất xưởng chứa 0xFF */
    c++;
    b[0] = (uint8_t)c; b[1] = (uint8_t)(c >> 8); b[2] = (uint8_t)(c >> 16); b[3] = (uint8_t)(c >> 24);
    if (at24_write(BOOT_CNT_ADDR, b, 4U) == I2C_OK) g_boot_count = c;
}

static void test_rw(void)
{
    static const uint16_t addr[5] = { T1_BASE + 0x00U, T1_BASE + 0x08U, T1_BASE + 0x10U,
                                      T1_BASE + 0x40U, T1_BASE + 0x9EU };   /* 0x019E..0x0201: 2 biên trang */
    static const uint8_t  len[5]  = { 1U, 2U, 3U, 64U, 100U };

    for (uint32_t k = 0; k < 5U; k++) {
        for (uint32_t i = 0; i < len[k]; i++) {
            wbuf[i] = (uint8_t)(g_boot_count * 7U + k * 31U + i);  /* mỗi lần chạy một mẫu khác */
            rbuf[i] = 0U;
        }
        int32_t rc = (int32_t)at24_write(addr[k], wbuf, len[k]);
        if (rc == 0) rc = (int32_t)at24_read(addr[k], rbuf, len[k]);
        g_t1_rc[k]  = rc;
        g_t1_err[k] = (rc == 0) ? compare(wbuf, rbuf, len[k]) : len[k];
    }
}

static void test_rollover(void)
{
    uint8_t v[8];
    uint8_t fill[68];
    uint32_t ok = 1U;

    for (uint32_t i = 0; i < 68U; i++) fill[i] = 0xAAU;
    if (at24_write(T2_PAGE, fill, 68U) != I2C_OK) { g_t2_ok = 0U; return; }   /* trang 0x0300 + 4 byte đầu trang 0x0340 */

    for (uint32_t i = 0; i < 8U; i++) v[i] = (uint8_t)(0x10U + i + g_boot_count);
    if (at24_write_raw(T2_PAGE + 60U, v, 8U) != I2C_OK) { g_t2_ok = 0U; return; } /* cố ý vượt biên trang */

    uint32_t c0 = DWT->CYCCNT;
    if (at24_read(T2_PAGE, rbuf, 64U) != I2C_OK) { g_t2_ok = 0U; return; }
    g_read64_us = (DWT->CYCCNT - c0) / (F_CPU_HZ / 1000000UL);
    for (uint32_t i = 0; i < 64U; i++) g_t2_page[i] = rbuf[i];

    if (at24_read(T2_PAGE + 64U, rbuf, 4U) != I2C_OK) { g_t2_ok = 0U; return; }
    for (uint32_t i = 0; i < 4U; i++) g_t2_next[i] = rbuf[i];

    for (uint32_t i = 0; i < 4U; i++) {
        if (g_t2_page[60U + i] != v[i])      ok = 0U;   /* 4 byte đầu vào offset 60..63 */
        if (g_t2_page[i]       != v[4U + i]) ok = 0U;   /* 4 byte sau quay về offset 0..3 */
        if (g_t2_next[i]       != 0xAAU)     ok = 0U;   /* trang kế tiếp không đổi */
    }
    for (uint32_t i = 4U; i < 60U; i++) if (g_t2_page[i] != 0xAAU) ok = 0U;
    g_t2_ok = ok;
}

int main(void)
{
    g_clock_hse = clock_use_hse();
    SysTick->LOAD = (F_CPU_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    led_init();

    i2c1_init(F_CPU_HZ, I2C_SCL_HZ);
    at24_init(F_CPU_HZ);
    delay_ms(5U);                                   /* EEPROM ổn định sau cấp nguồn */

    g_probe_rc = (int32_t)i2c1_probe(AT24_ADDR7);
    if (g_probe_rc == 0) {
        test_boot_counter();
        test_rw();
        test_rollover();
    }

    uint32_t ok = (g_probe_rc == 0) && (g_t2_ok == 1U) && (g_boot_count != 0U);
    for (uint32_t k = 0; k < 5U; k++) if (g_t1_rc[k] != 0 || g_t1_err[k] != 0U) ok = 0U;
    g_all_ok = ok;

    for (;;) {
        if (ok) {
            GPIOC->BSRR = (1UL << (LED_PIN + 16U)); /* sáng đứng */
        } else {
            GPIOC->ODR ^= (1UL << LED_PIN);         /* nháy 4 Hz */
            delay_ms(125U);
        }
    }
}
