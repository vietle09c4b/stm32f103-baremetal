/* CanLed.c - Application SWC.
 * Gửi AliveCounter mỗi 100 ms và bật/tắt LED theo LedCmd nhận được. Toàn bộ truy cập ra ngoài đi qua RTE. */
#include "Rte_CanLed.h"

volatile uint8   CanLed_AliveCounter = 0u;     /* quan sát khi debug */
volatile boolean CanLed_LedCmd       = FALSE;

void CanLed_Run100ms(void)
{
    CanLed_AliveCounter++;
    (void)Rte_Write_PpCanLedStatus_AliveCounter(CanLed_AliveCounter);
}

void CanLed_Run10ms(void)
{
    boolean cmd = FALSE;
    if (Rte_Read_RpLedCmd_LedCmd(&cmd) == RTE_E_OK) {
        CanLed_LedCmd = cmd;
        (void)Rte_Call_RpLed_Set(cmd);
    }
}
