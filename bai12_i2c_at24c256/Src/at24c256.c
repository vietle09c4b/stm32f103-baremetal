/*
 * at24c256.c - đọc ghi EEPROM AT24C256C qua I2C1
 *
 * Datasheet Atmel-8568F (01/2015):
 *   mục 5   512 trang x 64 byte, địa chỉ từ 15 bit gửi trong 2 byte (bit 15 không dùng)
 *   mục 7   byte địa chỉ thiết bị 1 0 1 0 A2 A1 A0 R/W
 *   mục 8   byte write / page write: 6 bit thấp của địa chỉ tự tăng, tới biên trang thì quay về
 *           đầu CÙNG trang; sau STOP là chu kỳ ghi trong tWR (max 5 ms), EEPROM không trả ACK
 *           cho tới khi ghi xong -> acknowledge polling
 *   mục 9   random read = dummy write 2 byte địa chỉ + repeated START + đọc; sequential read:
 *           master ACK thì EEPROM tăng địa chỉ, master NACK + STOP thì dừng
 */
#include "stm32f1xx.h"
#include "at24c256.h"

volatile uint32_t at24_last_polls;
volatile uint32_t at24_last_twr_us;
volatile uint32_t at24_max_twr_us;

static uint32_t s_cyc_per_us = 8U;

void at24_init(uint32_t cpu_hz)
{
    s_cyc_per_us = cpu_hz / 1000000UL;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* DWT->CYCCNT để đo thời gian */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

i2c_status_t at24_read(uint16_t mem, uint8_t *buf, uint32_t len)
{
    uint8_t a[2];
    mem &= (AT24_SIZE - 1U);
    a[0] = (uint8_t)(mem >> 8);          /* first word address  */
    a[1] = (uint8_t)(mem & 0xFFU);       /* second word address */
    return i2c1_write_read(AT24_ADDR7, a, 2U, buf, len);
}

i2c_status_t at24_wait_ready(uint32_t timeout_us)
{
    uint32_t t0 = DWT->CYCCNT;
    uint32_t polls = 0U;
    i2c_status_t st;

    for (;;) {
        st = i2c1_probe(AT24_ADDR7);
        if (st == I2C_OK) break;
        if (st != I2C_ERR_NACK) return st;           /* lỗi bus thật, không phải đang ghi */
        polls++;
        if ((DWT->CYCCNT - t0) / s_cyc_per_us > timeout_us) return I2C_ERR_TIMEOUT;
    }
    at24_last_polls  = polls;
    at24_last_twr_us = (DWT->CYCCNT - t0) / s_cyc_per_us;
    if (at24_last_twr_us > at24_max_twr_us) at24_max_twr_us = at24_last_twr_us;
    return I2C_OK;
}

i2c_status_t at24_write_raw(uint16_t mem, const uint8_t *data, uint32_t len)
{
    uint8_t frame[2U + 2U * AT24_PAGE_SIZE];      /* đủ cho minh họa ghi quá 64 byte */
    i2c_status_t st;

    if (len > 2U * AT24_PAGE_SIZE) return I2C_ERR_BUS;
    mem &= (AT24_SIZE - 1U);
    frame[0] = (uint8_t)(mem >> 8);
    frame[1] = (uint8_t)(mem & 0xFFU);
    for (uint32_t i = 0; i < len; i++) frame[2U + i] = data[i];

    st = i2c1_write(AT24_ADDR7, frame, 2U + len);   /* STOP ở cuối khởi động chu kỳ ghi tWR */
    if (st != I2C_OK) return st;
    return at24_wait_ready(2U * AT24_TWR_MAX_US);
}

i2c_status_t at24_write(uint16_t mem, const uint8_t *data, uint32_t len)
{
    while (len > 0U) {
        uint32_t room  = AT24_PAGE_SIZE - (mem % AT24_PAGE_SIZE);   /* số byte còn lại tới biên trang */
        uint32_t chunk = (len < room) ? len : room;
        i2c_status_t st = at24_write_raw(mem, data, chunk);
        if (st != I2C_OK) return st;
        mem  = (uint16_t)(mem + chunk);
        data += chunk;
        len  -= chunk;
    }
    return I2C_OK;
}
