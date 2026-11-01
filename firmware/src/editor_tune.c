/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* User tunings (tune.h), command 80: the web editor reads a Scala file, folds and sorts its degrees, and sends the
 * table; the device keeps it in RAM as the project's (saved with it, as any edit: autosave or SAVE). Request: op, then
 * its arguments; reply: op, rc, then its data. rc 0 done, 1 bad arguments (or an empty slot to GET). Pitches are
 * hundredths of a cent, u32 (5 x 7 bits, LSB first, as the backup's).
 *   op 0 list:   -                                  -> slots, max degrees, TUNING, per slot: count (0 empty), name, title
 *   op 1 get:    slot                               -> slot, count, name, title, count x u32 (degree 1..count, the period last)
 *   op 2 put:    slot, count, name, title, count x u32 -> slot
 *   op 3 clear:  slot                               -> slot (TUNING on it: OFF)
 *   op 4 select: TUNING (0 OFF, 1..slots)           -> TUNING */
#define ED_TUNE 80u
/* a 0-terminated string of at most n - 1 bytes at a[*at] (na in all) into s (n, 0-padded): 1 read */
static int ed_tune_str(const uint8_t *a, uint32_t na, uint32_t *at, char *s, uint32_t n)
{
    uint32_t i = 0;
    memset(s, 0, n);
    while (*at < na && a[*at]) {
        if (i + 1u >= n) return 0;
        s[i++] = (char)a[(*at)++];
    }
    if (*at >= na) return 0;
    (*at)++;
    return 1;
}
static int ed_tune_handle(uint32_t cmd, const uint8_t *a, uint32_t na)
{
    uint32_t op = na ? a[0] : 127u, k = na > 1u ? a[1] : 127u, rc = 0, i;
    if (cmd != ED_TUNE) return 0;
    if (op == 0u) rc = na != 1u;
    else if (op == 1u) rc = na != 2u || !tune_used(k);
    else if (op == 3u) rc = na != 2u || k >= TUNE_SLOTS;
    else if (op == 4u) rc = na != 2u || k > TUNE_SLOTS;
    else if (op == 2u) {
        int32_t pitch[TUNE_DEGREES + 1u];
        char name[TUNE_NAME], title[TUNE_TITLE];
        uint32_t n = na > 2u ? a[2] : 0u, at = 3;
        rc = k >= TUNE_SLOTS || !n || n > TUNE_DEGREES || !ed_tune_str(a, na, &at, name, sizeof name) ||
             !ed_tune_str(a, na, &at, title, sizeof title) || at + 5u * n != na;
        for (i = 1; !rc && i <= n; i++) {
            if (a[at + 5u * (i - 1u) + 4u] > 15u) rc = 1;
            pitch[i] = (int32_t)ed_bk_r32(a + at + 5u * (i - 1u));
        }
        pitch[0] = 0;
        if (!rc) rc = !tune_put(k, name, title, n, pitch);
    } else
        rc = 1;
    if (!rc && op == 3u) {
        tune_clear(k);
        if (tune_sel == k + 1u) tune_sel = 0;
    }
    if (!rc && op == 4u) tune_sel = (uint8_t)k;
    if (!rc && op != 0u && op != 1u) ui.force = 1;
    ed_b(op); ed_b(rc);
    if (rc) return 1;
    if (op == 0u) {
        ed_b(TUNE_SLOTS); ed_b(TUNE_DEGREES); ed_b(tune_sel);
        for (i = 0; i < TUNE_SLOTS; i++) {
            ed_b(tune_micro[i].count); ed_str(tune_name(i), TUNE_NAME); ed_str(tune_title(i), TUNE_TITLE);
        }
    } else if (op == 4u) {
        ed_b(tune_sel);
    } else {
        ed_b(k);
        if (op == 1u) {
            ed_b(tune_micro[k].count); ed_str(tune_name(k), TUNE_NAME); ed_str(tune_title(k), TUNE_TITLE);
            for (i = 1; i <= tune_micro[k].count; i++) ed_bk_u32((uint32_t)tune_slot[k].pitch[i]);
        }
    }
    return 1;
}
