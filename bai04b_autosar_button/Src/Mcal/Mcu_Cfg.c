/* Mcu_Cfg.c - dữ liệu cấu hình MCU Driver. */
#include "Mcu_Cfg.h"
#include "Stm32f103_Reg.h"

const Mcu_ConfigType Mcu_Config = {
    .Apb2ClockEnable = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN   /* nút ở port A, LED ở port C */
};
