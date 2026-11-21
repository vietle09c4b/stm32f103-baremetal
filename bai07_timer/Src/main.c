/*
 * main.c - Bài 7: TIM2 time base + output compare, TIM3 input capture trên Blue Pill
 *
 * Nối dây: một dây jumper từ PA0 (TIM2_CH1, output compare toggle) sang PA6 (TIM3_CH1).
 *
 * - TIM2 update mỗi 1 s: phần mềm đảo LED PC13 khi thấy cờ UIF.
 * - TIM2_CH1 toggle PA0 mỗi 1 s bằng phần cứng -> sóng vuông chu kỳ 2 s.
 * - TIM3_CH1 bắt cạnh lên của PA0 trên PA6, đếm 10 kHz -> chu kỳ kỳ vọng 20000 tick.
 *
 * Clock: HSI 8 MHz sau reset, AHB /1, APB1 /1 -> TIM2CLK = TIM3CLK = 8 MHz.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "tim.h"

#define LED_PIN   13U              /* PC13, LED nối kiểu sink: 0 = sáng */

/* Quan sát bằng Live Expressions khi debug */
volatile uint32_t g_seconds;       /* số lần UIF của TIM2 */
volatile uint32_t g_captures;      /* số lần TIM3 bắt cạnh lên */
volatile uint16_t g_capture;       /* giá trị CCR1 lần bắt gần nhất */
volatile uint16_t g_period_ticks;  /* chu kỳ PA0 tính bằng tick 10 kHz */
volatile uint32_t g_period_ms;     /* chu kỳ PA0 tính bằng ms */
volatile uint32_t g_overcaptures;  /* số lần CC1OF = 1 */

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
        GPIOC->BRR  = (1UL << LED_PIN);
    } else {
        GPIOC->BSRR = (1UL << LED_PIN);
    }
}

int main(void)
{
    uint16_t prev = 0U;

    led_init();
    tim3_ic_pa6_init();
    tim2_1hz_oc_pa0_init();

    for (;;) {
        /* Time base: TIM2 tràn mỗi 1 s */
        if ((TIM2->SR & TIM_SR_UIF) != 0U) {
            TIM2->SR = ~TIM_SR_UIF;                  /* rc_w0: chỉ xóa UIF */
            led_toggle();
            g_seconds++;
        }

        /* Input capture: TIM3 bắt cạnh lên trên PA6 */
        if ((TIM3->SR & TIM_SR_CC1IF) != 0U) {
            uint16_t now = (uint16_t)TIM3->CCR1;      /* đọc CCR1 xóa CC1IF */
            if (g_captures > 0U) {
                g_period_ticks = (uint16_t)(now - prev);   /* modulo 2^16 */
                g_period_ms = ((uint32_t)g_period_ticks * 1000U) / TIM3_TICK_HZ;
            }
            prev = now;
            g_capture = now;
            g_captures++;

            if ((TIM3->SR & TIM_SR_CC1OF) != 0U) {
                TIM3->SR = ~TIM_SR_CC1OF;
                g_overcaptures++;
            }
        }
    }
}
