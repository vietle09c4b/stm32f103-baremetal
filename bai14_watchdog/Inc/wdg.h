/*
 * wdg.h - IWDG, WWDG và cờ reset trên STM32F103C8, mức thanh ghi (RM0008 Rev 21, chương 19, 20, mục 7.3.10)
 */
#ifndef WDG_H
#define WDG_H

#include <stdint.h>

/* Khóa ghi vào IWDG_KR (RM0008 19.4.1) */
#define IWDG_KEY_RELOAD   0xAAAAU
#define IWDG_KEY_ACCESS   0x5555U
#define IWDG_KEY_START    0xCCCCU

void     lsi_start(void);
void     iwdg_start(uint32_t pr, uint32_t rl);
void     iwdg_refresh(void);

void     wwdg_start(uint32_t wdgtb, uint32_t window, uint32_t counter, uint32_t ewi);
void     wwdg_refresh(uint32_t counter);

uint32_t reset_flags_read_and_clear(void);   /* trả RCC_CSR[31:24] trước khi xóa */

#endif /* WDG_H */
