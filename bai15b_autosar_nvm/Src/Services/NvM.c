/* NvM.c - NVRAM Manager. AUTOSAR CP SWS NVRAMManager R20-11.
 * NvM chỉ gọi MemIf (SWS_NvM_00465) và Crc (SWS_NvM_00466); không biết Fee hay Fls bên dưới.
 * Mọi request bất đồng bộ được xếp hàng và xử lý trong NvM_MainFunction (SWS_NvM_00256).
 * NvM_ReadAll/NvM_WriteAll chỉ bắt đầu khi hàng đợi single block trống (SWS_NvM_00243, 00254).
 * NV block trong Flash = dữ liệu + CRC16 (byte thấp trước); thứ tự byte CRC do hiện thực tự chọn. */
#include "NvM.h"
#include "MemIf.h"
#include "Crc.h"
#include "Det.h"
#include "Dem.h"

#define NVM_DET(api, err)     (void)Det_ReportError(DET_MODULE_ID_NVM, 0u, (api), (err))
#define NVM_FIRST_USER_BLOCK  2u

typedef enum { NVM_OP_NONE = 0, NVM_OP_READ, NVM_OP_WRITE } NvM_OpType;
typedef enum { NVM_MULTI_NONE = 0, NVM_MULTI_READALL, NVM_MULTI_WRITEALL } NvM_MultiType;

typedef struct {
    NvM_BlockIdType BlockId;
    const void     *SrcPtr;
} NvM_QueueEntryType;

static boolean               NvM_InitDone = FALSE;
static NvM_RequestResultType NvM_Result[NVM_NUM_BLOCKS];       /* Administrative block: error/status */
volatile NvM_RamStateType    NvM_RamState[NVM_NUM_BLOCKS];
volatile uint16              NvM_LastCrcCalc[NVM_NUM_BLOCKS];
volatile uint16              NvM_LastCrcStored[NVM_NUM_BLOCKS];
volatile uint32              NvM_WriteRetryCount = 0u;

static NvM_QueueEntryType NvM_Queue[NVM_SIZE_STANDARD_JOB_QUEUE];
static uint8              NvM_QHead = 0u, NvM_QCount = 0u;

static NvM_MultiType  NvM_MultiPending = NVM_MULTI_NONE;   /* đã yêu cầu, chưa bắt đầu */
static NvM_MultiType  NvM_MultiActive  = NVM_MULTI_NONE;   /* đang chạy */
static NvM_BlockIdType NvM_MultiIter   = 0u;
static boolean        NvM_MultiFailed  = FALSE;
static boolean        NvM_MultiInBlock = FALSE;   /* đang xử lý block NvM_MultiIter */

/* Job hiện tại (một block) */
static NvM_OpType      NvM_CurOp      = NVM_OP_NONE;
static NvM_BlockIdType NvM_CurBlock   = 0u;
static const void     *NvM_CurSrc     = NULL_PTR;
static boolean         NvM_CurIssued  = FALSE;   /* đã gửi xuống MemIf, đang chờ kết quả */
static uint8           NvM_CurRetries = 0u;

static uint8 NvM_Mirror[NVM_MAX_BLOCK_LENGTH + NVM_CRC16_BYTES];   /* bản sao trung gian trong NvM */

static void NvM_Copy(uint8 *Dst, const uint8 *Src, uint16 Len)
{
    for (uint16 i = 0u; i < Len; i++) { Dst[i] = Src[i]; }
}

static boolean NvM_IsUserBlock(NvM_BlockIdType Id)
{
    return ((Id >= NVM_FIRST_USER_BLOCK) && (Id < NVM_NUM_BLOCKS)) ? TRUE : FALSE;
}

static uint16 NvM_FeeBlockNumber(NvM_BlockIdType Id)
{
    /* SWS_NvM_00122, 00123: (NvMNvBlockBaseNumber << NvMDatasetSelectionBits) + DataIndex, DataIndex = 0 */
    return (uint16)(NvM_BlockDescriptor[Id].NvBlockBaseNumber << NVM_DATASET_SELECTION_BITS);
}

