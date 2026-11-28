/* Port_Cfg.c - dữ liệu cấu hình Port Driver (trong dự án thật do công cụ cấu hình sinh ra).
 * Chân không có trong bảng giữ trạng thái reset; PA13, PA14 (SWD) không được đụng tới. */
#include "Port_Cfg.h"

static const Port_PinConfigType Port_PinConfig[] = {
    /* Nút nhấn nối PA0 xuống GND: input có điện trở kéo lên nội */
    { PortConf_PortPin_Button, PORT_PIN_IN,  PORT_MODE_IN_PULL,     STD_LOW,  PORT_PULL_UP   },
    /* LED người dùng: output push-pull 2 MHz, mức ban đầu STD_HIGH = LED tắt (LED nối kiểu sink) */
    { PortConf_PortPin_Led,    PORT_PIN_OUT, PORT_MODE_OUT_PP_2MHZ, STD_HIGH, PORT_PULL_NONE },
};

const Port_ConfigType Port_Config = {
    .Pins    = Port_PinConfig,
    .NumPins = (uint8)(sizeof(Port_PinConfig) / sizeof(Port_PinConfig[0]))
};
