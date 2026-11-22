/*
 * dma.h - DMA1 mức thanh ghi trên STM32F103 (Blue Pill)
 *
 *   Channel 4 : USART1_TX, bộ nhớ -> USART1_DR, ngắt transfer complete (IRQ 14)
 *   Channel 1 : memory-to-memory (MEM2MEM = 1), chờ cờ TCIF1 bằng polling
 *
 * Tài liệu: RM0008 Rev 21 chương 13 (DMA) và 27.3.13 (USART transmission using DMA).
 * STM32F103C8 (medium-density) chỉ có DMA1 với 7 channel; DMA2 chỉ có trên high-density, XL-density.
 */
#ifndef DMA_H
#define DMA_H

#include <stdint.h>

/* Cấu hình cố định DMA1 Channel 4 cho USART1_TX và bật ngắt trong NVIC */
void dma1_ch4_usart1_tx_init(void);

/* Bắt đầu gửi len byte (1..65535) từ buf qua USART1 bằng DMA.
 * Trả về 0 nếu đã bắt đầu, -1 nếu lần gửi trước chưa xong.
 * buf phải giữ nguyên nội dung cho tới khi g_dma_tx_busy = 0. */
int  dma1_usart1_send(const uint8_t *buf, uint16_t len);

/* Copy n word 32 bit từ src sang dst bằng DMA1 Channel 1 (memory-to-memory), chờ tới khi xong.
 * Trả về 0 nếu xong, -1 nếu có transfer error. */
int  dma1_ch1_memcpy32(const uint32_t *src, uint32_t *dst, uint16_t n);

extern volatile uint8_t  g_dma_tx_busy;    /* 1 trong lúc Channel 4 đang chuyển */
extern volatile uint32_t g_dma_tx_done;    /* số lần transfer complete của Channel 4 */
extern volatile uint32_t g_dma_tx_error;   /* số lần transfer error của Channel 4 */

#endif /* DMA_H */
