/* Rte_ButtonLed.h - API RTE cho SWC ButtonLed (viết tay, thay cho file do RTE generator sinh ra).
 * Hai port yêu cầu kiểu client-server: RpButton (operation Get) và RpLed (operation Set),
 * theo mẫu Rte_Call_<p>_<o> (SWS_IoHwAb_00143). */
#ifndef RTE_BUTTONLED_H
#define RTE_BUTTONLED_H

#include "Std_Types.h"

Std_ReturnType Rte_Call_RpButton_Get(boolean *Pressed);
Std_ReturnType Rte_Call_RpLed_Set(boolean On);

/* Runnable của SWC, được RTE gọi từ task 10 ms (TimingEvent) */
void ButtonLed_Run10ms(void);

/* Biến quan sát khi debug */
extern volatile uint32 ButtonLed_PressCount;

#endif /* RTE_BUTTONLED_H */
