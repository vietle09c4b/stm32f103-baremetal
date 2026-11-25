/* main.c - đóng vai EcuM: khởi tạo theo thứ tự MCAL -> ECU Abstraction -> Services -> RTE/OS.
 * vietc4b.com, series bare-metal STM32F103: CAN theo kiến trúc AUTOSAR
 * (SWC -> RTE -> Com -> PduR -> LSduR -> CanIf -> Can -> bxCAN).
 *
 * Nối dây: PB9 (CAN_TX) -> TXD, PB8 (CAN_RX) <- RXD của module TJA1050; CANH/CANL -> PCAN-USB
 * (D-Sub chân 7/2, GND chân 3), hai đầu bus có 120 ohm. PCAN-View 500 kbit/s.
 * Kết quả: 0x123 DLC 8 mỗi 100 ms (byte 0 = AliveCounter); gửi 0x321 byte 0 = 01 -> LED PC13 sáng. */
#include "Mcu.h"
#include "Port.h"
#include "IoHwAb.h"
#include "Can.h"
#include "CanIf.h"
#include "LSduR.h"
#include "PduR_LSduR.h"
#include "Com.h"
#include "Os.h"

volatile Std_ReturnType EcuM_ClockStatus = E_NOT_OK;   /* quan sát: E_OK = đang chạy HSE */

int main(void)
{
    Mcu_Init(&Mcu_Config);                                            /* 1. clock GPIOB, GPIOC, AFIO, CAN1 */
    EcuM_ClockStatus = Mcu_InitClock(McuConf_McuClockSettingConfig_Hse8MHz);   /* 2. SYSCLK = HSE */
    if (EcuM_ClockStatus != E_OK) {
        for (;;) { }                                                  /* không chạy CAN với HSI */
    }
    Port_Init(&Port_Config);              /* 3. PB8/PB9 cho CAN (remap), PC13 output */
    IoHwAb_Init0(NULL_PTR);               /* 4. LED tắt */
    Can_Init(&Can_Config);                /* 5. bxCAN: bit timing, filter, ngắt; controller STOPPED */
    CanIf_Init(&CanIf_Config);            /* 6. communication stack, từ dưới lên */
    LSduR_Init();
    PduR_Init();
    Com_Init(&Com_Config);
    (void)CanIf_SetControllerMode(CanConf_CanController_0, CAN_CS_STARTED);   /* 7. vai trò CanSM */
    Os_Start();                           /* 8. SysTick 1 ms, task 10 ms / 100 ms */
    return 0;
}
