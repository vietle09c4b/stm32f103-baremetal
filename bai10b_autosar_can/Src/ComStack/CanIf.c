/* CanIf.c - CAN Interface.
 * Không truy cập thanh ghi. Chuyển PDU giữa tầng trên (LSduR) và Can Driver qua bảng cấu hình:
 *   - Tx: TxPduId -> CAN ID, DLC, HTH -> Can_Write. Không cấu hình transmit buffer, nên CAN_BUSY
 *     được trả về tầng trên dưới dạng E_NOT_OK.
 *   - Rx: HRH là Basic CAN (bộ lọc phần cứng có thể nhận nhiều ID) nên CanIf lọc phần mềm theo CAN ID
 *     (SWS_CANIF_00663), kiểm tra DLC, rồi gọi LSduR_CanIfRxIndication.
 * Từ R24-11, tầng trên của mọi communication interface là LSduR (CanIf R24-11 change history). */
#include "CanIf.h"
#include "CanIf_Cbk.h"
#include "Can.h"
#include "LSduR.h"

volatile Can_ControllerStateType CanIf_ControllerMode = CAN_CS_UNINIT;

static const CanIf_ConfigType *CanIf_Cfg = NULL_PTR;

void CanIf_Init(const CanIf_ConfigType *ConfigPtr)
{
    CanIf_Cfg            = ConfigPtr;
    CanIf_ControllerMode = CAN_CS_STOPPED;          /* Can_Init đã đưa controller về STOPPED */
}

Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, Can_ControllerStateType ControllerMode)
{
    return Can_SetControllerMode(ControllerId, ControllerMode);   /* kết quả về qua ControllerModeIndication */
}

void CanIf_ControllerModeIndication(uint8 ControllerId, Can_ControllerStateType ControllerMode)
{
    (void)ControllerId;
    CanIf_ControllerMode = ControllerMode;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    if ((CanIf_Cfg == NULL_PTR) || (PduInfoPtr == NULL_PTR) || (TxPduId >= CanIf_Cfg->NumTxPdus)) {
        return E_NOT_OK;                            /* DET: CANIF_E_UNINIT / PARAM_POINTER / INVALID_TXPDUID */
    }
    if (CanIf_ControllerMode != CAN_CS_STARTED) {
        return E_NOT_OK;                            /* controller chưa STARTED: không nhận yêu cầu gửi */
    }

    const CanIf_TxPduCfgType *cfg = &CanIf_Cfg->TxPdus[TxPduId];
    Can_PduType canPdu;
    canPdu.swPduHandle = TxPduId;
    canPdu.id          = cfg->CanId;
    canPdu.length      = (PduInfoPtr->SduLength < cfg->Dlc) ? (uint8)PduInfoPtr->SduLength : cfg->Dlc;
    canPdu.sdu         = PduInfoPtr->SduDataPtr;

    return (Can_Write(cfg->Hth, &canPdu) == E_OK) ? E_OK : E_NOT_OK;
}

void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    if ((CanIf_Cfg == NULL_PTR) || (CanTxPduId >= CanIf_Cfg->NumTxPdus)) {
        return;
    }
    LSduR_CanIfTxConfirmation(CanIf_Cfg->TxPdus[CanTxPduId].UpperPduId, E_OK);
}

void CanIf_RxIndication(const Can_HwType *Mailbox, const PduInfoType *PduInfoPtr)
{
    if ((CanIf_Cfg == NULL_PTR) || (Mailbox == NULL_PTR) || (PduInfoPtr == NULL_PTR)) {
        return;
    }
    for (uint8 i = 0u; i < CanIf_Cfg->NumRxPdus; i++) {            /* software filtering */
        const CanIf_RxPduCfgType *cfg = &CanIf_Cfg->RxPdus[i];
        if ((cfg->Hrh == Mailbox->Hoh) && (cfg->CanId == Mailbox->CanId)) {
            if (PduInfoPtr->SduLength < cfg->MinDlc) {
                return;                             /* DLC check thất bại: bỏ PDU */
            }
            LSduR_CanIfRxIndication(cfg->UpperPduId, PduInfoPtr);
            return;
        }
    }
    /* không khớp Rx PDU nào: bỏ qua */
}
