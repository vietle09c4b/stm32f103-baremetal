/* LSduR.h - Linklayer SDU Router (AUTOSAR CP SWS LSduRouter R25-11, trạng thái DRAFT).
 * Giao diện phía trên (PduR):  LSduR_<User:Up>Transmit     -> LSduR_PduRTransmit
 * Giao diện phía dưới (CanIf): LSduR_<User:Lo>RxIndication -> LSduR_CanIfRxIndication (CP_SWS_LSduR_91011, SID 0x42)
 *                              LSduR_<User:Lo>TxConfirmation -> LSduR_CanIfTxConfirmation */
#ifndef LSDUR_H
#define LSDUR_H

#include "ComStack_Types.h"
#include "LSduR_Cfg.h"

void           LSduR_Init(void);
Std_ReturnType LSduR_PduRTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
void           LSduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void           LSduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#endif /* LSDUR_H */
