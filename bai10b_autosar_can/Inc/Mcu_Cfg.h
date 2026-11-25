/* Mcu_Cfg.h - cấu hình MCU Driver (trong dự án thật do công cụ cấu hình sinh ra). */
#ifndef MCU_CFG_H
#define MCU_CFG_H

#include "Std_Types.h"

typedef uint8 Mcu_ClockType;

#define McuConf_McuClockSettingConfig_Hse8MHz  ((Mcu_ClockType)0u)   /* SYSCLK = HCLK = PCLK1 = HSE 8 MHz */

typedef struct {
    uint32 Apb2ClockEnable;   /* các bit RCC_APB2ENR: GPIOB, GPIOC, AFIO */
    uint32 Apb1ClockEnable;   /* các bit RCC_APB1ENR: CAN1 */
} Mcu_ConfigType;

extern const Mcu_ConfigType Mcu_Config;

#endif /* MCU_CFG_H */
