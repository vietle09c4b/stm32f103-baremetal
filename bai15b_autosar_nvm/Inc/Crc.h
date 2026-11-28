/* Crc.h - CRC Library (Libraries), chỉ phần CRC16. AUTOSAR CP SWS CRCLibrary R20-11.
 * SWS_Crc_00002: CCITT-FALSE, đa thức 0x1021, giá trị đầu 0xFFFF, không đảo bit vào/ra, XOR ra 0x0000,
 * check (chuỗi "123456789") = 0x29B1. API: SWS_Crc_00019. */
#ifndef CRC_H
#define CRC_H

#include "Std_Types.h"

uint16 Crc_CalculateCRC16(const uint8 *Crc_DataPtr, uint32 Crc_Length,
                          uint16 Crc_StartValue16, boolean Crc_IsFirstCall);

#endif /* CRC_H */
