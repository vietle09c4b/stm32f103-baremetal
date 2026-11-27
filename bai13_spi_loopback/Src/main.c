/*
 * main.c - Bài 13: SPI1 mức thanh ghi trên STM32F103C8 (Blue Pill), kiểm chứng bằng loopback
 *
 * Nối dây: một dây jumper PA7 (MOSI) -> PA6 (MISO). PA5 (SCK) và PA4 (CS) để trống.
 * Clock:   HSE 8 MHz, không PLL -> SYSCLK = HCLK = PCLK2 = 8 MHz. SPI1 nằm trên APB2.
 *
 * Chương trình chạy các phép thử một lần sau reset, kết quả xem bằng Live Expressions:
 *   T1  256 byte ở mode 0..3 (8 bit) và 256 frame 16 bit; mức nghỉ của SCK đọc qua GPIOA_IDR
 *   T2  thời gian truyền 256 byte ở BR = /2 .. /256: kiểu từng byte và kiểu liên tục (25.3.5)
 *   T3  cố ý gây OVR (gửi 3 byte không đọc), rồi xóa bằng đọc DR + đọc SR
 *   T4  CRC phần cứng 8 bit (CRCPR = 7) qua 16 byte, CRCNEXT sau byte cuối
 *   T5  cố ý gây MODF (SSM = 1, SSI = 0), rồi xóa và khôi phục master
 *   T6  sau đó gửi 0x55 mỗi 250 ms: có jumper nhận 0x55, rút jumper nhận 0xFF (MISO kéo lên)
 * LED PC13: sáng đứng = T1..T5 đạt; nháy 4 Hz = có phép thử sai.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "spi1.h"

#define F_CPU_HZ        8000000UL
#define LED_PIN         13U
#define N_BYTES         256U
#define TEST_BR         2U                  /* /8 -> 1 MHz cho T1, T3, T4, T5 */

/* ---- Biến quan sát bằng Live Expressions ---- */
volatile uint32_t g_ms;
volatile uint32_t g_clock_hse;
volatile uint32_t g_t1_err[5];              /* mode 0..3 (8 bit), [4] = mode 0, 16 bit */
volatile int32_t  g_t1_rc[5];
volatile uint32_t g_sck_idle[4];            /* mức PA5 khi nghỉ ở mode 0..3 */
volatile uint32_t g_t2_simple_us[8];
volatile uint32_t g_t2_simple_err[8];
volatile uint32_t g_t2_burst_us[8];
volatile uint32_t g_t2_burst_err[8];
volatile int32_t  g_t2_burst_rc[8];
volatile uint32_t g_t2_theory_us[8];
volatile uint32_t g_ovr_sr_before;          /* SR sau 3 byte không đọc */
volatile uint32_t g_ovr_dr;                 /* byte DR trả về khi đang OVR */
volatile uint32_t g_ovr_sr_after;           /* SR sau chuỗi xóa */
volatile uint32_t g_ovr_recover_rx;
volatile uint32_t g_crc_tx, g_crc_rx, g_crc_rcv, g_crc_sw, g_crc_sr;
volatile uint32_t g_modf_sr, g_modf_cr1, g_modf_sr_after, g_modf_cr1_after, g_modf_recover_rx;
volatile uint32_t g_live_rx;
volatile uint32_t g_live_count;
volatile uint32_t g_all_ok;

void SysTick_Handler(void)
{
    g_ms++;
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
    GPIOC->BSRR = (1UL << LED_PIN);
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13)) | GPIO_CRH_MODE13_1;
}

static uint32_t cyc_to_us(uint32_t cyc) { return cyc / (F_CPU_HZ / 1000000UL); }

/* chờ một cờ SR với giới hạn số vòng; trả 1 nếu cờ đạt trạng thái mong muốn */
static uint32_t sr_wait(uint32_t mask, uint32_t want_set)
{
    for (uint32_t t = 0; t < 100000UL; t++) {
        uint32_t v = SPI1->SR & mask;
        if (want_set ? (v != 0U) : (v == 0U)) return 1U;
    }
    return 0U;
}

static uint8_t tx[N_BYTES];
static uint8_t rx[N_BYTES];

static uint32_t count_err(uint32_t n)
{
    uint32_t bad = 0U;
    for (uint32_t i = 0; i < n; i++) if (tx[i] != rx[i]) bad++;
    return bad;
}

