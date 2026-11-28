/* Fee.c - Flash EEPROM Emulation. AUTOSAR CP SWS FlashEEPROMEmulation R20-11.
 * Fee chỉ gọi API của Fls, không truy cập thanh ghi hay địa chỉ vật lý (Fee độc lập vi điều khiển).
 * Mỗi lần ghi, Fee thêm một bản ghi mới vào cuối bank đang dùng thay vì xóa sector, nên số lần xóa
 * giảm theo số bản ghi chứa được trong một bank (SWS_Fee_00103). Khi bank đầy, Fee dọn rác: xóa bank kia,
 * chép bản ghi mới nhất của từng block sang, rồi mới ghi đầu bank (bố cục: xem Fee_Cfg.h).
 * Bản ghi chưa có commit được coi là "corrupted" (SWS_Fee_00153, 00154) và đọc ra MEMIF_BLOCK_INCONSISTENT. */
#include "Fee.h"
#include "Fls.h"
#include "Det.h"

#define FEE_NONE              0xFFFFu
#define FEE_PAD(len)          ((uint16)(((uint16)(len) + 1u) & 0xFFFEu))
#define FEE_REC_SIZE(len)     ((uint32)FEE_REC_HDR_SIZE + (uint32)FEE_PAD(len) + (uint32)FEE_REC_COMMIT_SIZE)
#define FEE_BANK_BASE(b)      ((uint32)(b) * (uint32)FEE_BANK_SIZE)
#define FEE_BANK_END(b)       (FEE_BANK_BASE(b) + (uint32)FEE_BANK_SIZE)
#define FEE_DET(api, err)     (void)Det_ReportError(DET_MODULE_ID_FEE, 0u, (api), (err))
#define FEE_DET_RT(api, err)  (void)Det_ReportRuntimeError(DET_MODULE_ID_FEE, 0u, (api), (err))

typedef enum {
    FEE_ST_IDLE = 0,
    FEE_ST_INIT_HDR0, FEE_ST_INIT_HDR1, FEE_ST_INIT_FMT_ERASE, FEE_ST_INIT_FMT_HDR, FEE_ST_INIT_SCAN,
    FEE_ST_JOB_START,
    FEE_ST_RD_COMMIT, FEE_ST_RD_DATA,
    FEE_ST_WR_HDRDATA, FEE_ST_WR_COMMIT,
    FEE_ST_INV_WRITE,
    FEE_ST_GC_ERASE, FEE_ST_GC_READ, FEE_ST_GC_WRITE, FEE_ST_GC_HDR
} Fee_StateType;

typedef enum { FEE_JOB_NONE = 0, FEE_JOB_READ, FEE_JOB_WRITE, FEE_JOB_INVALIDATE } Fee_JobType;

typedef struct {
    uint16 RecOffset;     /* offset Fls của bản ghi mới nhất, FEE_NONE nếu chưa có */
    uint16 Length;        /* độ dài trong header bản ghi (0 = block bị vô hiệu) */
} Fee_IndexType;

static const Fee_ConfigType *Fee_CfgPtr     = NULL_PTR;
static MemIf_StatusType      Fee_Status     = MEMIF_UNINIT;
static MemIf_JobResultType   Fee_JobRes     = MEMIF_JOB_OK;
static Fee_StateType         Fee_State      = FEE_ST_IDLE;
static boolean               Fee_FlsPending = FALSE;

static Fee_JobType  Fee_Job       = FEE_JOB_NONE;
static uint8        Fee_JobIdx    = 0u;            /* chỉ số block trong bảng cấu hình */
static uint16       Fee_JobOffset = 0u;
static uint16       Fee_JobLength = 0u;
static uint8       *Fee_JobDst    = NULL_PTR;
static const uint8 *Fee_JobSrc    = NULL_PTR;
static uint32       Fee_JobRec    = 0u;            /* offset của bản ghi đang ghi */

