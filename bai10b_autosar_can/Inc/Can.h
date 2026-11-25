/* Can.h - CAN Driver (MCAL). API theo AUTOSAR CP SWS CAN Driver R25-11:
 * Can_Init (SWS_Can_00223), Can_SetControllerMode (SID 0x03), Can_Write (SWS_Can_00233, SID 0x06),
 * Can_MainFunction_Write, Can_MainFunction_Mode (scheduled functions, chế độ polling). */
#ifndef CAN_H
#define CAN_H

#include "Can_GeneralTypes.h"
#include "Can_Cfg.h"

void           Can_Init(const Can_ConfigType *Config);
Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo);
void           Can_MainFunction_Write(void);
void           Can_MainFunction_Mode(void);

/* Biến quan sát khi debug (không thuộc SWS) */
extern volatile uint32 Can_Esr;            /* bản sao CAN_ESR: TEC, REC, LEC, BOFF, EPVF, EWGF */
extern volatile uint32 Can_RxIsrCount;
extern volatile uint32 Can_TxConfCount;

#endif /* CAN_H */
