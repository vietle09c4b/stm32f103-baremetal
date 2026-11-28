/* NvM_Cfg.h - cấu hình NvM (NvMCommon, NvMBlockDescriptor). Trong dự án thật do công cụ cấu hình sinh ra. */
#ifndef NVM_CFG_H
#define NVM_CFG_H

#include "Std_Types.h"

#define NVM_DEV_ERROR_DETECT         STD_ON
#define NVM_API_CONFIG_CLASS         2u      /* NvMApiConfigClass = NVM_API_CONFIG_CLASS_2 */
#define NVM_POLLING_MODE             STD_ON  /* NvMPollingMode: NvM hỏi MemIf_GetJobResult */
#define NVM_DATASET_SELECTION_BITS   0u      /* NvMDatasetSelectionBits: chỉ có block NATIVE */
#define NVM_SIZE_STANDARD_JOB_QUEUE  4u      /* NvMSizeStandardJobQueue */
#define NVM_NUM_BLOCKS               4u      /* block ID 0..3; 0 và 1 dành riêng */
#define NVM_MAX_BLOCK_LENGTH         4u
#define NVM_CRC16_BYTES              2u

/* Symbolic name của block (NvMNvramBlockIdentifier) */
#define NvMConf_NvMBlockDescriptor_BootCounter   ((uint16)2u)
#define NvMConf_NvMBlockDescriptor_LedConfig     ((uint16)3u)

typedef enum { NVM_BLOCK_NATIVE = 0, NVM_BLOCK_REDUNDANT, NVM_BLOCK_DATASET } NvM_BlockManagementType;
typedef enum { NVM_CRC_NONE = 0, NVM_CRC8, NVM_CRC16, NVM_CRC32 } NvM_BlockCrcType;

typedef struct {
    uint16                  NvBlockBaseNumber;        /* NvMNvBlockBaseNumber (1..65534) */
    uint8                   NvramDeviceId;            /* NvMNvramDeviceId: thiết bị MemIf */
    NvM_BlockManagementType ManagementType;           /* NvMBlockManagementType */
    uint16                  NvBlockLength;            /* NvMNvBlockLength (byte dữ liệu, chưa kể CRC) */
    NvM_BlockCrcType        CrcType;                  /* NvMBlockUseCrc + NvMBlockCrcType */
    void                   *RamBlockDataAddress;      /* NvMRamBlockDataAddress: permanent RAM block */
    const void             *RomBlockDataAddress;      /* NvMRomBlockDataAddress: dữ liệu mặc định */
    boolean                 SelectBlockForReadAll;    /* NvMSelectBlockForReadAll */
    boolean                 SelectBlockForWriteAll;   /* NvMSelectBlockForWriteAll */
    boolean                 UseSetRamBlockStatus;     /* NvMBlockUseSetRamBlockStatus */
    uint8                   MaxNumOfWriteRetries;     /* NvMMaxNumOfWriteRetries (0..7) */
} NvM_BlockDescriptorType;

extern const NvM_BlockDescriptorType NvM_BlockDescriptor[NVM_NUM_BLOCKS];

#endif /* NVM_CFG_H */
