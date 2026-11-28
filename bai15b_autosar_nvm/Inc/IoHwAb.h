/* IoHwAb.h - I/O Hardware Abstraction (ECU Abstraction Layer).
 * SWS IoHwAb R25-11 không chuẩn hóa C-API (tr.8): tên GET/SET do ECU tự đặt (SWS_IoHwAb_00069, 00070).
 * IoHwAb_Init0 theo SWS_IoHwAb_00119; job processing function do BSW Scheduler gọi tuần hoàn (SWS_IoHwAb_00035). */
#ifndef IOHWAB_H
#define IOHWAB_H

#include "Std_Types.h"

typedef uint8 IoHwAb0_ConfigType;                         /* chưa dùng, ConfigPtr = NULL_PTR (SWS_IoHwAb_00158) */

void           IoHwAb_Init0(const IoHwAb0_ConfigType *ConfigPtr);
Std_ReturnType IoHwAb_Led_Set(boolean On);                /* SET operation, ECU signal "Led" */
Std_ReturnType IoHwAb_Button_Get(boolean *Pressed);       /* GET operation, ECU signal "Button" */
void           IoHwAb_MainFunction_Button(void);          /* scheduled function, chu kỳ IOHWAB_BUTTON_SAMPLE_MS */

/* Biến quan sát khi debug: số lần mức thô đổi (kể cả dội) và mức thô gần nhất */
extern volatile uint32 IoHwAb_Button_RawChanges;
extern volatile uint8  IoHwAb_Button_RawLevel;

#endif /* IOHWAB_H */
