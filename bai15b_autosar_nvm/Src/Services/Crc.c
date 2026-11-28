/* Crc.c - CRC16 CCITT-FALSE tính từng bit (SWS_Crc_00002, SWS_Crc_00019).
 * Khi Crc_IsFirstCall = TRUE, giá trị đầu là 0xFFFF; khi FALSE, dùng Crc_StartValue16 là kết quả lần gọi
 * trước (XOR ra bằng 0 nên kết quả lần trước dùng thẳng làm giá trị đầu). */
#include "Crc.h"

#define CRC16_POLY     0x1021u
#define CRC16_INIT     0xFFFFu
#define CRC16_XOROUT   0x0000u

uint16 Crc_CalculateCRC16(const uint8 *Crc_DataPtr, uint32 Crc_Length,
                          uint16 Crc_StartValue16, boolean Crc_IsFirstCall)
{
    uint16 crc = (Crc_IsFirstCall != FALSE) ? CRC16_INIT : (uint16)(Crc_StartValue16 ^ CRC16_XOROUT);

    for (uint32 i = 0u; i < Crc_Length; i++) {
        crc = (uint16)(crc ^ (uint16)((uint32)Crc_DataPtr[i] << 8u));
        for (uint8 b = 0u; b < 8u; b++) {
            crc = ((crc & 0x8000u) != 0u) ? (uint16)((crc << 1) ^ CRC16_POLY) : (uint16)(crc << 1);
        }
    }
    return (uint16)(crc ^ CRC16_XOROUT);
}
