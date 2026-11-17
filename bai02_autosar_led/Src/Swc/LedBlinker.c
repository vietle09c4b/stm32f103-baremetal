/* LedBlinker.c - Application SWC.
 * Chỉ include Rte_LedBlinker.h: không biết LED nằm ở chân nào, mức nào, trên vi điều khiển nào. */
#include "Rte_LedBlinker.h"

static boolean LedBlinker_On = FALSE;

void LedBlinker_Run500ms(void)
{
    LedBlinker_On = (LedBlinker_On != FALSE) ? FALSE : TRUE;
    (void)Rte_Call_RpLed_Set(LedBlinker_On);
}
