/* NvmDemo.c - Application SWC. Chỉ include Rte_NvmDemo.h: không biết dữ liệu nằm ở trang Flash nào,
 * được Fee tổ chức ra sao hay CRC tính thế nào.
 *  - BootCounter: tăng 1 mỗi lần khởi động, ghi ngay bằng WriteBlock.
 *  - LedConfig: chu kỳ nháy LED; nhấn ngắn đổi 100 -> 250 -> 500 -> 1000 ms và báo NvM "đã thay đổi"
 *    (SetRamBlockStatus); dữ liệu chỉ được ghi khi tắt máy (NvM_WriteAll).
 *  - Giữ nút từ 2 s: yêu cầu EcuM tắt máy. */
#include "Rte_NvmDemo.h"

#define NVMDEMO_LONG_PRESS_TICKS   200u     /* 200 x 10 ms = 2 s */

volatile uint8  NvmDemo_BootReadResult = 0xFFu;
volatile uint8  NvmDemo_LedReadResult  = 0xFFu;
volatile uint32 NvmDemo_ShortPresses   = 0u;

static boolean NvmDemo_WasPressed  = FALSE;
static boolean NvmDemo_Armed       = FALSE;   /* chỉ nhận nút sau khi đã thấy nút nhả một lần */
static uint16  NvmDemo_PressTicks  = 0u;
static boolean NvmDemo_ShutdownReq = FALSE;
static uint16  NvmDemo_BlinkTicks  = 0u;
static boolean NvmDemo_LedOn       = FALSE;

static uint16 NvmDemo_NextPeriod(uint16 p)
{
    switch (p) {
    case 100u: return 250u;
    case 250u: return 500u;
    case 500u: return 1000u;
    default:   return 100u;
    }
}

void NvmDemo_Init(void)
{
    NvM_RequestResultType_Rte r = 0xFFu;
    (void)Rte_Call_NvMBootCounter_GetErrorStatus(&r);
    NvmDemo_BootReadResult = r;
    (void)Rte_Call_NvMLedConfig_GetErrorStatus(&r);
    NvmDemo_LedReadResult = r;

    Rte_Pim_BootCounter()->BootCount++;
    (void)Rte_Call_NvMBootCounter_WriteBlock(NULL_PTR);     /* NULL: dùng permanent RAM block */
}

void NvmDemo_Run10ms(void)
{
    boolean pressed = FALSE;
    (void)Rte_Call_RpButton_Get(&pressed);
    if (NvmDemo_Armed == FALSE) {                            /* nút kẹt lúc khởi động không gây tắt máy */
        if (pressed == FALSE) { NvmDemo_Armed = TRUE; }
        pressed = FALSE;
    }

    if (pressed != FALSE) {
        if (NvmDemo_PressTicks < 0xFFFFu) { NvmDemo_PressTicks++; }
        if ((NvmDemo_PressTicks >= NVMDEMO_LONG_PRESS_TICKS) && (NvmDemo_ShutdownReq == FALSE)) {
            NvmDemo_ShutdownReq = TRUE;
            (void)Rte_Call_RpShutdown_Request();
        }
    } else {
        if ((NvmDemo_WasPressed != FALSE) && (NvmDemo_PressTicks < NVMDEMO_LONG_PRESS_TICKS)) {
            NvmDemo_LedConfigType *cfg = Rte_Pim_LedConfig();
            cfg->BlinkPeriodMs = NvmDemo_NextPeriod(cfg->BlinkPeriodMs);
            cfg->ChangeCount++;
            (void)Rte_Call_NvMLedConfig_SetRamBlockStatus(TRUE);
            NvmDemo_ShortPresses++;
        }
        NvmDemo_PressTicks = 0u;
    }
    NvmDemo_WasPressed = pressed;

    if (NvmDemo_ShutdownReq != FALSE) {
        return;                                              /* EcuM điều khiển LED khi tắt máy */
    }
    uint16 half = (uint16)(Rte_Pim_LedConfig()->BlinkPeriodMs / 20u);   /* nửa chu kỳ, đơn vị 10 ms */
    if (half == 0u) { half = 1u; }
    if (++NvmDemo_BlinkTicks >= half) {
        NvmDemo_BlinkTicks = 0u;
        NvmDemo_LedOn = (NvmDemo_LedOn != FALSE) ? FALSE : TRUE;
        (void)Rte_Call_RpLed_Set(NvmDemo_LedOn);
    }
}
