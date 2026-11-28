/* Det.c - Default Error Tracer rút gọn (System Services). Chỉ lưu lỗi gần nhất và bộ đếm. */
#include "Det.h"

volatile Det_ErrorType Det_LastError;
volatile uint32        Det_ErrorCount = 0u;
volatile Det_ErrorType Det_LastRuntimeError;
volatile uint32        Det_RuntimeErrorCount = 0u;
volatile Det_ErrorType Det_LastTransientFault;
volatile uint32        Det_TransientFaultCount = 0u;

static void Det_Store(volatile Det_ErrorType *Dst, uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Dst->ModuleId   = ModuleId;
    Dst->InstanceId = InstanceId;
    Dst->ApiId      = ApiId;
    Dst->ErrorId    = ErrorId;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_Store(&Det_LastError, ModuleId, InstanceId, ApiId, ErrorId);
    Det_ErrorCount++;
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_Store(&Det_LastRuntimeError, ModuleId, InstanceId, ApiId, ErrorId);
    Det_RuntimeErrorCount++;
    return E_OK;
}

Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId)
{
    Det_Store(&Det_LastTransientFault, ModuleId, InstanceId, ApiId, FaultId);
    Det_TransientFaultCount++;
    return E_OK;
}
