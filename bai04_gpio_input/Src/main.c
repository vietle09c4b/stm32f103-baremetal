/*
 * main.c - Bài 4: GPIO input trên Blue Pill (STM32F103C8T6), mức thanh ghi
 *
 * Nối dây: nút nhấn 2 chân giữa PA0 và GND. Không cần điện trở ngoài:
 * PA0 dùng pull-up nội (RM0008 Table 20: CNF0 = 10, MODE0 = 00, ODR0 = 1).
 *   Không nhấn: PA0 = 1 (pull-up)   -> LED PC13 tắt
 *   Nhấn      : PA0 = 0 (nối GND)   -> LED PC13 sáng (LED nối kiểu sink)
 *
 * Header: CMSIS của ST (stm32f1xx.h -> stm32f103xb.h, symbol STM32F103xB).
 * Header chỉ định nghĩa địa chỉ, struct thanh ghi và tên bit; không có hàm thư viện.
 * Clock: giữ HSI 8 MHz sau reset (HCLK = PCLK2 = 8 MHz), SysTick 1 ms.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "debounce.h"

#define HCLK_HZ    8000000UL      /* HSI sau reset, không bật PLL */
#define BTN_PIN    0U             /* PA0 */
#define LED_PIN    13U            /* PC13 */

/* Biến toàn cục để quan sát bằng Live Expressions khi debug */
volatile uint32_t g_ms;           /* đếm mili giây từ SysTick */
volatile uint32_t g_raw_falls;    /* số cạnh xuống đọc thô từ IDR (có cả dội) */
volatile uint32_t g_presses;      /* số lần nhấn sau khi chống dội */
volatile uint32_t g_btn_raw;      /* mức thô PA0 lần đọc gần nhất */
volatile uint32_t g_btn_stable;   /* mức đã lọc */

void SysTick_Handler(void)
{
    g_ms++;
}

static void systick_init_1ms(void)
{
    SysTick->LOAD = HCLK_HZ / 1000UL - 1UL;               /* 7999 (PM0056 4.5) */
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |          /* nguồn HCLK */
                    SysTick_CTRL_TICKINT_Msk   |          /* ngắt mỗi lần về 0 */
                    SysTick_CTRL_ENABLE_Msk;
}

static void gpio_init(void)
{
    /* 1. Bật clock GPIOA và GPIOC trên APB2 (RCC_APB2ENR bit 2, bit 4) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;                                    /* chờ ghi xong */

    /* 2. PA0: input pull-up/pull-down (CNF0 = 10, MODE0 = 00) -> 4 bit = 1000 */
    uint32_t crl = GPIOA->CRL;
    crl &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);              /* xóa 4 bit, reset là 0100 */
    crl |=  GPIO_CRL_CNF0_1;                               /* CNF0 = 10 */
    GPIOA->CRL = crl;

    /* 3. Chọn pull-up: ODR0 = 1 (ODR0 = 0 sẽ là pull-down) */
    GPIOA->BSRR = (1UL << BTN_PIN);

    /* 4. PC13: output push-pull 2 MHz (CNF13 = 00, MODE13 = 10), LED tắt */
    GPIOC->BSRR = (1UL << LED_PIN);
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;
    GPIOC->CRH = crh;
}

static inline uint8_t button_read_raw(void)
{
    return (GPIOA->IDR & (1UL << BTN_PIN)) ? 1u : 0u;      /* IDR0 */
}

int main(void)
{
    gpio_init();
    systick_init_1ms();

    debounce_t db;
    uint8_t prev_raw = button_read_raw();
    debounce_init(&db, prev_raw, g_ms);

    for (;;) {
        uint8_t raw = button_read_raw();
        uint32_t now = g_ms;

        if (prev_raw == 1u && raw == 0u) {
            g_raw_falls++;                                  /* đếm cả xung dội */
        }
        prev_raw = raw;

        db_event_t ev = debounce_update(&db, raw, now);
        if (ev == DB_FALL) {
            g_presses++;
        }

        /* LED theo mức đã lọc: nhấn (0) -> BRR xóa PC13 -> LED sáng */
        if (db.stable == 0u) GPIOC->BRR  = (1UL << LED_PIN);
        else                 GPIOC->BSRR = (1UL << LED_PIN);

        g_btn_raw    = raw;
        g_btn_stable = db.stable;
    }
}
