/* Com_Cfg.h - cấu hình Com: I-PDU và signal (ComIPdu, ComSignal), tương ứng một DBC hai message:
 *   0x123 CanLedStatus (Tx, DLC 8, chu kỳ 100 ms): AliveCounter = byte 0
 *   0x321 LedCmd       (Rx, DLC >= 1):              LedCmd = bit 0 của byte 0 */
#ifndef COM_CFG_H
#define COM_CFG_H

#include "ComStack_Types.h"

typedef uint16 Com_SignalIdType;

/* ID I-PDU (ComIPduHandleId) */
#define ComConf_ComIPdu_CanLedStatus    ((PduIdType)0u)
#define ComConf_ComIPdu_LedCmd          ((PduIdType)1u)

/* ID signal (ComHandleId) */
#define ComConf_ComSignal_AliveCounter  ((Com_SignalIdType)0u)
#define ComConf_ComSignal_LedCmd        ((Com_SignalIdType)1u)

#define COM_MAIN_FUNCTION_TX_PERIOD_MS  10u      /* chu kỳ gọi Com_MainFunctionTx */
#define COM_MAX_IPDUS                   4u

typedef enum { COM_SEND = 0u, COM_RECEIVE = 1u } Com_IPduDirectionType;

typedef struct {
    PduLengthType          Length;
    Com_IPduDirectionType  Direction;
    uint8                 *Buffer;          /* bộ đệm I-PDU của Com */
    const uint8           *InitData;        /* giá trị ban đầu (NULL_PTR: toàn 0) */
    uint16                 TxPeriodMs;      /* ComTxModeTimePeriod, chỉ dùng với Tx PERIODIC */
    PduIdType              PduRPduId;       /* ID gửi cho PduR_ComTransmit */
} Com_IPduCfgType;

typedef struct {
    PduIdType IPdu;
    uint16    BitPosition;                  /* little endian, không vượt ranh giới byte trong ví dụ */
    uint8     BitSize;                      /* 1..8 */
} Com_SignalCfgType;

typedef struct {
    const Com_IPduCfgType   *IPdus;
    uint8                    NumIPdus;
    const Com_SignalCfgType *Signals;
    uint8                    NumSignals;
} Com_ConfigType;

extern const Com_ConfigType Com_Config;

#endif /* COM_CFG_H */
