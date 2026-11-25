/* Mcu_Cfg.c - dữ liệu cấu hình MCU Driver. */
#include "Mcu_Cfg.h"
#include "Stm32f103_Reg.h"

const Mcu_ConfigType Mcu_Config = {
    .Apb2ClockEnable = RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN,
    .Apb1ClockEnable = RCC_APB1ENR_CAN1EN
};
