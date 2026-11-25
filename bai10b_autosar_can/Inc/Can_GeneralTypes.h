/* Can_GeneralTypes.h - kiểu dữ liệu dùng chung giữa Can Driver và CanIf (AUTOSAR CP SWS CAN Driver R25-11). */
#ifndef CAN_GENERALTYPES_H
#define CAN_GENERALTYPES_H

#include "ComStack_Types.h"

typedef uint32 Can_IdType;                 /* SWS_Can_00416: bit 31 = IDE, bit 30 = CAN FD */
typedef uint16 Can_HwHandleType;           /* SWS_Can_00429: HRH/HTH */

typedef struct {                           /* SWS_Can_00415 */
    PduIdType  swPduHandle;                /* ID do CanIf cấp, trả lại trong CanIf_TxConfirmation */
    uint8      length;                     /* DLC */
    Can_IdType id;
    uint8     *sdu;
} Can_PduType;

typedef struct {                           /* SWS_CAN_00496 */
    Can_IdType       CanId;
    Can_HwHandleType Hoh;
    uint8            ControllerId;
} Can_HwType;

typedef enum {                             /* Can_ControllerStateType */
    CAN_CS_UNINIT  = 0x00u,
    CAN_CS_STARTED = 0x01u,
    CAN_CS_STOPPED = 0x02u,
    CAN_CS_SLEEP   = 0x03u
} Can_ControllerStateType;

#define CAN_BUSY  ((Std_ReturnType)0x02u)  /* SWS_Can_00039: không còn hardware object trống */

#endif /* CAN_GENERALTYPES_H */
