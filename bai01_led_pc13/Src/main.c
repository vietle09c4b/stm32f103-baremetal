/*
 * bai01_led_pc13 - Bare-metal STM32F103C8T6 (Blue Pill): tạo thanh ghi từ địa chỉ và nhấp nháy LED PC13.
 * vietc4b.com, series bare-metal STM32F103, bài 1.
 *
 * Nguồn: RM0008 Rev 21 (memory map 3.3, RCC_APB2ENR 7.3.7, GPIO 9.2), DS5319 Rev 20 (ghi chú PC13..PC15).
 * Blue Pill: LED người dùng nối PC13 kiểu sink -> PC13 = 0 thì LED sáng.
 *
 * USE_STRUCT = 0: thanh ghi tạo bằng base + offset và ép kiểu con trỏ volatile.
 * USE_STRUCT = 1: cùng thanh ghi nhưng truy cập qua struct (cách header CMSIS của ST làm).
 */
#include <stdint.h>

#ifndef USE_STRUCT
#define USE_STRUCT 0
#endif

/* ---- Địa chỉ (RM0008 Table 3, memory map) ---- */
#define PERIPH_BASE        0x40000000UL
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x00010000UL)   /* 0x4001 0000 */
#define AHBPERIPH_BASE     (PERIPH_BASE + 0x00020000UL)   /* 0x4002 0000 */
#define GPIOC_BASE         (APB2PERIPH_BASE + 0x1000UL)   /* 0x4001 1000 */
#define RCC_BASE           (AHBPERIPH_BASE + 0x1000UL)    /* 0x4002 1000 */

/* ---- Bit ---- */
#define RCC_APB2ENR_IOPCEN (1UL << 4)                     /* RM0008 7.3.7 */
#define LED_PIN            13U
#define CRH_SHIFT(pin)     (((pin) - 8U) * 4U)            /* PC13 -> bit 23:20 */
#define CRH_OUT_PP_2MHZ    0x2UL                          /* CNF = 00 (push-pull), MODE = 10 (2 MHz) */

#if USE_STRUCT == 0
/* Thanh ghi = *(volatile uint32_t *)(base + offset). volatile: mỗi lần đọc/ghi đều thật sự truy cập bộ nhớ. */
#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE   + 0x18UL))
#define GPIOC_CRH     (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))
#define GPIOC_ODR     (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))
#define GPIOC_BSRR    (*(volatile uint32_t *)(GPIOC_BASE + 0x10UL))
#define GPIOC_BRR     (*(volatile uint32_t *)(GPIOC_BASE + 0x14UL))
#else
/* Struct: thứ tự và kích thước thành viên phải khớp offset trong RM0008 (mỗi thanh ghi 4 byte). */
typedef struct {
    volatile uint32_t CR;        /* 0x00 */
    volatile uint32_t CFGR;      /* 0x04 */
    volatile uint32_t CIR;       /* 0x08 */
    volatile uint32_t APB2RSTR;  /* 0x0C */
    volatile uint32_t APB1RSTR;  /* 0x10 */
    volatile uint32_t AHBENR;    /* 0x14 */
    volatile uint32_t APB2ENR;   /* 0x18 */
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CRL;       /* 0x00 */
    volatile uint32_t CRH;       /* 0x04 */
    volatile uint32_t IDR;       /* 0x08 */
    volatile uint32_t ODR;       /* 0x0C */
    volatile uint32_t BSRR;      /* 0x10 */
    volatile uint32_t BRR;       /* 0x14 */
    volatile uint32_t LCKR;      /* 0x18 */
} GPIO_TypeDef;

#define RCC     ((RCC_TypeDef  *)RCC_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define RCC_APB2ENR   (RCC->APB2ENR)
#define GPIOC_CRH     (GPIOC->CRH)
#define GPIOC_ODR     (GPIOC->ODR)
#define GPIOC_BSRR    (GPIOC->BSRR)
#define GPIOC_BRR     (GPIOC->BRR)
#endif

/* Số vòng đếm cho mỗi nửa chu kỳ nháy */
#ifndef BLINK_DELAY
#define BLINK_DELAY 400000U   /* đổi thành 100000U: LED nháy nhanh gấp 4, cách xác nhận code đã thật sự được nạp */
#endif

/* Delay thô: biến đếm volatile để trình biên dịch không xóa vòng lặp ở -O2. Thời gian chỉ gần đúng (HSI 8 MHz). */
static void delay(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++) { }
}

int main(void)
{
    /* 1. Bật clock cho GPIOC: GPIO của F103 nằm trên APB2 */
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC_APB2ENR;                                   /* đọc lại: chờ clock có hiệu lực trước khi ghi GPIOC */

    /* 2. PC13 = output push-pull 2 MHz. Reset của CRH là 0x4444 4444 (input thả nổi),
     *    nên phải xóa cả 4 bit CNF13:MODE13 rồi mới ghi giá trị mới. DS5319: PC13 tối đa 2 MHz. */
    uint32_t crh = GPIOC_CRH;
    crh &= ~(0xFUL << CRH_SHIFT(LED_PIN));
    crh |=  (CRH_OUT_PP_2MHZ << CRH_SHIFT(LED_PIN));
    GPIOC_CRH = crh;

    /* 3. Nhấp nháy: BRR (xóa) và BSRR (đặt) là ghi nguyên tử, không đọc-sửa-ghi ODR.
     *    LED nối kiểu sink: PC13 = 0 -> sáng, PC13 = 1 -> tắt. */
    for (;;) {
        GPIOC_BRR  = (1UL << LED_PIN);                   /* PC13 = 0: LED sáng */
        delay(BLINK_DELAY);
        GPIOC_BSRR = (1UL << LED_PIN);                   /* PC13 = 1: LED tắt */
        delay(BLINK_DELAY);
    }
}