static Fee_IndexType Fee_Index[FEE_NUM_BLOCKS];
static Fee_IndexType Fee_NewIndex[FEE_NUM_BLOCKS]; /* chỉ mục của bank đích khi dọn rác */
static uint8         Fee_GcIdx = 0u;
static uint32        Fee_GcDst = 0u;

static uint8  Fee_Buf[FEE_REC_HDR_SIZE + FEE_MAX_BLOCK_SIZE + FEE_REC_COMMIT_SIZE];
static uint8  Fee_Hdr0[FEE_BANK_HDR_SIZE];
static uint8  Fee_Hdr1[FEE_BANK_HDR_SIZE];
static uint32 Fee_ScanPos = 0u;

volatile uint8  Fee_ActiveBank  = 0u;
volatile uint16 Fee_BankSeq     = 0u;
volatile uint32 Fee_FreeOffset  = 0u;
volatile uint32 Fee_GcCount     = 0u;
volatile uint32 Fee_FormatCount = 0u;
volatile uint32 Fee_ScanRecords = 0u;

static uint16 Fee_Get16(const uint8 *p)     { return (uint16)((uint16)p[0] | ((uint16)p[1] << 8)); }
static void   Fee_Put16(uint8 *p, uint16 v) { p[0] = (uint8)(v & 0xFFu); p[1] = (uint8)(v >> 8); }

static sint8 Fee_FindBlock(uint16 BlockNumber)
{
    for (uint8 i = 0u; i < Fee_CfgPtr->NumBlocks; i++) {
        if (Fee_CfgPtr->Blocks[i].BlockNumber == BlockNumber) {
            return (sint8)i;
        }
    }
    return -1;
}

/* ---------- gửi job xuống Fls ---------- */

static boolean Fee_FlsRead(uint32 Off, uint8 *Buf, uint32 Len, Fee_StateType Next)
{
    if (Fls_Read(Off, Buf, Len) != E_OK) { return FALSE; }
    Fee_FlsPending = TRUE;
    Fee_State      = Next;
    return TRUE;
}

static boolean Fee_FlsWrite(uint32 Off, const uint8 *Buf, uint32 Len, Fee_StateType Next)
{
    if (Fls_Write(Off, Buf, Len) != E_OK) { return FALSE; }
    Fee_FlsPending = TRUE;
    Fee_State      = Next;
    return TRUE;
}

static boolean Fee_FlsErase(uint32 Off, uint32 Len, Fee_StateType Next)
{
    if (Fls_Erase(Off, Len) != E_OK) { return FALSE; }
    Fee_FlsPending = TRUE;
    Fee_State      = Next;
    return TRUE;
}

/* ---------- kết thúc job của tầng trên ---------- */

static void Fee_FinishJob(MemIf_JobResultType Result)
{
    Fee_JobRes = Result;
    Fee_Job    = FEE_JOB_NONE;
    Fee_State  = FEE_ST_IDLE;
    Fee_Status = MEMIF_IDLE;
}

static void Fee_InitFailed(void)
{
    Fee_State = FEE_ST_IDLE;          /* giữ MEMIF_UNINIT: tầng trên không gửi được job */
    FEE_DET(FEE_SID_MAINFUNCTION, FEE_E_UNINIT);
}

static void Fee_InitDone(void)
{
    Fee_State  = FEE_ST_IDLE;
    Fee_Status = MEMIF_IDLE;          /* SWS_Fee_00169: khởi tạo xong trong main function */
}

/* ---------- khởi tạo: chọn bank, định dạng hoặc quét ---------- */

static void Fee_ScanNext(void)
{
    uint32 end = FEE_BANK_END(Fee_ActiveBank);
    if ((Fee_ScanPos + FEE_REC_HDR_SIZE) > end) {
        Fee_FreeOffset = end;
        Fee_InitDone();
        return;
    }
    if (Fee_FlsRead(Fee_ScanPos, Fee_Buf, FEE_REC_HDR_SIZE, FEE_ST_INIT_SCAN) == FALSE) {
        Fee_InitFailed();
    }
}

