/*
 * systick.h - Delay mili giây bằng SysTick (polling COUNTFLAG) cho STM32F103 (Cortex-M3)
 *
 * Tài liệu: PM0056 Rev 7, mục 4.5 SysTick timer (STK)
 *   STK_CTRL  0xE000E010: COUNTFLAG bit 16, CLKSOURCE bit 2, TICKINT bit 1, ENABLE bit 0
 *   STK_LOAD  0xE000E014: RELOAD[23:0], chu kỳ N xung -> RELOAD = N - 1
 *   STK_VAL   0xE000E018: ghi giá trị bất kỳ -> VAL = 0 và COUNTFLAG = 0
 *   STK_CALIB 0xE000E01C: NOREF, SKEW, TENMS
 */
#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/* Clock lõi sau reset: HSI 8 MHz, không chia AHB -> HCLK = 8 MHz */
#define SYSTICK_HCLK_HZ        8000000UL

/* 0: SysTick chạy bằng HCLK (CLKSOURCE = 1)
 * 1: SysTick chạy bằng HCLK/8 (CLKSOURCE = 0) */
#define SYSTICK_USE_HCLK_DIV8  0

#if SYSTICK_USE_HCLK_DIV8
#define SYSTICK_CLK_HZ         (SYSTICK_HCLK_HZ / 8UL)
#else
#define SYSTICK_CLK_HZ         SYSTICK_HCLK_HZ
#endif

/* Số xung trong 1 ms trừ 1 (PM0056 4.5.2: multi-shot N xung -> RELOAD = N - 1) */
#define SYSTICK_LOAD_1MS       ((SYSTICK_CLK_HZ / 1000UL) - 1UL)

#if (SYSTICK_LOAD_1MS > 0xFFFFFFUL) || (SYSTICK_LOAD_1MS == 0UL)
#error "SYSTICK_LOAD_1MS nam ngoai dai 0x000001..0xFFFFFF cua RELOAD[23:0]"
#endif

void systick_delay_ms(uint32_t ms);   /* chặn CPU trong ms mili giây */

#endif /* SYSTICK_H */
