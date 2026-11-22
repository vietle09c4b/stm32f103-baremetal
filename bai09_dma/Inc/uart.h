/*
 * uart.h - Driver USART1 mức thanh ghi cho STM32F103 (Blue Pill)
 *
 * USART1: TX = PA9, RX = PA10 (RM0008 Table 54, USART1_REMAP = 0).
 * Khung truyền 8N1: 1 start bit, 8 data bit, không parity, 1 stop bit.
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

/* Tính giá trị BRR = USARTDIV * 16, làm tròn tới số nguyên gần nhất.
 * RM0008 27.3.4: baud = fCK / (16 * USARTDIV); BRR[15:4] = mantissa, BRR[3:0] = fraction. */
uint16_t uart_compute_brr(uint32_t periph_clk, uint32_t baudrate);

void    uart1_init(uint32_t periph_clk, uint32_t baudrate);   /* bật TX và RX */
void    uart1_write(uint8_t ch);                              /* chờ TXE rồi ghi DR */
uint8_t uart1_read(void);                                     /* chờ RXNE rồi đọc DR */
int     uart1_rx_ready(void);                                 /* 1 nếu RXNE = 1 */
void    uart1_flush(void);                                    /* chờ TC = 1 */

#endif /* UART_H */
