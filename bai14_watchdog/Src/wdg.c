/*
 * wdg.c - IWDG, WWDG và cờ reset trên STM32F103C8, mức thanh ghi
 *
 * Tài liệu: RM0008 Rev 21
 *   7.2.5   LSI ~40 kHz (30–60 kHz), LSION/LSIRDY trong RCC_CSR
 *   7.3.10  RCC_CSR: cờ reset, RMVF
 *   19      IWDG: KR, PR, RLR, SR
 *   20      WWDG: CR, CFR, SR
 * Errata ES096 Rev 15: 2.2.12 (chờ 100 µs sau LSIRDY), 2.7.1–2.7.3 (PVU/RVU)
 */
#include "stm32f1xx.h"
#include "wdg.h"

/* ------------------------------------------------------------- LSI */
void lsi_start(void)
{
    RCC->CSR |= RCC_CSR_LSION;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0U) { }
    /* ES096 2.2.12: LSIRDY bật nhưng tần số có thể chưa vào dải, chờ thêm 100 µs.
     * Vòng lặp này chạy trên HSE 8 MHz: 400 vòng x ít nhất 2 chu kỳ = hơn 100 µs. */
    for (volatile uint32_t i = 0; i < 400U; i++) { }
}

/* ------------------------------------------------------------ IWDG */
/* RM0008 19.3: 0xCCCC khởi động bộ đếm từ 0xFFF; 0x5555 mở quyền ghi PR, RLR; 0xAAAA nạp RLR.
 * Sau khi ghi PR, RLR phải chờ PVU, RVU về 0 rồi mới nạp, để lần nạp dùng giá trị mới. */
void iwdg_start(uint32_t pr, uint32_t rl)
{
    IWDG->KR  = IWDG_KEY_START;              /* bật IWDG; LSI được phần cứng bật theo */
    IWDG->KR  = IWDG_KEY_ACCESS;             /* mở khóa ghi PR, RLR */
    IWDG->PR  = pr & 0x7U;
    IWDG->RLR = rl & 0xFFFU;
    while (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) { }   /* tối đa 5 chu kỳ LSI */
    IWDG->KR  = IWDG_KEY_RELOAD;             /* nạp RLR, đồng thời khóa lại PR, RLR */
}

void iwdg_refresh(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;
}

/* ------------------------------------------------------------ WWDG */
/* RM0008 20.4: t = tPCLK1 x 4096 x 2^WDGTB x (T[5:0] + 1). Giá trị ghi vào CR luôn có T6 = 1. */
void wwdg_start(uint32_t wdgtb, uint32_t window, uint32_t counter, uint32_t ewi)
{
    RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;
    (void)RCC->APB1ENR;
    WWDG->CFR = ((wdgtb & 3U) << WWDG_CFR_WDGTB_Pos) | (window & 0x7FU) | (ewi ? WWDG_CFR_EWI : 0U);
    WWDG->SR  = 0U;                                        /* xóa EWIF (rc_w0) */
    WWDG->CR  = WWDG_CR_WDGA | 0x40U | (counter & 0x3FU);  /* bật, T6 = 1 */
}

void wwdg_refresh(uint32_t counter)
{
    WWDG->CR = WWDG_CR_WDGA | 0x40U | (counter & 0x3FU);  /* chỉ hợp lệ khi T[6:0] <= W[6:0] */
}

/* ------------------------------------------------------ cờ reset */
uint32_t reset_flags_read_and_clear(void)
{
    uint32_t f = RCC->CSR >> 24;           /* bit 7 LPWR, 6 WWDG, 5 IWDG, 4 SFT, 3 POR, 2 PIN */
    RCC->CSR |= RCC_CSR_RMVF;              /* system reset không xóa các cờ này */
    return f;
}