/* ---------------- T1: bốn mode, 8 bit và 16 bit ---------------- */
static void test_modes(void)
{
    for (uint32_t m = 0; m < 4U; m++) {
        spi1_config(TEST_BR, m, 0U);
        for (uint32_t i = 0; i < N_BYTES; i++) { tx[i] = (uint8_t)(i ^ (m * 0x5AU)); rx[i] = 0U; }
        spi1_cs_low();
        g_t1_rc[m] = (int32_t)spi1_transfer(tx, rx, N_BYTES);
        spi1_cs_high();
        g_t1_err[m] = count_err(N_BYTES);
        g_sck_idle[m] = (GPIOA->IDR >> 5) & 1U;      /* SCK đang nghỉ: phải bằng CPOL */
    }

    spi1_config(TEST_BR, SPI_MODE0, 1U);             /* DFF = 1 */
    uint32_t bad = 0U;
    int32_t rc = 0;
    spi1_cs_low();
    for (uint32_t i = 0; i < N_BYTES && rc == 0; i++) {
        uint16_t w = (uint16_t)((i * 257U) ^ 0x5AA5U), r = 0U;
        rc = (int32_t)spi1_xfer16(w, &r);
        if (r != w) bad++;
    }
    spi1_cs_high();
    g_t1_rc[4] = rc;
    g_t1_err[4] = bad;
}

/* ---------------- T2: tốc độ ở 8 hệ số chia ---------------- */
static void test_speed(void)
{
    for (uint32_t i = 0; i < N_BYTES; i++) tx[i] = (uint8_t)(0xFFU - i);

    for (uint32_t br = 0; br < 8U; br++) {
        spi1_config(br, SPI_MODE0, 0U);
        g_t2_theory_us[br] = N_BYTES * 8UL * (2UL << br) / (F_CPU_HZ / 1000000UL);

        for (uint32_t i = 0; i < N_BYTES; i++) rx[i] = 0U;
        uint32_t c0 = DWT->CYCCNT;
        (void)spi1_transfer(tx, rx, N_BYTES);
        g_t2_simple_us[br]  = cyc_to_us(DWT->CYCCNT - c0);
        g_t2_simple_err[br] = count_err(N_BYTES);

        for (uint32_t i = 0; i < N_BYTES; i++) rx[i] = 0U;
        c0 = DWT->CYCCNT;
        g_t2_burst_rc[br]  = (int32_t)spi1_transfer_burst(tx, rx, N_BYTES);
        g_t2_burst_us[br]  = cyc_to_us(DWT->CYCCNT - c0);
        g_t2_burst_err[br] = count_err(N_BYTES);
        if (SPI1->SR & SPI_SR_OVR) { (void)SPI1->DR; (void)SPI1->SR; }
    }
}

/* ---------------- T3: overrun ---------------- */
static uint32_t test_ovr(void)
{
    spi1_config(TEST_BR, SPI_MODE0, 0U);
    static const uint8_t b[3] = { 0xA1U, 0xA2U, 0xA3U };
    for (uint32_t i = 0; i < 3U; i++) {              /* ghi 3 byte, cố ý không đọc DR */
        if (!sr_wait(SPI_SR_TXE, 1U)) return 0U;
        SPI1->DR = b[i];
    }
    if (!sr_wait(SPI_SR_TXE, 1U)) return 0U;
    if (!sr_wait(SPI_SR_BSY, 0U)) return 0U;         /* 25.3.5: chờ TXE trước rồi mới chờ BSY */

    g_ovr_sr_before = SPI1->SR;                      /* mong đợi OVR = 1, RXNE = 1 */
    g_ovr_dr        = SPI1->DR;                      /* bước 1 xóa OVR: đọc DR */
    (void)SPI1->SR;                                  /* bước 2: đọc SR */
    g_ovr_sr_after  = SPI1->SR;

    uint8_t r = 0U;
    (void)spi1_xfer8(0x3CU, &r);
    g_ovr_recover_rx = r;

    return ((g_ovr_sr_before & SPI_SR_OVR) != 0U) && (g_ovr_dr == 0xA1U)
        && ((g_ovr_sr_after & (SPI_SR_OVR | SPI_SR_RXNE)) == 0U) && (r == 0x3CU);
}

/* ---------------- T4: CRC phần cứng ---------------- */
static uint8_t crc8_sw(const uint8_t *d, uint32_t n)   /* đa thức 0x07, giá trị đầu 0, MSB trước */
{
    uint8_t c = 0U;
    for (uint32_t i = 0; i < n; i++) {
        c ^= d[i];
        for (uint32_t k = 0; k < 8U; k++) c = (c & 0x80U) ? (uint8_t)((c << 1) ^ 0x07U) : (uint8_t)(c << 1);
    }
    return c;
}

