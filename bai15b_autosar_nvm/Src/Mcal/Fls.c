/* Fls.c - Flash Driver (MCAL) cho Flash nội STM32F103C8. AUTOSAR CP SWS FlashDriver R20-11.
 * Erase, Write, Read chỉ nhận job rồi trả về (SWS_Fls_00327..00330, 00331..00334, 00337..00340);
 * việc thật chạy trong Fls_MainFunction (SWS_Fls_00220, 00225, 00238).
 * Fls không đệm dữ liệu, dùng thẳng buffer của tầng trên (SWS_Fls_00002).
 * Truy cập thanh ghi FPEC nằm trong Fls_ac.c (SWS_Fls_00137). */
#include "Fls.h"
#include "Fls_Ac.h"
#include "Det.h"

typedef enum { FLS_JOB_NONE = 0, FLS_JOB_ERASE, FLS_JOB_WRITE, FLS_JOB_READ } Fls_JobType;

static const Fls_ConfigType *Fls_CfgPtr   = NULL_PTR;           /* SWS_Fls_00191 */
static MemIf_StatusType      Fls_Status   = MEMIF_UNINIT;
static MemIf_JobResultType   Fls_JobRes   = MEMIF_JOB_OK;
static Fls_JobType           Fls_Job      = FLS_JOB_NONE;
static Fls_AddressType       Fls_JobAddr  = 0u;                 /* offset hiện tại */
static Fls_LengthType        Fls_JobLen   = 0u;                 /* số byte còn lại */
static const uint8          *Fls_JobSrc   = NULL_PTR;
static uint8                *Fls_JobDst   = NULL_PTR;

volatile uint32 Fls_EraseCount      = 0u;
volatile uint32 Fls_WriteHalfCount  = 0u;
volatile uint32 Fls_LastEraseCycles = 0u;

#define FLS_DET(api, err)  (void)Det_ReportError(DET_MODULE_ID_FLS, 0u, (api), (err))

static uint32 Fls_Phys(Fls_AddressType Offset)
{
    return Fls_CfgPtr->BaseAddress + Offset;                    /* SWS_Fls_00217: cộng địa chỉ gốc */
}

static void Fls_FinishJob(MemIf_JobResultType Result)
{
    if (Fls_JobRes == MEMIF_JOB_PENDING) {
        Fls_JobRes = Result;                                    /* SWS_Fls_00345 */
    }
    Fls_Job    = FLS_JOB_NONE;
    Fls_Status = MEMIF_IDLE;                                    /* SWS_Fls_00346 */
}

void Fls_Init(const Fls_ConfigType *ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        FLS_DET(FLS_SID_INIT, FLS_E_PARAM_POINTER);
        return;
    }
    if (Fls_Status == MEMIF_BUSY) {                             /* SWS_Fls_00268 */
        FLS_DET(FLS_SID_INIT, FLS_E_BUSY);
        return;
    }
    if ((ConfigPtr->SectorSize == 0u) || (ConfigPtr->PageSize == 0u) ||
        ((ConfigPtr->TotalSize % ConfigPtr->SectorSize) != 0u)) {   /* SWS_Fls_00015 */
        FLS_DET(FLS_SID_INIT, FLS_E_PARAM_CONFIG);
        return;
    }
    Fls_CfgPtr = ConfigPtr;
    Fls_Job    = FLS_JOB_NONE;
    Fls_Ac_Lock();                                              /* FPEC chỉ mở trong lúc thực hiện job */
    Fls_Status = MEMIF_IDLE;                                    /* SWS_Fls_00323 */
    Fls_JobRes = MEMIF_JOB_OK;                                  /* SWS_Fls_00324 */
}

