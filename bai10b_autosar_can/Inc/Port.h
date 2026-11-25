/* Port.h - Port Driver (MCAL). API theo AUTOSAR CP SWS Port Driver R25-11, SWS_Port_00140. */
#ifndef PORT_H
#define PORT_H

#include "Std_Types.h"
#include "Port_Cfg.h"

void Port_Init(const Port_ConfigType *ConfigPtr);

#endif /* PORT_H */
