/*
 * host_test.c - Kiểm thử debounce.c trên máy tính
 *   gcc -std=c11 -Wall -Wextra -fsanitize=address,undefined -I../Inc host_test.c ../Src/debounce.c -o t && ./t
 */
#include <stdio.h>
#include "debounce.h"

static int pass, fail;
#define CHECK(c, msg) do { if (c) pass++; else { fail++; printf("FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

/* Mô phỏng: chạy debounce mỗi 1 ms trên một chuỗi mức thô, đếm sự kiện */
static void run(const uint8_t *lv, int n, uint32_t t0, int *falls, int *rises, debounce_t *db)
{
    *falls = *rises = 0;
    for (int i = 0; i < n; i++) {
        db_event_t e = debounce_update(db, lv[i], t0 + (uint32_t)i);
        if (e == DB_FALL) (*falls)++;
        if (e == DB_RISE) (*rises)++;
    }
}

int main(void)
{
    debounce_t db; int f, r;
    uint8_t buf[400];

    /* 1. Mức ổn định 1 không sinh sự kiện */
    debounce_init(&db, 1, 0);
    for (int i = 0; i < 100; i++) buf[i] = 1;
    run(buf, 100, 0, &f, &r, &db);
    CHECK(f == 0 && r == 0 && db.stable == 1, "idle high");

    /* 2. Nhấn có dội 5 ms (1010101...), rồi giữ 0 trong 100 ms, nhả có dội, giữ 1 */
    int k = 0;
    for (int i = 0; i < 5; i++) buf[k++] = (uint8_t)(i & 1);      /* 0 1 0 1 0 */
    for (int i = 0; i < 100; i++) buf[k++] = 0;
    for (int i = 0; i < 6; i++) buf[k++] = (uint8_t)((i & 1) ^ 1); /* 1 0 1 0 1 0 */
    for (int i = 0; i < 100; i++) buf[k++] = 1;
    debounce_init(&db, 1, 1000);
    run(buf, k, 1000, &f, &r, &db);
    CHECK(f == 1, "bounce press -> 1 fall");
    CHECK(r == 1, "bounce release -> 1 rise");
    CHECK(db.stable == 1, "end released");

    /* 3. Xung nhiễu ngắn hơn 20 ms bị bỏ qua */
    k = 0;
    for (int i = 0; i < 19; i++) buf[k++] = 0;
    for (int i = 0; i < 50; i++) buf[k++] = 1;
    debounce_init(&db, 1, 0);
    run(buf, k, 0, &f, &r, &db);
    CHECK(f == 0 && r == 0, "19 ms glitch ignored");

    /* 4. Đúng 20 ms ổn định thì chấp nhận: sự kiện ở mẫu thứ 21 (t = t_change + 20) */
    debounce_init(&db, 1, 0);
    int at = -1;
    for (int i = 0; i < 40; i++) {
        if (debounce_update(&db, 0, (uint32_t)i) == DB_FALL && at < 0) at = i;
    }
    /* t = 0: mức thô đổi (t_change = 0); t = 20: đủ 20 ms */
    CHECK(at == 20, "accept after exactly DEBOUNCE_MS");

    /* 5. Tràn bộ đếm ms (0xFFFFFFF0 -> 0x10) vẫn đúng */
    debounce_init(&db, 1, 0xFFFFFFF0u);
    at = -1;
    for (uint32_t i = 0; i < 40; i++) {
        if (debounce_update(&db, 0, 0xFFFFFFF0u + i) == DB_FALL && at < 0) at = (int)i;
    }
    CHECK(at == 20, "wraparound");

    /* 6. Giá trị raw khác 0/1 (IDR & mask) được chuẩn hóa */
    debounce_init(&db, 1, 0);
    for (uint32_t i = 0; i < 30; i++) (void)debounce_update(&db, 0, i);
    for (uint32_t i = 30; i < 60; i++) (void)debounce_update(&db, 0x80, i);
    CHECK(db.stable == 1, "raw 0x80 treated as 1");

    /* 7. Dội kéo dài liên tục (mỗi 1 ms đổi mức) không bao giờ sinh sự kiện */
    debounce_init(&db, 1, 0);
    f = r = 0;
    for (uint32_t i = 0; i < 300; i++) {
        db_event_t e = debounce_update(&db, (uint8_t)(i & 1), i);
        if (e == DB_FALL) f++;
        if (e == DB_RISE) r++;
    }
    CHECK(f == 0 && r == 0, "continuous chatter ignored");

    /* 8. 10 lần nhấn sạch -> 10 fall, 10 rise */
    debounce_init(&db, 1, 0);
    f = r = 0; uint32_t t = 0;
    for (int p = 0; p < 10; p++) {
        for (int i = 0; i < 50; i++, t++) { db_event_t e = debounce_update(&db, 0, t); f += (e == DB_FALL); r += (e == DB_RISE); }
        for (int i = 0; i < 50; i++, t++) { db_event_t e = debounce_update(&db, 1, t); f += (e == DB_FALL); r += (e == DB_RISE); }
    }
    CHECK(f == 10 && r == 10, "10 clean presses");

    /* 9. Đọc nhiều lần trong cùng 1 ms (vòng lặp nhanh hơn SysTick) */
    debounce_init(&db, 1, 0);
    f = 0;
    for (uint32_t ms = 0; ms < 60; ms++)
        for (int rep = 0; rep < 1000; rep++)
            f += (debounce_update(&db, ms < 30 ? 0 : 1, ms) == DB_FALL);
    CHECK(f == 1 && db.stable == 1, "many reads per ms");

    printf("%d/%d passed\n", pass, pass + fail);
    return fail ? 1 : 0;
}
