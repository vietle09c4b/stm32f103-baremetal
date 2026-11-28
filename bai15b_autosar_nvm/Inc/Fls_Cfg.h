/* Fls_Cfg.h - cấu hình tiền biên dịch của Fls (trong dự án thật do công cụ cấu hình sinh ra).
 * Vùng dành cho Fls: 4 trang cuối của STM32F103C8, 0x0800 F000 - 0x0800 FFFF (trang 60..63, PM0075 Table 1).
 * Linker script chỉ cấp 60 KB cho chương trình nên mã không bao giờ nằm trong vùng này. */
#ifndef FLS_CFG_H
#define FLS_CFG_H

#define FLS_BASE_ADDRESS            0x0800F000UL
#define FLS_TOTAL_SIZE              4096u
#define FLS_SECTOR_SIZE             1024u    /* trang 1 KB, medium-density (PM0075 1.2) */
#define FLS_PAGE_SIZE               2u       /* ghi theo half-word (PM0075 2.3.3) */
#define FLS_ERASED_VALUE            0xFFu    /* FlsErasedValue */

#define FLS_DEV_ERROR_DETECT        STD_ON   /* FlsDevErrorDetect */
#define FLS_WRITE_VERIFICATION      STD_ON   /* FlsWriteVerificationEnabled: SWS_Fls_00056 */
#define FLS_ERASE_VERIFICATION      STD_ON   /* FlsEraseVerificationEnabled: SWS_Fls_00022 (sau xóa)
                                                và SWS_Fls_00055 (ô phải trống trước khi ghi) */

#endif /* FLS_CFG_H */
