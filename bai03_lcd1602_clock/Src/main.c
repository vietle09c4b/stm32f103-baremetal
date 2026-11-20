/*
 * main.c - Bài 3: đồng hồ trên LCD1602 qua module I2C PCF8574 (Blue Pill STM32F103C8T6)
 *
 * Nối dây:
 *   Module I2C  ->  Blue Pill / ST-LINK V2
 *   GND         ->  GND
 *   VCC         ->  5V (chân 5V của ST-LINK V2, hoặc chân 5V Blue Pill khi cắm USB)
 *   SDA         ->  PB7
 *   SCL         ->  PB6
 *
 * Nguồn thời gian: thạch anh HSE 8 MHz trên Blue Pill (sai số cỡ vài chục ppm),
 * SysTick 1 ms. Giờ ban đầu lấy theo thời điểm biên dịch (__DATE__, __TIME__),
 * nên LCD chậm hơn giờ thật đúng bằng khoảng thời gian từ lúc build tới lúc chạy.
 * Chỉnh CLOCK_OFFSET_S nếu muốn bù khoảng trễ đó.
 *
 * LED PC13 đảo trạng thái mỗi giây; nháy nhanh 10 Hz nếu không tìm thấy LCD.
 */
#include <stdint.h>
#include "stm32f103_reg.h"
#include "timebase.h"
#include "i2c1.h"
#include "lcd1602_i2c.h"
#include "clock.h"

#ifndef CLOCK_OFFSET_S
#define CLOCK_OFFSET_S  0U
#endif

#define HSE_HZ        8000000UL
#define HSI_HZ        8000000UL
#define LED_PIN       13U

/*
 * Chuyển SYSCLK sang HSE 8 MHz (RM0008 mục 7.3.1, 7.3.2). Không bật PLL nên
 * HCLK = PCLK1 = PCLK2 = 8 MHz và Flash không cần wait state.
 * Nếu thạch anh không dao động, giữ HSI 8 MHz (kém chính xác hơn, cỡ 1 %).
 */
static uint32_t clock_use_hse(void)
{
    RCC->CR |= RCC_CR_HSEON;
    for (uint32_t t = 0; t < 200000UL; t++) {
        if (RCC->CR & RCC_CR_HSERDY) {
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSE;
            while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSE) { }
            return HSE_HZ;
        }
    }
    RCC->CR &= ~RCC_CR_HSEON;
    return HSI_HZ;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    uint32_t crh = GPIOC->CRH;
    crh &= ~(0xFUL << 20);
    crh |=  (0x2UL << 20);            /* PC13 output push-pull 2 MHz */
    GPIOC->CRH = crh;
    GPIOC->BSRR = (1UL << LED_PIN);   /* LED tắt (nối kiểu sink) */
}

static void led_toggle(void)
{
    if (GPIOC->ODR & (1UL << LED_PIN)) GPIOC->BRR  = (1UL << LED_PIN);
    else                               GPIOC->BSRR = (1UL << LED_PIN);
}

int main(void)
{
    uint32_t hclk = clock_use_hse();
    timebase_init(hclk);
    led_init();
    i2c1_init(hclk);                  /* APB1 không chia: PCLK1 = HCLK */

    /* Chưa thấy LCD: LED nháy nhanh và thử lại cho tới khi module trả ACK */
    uint8_t addr = 0u;
    while (addr == 0u || lcd_init(addr) != 0) {
        addr = lcd_detect();
        for (int i = 0; i < 10; i++) { led_toggle(); delay_ms(50); }
    }

    clock_t_ now;
    if (clock_from_build(&now, __DATE__, __TIME__) != 0) {
        now = (clock_t_){ 2026, 1, 1, 0, 0, 0 };
    }
#if CLOCK_OFFSET_S > 0
    for (uint32_t i = 0; i < CLOCK_OFFSET_S; i++) clock_tick(&now);
#endif

    char l1[17], l2[17];
    uint32_t next = millis();

    for (;;) {
        if ((int32_t)(millis() - next) >= 0) {
            next += 1000U;

            /* Có lỗi I2C (cắm lại dây, nhiễu): khởi tạo lại LCD rồi vẽ lại */
            if (!lcd_ok()) {
                (void)lcd_init(addr);
            }

            clock_format(&now, l1, l2);
            lcd_set_cursor(0, 0);  lcd_print(l1);
            lcd_set_cursor(1, 0);  lcd_print(l2);
            led_toggle();

            clock_tick(&now);
        }
    }
}
