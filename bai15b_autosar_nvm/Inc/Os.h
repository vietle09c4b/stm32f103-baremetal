/* Os.h - lập lịch tối giản bằng SysTick, đóng vai OS và BSW Scheduler (không phải AUTOSAR OS). */
#ifndef OS_H
#define OS_H

#include "Std_Types.h"

#define OS_CPU_HZ         8000000u   /* HSI 8 MHz sau reset */
#define OS_TICK_HZ        1000u      /* SysTick 1 ms */
#define OS_TASK_1MS       1u         /* BSW: NvM, Fee, Fls MainFunction, EcuM */
#define OS_TASK_5MS       5u         /* BSW: IoHwAb_MainFunction_Button */
#define OS_TASK_10MS      10u        /* SWC: Rte_Task_10ms */

void   Os_Start(void);               /* không trả về */
uint32 Os_GetTick(void);
uint32 Os_GetCycles(void);           /* DWT_CYCCNT */
uint32 Os_CyclesToUs(uint32 Cycles);

#endif /* OS_H */
