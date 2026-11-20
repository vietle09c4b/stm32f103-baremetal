/*
 * timebase.c - SysTick 1 ms (PM0056 mục 4.5)
 */
#include "timebase.h"
#include "stm32f103_reg.h"

static volatile uint32_t tick_ms;

void SysTick_Handler(void)
{
    tick_ms++;
}

void timebase_init(uint32_t hclk_hz)
{
    SYSTICK->LOAD = hclk_hz / 1000UL - 1UL;     /* 8 MHz -> 7999 */
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

uint32_t millis(void)
{
    return tick_ms;    /* đọc 32 bit trên Cortex-M3 là một lệnh, không bị xé */
}

void delay_ms(uint32_t ms)
{
    uint32_t start = tick_ms;
    while ((uint32_t)(tick_ms - start) < ms) { }
}
