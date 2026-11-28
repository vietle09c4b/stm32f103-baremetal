/* Fls.h - Flash Driver (MCAL, nhóm Memory Drivers). AUTOSAR CP SWS FlashDriver R20-11.
 * Service ID theo chương 8.3: Init 0x00, Erase 0x01, Write 0x02, Cancel 0x03, GetStatus 0x04,
 * GetJobResult 0x05, MainFunction 0x06, Read 0x07.
 * Không hiện thực: Fls_Compare (0x08), Fls_SetMode (0x09), Fls_BlankCheck, Fls_GetVersionInfo. */
#ifndef FLS_H
#define FLS_H

#include "Std_Types.h"
#include "MemIf.h"
#include "Fls_Cfg.h"

typedef uint32 Fls_AddressType;    /* SWS_Fls_00369: offset tính từ FlsBaseAddress */
typedef uint32 Fls_LengthType;     /* SWS_Fls_00370 */

typedef struct {
    Fls_AddressType BaseAddress;   /* FlsBaseAddress: địa chỉ vật lý của offset 0 */
    Fls_LengthType  TotalSize;     /* FlsTotalSize */
    Fls_LengthType  SectorSize;    /* FlsSectorSize: đơn vị xóa nhỏ nhất */
    Fls_LengthType  PageSize;      /* FlsPageSize: đơn vị ghi nhỏ nhất */
    Fls_LengthType  MaxReadNormalMode;   /* số byte đọc tối đa mỗi lần gọi Fls_MainFunction */
    Fls_LengthType  MaxWriteNormalMode;  /* số byte ghi tối đa mỗi lần gọi Fls_MainFunction */
} Fls_ConfigType;                  /* SWS_Fls_00368: cấu trúc phụ thuộc phần cứng */

/* Mã lỗi development (SWS_Fls_00004), runtime (SWS_Fls_91001), transient fault (SWS_Fls_91002) */
#define FLS_E_PARAM_CONFIG          0x01u
#define FLS_E_PARAM_ADDRESS         0x02u
#define FLS_E_PARAM_LENGTH          0x03u
#define FLS_E_PARAM_DATA            0x04u
#define FLS_E_UNINIT                0x05u
#define FLS_E_BUSY                  0x06u
#define FLS_E_VERIFY_ERASE_FAILED   0x07u
#define FLS_E_VERIFY_WRITE_FAILED   0x08u
#define FLS_E_TIMEOUT               0x09u
#define FLS_E_PARAM_POINTER         0x0Au
#define FLS_E_ERASE_FAILED          0x01u   /* transient fault */
#define FLS_E_WRITE_FAILED          0x02u   /* transient fault */

#define FLS_SID_INIT          0x00u
#define FLS_SID_ERASE         0x01u
#define FLS_SID_WRITE         0x02u
#define FLS_SID_CANCEL        0x03u
#define FLS_SID_GETSTATUS     0x04u
#define FLS_SID_GETJOBRESULT  0x05u
#define FLS_SID_MAINFUNCTION  0x06u
#define FLS_SID_READ          0x07u

void                Fls_Init(const Fls_ConfigType *ConfigPtr);
Std_ReturnType      Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length);
Std_ReturnType      Fls_Write(Fls_AddressType TargetAddress, const uint8 *SourceAddressPtr, Fls_LengthType Length);
Std_ReturnType      Fls_Read(Fls_AddressType SourceAddress, uint8 *TargetAddressPtr, Fls_LengthType Length);
void                Fls_Cancel(void);
MemIf_StatusType    Fls_GetStatus(void);
MemIf_JobResultType Fls_GetJobResult(void);
void                Fls_MainFunction(void);   /* SchM_Fls.h trong hệ thống thật */

extern const Fls_ConfigType Fls_Config;

/* Biến quan sát khi debug */
extern volatile uint32 Fls_EraseCount;         /* số sector đã xóa */
extern volatile uint32 Fls_WriteHalfCount;     /* số half-word đã ghi */
extern volatile uint32 Fls_LastEraseCycles;    /* thời gian BSY = 1 của lần xóa sector gần nhất (chu kỳ DWT) */

#endif /* FLS_H */
