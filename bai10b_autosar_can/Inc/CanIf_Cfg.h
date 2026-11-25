/* CanIf_Cfg.h - cấu hình CanIf: bảng Tx L-PDU và Rx L-PDU (CanIfTxPduCfg, CanIfRxPduCfg). */
#ifndef CANIF_CFG_H
#define CANIF_CFG_H

#include "Can_GeneralTypes.h"

/* ID của PDU trong không gian ID của CanIf */
#define CanIfConf_CanIfTxPduCfg_CanLedStatus   ((PduIdType)0u)   /* 0x123, DLC 8 */
#define CanIfConf_CanIfRxPduCfg_LedCmd         ((PduIdType)0u)   /* 0x321 */

typedef struct {
    Can_IdType       CanId;
    uint8            Dlc;
    Can_HwHandleType Hth;
    PduIdType        UpperPduId;   /* ID của PDU ở tầng trên (LSduR) dùng cho TxConfirmation */
} CanIf_TxPduCfgType;

typedef struct {
    Can_IdType       CanId;
    Can_HwHandleType Hrh;
    uint8            MinDlc;       /* DLC check (CanIfRxPduDataLength) */
    PduIdType        UpperPduId;   /* ID của PDU ở tầng trên (LSduR) dùng cho RxIndication */
} CanIf_RxPduCfgType;

typedef struct {
    const CanIf_TxPduCfgType *TxPdus;
    uint8                     NumTxPdus;
    const CanIf_RxPduCfgType *RxPdus;
    uint8                     NumRxPdus;
} CanIf_ConfigType;

extern const CanIf_ConfigType CanIf_Config;

#endif /* CANIF_CFG_H */
