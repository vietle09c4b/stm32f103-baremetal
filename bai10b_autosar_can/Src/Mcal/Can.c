/* Can.c - CAN Driver (MCAL) cho bxCAN (CAN1) của STM32F103, mức thanh ghi.
 *
 * Tầng duy nhất truy cập thanh ghi CAN. Phía trên chỉ thấy HTH/HRH, Can_PduType và các callback của CanIf.
 *   - Can_Init: Sleep -> Initialization, cấu hình MCR, BTR, bộ lọc, ngắt FIFO0; controller ở CAN_CS_STOPPED
 *     (SWS_Can_00259). Trên bxCAN, trạng thái STOPPED tương ứng Initialization mode (INAK = 1).
 *   - Can_SetControllerMode: bất đồng bộ; chỉ gửi yêu cầu (INRQ), kết quả báo qua
 *     CanIf_ControllerModeIndication trong Can_MainFunction_Mode (polling).
 *   - Can_Write: chọn mailbox trống, ghi ID/DLC/data, đặt TXRQ; hết mailbox -> CAN_BUSY.
 *   - Nhận: ngắt FIFO0 -> CanIf_RxIndication (interrupt mode).
 *   - Xác nhận gửi: Can_MainFunction_Write đọc RQCP/TXOK -> CanIf_TxConfirmation (polling mode).
 *
 * RM0008 Rev 21: 24.4.1 Initialization mode, 24.4.2 Normal mode, 24.7.1 Transmission handling,
 * 24.7.4 Identifier filtering, 24.7.5 Message storage, 24.9 bxCAN registers. */
#include "Can.h"
#include "CanIf_Cbk.h"
#include "Stm32f103_Reg.h"

#define CAN_TIMEOUT        200000UL
#define CAN_NUM_TX_MB      3u

volatile uint32 Can_Esr;
volatile uint32 Can_RxIsrCount;
volatile uint32 Can_TxConfCount;

static Can_ControllerStateType Can_State     = CAN_CS_UNINIT;
static Can_ControllerStateType Can_Requested = CAN_CS_UNINIT;
static PduIdType               Can_TxPduHandle[CAN_NUM_TX_MB];   /* swPduHandle theo mailbox (SWS_Can_00276) */
static boolean                 Can_TxPending[CAN_NUM_TX_MB];

static void Can_FilterInit(const Can_ConfigType *Config)
{
    CAN1->FMR   |=  CAN_FMR_FINIT;                 /* filter init mode */
    CAN1->FA1R  &= ~(1UL << 0);                    /* tắt bank 0 trước khi sửa */
    CAN1->FS1R  |=  (1UL << 0);                    /* bank 0: 32 bit */
    CAN1->FM1R  |=  (1UL << 0);                    /* bank 0: identifier list */
    CAN1->FFA1R &= ~(1UL << 0);                    /* bank 0 -> FIFO0 */
    CAN1->sFilterRegister[0].FR1 = (uint32)Config->RxStdId[0] << 21;   /* STID ở bit 31:21, IDE = RTR = 0 */
    CAN1->sFilterRegister[0].FR2 = (uint32)Config->RxStdId[1] << 21;
    CAN1->FA1R  |=  (1UL << 0);
    CAN1->FMR   &= ~CAN_FMR_FINIT;
}

void Can_Init(const Can_ConfigType *Config)
{
    uint32 t;

    if ((Config == NULL_PTR) || (Can_State != CAN_CS_UNINIT)) {
        return;                                     /* DET: CAN_E_PARAM_POINTER / CAN_E_TRANSITION */
    }

    /* Sleep (sau reset) -> Initialization: xóa SLEEP, đặt INRQ, chờ INAK = 1 và SLAK = 0 */
    CAN1->MCR = (CAN1->MCR & ~CAN_MCR_SLEEP) | CAN_MCR_INRQ;
    for (t = 0u; t < CAN_TIMEOUT; t++) {
        if ((CAN1->MSR & (CAN_MSR_INAK | CAN_MSR_SLAK)) == CAN_MSR_INAK) { break; }
    }
    if (t == CAN_TIMEOUT) {
        return;                                     /* phần cứng không vào Initialization: giữ UNINIT */
    }

    /* DBF: dừng CAN khi CPU dừng ở breakpoint; ABOM: tự phục hồi bus-off; TXFP = 0: ưu tiên theo ID */
    CAN1->MCR = CAN_MCR_DBF | CAN_MCR_ABOM | CAN_MCR_INRQ;
    CAN1->BTR = Config->Btr;                        /* chỉ ghi được khi INAK = 1 */
    Can_FilterInit(Config);

    CAN1->IER = CAN_IER_FMPIE0;                     /* ngắt khi FIFO0 có message */
    NVIC_IPR_BYTE(CAN1_RX0_IRQN) = (uint8)(Config->RxIrqPriority << 4);   /* 4 bit ưu tiên */
    NVIC_ISER0 = 1UL << CAN1_RX0_IRQN;

    for (uint8 i = 0u; i < CAN_NUM_TX_MB; i++) { Can_TxPending[i] = FALSE; }
    Can_State     = CAN_CS_STOPPED;                 /* SWS_Can_00259 */
    Can_Requested = CAN_CS_STOPPED;
}

Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition)
{
    if ((Controller != CanConf_CanController_0) || (Can_State == CAN_CS_UNINIT)) {
        return E_NOT_OK;                            /* DET: CAN_E_PARAM_CONTROLLER / CAN_E_UNINIT */
    }
    switch (Transition) {
    case CAN_CS_STARTED:
        if (Can_State != CAN_CS_STOPPED) { return E_NOT_OK; }       /* chỉ STOPPED -> STARTED */
        CAN1->MCR &= ~CAN_MCR_INRQ;                 /* rời Initialization sau 11 bit recessive */
        break;
    case CAN_CS_STOPPED:
        CAN1->MCR |= CAN_MCR_INRQ;                  /* về Initialization */
        break;
    default:
        return E_NOT_OK;                            /* SLEEP không hiện thực trong ví dụ */
    }
    Can_Requested = Transition;
    return E_OK;
}

