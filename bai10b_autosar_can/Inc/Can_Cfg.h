/* Can_Cfg.h - cấu hình Can Driver (trong dự án thật do công cụ cấu hình sinh ra từ ECUC).
 * ID của HRH và HTH dùng chung một dải, bắt đầu từ 0, liên tục (ECUC_Can_00326). */
#ifndef CAN_CFG_H
#define CAN_CFG_H

#include "Can_GeneralTypes.h"

#define CanConf_CanController_0          0u
#define CanConf_CanHardwareObject_Rx0    ((Can_HwHandleType)0u)   /* HRH: FIFO0 + filter bank 0 */
#define CanConf_CanHardwareObject_Tx0    ((Can_HwHandleType)1u)   /* HTH: 3 transmit mailbox */

/* PCLK1 = 8 MHz (HSE): BRP = 0, TS1 = 12, TS2 = 1, SJW = 0 -> 16 tq, 500 kbit/s, sample point 87,5 % */
#define CAN_BTR_500K_PCLK8M              0x001C0000UL

typedef struct {
    uint32 Btr;                 /* giá trị CAN_BTR */
    uint16 RxStdId[2];          /* bank 0, identifier list 32 bit: 2 standard ID vào FIFO0 */
    uint8  RxIrqPriority;       /* mức ưu tiên NVIC của USB_LP_CAN1_RX0 */
} Can_ConfigType;

extern const Can_ConfigType Can_Config;

#endif /* CAN_CFG_H */
