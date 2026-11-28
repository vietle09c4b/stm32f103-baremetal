/* MemIf.h - Memory Abstraction Interface (ECU Abstraction Layer, nhóm Memory Hardware Abstraction).
 * AUTOSAR CP SWS MemoryAbstractionInterface R20-11.
 * Kiểu dữ liệu: SWS_MemIf_00064 (MemIf_StatusType), 00065 (MemIf_JobResultType), 00066 (MemIf_ModeType).
 * SWS chỉ đặt tên các giá trị liệt kê, không quy định trị số; trị số dưới đây do hiện thực tự chọn.
 * Chỉ có một module bên dưới (Fee) nên API là tập macro ánh xạ thẳng sang Fee (SWS_MemIf_00019),
 * tham số DeviceIndex bị bỏ qua (SWS_MemIf_00018). */
#ifndef MEMIF_H
#define MEMIF_H

#include "Std_Types.h"

typedef enum {
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY,
    MEMIF_BUSY_INTERNAL
} MemIf_StatusType;

typedef enum {
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_FAILED,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED,
    MEMIF_BLOCK_INCONSISTENT,
    MEMIF_BLOCK_INVALID
} MemIf_JobResultType;

typedef enum {
    MEMIF_MODE_SLOW = 0,
    MEMIF_MODE_FAST
} MemIf_ModeType;

#define MEMIF_NUMBER_OF_DEVICES  1u
#define MEMIF_BROADCAST_ID       0xFFu

#include "Fee.h"   /* sau phần kiểu dữ liệu, vì Fee.h dùng các kiểu ở trên */

/* MemIf_SetMode không được ánh xạ: Fee của ví dụ không hỗ trợ đổi chế độ (FeeSetModeSupported = FALSE) */
#define MemIf_Read(DeviceIndex, BlockNumber, BlockOffset, DataBufferPtr, Length) \
    ((void)(DeviceIndex), Fee_Read((BlockNumber), (BlockOffset), (DataBufferPtr), (Length)))
#define MemIf_Write(DeviceIndex, BlockNumber, DataBufferPtr) \
    ((void)(DeviceIndex), Fee_Write((BlockNumber), (DataBufferPtr)))
#define MemIf_Cancel(DeviceIndex)                   ((void)(DeviceIndex), Fee_Cancel())
#define MemIf_GetStatus(DeviceIndex)                ((void)(DeviceIndex), Fee_GetStatus())
#define MemIf_GetJobResult(DeviceIndex)             ((void)(DeviceIndex), Fee_GetJobResult())
#define MemIf_InvalidateBlock(DeviceIndex, BlockNumber) \
    ((void)(DeviceIndex), Fee_InvalidateBlock(BlockNumber))
#define MemIf_EraseImmediateBlock(DeviceIndex, BlockNumber) \
    ((void)(DeviceIndex), Fee_EraseImmediateBlock(BlockNumber))

#endif /* MEMIF_H */
