/*
 * main.c - Bài 8: ngắt trên Blue Pill (STM32F103C8T6)
 *
 * Nút nhấn rời nối PA0 với GND (pull-up nội), LED PC13 trên board.
 *
 * - SysTick_Handler   : g_ms tăng mỗi 1 ms.
 * - TIM2_IRQHandler   : mỗi 1 s đảo LED PC13.
 * - EXTI0_IRQHandler  : mỗi cạnh xuống trên PA0; chống dội 50 ms bằng g_ms.
 * Vòng for trong main() để trống: mọi công việc nằm trong các trình phục vụ ngắt.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "irq_drivers.h"

#define LED_PIN        13U         /* PC13, LED nối kiểu sink: 0 = sáng */
#define DEBOUNCE_MS    50U

/* Quan sát bằng Live Expressions */
volatile uint32_t g_ms;            /* số lần SysTick_Handler chạy */
volatile uint32_t g_tim2_ticks;    /* số lần TIM2 update */
volatile uint32_t g_exti_edges;    /* mọi cạnh xuống trên PA0, kể cả dội */
volatile uint32_t g_presses;       /* lần nhấn hợp lệ sau chống dội */
static uint32_t   s_last_press_ms;

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;                      /* output push-pull 2 MHz */
    GPIOC->CRH = crh;
}

static void led_toggle(void)
{
    if ((GPIOC->ODR & (1UL << LED_PIN)) != 0UL) {
        GPIOC->BRR  = (1UL << LED_PIN);
    } else {
        GPIOC->BSRR = (1UL << LED_PIN);
    }
}

/* ---- Trình phục vụ ngắt: tên phải trùng tên trong vector table (startup .s) ---- */

void SysTick_Handler(void)
{
    g_ms++;                                         /* cờ COUNTFLAG không cần xóa */
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM2->SR = ~TIM_SR_UIF;                     /* xóa cờ trước, tránh vào lại ISR */
        led_toggle();
        g_tim2_ticks++;
    }
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        EXTI->PR = EXTI_PR_PR0;                     /* rc_w1: ghi 1 xóa đúng line 0 */
        g_exti_edges++;
        if ((g_ms - s_last_press_ms) >= DEBOUNCE_MS) {
            s_last_press_ms = g_ms;
            g_presses++;
        }
    }
}

int main(void)
{
    __disable_irq();                                /* PRIMASK = 1 trong lúc cấu hình */
    led_init();
    systick_1ms_irq_init();
    tim2_1hz_irq_init();
    exti0_pa0_falling_init();
    __enable_irq();                                 /* PRIMASK = 0 */

    for (;;) {
        /* không có polling: CPU chỉ chạy ISR khi có sự kiện */
    }
}
