/* TestHook.c - lệnh thử nghiệm điều khiển bằng debugger, chạy trong task 1 ms (sau EcuM).
 * Mã này cố ý đi tắt qua các lớp để giả lập lỗi và đo đạc; không dùng trong sản phẩm. */
#include "TestHook.h"
#include "EcuM.h"
#include "NvM.h"
#include "Fee.h"
#include "Fls.h"
#include "Fls_Ac.h"
#include "Os.h"
#include "Rte_NvmDemo.h"

volatile uint8  TestHook_Cmd  = 0u;
volatile uint8  TestHook_Busy = 0u;
volatile uint32 TestHook_T3_Offset = 0u;
volatile uint32 TestHook_T3_Err    = 0xFFFFFFFFu;
volatile uint32 TestHook_T4_Done   = 0u;
volatile uint32 TestHook_T4_Failed = 0u;
volatile uint32 TestHook_T4_MaxUs  = 0u;
volatile uint32 TestHook_T4_MinUs  = 0xFFFFFFFFu;
volatile uint32 TestHook_T4_GcBefore = 0u, TestHook_T4_GcAfter = 0u;
volatile uint32 TestHook_T4_TotalUs  = 0u;
volatile uint8  TestHook_EraseResult = 0xFFu;

static boolean TestHook_T4_Waiting = FALSE;
static uint32  TestHook_T4_T0 = 0u, TestHook_T4_Start = 0u;

static boolean TestHook_StackIdle(void)
{
    NvM_RequestResultType r = NVM_REQ_PENDING;
    (void)NvM_GetErrorStatus(NvMConf_NvMBlockDescriptor_BootCounter, &r);
    return ((r != NVM_REQ_PENDING) && (Fee_GetStatus() == MEMIF_IDLE) && (Fls_GetStatus() == MEMIF_IDLE))
           ? TRUE : FALSE;
}

static void TestHook_T3(void)
{
    uint32 off = 0u;
    if (Fee_Dbg_GetDataOffset(NvMConf_NvMBlockDescriptor_BootCounter, &off) != E_OK) {
        TestHook_T3_Err = 0xEEEEEEEEu;
        return;
    }
    TestHook_T3_Offset = off;
    Fls_Ac_Unlock();
    TestHook_T3_Err = Fls_Ac_ProgramHalf(FLS_BASE_ADDRESS + off, 0x0000u);   /* 0x0000 luôn ghi được */
    Fls_Ac_Lock();
}

static void TestHook_T4Step(void)
{
    NvM_RequestResultType r = NVM_REQ_PENDING;
    (void)NvM_GetErrorStatus(NvMConf_NvMBlockDescriptor_BootCounter, &r);
    if (TestHook_T4_Waiting != FALSE) {
        if (r == NVM_REQ_PENDING) { return; }
        uint32 us = Os_CyclesToUs(Os_GetCycles() - TestHook_T4_T0);
        if (us > TestHook_T4_MaxUs) { TestHook_T4_MaxUs = us; }
        if (us < TestHook_T4_MinUs) { TestHook_T4_MinUs = us; }
        if (r != NVM_REQ_OK)        { TestHook_T4_Failed++; }
        TestHook_T4_Done++;
        TestHook_T4_Waiting = FALSE;
    }
    if (TestHook_T4_Done >= TESTHOOK_STRESS_WRITES) {
        TestHook_T4_GcAfter = Fee_GcCount;
        TestHook_T4_TotalUs = Os_CyclesToUs(Os_GetCycles() - TestHook_T4_Start);
        TestHook_Busy = 0u;
        return;
    }
    Rte_Pim_BootCounter()->BootCount++;
    TestHook_T4_T0 = Os_GetCycles();
    if (NvM_WriteBlock(NvMConf_NvMBlockDescriptor_BootCounter, NULL_PTR) == E_OK) {
        TestHook_T4_Waiting = TRUE;
    }
}

void TestHook_MainFunction(void)
{
    if (EcuM_State != ECUM_STATE_RUN) { return; }

    if (TestHook_Busy == 2u) { TestHook_T4Step(); return; }
    if (TestHook_Busy == 3u) {
        if (Fls_GetStatus() == MEMIF_IDLE) {
            TestHook_EraseResult = (uint8)Fls_GetJobResult();
            TestHook_Busy = 0u;
        }
        return;
    }
    if ((TestHook_Cmd == 0u) || (TestHook_StackIdle() == FALSE)) { return; }

    switch (TestHook_Cmd) {
    case 1u:
        TestHook_T3();
        break;
    case 2u:
        TestHook_T4_Done = 0u; TestHook_T4_Failed = 0u;
        TestHook_T4_MaxUs = 0u; TestHook_T4_MinUs = 0xFFFFFFFFu;
        TestHook_T4_GcBefore = Fee_GcCount;
        TestHook_T4_Start = Os_GetCycles();
        TestHook_T4_Waiting = FALSE;
        TestHook_Busy = 2u;
        break;
    case 3u:
        if (Fls_Erase(0u, FLS_TOTAL_SIZE) == E_OK) { TestHook_Busy = 3u; }
        break;
    default:
        break;
    }
    TestHook_Cmd = 0u;
}
