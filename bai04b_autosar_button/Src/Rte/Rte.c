/* Rte.c - RTE viết tay.
 * RTE generator thật sinh ra hai phần: hàm Rte_Call nối port của SWC tới server (ở đây là IoHwAb),
 * và thân OS task gọi các runnable theo TimingEvent. */
#include "Rte.h"
#include "Rte_ButtonLed.h"
#include "IoHwAb.h"

Std_ReturnType Rte_Call_RpButton_Get(boolean *Pressed)
{
    return IoHwAb_Button_Get(Pressed);            /* client-server, gọi trực tiếp trong cùng ECU */
}

Std_ReturnType Rte_Call_RpLed_Set(boolean On)
{
    return IoHwAb_Led_Set(On);
}

void Rte_Task_10ms(void)
{
    ButtonLed_Run10ms();                          /* TimingEvent 10 ms */
}
