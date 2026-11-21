/*
 * irq_drivers.c - Cấu hình SysTick, TIM2, EXTI0 ở chế độ ngắt và NVIC mức thanh ghi
 */
#include "stm32f1xx.h"
#include "irq_drivers.h"

/* ---------------- NVIC ----------------------------------------------------- */

void nvic_set_priority(int32_t irqn, uint32_t prio)
{
    uint8_t value = (uint8_t)((prio & 0xFU) << (8U - __NVIC_PRIO_BITS));   /* 4 bit cao */

    if (irqn >= 0) {
        NVIC->IPR[(uint32_t)irqn] = value;                 /* IPRn: mỗi IRQ một byte */
    } else {
        /* System handler: SHPR1..3, SysTick (exception 15) ở byte 15 - 4 = 11 */
        SCB->SHPR[((uint32_t)irqn & 0xFU) - 4U] = value;
    }
}

void nvic_enable_irq(int32_t irqn)
{
    /* ISER0 cho IRQ 0..31, ISER1 cho IRQ 32..63; ghi 1 để bật, ghi 0 không tác dụng */
    NVIC->ISER[(uint32_t)irqn >> 5U] = 1UL << ((uint32_t)irqn & 0x1FU);
}

/* ---------------- SysTick 1 ms -------------------------------------------- */

void systick_1ms_irq_init(void)
{
    SysTick->LOAD = (HCLK_HZ / 1000UL) - 1UL;              /* 7999: 8000 xung = 1 ms */
    SysTick->VAL  = 0UL;
    nvic_set_priority(SysTick_IRQn, PRIO_SYSTICK);
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk             /* HCLK */
                  | SysTick_CTRL_TICKINT_Msk               /* phát exception khi về 0 */
                  | SysTick_CTRL_ENABLE_Msk;
}

/* ---------------- TIM2 update 1 Hz ---------------------------------------- */

void tim2_1hz_irq_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    TIM2->PSC = 8000U - 1U;                                /* 8 MHz / 8000 = 1 kHz */
    TIM2->ARR = 1000U - 1U;                                /* 1 kHz / 1000 = 1 Hz  */
    TIM2->EGR = TIM_EGR_UG;                                /* nạp PSC */
    TIM2->SR  = ~TIM_SR_UIF;                               /* xóa UIF do UG tạo ra */

    TIM2->DIER = TIM_DIER_UIE;                             /* cho phép ngắt update */
    nvic_set_priority(TIM2_IRQn, PRIO_TIM2);
    nvic_enable_irq(TIM2_IRQn);

    TIM2->CR1 = TIM_CR1_CEN;
}

/* ---------------- EXTI0 trên PA0 ------------------------------------------ */

void exti0_pa0_falling_init(void)
{
    /* 1. Clock GPIOA và AFIO (AFIO_EXTICR chỉ đọc/ghi được khi AFIOEN = 1) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;

    /* 2. PA0 input pull-up: CNF = 10, MODE = 00 trong CRL; ODR0 = 1 chọn pull-up */
    uint32_t crl = GPIOA->CRL;
    crl &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    crl |=  GPIO_CRL_CNF0_1;
    GPIOA->CRL  = crl;
    GPIOA->BSRR = GPIO_BSRR_BS0;

    /* 3. EXTI0 lấy tín hiệu từ port A: EXTI0[3:0] = 0000 trong AFIO_EXTICR1 */
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~AFIO_EXTICR1_EXTI0) | AFIO_EXTICR1_EXTI0_PA;

    /* 4. Cạnh xuống (nhấn nút kéo PA0 về GND), không dùng cạnh lên */
    EXTI->FTSR |=  EXTI_FTSR_TR0;
    EXTI->RTSR &= ~EXTI_RTSR_TR0;

    /* 5. Xóa pending cũ (rc_w1: ghi 1 để xóa) rồi bỏ mask line 0 */
    EXTI->PR   = EXTI_PR_PR0;
    EXTI->IMR |= EXTI_IMR_MR0;

    /* 6. NVIC */
    nvic_set_priority(EXTI0_IRQn, PRIO_EXTI0);
    nvic_enable_irq(EXTI0_IRQn);
}
