/* TestHook.h - lệnh thử nghiệm điều khiển bằng debugger (KHÔNG thuộc kiến trúc AUTOSAR).
 * Trong gdb: set var TestHook_Cmd = <lệnh>, rồi cho chạy tiếp.
 *   1 = T3: ghi 0x0000 đè half-word đầu của dữ liệu block BootCounter (giả lập lỗi bit), rồi reset để đọc lại
 *   2 = T4: ghi block BootCounter liên tiếp TESTHOOK_STRESS_WRITES lần (mỗi lần tăng bộ đếm) để Fee dọn rác
 *   3 = xóa toàn bộ 4 sector của Fls để lần khởi động sau bắt đầu từ Flash trống (T1) */
#ifndef TESTHOOK_H
#define TESTHOOK_H

#include "Std_Types.h"

#define TESTHOOK_STRESS_WRITES   400u

void TestHook_MainFunction(void);

extern volatile uint8  TestHook_Cmd;
extern volatile uint8  TestHook_Busy;
extern volatile uint32 TestHook_T3_Offset;       /* offset Fls của ô bị ghi đè */
extern volatile uint32 TestHook_T3_Err;          /* cờ lỗi FLASH_SR khi ghi 0x0000 */
extern volatile uint32 TestHook_T4_Done;         /* số lần ghi đã xong */
extern volatile uint32 TestHook_T4_Failed;       /* số lần ghi trả NVM_REQ_NOT_OK */
extern volatile uint32 TestHook_T4_MaxUs;        /* lần ghi lâu nhất (có dọn rác) */
extern volatile uint32 TestHook_T4_MinUs;        /* lần ghi nhanh nhất */
extern volatile uint32 TestHook_T4_GcBefore, TestHook_T4_GcAfter;
extern volatile uint32 TestHook_T4_TotalUs;
extern volatile uint8  TestHook_EraseResult;     /* kết quả Fls cho lệnh 3 */

#endif /* TESTHOOK_H */
