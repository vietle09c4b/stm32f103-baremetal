/* Com_Cbk.h - callback Com cung cấp cho PduR:
 * Com_RxIndication (SWS_Com_00123, SID 0x42), Com_TxConfirmation (SWS_Com_00124, SID 0x40). */
#ifndef COM_CBK_H
#define COM_CBK_H

#include "ComStack_Types.h"

void Com_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#endif /* COM_CBK_H */
