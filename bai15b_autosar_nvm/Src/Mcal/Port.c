/* Port.c - Port Driver (MCAL).
 * Port_Init khởi tạo mọi chân trong cấu hình (SWS_Port_00041). Mức output ban đầu được ghi vào ODR
 * TRƯỚC khi chuyển chân sang output (SWS_Port_00055), nên chân không xuất mức sai (SWS_Port_00043).
 * Với input có điện trở kéo, STM32F103 dùng bit ODR của chân để chọn kéo lên (1) hay kéo xuống (0)
 * (RM0008 Table 20); bit này cũng được ghi trước khi đổi chế độ chân. */
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
        } else if (cfg->Mode == PORT_MODE_IN_PULL) {
            if (cfg->Pull == PORT_PULL_UP)     { gpio->BSRR = mask; }   /* ODRy = 1: kéo lên */
            else                               { gpio->BRR  = mask; }   /* ODRy = 0: kéo xuống */
        } else {
            /* input floating / analog: ODR không dùng */
        }

        volatile uint32 *cr    = (pin < 8u) ? &gpio->CRL : &gpio->CRH;
        uint32           shift = (pin % 8u) * 4u;
        uint32           val   = *cr;
        val &= ~((uint32)0xFu << shift);                       /* reset = 0100: phải xóa đủ 4 bit */
        val |= ((uint32)cfg->Mode & 0xFu) << shift;
        *cr  = val;
    }
}
