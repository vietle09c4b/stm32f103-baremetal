/* IoHwAb_Cfg.h - thuộc tính của các ECU signal (trong dự án thật do công cụ cấu hình sinh ra).
 * Mỗi tín hiệu input có thuộc tính Filtering/Debounce (SWS_IoHwAb_00019). */
#ifndef IOHWAB_CFG_H
#define IOHWAB_CFG_H

#include "Dio_Cfg.h"

/* ECU signal "Button": nút nối PA0 xuống GND, nhấn = mức thấp */
#define IOHWAB_BUTTON_CHANNEL        DioConf_DioChannel_Button
#define IOHWAB_BUTTON_ACTIVE_LEVEL   STD_LOW
#define IOHWAB_BUTTON_SAMPLE_MS      5u      /* chu kỳ gọi IoHwAb_MainFunction_Button */
#define IOHWAB_BUTTON_DEBOUNCE_MS    20u     /* mức thô phải giữ nguyên ít nhất 20 ms */
/* Số mẫu liên tiếp bằng nhau: mẫu đầu đánh dấu lúc mức đổi, thêm 20 / 5 = 4 khoảng nữa -> 5 mẫu,
 * tức khoảng cách giữa mẫu đầu và mẫu cuối đúng bằng 20 ms */
#define IOHWAB_BUTTON_DEBOUNCE_CNT   ((IOHWAB_BUTTON_DEBOUNCE_MS / IOHWAB_BUTTON_SAMPLE_MS) + 1u)

/* ECU signal "Led": LED nối kiểu sink trên PC13, bật = mức thấp */
#define IOHWAB_LED_CHANNEL           DioConf_DioChannel_Led
#define IOHWAB_LED_ACTIVE_LEVEL      STD_LOW

#endif /* IOHWAB_CFG_H */
