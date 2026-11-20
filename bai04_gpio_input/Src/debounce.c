/*
 * debounce.c - Chống dội phím bằng thời gian ổn định
 */
#include "debounce.h"

void debounce_init(debounce_t *db, uint8_t level, uint32_t now_ms)
{
    db->stable   = level ? 1u : 0u;
    db->last_raw = db->stable;
    db->t_change = now_ms;
}

db_event_t debounce_update(debounce_t *db, uint8_t raw, uint32_t now_ms)
{
    raw = raw ? 1u : 0u;

    if (raw != db->last_raw) {          /* mức thô vừa đổi: bắt đầu đếm lại */
        db->last_raw = raw;
        db->t_change = now_ms;
        return DB_NONE;
    }
    if (raw != db->stable &&
        (uint32_t)(now_ms - db->t_change) >= DEBOUNCE_MS) {   /* giữ đủ lâu */
        db->stable = raw;
        return raw ? DB_RISE : DB_FALL;
    }
    return DB_NONE;
}
