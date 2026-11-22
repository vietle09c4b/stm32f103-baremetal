/*
 * main.c - Bài 9: DMA trên Blue Pill (STM32F103C8T6), mức thanh ghi
 *
 * Nối dây với mạch USB-UART 3,3 V (CP2102), giống bài UART:
 *   CP2102 RXD  <-  PA9  (USART1_TX)
 *   CP2102 TXD  ->  PA10 (USART1_RX, không dùng trong bài này)
 *   CP2102 GND  --  GND
 * Terminal (PuTTY): 115200 baud, 8N1, không flow control.
 *
 * Chương trình:
 *   1. Copy bảng 32 word từ flash sang RAM bằng DMA1 Channel 1 (memory-to-memory), so sánh kết quả.
 *   2. Mỗi 1 s gửi một dòng chữ qua USART1 bằng DMA1 Channel 4; ngắt transfer complete báo xong.
 *   3. Trong lúc DMA gửi, vòng lặp chính vẫn chạy và đếm số vòng lặp (g_loops_during_tx).
 *
 * Clock: HSI 8 MHz sau reset, không PLL -> HCLK = PCLK2 = 8 MHz.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "uart.h"
#include "dma.h"

#define HCLK_HZ        8000000UL
#define PCLK2_HZ       8000000UL
#define UART_BAUDRATE  115200UL
#define LED_PIN        13U          /* PC13, LED nối kiểu sink: 0 = sáng */
#define M2M_WORDS      32U

/* ---- Biến quan sát bằng Live Expressions ---- */
volatile uint32_t g_ms;                 /* tăng trong SysTick_Handler */
volatile uint32_t g_tx_started;         /* số lần gọi dma1_usart1_send thành công */
volatile uint32_t g_loops_during_tx;    /* số vòng lặp main chạy trong lúc DMA đang gửi */
volatile uint32_t g_loops_last;         /* giá trị của lần gửi trước, in ra terminal */
volatile int32_t  g_m2m_result = -2;    /* 0 = đúng, -1 = TEIF1, -3 = sai dữ liệu */
volatile uint32_t g_m2m_dst[M2M_WORDS];

static const uint32_t s_m2m_src[M2M_WORDS] = {
    0x00000000, 0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777,
    0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF,
    0x01234567, 0x89ABCDEF, 0xDEADBEEF, 0xCAFEBABE, 0x0BADF00D, 0x12345678, 0x87654321, 0xA5A5A5A5,
    0x5A5A5A5A, 0x0F0F0F0F, 0xF0F0F0F0, 0x00FF00FF, 0xFF00FF00, 0x13579BDF, 0x2468ACE0, 0xFEEDFACE
};

static uint8_t s_tx_buf[64];            /* không sửa khi g_dma_tx_busy = 1 */

void SysTick_Handler(void)
{
    g_ms++;
}

static void systick_1ms_init(void)
{
    SysTick->LOAD = (HCLK_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;                      /* output push-pull 2 MHz */
    GPIOC->CRH = crh;
}

static void led_toggle(void)
{
    GPIOC->ODR ^= (1UL << LED_PIN);
}

/* Ghi chuỗi s vào p, trả về con trỏ sau ký tự cuối */
static uint8_t *put_str(uint8_t *p, const char *s)
{
    while (*s != '\0') {
        *p++ = (uint8_t)*s++;
    }
    return p;
}

/* Ghi số nguyên không dấu dạng thập phân */
static uint8_t *put_u32(uint8_t *p, uint32_t v)
{
    char tmp[10];
    uint32_t n = 0U;
    do {
        tmp[n++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U);
    while (n > 0U) {
        *p++ = (uint8_t)tmp[--n];
    }
    return p;
}

static void send_and_wait(uint16_t len)
{
    while (dma1_usart1_send(s_tx_buf, len) != 0) {
    }
    while (g_dma_tx_busy != 0U) {
    }
}

int main(void)
{
    led_init();
    uart1_init(PCLK2_HZ, UART_BAUDRATE);
    dma1_ch4_usart1_tx_init();
    systick_1ms_init();

    /* 1. Memory-to-memory: flash -> RAM */
    int rc = dma1_ch1_memcpy32(s_m2m_src, (uint32_t *)g_m2m_dst, M2M_WORDS);
    if (rc == 0) {
        for (uint32_t i = 0U; i < M2M_WORDS; i++) {
            if (g_m2m_dst[i] != s_m2m_src[i]) {
                rc = -3;
                break;
            }
        }
    }
    g_m2m_result = rc;

    uint8_t *p = put_str(s_tx_buf, "\r\nSTM32F103 DMA1: M2M 32 word ");
    p = put_str(p, (rc == 0) ? "OK\r\n" : "LOI\r\n");
    send_and_wait((uint16_t)(p - s_tx_buf));

    uint32_t next_ms = g_ms + 1000U;
    uint32_t last_done = g_dma_tx_done;

    for (;;) {
        if (g_dma_tx_busy != 0U) {
            g_loops_during_tx++;                    /* CPU vẫn chạy khi DMA đang gửi */
        }

        if (g_dma_tx_done != last_done) {           /* có transfer complete mới */
            last_done = g_dma_tx_done;
            led_toggle();
        }

        if ((int32_t)(g_ms - next_ms) >= 0 && g_dma_tx_busy == 0U) {
            next_ms += 1000U;
            g_loops_last = g_loops_during_tx;
            g_loops_during_tx = 0U;

            p = put_str(s_tx_buf, "DMA #");
            p = put_u32(p, g_tx_started + 1U);
            p = put_str(p, "  loops trong luc gui truoc: ");
            p = put_u32(p, g_loops_last);
            p = put_str(p, "\r\n");

            if (dma1_usart1_send(s_tx_buf, (uint16_t)(p - s_tx_buf)) == 0) {
                g_tx_started++;
            }
        }
    }
}
