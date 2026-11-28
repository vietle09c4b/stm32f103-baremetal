/* main.c - vietc4b.com, series bare-metal STM32F103, bài 15b:
 * memory stack AUTOSAR Classic R20-11 (NvM -> MemIf -> Fee -> Fls) trên Flash nội của Blue Pill. */
#include "EcuM.h"
#include "Os.h"

int main(void)
{
    EcuM_Init();      /* MCAL -> ECU Abstraction -> Services, rồi NvM_ReadAll */
    Os_Start();       /* SysTick 1 ms: memory stack + EcuM; 5 ms: IoHwAb; 10 ms: SWC */
    return 0;
}
