/* Stm32f103_Reg.h - định nghĩa thanh ghi, CHỈ được include trong tầng MCAL (Mcu.c, Port.c, Dio.c) và Os.c.
 * Địa chỉ: RM0008 Rev 21 (memory map 3.3, RCC 7.3, GPIO 9.2), PM0056 (SysTick 4.5). */
#ifndef STM32F103_REG_H
#define STM32F103_REG_H

#include "Std_Types.h"

typedef struct {
    volatile uint32 CR;        /* 0x00 */
    volatile uint32 CFGR;      /* 0x04 */
    volatile uint32 CIR;       /* 0x08 */
    volatile uint32 APB2RSTR;  /* 0x0C */
    volatile uint32 APB1RSTR;  /* 0x10 */
    volatile uint32 AHBENR;    /* 0x14 */
    volatile uint32 APB2ENR;   /* 0x18 */
} RCC_TypeDef;

typedef struct {
    volatile uint32 CRL;       /* 0x00 */
    volatile uint32 CRH;       /* 0x04 */
    volatile uint32 IDR;       /* 0x08 */
    volatile uint32 ODR;       /* 0x0C */
    volatile uint32 BSRR;      /* 0x10 */
    volatile uint32 BRR;       /* 0x14 */
    volatile uint32 LCKR;      /* 0x18 */
} GPIO_TypeDef;

typedef struct {
    volatile uint32 CTRL;      /* 0x00 SYST_CSR */
    volatile uint32 LOAD;      /* 0x04 SYST_RVR */
    volatile uint32 VAL;       /* 0x08 SYST_CVR */
    volatile uint32 CALIB;     /* 0x0C SYST_CALIB */
} SysTick_TypeDef;

#define RCC      ((RCC_TypeDef *)0x40021000UL)
#define GPIOA    ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB    ((GPIO_TypeDef *)0x40010C00UL)
#define GPIOC    ((GPIO_TypeDef *)0x40011000UL)
#define SYSTICK  ((SysTick_TypeDef *)0xE000E010UL)

#define RCC_APB2ENR_IOPAEN  (1UL << 2)
#define RCC_APB2ENR_IOPBEN  (1UL << 3)
#define RCC_APB2ENR_IOPCEN  (1UL << 4)

#endif /* STM32F103_REG_H */
