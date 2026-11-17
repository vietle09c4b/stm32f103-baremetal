/* Os.c - SysTick 1 ms làm nhịp hệ thống (PM0056 4.5), task 500 ms gọi Rte_Task_500ms.
 * HCLK = HSI 8 MHz sau reset; CLKSOURCE = 1 (processor clock): RELOAD = 8 000 000 / 1000 - 1 = 7999. */
#include "Os.h"
#include "Rte.h"
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

    uint32 next = OS_TASK_500MS;
    for (;;) {
        if ((int32_t)(Os_GetTick() - next) >= 0) {   /* so sánh an toàn khi bộ đếm tràn */
            next += OS_TASK_500MS;
            Rte_Task_500ms();
        }
    }
}
