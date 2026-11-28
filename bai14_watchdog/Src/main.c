/*
 * main.c - Bài 14: Watchdog IWDG và WWDG trên STM32F103C8 (Blue Pill), mức thanh ghi
 *
 * Phần cứng: chỉ Blue Pill + ST-LINK V2, không cần linh kiện ngoài.
 * Clock:     HSE 8 MHz, không PLL -> SYSCLK = HCLK = PCLK1 = 8 MHz.
 *
 * Chương trình là một máy trạng thái chạy qua nhiều lần reset. Bước hiện tại và số mili giây
 * đếm bằng SysTick được ghi vào thanh ghi backup BKP_DRx, vì system reset không xóa miền backup.
 *   bước 0  khởi động mới: ghi cờ reset, T1 = IWDG /64, RL = 624 (1,000 s @ 40 kHz), không refresh
 *   bước 1  sau reset IWDG: lưu thời gian T1; T2 = refresh IWDG mỗi 100 ms trong 3 s;
 *           T3 = WWDG (WDGTB = 3, W = 0x50, T = 0x7F), refresh ở 50 ms, trước khi cửa sổ mở
 *   bước 3  sau reset WWDG: T4 = refresh WWDG mỗi 230 ms trong 3 s;
 *           T5 = bật EWI, ngừng refresh: ghi thời điểm ngắt EWI và thời điểm reset
 *   bước 5  sau reset WWDG: tổng hợp kết quả, LED sáng đứng nếu đạt, nháy 4 Hz nếu sai
 * Chạy lại từ đầu: rút nguồn board (power reset xóa trạng thái vì kết quả cũ có cờ POR).
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "wdg.h"

#define F_CPU_HZ      8000000UL
#define LED_PIN       13U
#define MAGIC         0xA500U

/* Bố trí thanh ghi backup (mỗi thanh ghi dùng 16 bit thấp) */
#define BK_STEP       (BKP->DR1)    /* MAGIC | bước */
#define BK_MS         (BKP->DR2)    /* mili giây đếm từ lúc bắt đầu đo */
#define BK_T1_MS      (BKP->DR3)
#define BK_T2_OK      (BKP->DR4)
#define BK_T3_MS      (BKP->DR5)
#define BK_T4_OK      (BKP->DR6)
#define BK_T5_EWI     (BKP->DR7)
#define BK_FLAGS01    (BKP->DR9)    /* cờ reset lần khởi động 0 (byte cao) và 1 (byte thấp) */
#define BK_FLAGS23    (BKP->DR10)   /* cờ reset lần khởi động 2 và 3 */

/* ---- Biến quan sát bằng gdb ---- */
volatile uint32_t g_ms;
volatile uint32_t g_clock_hse;
volatile uint32_t g_step;
volatile uint32_t g_flags[4];       /* RCC_CSR[31:24] ở 4 lần khởi động */
volatile uint32_t g_t1_ms;          /* thời gian IWDG reset thực đo */
volatile uint32_t g_lsi_hz;         /* tần số LSI suy ra */
volatile uint32_t g_t2_ok;
volatile uint32_t g_t3_ms;          /* thời điểm refresh WWDG quá sớm */
volatile uint32_t g_t4_ok;
volatile uint32_t g_t5_ewi_ms;      /* thời điểm ngắt EWI */
volatile uint32_t g_t5_rst_ms;      /* thời điểm reset do WWDG hết thời gian */
volatile uint32_t g_all_ok;

static volatile uint32_t s_count_on;
static volatile uint32_t s_rel;

void SysTick_Handler(void)
{
    g_ms++;
    if (s_count_on) {
        s_rel++;
        BK_MS = (uint16_t)s_rel;     /* lưu vào miền backup mỗi 1 ms */
    }
}

void WWDG_IRQHandler(void)
{
    BK_T5_EWI = (uint16_t)s_rel;     /* bộ đếm WWDG vừa chạm 0x40 */
    WWDG->SR = 0U;                   /* xóa EWIF; không refresh -> reset ở 0x3F */
}

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

/* RM0008 6.1: bật clock PWR, BKP rồi đặt DBP để ghi được BKP_DRx */
static void backup_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_DBP;
}

static void count_start(void) { s_rel = 0U; BK_MS = 0U; s_count_on = 1U; }

static void set_step(uint32_t s) { BK_STEP = (uint16_t)(MAGIC | s); }

static void delay_ms(uint32_t ms)
{
    uint32_t t0 = g_ms;
    while ((g_ms - t0) < ms) { }
}

