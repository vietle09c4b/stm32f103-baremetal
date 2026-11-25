/* CanIf_Cbk.h - callback CanIf cung cấp cho Can Driver:
 * CanIf_RxIndication (SWS_CANIF_00006, SID 0x14), CanIf_TxConfirmation (SWS_CANIF_00007, SID 0x13),
 * CanIf_ControllerModeIndication. */
#ifndef CANIF_CBK_H
#define CANIF_CBK_H

#include "Can_GeneralTypes.h"

void CanIf_RxIndication(const Can_HwType *Mailbox, const PduInfoType *PduInfoPtr);
void CanIf_TxConfirmation(PduIdType CanTxPduId);
void CanIf_ControllerModeIndication(uint8 ControllerId, Can_ControllerStateType ControllerMode);

#endif /* CANIF_CBK_H */
