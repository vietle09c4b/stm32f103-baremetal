/* Rte_NvmDemo.h - API RTE cho SWC NvmDemo (viết tay, thay cho file do RTE generator sinh ra).
 * SWC có hai require port kiểu client-server nối tới service port PS_{Block} của NvM
 * (interface NvMService, SWS_NvM_00847); BlockId là port-defined argument nên SWC không truyền.
 * RAM block của NvM là PerInstanceMemory (Rte_Pim) của SWC. */
#ifndef RTE_NVMDEMO_H
#define RTE_NVMDEMO_H

#include "Std_Types.h"

typedef uint8 NvM_RequestResultType_Rte;   /* NvM_RequestResultType (Rte_NvM_Type.h) */

typedef struct {
    uint32 BootCount;
} NvmDemo_BootCounterType;                 /* 4 byte */

typedef struct {
    uint16 BlinkPeriodMs;
    uint16 ChangeCount;
} NvmDemo_LedConfigType;                   /* 4 byte */

extern NvmDemo_BootCounterType Rte_Pim_NvmDemo_BootCounter;
extern NvmDemo_LedConfigType   Rte_Pim_NvmDemo_LedConfig;

#define Rte_Pim_BootCounter()  (&Rte_Pim_NvmDemo_BootCounter)
#define Rte_Pim_LedConfig()    (&Rte_Pim_NvmDemo_LedConfig)

/* Port NvMBootCounter (NvMService) */
Std_ReturnType Rte_Call_NvMBootCounter_WriteBlock(const void *SrcPtr);
Std_ReturnType Rte_Call_NvMBootCounter_GetErrorStatus(NvM_RequestResultType_Rte *RequestResult);
/* Port NvMLedConfig (NvMService) */
Std_ReturnType Rte_Call_NvMLedConfig_SetRamBlockStatus(boolean BlockChanged);
Std_ReturnType Rte_Call_NvMLedConfig_GetErrorStatus(NvM_RequestResultType_Rte *RequestResult);
/* Port nút nhấn và LED (IoHwAb), port yêu cầu tắt máy (EcuM) */
Std_ReturnType Rte_Call_RpButton_Get(boolean *Pressed);
Std_ReturnType Rte_Call_RpLed_Set(boolean On);
Std_ReturnType Rte_Call_RpShutdown_Request(void);

/* Runnable của SWC */
void NvmDemo_Init(void);        /* InitEvent: chạy một lần sau NvM_ReadAll */
void NvmDemo_Run10ms(void);     /* TimingEvent 10 ms */

/* Biến quan sát khi debug */
extern volatile uint8  NvmDemo_BootReadResult;   /* kết quả NvM của block BootCounter sau ReadAll */
extern volatile uint8  NvmDemo_LedReadResult;    /* kết quả NvM của block LedConfig sau ReadAll */
extern volatile uint32 NvmDemo_ShortPresses;

#endif /* RTE_NVMDEMO_H */
