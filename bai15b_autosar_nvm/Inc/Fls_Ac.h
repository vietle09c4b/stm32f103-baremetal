/* Fls_Ac.h - flash access code của Fls (SWS_Fls_00137: đặt trong module C riêng Fls_ac.c).
 * Chỉ Fls.c được gọi các hàm này; riêng phép thử T3 (TestHook.c) gọi Fls_Ac_ProgramHalf để
 * cố ý làm hỏng một ô dữ liệu, mô phỏng lỗi bit nằm ngoài kiểm soát của memory stack. */
#ifndef FLS_AC_H
#define FLS_AC_H

#include "Std_Types.h"

void   Fls_Ac_Unlock(void);
void   Fls_Ac_Lock(void);
uint32 Fls_Ac_ErasePage(uint32 PhysAddr);                 /* trả về cờ lỗi FLASH_SR (0 = thành công) */
uint32 Fls_Ac_ProgramHalf(uint32 PhysAddr, uint16 Data);  /* trả về cờ lỗi FLASH_SR (0 = thành công) */
uint8  Fls_Ac_ReadByte(uint32 PhysAddr);                  /* đọc một byte trong vùng Flash */
uint32 Fls_Ac_GetCycles(void);                            /* bộ đếm chu kỳ DWT, dùng để đo thời gian */

#endif /* FLS_AC_H */