static void Fee_OnScan(void)
{
    uint16 blk = Fee_Get16(&Fee_Buf[0]);
    uint16 len = Fee_Get16(&Fee_Buf[2]);
    uint32 end = FEE_BANK_END(Fee_ActiveBank);

    if ((blk == 0xFFFFu) && (len == 0xFFFFu)) {        /* hết bản ghi: phần còn lại của bank trống */
        Fee_FreeOffset = Fee_ScanPos;
        Fee_InitDone();
        return;
    }
    if ((blk == 0xFFFFu) || (len == 0xFFFFu) || (len > FEE_MAX_BLOCK_SIZE) ||
        ((Fee_ScanPos + FEE_REC_SIZE(len)) > end)) {
        /* header ghi dở (mất điện giữa chừng): không biết bản ghi dài bao nhiêu,
         * nên coi bank đã đầy; lần ghi kế tiếp sẽ dọn rác sang bank kia */
        Fee_FreeOffset = end;
        Fee_InitDone();
        return;
    }
    sint8 idx = Fee_FindBlock(blk);
    if (idx >= 0) {                                   /* bản ghi sau thay thế bản ghi trước */
        Fee_Index[idx].RecOffset = (uint16)Fee_ScanPos;
        Fee_Index[idx].Length    = len;
    }
    Fee_ScanRecords++;
    Fee_ScanPos += FEE_REC_SIZE(len);
    Fee_ScanNext();
}

static void Fee_SelectBank(void)
{
    boolean v0 = (Fee_Get16(&Fee_Hdr0[2]) == FEE_BANK_MAGIC) ? TRUE : FALSE;
    boolean v1 = (Fee_Get16(&Fee_Hdr1[2]) == FEE_BANK_MAGIC) ? TRUE : FALSE;
    uint16  s0 = Fee_Get16(&Fee_Hdr0[0]);
    uint16  s1 = Fee_Get16(&Fee_Hdr1[0]);

    if ((v0 == FALSE) && (v1 == FALSE)) {             /* Flash chưa từng định dạng: xóa bank 0 */
        if (Fee_FlsErase(FEE_BANK_BASE(0u), FEE_BANK_SIZE, FEE_ST_INIT_FMT_ERASE) == FALSE) {
            Fee_InitFailed();
        }
        return;
    }
    if ((v0 != FALSE) && (v1 != FALSE)) {             /* cả hai hợp lệ: chọn số thứ tự mới hơn */
        Fee_ActiveBank = ((sint16)(s1 - s0) > 0) ? 1u : 0u;
    } else {
        Fee_ActiveBank = (v1 != FALSE) ? 1u : 0u;
    }
    Fee_BankSeq = (Fee_ActiveBank == 1u) ? s1 : s0;
    Fee_ScanPos = FEE_BANK_BASE(Fee_ActiveBank) + FEE_BANK_HDR_SIZE;
    Fee_ScanNext();
}

/* ---------- dọn rác ---------- */

static void Fee_StartUserOp(void);

static void Fee_GcFailed(void)
{
    Fee_FinishJob(MEMIF_JOB_FAILED);                  /* bank đang dùng chưa bị động tới */
}

static void Fee_GcNext(void)
{
    uint8 other = (uint8)(Fee_ActiveBank ^ 1u);
    while (Fee_GcIdx < Fee_CfgPtr->NumBlocks) {
        if (Fee_Index[Fee_GcIdx].RecOffset != FEE_NONE) {
            if (Fee_FlsRead(Fee_Index[Fee_GcIdx].RecOffset, Fee_Buf,
                            FEE_REC_SIZE(Fee_Index[Fee_GcIdx].Length), FEE_ST_GC_READ) == FALSE) {
                Fee_GcFailed();
            }
            return;
        }
        Fee_GcIdx++;
    }
    /* đã chép xong: ghi đầu bank (seq trước, magic sau cùng) để bank mới có hiệu lực */
    Fee_Put16(&Fee_Hdr0[0], (uint16)(Fee_BankSeq + 1u));
    Fee_Put16(&Fee_Hdr0[2], FEE_BANK_MAGIC);
    if (Fee_FlsWrite(FEE_BANK_BASE(other), Fee_Hdr0, FEE_BANK_HDR_SIZE, FEE_ST_GC_HDR) == FALSE) {
        Fee_GcFailed();
    }
}

