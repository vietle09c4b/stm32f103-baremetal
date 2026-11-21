/*
 * main.c - Bài 6: delay chính xác bằng SysTick trên Blue Pill (STM32F103C8T6)
 *
 * Mỗi 1000 ms đảo LED PC13 -> LED sáng 1 s, tắt 1 s (chu kỳ 2 s).
 * Kiểm chứng bằng debug: Live Expressions (g_seconds, g_calib) và SFRs SysTick.
 *
 * Clock: giữ HSI 8 MHz sau reset -> HCLK = 8 MHz.
 * Header: CMSIS của ST (stm32f1xx.h -> stm32f103xb.h -> core_cm3.h chứa SysTick_Type).
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "systick.h"

#define LED_PIN       13U          /* PC13, LED nối kiểu sink: 0 = sáng */

/* Quan sát bằng Live Expressions khi debug */
volatile uint32_t g_seconds;       /* số lần delay 1000 ms đã xong */
volatile uint32_t g_calib;         /* giá trị STK_CALIB đọc lúc khởi động */

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);                 /* LED tắt trước khi chuyển sang output */
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;                      /* output push-pull 2 MHz */
    GPIOC->CRH = crh;
}

static void led_toggle(void)
{
    if ((GPIOC->ODR & (1UL << LED_PIN)) != 0UL) {
        GPIOC->BRR  = (1UL << LED_PIN);             /* đang tắt -> bật */
    } else {
        GPIOC->BSRR = (1UL << LED_PIN);             /* đang sáng -> tắt */
    }
}

int main(void)
{
    led_init();
    g_calib = SysTick->CALIB;

    for (;;) {
        systick_delay_ms(1000U);
        led_toggle();
        g_seconds++;
    }
}
