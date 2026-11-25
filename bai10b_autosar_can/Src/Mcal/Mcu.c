/* Mcu.c - MCU Driver (MCAL).
 * Mcu_Init bật clock ngoại vi (RCC_APB2ENR, RCC_APB1ENR); Mcu_InitClock chuyển SYSCLK sang HSE 8 MHz.
 * HSI không đủ chính xác cho CAN 500 kbit/s (dung sai cho phép của cấu hình bit timing cỡ 0,3 %),
 * nên ECU dùng thạch anh HSE và không bật PLL: Flash giữ 0 wait state (RM0008 3.3.3). */
#include "Mcu.h"
#include "Stm32f103_Reg.h"

#define MCU_HSE_TIMEOUT   200000UL

void Mcu_Init(const Mcu_ConfigType *ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return;                                   /* DET không có trong ví dụ này */
    }
    RCC->APB2ENR |= ConfigPtr->Apb2ClockEnable;
    RCC->APB1ENR |= ConfigPtr->Apb1ClockEnable;
    (void)RCC->APB1ENR;                           /* đọc lại trước khi Port/Can truy cập ngoại vi */
}

Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting)
{
    if (ClockSetting != McuConf_McuClockSettingConfig_Hse8MHz) {
        return E_NOT_OK;
    }
    RCC->CR |= RCC_CR_HSEON;                      /* RM0008 7.3.1 */
    for (uint32 t = 0u; t < MCU_HSE_TIMEOUT; t++) {
        if ((RCC->CR & RCC_CR_HSERDY) != 0u) {
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSE;    /* RM0008 7.3.2 */
            while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSE) { }
            return E_OK;
        }
    }
    RCC->CR &= ~RCC_CR_HSEON;                     /* không có thạch anh: giữ HSI */
    return E_NOT_OK;
}
