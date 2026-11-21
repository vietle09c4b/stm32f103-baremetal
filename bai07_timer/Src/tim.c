/*
 * tim.c - TIM2 và TIM3 mức thanh ghi trên STM32F103 (RM0008 Rev 21, chương 15)
 */
#include "stm32f1xx.h"
#include "tim.h"

/* 4 bit cấu hình chân trong CRL (RM0008 Table 20, 21) */
#define CFG_AF_PP_2MHZ   0xAU    /* CNF = 10 (AF push-pull), MODE = 10 (output 2 MHz) */
#define CFG_IN_FLOATING  0x4U    /* CNF = 01 (input floating), MODE = 00 */

#define PA0_SHIFT        (0U * 4U)   /* PA0 -> CRL bit 3:0   */
#define PA6_SHIFT        (6U * 4U)   /* PA6 -> CRL bit 27:24 */

void tim2_1hz_oc_pa0_init(void)
{
    /* 1. Clock: GPIOA trên APB2, TIM2 trên APB1 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    /* 2. PA0 = TIM2_CH1 (không remap): alternate function push-pull (Table 23) */
    uint32_t crl = GPIOA->CRL;
    crl &= ~(0xFU << PA0_SHIFT);
    crl |=  (CFG_AF_PP_2MHZ << PA0_SHIFT);
    GPIOA->CRL = crl;

    /* 3. Time base: 8 MHz / (7999 + 1) = 1 kHz, đếm 0..999 -> update mỗi 1 s */
    TIM2->PSC  = TIM2_PSC_VALUE;
    TIM2->ARR  = TIM2_ARR_VALUE;
    TIM2->CCR1 = 0U;                                   /* so khớp khi CNT = 0 */

    /* 4. CH1 output compare, OC1M = 011 (toggle), CC1S = 00 (output), OC1PE = 0 */
    TIM2->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;

    /* 5. Bật đầu ra OC1, cực tính active high (CC1P = 0) */
    TIM2->CCER = TIM_CCER_CC1E;

    /* 6. PSC chỉ được nạp ở update event: tạo update bằng phần mềm rồi xóa UIF
     *    (UG set UIF khi URS = 0, RM0008 15.4.5) */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR  = ~TIM_SR_UIF;

    /* 7. Bật bộ đếm */
    TIM2->CR1 = TIM_CR1_CEN;
}

void tim3_ic_pa6_init(void)
{
    /* 1. Clock: GPIOA trên APB2, TIM3 trên APB1 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    /* 2. PA6 = TIM3_CH1 (không remap): input floating (Table 23) */
    uint32_t crl = GPIOA->CRL;
    crl &= ~(0xFU << PA6_SHIFT);
    crl |=  (CFG_IN_FLOATING << PA6_SHIFT);
    GPIOA->CRL = crl;

    /* 3. 8 MHz / (799 + 1) = 10 kHz, đếm tự do 0..0xFFFF */
    TIM3->PSC = TIM3_PSC_VALUE;
    TIM3->ARR = 0xFFFFU;

    /* 4. CC1S = 01: IC1 nối với TI1, không lọc (IC1F = 0000), không chia (IC1PSC = 00) */
    TIM3->CCMR1 = TIM_CCMR1_CC1S_0;

    /* 5. Bắt cạnh lên (CC1P = 0), bật capture (CC1E = 1) */
    TIM3->CCER = TIM_CCER_CC1E;

    /* 6. Nạp PSC bằng update phần mềm, xóa mọi cờ trước khi chạy */
    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR  = 0U;

    /* 7. Bật bộ đếm */
    TIM3->CR1 = TIM_CR1_CEN;
}
