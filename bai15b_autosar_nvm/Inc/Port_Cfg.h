/* Port_Cfg.h - kiểu dữ liệu và symbolic name của Port Driver.
 * Port_PinType: uint8 (SWS_Port_00219); mã hóa ID = chỉ số port x 16 + số chân (A = 0, B = 1, C = 2). */
#ifndef PORT_CFG_H
#define PORT_CFG_H

#include "Std_Types.h"

typedef uint8 Port_PinType;
typedef uint8 Port_PinModeType;

typedef enum {
    PORT_PIN_IN  = 0x00u,     /* SWS_Port_00230 */
    PORT_PIN_OUT = 0x01u
} Port_PinDirectionType;

/* Điện trở kéo nội: cấu hình tùy chọn khi phần cứng hỗ trợ (SWS_Port_00081).
 * Kiểu này không có tên chuẩn trong SWS; tên do hiện thực tự đặt. */
typedef enum {
    PORT_PULL_NONE = 0x00u,
    PORT_PULL_UP   = 0x01u,
    PORT_PULL_DOWN = 0x02u
} Port_PinPullType;

/* Chế độ STM32F103: giá trị 4 bit CNF[1:0]:MODE[1:0] ghi vào CRL/CRH (RM0008 Table 20, 9.2.1, 9.2.2) */
#define PORT_MODE_OUT_PP_2MHZ     ((Port_PinModeType)0x2u)   /* CNF = 00, MODE = 10 */
#define PORT_MODE_IN_FLOATING     ((Port_PinModeType)0x4u)   /* CNF = 01, MODE = 00 */
#define PORT_MODE_IN_PULL         ((Port_PinModeType)0x8u)   /* CNF = 10, MODE = 00; ODR chọn up/down */

typedef struct {
    Port_PinType          Pin;
    Port_PinDirectionType Direction;
    Port_PinModeType      Mode;
    uint8                 InitialLevel;   /* STD_HIGH / STD_LOW, chỉ dùng với output */
    Port_PinPullType      Pull;           /* chỉ dùng với input PORT_MODE_IN_PULL */
} Port_PinConfigType;

typedef struct {
    const Port_PinConfigType *Pins;
    uint8                     NumPins;
} Port_ConfigType;

/* Symbolic name (SWS_Port_00006, SWS_Port_00207) */
#define PortConf_PortPin_Button ((Port_PinType)((0u * 16u) + 0u))       /* PA0  = 0  */
#define PortConf_PortPin_Led    ((Port_PinType)((2u * 16u) + 13u))      /* PC13 = 45 */

extern const Port_ConfigType Port_Config;

#endif /* PORT_CFG_H */
