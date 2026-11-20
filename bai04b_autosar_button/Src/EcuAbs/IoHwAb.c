/* IoHwAb.c - I/O Hardware Abstraction.
 * Biết phần cứng của ECU (board Blue Pill): nút nối PA0 xuống GND, LED nối kiểu sink trên PC13.
 * Đảo mức là trách nhiệm của IoHwAb, không phải Port hay Dio (SWS_Port_00082).
 * Chống dội theo thuộc tính Filtering/Debounce của tín hiệu input (SWS_IoHwAb_00019): một job processing
 * function tuần hoàn đọc Dio và lọc; GET chỉ trả giá trị đã lọc (giống use case "Multiplexed I/O" mục 7.6). */
#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
#include "Dio.h"

volatile uint32 IoHwAb_Button_RawChanges = 0u;
volatile uint8  IoHwAb_Button_RawLevel   = STD_HIGH;

static Dio_LevelType IoHwAb_Button_LastRaw  = STD_HIGH;   /* mức thô lần lấy mẫu trước */
static Dio_LevelType IoHwAb_Button_Filtered = STD_HIGH;   /* mức đã lọc */
static uint8         IoHwAb_Button_StableCnt = 0u;        /* số mẫu liên tiếp bằng LastRaw */

void IoHwAb_Init0(const IoHwAb0_ConfigType *ConfigPtr)
{
    (void)ConfigPtr;
    Dio_LevelType lvl = Dio_ReadChannel(IOHWAB_BUTTON_CHANNEL);   /* gọi sau Port_Init */
    IoHwAb_Button_LastRaw   = lvl;
    IoHwAb_Button_Filtered  = lvl;
    IoHwAb_Button_RawLevel  = lvl;
    IoHwAb_Button_StableCnt = (uint8)IOHWAB_BUTTON_DEBOUNCE_CNT;
    (void)IoHwAb_Led_Set(FALSE);                  /* trạng thái an toàn: LED tắt */
}

void IoHwAb_MainFunction_Button(void)
{
    Dio_LevelType raw = Dio_ReadChannel(IOHWAB_BUTTON_CHANNEL);
    IoHwAb_Button_RawLevel = raw;

    if (raw != IoHwAb_Button_LastRaw) {           /* mức thô đổi: bắt đầu đếm lại */
        IoHwAb_Button_LastRaw   = raw;
        IoHwAb_Button_StableCnt = 1u;
        IoHwAb_Button_RawChanges++;
    } else if (IoHwAb_Button_StableCnt < (uint8)IOHWAB_BUTTON_DEBOUNCE_CNT) {
        IoHwAb_Button_StableCnt++;
    } else {
        /* đã đủ số mẫu, giữ nguyên bộ đếm */
    }

    if ((IoHwAb_Button_StableCnt >= (uint8)IOHWAB_BUTTON_DEBOUNCE_CNT) &&
        (IoHwAb_Button_Filtered != IoHwAb_Button_LastRaw)) {
        IoHwAb_Button_Filtered = IoHwAb_Button_LastRaw;
    }
}

Std_ReturnType IoHwAb_Button_Get(boolean *Pressed)
{
    if (Pressed == NULL_PTR) {
        return E_NOT_OK;
    }
    *Pressed = (IoHwAb_Button_Filtered == (Dio_LevelType)IOHWAB_BUTTON_ACTIVE_LEVEL) ? TRUE : FALSE;
    return E_OK;
}

Std_ReturnType IoHwAb_Led_Set(boolean On)
{
    Dio_LevelType level = (On != FALSE) ? (Dio_LevelType)IOHWAB_LED_ACTIVE_LEVEL
                                        : (Dio_LevelType)(IOHWAB_LED_ACTIVE_LEVEL ^ 1u);
    Dio_WriteChannel(IOHWAB_LED_CHANNEL, level);
    return E_OK;
}
