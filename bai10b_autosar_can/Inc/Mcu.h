/* Mcu.h - MCU Driver (MCAL). API theo AUTOSAR CP SWS MCU Driver R25-11:
 * Mcu_Init (SWS_Mcu_00153), Mcu_InitClock (SWS_Mcu_00155). */
#ifndef MCU_H
#define MCU_H

#include "Std_Types.h"
#include "Mcu_Cfg.h"

void           Mcu_Init(const Mcu_ConfigType *ConfigPtr);
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);

#endif /* MCU_H */