static void Fee_StartGc(void)
{
    uint8 other = (uint8)(Fee_ActiveBank ^ 1u);
    for (uint8 i = 0u; i < FEE_NUM_BLOCKS; i++) {
        Fee_NewIndex[i].RecOffset = FEE_NONE;
        Fee_NewIndex[i].Length    = 0u;
    }
    Fee_GcIdx = 0u;
    Fee_GcDst = FEE_BANK_BASE(other) + FEE_BANK_HDR_SIZE;
    if (Fee_FlsErase(FEE_BANK_BASE(other), FEE_BANK_SIZE, FEE_ST_GC_ERASE) == FALSE) {
        Fee_GcFailed();
    }
}

/* ---------- job của tầng trên ---------- */

static void Fee_StartRead(void)
{
    const Fee_IndexType *ix = &Fee_Index[Fee_JobIdx];
    if (ix->RecOffset == FEE_NONE) {
        Fee_FinishJob(MEMIF_BLOCK_INCONSISTENT);      /* không tìm thấy block (SWS_MemIf_00065) */
        return;
    }
    uint32 commitOff = (uint32)ix->RecOffset + FEE_REC_HDR_SIZE + FEE_PAD(ix->Length);
    if (Fee_FlsRead(commitOff, Fee_Buf, FEE_REC_COMMIT_SIZE, FEE_ST_RD_COMMIT) == FALSE) {
        Fee_FinishJob(MEMIF_JOB_FAILED);
    }
}

static void Fee_OnReadCommit(void)
{
    const Fee_IndexType *ix = &Fee_Index[Fee_JobIdx];
    if (Fee_Get16(Fee_Buf) != FEE_COMMIT_MAGIC) {
        Fee_FinishJob(MEMIF_BLOCK_INCONSISTENT);      /* SWS_Fee_00023: bản ghi ghi dở */
        return;
    }
    if (ix->Length == 0u) {
        Fee_FinishJob(MEMIF_BLOCK_INVALID);           /* SWS_Fee_00075 */
        return;
    }
    if (ix->Length != Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize) {
        Fee_FinishJob(MEMIF_BLOCK_INCONSISTENT);
        return;
    }
    uint32 dataOff = (uint32)ix->RecOffset + FEE_REC_HDR_SIZE + Fee_JobOffset;
    if (Fee_FlsRead(dataOff, Fee_JobDst, Fee_JobLength, FEE_ST_RD_DATA) == FALSE) {
        Fee_FinishJob(MEMIF_JOB_FAILED);
    }
}

static void Fee_BeginWrite(void)
{
    uint16 size = Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize;
    uint16 pad  = FEE_PAD(size);

    Fee_Put16(&Fee_Buf[0], Fee_CfgPtr->Blocks[Fee_JobIdx].BlockNumber);
    Fee_Put16(&Fee_Buf[2], size);
    for (uint16 i = 0u; i < pad; i++) {
        Fee_Buf[FEE_REC_HDR_SIZE + i] = (i < size) ? Fee_JobSrc[i] : 0xFFu;
    }
    Fee_JobRec = Fee_FreeOffset;
    /* SWS_Fee_00153: từ lúc này block được coi là "corrupted" cho tới khi commit được ghi */
    Fee_Index[Fee_JobIdx].RecOffset = (uint16)Fee_JobRec;
    Fee_Index[Fee_JobIdx].Length    = size;
    if (Fee_FlsWrite(Fee_JobRec, Fee_Buf, (uint32)FEE_REC_HDR_SIZE + pad, FEE_ST_WR_HDRDATA) == FALSE) {
        Fee_FinishJob(MEMIF_JOB_FAILED);
    }
}

