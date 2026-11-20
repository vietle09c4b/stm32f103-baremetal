/*
 * i2c1.h - driver I2C1 master, chế độ polling, Standard mode 100 kHz
 * Chân: PB6 = SCL, PB7 = SDA (không remap). Cả hai chân là FT (chịu 5 V),
 * nên dùng được với module PCF8574 có điện trở kéo lên 5 V.
 */
#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY,      /* bus bận quá thời gian chờ          */
    I2C_ERR_NACK,      /* slave không trả ACK (sai địa chỉ)  */
    I2C_ERR_TIMEOUT,   /* cờ trạng thái không lên đúng hạn   */
    I2C_ERR_BUS        /* lỗi bus hoặc mất trọng tài         */
} i2c_status_t;

/* pclk1_hz: tần số APB1 (PCLK1), tối thiểu 2 MHz cho Standard mode */
void         i2c1_init(uint32_t pclk1_hz);

/* Ghi len byte tới slave có địa chỉ 7 bit addr7 trong một giao dịch */
i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, uint32_t len);

/* Trả I2C_OK nếu slave addr7 trả ACK */
i2c_status_t i2c1_probe(uint8_t addr7);

#endif /* I2C1_H */
