/* NvM.h - NVRAM Manager (Services Layer, Memory Services). AUTOSAR CP SWS NVRAMManager R20-11.
 * API configuration class 2 (SWS_NvM_00561), chỉ hiện thực phần dùng trong ví dụ:
 *   NvM_Init 0x00, NvM_GetErrorStatus 0x04, NvM_SetRamBlockStatus 0x05, NvM_WriteBlock 0x07,
 *   NvM_ReadAll 0x0C, NvM_WriteAll 0x0D, NvM_MainFunction 0x0E.
 * Chỉ hỗ trợ block NVM_BLOCK_NATIVE với permanent RAM block, CRC16, ROM default.
 * Block ID 0 (kết quả multi block) và 1 (configuration ID) là ID dành riêng (SWS_NvM_00471);
 * block 1 không được cấu hình vì NvMDynamicConfiguration = FALSE (SWS_NvM_00248). */
#ifndef NVM_H
#define NVM_H

#include "Std_Types.h"
#include "NvM_Cfg.h"

typedef uint16 NvM_BlockIdType;                 /* SWS_NvM_00471 */
typedef uint8  NvM_RequestResultType;           /* SWS_NvM_00470 */
#define NVM_REQ_OK                 ((NvM_RequestResultType)0x00u)
#define NVM_REQ_NOT_OK             ((NvM_RequestResultType)0x01u)
#define NVM_REQ_PENDING            ((NvM_RequestResultType)0x02u)
#define NVM_REQ_INTEGRITY_FAILED   ((NvM_RequestResultType)0x03u)
#define NVM_REQ_BLOCK_SKIPPED      ((NvM_RequestResultType)0x04u)
#define NVM_REQ_NV_INVALIDATED     ((NvM_RequestResultType)0x05u)
#define NVM_REQ_CANCELED           ((NvM_RequestResultType)0x06u)
#define NVM_REQ_RESTORED_DEFAULTS  ((NvM_RequestResultType)0x08u)

typedef uint8 NvM_ConfigType;                   /* SWS_NvM_00881: ConfigPtr luôn là NULL_PTR */

/* Mã lỗi development (SWS_NvM_91004) và runtime (SWS_NvM_00947) */
#define NVM_E_PARAM_BLOCK_ID       0x0Au
#define NVM_E_PARAM_ADDRESS        0x0Du
#define NVM_E_PARAM_POINTER        0x0Fu
#define NVM_E_UNINIT               0x14u
#define NVM_E_BLOCK_PENDING        0x15u
#define NVM_E_BLOCK_CONFIG         0x18u
#define NVM_E_QUEUE_FULL           0xA0u

#define NVM_SID_INIT               0x00u
#define NVM_SID_GETERRORSTATUS     0x04u
#define NVM_SID_SETRAMBLOCKSTATUS  0x05u
#define NVM_SID_WRITEBLOCK         0x07u
#define NVM_SID_READALL            0x0Cu
#define NVM_SID_WRITEALL           0x0Du
#define NVM_SID_MAINFUNCTION       0x0Eu

/* Trạng thái RAM block (Figure 10, mục 7.2.2.14) */
typedef enum {
    NVM_RAM_INVALID_UNCHANGED = 0,
    NVM_RAM_VALID_UNCHANGED,
    NVM_RAM_VALID_CHANGED
} NvM_RamStateType;

void           NvM_Init(const NvM_ConfigType *ConfigPtr);
void           NvM_ReadAll(void);
void           NvM_WriteAll(void);
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void *NvM_SrcPtr);
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr);
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged);
void           NvM_MainFunction(void);   /* SchM_NvM.h trong hệ thống thật */

/* Biến quan sát khi debug */
extern volatile NvM_RamStateType NvM_RamState[NVM_NUM_BLOCKS];
extern volatile uint16           NvM_LastCrcCalc[NVM_NUM_BLOCKS];    /* CRC tính lại ở lần đọc gần nhất */
extern volatile uint16           NvM_LastCrcStored[NVM_NUM_BLOCKS];  /* CRC đọc từ Flash ở lần đọc gần nhất */
extern volatile uint32           NvM_WriteRetryCount;

#endif /* NVM_H */