static void Fee_BeginInvalidate(void)
{
    Fee_Put16(&Fee_Buf[0], Fee_CfgPtr->Blocks[Fee_JobIdx].BlockNumber);
    Fee_Put16(&Fee_Buf[2], 0u);                       /* độ dài 0 = vô hiệu */
    Fee_Put16(&Fee_Buf[4], FEE_COMMIT_MAGIC);
    Fee_JobRec = Fee_FreeOffset;
    Fee_Index[Fee_JobIdx].RecOffset = (uint16)Fee_JobRec;
    Fee_Index[Fee_JobIdx].Length    = 0u;
    if (Fee_FlsWrite(Fee_JobRec, Fee_Buf, FEE_REC_SIZE(0u), FEE_ST_INV_WRITE) == FALSE) {
        Fee_FinishJob(MEMIF_JOB_FAILED);
    }
}

/* Ghi hoặc vô hiệu: dọn rác trước nếu bank đang dùng không đủ chỗ */
static void Fee_StartUserOp(void)
{
    uint16 len  = (Fee_Job == FEE_JOB_WRITE) ? Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize : 0u;
    uint32 need = FEE_REC_SIZE(len);

    if ((Fee_FreeOffset + need) > FEE_BANK_END(Fee_ActiveBank)) {
        Fee_StartGc();
        return;
    }
    if (Fee_Job == FEE_JOB_WRITE) { Fee_BeginWrite(); }
    else                          { Fee_BeginInvalidate(); }
}

static void Fee_StartJob(void)
{
    switch (Fee_Job) {
    case FEE_JOB_READ:       Fee_StartRead();   break;
    case FEE_JOB_WRITE:
    case FEE_JOB_INVALIDATE: Fee_StartUserOp(); break;
    default:                 Fee_FinishJob(MEMIF_JOB_FAILED); break;
    }
}

/* ---------- xử lý khi job Fls kết thúc ---------- */