void Can_MainFunction_Mode(void)
{
    uint32 inak = CAN1->MSR & CAN_MSR_INAK;

    if ((Can_Requested == CAN_CS_STARTED) && (Can_State != CAN_CS_STARTED) && (inak == 0u)) {
        Can_State = CAN_CS_STARTED;
        CanIf_ControllerModeIndication(CanConf_CanController_0, CAN_CS_STARTED);
    } else if ((Can_Requested == CAN_CS_STOPPED) && (Can_State != CAN_CS_STOPPED) && (inak != 0u)) {
        Can_State = CAN_CS_STOPPED;
        CanIf_ControllerModeIndication(CanConf_CanController_0, CAN_CS_STOPPED);
    } else {
        /* chưa đổi trạng thái */
    }
}

Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo)
{
    if ((Hth != CanConf_CanHardwareObject_Tx0) || (PduInfo == NULL_PTR) || (PduInfo->length > 8u) ||
        ((PduInfo->sdu == NULL_PTR) && (PduInfo->length != 0u))) {
        return E_NOT_OK;                            /* DET: CAN_E_PARAM_HANDLE / POINTER / DATA_LENGTH */
    }
    if (Can_State != CAN_CS_STARTED) {
        return E_NOT_OK;
    }

    uint32 tsr = CAN1->TSR;
    if ((tsr & CAN_TSR_TME_MASK) == 0u) {
        return CAN_BUSY;                            /* cả 3 mailbox đang chờ gửi */
    }
    uint32 mb = (tsr & CAN_TSR_CODE_MASK) >> CAN_TSR_CODE_POS;

    uint8 b[8] = {0u};
    for (uint8 i = 0u; i < PduInfo->length; i++) { b[i] = PduInfo->sdu[i]; }

    Can_TxPduHandle[mb] = PduInfo->swPduHandle;
    Can_TxPending[mb]   = TRUE;

    CAN1->sTxMailBox[mb].TIR  = (PduInfo->id & 0x7FFUL) << 21;          /* standard ID, IDE = 0, RTR = 0 */
    CAN1->sTxMailBox[mb].TDTR = PduInfo->length;
    CAN1->sTxMailBox[mb].TDLR = ((uint32)b[3] << 24) | ((uint32)b[2] << 16) | ((uint32)b[1] << 8) | b[0];
    CAN1->sTxMailBox[mb].TDHR = ((uint32)b[7] << 24) | ((uint32)b[6] << 16) | ((uint32)b[5] << 8) | b[4];
    CAN1->sTxMailBox[mb].TIR |= CAN_TIR_TXRQ;
    return E_OK;
}

void Can_MainFunction_Write(void)
{
    for (uint8 mb = 0u; mb < CAN_NUM_TX_MB; mb++) {
        uint32 tsr  = CAN1->TSR;
        uint32 rqcp = CAN_TSR_RQCP0 << (8u * mb);
        if ((tsr & rqcp) != 0u) {
            boolean ok = ((tsr & (CAN_TSR_TXOK0 << (8u * mb))) != 0u) ? TRUE : FALSE;
            CAN1->TSR = rqcp;                       /* ghi 1 xóa RQCP, kéo theo TXOK/ALST/TERR */
            if ((ok != FALSE) && (Can_TxPending[mb] != FALSE)) {
                Can_TxPending[mb] = FALSE;
                Can_TxConfCount++;
                CanIf_TxConfirmation(Can_TxPduHandle[mb]);
            }
        }
    }
    Can_Esr = CAN1->ESR;                            /* quan sát lỗi bus khi debug */
}

/* Tên handler theo bảng vector trong Startup/startup_stm32f103c8tx.s (IRQ 20) */
void USB_LP_CAN_RX0_IRQHandler(void)
{
    while ((CAN1->RF0R & CAN_RF0R_FMP0_MASK) != 0u) {
        uint32 rir  = CAN1->sFIFOMailBox[0].RIR;
        uint32 rdtr = CAN1->sFIFOMailBox[0].RDTR;
        uint32 rdlr = CAN1->sFIFOMailBox[0].RDLR;
        uint32 rdhr = CAN1->sFIFOMailBox[0].RDHR;
        CAN1->RF0R = CAN_RF0R_RFOM0;                /* nhả output mailbox sau khi đã chép */

        uint8 data[8];
        for (uint8 i = 0u; i < 4u; i++) {
            data[i]      = (uint8)(rdlr >> (8u * i));
            data[i + 4u] = (uint8)(rdhr >> (8u * i));
        }

        Can_HwType  mailbox;
        PduInfoType pdu;
        mailbox.CanId        = (rir >> 21) & 0x7FFUL;
        mailbox.Hoh          = CanConf_CanHardwareObject_Rx0;
        mailbox.ControllerId = CanConf_CanController_0;
        pdu.SduDataPtr  = data;
        pdu.MetaDataPtr = NULL_PTR;
        uint32 dlc      = rdtr & 0xFu;
        pdu.SduLength   = (PduLengthType)((dlc > 8u) ? 8u : dlc);   /* ISO 11898-1, Classical CAN: DLC 9..15 = 8 byte */

        Can_RxIsrCount++;
        CanIf_RxIndication(&mailbox, &pdu);         /* SWS_CANIF_00006 */
    }
}
