/* IoHwAb.c - I/O Hardware Abstraction.
 * Biết phần cứng của ECU (board Blue Pill): LED nối kiểu sink trên PC13, nên "bật" = mức thấp.
 * Việc đảo mức là trách nhiệm của IoHwAb, không phải Port hay Dio (SWS Port Driver R25-11, tr.18). */
#include "IoHwAb.h"
#include "Dio.h"

#define IOHWAB_LED_CHANNEL       DioConf_DioChannel_Led
#define IOHWAB_LED_ACTIVE_LEVEL  STD_LOW

void IoHwAb_Init0(const IoHwAb0_ConfigType *ConfigPtr)
{
    (void)ConfigPtr;
    (void)IoHwAb_Led_Set(FALSE);                  /* trạng thái an toàn: LED tắt */
}

Std_ReturnType IoHwAb_Led_Set(boolean On)
{
    Dio_LevelType level = (On != FALSE) ? (Dio_LevelType)IOHWAB_LED_ACTIVE_LEVEL
                                        : (Dio_LevelType)(IOHWAB_LED_ACTIVE_LEVEL ^ 1u);
    Dio_WriteChannel(IOHWAB_LED_CHANNEL, level);
    return E_OK;
}
