/*
 * stm32f103_reg.h - thanh ghi STM32F103 dùng trong bài 3 (LCD1602 I2C + đồng hồ)
 * Địa chỉ và offset theo RM0008 Rev 21:
 *   RCC   0x4002 1000 (mục 7.3), GPIOB 0x4001 0C00, GPIOC 0x4001 1000 (mục 9.2),
 *   I2C1  0x4000 5400 (mục 26.6), SysTick 0xE000 E010 (PM0056 mục 4.5).
 */
#ifndef STM32F103_REG_H
#define STM32F103_REG_H

#include <stdint.h>

#define __IO volatile

typedef struct {
    __IO uint32_t CR;        /* 0x00 */
    __IO uint32_t CFGR;      /* 0x04 */
    __IO uint32_t CIR;       /* 0x08 */
    __IO uint32_t APB2RSTR;  /* 0x0C */
    __IO uint32_t APB1RSTR;  /* 0x10 */
    __IO uint32_t AHBENR;    /* 0x14 */
    __IO uint32_t APB2ENR;   /* 0x18 */
    __IO uint32_t APB1ENR;   /* 0x1C */
    __IO uint32_t BDCR;      /* 0x20 */
    __IO uint32_t CSR;       /* 0x24 */
} RCC_TypeDef;

typedef struct {
    __IO uint32_t CRL;       /* 0x00 */
    __IO uint32_t CRH;       /* 0x04 */
    __IO uint32_t IDR;       /* 0x08 */
    __IO uint32_t ODR;       /* 0x0C */
    __IO uint32_t BSRR;      /* 0x10 */
    __IO uint32_t BRR;       /* 0x14 */
    __IO uint32_t LCKR;      /* 0x18 */
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR1;       /* 0x00 */
    __IO uint32_t CR2;       /* 0x04 */
    __IO uint32_t OAR1;      /* 0x08 */
    __IO uint32_t OAR2;      /* 0x0C */
    __IO uint32_t DR;        /* 0x10 */
    __IO uint32_t SR1;       /* 0x14 */
    __IO uint32_t SR2;       /* 0x18 */
    __IO uint32_t CCR;       /* 0x1C */
    __IO uint32_t TRISE;     /* 0x20 */
} I2C_TypeDef;

typedef struct {
    __IO uint32_t CTRL;      /* 0x00 STK_CTRL  */
    __IO uint32_t LOAD;      /* 0x04 STK_LOAD  */
    __IO uint32_t VAL;       /* 0x08 STK_VAL   */
    __IO uint32_t CALIB;     /* 0x0C STK_CALIB */
} SysTick_TypeDef;

#define RCC      ((RCC_TypeDef     *)0x40021000UL)
#define GPIOB    ((GPIO_TypeDef    *)0x40010C00UL)
#define GPIOC    ((GPIO_TypeDef    *)0x40011000UL)
#define I2C1     ((I2C_TypeDef     *)0x40005400UL)
#define SYSTICK  ((SysTick_TypeDef *)0xE000E010UL)

/* RCC */
#define RCC_CR_HSEON        (1UL << 16)
#define RCC_CR_HSERDY       (1UL << 17)
#define RCC_CFGR_SW_MASK    (3UL << 0)
#define RCC_CFGR_SW_HSE     (1UL << 0)
#define RCC_CFGR_SWS_MASK   (3UL << 2)
#define RCC_CFGR_SWS_HSE    (1UL << 2)
#define RCC_APB2ENR_IOPBEN  (1UL << 3)
#define RCC_APB2ENR_IOPCEN  (1UL << 4)
#define RCC_APB1ENR_I2C1EN  (1UL << 21)

/* I2C (RM0008 mục 26.6) */
#define I2C_CR1_PE          (1UL << 0)
#define I2C_CR1_START       (1UL << 8)
#define I2C_CR1_STOP        (1UL << 9)
#define I2C_CR1_SWRST       (1UL << 15)
#define I2C_SR1_SB          (1UL << 0)
#define I2C_SR1_ADDR        (1UL << 1)
#define I2C_SR1_BTF         (1UL << 2)
#define I2C_SR1_TXE         (1UL << 7)
#define I2C_SR1_BERR        (1UL << 8)
#define I2C_SR1_ARLO        (1UL << 9)
#define I2C_SR1_AF          (1UL << 10)
#define I2C_SR2_BUSY        (1UL << 1)

/* SysTick (PM0056 mục 4.5.1) */
#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_TICKINT   (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)   /* 1 = clock lõi (HCLK) */

#endif /* STM32F103_REG_H */