static uint32_t test_crc(void)
{
    static const uint8_t msg[16] = { 'v','i','e','t','c','4','b','.','c','o','m',' ','S','P','I','!' };
    uint8_t r;

    spi1_config(TEST_BR, SPI_MODE0, 0U);
    spi1_disable();                                  /* CRCEN chỉ ghi khi SPE = 0 */
    SPI1->CRCPR = 0x07U;
    SPI1->CR1 |= SPI_CR1_CRCEN;                      /* bật CRC: xóa RXCRCR và TXCRCR */
    SPI1->CR1 |= SPI_CR1_SPE;

    for (uint32_t i = 0; i < 15U; i++) (void)spi1_xfer8(msg[i], &r);

    if (!sr_wait(SPI_SR_TXE, 1U)) return 0U;
    SPI1->DR = msg[15];
    SPI1->CR1 |= SPI_CR1_CRCNEXT;                    /* ghi ngay sau byte cuối (25.3.6) */
    if (!sr_wait(SPI_SR_RXNE, 1U)) return 0U;
    (void)SPI1->DR;                                  /* byte dữ liệu cuối */
    if (!sr_wait(SPI_SR_RXNE, 1U)) return 0U;
    g_crc_rcv = SPI1->DR;                            /* byte CRC đi vòng qua dây */
    if (!sr_wait(SPI_SR_TXE, 1U)) return 0U;
    if (!sr_wait(SPI_SR_BSY, 0U)) return 0U;

    g_crc_sr = SPI1->SR;
    g_crc_tx = SPI1->TXCRCR;
    g_crc_rx = SPI1->RXCRCR;
    g_crc_sw = crc8_sw(msg, 16U);

    SPI1->SR = (uint16_t)~SPI_SR_CRCERR;             /* CRCERR là rc_w0 */
    spi1_disable();
    SPI1->CR1 &= ~(SPI_CR1_CRCEN | SPI_CR1_CRCNEXT);

    return ((g_crc_sr & SPI_SR_CRCERR) == 0U) && (g_crc_rcv == g_crc_tx) && (g_crc_rx == g_crc_tx);
}

/* ---------------- T5: master mode fault ---------------- */
static uint32_t test_modf(void)
{
    spi1_disable();
    SPI1->CR1 = (TEST_BR << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_MSTR;   /* SSI = 0: NSS nội ở mức thấp */
    SPI1->CR1 |= SPI_CR1_SPE;

    g_modf_sr  = SPI1->SR;                           /* bước 1 xóa MODF: đọc SR khi MODF = 1 */
    g_modf_cr1 = SPI1->CR1;                          /* mong đợi MSTR = 0, SPE = 0 */

    spi1_config(TEST_BR, SPI_MODE0, 0U);             /* bước 2: ghi CR1 (SSI = 1), rồi bật lại */
    g_modf_sr_after  = SPI1->SR;
    g_modf_cr1_after = SPI1->CR1;

    uint8_t r = 0U;
    (void)spi1_xfer8(0xC3U, &r);
    g_modf_recover_rx = r;

    return ((g_modf_sr & SPI_SR_MODF) != 0U)
        && ((g_modf_cr1 & (SPI_CR1_MSTR | SPI_CR1_SPE)) == 0U)
        && ((g_modf_sr_after & SPI_SR_MODF) == 0U)
        && ((g_modf_cr1_after & (SPI_CR1_MSTR | SPI_CR1_SPE)) == (SPI_CR1_MSTR | SPI_CR1_SPE))
        && (r == 0xC3U);
}

int main(void)
{
    g_clock_hse = clock_use_hse();
    SysTick->LOAD = (F_CPU_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* DWT->CYCCNT để đo thời gian */
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    led_init();

    spi1_gpio_init();

    test_modes();
    test_speed();
    uint32_t ok_ovr  = test_ovr();
    uint32_t ok_crc  = test_crc();
    uint32_t ok_modf = test_modf();

    uint32_t ok = ok_ovr && ok_crc && ok_modf;
    for (uint32_t m = 0; m < 5U; m++) if (g_t1_rc[m] != 0 || g_t1_err[m] != 0U) ok = 0U;
    for (uint32_t m = 0; m < 4U; m++) if (g_sck_idle[m] != (m >> 1)) ok = 0U;   /* CPOL = bit 1 của mode */
    for (uint32_t b = 0; b < 8U; b++) if (g_t2_simple_err[b] != 0U) ok = 0U;
    g_all_ok = ok;

    spi1_config(TEST_BR, SPI_MODE0, 0U);
    uint32_t t_led = g_ms, t_live = g_ms;
    for (;;) {
        if ((g_ms - t_live) >= 250U) {               /* T6: rút jumper PA7-PA6 thì nhận 0xFF */
            t_live = g_ms;
            uint8_t r = 0U;
            spi1_cs_low();
            (void)spi1_xfer8(0x55U, &r);
            spi1_cs_high();
            g_live_rx = r;
            g_live_count++;
        }
        if (ok) {
            GPIOC->BRR = (1UL << LED_PIN);           /* sáng đứng */
        } else if ((g_ms - t_led) >= 125U) {
            t_led = g_ms;
            GPIOC->ODR ^= (1UL << LED_PIN);          /* nháy 4 Hz */
        }
    }
}
