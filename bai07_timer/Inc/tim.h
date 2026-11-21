/*
 * tim.h - TIM2 (time base + output compare toggle PA0) và TIM3 (input capture PA6)
 *         cho STM32F103 (Blue Pill), mức thanh ghi
 *
 * Tài liệu: RM0008 Rev 21
 *   15.3.1  Time-base unit (PSC có buffer, nạp ở update event)
 *   15.3.5  Input capture mode
 *   15.3.8  Output compare mode (OCxM = 011: toggle)
 *   15.4.5  TIMx_SR (UIF bit 0, CC1IF bit 1, CC1OF bit 9; rc_w0)
 *   9.1.11  Table 23: CHx output compare = AF push-pull, input capture = input floating
 *   Figure 8: TIMxCLK = PCLK1 nếu APB1 prescaler = 1, ngược lại 2 x PCLK1
 */
#ifndef TIM_H
#define TIM_H

#include <stdint.h>

/* HSI 8 MHz, AHB /1, APB1 /1 sau reset -> TIM2CLK = TIM3CLK = 8 MHz */
#define TIM_CLK_HZ          8000000UL

/* TIM2: 8 MHz / 8000 = 1 kHz; 1 kHz / 1000 = 1 Hz update */
#define TIM2_PSC_VALUE      (8000U - 1U)
#define TIM2_ARR_VALUE      (1000U - 1U)

/* TIM3: 8 MHz / 800 = 10 kHz -> 1 tick = 0,1 ms */
#define TIM3_PSC_VALUE      (800U - 1U)
#define TIM3_TICK_HZ        (TIM_CLK_HZ / (TIM3_PSC_VALUE + 1U))

void tim2_1hz_oc_pa0_init(void);   /* update 1 Hz, CH1 toggle trên PA0 */
void tim3_ic_pa6_init(void);       /* CH1 capture cạnh lên trên PA6 */

#endif /* TIM_H */
