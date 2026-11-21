/*
 * irq_drivers.h - Ba nguồn ngắt trên STM32F103 (Blue Pill), mức thanh ghi
 *
 *   SysTick (exception 15)  : nhịp 1 ms, TICKINT = 1          (PM0056 4.5)
 *   TIM2    (IRQ 28)        : update 1 Hz, DIER.UIE = 1        (RM0008 15.4.4)
 *   EXTI0   (IRQ 6)         : PA0 cạnh xuống (nút nhấn về GND) (RM0008 10.2, 9.4.3)
 *
 * NVIC được ghi trực tiếp qua NVIC->ISER[] và NVIC->IPR[] (PM0056 4.3.2, 4.3.7);
 * STM32F103 chỉ dùng 4 bit cao của mỗi byte ưu tiên -> 16 mức 0..15, 0 cao nhất.
 */
#ifndef IRQ_DRIVERS_H
#define IRQ_DRIVERS_H

#include <stdint.h>

#define HCLK_HZ          8000000UL      /* HSI 8 MHz sau reset, AHB/APB1 không chia */

/* Mức ưu tiên (0..15): số nhỏ ưu tiên cao */
#define PRIO_SYSTICK     0U
#define PRIO_EXTI0       1U
#define PRIO_TIM2        2U

void nvic_set_priority(int32_t irqn, uint32_t prio);   /* IRQ >= 0 hoặc SysTick (-1) */
void nvic_enable_irq(int32_t irqn);                    /* chỉ cho IRQ >= 0 */

void systick_1ms_irq_init(void);
void tim2_1hz_irq_init(void);
void exti0_pa0_falling_init(void);

#endif /* IRQ_DRIVERS_H */
