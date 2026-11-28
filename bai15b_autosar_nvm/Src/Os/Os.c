/* Os.c - SysTick 1 ms làm nhịp hệ thống (PM0056 4.5), HCLK = HSI 8 MHz: RELOAD = 7999.
 * Task 1 ms gọi MainFunction của memory stack theo thứ tự trên xuống dưới (NvM -> Fee -> Fls), nên một
 * request từ NvM đi xuống tới Fls ngay trong cùng chu kỳ; kết quả được hỏi lại ở chu kỳ sau.
 * Lưu ý: khi Fls xóa một sector, CPU dừng khoảng 20 ms (PM0075 1.2) và SysTick_Handler cũng phải chờ,
 * nên đếm tick bị hụt; thời gian được đo bằng DWT_CYCCNT thay vì tick. */
#include "Os.h"
#include "Rte.h"
#include "IoHwAb.h"
#include "NvM.h"
#include "Fee.h"
#include "Fls.h"
#include "EcuM.h"
#include "TestHook.h"
#include "Stm32f103_Reg.h"

#define SYSTICK_CTRL_ENABLE   (1UL << 0)
#define SYSTICK_CTRL_TICKINT  (1UL << 1)
#define SYSTICK_CTRL_CLKSRC   (1UL << 2)

static volatile uint32 Os_TickCount = 0u;

void SysTick_Handler(void)
{
    Os_TickCount++;
}

uint32 Os_GetTick(void)              { return Os_TickCount; }
uint32 Os_GetCycles(void)            { return DWT->CYCCNT; }
uint32 Os_CyclesToUs(uint32 Cycles)  { return Cycles / (OS_CPU_HZ / 1000000u); }

void Os_Start(void)
{
    SYSTICK->LOAD = (OS_CPU_HZ / OS_TICK_HZ) - 1u;
    SYSTICK->VAL  = 0u;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSRC | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    uint32 next1  = OS_TASK_1MS;
    uint32 next5  = OS_TASK_5MS;
    uint32 next10 = OS_TASK_10MS;
    for (;;) {
        uint32 now = Os_GetTick();
        if ((sint32)(now - next1) >= 0) {
            next1 = now + OS_TASK_1MS;             /* không đuổi theo tick đã mất khi CPU bị dừng */
            NvM_MainFunction();
            Fee_MainFunction();
            Fls_MainFunction();
            EcuM_MainFunction();
            TestHook_MainFunction();
        }
        if ((sint32)(now - next5) >= 0) {
            next5 = now + OS_TASK_5MS;
            IoHwAb_MainFunction_Button();
        }
        if (((sint32)(now - next10) >= 0) && (EcuM_State == ECUM_STATE_RUN)) {
            next10 = now + OS_TASK_10MS;
            Rte_Task_10ms();
        }
    }
}
