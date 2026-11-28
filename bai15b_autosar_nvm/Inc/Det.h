/* Det.h - Default Error Tracer rút gọn: chỉ ghi lại lỗi gần nhất và số lỗi để đọc bằng debugger.
 * Không phải hiện thực đầy đủ SWS DET. Module ID theo AUTOSAR List of Basic Software Modules. */
#ifndef DET_H
#define DET_H

#include "Std_Types.h"

#define DET_MODULE_ID_NVM   20u
#define DET_MODULE_ID_FEE   21u
#define DET_MODULE_ID_MEMIF 22u
#define DET_MODULE_ID_FLS   92u

typedef struct {
    uint16 ModuleId;
    uint8  InstanceId;
    uint8  ApiId;
    uint8  ErrorId;
} Det_ErrorType;

extern volatile Det_ErrorType Det_LastError;
extern volatile uint32        Det_ErrorCount;
extern volatile Det_ErrorType Det_LastRuntimeError;
extern volatile uint32        Det_RuntimeErrorCount;
extern volatile Det_ErrorType Det_LastTransientFault;
extern volatile uint32        Det_TransientFaultCount;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId);

#endif /* DET_H */
