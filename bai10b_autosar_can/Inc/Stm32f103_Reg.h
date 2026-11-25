/* Stm32f103_Reg.h - định nghĩa thanh ghi, CHỈ được include trong tầng MCAL (Mcu, Port, Dio, Can) và Os.c.
 * Địa chỉ và offset: RM0008 Rev 21 (memory map 3.3, RCC 7.3, GPIO/AFIO 9.4, bxCAN 24.9.5),
 * PM0056 (NVIC 4.3, SysTick 4.5). */
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
    volatile uint32 APB1ENR;   /* 0x1C */
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
    volatile uint32 EVCR;      /* 0x00 */
    volatile uint32 MAPR;      /* 0x04 */
} AFIO_TypeDef;

typedef struct {
    volatile uint32 TIR;       /* identifier, IDE, RTR, TXRQ */
    volatile uint32 TDTR;      /* DLC, TGT, TIME */
    volatile uint32 TDLR;      /* data byte 0..3 */
    volatile uint32 TDHR;      /* data byte 4..7 */
} CAN_TxMailBox_TypeDef;

typedef struct {
    volatile uint32 RIR;
    volatile uint32 RDTR;      /* DLC, FMI, TIME */
    volatile uint32 RDLR;
    volatile uint32 RDHR;
} CAN_FIFOMailBox_TypeDef;

typedef struct {
    volatile uint32 FR1;
    volatile uint32 FR2;
} CAN_FilterRegister_TypeDef;

typedef struct {
    volatile uint32 MCR;                         /* 0x000 */
    volatile uint32 MSR;                         /* 0x004 */
    volatile uint32 TSR;                         /* 0x008 */
    volatile uint32 RF0R;                        /* 0x00C */
    volatile uint32 RF1R;                        /* 0x010 */
    volatile uint32 IER;                         /* 0x014 */
    volatile uint32 ESR;                         /* 0x018 */
    volatile uint32 BTR;                         /* 0x01C */
    uint32          RESERVED0[88];               /* 0x020..0x17F */
    CAN_TxMailBox_TypeDef   sTxMailBox[3];       /* 0x180..0x1AF */
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];     /* 0x1B0..0x1CF */
    uint32          RESERVED1[12];               /* 0x1D0..0x1FF */
    volatile uint32 FMR;                         /* 0x200 */
    volatile uint32 FM1R;                        /* 0x204 */
    uint32          RESERVED2;
    volatile uint32 FS1R;                        /* 0x20C */
    uint32          RESERVED3;
    volatile uint32 FFA1R;                       /* 0x214 */
    uint32          RESERVED4;
    volatile uint32 FA1R;                        /* 0x21C */
    uint32          RESERVED5[8];                /* 0x220..0x23F */
    CAN_FilterRegister_TypeDef sFilterRegister[14]; /* 0x240..0x2AF, 14 bank trên STM32F103 */
} CAN_TypeDef;

typedef struct {
    volatile uint32 CTRL;      /* 0x00 SYST_CSR */
    volatile uint32 LOAD;      /* 0x04 SYST_RVR */
    volatile uint32 VAL;       /* 0x08 SYST_CVR */
    volatile uint32 CALIB;     /* 0x0C SYST_CALIB */
} SysTick_TypeDef;

#define RCC      ((RCC_TypeDef *)0x40021000UL)
#define AFIO     ((AFIO_TypeDef *)0x40010000UL)
#define GPIOA    ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB    ((GPIO_TypeDef *)0x40010C00UL)
#define GPIOC    ((GPIO_TypeDef *)0x40011000UL)
#define CAN1     ((CAN_TypeDef *)0x40006400UL)
#define SYSTICK  ((SysTick_TypeDef *)0xE000E010UL)
#define NVIC_ISER0  (*(volatile uint32 *)0xE000E100UL)
#define NVIC_IPR_BYTE(irq)  (*(volatile uint8 *)(0xE000E400UL + (uint32)(irq)))

/* RCC (RM0008 7.3.1, 7.3.2, 7.3.7, 7.3.8) */
#define RCC_CR_HSEON          (1UL << 16)
#define RCC_CR_HSERDY         (1UL << 17)
#define RCC_CFGR_SW_MASK      (3UL << 0)
#define RCC_CFGR_SW_HSE       (1UL << 0)
#define RCC_CFGR_SWS_MASK     (3UL << 2)
#define RCC_CFGR_SWS_HSE      (1UL << 2)
#define RCC_APB2ENR_AFIOEN    (1UL << 0)
#define RCC_APB2ENR_IOPAEN    (1UL << 2)
#define RCC_APB2ENR_IOPBEN    (1UL << 3)
#define RCC_APB2ENR_IOPCEN    (1UL << 4)
#define RCC_APB1ENR_CAN1EN    (1UL << 25)

/* AFIO_MAPR (RM0008 9.4.2): CAN_REMAP[14:13], SWJ_CFG[26:24] chỉ ghi */
#define AFIO_MAPR_CAN_REMAP_MASK  (3UL << 13)
#define AFIO_MAPR_CAN_REMAP_PB8   (2UL << 13)    /* 10: CAN_RX = PB8, CAN_TX = PB9 */
#define AFIO_MAPR_SWJ_CFG_MASK    (7UL << 24)

/* bxCAN (RM0008 24.9.2, 24.9.3, 24.9.4) */
#define CAN_MCR_INRQ          (1UL << 0)
#define CAN_MCR_SLEEP         (1UL << 1)
#define CAN_MCR_ABOM          (1UL << 6)
#define CAN_MCR_DBF           (1UL << 16)
#define CAN_MSR_INAK          (1UL << 0)
#define CAN_MSR_SLAK          (1UL << 1)
#define CAN_TSR_RQCP0         (1UL << 0)
#define CAN_TSR_TXOK0         (1UL << 1)
#define CAN_TSR_CODE_POS      24u
#define CAN_TSR_CODE_MASK     (3UL << 24)
#define CAN_TSR_TME_MASK      (7UL << 26)
#define CAN_RF0R_FMP0_MASK    (3UL << 0)
#define CAN_RF0R_RFOM0        (1UL << 5)
#define CAN_IER_FMPIE0        (1UL << 1)
#define CAN_BTR_LBKM          (1UL << 30)
#define CAN_BTR_SILM          (1UL << 31)
#define CAN_TIR_TXRQ          (1UL << 0)
#define CAN_FMR_FINIT         (1UL << 0)

#define CAN1_RX0_IRQN         20u                /* USB_LP_CAN1_RX0, RM0008 Table 63 */

#endif /* STM32F103_REG_H */
