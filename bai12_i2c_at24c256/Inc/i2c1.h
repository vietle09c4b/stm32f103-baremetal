/*
 * i2c1.h - I2C1 master, polling, STM32F103C8 (Blue Pill), mức thanh ghi
 *
 * Chân: PB6 = SCL, PB7 = SDA (không remap, RM0008 Table 41/AFIO_MAPR.I2C1_REMAP = 0).
 * Tốc độ: Standard mode, CCR = 46 ở PCLK1 = 8 MHz -> khoảng 87 kHz (dưới 88 kHz, ES096 mục 2.8.5).
 */
#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY,      /* BUSY không về 0 trong thời gian chờ          */
    I2C_ERR_NACK,      /* slave không trả ACK (AF = 1)                 */
    I2C_ERR_TIMEOUT,   /* cờ trạng thái không lên trong thời gian chờ  */
    I2C_ERR_BUS        /* BERR hoặc ARLO                               */
} i2c_status_t;

/* pclk1_hz: tần số APB1. scl_hz: tần số SCL mong muốn (Standard mode, <= 100 kHz) */
void         i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz);

/* S + addr/W + data[0..len-1] + P */
i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, uint32_t len);

/* S + addr/W + wr[0..wlen-1] + Sr + addr/R + rd[0..rlen-1] (NACK byte cuối) + P */
i2c_status_t i2c1_write_read(uint8_t addr7, const uint8_t *wr, uint32_t wlen,
                             uint8_t *rd, uint32_t rlen);

/* S + addr/W + P: trả I2C_OK nếu slave trả ACK */
i2c_status_t i2c1_probe(uint8_t addr7);

extern volatile uint32_t i2c1_error_count;
extern volatile uint32_t i2c1_last_sr1;

#endif /* I2C1_H */
