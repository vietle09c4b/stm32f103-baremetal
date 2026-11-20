/*
 * timebase.h - nhịp hệ thống 1 ms bằng SysTick
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

void     timebase_init(uint32_t hclk_hz);   /* SysTick ngắt mỗi 1 ms */
uint32_t millis(void);                      /* số ms từ lúc khởi động */
void     delay_ms(uint32_t ms);

#endif /* TIMEBASE_H */
