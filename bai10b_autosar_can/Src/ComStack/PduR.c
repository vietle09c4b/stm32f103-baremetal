/* PduR.c - PDU Router.
 * Định tuyến I-PDU theo ID tĩnh, không xét nội dung (SWS PduR R25-11, chương 1).
 * Ví dụ chỉ có đường Com <-> LSduR; ECU thật còn có gateway (CAN -> CAN), Dcm, CanTp, IpduM. */
#include "PduR_Com.h"
#include "PduR_LSduR.h"
#include "LSduR.h"
#include "Com_Cbk.h"
#include "Com_Cfg.h"

typedef struct {
    PduIdType LSduRTxPduId;    /* đích phía dưới */
    PduIdType ComTxPduId;      /* nguồn phía trên, nhận TxConfirmation */
} PduR_TxRouteType;

/* Bảng định tuyến (PduRRoutingPath) - trong dự án thật do công cụ cấu hình sinh ra */
static const PduR_TxRouteType PduR_TxRoute[] = {
    { LSduRConf_TxPdu_CanLedStatus, ComConf_ComIPdu_CanLedStatus },
};
static const PduIdType PduR_RxRoute[] = {
    ComConf_ComIPdu_LedCmd,                      /* PduR Rx 0 -> Com I-PDU LedCmd */
};

#define PDUR_NUM_TX  (sizeof(PduR_TxRoute) / sizeof(PduR_TxRoute[0]))
#define PDUR_NUM_RX  (sizeof(PduR_RxRoute) / sizeof(PduR_RxRoute[0]))

void PduR_Init(void)
{
}

Std_ReturnType PduR_ComTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    if (TxPduId >= PDUR_NUM_TX) {
        return E_NOT_OK;
    }
    return LSduR_PduRTransmit(PduR_TxRoute[TxPduId].LSduRTxPduId, PduInfoPtr);
}

void PduR_LSduRRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (RxPduId < PDUR_NUM_RX) {
        Com_RxIndication(PduR_RxRoute[RxPduId], PduInfoPtr);
    }
}

void PduR_LSduRTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId < PDUR_NUM_TX) {
        Com_TxConfirmation(PduR_TxRoute[TxPduId].ComTxPduId, result);
    }
}
