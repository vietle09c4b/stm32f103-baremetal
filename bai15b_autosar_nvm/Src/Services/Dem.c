/* Dem.c - DEM rút gọn: đếm số lần báo của từng event để đọc bằng debugger. */
#include "Dem.h"

volatile uint16 Dem_FailedCount[DEM_NUM_EVENTS];
volatile uint16 Dem_PassedCount[DEM_NUM_EVENTS];

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    if (EventId >= DEM_NUM_EVENTS) {
        return E_NOT_OK;
    }
    if (EventStatus == DEM_EVENT_STATUS_FAILED) { Dem_FailedCount[EventId]++; }
    else                                        { Dem_PassedCount[EventId]++; }
    return E_OK;
}
