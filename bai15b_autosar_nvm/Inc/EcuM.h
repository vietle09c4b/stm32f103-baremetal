/* EcuM.h - ECU State Manager rút gọn (không phải SWS EcuM): khởi động BSW, chờ NvM_ReadAll,
 * chạy SWC, và khi có yêu cầu tắt máy thì gọi NvM_WriteAll rồi dừng ở trạng thái "đã tắt". */
#ifndef ECUM_H
#define ECUM_H

#include "Std_Types.h"

typedef enum {
    ECUM_STATE_STARTUP = 0,    /* chờ NvM_ReadAll */
    ECUM_STATE_RUN,
    ECUM_STATE_SHUTDOWN,       /* chờ NvM_WriteAll */
    ECUM_STATE_OFF             /* đã ghi xong, có thể rút điện */
} EcuM_StateType;

void EcuM_Init(void);
void EcuM_MainFunction(void);
void EcuM_RequestShutdown(void);

extern volatile EcuM_StateType EcuM_State;
extern volatile uint32         EcuM_ReadAllUs;      /* thời gian NvM_ReadAll */
extern volatile uint32         EcuM_WriteAllUs;     /* thời gian NvM_WriteAll */
extern volatile uint8          EcuM_ReadAllResult;  /* kết quả block 0 sau ReadAll */
extern volatile uint8          EcuM_WriteAllResult; /* kết quả block 0 sau WriteAll */

#endif /* ECUM_H */