static void Fee_OnFlsDone(MemIf_JobResultType r)
{
    boolean ok = (r == MEMIF_JOB_OK) ? TRUE : FALSE;
    uint8 other = (uint8)(Fee_ActiveBank ^ 1u);

    switch (Fee_State) {
    case FEE_ST_INIT_HDR0:
        if ((ok == FALSE) ||
            (Fee_FlsRead(FEE_BANK_BASE(1u), Fee_Hdr1, FEE_BANK_HDR_SIZE, FEE_ST_INIT_HDR1) == FALSE)) {
            Fee_InitFailed();
        }
        break;
    case FEE_ST_INIT_HDR1:
        if (ok == FALSE) { Fee_InitFailed(); } else { Fee_SelectBank(); }
        break;
    case FEE_ST_INIT_FMT_ERASE:
        Fee_Put16(&Fee_Hdr0[0], 1u);
        Fee_Put16(&Fee_Hdr0[2], FEE_BANK_MAGIC);
        if ((ok == FALSE) ||
            (Fee_FlsWrite(FEE_BANK_BASE(0u), Fee_Hdr0, FEE_BANK_HDR_SIZE, FEE_ST_INIT_FMT_HDR) == FALSE)) {
            Fee_InitFailed();
        }
        break;
    case FEE_ST_INIT_FMT_HDR:
        if (ok == FALSE) { Fee_InitFailed(); break; }
        Fee_ActiveBank = 0u;
        Fee_BankSeq    = 1u;
        Fee_FreeOffset = FEE_BANK_BASE(0u) + FEE_BANK_HDR_SIZE;
        Fee_FormatCount++;
        Fee_InitDone();
        break;
    case FEE_ST_INIT_SCAN:
        if (ok == FALSE) { Fee_InitFailed(); } else { Fee_OnScan(); }
        break;

    case FEE_ST_RD_COMMIT:
        if (ok == FALSE) { Fee_FinishJob(MEMIF_JOB_FAILED); } else { Fee_OnReadCommit(); }
        break;
    case FEE_ST_RD_DATA:
        Fee_FinishJob((ok != FALSE) ? MEMIF_JOB_OK : MEMIF_JOB_FAILED);
        break;

    case FEE_ST_WR_HDRDATA:
        if (ok == FALSE) {
            Fee_FreeOffset = Fee_JobRec + FEE_REC_SIZE(Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize);
            Fee_FinishJob(MEMIF_JOB_FAILED);
            break;
        }
        Fee_Put16(&Fee_Buf[0], FEE_COMMIT_MAGIC);
        if (Fee_FlsWrite(Fee_JobRec + FEE_REC_HDR_SIZE + FEE_PAD(Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize),
                         Fee_Buf, FEE_REC_COMMIT_SIZE, FEE_ST_WR_COMMIT) == FALSE) {
            Fee_FinishJob(MEMIF_JOB_FAILED);
        }
        break;
    case FEE_ST_WR_COMMIT:                            /* SWS_Fee_00154: block hết "corrupted" */
        Fee_FreeOffset = Fee_JobRec + FEE_REC_SIZE(Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize);
        Fee_FinishJob((ok != FALSE) ? MEMIF_JOB_OK : MEMIF_JOB_FAILED);
        break;
    case FEE_ST_INV_WRITE:
        Fee_FreeOffset = Fee_JobRec + FEE_REC_SIZE(0u);
        Fee_FinishJob((ok != FALSE) ? MEMIF_JOB_OK : MEMIF_JOB_FAILED);
        break;

    case FEE_ST_GC_ERASE:
        if (ok == FALSE) { Fee_GcFailed(); } else { Fee_GcNext(); }
        break;
    case FEE_ST_GC_READ: {
        uint16 len = Fee_Index[Fee_GcIdx].Length;
        uint32 rs  = FEE_REC_SIZE(len);
        if (ok == FALSE) { Fee_GcFailed(); break; }
        if (Fee_Get16(&Fee_Buf[rs - FEE_REC_COMMIT_SIZE]) != FEE_COMMIT_MAGIC) {
            Fee_GcIdx++;                              /* bản ghi ghi dở: không chép */
            Fee_GcNext();
            break;
        }
        Fee_NewIndex[Fee_GcIdx].RecOffset = (uint16)Fee_GcDst;
        Fee_NewIndex[Fee_GcIdx].Length    = len;
        if (Fee_FlsWrite(Fee_GcDst, Fee_Buf, rs, FEE_ST_GC_WRITE) == FALSE) { Fee_GcFailed(); }
        break;
    }
    case FEE_ST_GC_WRITE:
        if (ok == FALSE) { Fee_GcFailed(); break; }
        Fee_GcDst += FEE_REC_SIZE(Fee_Index[Fee_GcIdx].Length);
        Fee_GcIdx++;
        Fee_GcNext();
        break;
    case FEE_ST_GC_HDR:
        if (ok == FALSE) { Fee_GcFailed(); break; }
        Fee_ActiveBank = other;
        Fee_BankSeq    = (uint16)(Fee_BankSeq + 1u);
        Fee_FreeOffset = Fee_GcDst;
        for (uint8 i = 0u; i < FEE_NUM_BLOCKS; i++) { Fee_Index[i] = Fee_NewIndex[i]; }
        Fee_GcCount++;
        {
            uint16 len  = (Fee_Job == FEE_JOB_WRITE) ? Fee_CfgPtr->Blocks[Fee_JobIdx].BlockSize : 0u;
            if ((Fee_FreeOffset + FEE_REC_SIZE(len)) > FEE_BANK_END(Fee_ActiveBank)) {
                Fee_FinishJob(MEMIF_JOB_FAILED);      /* vẫn không đủ chỗ sau khi dọn */
            } else if (Fee_Job == FEE_JOB_WRITE) {
                Fee_BeginWrite();
            } else {
                Fee_BeginInvalidate();
            }
        }
        break;
    default:
        break;
    }
}

