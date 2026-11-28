/* Fls_ac.c - flash access code của Fls (SWS_Fls_00137), mức thanh ghi FPEC, tái dùng từ bài 15.
 * PM0075 Rev 2: 2.3.2 unlock (KEY1 rồi KEY2), 2.3.3 ghi half-word (PG), 2.3.4 xóa trang (PER, AR, STRT),
 * 3.4 FLASH_SR (EOP, WRPRTERR, PGERR xóa bằng cách ghi 1).
 * ES096 Rev 15 2.2.11: BSY lên trễ một chu kỳ sau lệnh, nên bỏ một lần đọc SR trước vòng chờ.
 * Mã chạy từ chính bank Flash đang ghi/xóa nên CPU dừng lấy lệnh tới khi BSY = 0 (PM0075 1.2):
 * vòng chờ BSY ở đây chỉ kết thúc khi thao tác đã xong. SWS_Fls_00215 cho phép chờ như vậy khi cần. */
#include "Fls_Ac.h"
#include "Stm32f103_Reg.h"

#define FLS_AC_SR_ERR   (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)
#define FLS_AC_SR_CLR   (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR)

static void Fls_Ac_WaitNotBusy(void)
{
    (void)FLASH->SR;                                  /* ES096 2.2.11 */
    while ((FLASH->SR & FLASH_SR_BSY) != 0u) { }
}

uint8 Fls_Ac_ReadByte(uint32 PhysAddr)
{
    return *(volatile const uint8 *)PhysAddr;
}

uint32 Fls_Ac_GetCycles(void)
{
    return DWT->CYCCNT;
}

void Fls_Ac_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0u) {
        FLASH->KEYR = FPEC_KEY1;
        FLASH->KEYR = FPEC_KEY2;
    }
}

void Fls_Ac_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

uint32 Fls_Ac_ErasePage(uint32 PhysAddr)
{
    Fls_Ac_WaitNotBusy();
    FLASH->SR  = FLS_AC_SR_CLR;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR  = PhysAddr;
    FLASH->CR |= FLASH_CR_STRT;
    Fls_Ac_WaitNotBusy();
    FLASH->CR &= ~FLASH_CR_PER;
    return FLASH->SR & FLS_AC_SR_ERR;
}

uint32 Fls_Ac_ProgramHalf(uint32 PhysAddr, uint16 Data)
{
    Fls_Ac_WaitNotBusy();
    FLASH->SR  = FLS_AC_SR_CLR;
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16 *)PhysAddr = Data;              /* bắt buộc ghi 16 bit */
    Fls_Ac_WaitNotBusy();
    FLASH->CR &= ~FLASH_CR_PG;
    return FLASH->SR & FLS_AC_SR_ERR;
}
