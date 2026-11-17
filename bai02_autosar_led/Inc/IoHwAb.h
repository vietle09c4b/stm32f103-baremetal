/* IoHwAb.h - I/O Hardware Abstraction (ECU Abstraction Layer).
 * SWS IoHwAb R25-11 không chuẩn hóa C-API (tr.8); tên hàm Set dưới đây do ECU tự định nghĩa (SWS_IoHwAb_00070). */
#ifndef IOHWAB_H
#define IOHWAB_H

#include "Std_Types.h"

typedef uint8 IoHwAb0_ConfigType;                         /* chưa dùng, ConfigPtr = NULL_PTR (SWS_IoHwAb_00158) */

void           IoHwAb_Init0(const IoHwAb0_ConfigType *ConfigPtr);   /* SWS_IoHwAb_00119 */
Std_ReturnType IoHwAb_Led_Set(boolean On);                /* SET operation cho ECU signal "Led" */

#endif /* IOHWAB_H */
