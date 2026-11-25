/* Rte.c - RTE viết tay.
 * Data element nối ra mạng CAN được ánh xạ sang signal của Com (system signal trong ECU extract):
 *   Rte_Write -> Com_SendSignal, Rte_Read -> Com_ReceiveSignal.
 * Client-server tới IoHwAb là lời gọi hàm trực tiếp trong cùng ECU. */
#include "Rte.h"
#include "Rte_CanLed.h"
#include "Com.h"
#include "IoHwAb.h"

static Std_ReturnType Rte_FromCom(uint8 comRet)
{
    return (comRet == E_OK) ? RTE_E_OK : RTE_E_COM_STOPPED;
}

Std_ReturnType Rte_Write_PpCanLedStatus_AliveCounter(uint8 data)
{
    return Rte_FromCom(Com_SendSignal(ComConf_ComSignal_AliveCounter, &data));
}

Std_ReturnType Rte_Read_RpLedCmd_LedCmd(boolean *data)
{
    if (data == NULL_PTR) {
        return E_NOT_OK;
    }
    return Rte_FromCom(Com_ReceiveSignal(ComConf_ComSignal_LedCmd, data));
}

Std_ReturnType Rte_Call_RpLed_Set(boolean On)
{
    return IoHwAb_Led_Set(On);
}

void Rte_Task_10ms(void)
{
    CanLed_Run10ms();
}

void Rte_Task_100ms(void)
{
    CanLed_Run100ms();
}
