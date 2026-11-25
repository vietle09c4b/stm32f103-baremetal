/* PduR_Cfg.h - ID của PDU trong không gian ID của PduR (PduRSrcPdu / PduRDestPdu). */
#ifndef PDUR_CFG_H
#define PDUR_CFG_H

#include "ComStack_Types.h"

#define PduRConf_PduRSrcPdu_CanLedStatus    ((PduIdType)0u)   /* Com -> PduR (Tx) */
#define PduRConf_PduRDestPdu_CanLedStatus   ((PduIdType)0u)   /* LSduR báo TxConfirmation với ID này */
#define PduRConf_PduRSrcPdu_LedCmd          ((PduIdType)0u)   /* LSduR -> PduR (Rx) */

#endif /* PDUR_CFG_H */
