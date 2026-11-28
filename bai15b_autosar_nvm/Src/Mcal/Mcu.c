/* Mcu.c - MCU Driver (MCAL).
 * RCC_APB2ENR điều khiển clock của nhiều ngoại vi và không phải thanh ghi I/O, nên do MCU Driver
 * khởi tạo (SWS_Mcu_00245, SWS_Port_00215). Clock hệ thống giữ HSI 8 MHz sau reset. */
#include "Mcu.h"
#include "Stm32f103_Reg.h"

void Mcu_Init(const Mcu_ConfigType *ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return;                                   /* DET không có trong ví dụ này */
    }
    RCC->APB2ENR |= ConfigPtr->Apb2ClockEnable;
    (void)RCC->APB2ENR;                           /* đọc lại trước khi Port_Init truy cập GPIO */
    /* HSI giữ nguyên trạng thái bật sau reset: FPEC cần HSI khi ghi/xóa (RM0008 3.3.3) */
    if (ConfigPtr->EnableCycleCounter != FALSE) { /* bộ đếm chu kỳ DWT để đo thời gian */
        DEMCR |= DEMCR_TRCENA;
        DWT->CYCCNT = 0u;
        DWT->CTRL  |= DWT_CTRL_CYCCNTENA;
    }
}
