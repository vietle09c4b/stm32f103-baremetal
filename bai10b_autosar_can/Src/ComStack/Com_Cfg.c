/* Com_Cfg.c - dữ liệu cấu hình Com (trong dự án thật sinh từ DBC/ARXML). */
#include "Com_Cfg.h"
#include "PduR_Cfg.h"

static uint8 Com_Buf_CanLedStatus[8];
static uint8 Com_Buf_LedCmd[8];

/* byte 1..7 giữ giá trị cố định 0x11..0x77 để dễ nhận ra frame trên PCAN-View */
static const uint8 Com_Init_CanLedStatus[8] = { 0x00u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u };

static const Com_IPduCfgType Com_IPduCfg[] = {
    /* Length, Direction,   Buffer,               InitData,              TxPeriodMs, PduRPduId */
    { 8u, COM_SEND,    Com_Buf_CanLedStatus, Com_Init_CanLedStatus, 100u, PduRConf_PduRSrcPdu_CanLedStatus },
    { 8u, COM_RECEIVE, Com_Buf_LedCmd,       NULL_PTR,              0u,   0u },
};

static const Com_SignalCfgType Com_SignalCfg[] = {
    /* IPdu,                        BitPosition, BitSize */
    { ComConf_ComIPdu_CanLedStatus, 0u,          8u },   /* AliveCounter */
    { ComConf_ComIPdu_LedCmd,       0u,          1u },   /* LedCmd */
};

const Com_ConfigType Com_Config = {
    .IPdus      = Com_IPduCfg,
    .NumIPdus   = (uint8)(sizeof(Com_IPduCfg) / sizeof(Com_IPduCfg[0])),
    .Signals    = Com_SignalCfg,
    .NumSignals = (uint8)(sizeof(Com_SignalCfg) / sizeof(Com_SignalCfg[0]))
};
