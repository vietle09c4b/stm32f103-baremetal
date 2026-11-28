/* Rte.c - RTE viết tay.
 * Rte_Call_<port>_<operation> của port NvMService gọi thẳng API NvM với BlockId cố định cho port
 * (port-defined argument, SWS_NvM_00847). PerInstanceMemory của SWC được dùng làm permanent RAM block. */
#include "Rte.h"
#include "Rte_NvmDemo.h"
#include "NvM.h"
#include "IoHwAb.h"
#include "EcuM.h"

NvmDemo_BootCounterType Rte_Pim_NvmDemo_BootCounter;
NvmDemo_LedConfigType   Rte_Pim_NvmDemo_LedConfig;

Std_ReturnType Rte_Call_NvMBootCounter_WriteBlock(const void *SrcPtr)
{
    return NvM_WriteBlock(NvMConf_NvMBlockDescriptor_BootCounter, SrcPtr);
}

Std_ReturnType Rte_Call_NvMBootCounter_GetErrorStatus(NvM_RequestResultType_Rte *RequestResult)
{
    return NvM_GetErrorStatus(NvMConf_NvMBlockDescriptor_BootCounter, RequestResult);
}

Std_ReturnType Rte_Call_NvMLedConfig_SetRamBlockStatus(boolean BlockChanged)
{
    return NvM_SetRamBlockStatus(NvMConf_NvMBlockDescriptor_LedConfig, BlockChanged);
}

Std_ReturnType Rte_Call_NvMLedConfig_GetErrorStatus(NvM_RequestResultType_Rte *RequestResult)
{
    return NvM_GetErrorStatus(NvMConf_NvMBlockDescriptor_LedConfig, RequestResult);
}

Std_ReturnType Rte_Call_RpButton_Get(boolean *Pressed)
{
    return IoHwAb_Button_Get(Pressed);
}

Std_ReturnType Rte_Call_RpLed_Set(boolean On)
{
    return IoHwAb_Led_Set(On);
}

Std_ReturnType Rte_Call_RpShutdown_Request(void)
{
    EcuM_RequestShutdown();
    return E_OK;
}

void Rte_Start(void)
{
    NvmDemo_Init();
}

void Rte_Task_10ms(void)
{
    NvmDemo_Run10ms();
}