/* ---------- API ---------- */

void Fee_Init(const Fee_ConfigType *ConfigPtr)
{
    if ((ConfigPtr == NULL_PTR) || (ConfigPtr->NumBlocks > FEE_NUM_BLOCKS)) {
        FEE_DET(FEE_SID_INIT, FEE_E_PARAM_POINTER);
        return;
    }
    Fee_CfgPtr     = ConfigPtr;
    Fee_Status     = MEMIF_UNINIT;
    Fee_JobRes     = MEMIF_JOB_OK;
    Fee_Job        = FEE_JOB_NONE;
    Fee_FlsPending = FALSE;
    Fee_ScanRecords = 0u;
    for (uint8 i = 0u; i < FEE_NUM_BLOCKS; i++) {
        Fee_Index[i].RecOffset = FEE_NONE;
        Fee_Index[i].Length    = 0u;
    }
    /* Fee_Init bất đồng bộ (SWS_Fee_00085): đọc đầu hai bank, chọn bank, quét trong Fee_MainFunction */
    if (Fee_FlsRead(FEE_BANK_BASE(0u), Fee_Hdr0, FEE_BANK_HDR_SIZE, FEE_ST_INIT_HDR0) == FALSE) {
        Fee_InitFailed();
    }
}

/* SWS_Fee_00022/00025: chấp nhận khi IDLE (hoặc BUSY_INTERNAL); SWS_Fee_00172/00174: từ chối khi UNINIT, BUSY */
static boolean Fee_CheckAccept(uint8 Api)
{
    if (Fee_Status == MEMIF_UNINIT) { FEE_DET(Api, FEE_E_UNINIT);  return FALSE; }
    if (Fee_Status == MEMIF_BUSY)   { FEE_DET_RT(Api, FEE_E_BUSY); return FALSE; }
    return TRUE;
}

Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8 *DataBufferPtr, uint16 Length)
{
    if (Fee_CheckAccept(FEE_SID_READ) == FALSE) { return E_NOT_OK; }
    sint8 idx = Fee_FindBlock(BlockNumber);
    if (idx < 0)                    { FEE_DET(FEE_SID_READ, FEE_E_INVALID_BLOCK_NO);  return E_NOT_OK; }
    uint16 size = Fee_CfgPtr->Blocks[idx].BlockSize;
    if (BlockOffset >= size)        { FEE_DET(FEE_SID_READ, FEE_E_INVALID_BLOCK_OFS); return E_NOT_OK; }
    if (DataBufferPtr == NULL_PTR)  { FEE_DET(FEE_SID_READ, FEE_E_PARAM_POINTER);     return E_NOT_OK; }
    if ((Length == 0u) || (((uint32)BlockOffset + Length) > size)) {
        FEE_DET(FEE_SID_READ, FEE_E_INVALID_BLOCK_LEN);
        return E_NOT_OK;
    }
    Fee_Job       = FEE_JOB_READ;                     /* SWS_Fee_00021, 00022 */
    Fee_JobIdx    = (uint8)idx;
    Fee_JobOffset = BlockOffset;
    Fee_JobLength = Length;
    Fee_JobDst    = DataBufferPtr;
    Fee_State     = FEE_ST_JOB_START;
    Fee_Status    = MEMIF_BUSY;
    Fee_JobRes    = MEMIF_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8 *DataBufferPtr)
{
    if (Fee_CheckAccept(FEE_SID_WRITE) == FALSE) { return E_NOT_OK; }
    sint8 idx = Fee_FindBlock(BlockNumber);
    if (idx < 0)                   { FEE_DET(FEE_SID_WRITE, FEE_E_INVALID_BLOCK_NO); return E_NOT_OK; }
    if (DataBufferPtr == NULL_PTR) { FEE_DET(FEE_SID_WRITE, FEE_E_PARAM_POINTER);    return E_NOT_OK; }
    Fee_Job    = FEE_JOB_WRITE;                       /* SWS_Fee_00024, 00025: offset luôn bằng 0 */
    Fee_JobIdx = (uint8)idx;
    Fee_JobSrc = DataBufferPtr;
    Fee_State  = FEE_ST_JOB_START;
    Fee_Status = MEMIF_BUSY;
    Fee_JobRes = MEMIF_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber)
{
    if (Fee_CheckAccept(FEE_SID_INVALIDATEBLOCK) == FALSE) { return E_NOT_OK; }
    sint8 idx = Fee_FindBlock(BlockNumber);
    if (idx < 0) { FEE_DET(FEE_SID_INVALIDATEBLOCK, FEE_E_INVALID_BLOCK_NO); return E_NOT_OK; }
    Fee_Job    = FEE_JOB_INVALIDATE;                  /* SWS_Fee_00036, 00037, 00192 */
    Fee_JobIdx = (uint8)idx;
    Fee_State  = FEE_ST_JOB_START;
    Fee_Status = MEMIF_BUSY;
    Fee_JobRes = MEMIF_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber)
{
    if (Fee_CheckAccept(FEE_SID_ERASEIMMEDIATE) == FALSE) { return E_NOT_OK; }
    sint8 idx = Fee_FindBlock(BlockNumber);
    if ((idx < 0) || (Fee_CfgPtr->Blocks[idx].ImmediateData == FALSE)) {
        FEE_DET(FEE_SID_ERASEIMMEDIATE, FEE_E_INVALID_BLOCK_NO);   /* SWS_Fee_00068, 00141 */
        return E_NOT_OK;
    }
    return E_NOT_OK;                                  /* ví dụ không cấu hình block immediate */
}

