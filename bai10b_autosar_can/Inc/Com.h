/* Com.h - AUTOSAR COM. API theo AUTOSAR CP SWS COM R25-11:
 * Com_SendSignal (SWS_Com_00197, SID 0x0a), Com_ReceiveSignal (SWS_Com_00198, SID 0x0b),
 * Com_MainFunctionTx (SWS_Com_00399, SID 0x19). */
#ifndef COM_H
#define COM_H

#include "ComStack_Types.h"
#include "Com_Cfg.h"

#define COM_SERVICE_NOT_AVAILABLE  ((uint8)0x80u)

void  Com_Init(const Com_ConfigType *config);
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void *SignalDataPtr);
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void *SignalDataPtr);
void  Com_MainFunctionTx(void);

/* Biến quan sát khi debug (không thuộc SWS) */
extern volatile uint32 Com_TxRequestCount;
extern volatile uint32 Com_TxConfirmCount;
extern volatile uint32 Com_RxIndicationCount;

#endif /* COM_H */
