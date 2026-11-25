/* Os.h - lập lịch tối giản bằng SysTick, đóng vai OS và BSW Scheduler cho ví dụ (không phải AUTOSAR OS). */
#ifndef OS_H
#define OS_H

#include "Std_Types.h"

#define OS_TICK_HZ        1000u      /* SysTick 1 ms */
#define OS_TASK_10MS      10u        /* BSW main function + runnable 10 ms */
#define OS_TASK_100MS     100u       /* runnable 100 ms */

void   Os_Start(void);               /* không trả về */
uint32 Os_GetTick(void);

#endif /* OS_H */
