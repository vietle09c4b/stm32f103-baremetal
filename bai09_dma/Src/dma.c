/*
 * dma.c - DMA1 mức thanh ghi trên STM32F103 (Blue Pill)
 *
 * RM0008 Rev 21:
 *   13.3.3  Channel configuration procedure: CPAR -> CMAR -> CNDTR -> PL -> CCR -> EN
 *   13.3.7  Table 78: USART1_TX nối DMA1 Channel 4
 *   13.4.1  DMA_ISR: mỗi channel 4 cờ GIF, TCIF, HTIF, TEIF
 *   13.4.2  DMA_IFCR: ghi 1 để xóa cờ tương ứng
 *   13.4.3  DMA_CCRx; 13.4.4 CNDTR chỉ ghi được khi channel tắt
 *   27.3.13 Transmission using DMA: DMAT trong USART_CR3, xóa TC trước khi bật channel
 */
#include "stm32f1xx.h"
#include "dma.h"

#define PRIO_DMA1_CH4   1U       /* mức ưu tiên NVIC (0..15) */

volatile uint8_t  g_dma_tx_busy;
volatile uint32_t g_dma_tx_done;
volatile uint32_t g_dma_tx_error;

/* ---------------- Channel 4: USART1_TX ---------------------------------- */

void dma1_ch4_usart1_tx_init(void)
{
    /* 1. Clock DMA1 nằm trên AHB (RCC_AHBENR bit 0) */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    (void)RCC->AHBENR;

    /* 2. Tắt channel trước khi cấu hình: CPAR, CMAR, CNDTR không ghi được khi EN = 1 */
    DMA1_Channel4->CCR = 0U;

    /* 3. Đích cố định là thanh ghi dữ liệu của USART1 */
    DMA1_Channel4->CPAR = (uint32_t)&USART1->DR;

    /* 4. DIR = 1 (đọc từ bộ nhớ), MINC = 1, PINC = 0, PSIZE = MSIZE = 8 bit,
     *    PL = 01 (medium), TCIE và TEIE bật; CIRC = 0, MEM2MEM = 0 */
    DMA1_Channel4->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PL_0
                       | DMA_CCR_TCIE | DMA_CCR_TEIE;

    /* 5. Xóa mọi cờ cũ của channel 4: ghi 1 vào CGIF4 xóa cả GIF, TCIF, HTIF, TEIF */
    DMA1->IFCR = DMA_IFCR_CGIF4;

    /* 6. USART1 phát yêu cầu DMA mỗi khi TXE = 1 */
    USART1->CR3 |= USART_CR3_DMAT;

    /* 7. NVIC: DMA1_Channel4_IRQn = 14 -> byte IPR 14 (4 bit cao), bit 14 của ISER0 */
    NVIC->IPR[DMA1_Channel4_IRQn] = (uint8_t)(PRIO_DMA1_CH4 << (8U - __NVIC_PRIO_BITS));
    NVIC->ISER[0] = 1UL << (uint32_t)DMA1_Channel4_IRQn;
}

int dma1_usart1_send(const uint8_t *buf, uint16_t len)
{
    if ((g_dma_tx_busy != 0U) || (len == 0U)) {
        return -1;
    }
    g_dma_tx_busy = 1U;

    DMA1_Channel4->CCR  &= ~DMA_CCR_EN;              /* channel phải tắt mới nạp lại được */
    DMA1_Channel4->CMAR  = (uint32_t)buf;            /* nguồn trong RAM hoặc flash */
    DMA1_Channel4->CNDTR = len;                      /* số byte, giảm sau mỗi lần chuyển */
    DMA1->IFCR           = DMA_IFCR_CGIF4;

    USART1->SR = ~USART_SR_TC;                       /* RM0008 27.3.13 bước 6: TC rc_w0, ghi 0 để xóa */
    DMA1_Channel4->CCR  |= DMA_CCR_EN;               /* bắt đầu: yêu cầu đầu tiên đến ngay vì TXE = 1 */
    return 0;
}

void DMA1_Channel4_IRQHandler(void)
{
    uint32_t isr = DMA1->ISR;

    if ((isr & DMA_ISR_TEIF4) != 0U) {               /* lỗi bus: phần cứng tự xóa EN */
        DMA1->IFCR = DMA_IFCR_CGIF4;
        g_dma_tx_error++;
        g_dma_tx_busy = 0U;
    } else if ((isr & DMA_ISR_TCIF4) != 0U) {
        DMA1->IFCR = DMA_IFCR_CGIF4;                 /* rc_w1 qua IFCR: chỉ xóa cờ channel 4 */
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;           /* CNDTR = 0, tắt channel để lần sau nạp lại */
        g_dma_tx_done++;
        g_dma_tx_busy = 0U;                          /* byte cuối có thể vẫn đang trong shift register */
    }
}

/* ---------------- Channel 1: memory-to-memory --------------------------- */

int dma1_ch1_memcpy32(const uint32_t *src, uint32_t *dst, uint16_t n)
{
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    (void)RCC->AHBENR;

    DMA1_Channel1->CCR   = 0U;
    DMA1_Channel1->CPAR  = (uint32_t)src;            /* phía "peripheral" là mảng nguồn */
    DMA1_Channel1->CMAR  = (uint32_t)dst;            /* phía memory là mảng đích */
    DMA1_Channel1->CNDTR = n;
    DMA1->IFCR           = DMA_IFCR_CGIF1;

    /* MEM2MEM = 1: không cần yêu cầu từ ngoại vi, chạy ngay khi EN = 1.
     * DIR = 0: đọc từ CPAR, ghi vào CMAR. PINC = MINC = 1, PSIZE = MSIZE = 32 bit. */
    DMA1_Channel1->CCR = DMA_CCR_MEM2MEM | DMA_CCR_PINC | DMA_CCR_MINC
                       | DMA_CCR_PSIZE_1 | DMA_CCR_MSIZE_1;
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    while ((DMA1->ISR & (DMA_ISR_TCIF1 | DMA_ISR_TEIF1)) == 0U) {
        /* chờ CNDTR về 0 */
    }
    int rc = ((DMA1->ISR & DMA_ISR_TEIF1) != 0U) ? -1 : 0;
    DMA1->IFCR = DMA_IFCR_CGIF1;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    return rc;
}
