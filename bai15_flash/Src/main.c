/*
 * main.c - Bài 15: Flash nội STM32F103C8 (Blue Pill), xóa trang và ghi dữ liệu mức thanh ghi
 *
 * Phần cứng: chỉ Blue Pill + ST-LINK V2.
 * Clock:     HSE 8 MHz, không PLL -> SYSCLK = HCLK = 8 MHz, Flash 0 wait state. HSI vẫn bật (cần cho FPEC).
 * Linker:    vùng FLASH giảm còn 62 KB, hai trang cuối dành cho dữ liệu:
 *              trang 62 = 0x0800 F800  (phép thử T2–T4)
 *              trang 63 = 0x0800 FC00  (nhật ký đếm khởi động T5)
 *   T1  mở khóa FPEC: LOCK 1 -> 0
 *   T2  xóa trang 62, đo thời gian, kiểm 512 half-word = 0xFFFF
 *   T3  ghi 512 half-word mẫu, đo thời gian, đọc lại so sánh
 *   T4  ghi đè ô đã ghi -> PGERR, ô giữ nguyên; ghi 0x0000 đè -> được phép
 *   T5  mỗi lần khởi động ghi bộ đếm vào half-word trống đầu tiên của trang 63
 * LED PC13: sáng đứng = T1..T5 đạt; nháy 4 Hz = có phép thử sai.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "flash.h"

#define F_CPU_HZ      8000000UL
#define LED_PIN       13U
#define TEST_PAGE     0x0800F800UL
#define LOG_PAGE      0x0800FC00UL
#define LOG_MAGIC     0xB15FU
#define N_HALF        (FLASH_PAGE_SIZE / 2U)

/* ---- Biến quan sát bằng gdb ---- */
volatile uint32_t g_clock_hse, g_hsi_on, g_acr;
volatile uint32_t g_lock_before, g_lock_after, g_lock_end;
volatile uint32_t g_t2_err, g_t2_us;                 /* xóa trang */
volatile uint32_t g_t3_err, g_t3_us, g_t3_mismatch;  /* ghi 512 half-word */
volatile uint32_t g_t3_ns_per_half;
volatile uint32_t g_t2_busy_us;                      /* thời gian BSY = 1 khi xóa trang */
volatile uint32_t g_t3_busy_min_ns, g_t3_busy_max_ns; /* thời gian BSY = 1 của mỗi lần ghi half-word */
volatile uint32_t g_t4_pgerr, g_t4_after, g_t4_zero_err, g_t4_zero_val;
volatile uint32_t g_boot_count, g_log_index, g_log_erased, g_t5_err;
volatile uint32_t g_all_ok;
volatile uint32_t g_ms;

void SysTick_Handler(void) { g_ms++; }

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

static void delay_ms(uint32_t ms) { uint32_t t0 = g_ms; while ((g_ms - t0) < ms) { } }
static uint32_t us_since(uint32_t c0) { return (DWT->CYCCNT - c0) / (F_CPU_HZ / 1000000UL); }
static uint16_t pattern(uint32_t i) { return (uint16_t)(0x1234U + i * 7U); }

/* T5: nhật ký append-only. Half-word 0 là LOG_MAGIC, các half-word sau là bộ đếm tăng dần. */
static void boot_log(void)
{
    volatile uint16_t *p = (volatile uint16_t *)LOG_PAGE;
    if (p[0] != LOG_MAGIC) {                          /* trang chưa định dạng */
        g_t5_err |= flash_erase_page(LOG_PAGE);
        g_t5_err |= flash_program_half(LOG_PAGE, LOG_MAGIC);
        g_log_erased = 1U;
    }
    uint32_t k = 1U;
    while (k < N_HALF && p[k] != 0xFFFFU) k++;       /* half-word trống đầu tiên */
    uint16_t last = (k > 1U) ? p[k - 1U] : 0U;
    if (k == N_HALF) {                                /* trang đầy: xóa rồi ghi lại từ đầu */
        g_t5_err |= flash_erase_page(LOG_PAGE);
        g_t5_err |= flash_program_half(LOG_PAGE, LOG_MAGIC);
        g_log_erased = 1U;
        k = 1U;
    }
    g_t5_err |= flash_program_half(LOG_PAGE + 2U * k, (uint16_t)(last + 1U));
    g_boot_count = (uint16_t)(last + 1U);
    g_log_index = k;
}

