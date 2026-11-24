/*
 * can.c - bxCAN (CAN1) mức thanh ghi trên STM32F103C8 (Blue Pill)
 *
 * RM0008 Rev 21:
 *   9.3.3 / Table 34   CAN_REMAP = 10: CAN_RX = PB8, CAN_TX = PB9
 *   Table 28           CAN_TX: AF push-pull; CAN_RX: input floating / pull-up
 *   24.4.1             Initialization mode: INRQ -> chờ INAK; cấu hình BTR, MCR
 *   24.4.2             Normal mode: xóa INRQ -> chờ INAK = 0 (11 bit recessive)
 *   24.5               SILM/LBKM chỉ đặt được trong Initialization mode
 *   24.7.1             Gửi: chọn mailbox trống (TME), ghi ID/DLC/data, đặt TXRQ
 *   24.7.3             Nhận: FMP0 != 0 -> đọc FIFO output mailbox -> RFOM0 = 1
 *   24.7.4             Filter 32 bit: STID ở bit 31:21, IDE bit 2, RTR bit 1
 *   24.9.2-24.9.4      Thanh ghi MCR, MSR, TSR, RF0R, IER, ESR, BTR, FMR, FM1R, FS1R, FFA1R, FA1R
 */
#include "stm32f1xx.h"
#include "can.h"

#define CAN_TIMEOUT       200000UL
#define PRIO_CAN_RX0      2U

volatile uint32_t    g_can_rx_count;
volatile uint32_t    g_can_rx_new;
volatile can_frame_t g_can_rx_last;
volatile uint32_t    g_can_tx_ok;
volatile uint32_t    g_can_tx_err;
volatile uint32_t    g_can_tx_busy;

static void can1_gpio_pb8_pb9(void)
{
    /* Clock GPIOB + AFIO (APB2), CAN1 (APB1) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    (void)RCC->APB1ENR;

    /* AFIO_MAPR: CAN_REMAP[14:13] = 10. SWJ_CFG[26:24] chỉ ghi, đọc ra không xác định,
     * nên ghi 000 (giữ SWD/JTAG như sau reset) thay vì giữ giá trị đọc được. */
    uint32_t mapr = AFIO->MAPR;
    mapr &= ~(AFIO_MAPR_CAN_REMAP | AFIO_MAPR_SWJ_CFG);
    mapr |=  AFIO_MAPR_CAN_REMAP_REMAP2;
    AFIO->MAPR = mapr;

    /* PB8 (CRH bit 3:0)  = CNF 10, MODE 00: input pull-up/pull-down; ODR8 = 1 -> pull-up
     * PB9 (CRH bit 7:4)  = CNF 10, MODE 11: AF push-pull 50 MHz */
    uint32_t crh = GPIOB->CRH;
    crh &= ~(0xFFUL);
    crh |=  (0x8UL << 0) | (0xBUL << 4);
    GPIOB->CRH = crh;
    GPIOB->BSRR = (1UL << 8);
}

static void can1_filter_list2(uint16_t id_a, uint16_t id_b)
{
    CAN1->FMR  |=  CAN_FMR_FINIT;          /* filter init mode, nhận tạm dừng */
    CAN1->FA1R &= ~(1UL << 0);             /* tắt bank 0 trước khi sửa */
    CAN1->FS1R |=  (1UL << 0);             /* bank 0: một cặp thanh ghi 32 bit */
    CAN1->FM1R |=  (1UL << 0);             /* bank 0: identifier list */
    CAN1->FFA1R &= ~(1UL << 0);            /* bank 0 -> FIFO0 */
    /* Standard ID: STID[10:0] ở bit 31:21, IDE = 0, RTR = 0 (data frame) */
    CAN1->sFilterRegister[0].FR1 = (uint32_t)id_a << 21;   /* filter number 0 */
    CAN1->sFilterRegister[0].FR2 = (uint32_t)id_b << 21;   /* filter number 1 */
    CAN1->FA1R |=  (1UL << 0);
    CAN1->FMR  &= ~CAN_FMR_FINIT;
}

