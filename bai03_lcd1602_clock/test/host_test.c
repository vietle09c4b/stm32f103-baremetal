/*
 * host_test.c - kiểm tra trên PC: lịch phần mềm và chuỗi byte gửi tới PCF8574
 * Build: gcc -std=c99 -Wall -Wextra -fsanitize=address,undefined -I../Inc \
 *            host_test.c ../Src/clock.c ../Src/lcd1602_i2c.c -o host_test
 */
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "lcd1602_i2c.h"
#include "i2c1.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- I2C và delay giả lập ---- */
static uint8_t log_buf[512];
static unsigned log_len;
static uint8_t present_addr = 0x3F;
static unsigned delay_total;

i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    if (addr7 != present_addr) return I2C_ERR_NACK;
    for (uint32_t i = 0; i < len && log_len < sizeof log_buf; i++) log_buf[log_len++] = data[i];
    return I2C_OK;
}
i2c_status_t i2c1_probe(uint8_t addr7) { return i2c1_write(addr7, NULL, 0); }
void delay_ms(uint32_t ms) { delay_total += ms; }

static void test_clock(void)
{
    clock_t_ c;
    char l1[17], l2[17];

    CHECK(clock_from_build(&c, "Nov 18 2026", "14:05:09") == 0);
    CHECK(c.year == 2026 && c.month == 11 && c.day == 18);
    CHECK(c.hour == 14 && c.min == 5 && c.sec == 9);
    CHECK(clock_weekday(&c) == 3);                 /* 18/11/2026 là thứ Tư */
    clock_format(&c, l1, l2);
    CHECK(strcmp(l1, "    14:05:09    ") == 0);
    CHECK(strcmp(l2, "T4  18/11/2026  ") == 0);
    CHECK(strlen(l1) == 16 && strlen(l2) == 16);

    CHECK(clock_from_build(&c, "Jan  5 2027", "09:00:00") == 0);   /* ngày 1 chữ số có dấu cách */
    CHECK(c.day == 5 && c.month == 1 && c.year == 2027);
    CHECK(clock_from_build(&c, "Xyz 18 2026", "14:05:09") != 0);

    c = (clock_t_){ 2026, 12, 31, 23, 59, 59 };
    clock_tick(&c);
    CHECK(c.year == 2027 && c.month == 1 && c.day == 1 && c.hour == 0 && c.min == 0 && c.sec == 0);

    c = (clock_t_){ 2028, 2, 28, 23, 59, 59 };      /* 2028 nhuận */
    clock_tick(&c);
    CHECK(c.month == 2 && c.day == 29);
    c = (clock_t_){ 2027, 2, 28, 23, 59, 59 };      /* 2027 không nhuận */
    clock_tick(&c);
    CHECK(c.month == 3 && c.day == 1);
    c = (clock_t_){ 2026, 4, 30, 23, 59, 59 };
    clock_tick(&c);
    CHECK(c.month == 5 && c.day == 1);

    c = (clock_t_){ 2026, 1, 1, 0, 0, 0 };          /* 86 400 giây = đúng 1 ngày */
    for (int i = 0; i < 86400; i++) clock_tick(&c);
    CHECK(c.day == 2 && c.hour == 0 && c.min == 0 && c.sec == 0);

    c = (clock_t_){ 2026, 11, 22, 0, 0, 0 };
    CHECK(clock_weekday(&c) == 0);                 /* 22/11/2026 là Chủ nhật */
    c = (clock_t_){ 2000, 1, 1, 0, 0, 0 };
    CHECK(clock_weekday(&c) == 6);                 /* 1/1/2000 là thứ Bảy */
}

static void test_lcd(void)
{
    CHECK(lcd_detect() == 0x3F);
    present_addr = 0x27;
    CHECK(lcd_detect() == 0x27);
    present_addr = 0x22;
    CHECK(lcd_detect() == 0x22);
    present_addr = 0x50;
    CHECK(lcd_detect() == 0);
    CHECK(lcd_init(0x27) != 0);

    present_addr = 0x27;
    log_len = 0;
    delay_total = 0;
    CHECK(lcd_init(0x27) == 0);
    /* 4 nibble khởi tạo x 2 byte + 5 lệnh x 4 byte */
    CHECK(log_len == 8 + 20);
    const uint8_t init_nib[8] = { 0x3C, 0x38, 0x3C, 0x38, 0x3C, 0x38, 0x2C, 0x28 };
    CHECK(memcmp(log_buf, init_nib, 8) == 0);
    /* Function set 0x28: nibble cao 0x2, nibble thấp 0x8; EN = bit 2, BL = bit 3 */
    const uint8_t func_set[4] = { 0x2C, 0x28, 0x8C, 0x88 };
    CHECK(memcmp(&log_buf[8], func_set, 4) == 0);
    CHECK(delay_total >= 50 + 5 + 1 + 1 + 1 + 2);

    log_len = 0;
    lcd_set_cursor(1, 0);                          /* lệnh 0xC0 */
    const uint8_t cur[4] = { 0xCC, 0xC8, 0x0C, 0x08 };
    CHECK(log_len == 4 && memcmp(log_buf, cur, 4) == 0);

    log_len = 0;
    lcd_print("A");                                /* 'A' = 0x41, RS = 1 */
    const uint8_t a[4] = { 0x4D, 0x49, 0x1D, 0x19 };
    CHECK(log_len == 4 && memcmp(log_buf, a, 4) == 0);
    for (unsigned i = 0; i < log_len; i++) CHECK((log_buf[i] & 0x02u) == 0);   /* RW luôn 0 */
}

int main(void)
{
    test_clock();
    test_lcd();
    printf("%d/%d checks passed\n", checks - fails, checks);
    return fails ? 1 : 0;
}
