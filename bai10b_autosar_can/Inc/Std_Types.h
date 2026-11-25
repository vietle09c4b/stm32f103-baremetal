/* Std_Types.h - kiểu dữ liệu chuẩn (rút gọn từ AUTOSAR SWS Standard Types + Platform Types) */
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint8_t   boolean;

#ifndef TRUE
#define TRUE   ((boolean)1u)
#endif
#ifndef FALSE
#define FALSE  ((boolean)0u)
#endif
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

typedef uint8 Std_ReturnType;
#define E_OK      ((Std_ReturnType)0x00u)
#define E_NOT_OK  ((Std_ReturnType)0x01u)

#define STD_HIGH  0x01u   /* mức vật lý cao */
#define STD_LOW   0x00u   /* mức vật lý thấp */
#define STD_ON    0x01u
#define STD_OFF   0x00u

#endif /* STD_TYPES_H */
