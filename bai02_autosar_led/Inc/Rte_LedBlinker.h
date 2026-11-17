/* Rte_LedBlinker.h - API RTE cho SWC LedBlinker (viết tay, thay cho file do RTE generator sinh ra).
 * Port yêu cầu RpLed (client-server), operation Set -> Rte_Call_<p>_<o> (SWS_IoHwAb_00143, mẫu Rte_Call). */
#ifndef RTE_LEDBLINKER_H
#define RTE_LEDBLINKER_H

#include "Std_Types.h"

Std_ReturnType Rte_Call_RpLed_Set(boolean On);

/* Runnable của SWC, được RTE gọi từ task 500 ms (TimingEvent) */
void LedBlinker_Run500ms(void);

#endif /* RTE_LEDBLINKER_H */
