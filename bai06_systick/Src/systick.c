/*
 * systick.c - Delay mili giây bằng SysTick, không dùng ngắt
 *
 * Mỗi lần bộ đếm 24 bit đếm từ RELOAD về 0, phần cứng set COUNTFLAG = 1.
 * Đọc STK_CTRL trả về COUNTFLAG và đồng thời xóa cờ này (PM0056 4.5.1),
 * nên mỗi lần vòng while thoát ra tương ứng đúng một chu kỳ 1 ms.
 */
#include "stm32f1xx.h"
#include "systick.h"

#if SYSTICK_USE_HCLK_DIV8
#define SYSTICK_CTRL_CLK   0UL                           /* CLKSOURCE = 0: HCLK/8 */
#else
#define SYSTICK_CTRL_CLK   SysTick_CTRL_CLKSOURCE_Msk    /* CLKSOURCE = 1: HCLK   */
#endif

void systick_delay_ms(uint32_t ms)
{
    /* 1. Nạp giá trị đếm cho chu kỳ 1 ms */
    SysTick->LOAD = SYSTICK_LOAD_1MS;

    /* 2. Ghi VAL: xóa bộ đếm về 0 và xóa COUNTFLAG còn sót từ lần trước */
    SysTick->VAL = 0UL;

    /* 3. Chọn clock và bật bộ đếm, không bật ngắt (TICKINT = 0) */
    SysTick->CTRL = SYSTICK_CTRL_CLK | SysTick_CTRL_ENABLE_Msk;

    /* 4. Mỗi lần COUNTFLAG = 1 là đã trôi qua 1 ms */
    for (uint32_t i = 0U; i < ms; i++) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0UL) {
            /* chờ bộ đếm về 0 */
        }
    }

    /* 5. Tắt SysTick */
    SysTick->CTRL = 0UL;
}