int main(void)
{
    g_clock_hse = clock_use_hse();
    SysTick->LOAD = (F_CPU_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    led_init();
    delay_ms(10U);

    g_hsi_on = (RCC->CR & RCC_CR_HSION) ? 1U : 0U;   /* RM0008 3.3.3: HSI phải bật khi ghi/xóa */
    g_acr    = FLASH->ACR;                            /* LATENCY = 0 ở 8 MHz, prefetch bật */

    /* T1: mở khóa */
    g_lock_before = (FLASH->CR & FLASH_CR_LOCK) ? 1U : 0U;
    flash_unlock();
    g_lock_after  = (FLASH->CR & FLASH_CR_LOCK) ? 1U : 0U;

    /* T2: xóa trang 62 */
    uint32_t c0 = DWT->CYCCNT;
    g_t2_err = flash_erase_page(TEST_PAGE);
    g_t2_us  = us_since(c0);
    g_t2_busy_us = flash_busy_cycles / (F_CPU_HZ / 1000000UL);

    /* T3: ghi 512 half-word */
    c0 = DWT->CYCCNT;
    uint32_t bmin = 0xFFFFFFFFUL, bmax = 0U;
    for (uint32_t i = 0; i < N_HALF; i++) {
        g_t3_err |= flash_program_half(TEST_PAGE + 2U * i, pattern(i));
        if (flash_busy_cycles < bmin) bmin = flash_busy_cycles;
        if (flash_busy_cycles > bmax) bmax = flash_busy_cycles;
    }
    g_t3_us = us_since(c0);
    g_t3_busy_min_ns = bmin * 125U;                   /* 1 chu kỳ = 125 ns ở 8 MHz */
    g_t3_busy_max_ns = bmax * 125U;
    g_t3_ns_per_half = (g_t3_us * 1000U) / N_HALF;
    for (uint32_t i = 0; i < N_HALF; i++) {
        if (((volatile uint16_t *)TEST_PAGE)[i] != pattern(i)) g_t3_mismatch++;
    }

    /* T4: ghi đè ô đã ghi */
    g_t4_pgerr    = flash_program_half(TEST_PAGE, 0x5555U);      /* ô đang là 0x1234 -> PGERR */
    g_t4_after    = *(volatile uint16_t *)TEST_PAGE;             /* phải còn 0x1234 */
    g_t4_zero_err = flash_program_half(TEST_PAGE, 0x0000U);      /* ngoại lệ: ghi 0x0000 được phép */
    g_t4_zero_val = *(volatile uint16_t *)TEST_PAGE;

    /* T5: nhật ký đếm khởi động */
    boot_log();

    flash_lock();
    g_lock_end = (FLASH->CR & FLASH_CR_LOCK) ? 1U : 0U;

    uint32_t ok = g_hsi_on && g_lock_before && !g_lock_after && g_lock_end
               && (g_t2_err == 0U) && (g_t3_err == 0U) && (g_t3_mismatch == 0U)
               && (g_t4_pgerr == FLASH_ERR_PG) && (g_t4_after == 0x1234U)
               && (g_t4_zero_err == 0U) && (g_t4_zero_val == 0x0000U)
               && (g_t5_err == 0U) && (g_boot_count != 0U);
    g_all_ok = ok;

    for (;;) {
        if (ok) {
            GPIOC->BRR = (1UL << LED_PIN);
        } else {
            GPIOC->ODR ^= (1UL << LED_PIN);
            delay_ms(125U);
        }
    }
}
