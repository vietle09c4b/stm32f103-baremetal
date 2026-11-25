/* Rte_CanLed.h - application header của SWC CanLed (RTE generator sinh ra trong dự án thật).
 * SWC chỉ include file này: không biết CAN ID, DLC, controller, chân GPIO hay thanh ghi nào. */
#ifndef RTE_CANLED_H
#define RTE_CANLED_H

#include "Std_Types.h"

#define RTE_E_OK           ((Std_ReturnType)0u)
#define RTE_E_COM_STOPPED  ((Std_ReturnType)128u)   /* Com trả COM_SERVICE_NOT_AVAILABLE */

/* Sender-receiver: P-port PpCanLedStatus, data element AliveCounter (uint8) */
Std_ReturnType Rte_Write_PpCanLedStatus_AliveCounter(uint8 data);
/* Sender-receiver: R-port RpLedCmd, data element LedCmd (boolean) */
Std_ReturnType Rte_Read_RpLedCmd_LedCmd(boolean *data);
/* Client-server: R-port RpLed, operation Set (server: IoHwAb) */
Std_ReturnType Rte_Call_RpLed_Set(boolean On);

/* Runnable của SWC */
void CanLed_Run10ms(void);      /* TimingEvent 10 ms: đọc lệnh LED */
void CanLed_Run100ms(void);     /* TimingEvent 100 ms: tăng AliveCounter */

#endif /* RTE_CANLED_H */
