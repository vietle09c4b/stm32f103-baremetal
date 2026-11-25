/*
 * lowpower.c - Stop mode + đánh thức bằng EXTI8 (PB8 = CAN_RX) trên STM32F103C8, mức thanh ghi
 *
 * RM0008 Rev 21:
 *   5.3.4 / Table 14   Stop: SLEEPDEEP = 1, PDDS = 0, LPDS chọn regulator; thoát bằng EXTI line
 *                      ở interrupt mode (WFI). Mọi pending bit EXTI và ngoại vi phải = 0, nếu không
 *                      lệnh vào Stop bị bỏ qua. Thức dậy: SYSCLK = HSI.
 *   5.4.1              PWR_CR: PDDS bit 1, LPDS bit 0 (clock PWR: RCC_APB1ENR.PWREN)
 *   9.4.5              AFIO_EXTICR3: EXTI8[3:0] = 0001 -> chân PB8
 *   10.2.4             EXTI: FTSR (cạnh xuống), IMR (cho phép ngắt), PR (ghi 1 để xóa)
 *   10.1.2             EXTI9_5 = IRQ 23
 * PM0056 2.5 + 4.4.6: SCB_SCR.SLEEPDEEP bit 2 chọn deep sleep (Stop/Standby trên STM32).
 *
 * bxCAN không nằm trong danh sách nguồn đánh thức Stop (chỉ EXTI), vì vậy dùng chính chân CAN_RX:
 * RXD của TJA1050 xuống LOW khi bus dominant, bit SOF của frame đầu tiên tạo cạnh xuống trên PB8.
 */
#include "stm32f1xx.h"
#include "lowpower.h"

#define PRIO_EXTI9_5   1U

volatile uint32_t g_exti8_count;

void exti8_pb8_arm(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;

    /* PB8 vẫn là input pull-up (CAN_RX), chỉ nối thêm vào bộ dò cạnh EXTI8 */
    AFIO->EXTICR[2] = (AFIO->EXTICR[2] & ~AFIO_EXTICR3_EXTI8) | AFIO_EXTICR3_EXTI8_PB;

    EXTI->RTSR &= ~EXTI_RTSR_TR8;
    EXTI->FTSR |=  EXTI_FTSR_TR8;         /* cạnh xuống: recessive (HIGH) -> dominant (LOW) */
    EXTI->PR    =  EXTI_PR_PR8;           /* xóa pending cũ: ghi 1, không dùng |= */
    EXTI->IMR  |=  EXTI_IMR_MR8;

    NVIC->ICPR[0] = 1UL << (uint32_t)EXTI9_5_IRQn;
    NVIC->IPR[EXTI9_5_IRQn] = (uint8_t)(PRIO_EXTI9_5 << (8U - __NVIC_PRIO_BITS));
    NVIC->ISER[0] = 1UL << (uint32_t)EXTI9_5_IRQn;
}

void stop_mode_enter(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

    PWR->CR &= ~PWR_CR_PDDS;              /* deepsleep = Stop (không phải Standby) */
    PWR->CR |=  PWR_CR_LPDS;              /* regulator low-power trong Stop */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __DSB();
    __WFI();                              /* PRIMASK = 1: ngắt pending vẫn đánh thức lõi, handler chỉ chạy sau khi
                                             PRIMASK = 0 (PM0056 2.5.2) */

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;   /* các lần WFI sau (nếu có) chỉ là Sleep */
}

/* Chỉ cần đánh thức một lần: tắt EXTI8 ngay để các cạnh CAN sau đó không gây ngắt liên tục */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR8) {
        EXTI->IMR &= ~EXTI_IMR_MR8;
        EXTI->FTSR &= ~EXTI_FTSR_TR8;
        EXTI->PR = EXTI_PR_PR8;
        g_exti8_count++;
    }
}
