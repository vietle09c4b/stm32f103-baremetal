/* Fee.h - Flash EEPROM Emulation (ECU Abstraction Layer, nhóm Memory Hardware Abstraction).
 * AUTOSAR CP SWS FlashEEPROMEmulation R20-11. Service ID theo chương 8.3:
 * Init 0x00, Read 0x02, Write 0x03, Cancel 0x04, GetStatus 0x05, GetJobResult 0x06,
 * InvalidateBlock 0x07, EraseImmediateBlock 0x09, MainFunction 0x12.
 * Không hiện thực: Fee_SetMode (FeeSetModeSupported = FALSE), Fee_GetVersionInfo,
 * Fee_JobEndNotification / Fee_JobErrorNotification (FeePollingMode = TRUE: Fee hỏi Fls_GetJobResult). */
#ifndef FEE_H
#define FEE_H

#include "Std_Types.h"
#include "MemIf.h"
#include "Fee_Cfg.h"

typedef struct {
    uint16  BlockNumber;        /* FeeBlockNumber: khác 0x0000 và 0xFFFF (SWS_Fee_00006) */
    uint16  BlockSize;          /* FeeBlockSize (byte) */
    boolean ImmediateData;      /* FeeImmediateData */
    uint32  NumberOfWriteCycles;/* FeeNumberOfWriteCycles */
} Fee_BlockConfigType;

typedef struct {
    const Fee_BlockConfigType *Blocks;
    uint8                      NumBlocks;
} Fee_ConfigType;

/* Mã lỗi development (SWS_Fee_00010) và runtime (SWS_Fee_91002) */
#define FEE_E_UNINIT              0x01u
#define FEE_E_INVALID_BLOCK_NO    0x02u
#define FEE_E_INVALID_BLOCK_OFS   0x03u
#define FEE_E_PARAM_POINTER       0x04u
#define FEE_E_INVALID_BLOCK_LEN   0x05u
#define FEE_E_BUSY                0x06u
#define FEE_E_INVALID_CANCEL      0x08u

#define FEE_SID_INIT              0x00u
#define FEE_SID_READ              0x02u
#define FEE_SID_WRITE             0x03u
#define FEE_SID_CANCEL            0x04u
#define FEE_SID_GETSTATUS         0x05u
#define FEE_SID_GETJOBRESULT      0x06u
#define FEE_SID_INVALIDATEBLOCK   0x07u
#define FEE_SID_ERASEIMMEDIATE    0x09u
#define FEE_SID_MAINFUNCTION      0x12u

void                Fee_Init(const Fee_ConfigType *ConfigPtr);
Std_ReturnType      Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8 *DataBufferPtr, uint16 Length);
Std_ReturnType      Fee_Write(uint16 BlockNumber, const uint8 *DataBufferPtr);
void                Fee_Cancel(void);
MemIf_StatusType    Fee_GetStatus(void);
MemIf_JobResultType Fee_GetJobResult(void);
Std_ReturnType      Fee_InvalidateBlock(uint16 BlockNumber);
Std_ReturnType      Fee_EraseImmediateBlock(uint16 BlockNumber);
void                Fee_MainFunction(void);   /* SchM_Fee.h trong hệ thống thật */

extern const Fee_ConfigType Fee_Config;

/* Hàm gỡ lỗi, KHÔNG thuộc SWS: trả offset Fls của vùng dữ liệu trong bản ghi mới nhất của một block.
 * Chỉ phép thử T3 dùng để biết ô nào cần làm hỏng. */
Std_ReturnType Fee_Dbg_GetDataOffset(uint16 BlockNumber, uint32 *FlsOffsetPtr);

/* Biến quan sát khi debug */
extern volatile uint8  Fee_ActiveBank;      /* vùng đang dùng: 0 hoặc 1 */
extern volatile uint16 Fee_BankSeq;         /* số thứ tự của vùng đang dùng */
extern volatile uint32 Fee_FreeOffset;      /* offset Fls của ô trống đầu tiên */
extern volatile uint32 Fee_GcCount;         /* số lần garbage collection */
extern volatile uint32 Fee_FormatCount;     /* số lần định dạng lúc khởi động */
extern volatile uint32 Fee_ScanRecords;     /* số bản ghi đã quét lúc khởi động */

#endif /* FEE_H */
