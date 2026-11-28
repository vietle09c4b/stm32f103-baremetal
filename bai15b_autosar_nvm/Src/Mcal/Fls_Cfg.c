/* Fls_Cfg.c - bộ cấu hình Fls (FlsConfigSet, FlsSector). */
#include "Fls.h"

const Fls_ConfigType Fls_Config = {
    .BaseAddress        = FLS_BASE_ADDRESS,
    .TotalSize          = FLS_TOTAL_SIZE,
    .SectorSize         = FLS_SECTOR_SIZE,
    .PageSize           = FLS_PAGE_SIZE,
    .MaxReadNormalMode  = 64u,   /* FlsMaxReadNormalMode */
    .MaxWriteNormalMode = 16u    /* FlsMaxWriteNormalMode: 8 half-word, khoảng 0,5 ms mỗi lần gọi */
};
