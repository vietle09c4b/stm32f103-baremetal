/* Port.c - Port Driver (MCAL).
 * Port Driver khởi tạo mọi chân trong cấu hình (SWS_Port_00041), kể cả chức năng thay thế của chân.
 * Trên STM32F103, việc chọn chân cho CAN nằm ở AFIO_MAPR (CAN_REMAP = 10: PB8/PB9, RM0008 9.3.3).
 * SWJ_CFG[26:24] là trường chỉ ghi, đọc ra không xác định, nên luôn ghi 000 (giữ SWD như sau reset). */
#include "Port.h"
#include "Stm32f103_Reg.h"

static GPIO_TypeDef * const Port_Base[] = { GPIOA, GPIOB, GPIOC };

void Port_Init(const Port_ConfigType *ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return;
    }

    uint32 mapr = AFIO->MAPR;
    mapr &= ~(ConfigPtr->AfioMaprMask | AFIO_MAPR_SWJ_CFG_MASK);
    mapr |=  ConfigPtr->AfioMaprValue;
    AFIO->MAPR = mapr;

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
            else                               { gpio->BRR  = mask; }
        } else {
            /* input floating: ODR không dùng */
        }

        volatile uint32 *cr    = (pin < 8u) ? &gpio->CRL : &gpio->CRH;
        uint32           shift = (pin % 8u) * 4u;
        uint32           val   = *cr;
        val &= ~((uint32)0xFu << shift);
        val |= ((uint32)cfg->Mode & 0xFu) << shift;
        *cr  = val;
    }
}
