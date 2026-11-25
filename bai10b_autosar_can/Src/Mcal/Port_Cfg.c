/* Port_Cfg.c - dữ liệu cấu hình Port Driver (trong dự án thật do công cụ cấu hình sinh ra).
 * PB8 là chân FT (chịu 5 V, DS5319 Table 5) nên nhận trực tiếp RXD của transceiver 5 V. */
#include "Port_Cfg.h"
#include "Stm32f103_Reg.h"

static const Port_PinConfigType Port_PinConfig[] = {
    /* CAN_RX: input pull-up (RM0008 Table 28 cho phép floating hoặc pull-up) */
    { PortConf_PortPin_CanRx, PORT_PIN_IN,  PORT_MODE_IN_PULL,     STD_LOW,  PORT_PULL_UP   },
    /* CAN_TX: alternate function push-pull */
    { PortConf_PortPin_CanTx, PORT_PIN_OUT, PORT_MODE_AF_PP_50MHZ, STD_HIGH, PORT_PULL_NONE },
    /* LED người dùng: output push-pull 2 MHz, mức ban đầu STD_HIGH = LED tắt (LED nối kiểu sink) */
    { PortConf_PortPin_Led,   PORT_PIN_OUT, PORT_MODE_OUT_PP_2MHZ, STD_HIGH, PORT_PULL_NONE },
};

const Port_ConfigType Port_Config = {
    .Pins          = Port_PinConfig,
    .NumPins       = (uint8)(sizeof(Port_PinConfig) / sizeof(Port_PinConfig[0])),
    .AfioMaprMask  = AFIO_MAPR_CAN_REMAP_MASK,
    .AfioMaprValue = AFIO_MAPR_CAN_REMAP_PB8
};
