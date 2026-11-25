/* Com.c - AUTOSAR COM (rút gọn).
 * Chuyển signal <-> I-PDU: Com_SendSignal ghi signal vào bộ đệm I-PDU, Com_MainFunctionTx gửi I-PDU
 * theo chu kỳ (transmission mode PERIODIC) qua PduR_ComTransmit. Phía nhận, Com_RxIndication (có thể
 * chạy trong ngắt) chép I-PDU vào bộ đệm; Com_ReceiveSignal tách signal ra.
 * Rút gọn so với SWS: I-PDU group được coi là đã start sau Com_Init; không có update bit, filter,
 * deadline monitoring, signal group. Signal 1..8 bit không vượt ranh giới byte nên đọc/ghi một byte
 * là nguyên tử; signal lớn hơn cần exclusive area (SchM_Enter_Com/SchM_Exit_Com). */
#include "Com.h"
#include "Com_Cbk.h"
#include "PduR_Com.h"

volatile uint32 Com_TxRequestCount;
volatile uint32 Com_TxConfirmCount;
volatile uint32 Com_RxIndicationCount;

static const Com_ConfigType *Com_Cfg = NULL_PTR;
static uint16 Com_TxTimer[COM_MAX_IPDUS];                    /* thời gian còn lại tới lần gửi kế tiếp, ms */

void Com_Init(const Com_ConfigType *config)
{
    if ((config == NULL_PTR) || (config->NumIPdus > COM_MAX_IPDUS)) {
        return;
    }
    for (uint8 p = 0u; p < config->NumIPdus; p++) {
        const Com_IPduCfgType *ip = &config->IPdus[p];
        for (PduLengthType i = 0u; i < ip->Length; i++) {
            ip->Buffer[i] = (ip->InitData != NULL_PTR) ? ip->InitData[i] : 0u;
        }
        Com_TxTimer[p] = ip->TxPeriodMs;
    }
    Com_Cfg = config;
}

uint8 Com_SendSignal(Com_SignalIdType SignalId, const void *SignalDataPtr)
{
    if ((Com_Cfg == NULL_PTR) || (SignalId >= Com_Cfg->NumSignals) || (SignalDataPtr == NULL_PTR)) {
        return COM_SERVICE_NOT_AVAILABLE;
    }
    const Com_SignalCfgType *sig = &Com_Cfg->Signals[SignalId];
    uint8 *buf   = Com_Cfg->IPdus[sig->IPdu].Buffer;
    uint8  idx   = (uint8)(sig->BitPosition / 8u);
    uint8  shift = (uint8)(sig->BitPosition % 8u);
    uint8  mask  = (uint8)(((1u << sig->BitSize) - 1u) << shift);
    uint8  value = *(const uint8 *)SignalDataPtr;

    buf[idx] = (uint8)((buf[idx] & (uint8)~mask) | (uint8)((value << shift) & mask));
    return E_OK;                                    /* SWS_Com_00624 */
}

uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void *SignalDataPtr)
{
    if ((Com_Cfg == NULL_PTR) || (SignalId >= Com_Cfg->NumSignals) || (SignalDataPtr == NULL_PTR)) {
        return COM_SERVICE_NOT_AVAILABLE;
    }
    const Com_SignalCfgType *sig = &Com_Cfg->Signals[SignalId];
    const uint8 *buf = Com_Cfg->IPdus[sig->IPdu].Buffer;
    uint8 idx   = (uint8)(sig->BitPosition / 8u);
    uint8 shift = (uint8)(sig->BitPosition % 8u);
    uint8 mask  = (uint8)((1u << sig->BitSize) - 1u);

    *(uint8 *)SignalDataPtr = (uint8)((buf[idx] >> shift) & mask);   /* SWS_Com_00631 */
    return E_OK;
}

void Com_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if ((Com_Cfg == NULL_PTR) || (RxPduId >= Com_Cfg->NumIPdus) || (PduInfoPtr == NULL_PTR)) {
        return;
    }
    const Com_IPduCfgType *ip = &Com_Cfg->IPdus[RxPduId];
    if (ip->Direction != COM_RECEIVE) {
        return;
    }
    PduLengthType n = (PduInfoPtr->SduLength < ip->Length) ? PduInfoPtr->SduLength : ip->Length;
    for (PduLengthType i = 0u; i < n; i++) {
        ip->Buffer[i] = PduInfoPtr->SduDataPtr[i];
    }
    Com_RxIndicationCount++;
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if ((Com_Cfg != NULL_PTR) && (TxPduId < Com_Cfg->NumIPdus) && (result == E_OK)) {
        Com_TxConfirmCount++;
    }
}

void Com_MainFunctionTx(void)
{
    if (Com_Cfg == NULL_PTR) {
        return;                                     /* SWS_Com_00665: chưa Com_Init */
    }
    for (uint8 p = 0u; p < Com_Cfg->NumIPdus; p++) {
        const Com_IPduCfgType *ip = &Com_Cfg->IPdus[p];
        if ((ip->Direction != COM_SEND) || (ip->TxPeriodMs == 0u)) {
            continue;
        }
        if (Com_TxTimer[p] > COM_MAIN_FUNCTION_TX_PERIOD_MS) {
            Com_TxTimer[p] = (uint16)(Com_TxTimer[p] - COM_MAIN_FUNCTION_TX_PERIOD_MS);
            continue;
        }
        Com_TxTimer[p] = ip->TxPeriodMs;            /* chu kỳ kế tiếp */

        PduInfoType pdu;
        pdu.SduDataPtr  = ip->Buffer;
        pdu.MetaDataPtr = NULL_PTR;
        pdu.SduLength   = ip->Length;
        if (PduR_ComTransmit(ip->PduRPduId, &pdu) == E_OK) {
            Com_TxRequestCount++;
        }
    }
}
