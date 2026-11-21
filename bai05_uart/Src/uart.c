/*
 * uart.c - Driver USART1 mức thanh ghi cho STM32F103 (Blue Pill)
 *
 * Tài liệu: RM0008 Rev 21
 *   27.3.4  Fractional baud rate generation (BRR = mantissa:fraction)
 *   27.6.1  USART_SR  (TXE bit 7, TC bit 6, RXNE bit 5)
 *   27.6.4  USART_CR1 (UE bit 13, TE bit 3, RE bit 2)
 *   9.1.11  Table 24: TX = alternate function push-pull, RX = input floating / input pull-up
 *   9.3.8   Table 54: USART1_REMAP = 0 -> TX = PA9, RX = PA10
 *
 * Khi biên dịch host test (UART_HOST_TEST), thanh ghi được thay bằng biến trong RAM.
 */
#ifdef UART_HOST_TEST
#include "host_regs.h"
#else
#include "stm32f1xx.h"
#endif
#include "uart.h"

#define UART_TX_PIN   9U     /* PA9  -> CRH bit 7:4  */
#define UART_RX_PIN   10U    /* PA10 -> CRH bit 11:8 */

/* 4 bit cấu hình chân trong CRH (RM0008 Table 20, 21) */
#define CFG_AF_PP_2MHZ   0xAU    /* CNF = 10 (AF push-pull), MODE = 10 (output 2 MHz) */
#define CFG_IN_PULL      0x8U    /* CNF = 10 (input pull-up/down), MODE = 00; ODR chọn up */

uint16_t uart_compute_brr(uint32_t periph_clk, uint32_t baudrate)
{
    /* 16 * USARTDIV = fCK / baud. Cộng baud/2 trước khi chia để làm tròn tới số gần nhất.
     * Khi phần lẻ tròn lên 16, phép cộng tự nhớ sang mantissa (RM0008 27.3.4, Example 3). */
    return (uint16_t)((periph_clk + (baudrate / 2U)) / baudrate);
}

static void uart1_gpio_init(void)
{
    uint32_t crh = GPIOA->CRH;
    crh &= ~((0xFU << ((UART_TX_PIN - 8U) * 4U)) | (0xFU << ((UART_RX_PIN - 8U) * 4U)));
    crh |=  (CFG_AF_PP_2MHZ << ((UART_TX_PIN - 8U) * 4U));    /* PA9: AF push-pull  */
    crh |=  (CFG_IN_PULL    << ((UART_RX_PIN - 8U) * 4U));    /* PA10: input pull   */
    GPIOA->BSRR = (1U << UART_RX_PIN);                       /* ODR10 = 1 -> pull-up */
    GPIOA->CRH  = crh;
}

void uart1_init(uint32_t periph_clk, uint32_t baudrate)
{
    /* 1. Clock: GPIOA (bit 2) và USART1 (bit 14) cùng nằm trên APB2 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;

    /* 2. Chân PA9/PA10 */
    uart1_gpio_init();

    /* 3. Bật USART, khung 8N1: M = 0, PCE = 0 (CR1), STOP = 00 (CR2, giá trị reset) */
    USART1->CR1 = USART_CR1_UE;
    USART1->CR2 = 0U;

    /* 4. Baud rate (fCK của USART1 là PCLK2) */
    USART1->BRR = uart_compute_brr(periph_clk, baudrate);

    /* 5. Bật bộ phát và bộ thu */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
}

void uart1_write(uint8_t ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0U) {
        /* chờ TDR trống */
    }
    USART1->DR = ch;                 /* ghi DR xóa TXE */
}

int uart1_rx_ready(void)
{
    return (USART1->SR & USART_SR_RXNE) ? 1 : 0;
}

uint8_t uart1_read(void)
{
    while ((USART1->SR & USART_SR_RXNE) == 0U) {
        /* chờ có dữ liệu */
    }
    return (uint8_t)(USART1->DR & 0xFFU);   /* đọc DR xóa RXNE (và ORE nếu đọc SR trước) */
}

void uart1_flush(void)
{
    while ((USART1->SR & USART_SR_TC) == 0U) {
        /* chờ khung cuối cùng rời khỏi shift register */
    }
}
