/*
 * spi1.h - Driver SPI1 mức thanh ghi cho STM32F103C8 (RM0008 Rev 21, chương 25)
 *
 * Chân (SPI1_REMAP = 0, RM0008 9.3.10): PA5 SCK, PA6 MISO, PA7 MOSI; PA4 dùng làm CS (GPIO thường).
 */
#ifndef SPI1_H
#define SPI1_H

#include <stdint.h>

/* Bốn mode SPI = (CPOL << 1) | CPHA, đúng vị trí bit 1 và bit 0 của SPI_CR1 */
#define SPI_MODE0   0U
#define SPI_MODE1   1U
#define SPI_MODE2   2U
#define SPI_MODE3   3U

/* Hệ số chia BR[2:0]: 0 -> fPCLK/2 ... 7 -> fPCLK/256 */
#define SPI_BR_DIV2     0U
#define SPI_BR_DIV4     1U
#define SPI_BR_DIV256   7U

typedef enum {
    SPI_OK = 0,
    SPI_ERR_TIMEOUT = 1,
    SPI_ERR_MODF = 2,
    SPI_ERR_OVR = 3
} spi_status_t;

void         spi1_gpio_init(void);
void         spi1_config(uint32_t br, uint32_t mode, uint32_t frame16);
void         spi1_disable(void);
spi_status_t spi1_xfer8(uint8_t tx, uint8_t *rx);
spi_status_t spi1_xfer16(uint16_t tx, uint16_t *rx);
spi_status_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint32_t n);
spi_status_t spi1_transfer_burst(const uint8_t *tx, uint8_t *rx, uint32_t n);
void         spi1_cs_low(void);
void         spi1_cs_high(void);

#endif /* SPI1_H */
