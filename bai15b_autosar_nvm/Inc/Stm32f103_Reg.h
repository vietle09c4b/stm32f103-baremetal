/* Stm32f103_Reg.h - định nghĩa thanh ghi, CHỈ được include trong tầng MCAL (Mcu, Port, Dio, Fls) và Os.c.
 * Địa chỉ: RM0008 Rev 21 (memory map 3.3, RCC 7.3, GPIO 9.2), PM0075 Rev 2 (FPEC 3.4),
 * PM0056 (SysTick 4.5), ARMv7-M ARM (DWT C1.8, DEMCR C1.6.5). */
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
    volatile uint32 ACR;       /* 0x00 */
    volatile uint32 KEYR;      /* 0x04 */
    volatile uint32 OPTKEYR;   /* 0x08 */
    volatile uint32 SR;        /* 0x0C */
    volatile uint32 CR;        /* 0x10 */
    volatile uint32 AR;        /* 0x14 */
    volatile uint32 RESERVED;  /* 0x18 */
    volatile uint32 OBR;       /* 0x1C */
    volatile uint32 WRPR;      /* 0x20 */
} FLASH_TypeDef;

typedef struct {
    volatile uint32 CTRL;      /* 0x00 SYST_CSR */
    volatile uint32 LOAD;      /* 0x04 SYST_RVR */
    volatile uint32 VAL;       /* 0x08 SYST_CVR */
    volatile uint32 CALIB;     /* 0x0C SYST_CALIB */
} SysTick_TypeDef;

typedef struct {
    volatile uint32 CTRL;      /* 0x00 */
    volatile uint32 CYCCNT;    /* 0x04 */
} DWT_TypeDef;

#define RCC      ((RCC_TypeDef *)0x40021000UL)
#define GPIOA    ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB    ((GPIO_TypeDef *)0x40010C00UL)
#define GPIOC    ((GPIO_TypeDef *)0x40011000UL)
#define FLASH    ((FLASH_TypeDef *)0x40022000UL)
#define SYSTICK  ((SysTick_TypeDef *)0xE000E010UL)
#define DWT      ((DWT_TypeDef *)0xE0001000UL)
#define DEMCR    (*(volatile uint32 *)0xE000EDFCUL)

#define RCC_CR_HSION        (1UL << 0)
#define RCC_APB2ENR_IOPAEN  (1UL << 2)
#define RCC_APB2ENR_IOPBEN  (1UL << 3)
#define RCC_APB2ENR_IOPCEN  (1UL << 4)

#define FLASH_SR_BSY        (1UL << 0)
#define FLASH_SR_PGERR      (1UL << 2)
#define FLASH_SR_WRPRTERR   (1UL << 4)
#define FLASH_SR_EOP        (1UL << 5)
#define FLASH_CR_PG         (1UL << 0)
#define FLASH_CR_PER        (1UL << 1)
#define FLASH_CR_STRT       (1UL << 6)
#define FLASH_CR_LOCK       (1UL << 7)
#define FPEC_KEY1           0x45670123UL   /* PM0075 2.3.1 */
#define FPEC_KEY2           0xCDEF89ABUL

#define DEMCR_TRCENA        (1UL << 24)
#define DWT_CTRL_CYCCNTENA  (1UL << 0)

#endif /* STM32F103_REG_H */
