/*
 * at24c256.h - EEPROM AT24C256C (32 768 x 8, trang 64 byte) qua I2C1
 * Datasheet Atmel-8568F-SEEPROM-AT24C256C (01/2015)
 */
#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include "i2c1.h"

#define AT24_ADDR7        0x50U      /* 1010 A2 A1 A0, A2 = A1 = A0 = 0 */
#define AT24_SIZE         32768U
#define AT24_PAGE_SIZE    64U
#define AT24_TWR_MAX_US   5000U      /* tWR max 5 ms */

/* Kết quả lần ghi gần nhất (xem bằng Live Expressions) */
extern volatile uint32_t at24_last_polls;     /* số lần địa chỉ bị NACK khi chờ ghi xong */
extern volatile uint32_t at24_last_twr_us;    /* thời gian từ STOP tới ACK đầu tiên, µs */
extern volatile uint32_t at24_max_twr_us;

void         at24_init(uint32_t cpu_hz);

/* Đọc len byte từ địa chỉ mem (random read + sequential read) */
i2c_status_t at24_read(uint16_t mem, uint8_t *buf, uint32_t len);

/* Ghi len byte từ địa chỉ mem, tự tách theo biên trang 64 byte, chờ ghi xong từng trang */
i2c_status_t at24_write(uint16_t mem, const uint8_t *data, uint32_t len);

/* Một lệnh page write, KHÔNG tách theo biên trang (dùng để minh họa roll-over) */
i2c_status_t at24_write_raw(uint16_t mem, const uint8_t *data, uint32_t len);

/* ACK polling: chờ tới khi EEPROM trả ACK hoặc hết timeout_us */
i2c_status_t at24_wait_ready(uint32_t timeout_us);

#endif /* AT24C256_H */
