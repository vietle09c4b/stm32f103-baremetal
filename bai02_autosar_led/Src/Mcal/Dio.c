/* Dio.c - DIO Driver (MCAL).
 * Ghi bằng BSRR/BRR: một lần ghi, không đọc-sửa-ghi, nên đáp ứng SWS_Dio_00005 (nhất quán khi có ngắt)
 * và reentrant (SWS_Dio_00060). DIO không cấu hình hướng chân; việc đó thuộc Port (SWS_Dio_00061). */
#include "Dio.h"
#include "Stm32f103_Reg.h"

static GPIO_TypeDef * const Dio_PortBase[DIO_NUM_PORTS] = { GPIOA, GPIOB, GPIOC };

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    uint8 port = (uint8)(ChannelId / 16u);
    if (port >= DIO_NUM_PORTS) {
        return STD_LOW;                           /* DET: DIO_E_PARAM_INVALID_CHANNEL_ID */
    }
    uint32 mask = 1UL << (ChannelId % 16u);
    return ((Dio_PortBase[port]->IDR & mask) != 0u) ? STD_HIGH : STD_LOW;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    uint8 port = (uint8)(ChannelId / 16u);
    if (port >= DIO_NUM_PORTS) {
        return;                                   /* DET: DIO_E_PARAM_INVALID_CHANNEL_ID */
    }
    uint32 mask = 1UL << (ChannelId % 16u);
    if (Level == STD_HIGH) {
        Dio_PortBase[port]->BSRR = mask;
    } else {
        Dio_PortBase[port]->BRR = mask;
    }
}
