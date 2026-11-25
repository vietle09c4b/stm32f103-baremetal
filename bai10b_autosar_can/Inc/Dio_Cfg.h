/* Dio_Cfg.h - kiểu dữ liệu và symbolic name của DIO Driver (SWS_Dio_00017, SWS_Dio_00026). */
#ifndef DIO_CFG_H
#define DIO_CFG_H

#include "Std_Types.h"

typedef uint8 Dio_ChannelType;     /* ID = chỉ số port x 16 + số chân, cùng cách mã hóa với Port */
typedef uint8 Dio_LevelType;       /* STD_LOW / STD_HIGH (SWS_Dio_00089) */

#define DioConf_DioChannel_Led    ((Dio_ChannelType)((2u * 16u) + 13u))   /* PC13 = 45 */

#define DIO_NUM_PORTS   3u          /* GPIOA..GPIOC */

#endif /* DIO_CFG_H */
