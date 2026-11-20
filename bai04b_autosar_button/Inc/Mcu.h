/* Mcu.h - MCU Driver (MCAL). API theo AUTOSAR CP SWS MCU Driver R25-11, SWS_Mcu_00153. */
#ifndef MCU_H
#define MCU_H

#include "Std_Types.h"
#include "Mcu_Cfg.h"

void Mcu_Init(const Mcu_ConfigType *ConfigPtr);

#endif /* MCU_H */
