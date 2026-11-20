/*
 * clock.c - đồng hồ lịch phần mềm
 */
#include "clock.h"

static int is_leap(uint16_t y)
{
    return ((y % 4u) == 0u && (y % 100u) != 0u) || (y % 400u) == 0u;
}

static uint8_t days_in_month(uint16_t y, uint8_t m)
{
    static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return (uint8_t)((m == 2u && is_leap(y)) ? 29u : d[m - 1u]);
}

static int two_digits(const char *p)
{
    int hi = (p[0] == ' ') ? 0 : p[0] - '0';
    int lo = p[1] - '0';
    if (hi < 0 || hi > 9 || lo < 0 || lo > 9) return -1;
    return hi * 10 + lo;
}

int clock_from_build(clock_t_ *c, const char *date, const char *time)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int m = 0;

    for (int i = 0; i < 12; i++) {
        if (date[0] == months[3 * i] && date[1] == months[3 * i + 1] &&
            date[2] == months[3 * i + 2]) {
            m = i + 1;
            break;
        }
    }
    int d  = two_digits(&date[4]);
    int y2 = two_digits(&date[9]);     /* hai số cuối của năm: "2026" -> 26 */
    int hh = two_digits(&time[0]);
    int mm = two_digits(&time[3]);
    int ss = two_digits(&time[6]);

    if (m == 0 || d < 1 || y2 < 0 || hh < 0 || hh > 23 || mm < 0 || mm > 59 ||
        ss < 0 || ss > 59) {
        return -1;
    }
    c->year  = (uint16_t)(2000 + y2);
    c->month = (uint8_t)m;
    c->day   = (uint8_t)d;
    c->hour  = (uint8_t)hh;
    c->min   = (uint8_t)mm;
    c->sec   = (uint8_t)ss;
    return 0;
}

void clock_tick(clock_t_ *c)
{
    if (++c->sec < 60u) return;
    c->sec = 0;
    if (++c->min < 60u) return;
    c->min = 0;
    if (++c->hour < 24u) return;
    c->hour = 0;
    if (++c->day <= days_in_month(c->year, c->month)) return;
    c->day = 1;
    if (++c->month <= 12u) return;
    c->month = 1;
    c->year++;
}

uint8_t clock_weekday(const clock_t_ *c)
{
    /* Thuật toán Sakamoto, đúng với lịch Gregory */
    static const uint8_t t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    uint16_t y = c->year;
    if (c->month < 3u) y--;
    return (uint8_t)((y + y / 4u - y / 100u + y / 400u + t[c->month - 1u] + c->day) % 7u);
}

static void put2(char *p, uint8_t v)
{
    p[0] = (char)('0' + v / 10u);
    p[1] = (char)('0' + v % 10u);
}

void clock_format(const clock_t_ *c, char line1[17], char line2[17])
{
    /* LCD HD44780 (ROM A00) không có chữ tiếng Việt có dấu */
    static const char *const wd[7] = { "CN", "T2", "T3", "T4", "T5", "T6", "T7" };
    const char *w = wd[clock_weekday(c)];

    for (int i = 0; i < 16; i++) { line1[i] = ' '; line2[i] = ' '; }
    line1[16] = '\0';
    line2[16] = '\0';

    /* "    HH:MM:SS    " */
    put2(&line1[4], c->hour);  line1[6] = ':';
    put2(&line1[7], c->min);   line1[9] = ':';
    put2(&line1[10], c->sec);

    /* "T4  DD/MM/YYYY  " */
    line2[0] = w[0];
    line2[1] = w[1];
    put2(&line2[4], c->day);    line2[6] = '/';
    put2(&line2[7], c->month);  line2[9] = '/';
    put2(&line2[10], (uint8_t)(c->year / 100u));
    put2(&line2[12], (uint8_t)(c->year % 100u));
}
