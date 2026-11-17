/* Rte.c - RTE viết tay.
 * RTE generator thật sinh ra hai phần: hàm Rte_Call nối port của SWC tới server (ở đây là IoHwAb),
 * và thân OS task gọi các runnable theo TimingEvent. */
#include "Rte.h"
#include "Rte_LedBlinker.h"
#include "IoHwAb.h"

Std_ReturnType Rte_Call_RpLed_Set(boolean On)
{
    return IoHwAb_Led_Set(On);                    /* client-server, gọi trực tiếp trong cùng ECU */
}

void Rte_Task_500ms(void)
{
    LedBlinker_Run500ms();                        /* TimingEvent 500 ms */
}
