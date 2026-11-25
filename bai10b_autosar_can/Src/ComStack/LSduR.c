/* LSduR.c - Linklayer SDU Router, định tuyến 1:1 giữa PduR và CanIf.
 * Từ R24-11, LSduR là tầng trên bắt buộc của mọi communication interface (CanIf, LinIf, FrIf, EthIf)
 * và là tầng dưới bắt buộc của PduR, Nm, Tp. Với R23-11 trở về trước, PduR gọi thẳng CanIf_Transmit.
 * Trong ECU nhỏ, SWS cho phép gộp PduR và LSduR thành một module; ví dụ này giữ riêng để thấy rõ tầng. */
#include "LSduR.h"
#include "CanIf.h"
#include "PduR_LSduR.h"
#include "PduR_Cfg.h"

typedef struct {
    PduIdType CanIfTxPduId;    /* đích phía dưới */
    PduIdType PduRTxPduId;     /* nguồn phía trên, dùng khi báo TxConfirmation */
} LSduR_TxPathType;

/* Bảng định tuyến (LSduRRoutingPath) - trong dự án thật do công cụ cấu hình sinh ra */
static const LSduR_TxPathType LSduR_TxPath[] = {
    { CanIfConf_CanIfTxPduCfg_CanLedStatus, PduRConf_PduRDestPdu_CanLedStatus },
};
static const PduIdType LSduR_RxPath[] = {
    PduRConf_PduRSrcPdu_LedCmd,                 /* LSduR Rx 0 -> PduR Rx 0 */
};

#define LSDUR_NUM_TX  (sizeof(LSduR_TxPath) / sizeof(LSduR_TxPath[0]))
#define LSDUR_NUM_RX  (sizeof(LSduR_RxPath) / sizeof(LSduR_RxPath[0]))

void LSduR_Init(void)
{
}

Std_ReturnType LSduR_PduRTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    if (TxPduId >= LSDUR_NUM_TX) {
        return E_NOT_OK;
    }
    return CanIf_Transmit(LSduR_TxPath[TxPduId].CanIfTxPduId, PduInfoPtr);
}

void LSduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (RxPduId < LSDUR_NUM_RX) {
        PduR_LSduRRxIndication(LSduR_RxPath[RxPduId], PduInfoPtr);
    }
}

void LSduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId < LSDUR_NUM_TX) {
        PduR_LSduRTxConfirmation(LSduR_TxPath[TxPduId].PduRTxPduId, result);
    }
}
