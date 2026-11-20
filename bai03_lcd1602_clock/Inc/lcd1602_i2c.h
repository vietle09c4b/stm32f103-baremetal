/*
 * lcd1602_i2c.h - LCD 16x2 HD44780 qua module I2C PCF8574 (giao tiếp 4 bit)
 *
 * Nối chân phổ biến trên module PCF8574 bán kèm LCD1602:
 *   P0 = RS, P1 = RW, P2 = EN, P3 = đèn nền, P4..P7 = D4..D7
 * Địa chỉ 7 bit: 0x27 (chip PCF8574T) hoặc 0x3F (chip PCF8574AT),
 * thay đổi được bằng ba cầu hàn A0..A2.
 */
#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <stdint.h>

/* Dò địa chỉ trong hai dải 0x20..0x27 và 0x38..0x3F; trả 0 nếu không thấy */
uint8_t lcd_detect(void);

/* Khởi tạo LCD theo HD44780 Figure 24 (4 bit), trả 0 nếu OK */
int  lcd_init(uint8_t addr7);

void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);   /* row 0..1, col 0..15 */
void lcd_print(const char *s);
void lcd_backlight(int on);

/* 0 nếu từ lần lcd_init gần nhất đã có lệnh ghi I2C thất bại */
int  lcd_ok(void);

#endif /* LCD1602_I2C_H */
