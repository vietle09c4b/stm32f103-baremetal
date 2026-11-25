/* PduR_Com.h - API PduR cung cấp cho Com: PduR_<User:Up>Transmit (SWS_PduR_00406, SID 0x49). */
#ifndef PDUR_COM_H
#define PDUR_COM_H

#include "ComStack_Types.h"
#include "PduR_Cfg.h"

Std_ReturnType PduR_ComTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

#endif /* PDUR_COM_H */
