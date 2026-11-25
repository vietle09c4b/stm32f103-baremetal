/* CanIf_Cfg.c - bảng PDU của CanIf (trong dự án thật sinh từ ECU extract / DBC). */
#include "CanIf_Cfg.h"
#include "Can_Cfg.h"
#include "LSduR_Cfg.h"

static const CanIf_TxPduCfgType CanIf_TxPduCfg[] = {
    { 0x123u, 8u, CanConf_CanHardwareObject_Tx0, LSduRConf_TxPdu_CanLedStatus },
};

static const CanIf_RxPduCfgType CanIf_RxPduCfg[] = {
    { 0x321u, CanConf_CanHardwareObject_Rx0, 1u, LSduRConf_RxPdu_LedCmd },
};

const CanIf_ConfigType CanIf_Config = {
    .TxPdus    = CanIf_TxPduCfg,
    .NumTxPdus = (uint8)(sizeof(CanIf_TxPduCfg) / sizeof(CanIf_TxPduCfg[0])),
    .RxPdus    = CanIf_RxPduCfg,
    .NumRxPdus = (uint8)(sizeof(CanIf_RxPduCfg) / sizeof(CanIf_RxPduCfg[0]))
};
