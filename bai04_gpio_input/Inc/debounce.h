/*
 * debounce.h - Chống dội phím bằng thời gian ổn định (không phụ thuộc phần cứng)
 *
 * Mức đọc từ IDR chỉ được chấp nhận là mức mới khi giữ nguyên liên tục
 * DEBOUNCE_MS mili giây. Module chỉ nhận mức thô và thời gian hiện tại,
 * nên kiểm thử được trên máy tính.
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

#define DEBOUNCE_MS  20U

typedef enum {
    DB_NONE = 0,     /* không có thay đổi mức ổn định */
    DB_FALL,         /* mức ổn định 1 -> 0 (nút được nhấn, pull-up) */
    DB_RISE          /* mức ổn định 0 -> 1 (nút được nhả) */
} db_event_t;

typedef struct {
    uint8_t  stable;     /* mức đã lọc */
    uint8_t  last_raw;   /* mức thô lần đọc trước */
    uint32_t t_change;   /* thời điểm mức thô đổi gần nhất (ms) */
} debounce_t;

void       debounce_init(debounce_t *db, uint8_t level, uint32_t now_ms);
db_event_t debounce_update(debounce_t *db, uint8_t raw, uint32_t now_ms);

#endif /* DEBOUNCE_H */
