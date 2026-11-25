/* IoHwAb.h - I/O Hardware Abstraction (ECU Abstraction Layer).
 * SWS IoHwAb R25-11 không chuẩn hóa C-API: tên SET do ECU tự đặt (SWS_IoHwAb_00070); IoHwAb_Init0 theo SWS_IoHwAb_00119. */
#ifndef IOHWAB_H
#define IOHWAB_H

#include "Std_Types.h"

typedef uint8 IoHwAb0_ConfigType;

#define IOHWAB_LED_CHANNEL       DioConf_DioChannel_Led
#define IOHWAB_LED_ACTIVE_LEVEL  STD_LOW          /* LED PC13 nối kiểu sink: mức thấp = sáng */

void           IoHwAb_Init0(const IoHwAb0_ConfigType *ConfigPtr);
Std_ReturnType IoHwAb_Led_Set(boolean On);        /* SET operation, ECU signal "Led" */

#endif /* IOHWAB_H */
