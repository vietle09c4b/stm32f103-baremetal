/* Fee_Cfg.c - FeeBlockConfiguration. Block number trùng NvMNvBlockBaseNumber vì NvMDatasetSelectionBits = 0.
 * FeeBlockSize = dữ liệu NvM 4 byte + CRC16 2 byte. */
#include "Fee.h"

static const Fee_BlockConfigType Fee_BlockCfg[FEE_NUM_BLOCKS] = {
    /* BlockNumber, BlockSize, ImmediateData, NumberOfWriteCycles */
    { 2u, 6u, FALSE, 100000u },    /* NvM block BootCounter */
    { 3u, 6u, FALSE, 10000u  },    /* NvM block LedConfig   */
};

const Fee_ConfigType Fee_Config = {
    .Blocks    = Fee_BlockCfg,
    .NumBlocks = FEE_NUM_BLOCKS
};
