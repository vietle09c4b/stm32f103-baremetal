/* Os.c - SysTick 1 ms làm nhịp hệ thống (PM0056 4.5). HCLK = HSE 8 MHz: RELOAD = 7999.
 * Mỗi 10 ms, thứ tự trong cùng tick:
 *   1. Can_MainFunction_Mode, Can_MainFunction_Write  (BSW: trạng thái controller, TxConfirmation)
 *   2. Rte_Task_100ms (mỗi 10 lần), Rte_Task_10ms      (SWC ghi/đọc signal)
 *   3. Com_MainFunctionTx                              (Com gửi I-PDU đến hạn, có giá trị mới nhất) */
#include "Os.h"
#include "Rte.h"
#include "Can.h"
#include "Com.h"
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

    uint32 next10 = OS_TASK_10MS;
    uint32 n10    = 0u;
    for (;;) {
        uint32 now = Os_GetTick();
        if ((int32_t)(now - next10) >= 0) {       /* so sánh an toàn khi bộ đếm tràn */
            next10 += OS_TASK_10MS;

            Can_MainFunction_Mode();
            Can_MainFunction_Write();

            if (++n10 >= (OS_TASK_100MS / OS_TASK_10MS)) {
                n10 = 0u;
                Rte_Task_100ms();
            }
            Rte_Task_10ms();

            Com_MainFunctionTx();
        }
    }
}