int can1_init(uint32_t btr, uint32_t mode, uint16_t id_a, uint16_t id_b)
{
    uint32_t t;

    can1_gpio_pb8_pb9();

    /* 1. Sleep (sau reset) -> Initialization: xóa SLEEP, đặt INRQ, chờ INAK = 1 và SLAK = 0 */
    CAN1->MCR = (CAN1->MCR & ~CAN_MCR_SLEEP) | CAN_MCR_INRQ;
    for (t = 0; t < CAN_TIMEOUT; t++) {
        if ((CAN1->MSR & (CAN_MSR_INAK | CAN_MSR_SLAK)) == CAN_MSR_INAK) break;
    }
    if (t == CAN_TIMEOUT) return -1;

    /* 2. MCR: ABOM = 1 (tự phục hồi bus-off), giữ DBF = 1 (dừng CAN khi CPU dừng ở breakpoint),
     *    TXFP = 0 (ưu tiên theo ID), NART = 0 (tự gửi lại), RFLM = 0, TTCM = 0 */
    CAN1->MCR = CAN_MCR_DBF | CAN_MCR_ABOM | CAN_MCR_INRQ;

    /* 3. Bit timing + chế độ test (chỉ ghi được khi INAK = 1) */
    if (mode == CAN_MODE_LOOPBACK_SILENT) {
        btr |= CAN_BTR_LBKM | CAN_BTR_SILM;
    }
    CAN1->BTR = btr;

    /* 4. Bộ lọc: chỉ nhận 2 standard ID vào FIFO0 */
    can1_filter_list2(id_a, id_b);

    /* 5. Ngắt FIFO0 message pending: USB_LP_CAN1_RX0_IRQn = 20 */
    CAN1->IER = CAN_IER_FMPIE0;
    NVIC->IPR[USB_LP_CAN1_RX0_IRQn] = (uint8_t)(PRIO_CAN_RX0 << (8U - __NVIC_PRIO_BITS));
    NVIC->ISER[0] = 1UL << (uint32_t)USB_LP_CAN1_RX0_IRQn;

    /* 6. Rời Initialization: xóa INRQ, chờ INAK = 0 (bxCAN cần 11 bit recessive trên CANRX) */
    CAN1->MCR &= ~CAN_MCR_INRQ;
    for (t = 0; t < CAN_TIMEOUT; t++) {
        if ((CAN1->MSR & CAN_MSR_INAK) == 0U) return 0;
    }
    return -2;
}

int can1_send_std(uint16_t id, const uint8_t *data, uint8_t dlc)
{
    uint32_t tsr = CAN1->TSR;
    if ((tsr & CAN_TSR_TME) == 0U) {           /* TME0..2 = 0: cả 3 mailbox đang chờ gửi */
        g_can_tx_busy++;
        return -1;
    }
    uint32_t mb = (tsr & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;   /* số mailbox trống kế tiếp */
    if (dlc > 8U) dlc = 8U;

    uint8_t b[8] = {0};
    for (uint32_t i = 0; i < dlc; i++) b[i] = data[i];

    CAN1->sTxMailBox[mb].TIR  = (uint32_t)(id & 0x7FFU) << 21;        /* IDE = 0, RTR = 0 */
    CAN1->sTxMailBox[mb].TDTR = dlc;                                    /* TGT = 0 */
    CAN1->sTxMailBox[mb].TDLR = ((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16)
                              | ((uint32_t)b[1] << 8)  |  (uint32_t)b[0];
    CAN1->sTxMailBox[mb].TDHR = ((uint32_t)b[7] << 24) | ((uint32_t)b[6] << 16)
                              | ((uint32_t)b[5] << 8)  |  (uint32_t)b[4];
    CAN1->sTxMailBox[mb].TIR |= CAN_TI0R_TXRQ;                         /* yêu cầu gửi */
    return (int)mb;
}

void can1_poll_tx_status(void)
{
    /* TSR mỗi mailbox 8 bit: RQCPx (bit 0), TXOKx (bit 1), ALSTx (bit 2), TERRx (bit 3) */
    for (uint32_t mb = 0; mb < 3U; mb++) {
        uint32_t tsr = CAN1->TSR;
        uint32_t rqcp = CAN_TSR_RQCP0 << (8U * mb);
        if (tsr & rqcp) {
            if (tsr & (CAN_TSR_TXOK0 << (8U * mb))) g_can_tx_ok++;
            else                                    g_can_tx_err++;
            CAN1->TSR = rqcp;                    /* ghi 1 xóa RQCPx, kéo theo TXOK/ALST/TERR */
        }
    }
}

/* FIFO0: mỗi lần FMP0 != 0 đọc một frame rồi nhả output mailbox */
/* Tên handler phải khớp bảng vector trong Startup/startup_stm32f103c8tx.s (USB_LP_CAN_RX0_IRQHandler),
 * khác tên IRQn trong CMSIS (USB_LP_CAN1_RX0_IRQn = 20). */
void USB_LP_CAN_RX0_IRQHandler(void)
{
    while ((CAN1->RF0R & CAN_RF0R_FMP0) != 0U) {
        uint32_t rir  = CAN1->sFIFOMailBox[0].RIR;
        uint32_t rdtr = CAN1->sFIFOMailBox[0].RDTR;
        uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;

        g_can_rx_last.id  = (rir >> 21) & 0x7FFU;
        g_can_rx_last.dlc = (uint8_t)(rdtr & 0xFU);
        g_can_rx_last.fmi = (uint8_t)((rdtr >> 8) & 0xFFU);
        for (uint32_t i = 0; i < 4U; i++) {
            g_can_rx_last.data[i]     = (uint8_t)(rdlr >> (8U * i));
            g_can_rx_last.data[i + 4] = (uint8_t)(rdhr >> (8U * i));
        }
        CAN1->RF0R = CAN_RF0R_RFOM0;             /* nhả mailbox, FMP0 giảm 1 */
        g_can_rx_count++;
        g_can_rx_new = 1U;
    }
}
