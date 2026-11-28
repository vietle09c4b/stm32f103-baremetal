/*
 * flash.c - Xóa trang và ghi half-word vào Flash nội STM32F103C8, mức thanh ghi
 *
 * PM0075 Rev 2:
 *   2.3.2  Mở khóa FPEC: ghi KEY1 rồi KEY2 vào FLASH_KEYR; sai trình tự -> khóa tới reset + bus error
 *   2.3.3  Ghi 16 bit: PG = 1, ghi half-word, chờ BSY = 0, đọc lại; ô khác 0xFFFF -> PGERR
 *   2.3.4  Xóa trang: PER = 1, FLASH_AR = địa chỉ trong trang, STRT = 1, chờ BSY = 0
 *   3.4    FLASH_SR: EOP, WRPRTERR, PGERR xóa bằng cách ghi 1
 * ES096 Rev 15 2.2.11: BSY lên trễ một chu kỳ sau STRT -> không đọc BSY ngay sau khi ghi CR.
 * RM0008 3.3.3: ghi/xóa Flash cần HSI đang bật.
 */
#include "stm32f1xx.h"
#include "flash.h"

#define SR_ERR_MASK   (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)
#define SR_CLR_MASK   (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR)

volatile uint32_t flash_busy_cycles;   /* số chu kỳ CPU từ lúc ra lệnh tới khi BSY = 0 (đo bằng DWT) */

void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FPEC_KEY1;
        FLASH->KEYR = FPEC_KEY2;
    }
}

void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void wait_not_busy(void)
{
    (void)FLASH->SR;                                  /* ES096 2.2.11: bỏ lần đọc ngay sau STRT/ghi */
    while (FLASH->SR & FLASH_SR_BSY) { }
}

uint32_t flash_erase_page(uint32_t page_addr)
{
    wait_not_busy();
    FLASH->SR = SR_CLR_MASK;                          /* xóa cờ cũ (ghi 1) */
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR  = page_addr;
    uint32_t c0 = DWT->CYCCNT;
    FLASH->CR |= FLASH_CR_STRT;
    wait_not_busy();                                  /* CPU dừng lấy lệnh tới khi xóa xong */
    flash_busy_cycles = DWT->CYCCNT - c0;
    FLASH->CR &= ~FLASH_CR_PER;

    uint32_t err = FLASH->SR & SR_ERR_MASK;
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE / 2U && err == 0U; i++) {
        if (((volatile uint16_t *)page_addr)[i] != 0xFFFFU) err |= FLASH_ERR_VERIFY;
    }
    return err;
}

uint32_t flash_program_half(uint32_t addr, uint16_t data)
{
    wait_not_busy();
    FLASH->SR = SR_CLR_MASK;
    FLASH->CR |= FLASH_CR_PG;
    uint32_t c0 = DWT->CYCCNT;
    *(volatile uint16_t *)addr = data;                /* bắt buộc ghi 16 bit, ghi 8 bit -> bus error */
    wait_not_busy();
    flash_busy_cycles = DWT->CYCCNT - c0;
    FLASH->CR &= ~FLASH_CR_PG;

    uint32_t err = FLASH->SR & SR_ERR_MASK;
    if (err == 0U && *(volatile uint16_t *)addr != data) err |= FLASH_ERR_VERIFY;
    return err;
}
