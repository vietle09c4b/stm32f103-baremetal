/* CanIf.h - CAN Interface (ECU Abstraction Layer). API theo AUTOSAR CP SWS CAN Interface R25-11:
 * CanIf_Init, CanIf_SetControllerMode, CanIf_Transmit (SWS_CANIF_00005, SID 0x49). */
#ifndef CANIF_H
#define CANIF_H

#include "Can_GeneralTypes.h"
#include "CanIf_Cfg.h"

void           CanIf_Init(const CanIf_ConfigType *ConfigPtr);
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, Can_ControllerStateType ControllerMode);
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

extern volatile Can_ControllerStateType CanIf_ControllerMode;   /* quan sát khi debug */

#endif /* CANIF_H */
