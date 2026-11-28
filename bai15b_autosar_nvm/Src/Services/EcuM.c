/* EcuM.c - ECU State Manager rút gọn.
 * Thứ tự khởi động BSW: MCAL (Mcu, Port, Fls) -> ECU Abstraction (IoHwAb, Fee) -> Services (NvM),
 * rồi NvM_ReadAll; SWC chỉ chạy sau khi ReadAll xong (SWS_NvM_00718: ReadAll nạp permanent RAM lúc khởi động).
 * Tắt máy: NvM_WriteAll đồng bộ permanent RAM block xuống NV (SWS_NvM_00018), chờ xong rồi bật LED. */
#include "EcuM.h"
#include "Mcu.h"
#include "Port.h"
#include "Fls.h"
#include "Fee.h"
#include "NvM.h"
#include "IoHwAb.h"
#include "Rte.h"
#include "Os.h"

volatile EcuM_StateType EcuM_State          = ECUM_STATE_STARTUP;
volatile uint32         EcuM_ReadAllUs      = 0u;
volatile uint32         EcuM_WriteAllUs     = 0u;
volatile uint8          EcuM_ReadAllResult  = 0xFFu;
volatile uint8          EcuM_WriteAllResult = 0xFFu;

static boolean EcuM_ShutdownReq = FALSE;
static uint32  EcuM_T0 = 0u;

void EcuM_Init(void)
{
    Mcu_Init(&Mcu_Config);        /* clock GPIO, DWT */
    Port_Init(&Port_Config);      /* PA0 input pull-up, PC13 output */
    Fls_Init(&Fls_Config);        /* MCAL trước */
    IoHwAb_Init0(NULL_PTR);
    Fee_Init(&Fee_Config);        /* bất đồng bộ: chọn bank và quét trong Fee_MainFunction */
    NvM_Init(NULL_PTR);
    EcuM_T0 = Os_GetCycles();
    NvM_ReadAll();                /* chạy trong NvM_MainFunction */
    EcuM_State = ECUM_STATE_STARTUP;
}

void EcuM_RequestShutdown(void)
{
    EcuM_ShutdownReq = TRUE;
}

void EcuM_MainFunction(void)
{
    NvM_RequestResultType r = NVM_REQ_PENDING;

    switch (EcuM_State) {
    case ECUM_STATE_STARTUP:
        (void)NvM_GetErrorStatus(0u, &r);
        if (r != NVM_REQ_PENDING) {
            EcuM_ReadAllUs     = Os_CyclesToUs(Os_GetCycles() - EcuM_T0);
            EcuM_ReadAllResult = r;
            EcuM_State         = ECUM_STATE_RUN;
            Rte_Start();
        }
        break;
    case ECUM_STATE_RUN:
        if (EcuM_ShutdownReq != FALSE) {
            EcuM_T0 = Os_GetCycles();
            NvM_WriteAll();
            EcuM_State = ECUM_STATE_SHUTDOWN;
            (void)IoHwAb_Led_Set(FALSE);
        }
        break;
    case ECUM_STATE_SHUTDOWN:
        (void)NvM_GetErrorStatus(0u, &r);
        if (r != NVM_REQ_PENDING) {
            EcuM_WriteAllUs     = Os_CyclesToUs(Os_GetCycles() - EcuM_T0);
            EcuM_WriteAllResult = r;
            EcuM_State          = ECUM_STATE_OFF;
            (void)IoHwAb_Led_Set(TRUE);   /* LED sáng đứng: đã ghi xong, có thể rút điện */
        }
        break;
    default:
        break;
    }
}
