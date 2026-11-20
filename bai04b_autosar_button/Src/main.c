/* main.c - đóng vai EcuM: khởi tạo BSW theo thứ tự MCAL -> ECU Abstraction -> OS.
 * vietc4b.com, series bare-metal STM32F103: đọc nút nhấn PA0, bật LED PC13 theo kiến trúc AUTOSAR. */
#include "Mcu.h"
#include "Port.h"
#include "IoHwAb.h"
#include "Os.h"

int main(void)
{
    Mcu_Init(&Mcu_Config);        /* 1. clock GPIOA, GPIOC (RCC_APB2ENR) */
    Port_Init(&Port_Config);      /* 2. PA0 input pull-up, PC13 output mức HIGH */
    IoHwAb_Init0(NULL_PTR);       /* 3. đọc mức ban đầu của nút, LED tắt */
    Os_Start();                   /* 4. SysTick 1 ms: task 5 ms (IoHwAb), task 10 ms (RTE -> SWC) */
    return 0;
}
