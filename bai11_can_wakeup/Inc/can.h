/*
 * can.h - bxCAN (CAN1) mức thanh ghi trên STM32F103C8 (Blue Pill), thêm Sleep mode (bài 11)
 * Chân: PB9 = CAN_TX, PB8 = CAN_RX (AFIO_MAPR CAN_REMAP = 10)
 */
#ifndef CAN_H
#define CAN_H

#include <stdint.h>

/* CAN_BTR với PCLK1 = 8 MHz (HSE, không PLL):
 * BRP = 0 -> tq = 125 ns; TS1 = 12 -> BS1 = 13 tq; TS2 = 1 -> BS2 = 2 tq; SJW = 0 -> 1 tq
 * 1 + 13 + 2 = 16 tq = 2 us -> 500 kbit/s, sample point 14/16 = 87,5 % */
#define CAN_BTR_500K_PCLK8M   0x001C0000UL

#define CAN_MODE_NORMAL           0U
#define CAN_MODE_LOOPBACK_SILENT  1U

typedef struct {
    uint32_t id;        /* standard ID 11 bit */
    uint8_t  dlc;
    uint8_t  data[8];
    uint8_t  fmi;       /* filter match index */
} can_frame_t;

/* Trả 0 nếu vào Normal mode thành công, -1 nếu không vào Init, -2 nếu không rời Init */
int  can1_init(uint32_t btr, uint32_t mode, uint16_t id_a, uint16_t id_b);
/* Trả số mailbox (0..2) đã dùng, -1 nếu cả 3 mailbox đang bận */
int  can1_send_std(uint16_t id, const uint8_t *data, uint8_t dlc);
/* Đọc kết quả các mailbox đã xong (RQCPx), cộng vào g_can_tx_ok / g_can_tx_err */
void can1_poll_tx_status(void);
/* Chờ cả 3 transmit mailbox trống (TME0..2 = 1). Trả 0 nếu xong, -1 nếu hết thời gian chờ */
int  can1_wait_tx_empty(void);
/* Normal -> Sleep: đặt SLEEP, chờ SLAK = 1. Trả 0 nếu vào Sleep, -1 nếu không */
int  can1_sleep(void);
/* Sleep -> Normal: xóa SLEEP, chờ SLAK = 0 (bxCAN đồng bộ 11 bit recessive). Trả 0/-1 */
int  can1_wakeup(void);

extern volatile uint32_t    g_can_rx_count;
extern volatile uint32_t    g_can_rx_new;     /* ISR đặt 1, main xóa */
extern volatile can_frame_t g_can_rx_last;
extern volatile uint32_t    g_can_tx_ok;
extern volatile uint32_t    g_can_tx_err;
extern volatile uint32_t    g_can_tx_busy;

#endif
