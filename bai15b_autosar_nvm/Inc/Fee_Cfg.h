/* Fee_Cfg.h - cấu hình tiền biên dịch của Fee.
 * Bố cục trong Flash là thiết kế riêng của hiện thực này: SWS Fee không chuẩn hóa cách đặt dữ liệu
 * và cách dọn rác (ghi chú dưới SWS_Fee_00007 và SWS_Fee_00037).
 *   Hai vùng (bank) luân phiên, mỗi vùng 2 sector = 2 KB:
 *     bank 0: offset Fls 0x000..0x7FF (0x0800 F000..F7FF), bank 1: 0x800..0xFFF (0x0800 F800..FFFF)
 *   Đầu bank: [seq 16 bit][magic 0x5EE5] - ghi SAU CÙNG khi định dạng hoặc dọn rác.
 *   Bản ghi:  [block number 16 bit][độ dài 16 bit][dữ liệu, đệm cho chẵn 2 byte][commit 0xA55A]
 *             độ dài 0 = block bị vô hiệu (Fee_InvalidateBlock). */
#ifndef FEE_CFG_H
#define FEE_CFG_H

#define FEE_VIRTUAL_PAGE_SIZE     2u        /* FeeVirtualPageSize = bội của FlsPageSize (SWS_Fee_00076) */
#define FEE_BANK_SIZE             2048u
#define FEE_NUM_BANKS             2u
#define FEE_BANK_HDR_SIZE         4u
#define FEE_REC_HDR_SIZE          4u
#define FEE_REC_COMMIT_SIZE       2u
#define FEE_BANK_MAGIC            0x5EE5u
#define FEE_COMMIT_MAGIC          0xA55Au
#define FEE_MAX_BLOCK_SIZE        16u       /* đủ cho mọi FeeBlockSize đã cấu hình */
#define FEE_NUM_BLOCKS            2u

#define FEE_DEV_ERROR_DETECT      STD_ON
#define FEE_POLLING_MODE          STD_ON    /* FeePollingMode */

#endif /* FEE_CFG_H */