static uint16 NvM_NvLength(NvM_BlockIdType Id)
{
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[Id];
    return (uint16)(d->NvBlockLength + ((d->CrcType == NVM_CRC16) ? NVM_CRC16_BYTES : 0u));
}

static boolean NvM_IsQueued(NvM_BlockIdType Id)
{
    for (uint8 i = 0u; i < NvM_QCount; i++) {
        if (NvM_Queue[(uint8)((NvM_QHead + i) % NVM_SIZE_STANDARD_JOB_QUEUE)].BlockId == Id) { return TRUE; }
    }
    return ((NvM_CurOp != NVM_OP_NONE) && (NvM_CurBlock == Id)) ? TRUE : FALSE;
}

/* ---------- API đồng bộ ---------- */

void NvM_Init(const NvM_ConfigType *ConfigPtr)
{
    (void)ConfigPtr;                                     /* SWS_NvM_00881 */
    for (uint8 i = 0u; i < NVM_NUM_BLOCKS; i++) {        /* SWS_NvM_00399 */
        NvM_Result[i]   = NVM_REQ_OK;
        NvM_RamState[i] = NVM_RAM_INVALID_UNCHANGED;     /* Figure 10: sau NvM_Init */
    }
    NvM_QHead = 0u;
    NvM_QCount = 0u;
    NvM_MultiPending = NVM_MULTI_NONE;
    NvM_MultiActive  = NVM_MULTI_NONE;
    NvM_CurOp        = NVM_OP_NONE;
    NvM_InitDone     = TRUE;                             /* "INIT DONE"; RAM block chưa bị động tới (SWS_NvM_00400) */
}

Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr)
{
    if (NvM_InitDone == FALSE)       { NVM_DET(NVM_SID_GETERRORSTATUS, NVM_E_UNINIT);        return E_NOT_OK; }
    if (RequestResultPtr == NULL_PTR){ NVM_DET(NVM_SID_GETERRORSTATUS, NVM_E_PARAM_POINTER); return E_NOT_OK; }
    if ((BlockId != 0u) && (NvM_IsUserBlock(BlockId) == FALSE)) {
        NVM_DET(NVM_SID_GETERRORSTATUS, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
    *RequestResultPtr = NvM_Result[BlockId];             /* SWS_NvM_00015; block 0 = kết quả multi block */
    return E_OK;
}

Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)
{
    if (NvM_InitDone == FALSE)            { NVM_DET(NVM_SID_SETRAMBLOCKSTATUS, NVM_E_UNINIT);         return E_NOT_OK; }
    if (NvM_IsUserBlock(BlockId) == FALSE){ NVM_DET(NVM_SID_SETRAMBLOCKSTATUS, NVM_E_PARAM_BLOCK_ID); return E_NOT_OK; }
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[BlockId];
    if ((d->RamBlockDataAddress == NULL_PTR) || (d->UseSetRamBlockStatus == FALSE)) {
        return E_OK;                                     /* SWS_NvM_00240: không có tác dụng với block khác */
    }
    /* SWS_NvM_00241, 00405, 00406; kết quả request không đổi (SWS_NvM_00910) */
    NvM_RamState[BlockId] = (BlockChanged != FALSE) ? NVM_RAM_VALID_CHANGED : NVM_RAM_INVALID_UNCHANGED;
    return E_OK;
}

/* ---------- request bất đồng bộ ---------- */

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void *NvM_SrcPtr)
{
    if (NvM_InitDone == FALSE)            { NVM_DET(NVM_SID_WRITEBLOCK, NVM_E_UNINIT);         return E_NOT_OK; }
    if (NvM_IsUserBlock(BlockId) == FALSE){ NVM_DET(NVM_SID_WRITEBLOCK, NVM_E_PARAM_BLOCK_ID); return E_NOT_OK; }
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[BlockId];
    if ((NvM_SrcPtr == NULL_PTR) && (d->RamBlockDataAddress == NULL_PTR)) {
        NVM_DET(NVM_SID_WRITEBLOCK, NVM_E_PARAM_ADDRESS);             /* SWS_NvM_00280 */
        return E_NOT_OK;
    }
    if (NvM_IsQueued(BlockId) != FALSE) {
        NVM_DET(NVM_SID_WRITEBLOCK, NVM_E_BLOCK_PENDING);
        return E_NOT_OK;
    }
    if (NvM_QCount >= NVM_SIZE_STANDARD_JOB_QUEUE) {                   /* SWS_NvM_00948 */
        (void)Det_ReportRuntimeError(DET_MODULE_ID_NVM, 0u, NVM_SID_WRITEBLOCK, NVM_E_QUEUE_FULL);
        return E_NOT_OK;
    }
    uint8 tail = (uint8)((NvM_QHead + NvM_QCount) % NVM_SIZE_STANDARD_JOB_QUEUE);
    NvM_Queue[tail].BlockId = BlockId;                                 /* SWS_NvM_00208 */
    NvM_Queue[tail].SrcPtr  = NvM_SrcPtr;
    NvM_QCount++;
    NvM_Result[BlockId] = NVM_REQ_PENDING;
    if (NvM_SrcPtr == NULL_PTR) {
        NvM_RamState[BlockId] = NVM_RAM_VALID_CHANGED;                 /* mục 7.2.2.14.2, điều kiện 2 */
    }
    return E_OK;
}

