/* Port_Cfg.c - dữ liệu cấu hình Port Driver (trong dự án thật do công cụ cấu hình sinh ra).
 * Chân không có trong bảng giữ trạng thái reset (input thả nổi); PA13, PA14 (SWD) không được đụng tới. */
#include "Port_Cfg.h"

static const Port_PinConfigType Port_PinConfig[] = {
    /* LED người dùng trên Blue Pill: output push-pull 2 MHz (DS5319: PC13 tối đa 2 MHz),
     * mức ban đầu STD_HIGH = LED tắt vì LED nối kiểu sink */
    { PortConf_PortPin_Led, PORT_PIN_OUT, PORT_MODE_OUT_PP_2MHZ, STD_HIGH },
};

const Port_ConfigType Port_Config = {
    .Pins    = Port_PinConfig,
    .NumPins = (uint8)(sizeof(Port_PinConfig) / sizeof(Port_PinConfig[0]))
};
