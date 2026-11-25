/* ComStack_Types.h - kiểu dữ liệu dùng chung của communication stack
 * (rút gọn từ AUTOSAR CP SWS Communication Stack Types). */
#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

typedef uint16 PduIdType;        /* ID của PDU, mỗi module có bảng ID riêng */
typedef uint16 PduLengthType;    /* độ dài PDU tính bằng byte */

typedef struct {
    uint8        *SduDataPtr;    /* con trỏ tới dữ liệu SDU */
    uint8        *MetaDataPtr;   /* metadata (ví dụ CAN ID), NULL_PTR nếu không cấu hình */
    PduLengthType SduLength;     /* độ dài SDU */
} PduInfoType;

#endif /* COMSTACK_TYPES_H */
