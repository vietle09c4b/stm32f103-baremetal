/* PduR_LSduR.h - callback PduR cung cấp cho LSduR:
 * PduR_<User:Lo>RxIndication (SWS_PduR_00362, SID 0x42), PduR_<User:Lo>TxConfirmation (SWS_PduR_00365, SID 0x40). */
#ifndef PDUR_LSDUR_H
#define PDUR_LSDUR_H

#include "ComStack_Types.h"
#include "PduR_Cfg.h"

void PduR_Init(void);
void PduR_LSduRRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void PduR_LSduRTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#endif /* PDUR_LSDUR_H */
