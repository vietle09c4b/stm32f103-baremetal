/* IoHwAb.c - I/O Hardware Abstraction.
 * Biết phần cứng của ECU: LED nối kiểu sink trên PC13. Đảo mức là trách nhiệm của IoHwAb (SWS_Port_00082). */
#include "Dio.h"
#include "IoHwAb.h"

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