void NvM_ReadAll(void)
{
    if (NvM_InitDone == FALSE) { NVM_DET(NVM_SID_READALL, NVM_E_UNINIT); return; }
    if ((NvM_MultiPending != NVM_MULTI_NONE) || (NvM_MultiActive != NVM_MULTI_NONE)) {
        NVM_DET(NVM_SID_READALL, NVM_E_BLOCK_PENDING);
        return;
    }
    NvM_MultiPending = NVM_MULTI_READALL;                /* SWS_NvM_00243: ghi nhận rồi trả về */
    NvM_Result[0]    = NVM_REQ_PENDING;
}

void NvM_WriteAll(void)
{
    if (NvM_InitDone == FALSE) { NVM_DET(NVM_SID_WRITEALL, NVM_E_UNINIT); return; }
    if ((NvM_MultiPending != NVM_MULTI_NONE) || (NvM_MultiActive != NVM_MULTI_NONE)) {
        NVM_DET(NVM_SID_WRITEALL, NVM_E_BLOCK_PENDING);
        return;
    }
    NvM_MultiPending = NVM_MULTI_WRITEALL;               /* SWS_NvM_00254 */
    NvM_Result[0]    = NVM_REQ_PENDING;
}

/* ---------- xử lý một block ---------- */

static void NvM_RestoreDefaults(NvM_BlockIdType Id)
{
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[Id];
    NvM_Copy((uint8 *)d->RamBlockDataAddress, (const uint8 *)d->RomBlockDataAddress, d->NvBlockLength);
    NvM_RamState[Id] = NVM_RAM_VALID_CHANGED;            /* SWS_NvM_00367: hợp lệ, coi như đã thay đổi */
    NvM_Result[Id]   = NVM_REQ_RESTORED_DEFAULTS;        /* SWS_NvM_00293, 00679 */
}