void Fee_Cancel(void)
{
    if (Fee_Status == MEMIF_UNINIT) { FEE_DET(FEE_SID_CANCEL, FEE_E_UNINIT); return; }
    if (Fee_Status != MEMIF_BUSY) {                   /* SWS_Fee_00164, 00184 */
        FEE_DET_RT(FEE_SID_CANCEL, FEE_E_INVALID_CANCEL);
        return;
    }
    Fls_Cancel();                                     /* SWS_Fee_00080 */
    Fee_FlsPending = FALSE;
    Fee_Job        = FEE_JOB_NONE;
    Fee_State      = FEE_ST_IDLE;
    Fee_Status     = MEMIF_IDLE;                      /* SWS_Fee_00081 */
    Fee_JobRes     = MEMIF_JOB_CANCELED;              /* SWS_Fee_00157 */
}

MemIf_StatusType Fee_GetStatus(void)
{
    return Fee_Status;                                /* SWS_Fee_00034, 00128, 00129 */
}

MemIf_JobResultType Fee_GetJobResult(void)
{
    if (Fee_Status == MEMIF_UNINIT) {                 /* SWS_Fee_00125 */
        FEE_DET(FEE_SID_GETJOBRESULT, FEE_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
    return Fee_JobRes;
}

void Fee_MainFunction(void)
{
    if (Fee_FlsPending != FALSE) {                    /* FeePollingMode: hỏi kết quả job Fls */
        if (Fls_GetStatus() == MEMIF_BUSY) {
            return;
        }
        Fee_FlsPending = FALSE;
        Fee_OnFlsDone(Fls_GetJobResult());
        return;
    }
    if (Fee_State == FEE_ST_JOB_START) {              /* SWS_Fee_00057 */
        Fee_StartJob();
    }
}

Std_ReturnType Fee_Dbg_GetDataOffset(uint16 BlockNumber, uint32 *FlsOffsetPtr)
{
    sint8 idx = Fee_FindBlock(BlockNumber);
    if ((idx < 0) || (FlsOffsetPtr == NULL_PTR) || (Fee_Index[idx].RecOffset == FEE_NONE)) {
        return E_NOT_OK;
    }
    *FlsOffsetPtr = (uint32)Fee_Index[idx].RecOffset + FEE_REC_HDR_SIZE;
    return E_OK;
}
