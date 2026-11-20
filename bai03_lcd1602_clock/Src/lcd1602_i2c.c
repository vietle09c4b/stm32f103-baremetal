/*
 * lcd1602_i2c.c - HD44780 4 bit qua PCF8574
 *
 * Mỗi byte gửi tới PCF8574 được chốt ra 8 chân P0..P7 ngay khi slave trả ACK.
 * Một nửa byte (nibble) cho LCD cần hai byte I2C: lần 1 có EN = 1, lần 2 có
 * EN = 0; HD44780 đọc D4..D7 ở cạnh xuống của EN. Ở 100 kHz, mỗi byte I2C
 * kéo dài khoảng 90 us, lớn hơn nhiều độ rộng xung EN tối thiểu 450 ns.
 */
#include "lcd1602_i2c.h"
#include "i2c1.h"
#include "timebase.h"

#define PIN_RS   0x01u
#define PIN_RW   0x02u   /* luôn 0: chỉ ghi vào LCD */
#define PIN_EN   0x04u
#define PIN_BL   0x08u

/* Lệnh HD44780 (datasheet Hitachi, Table 6) */
#define CMD_CLEAR          0x01u
#define CMD_ENTRY_INC      0x06u   /* I/D = 1, S = 0 */
#define CMD_DISPLAY_OFF    0x08u
#define CMD_DISPLAY_ON     0x0Cu   /* D = 1, C = 0, B = 0 */
#define CMD_FUNC_4BIT_2L   0x28u   /* DL = 0, N = 1, F = 0 (5x8) */
#define CMD_SET_DDRAM      0x80u

static uint8_t lcd_addr;
static uint8_t backlight = PIN_BL;
static uint8_t lcd_fail;          /* 1 nếu có lần ghi I2C thất bại */

/* Gửi 1 byte tới LCD dưới dạng hai nibble, gói trong một giao dịch I2C */
static void lcd_send(uint8_t value, uint8_t rs)
{
    uint8_t hi = (uint8_t)(value & 0xF0u);
    uint8_t lo = (uint8_t)(((unsigned)value << 4) & 0xF0u);
    uint8_t ctl = (uint8_t)(backlight | rs);
    uint8_t buf[4] = {
        (uint8_t)(hi | ctl | PIN_EN), (uint8_t)(hi | ctl),
        (uint8_t)(lo | ctl | PIN_EN), (uint8_t)(lo | ctl)
    };
    if (i2c1_write(lcd_addr, buf, sizeof buf) != I2C_OK) lcd_fail = 1u;
}

/* Chỉ dùng trong pha khởi tạo, khi LCD còn ở chế độ 8 bit */
static void lcd_send_nibble(uint8_t nibble)
{
    uint8_t d = (uint8_t)((nibble << 4) | backlight);
    uint8_t buf[2] = { (uint8_t)(d | PIN_EN), d };
    if (i2c1_write(lcd_addr, buf, sizeof buf) != I2C_OK) lcd_fail = 1u;
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send(cmd, 0);
    if (cmd == CMD_CLEAR || cmd == 0x02u) {
        delay_ms(2);                  /* clear/home: tối đa 1,52 ms */
    }
}

uint8_t lcd_detect(void)
{
    static const uint8_t base[2] = { 0x27u, 0x3Fu };   /* thử địa chỉ mặc định trước */
    for (int b = 0; b < 2; b++) {
        if (i2c1_probe(base[b]) == I2C_OK) return base[b];
    }
    for (uint8_t a = 0x20u; a <= 0x3Fu; a++) {
        if (a == 0x28u) a = 0x38u;                        /* bỏ qua 0x28..0x37 */
        if (i2c1_probe(a) == I2C_OK) return a;
    }
    return 0u;
}

int lcd_init(uint8_t addr7)
{
    lcd_addr = addr7;
    lcd_fail = 0u;
    if (i2c1_probe(lcd_addr) != I2C_OK) return -1;

    delay_ms(50);                     /* > 40 ms sau khi VCC đạt 2,7 V */

    /* HD44780 Figure 24: ba lần 0x3 rồi 0x2 để vào chế độ 4 bit */
    lcd_send_nibble(0x3u);  delay_ms(5);    /* > 4,1 ms */
    lcd_send_nibble(0x3u);  delay_ms(1);    /* > 100 us */
    lcd_send_nibble(0x3u);  delay_ms(1);
    lcd_send_nibble(0x2u);  delay_ms(1);

    lcd_cmd(CMD_FUNC_4BIT_2L);
    lcd_cmd(CMD_DISPLAY_OFF);
    lcd_cmd(CMD_CLEAR);
    lcd_cmd(CMD_ENTRY_INC);
    lcd_cmd(CMD_DISPLAY_ON);
    return lcd_fail ? -1 : 0;
}

int lcd_ok(void)
{
    return !lcd_fail;
}

void lcd_clear(void)
{
    lcd_cmd(CMD_CLEAR);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (uint8_t)((row ? 0x40u : 0x00u) + (col & 0x0Fu));  /* DDRAM 00h / 40h */
    lcd_cmd((uint8_t)(CMD_SET_DDRAM | addr));
}

void lcd_print(const char *s)
{
    while (*s) {
        lcd_send((uint8_t)*s++, PIN_RS);
    }
}

void lcd_backlight(int on)
{
    backlight = on ? PIN_BL : 0u;
    uint8_t b = backlight;
    (void)i2c1_write(lcd_addr, &b, 1);
}
