/* ButtonLed.c - Application SWC.
 * Chỉ include Rte_ButtonLed.h: không biết nút và LED nằm ở chân nào, mức nào, có dội hay không. */
#include "Rte_ButtonLed.h"

volatile uint32 ButtonLed_PressCount = 0u;
static boolean  ButtonLed_WasPressed = FALSE;

void ButtonLed_Run10ms(void)
{
    boolean pressed = FALSE;

    if (Rte_Call_RpButton_Get(&pressed) != E_OK) {
        return;
    }
    if ((pressed != FALSE) && (ButtonLed_WasPressed == FALSE)) {
        ButtonLed_PressCount++;                   /* cạnh "vừa nhấn" của tín hiệu đã lọc */
    }
    ButtonLed_WasPressed = pressed;

    (void)Rte_Call_RpLed_Set(pressed);            /* LED sáng khi nút đang được nhấn */
}