static void NvM_ReadFinished(MemIf_JobResultType r)
{
    NvM_BlockIdType id = NvM_CurBlock;
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[id];
    boolean hasRom = (d->RomBlockDataAddress != NULL_PTR) ? TRUE : FALSE;
    NvM_RequestResultType noRom = NVM_REQ_NOT_OK;

    if (r == MEMIF_JOB_OK) {
        if (d->CrcType == NVM_CRC16) {
            uint16 calc   = Crc_CalculateCRC16(NvM_Mirror, d->NvBlockLength, 0u, TRUE);
            uint16 stored = (uint16)((uint16)NvM_Mirror[d->NvBlockLength] |
                                     ((uint16)NvM_Mirror[d->NvBlockLength + 1u] << 8));
            NvM_LastCrcCalc[id]   = calc;
            NvM_LastCrcStored[id] = stored;
            if (calc != stored) {                        /* SWS_NvM_00294: CRC sai */
                (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_INTEGRITY_FAILED, DEM_EVENT_STATUS_FAILED);
                (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_FAILED);
                if (hasRom != FALSE) { NvM_RestoreDefaults(id); }
                else                 { NvM_Result[id] = NVM_REQ_INTEGRITY_FAILED; }   /* SWS_NvM_00295 */
                return;
            }
        }
        NvM_Copy((uint8 *)d->RamBlockDataAddress, NvM_Mirror, d->NvBlockLength);
        NvM_RamState[id] = NVM_RAM_VALID_UNCHANGED;      /* SWS_NvM_00251 */
        NvM_Result[id]   = NVM_REQ_OK;                   /* SWS_NvM_00290 */
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_PASSED);
        return;
    }
    if (r == MEMIF_BLOCK_INVALID) {                      /* SWS_NvM_00676: không báo DEM */
        noRom = NVM_REQ_NV_INVALIDATED;                  /* SWS_NvM_00342 */
    } else if (r == MEMIF_BLOCK_INCONSISTENT) {          /* SWS_NvM_00677 */
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_INTEGRITY_FAILED, DEM_EVENT_STATUS_FAILED);
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_FAILED);
        noRom = NVM_REQ_INTEGRITY_FAILED;                /* SWS_NvM_00360 */
    } else {                                             /* MEMIF_JOB_FAILED, MEMIF_JOB_CANCELED */
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_REQ_FAILED, DEM_EVENT_STATUS_FAILED);
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_FAILED);
        noRom = NVM_REQ_NOT_OK;                          /* SWS_NvM_00361, 00678 */
    }
    if (hasRom != FALSE) { NvM_RestoreDefaults(id); }   /* SWS_NvM_00293, 00679 */
    else                 { NvM_Result[id] = noRom; }    /* RAM block giữ "invalid" (SWS_NvM_00680) */
}

static boolean NvM_IssueWrite(void)
{
    NvM_BlockIdType id = NvM_CurBlock;
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[id];
    const uint8 *src = (NvM_CurSrc != NULL_PTR) ? (const uint8 *)NvM_CurSrc           /* SWS_NvM_00210 */
                                                : (const uint8 *)d->RamBlockDataAddress; /* SWS_NvM_00900 */
    NvM_Copy(NvM_Mirror, src, d->NvBlockLength);
    if (d->CrcType == NVM_CRC16) {                       /* SWS_NvM_00212, 00253: tính CRC trước khi ghi */
        uint16 crc = Crc_CalculateCRC16(NvM_Mirror, d->NvBlockLength, 0u, TRUE);
        NvM_Mirror[d->NvBlockLength]      = (uint8)(crc & 0xFFu);
        NvM_Mirror[d->NvBlockLength + 1u] = (uint8)(crc >> 8);
    }
    return (MemIf_Write(d->NvramDeviceId, NvM_FeeBlockNumber(id), NvM_Mirror) == E_OK) ? TRUE : FALSE;
}

