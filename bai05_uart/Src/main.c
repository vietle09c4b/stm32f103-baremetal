/*
 * main.c - Bài 5: UART trên Blue Pill (STM32F103C8T6), mức thanh ghi
 *
 * Nối dây với mạch USB-UART 3,3 V (CP2102):
 *   CP2102 RXD  <-  PA9  (USART1_TX)
 *   CP2102 TXD  ->  PA10 (USART1_RX)
 *   CP2102 GND  --  GND
 * Board vẫn lấy nguồn 3,3 V từ ST-LINK; không nối chân 3V3/5V của CP2102.
 *
 * Terminal (PuTTY): 115200 baud, 8 data bit, không parity, 1 stop bit, không flow control.
 *   Gõ '1' -> LED PC13 sáng, gõ '0' -> LED tắt, ký tự khác chỉ được báo lại.
 *
 * Clock: giữ HSI 8 MHz sau reset, không bật PLL -> HCLK = PCLK2 = 8 MHz.
 * Header: CMSIS của ST (stm32f1xx.h -> stm32f103xb.h, symbol STM32F103xB).
 */
#include <stdint.h>
#include <stdio.h>
#include "stm32f1xx.h"
#include "uart.h"

#define PCLK2_HZ      8000000UL   /* USART1 dùng PCLK2 (RM0008 27.3.4) */
#define UART_BAUDRATE 115200UL
#define LED_PIN       13U         /* PC13, LED nối kiểu sink: 0 = sáng */

/* Biến toàn cục để quan sát bằng Live Expressions khi debug */
volatile uint8_t  g_last_rx;      /* ký tự nhận gần nhất */
volatile uint32_t g_rx_count;     /* số ký tự đã nhận */
volatile uint32_t g_tx_count;     /* số byte đã gửi qua __io_putchar */

/* printf của newlib gọi _write() trong syscalls.c, _write() gọi __io_putchar() cho từng ký tự */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart1_write('\r');          /* terminal cần CR LF để xuống dòng và về đầu dòng */
        g_tx_count++;
    }
    uart1_write((uint8_t)ch);
    g_tx_count++;
    return ch;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);                 /* LED tắt trước khi chuyển sang output */
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;                      /* output push-pull 2 MHz */
    GPIOC->CRH = crh;
}

int main(void)
{
    led_init();
    uart1_init(PCLK2_HZ, UART_BAUDRATE);

    printf("\nSTM32F103 USART1 bare-metal: 115200 8N1, BRR = 0x%X\n", (unsigned)USART1->BRR);
    printf("Go '1' de bat LED PC13, '0' de tat LED\n");

    for (;;) {
        uint8_t c = uart1_read();
        g_last_rx = c;
        g_rx_count++;

        if (c == '1') {
            GPIOC->BRR = (1UL << LED_PIN);          /* PC13 = 0 -> LED sáng */
        } else if (c == '0') {
            GPIOC->BSRR = (1UL << LED_PIN);         /* PC13 = 1 -> LED tắt */
        }

        if (c >= 0x20U && c < 0x7FU) {
            printf("Nhan '%c' (0x%02X), tong %lu ky tu\n", c, c, (unsigned long)g_rx_count);
        } else {
            printf("Nhan 0x%02X, tong %lu ky tu\n", c, (unsigned long)g_rx_count);
        }
    }
}
