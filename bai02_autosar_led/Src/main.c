/* main.c - đóng vai EcuM: khởi tạo BSW theo thứ tự MCAL -> ECU Abstraction -> OS.
 * vietc4b.com, series bare-metal STM32F103: nháy LED PC13 theo kiến trúc AUTOSAR. */
#include "Mcu.h"
#include "Port.h"
#include "IoHwAb.h"
#include "Os.h"

int main(void)
{
    Mcu_Init(&Mcu_Config);        /* 1. clock GPIOC (RCC_APB2ENR) */
    Port_Init(&Port_Config);      /* 2. PC13 output push-pull, mức ban đầu HIGH (GPIOC_CRH, BSRR) */
    IoHwAb_Init0(NULL_PTR);       /* 3. LED về trạng thái tắt qua Dio */
    Os_Start();                   /* 4. SysTick 1 ms, task 500 ms -> RTE -> SWC */
    return 0;
}
