/*
 * flash.h - Xóa trang và ghi half-word vào Flash nội STM32F103C8, mức thanh ghi
 * Tài liệu: PM0075 Rev 2 (STM32F10xxx Flash programming manual), RM0008 Rev 21 mục 3.3.3, ES096 Rev 15
 */
#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#define FLASH_PAGE_SIZE   1024U
#define FPEC_KEY1         0x45670123UL      /* PM0075 2.3.1 */
#define FPEC_KEY2         0xCDEF89ABUL

/* Giá trị trả về: 0 = thành công, hoặc tổ hợp cờ lỗi của FLASH_SR */
#define FLASH_OK          0U
#define FLASH_ERR_PG      (1U << 2)         /* PGERR */
#define FLASH_ERR_WRP     (1U << 4)         /* WRPRTERR */
#define FLASH_ERR_VERIFY  (1U << 8)         /* đọc lại không khớp */

extern volatile uint32_t flash_busy_cycles;

void     flash_unlock(void);
void     flash_lock(void);
uint32_t flash_erase_page(uint32_t page_addr);
uint32_t flash_program_half(uint32_t addr, uint16_t data);

#endif /* FLASH_H */
