/* NvM_Cfg.c - bảng NVRAM block descriptor (SWS_NvM_00143: sinh lúc cấu hình, nằm trong ROM).
 * RAM block là PerInstanceMemory của SWC NvmDemo, do RTE cấp phát (Rte.c). */
#include "NvM.h"
#include "Rte_NvmDemo.h"

static const NvmDemo_BootCounterType NvM_Rom_BootCounter = { .BootCount = 0u };
static const NvmDemo_LedConfigType   NvM_Rom_LedConfig   = { .BlinkPeriodMs = 500u, .ChangeCount = 0u };

const NvM_BlockDescriptorType NvM_BlockDescriptor[NVM_NUM_BLOCKS] = {
    [0] = { 0u },                              /* dành riêng: kết quả multi block */
    [1] = { 0u },                              /* dành riêng: configuration ID (không dùng) */
    [NvMConf_NvMBlockDescriptor_BootCounter] = {
        .NvBlockBaseNumber      = 2u,
        .NvramDeviceId          = 0u,
        .ManagementType         = NVM_BLOCK_NATIVE,
        .NvBlockLength          = (uint16)sizeof(NvmDemo_BootCounterType),
        .CrcType                = NVM_CRC16,
        .RamBlockDataAddress    = &Rte_Pim_NvmDemo_BootCounter,
        .RomBlockDataAddress    = &NvM_Rom_BootCounter,
        .SelectBlockForReadAll  = TRUE,
        .SelectBlockForWriteAll = FALSE,       /* ghi ngay bằng NvM_WriteBlock lúc khởi động */
        .UseSetRamBlockStatus   = FALSE,
        .MaxNumOfWriteRetries   = 1u
    },
    [NvMConf_NvMBlockDescriptor_LedConfig] = {
        .NvBlockBaseNumber      = 3u,
        .NvramDeviceId          = 0u,
        .ManagementType         = NVM_BLOCK_NATIVE,
        .NvBlockLength          = (uint16)sizeof(NvmDemo_LedConfigType),
        .CrcType                = NVM_CRC16,
        .RamBlockDataAddress    = &Rte_Pim_NvmDemo_LedConfig,
        .RomBlockDataAddress    = &NvM_Rom_LedConfig,
        .SelectBlockForReadAll  = TRUE,
        .SelectBlockForWriteAll = TRUE,        /* chỉ ghi khi tắt máy (NvM_WriteAll) */
        .UseSetRamBlockStatus   = TRUE,
        .MaxNumOfWriteRetries   = 1u
    },
};
