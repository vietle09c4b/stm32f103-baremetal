/*
 * main.c - Bài 10: CAN (bxCAN) trên Blue Pill (STM32F103C8T6), mức thanh ghi
 *
 * Nối dây (giai đoạn B, Normal mode):
 *   PB9 (CAN_TX) -> TXD  module TJA1050/MCP2551
 *   PB8 (CAN_RX) <- RXD  module (mức 5 V, PB8 là chân FT)
 *   5V, GND      -> VCC, GND module (Blue Pill cấp qua cổng micro-USB)
 *   CANH, CANL   -> D-Sub PCAN-USB chân 7 (CAN_H), chân 2 (CAN_L), GND chân 3
 *   Hai đầu bus có điện trở 120 ohm.
 * PCAN-View: 500 kbit/s.
 *
 * Chương trình:
 *   - Mỗi 100 ms gửi ID 0x123, DLC 8: byte0 = bộ đếm, byte1..7 = 0x11..0x77.
 *   - Nhận ID 0x321 (bộ lọc bank 0, FIFO0, ngắt RX0): byte0 bit0 = 1 -> LED PC13 sáng.
 *   - Chế độ CAN_MODE_LOOPBACK_SILENT (giai đoạn A): bxCAN tự nhận lại frame 0x123,
 *     LED đảo mỗi lần nhận, không cần transceiver.
 *
 * Clock: HSE 8 MHz, không PLL -> SYSCLK = HCLK = PCLK1 = 8 MHz.
 * HSI không đủ chính xác cho CAN (sai số cho phép của cấu hình 500 kbit/s cỡ 0,3 %).
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "can.h"

#ifndef APP_CAN_MODE
#define APP_CAN_MODE   CAN_MODE_NORMAL              /* CAN_MODE_LOOPBACK_SILENT: tự kiểm tra không cần transceiver */
#endif

#define HSE_HZ         8000000UL
#define LED_PIN        13U                          /* PC13, LED nối kiểu sink: 0 = sáng */
#define ID_TX          0x123U
#define ID_RX          0x321U
#define TX_PERIOD_MS   100U

/* ---- Biến quan sát bằng Live Expressions ---- */
volatile uint32_t g_ms;
volatile uint32_t g_clock_hse;          /* 1 = đang chạy HSE */
volatile int32_t  g_can_init_rc = 99;   /* 0 = Normal/Test mode OK, -1/-2 = lỗi INAK */
volatile uint32_t g_tx_count;
volatile uint32_t g_esr;                /* bản sao CAN_ESR: REC, TEC, LEC, BOFF, EPVF, EWGF */

void SysTick_Handler(void)
{
    g_ms++;
}

/* RM0008 7.3.1 (HSEON/HSERDY), 7.3.2 (SW/SWS). Không bật PLL nên Flash 0 wait state. */
static uint32_t clock_use_hse(void)
{
    RCC->CR |= RCC_CR_HSEON;
    for (uint32_t t = 0; t < 200000UL; t++) {
        if (RCC->CR & RCC_CR_HSERDY) {
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;
            while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) { }
            return 1U;
        }
    }
    RCC->CR &= ~RCC_CR_HSEON;
    return 0U;
}

static void systick_1ms_init(void)
{
    SysTick->LOAD = (HSE_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = (1UL << LED_PIN);                 /* tắt */
    uint32_t crh = GPIOC->CRH;
    crh &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    crh |=  GPIO_CRH_MODE13_1;                      /* output push-pull 2 MHz */
    GPIOC->CRH = crh;
}

static void led_set(uint32_t on)
{
    GPIOC->BSRR = on ? (1UL << (LED_PIN + 16U)) : (1UL << LED_PIN);
}

int main(void)
{
    g_clock_hse = clock_use_hse();
    led_init();
    systick_1ms_init();

    if (g_clock_hse == 0U) {
        for (;;) { }                                /* không có HSE: dừng, không chạy CAN với HSI */
    }

    g_can_init_rc = can1_init(CAN_BTR_500K_PCLK8M, APP_CAN_MODE, ID_RX, ID_TX);
    if (g_can_init_rc != 0) {
        led_set(1U);                                /* LED sáng liên tục = không vào được Normal mode */
        for (;;) { }
    }

    uint8_t  data[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint32_t next_ms = g_ms + TX_PERIOD_MS;
    uint32_t led = 0U;

    for (;;) {
        if ((int32_t)(g_ms - next_ms) >= 0) {
            next_ms += TX_PERIOD_MS;
            data[0] = (uint8_t)g_tx_count;
            if (can1_send_std(ID_TX, data, 8U) >= 0) {
                g_tx_count++;
            }
        }

        can1_poll_tx_status();
        g_esr = CAN1->ESR;

        if (g_can_rx_new != 0U) {
            g_can_rx_new = 0U;
            if (g_can_rx_last.id == ID_RX) {
                led_set(g_can_rx_last.data[0] & 0x01U);     /* PCAN-View điều khiển LED */
            } else if (g_can_rx_last.id == ID_TX) {
                led ^= 1U;                                  /* loopback: đảo LED mỗi frame */
                led_set(led);
            }
        }
    }
}
