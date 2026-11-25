/* Can_Cfg.c - dữ liệu cấu hình Can Driver. */
#include "Can_Cfg.h"

const Can_ConfigType Can_Config = {
    .Btr           = CAN_BTR_500K_PCLK8M,
    .RxStdId       = { 0x321u, 0x321u },     /* chỉ nhận lệnh LED 0x321 */
    .RxIrqPriority = 2u
};
