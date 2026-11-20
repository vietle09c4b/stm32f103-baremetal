/*
 * clock.h - đồng hồ lịch phần mềm (không phụ thuộc phần cứng, test được trên PC)
 */
#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef struct {
    uint16_t year;    /* 2000..2099 */
    uint8_t  month;   /* 1..12 */
    uint8_t  day;     /* 1..31 */
    uint8_t  hour;    /* 0..23 */
    uint8_t  min;     /* 0..59 */
    uint8_t  sec;     /* 0..59 */
} clock_t_;

/* Lấy thời điểm biên dịch từ chuỗi __DATE__ ("Nov 18 2026") và __TIME__ ("14:05:09") */
int  clock_from_build(clock_t_ *c, const char *date, const char *time);

/* Tăng 1 giây, tự chuyển phút, giờ, ngày, tháng, năm (có năm nhuận) */
void clock_tick(clock_t_ *c);

/* Thứ trong tuần: 0 = Chủ nhật .. 6 = Thứ bảy */
uint8_t clock_weekday(const clock_t_ *c);

/* Dòng 1: "    14:05:09    ", dòng 2: "T4  18/11/2026  "; mỗi buffer >= 17 byte */
void clock_format(const clock_t_ *c, char line1[17], char line2[17]);

#endif /* CLOCK_H */