int main(void)
{
    uint32_t flags = reset_flags_read_and_clear();   /* đọc trước mọi việc khác */

    g_clock_hse = clock_use_hse();
    SysTick->LOAD = (F_CPU_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    led_init();
    backup_init();

    uint32_t step = ((BK_STEP & 0xFF00U) == MAGIC) ? (BK_STEP & 0xFFU) : 0U;
    if (flags & (RCC_CSR_PORRSTF >> 24)) step = 0U;    /* cấp điện lại: chạy lại từ đầu */
    g_step = step;

    switch (step) {
    case 0U:                                          /* ---- T1: IWDG hết thời gian ---- */
        BK_FLAGS01 = (uint16_t)(flags << 8);
        BK_FLAGS23 = 0U;
        BK_T1_MS = BK_T2_OK = BK_T3_MS = BK_T4_OK = BK_T5_EWI = 0U;
        set_step(1U);
        lsi_start();
        iwdg_start(4U, 624U);                         /* /64, 625 nhịp -> 1,000 s @ 40 kHz */
        count_start();
        for (;;) { }                                  /* cố ý không refresh */

    case 1U:                                          /* ---- T2 và T3 ---- */
        BK_FLAGS01 = (uint16_t)((BK_FLAGS01 & 0xFF00U) | flags);
        BK_T1_MS = BK_MS;
        iwdg_start(4U, 624U);                         /* T2: IWDG chạy lại, refresh đều */
        for (uint32_t t = 0; t < 30U; t++) { delay_ms(100U); iwdg_refresh(); }
        BK_T2_OK = 1U;                                /* qua 3 s không reset */

        set_step(3U);                                 /* T3: refresh WWDG trước khi cửa sổ mở */
        wwdg_start(3U, 0x50U, 0x3FU, 0U);             /* tick = 4096 x 8 / 8 MHz = 4,096 ms */
        count_start();
        while (s_rel < 50U) { iwdg_refresh(); }
        BK_T3_MS = (uint16_t)s_rel;
        wwdg_refresh(0x3FU);                          /* T[6:0] > W -> reset ngay */
        for (;;) { }

    case 3U:                                          /* ---- T4 và T5 ---- */
        BK_FLAGS23 = (uint16_t)(flags << 8);
        wwdg_start(3U, 0x50U, 0x3FU, 0U);
        {
            uint32_t t0 = g_ms, last = g_ms;
            while ((g_ms - t0) < 3000U) {             /* T4: refresh trong cửa sổ */
                if ((g_ms - last) >= 230U) { wwdg_refresh(0x3FU); last = g_ms; }
            }
            while ((g_ms - last) < 230U) { }
        }
        BK_T4_OK = 1U;

        set_step(5U);                                 /* T5: bật EWI rồi ngừng refresh */
        WWDG->CFR |= WWDG_CFR_EWI;
        NVIC_EnableIRQ(WWDG_IRQn);
        wwdg_refresh(0x3FU);                          /* bộ đếm về 0x7F */
        count_start();
        for (;;) { }

    default:                                          /* ---- bước 5: tổng hợp ---- */
        if (step == 5U && (BK_FLAGS23 & 0xFFU) == 0U && (BK_FLAGS23 & 0xFF00U) != 0U) {
            BK_FLAGS23 = (uint16_t)(BK_FLAGS23 | flags);   /* chỉ ghi ở lần khởi động đầu của bước 5 */
        }
        break;
    }

    /* ---- Kết quả ---- */
    g_flags[0] = BK_FLAGS01 >> 8;   g_flags[1] = BK_FLAGS01 & 0xFFU;
    g_flags[2] = BK_FLAGS23 >> 8;   g_flags[3] = BK_FLAGS23 & 0xFFU;
    g_t1_ms     = BK_T1_MS;
    g_lsi_hz    = (g_t1_ms != 0U) ? (40000000UL / g_t1_ms) : 0U;   /* 64 x 625 nhịp / t */
    g_t2_ok     = BK_T2_OK;
    g_t3_ms     = BK_T3_MS;
    g_t4_ok     = BK_T4_OK;
    g_t5_ewi_ms = BK_T5_EWI;
    g_t5_rst_ms = BK_MS;

    const uint32_t IW = RCC_CSR_IWDGRSTF >> 24, WW = RCC_CSR_WWDGRSTF >> 24;
    uint32_t ok = (g_flags[1] & IW) && (g_flags[2] & WW) && (g_flags[3] & WW)
               && (g_t1_ms >= 667U) && (g_t1_ms <= 1334U)          /* LSI 30–60 kHz */
               && g_t2_ok && g_t4_ok
               && (g_t3_ms >= 50U) && (g_t3_ms <= 52U)
               && (g_t5_ewi_ms >= 250U) && (g_t5_ewi_ms <= 262U)   /* 63 nhịp x 4,096 ms */
               && (g_t5_rst_ms >= 254U) && (g_t5_rst_ms <= 266U);  /* 64 nhịp x 4,096 ms */
    g_all_ok = ok;

    for (;;) {
        if (ok) {
            GPIOC->BRR = (1UL << LED_PIN);            /* sáng đứng */
        } else {
            GPIOC->ODR ^= (1UL << LED_PIN);           /* nháy 4 Hz */
            delay_ms(125U);
        }
    }
}
