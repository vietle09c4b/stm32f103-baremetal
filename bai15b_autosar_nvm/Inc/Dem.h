/* Dem.h - Diagnostic Event Manager rút gọn: chỉ đếm số lần mỗi event được báo FAILED/PASSED.
 * Không phải hiện thực SWS DEM. Event ID trong dự án thật do cấu hình DEM gán
 * (SWS NvM 7.3.5: "value assigned by DEM"). */
#ifndef DEM_H
#define DEM_H

#include "Std_Types.h"

typedef uint16 Dem_EventIdType;
typedef uint8  Dem_EventStatusType;

#define DEM_EVENT_STATUS_PASSED  0x00u
#define DEM_EVENT_STATUS_FAILED  0x01u

#define DemConf_DemEventParameter_NVM_E_HARDWARE           1u   /* production error (SWS_NvM_00835) */
#define DemConf_DemEventParameter_NVM_E_INTEGRITY_FAILED   2u   /* extended production error */
#define DemConf_DemEventParameter_NVM_E_REQ_FAILED         3u   /* extended production error */
#define DEM_NUM_EVENTS                                     4u

extern volatile uint16 Dem_FailedCount[DEM_NUM_EVENTS];
extern volatile uint16 Dem_PassedCount[DEM_NUM_EVENTS];

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);

#endif /* DEM_H */
