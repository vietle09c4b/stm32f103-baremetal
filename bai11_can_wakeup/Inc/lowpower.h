/*
 * lowpower.h - Stop mode + đánh thức bằng EXTI8 (PB8 = CAN_RX) trên STM32F103C8, mức thanh ghi
 */
#ifndef LOWPOWER_H
#define LOWPOWER_H

#include <stdint.h>

/* Cấu hình EXTI8 bắt cạnh xuống trên PB8 và bật vector EXTI9_5 (IRQ 23).
 * Gọi khi interrupt đang bị chặn (PRIMASK = 1). */
void exti8_pb8_arm(void);

/* Vào Stop mode (PDDS = 0, LPDS = 1, SLEEPDEEP = 1, WFI). Trả về khi MCU đã thức;
 * lúc đó SYSCLK là HSI 8 MHz (RM0008 5.3.4), phần gọi phải tự bật lại HSE. */
void stop_mode_enter(void);

extern volatile uint32_t g_exti8_count;     /* số lần EXTI8 báo cạnh xuống */

#endif
