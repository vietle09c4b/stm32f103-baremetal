/* Port.c - Port Driver (MCAL).
 * Port_Init khởi tạo mọi chân trong cấu hình (SWS_Port_00041). Mức output ban đầu được ghi vào ODR
 * TRƯỚC khi chuyển chân sang output (SWS_Port_00055), nên chân không xuất mức sai (SWS_Port_00043). */
#include "Port.h"
#include "Stm32f103_Reg.h"

static GPIO_TypeDef * const Port_Base[] = { GPIOA, GPIOB, GPIOC };

void Port_Init(const Port_ConfigType *ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return;
    }
    for (uint8 i = 0u; i < ConfigPtr->NumPins; i++) {
        const Port_PinConfigType *cfg = &ConfigPtr->Pins[i];
        GPIO_TypeDef *gpio  = Port_Base[cfg->Pin / 16u];
        uint32        pin   = (uint32)(cfg->Pin % 16u);
        uint32        mask  = 1UL << pin;

        if (cfg->Direction == PORT_PIN_OUT) {
            if (cfg->InitialLevel == STD_HIGH) { gpio->BSRR = mask; }   /* SWS_Port_00055: mức trước, hướng sau */
            else                               { gpio->BRR  = mask; }
        }

        volatile uint32 *cr    = (pin < 8u) ? &gpio->CRL : &gpio->CRH;
        uint32           shift = (pin % 8u) * 4u;
        uint32           val   = *cr;
        val &= ~(0xFUL << shift);                       /* reset = 0100: phải xóa đủ 4 bit */
        val |= ((uint32)cfg->Mode & 0xFUL) << shift;
        *cr  = val;
    }
}