static void NvM_WriteFinished(MemIf_JobResultType r)
{
    NvM_BlockIdType id = NvM_CurBlock;
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[id];

    if (r == MEMIF_JOB_OK) {
        if (NvM_CurSrc == NULL_PTR) {
            NvM_RamState[id] = NVM_RAM_VALID_UNCHANGED;  /* SWS_NvM_00472 */
        }
        NvM_Result[id] = NVM_REQ_OK;
        (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_PASSED);
        NvM_CurOp = NVM_OP_NONE;
        return;
    }
    if (NvM_CurRetries < d->MaxNumOfWriteRetries) {      /* SWS_NvM_00213, 00296: đếm số lần thử lại */
        NvM_CurRetries++;
        NvM_WriteRetryCount++;
        NvM_CurIssued = FALSE;                           /* gửi lại ở lần gọi MainFunction sau */
        return;
    }
    NvM_Result[id] = NVM_REQ_NOT_OK;                     /* SWS_NvM_00213, 00683 */
    (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_REQ_FAILED, DEM_EVENT_STATUS_FAILED);
    (void)Dem_SetEventStatus(DemConf_DemEventParameter_NVM_E_HARDWARE, DEM_EVENT_STATUS_FAILED);
    if (NvM_CurSrc == NULL_PTR) {
        NvM_RamState[id] = NVM_RAM_INVALID_UNCHANGED;    /* mục 7.2.2.14.3, điều kiện 5, 6 */
    }
    NvM_CurOp = NVM_OP_NONE;
}

/* Gửi job hiện tại xuống MemIf hoặc chờ kết quả. Trả TRUE khi job đã xong. */
static boolean NvM_ProcessCurrent(void)
{
    const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[NvM_CurBlock];

    if (NvM_CurIssued == FALSE) {
        MemIf_StatusType st = MemIf_GetStatus(d->NvramDeviceId);
        if ((st == MEMIF_UNINIT) || (st == MEMIF_BUSY)) {
            return FALSE;                                /* tầng dưới chưa sẵn sàng: chờ chu kỳ sau */
        }
        Std_ReturnType rq;
        if (NvM_CurOp == NVM_OP_READ) {
            rq = MemIf_Read(d->NvramDeviceId, NvM_FeeBlockNumber(NvM_CurBlock), 0u,
                            NvM_Mirror, NvM_NvLength(NvM_CurBlock));
        } else {
            rq = (NvM_IssueWrite() != FALSE) ? E_OK : E_NOT_OK;
        }
        if (rq != E_OK) {                                /* bị từ chối: coi như job thất bại */
            if (NvM_CurOp == NVM_OP_READ) { NvM_ReadFinished(MEMIF_JOB_FAILED); NvM_CurOp = NVM_OP_NONE; }
            else                          { NvM_WriteFinished(MEMIF_JOB_FAILED); }
            return (NvM_CurOp == NVM_OP_NONE) ? TRUE : FALSE;
        }
        NvM_CurIssued = TRUE;
        return FALSE;
    }
    MemIf_JobResultType r = MemIf_GetJobResult(d->NvramDeviceId);   /* NvMPollingMode */
    if (r == MEMIF_JOB_PENDING) {
        return FALSE;
    }
    if (NvM_CurOp == NVM_OP_READ) {
        NvM_ReadFinished(r);
        NvM_CurOp = NVM_OP_NONE;
    } else {
        NvM_WriteFinished(r);
    }
    return (NvM_CurOp == NVM_OP_NONE) ? TRUE : FALSE;
}

static void NvM_StartBlockJob(NvM_OpType Op, NvM_BlockIdType Id, const void *Src)
{
    NvM_CurOp      = Op;
    NvM_CurBlock   = Id;
    NvM_CurSrc     = Src;
    NvM_CurIssued  = FALSE;
    NvM_CurRetries = 0u;
}

/* ---------- multi block ---------- */

static void NvM_MultiStart(void)
{
    NvM_MultiActive  = NvM_MultiPending;
    NvM_MultiPending = NVM_MULTI_NONE;
    NvM_MultiIter    = NVM_FIRST_USER_BLOCK;             /* SWS_NvM_00244: bỏ qua block 0 và 1 */
    NvM_MultiFailed  = FALSE;
    NvM_MultiInBlock = FALSE;
    for (NvM_BlockIdType id = NVM_FIRST_USER_BLOCK; id < NVM_NUM_BLOCKS; id++) {
        const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[id];
        boolean sel = (NvM_MultiActive == NVM_MULTI_READALL) ? d->SelectBlockForReadAll
                                                              : d->SelectBlockForWriteAll;
        if ((sel != FALSE) && (d->RamBlockDataAddress != NULL_PTR)) {
            NvM_Result[id] = NVM_REQ_PENDING;            /* SWS_NvM_00304, 00549 */
        }
    }
}

