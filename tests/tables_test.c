/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The readouts computed instead of stored (build/gen/melodee_tables.h, tools/gen_tables.py exp_steps): every value
 * equals the table it replaced (kept for this test only, MELODEE_TABLE_REFS). */
#define MELODEE_TABLE_REFS 1
#include <stdio.h>
#include "melodee_tables.h"

int main(void)
{
    uint32_t v, bad = 0;
    for (v = 0; v < 128u; v++) {
        if (time_ms_x10(v) != TIME_MS_X10[v] || lfo_hz_x100(v) != LFO_HZ_X100[v] || cutoff_hz(v) != CUTOFF_HZ[v] ||
            level_db_x10(v) != LEVEL_DB_X10[v]) {
            printf("tables: %u: %u %u %u %d, stored %u %u %u %d\n", v, time_ms_x10(v), lfo_hz_x100(v), cutoff_hz(v),
                   level_db_x10(v), TIME_MS_X10[v], LFO_HZ_X100[v], CUTOFF_HZ[v], LEVEL_DB_X10[v]);
            bad++;
        }
        if (time_ms_x10(v + 128u) != time_ms_x10(v))      /* the index wraps as the table's v & 127 did */
            bad++;
    }
    printf("tables: TIME / LFO Hz / CUTOFF / dB readouts computed as stored: %s\n", bad ? "FAIL" : "ok");
    return bad != 0;
}
