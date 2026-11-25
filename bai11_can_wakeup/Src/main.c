/*
 * main.c - Bài 11: đánh thức STM32F103C8 (Blue Pill) từ Stop mode qua bus CAN, mức thanh ghi
 *
 * Phần cứng giữ nguyên bài 10:
 *   PB9 (CAN_TX) -> TXD module TJA1050, PB8 (CAN_RX) <- RXD module (PB8 chịu 5 V)
 *   CANH/CANL -> PCAN-USB, 120 ohm hai đầu bus, PCAN-View 500 kbit/s.
 *
 * Chương trình:
 *   RUN  : mỗi 100 ms gửi ID 0x123, DLC 8: byte0 = bộ đếm, byte1 = số lần thức, byte2 = mã kết quả
 *          bước thức gần nhất. Nhận ID 0x321: byte0 = 0x00/0x01 tắt/bật LED PC13, byte0 = 0x02 -> ngủ.
 *   SLEEP: chờ 3 transmit mailbox trống -> bxCAN Sleep -> tắt LED, tắt SysTick -> EXTI8 cạnh xuống
 *          trên PB8 -> Stop mode (regulator low-power).
 *   WAKE : frame bất kỳ trên bus kéo RXD xuống LOW -> EXTI8 -> MCU thức với HSI -> bật lại HSE ->
 *          bxCAN rời Sleep (đồng bộ 11 bit recessive) -> bật SysTick -> RUN.
 *
 * Clock: HSE 8 MHz, không PLL -> SYSCLK = HCLK = PCLK1 = 8 MHz (BTR 500 kbit/s tính theo 8 MHz).
 * HSI (DS5319: -1,1..+1,8 % ở 25 °C) không đủ chính xác cho CAN, nên phải về HSE trước khi xóa SLEEP.
 */
#include <stdint.h>
#include "stm32f1xx.h"
#include "can.h"
#include "lowpower.h"

/* #define WAKE_DIAG */   /* bật để chẩn đoán bằng LED PC13: sáng = đã ra khỏi WFI, tắt = HSE đã chạy, nháy = thức nhưng lỗi */

#define HSE_HZ         8000000UL
#define LED_PIN        13U                          /* PC13, LED nối kiểu sink: 0 = sáng */
#define ID_TX          0x123U
#define ID_RX          0x321U
#define TX_PERIOD_MS   100U
#define CMD_SLEEP      0x02U

/* ---- Biến quan sát bằng Live Expressions ---- */
volatile uint32_t g_ms;
volatile uint32_t g_clock_hse;          /* 1 = đang chạy HSE */
volatile int32_t  g_can_init_rc = 99;
volatile uint32_t g_tx_count;
volatile uint32_t g_esr;
volatile uint32_t g_sleep_count;        /* số lần vào Stop */
volatile uint32_t g_wake_count;         /* số lần thức */
volatile uint32_t g_sws_after_wake;     /* RCC_CFGR.SWS ngay sau WFI: 0 = HSI, 1 = HSE */
volatile uint32_t g_cyc_hse;            /* chu kỳ từ lúc thức tới khi chạy lại HSE (DWT) */
volatile uint32_t g_cyc_sync;           /* chu kỳ chờ bxCAN rời Sleep (SLAK = 0) */
volatile uint32_t g_wkui;               /* MSR.WKUI đọc được sau khi thức */
volatile int32_t  g_wake_rc;            /* 0 = OK, -1 = không có HSE, -2 = bxCAN không rời Sleep */
volatile uint32_t g_first_rx_id;        /* ID frame đầu tiên nhận được sau khi thức */

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

static void systick_1ms_start(void)
{
    SysTick->LOAD = (HSE_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void systick_stop(void)
{
    SysTick->CTRL = 0UL;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;             /* SysTick pending cũng chặn việc vào Stop */
}

static void dwt_cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
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

/* Toàn bộ chuỗi ngủ - thức. Trả 0 nếu bus CAN chạy lại bình thường. */
static int32_t sleep_until_can_activity(void)
{
    (void)can1_wait_tx_empty();                     /* không bỏ dở frame đang chờ gửi */
    if (can1_sleep() != 0) return -3;

    led_set(0U);
    systick_stop();

    __disable_irq();                                /* tránh mất cạnh xuống giữa lúc cấu hình và WFI */
    exti8_pb8_arm();
    g_sleep_count++;
    stop_mode_enter();                              /* ---- MCU dừng ở đây cho tới khi có cạnh xuống ---- */

    uint32_t c0 = DWT->CYCCNT;
#ifdef WAKE_DIAG
    led_set(1U);                                    /* chẩn đoán: LED sáng = đã ra khỏi WFI */
#endif
    g_sws_after_wake = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
    g_clock_hse = clock_use_hse();
    uint32_t c1 = DWT->CYCCNT;
#ifdef WAKE_DIAG
    led_set(0U);                                    /* chẩn đoán: LED tắt = HSE đã chạy */
#endif
    __enable_irq();                                 /* EXTI9_5_IRQHandler chạy tại đây */

    g_wake_count++;
    g_cyc_hse = c1 - c0;
    if (g_clock_hse == 0U) return -1;

    g_wkui = (CAN1->MSR & CAN_MSR_WKUI) ? 1U : 0U;
    CAN1->MSR = CAN_MSR_WKUI;                       /* rc_w1: ghi 1 để xóa */

    uint32_t c2 = DWT->CYCCNT;
    int rc = can1_wakeup();
    g_cyc_sync = DWT->CYCCNT - c2;
    systick_1ms_start();
    return (rc == 0) ? 0 : -2;
}

int main(void)
{
    g_clock_hse = clock_use_hse();
    led_init();
    systick_1ms_start();
    dwt_cycle_counter_init();

    if (g_clock_hse == 0U) {
        for (;;) { }                                /* không có HSE: dừng, không chạy CAN với HSI */
    }

    g_can_init_rc = can1_init(CAN_BTR_500K_PCLK8M, CAN_MODE_NORMAL, ID_RX, ID_TX);
    if (g_can_init_rc != 0) {
        led_set(1U);                                /* LED sáng liên tục = không vào được Normal mode */
        for (;;) { }
    }

    uint8_t  data[8] = {0x00, 0x00, 0x00, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint32_t next_ms = g_ms + TX_PERIOD_MS;
    uint32_t wait_first_rx = 0U;

    for (;;) {
        if ((int32_t)(g_ms - next_ms) >= 0) {
            next_ms += TX_PERIOD_MS;
            data[0] = (uint8_t)g_tx_count;
            data[1] = (uint8_t)g_wake_count;
            data[2] = (uint8_t)g_wake_rc;
            if (can1_send_std(ID_TX, data, 8U) >= 0) {
                g_tx_count++;
            }
#ifdef WAKE_DIAG
            if (g_wake_rc != 0) { GPIOC->ODR ^= (1UL << LED_PIN); }   /* chẩn đoán: nháy = thức nhưng lỗi */
#endif
        }

        can1_poll_tx_status();
        g_esr = CAN1->ESR;

        if (g_can_rx_new != 0U) {
            g_can_rx_new = 0U;
            if (wait_first_rx != 0U) {
                wait_first_rx = 0U;
                g_first_rx_id = g_can_rx_last.id;
            }
            if (g_can_rx_last.id == ID_RX) {
                if (g_can_rx_last.data[0] == CMD_SLEEP) {
                    g_wake_rc = sleep_until_can_activity();
                    next_ms = g_ms + TX_PERIOD_MS;
                    wait_first_rx = 1U;
                } else {
                    led_set(g_can_rx_last.data[0] & 0x01U);
                }
            }
        }
    }
}
