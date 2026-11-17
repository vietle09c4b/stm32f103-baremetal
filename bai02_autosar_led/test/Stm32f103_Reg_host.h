#ifndef STM32F103_REG_H
#define STM32F103_REG_H
#include "Std_Types.h"
typedef struct { volatile uint32 CR,CFGR,CIR,APB2RSTR,APB1RSTR,AHBENR,APB2ENR; } RCC_TypeDef;
typedef struct { volatile uint32 CRL,CRH,IDR,ODR,BSRR,BRR,LCKR; } GPIO_TypeDef;
typedef struct { volatile uint32 CTRL,LOAD,VAL,CALIB; } SysTick_TypeDef;
extern RCC_TypeDef sim_rcc; extern GPIO_TypeDef sim_gpio[3]; extern SysTick_TypeDef sim_systick;
#define RCC (&sim_rcc)
#define GPIOA (&sim_gpio[0])
#define GPIOB (&sim_gpio[1])
#define GPIOC (&sim_gpio[2])
#define SYSTICK (&sim_systick)
#define RCC_APB2ENR_IOPAEN (1UL<<2)
#define RCC_APB2ENR_IOPBEN (1UL<<3)
#define RCC_APB2ENR_IOPCEN (1UL<<4)
#endif
