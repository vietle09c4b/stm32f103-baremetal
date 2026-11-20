/* Os.c - SysTick 1 ms làm nhịp hệ thống (PM0056 4.5).
 * Task 5 ms gọi job processing function của IoHwAb (vai trò BSW Scheduler, SWS_IoHwAb_00035),
 * task 10 ms gọi Rte_Task_10ms. HCLK = HSI 8 MHz: RELOAD = 8 000 000 / 1000 - 1 = 7999. */
#include "Os.h"
#include "Rte.h"
#include "IoHwAb.h"
#include "Stm32f103_Reg.h"

#define OS_CPU_HZ             8000000u
#define SYSTICK_CTRL_ENABLE   (1UL << 0)
#define SYSTICK_CTRL_TICKINT  (1UL << 1)
#define SYSTICK_CTRL_CLKSRC   (1UL << 2)

static volatile uint32 Os_TickCount = 0u;

void SysTick_Handler(void)
{
    Os_TickCount++;
}

uint32 Os_GetTick(void)
{
    return Os_TickCount;
}

void Os_Start(void)
{
    SYSTICK->LOAD = (OS_CPU_HZ / OS_TICK_HZ) - 1u;
    SYSTICK->VAL  = 0u;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSRC | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    uint32 next5  = OS_TASK_5MS;
    uint32 next10 = OS_TASK_10MS;
    for (;;) {
        uint32 now = Os_GetTick();
        if ((int32_t)(now - next5) >= 0) {        /* so sánh an toàn khi bộ đếm tràn */
            next5 += OS_TASK_5MS;
            IoHwAb_MainFunction_Button();         /* BSW chạy trước SWC trong cùng tick */
        }
        if ((int32_t)(now - next10) >= 0) {
            next10 += OS_TASK_10MS;
            Rte_Task_10ms();
        }
    }
}
