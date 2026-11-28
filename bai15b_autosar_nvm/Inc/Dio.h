/* Dio.h - DIO Driver (MCAL). API theo AUTOSAR CP SWS DIO Driver R25-11. */
#ifndef DIO_H
#define DIO_H

#include "Std_Types.h"
#include "Dio_Cfg.h"

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);
void          Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);

#endif /* DIO_H */