/* Kiểm tra chung cho Erase/Write/Read: đã khởi tạo, không bận (SWS_Fls_00065/00023, 00066/00030, 00099/00100) */
static boolean Fls_CheckState(uint8 Api)
{
    if (Fls_Status == MEMIF_UNINIT) {
        FLS_DET(Api, FLS_E_UNINIT);
        return FALSE;
    }
    if (Fls_Status == MEMIF_BUSY) {
        FLS_DET(Api, FLS_E_BUSY);
        return FALSE;
    }
    return TRUE;
}

Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    if (Fls_CheckState(FLS_SID_ERASE) == FALSE) {
        return E_NOT_OK;
    }
    /* SWS_Fls_00020: địa chỉ đầu phải trùng biên sector và nằm trong vùng Flash */
    if ((TargetAddress >= Fls_CfgPtr->TotalSize) || ((TargetAddress % Fls_CfgPtr->SectorSize) != 0u)) {
        FLS_DET(FLS_SID_ERASE, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    /* SWS_Fls_00021: Length > 0, địa chỉ cuối trùng biên sector và không vượt vùng Flash */
    if ((Length == 0u) || (Length > (Fls_CfgPtr->TotalSize - TargetAddress)) ||
        (((TargetAddress + Length) % Fls_CfgPtr->SectorSize) != 0u)) {
        FLS_DET(FLS_SID_ERASE, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    Fls_JobAddr = TargetAddress;                                /* SWS_Fls_00327 */
    Fls_JobLen  = Length;
    Fls_Job     = FLS_JOB_ERASE;
    Fls_Status  = MEMIF_BUSY;                                   /* SWS_Fls_00328 */
    Fls_JobRes  = MEMIF_JOB_PENDING;                            /* SWS_Fls_00329 */
    return E_OK;                                                /* SWS_Fls_00330 */
}

Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8 *SourceAddressPtr, Fls_LengthType Length)
{
    if (Fls_CheckState(FLS_SID_WRITE) == FALSE) {
        return E_NOT_OK;
    }
    /* SWS_Fls_00026: địa chỉ đầu trùng biên page (2 byte) và nằm trong vùng Flash */
    if ((TargetAddress >= Fls_CfgPtr->TotalSize) || ((TargetAddress % Fls_CfgPtr->PageSize) != 0u)) {
        FLS_DET(FLS_SID_WRITE, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    /* SWS_Fls_00027: Length > 0, địa chỉ cuối trùng biên page, không vượt vùng Flash */
    if ((Length == 0u) || (Length > (Fls_CfgPtr->TotalSize - TargetAddress)) ||
        (((TargetAddress + Length) % Fls_CfgPtr->PageSize) != 0u)) {
        FLS_DET(FLS_SID_WRITE, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (SourceAddressPtr == NULL_PTR) {                         /* SWS_Fls_00157 */
        FLS_DET(FLS_SID_WRITE, FLS_E_PARAM_DATA);
        return E_NOT_OK;
    }
    Fls_JobAddr = TargetAddress;                                /* SWS_Fls_00331 */
    Fls_JobLen  = Length;
    Fls_JobSrc  = SourceAddressPtr;
    Fls_Job     = FLS_JOB_WRITE;
    Fls_Status  = MEMIF_BUSY;                                   /* SWS_Fls_00332 */
    Fls_JobRes  = MEMIF_JOB_PENDING;                            /* SWS_Fls_00333 */
    return E_OK;                                                /* SWS_Fls_00334 */
}

Std_ReturnType Fls_Read(Fls_AddressType SourceAddress, uint8 *TargetAddressPtr, Fls_LengthType Length)
{
    if (Fls_CheckState(FLS_SID_READ) == FALSE) {
        return E_NOT_OK;
    }
    if (SourceAddress >= Fls_CfgPtr->TotalSize) {               /* SWS_Fls_00097 */
        FLS_DET(FLS_SID_READ, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    if ((Length == 0u) || (Length > (Fls_CfgPtr->TotalSize - SourceAddress))) {   /* SWS_Fls_00098 */
        FLS_DET(FLS_SID_READ, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (TargetAddressPtr == NULL_PTR) {                         /* SWS_Fls_00158 */
        FLS_DET(FLS_SID_READ, FLS_E_PARAM_DATA);
        return E_NOT_OK;
    }
    Fls_JobAddr = SourceAddress;                                /* SWS_Fls_00337 */
    Fls_JobLen  = Length;
    Fls_JobDst  = TargetAddressPtr;
    Fls_Job     = FLS_JOB_READ;
    Fls_Status  = MEMIF_BUSY;                                   /* SWS_Fls_00338 */
    Fls_JobRes  = MEMIF_JOB_PENDING;                            /* SWS_Fls_00339 */
    return E_OK;                                                /* SWS_Fls_00340 */
}

void Fls_Cancel(void)
{
    if (Fls_Status == MEMIF_UNINIT) {                           /* SWS_Fls_00356 */
        FLS_DET(FLS_SID_CANCEL, FLS_E_UNINIT);
        return;
    }
    Fls_Job     = FLS_JOB_NONE;                                 /* SWS_Fls_00335 */
    Fls_JobAddr = 0u;
    Fls_JobLen  = 0u;
    Fls_JobSrc  = NULL_PTR;
    Fls_JobDst  = NULL_PTR;
    Fls_Status  = MEMIF_IDLE;                                   /* SWS_Fls_00336 */
    if (Fls_JobRes == MEMIF_JOB_PENDING) {
        Fls_JobRes = MEMIF_JOB_CANCELED;                        /* SWS_Fls_00033 */
    }
}

MemIf_StatusType Fls_GetStatus(void)
{
    return Fls_Status;                                          /* SWS_Fls_00034; MEMIF_UNINIT trước Fls_Init */
}

MemIf_JobResultType Fls_GetJobResult(void)
{
    if (Fls_Status == MEMIF_UNINIT) {                           /* SWS_Fls_00358 */
        FLS_DET(FLS_SID_GETJOBRESULT, FLS_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
    return Fls_JobRes;                                          /* SWS_Fls_00035, 00036 */
}

static void Fls_ProcessErase(void)
{
    uint32 phys = Fls_Phys(Fls_JobAddr);
    uint32 t0   = Fls_Ac_GetCycles();

    Fls_Ac_Unlock();
    uint32 err = Fls_Ac_ErasePage(phys);                        /* SWS_Fls_00196: một sector mỗi chu kỳ */
    Fls_Ac_Lock();
    Fls_LastEraseCycles = Fls_Ac_GetCycles() - t0;
    Fls_EraseCount++;

    if (err != 0u) {                                            /* SWS_Fls_00104 */
        (void)Det_ReportTransientFault(DET_MODULE_ID_FLS, 0u, FLS_SID_MAINFUNCTION, FLS_E_ERASE_FAILED);
        Fls_FinishJob(MEMIF_JOB_FAILED);
        return;
    }
#if (FLS_ERASE_VERIFICATION == STD_ON)
    for (uint32 i = 0u; i < Fls_CfgPtr->SectorSize; i++) {      /* SWS_Fls_00022 */
        if (Fls_Ac_ReadByte(phys + i) != FLS_ERASED_VALUE) {
            (void)Det_ReportRuntimeError(DET_MODULE_ID_FLS, 0u, FLS_SID_MAINFUNCTION, FLS_E_VERIFY_ERASE_FAILED);
            Fls_FinishJob(MEMIF_JOB_FAILED);
            return;
        }
    }
#endif
    Fls_JobAddr += Fls_CfgPtr->SectorSize;
    Fls_JobLen  -= Fls_CfgPtr->SectorSize;
    if (Fls_JobLen == 0u) {
        Fls_FinishJob(MEMIF_JOB_OK);
    }
}

static void Fls_ProcessWrite(void)
{
    Fls_LengthType chunk = (Fls_JobLen < Fls_CfgPtr->MaxWriteNormalMode) ? Fls_JobLen
                                                                        : Fls_CfgPtr->MaxWriteNormalMode;
    uint32 phys = Fls_Phys(Fls_JobAddr);
    MemIf_JobResultType res = MEMIF_JOB_PENDING;

#if (FLS_ERASE_VERIFICATION == STD_ON)
    for (uint32 i = 0u; i < chunk; i++) {                       /* SWS_Fls_00055: ô phải còn trống */
        if (Fls_Ac_ReadByte(phys + i) != FLS_ERASED_VALUE) {
            (void)Det_ReportRuntimeError(DET_MODULE_ID_FLS, 0u, FLS_SID_MAINFUNCTION, FLS_E_VERIFY_ERASE_FAILED);
            Fls_FinishJob(MEMIF_JOB_FAILED);
            return;
        }
    }
#endif
    Fls_Ac_Unlock();
    for (uint32 i = 0u; i < chunk; i += 2u) {
        /* SWS_Fls_00389: buffer chỉ được coi là căn theo byte; ghép half-word little-endian */
        uint16 half = (uint16)((uint16)Fls_JobSrc[i] | ((uint16)Fls_JobSrc[i + 1u] << 8));
        if (Fls_Ac_ProgramHalf(phys + i, half) != 0u) {
            (void)Det_ReportTransientFault(DET_MODULE_ID_FLS, 0u, FLS_SID_MAINFUNCTION, FLS_E_WRITE_FAILED);
            res = MEMIF_JOB_FAILED;                             /* SWS_Fls_00105 */
            break;
        }
        Fls_WriteHalfCount++;
    }
    Fls_Ac_Lock();
    if (res == MEMIF_JOB_FAILED) {
        Fls_FinishJob(MEMIF_JOB_FAILED);
        return;
    }
#if (FLS_WRITE_VERIFICATION == STD_ON)
    for (uint32 i = 0u; i < chunk; i++) {                       /* SWS_Fls_00056 */
        if (Fls_Ac_ReadByte(phys + i) != Fls_JobSrc[i]) {
            (void)Det_ReportRuntimeError(DET_MODULE_ID_FLS, 0u, FLS_SID_MAINFUNCTION, FLS_E_VERIFY_WRITE_FAILED);
            Fls_FinishJob(MEMIF_JOB_FAILED);
            return;
        }
    }
#endif
    Fls_JobAddr += chunk;
    Fls_JobSrc  += chunk;
    Fls_JobLen  -= chunk;
    if (Fls_JobLen == 0u) {
        Fls_FinishJob(MEMIF_JOB_OK);
    }
}

static void Fls_ProcessRead(void)
{
    Fls_LengthType chunk = (Fls_JobLen < Fls_CfgPtr->MaxReadNormalMode) ? Fls_JobLen
                                                                       : Fls_CfgPtr->MaxReadNormalMode;
    uint32 phys = Fls_Phys(Fls_JobAddr);
    for (uint32 i = 0u; i < chunk; i++) {                       /* SWS_Fls_00239: đọc byte, không cần căn lề */
        Fls_JobDst[i] = Fls_Ac_ReadByte(phys + i);
    }
    Fls_JobAddr += chunk;
    Fls_JobDst  += chunk;
    Fls_JobLen  -= chunk;
    if (Fls_JobLen == 0u) {
        Fls_FinishJob(MEMIF_JOB_OK);
    }
}

void Fls_MainFunction(void)
{
    if ((Fls_Status != MEMIF_BUSY) || (Fls_Job == FLS_JOB_NONE)) {
        return;                                                 /* SWS_Fls_00039 */
    }
    switch (Fls_Job) {                                          /* SWS_Fls_00037, 00040 */
    case FLS_JOB_ERASE: Fls_ProcessErase(); break;
    case FLS_JOB_WRITE: Fls_ProcessWrite(); break;
    case FLS_JOB_READ:  Fls_ProcessRead();  break;
    default:            break;
    }
}