static void NvM_MultiStep(void)
{
    if (NvM_MultiInBlock != FALSE) {
        if ((NvM_CurOp != NVM_OP_NONE) && (NvM_ProcessCurrent() == FALSE)) { return; }
        if (NvM_Result[NvM_MultiIter] != NVM_REQ_OK) { NvM_MultiFailed = TRUE; }   /* ghi chú dưới 00895 */
        NvM_MultiInBlock = FALSE;
        NvM_MultiIter++;
    }
    while (NvM_MultiIter < NVM_NUM_BLOCKS) {
        NvM_BlockIdType id = NvM_MultiIter;
        const NvM_BlockDescriptorType *d = &NvM_BlockDescriptor[id];

        if (NvM_MultiActive == NVM_MULTI_READALL) {
            /* SWS_NvM_00118, 00245: chỉ block có permanent RAM, được chọn, không phải DATASET */
            if ((d->SelectBlockForReadAll != FALSE) && (d->RamBlockDataAddress != NULL_PTR) &&
                (d->ManagementType != NVM_BLOCK_DATASET)) {
                NvM_StartBlockJob(NVM_OP_READ, id, NULL_PTR);
                NvM_MultiInBlock = TRUE;
                (void)NvM_ProcessCurrent();
                return;
            }
        } else {
            if ((d->SelectBlockForWriteAll != FALSE) && (d->RamBlockDataAddress != NULL_PTR)) {
                if (NvM_RamState[id] == NVM_RAM_VALID_CHANGED) {   /* SWS_NvM_00252, 00682 */
                    NvM_StartBlockJob(NVM_OP_WRITE, id, NULL_PTR);
                    NvM_MultiInBlock = TRUE;
                    (void)NvM_ProcessCurrent();
                    return;
                }
                NvM_Result[id]  = NVM_REQ_BLOCK_SKIPPED;           /* SWS_NvM_00298 */
                NvM_MultiFailed = TRUE;                            /* khác NVM_REQ_OK (ghi chú dưới 00318) */
            }
        }
        NvM_MultiIter++;
    }
    /* SWS_NvM_00895/00301 (ReadAll), 00896/00318 (WriteAll) */
    NvM_Result[0]   = (NvM_MultiFailed != FALSE) ? NVM_REQ_NOT_OK : NVM_REQ_OK;
    NvM_MultiActive = NVM_MULTI_NONE;
}

void NvM_MainFunction(void)
{
    if (NvM_InitDone == FALSE) {
        return;                                          /* SWS_NvM_00257 */
    }
    if (NvM_MultiActive != NVM_MULTI_NONE) {
        NvM_MultiStep();
        return;
    }
    if (NvM_CurOp != NVM_OP_NONE) {
        (void)NvM_ProcessCurrent();
        return;
    }
    if (NvM_QCount > 0u) {                               /* hàng đợi FIFO (NvMJobPrioritization = FALSE) */
        NvM_QueueEntryType e = NvM_Queue[NvM_QHead];
        NvM_QHead = (uint8)((NvM_QHead + 1u) % NVM_SIZE_STANDARD_JOB_QUEUE);
        NvM_QCount--;
        NvM_StartBlockJob(NVM_OP_WRITE, e.BlockId, e.SrcPtr);
        (void)NvM_ProcessCurrent();
        return;
    }
    if (NvM_MultiPending != NVM_MULTI_NONE) {            /* hàng đợi đã trống */
        NvM_MultiStart();
        NvM_MultiStep();
    }
}
