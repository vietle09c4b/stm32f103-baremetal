/* Mcu_Cfg.h - cấu hình MCU Driver (trong dự án thật do công cụ cấu hình sinh ra). */
#ifndef MCU_CFG_H
#define MCU_CFG_H

#include "Std_Types.h"

typedef struct {
    uint32  Apb2ClockEnable;    /* các bit RCC_APB2ENR cần bật cho ngoại vi đang dùng */
    boolean EnableCycleCounter; /* bật DWT_CYCCNT */
} Mcu_ConfigType;

extern const Mcu_ConfigType Mcu_Config;

#endif /* MCU_CFG_H */
