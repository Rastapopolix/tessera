/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
#pragma once
/* User tunings: Scala (.scl) files the web editor reads and sends (editor_tune.c), TUNE_SLOTS tables in the micro
 * path's format (microtonal.h: each degree above the root in hundredths of a cent, the last entry the period). They
 * are the project's (its TUNE part, project_tsp.c), so a project plays the same on any FM-1. TUNE (TX_TUNE) picks one
 * for the whole project, as SCL is the song's; 0 OFF plays SCL's scale. A tuning stands for SCL wherever the micro
 * path reads it (micro_scale): the keys, the generators, QNT, chords and recordings play its degrees. */
#define TUNE_SLOTS 4u
#define TUNE_DEGREES 64u                            /* degrees a period at most (the catalogue's largest: 53EDO) */
#define TUNE_NAME 8u                                /* a slot's short name, 7 ASCII characters, 0-padded */
#define TUNE_TITLE 24u                              /* its title (the file's description), 23 characters, 0-padded */
#define TUNE_PERIOD_MAX 480000                      /* four octaves, in hundredths of a cent */
typedef struct {
    char name[TUNE_NAME];
    char title[TUNE_TITLE];
    int32_t pitch[TUNE_DEGREES + 1u];               /* pitch[0] 0, pitch[count] the period */
} tune_slot_t;
static tune_slot_t tune_slot[TUNE_SLOTS] __attribute__((section(".pool")));   /* (static RAM is nearly full) */
static micro_scale_t tune_micro[TUNE_SLOTS] = {     /* count 0: the slot is empty */
    {0, tune_slot[0].pitch}, {0, tune_slot[1].pitch}, {0, tune_slot[2].pitch}, {0, tune_slot[3].pitch}};
_Static_assert(TUNE_SLOTS == 4u, "tune_micro's initialiser");
#define SCALE_PICK_TOTAL (SCALE_TOTAL + TUNE_SLOTS)   /* SCL's picker: the catalogue, then the slots (scale_picker.c) */
#define SCALE_FAM_USER (SCALE_FAMILIES + 2u)        /* its families: 0 ALL, 1..SCALE_FAMILIES, FAV, USER */
static uint8_t tune_sel;                            /* TX_TUNE: 0 OFF, 1..TUNE_SLOTS that slot (the project's) */

static int tune_used(uint32_t k) { return k < TUNE_SLOTS && tune_micro[k].count != 0u; }
/* the tuning TUNE plays, 0: none (OFF, or its slot empty) */
static const micro_scale_t *tune_scale(void)
{
    return tune_sel && tune_used(tune_sel - 1u) ? &tune_micro[tune_sel - 1u] : 0;
}
static const char *tune_name(uint32_t k) { return tune_used(k) ? tune_slot[k].name : ""; }
static const char *tune_title(uint32_t k) { return tune_used(k) ? tune_slot[k].title : ""; }

/* count degrees, pitch[0..count] (hundredths of a cent): 1 a table the micro path can play. 0 at the root, every
 * degree above the one before, the period above the last degree and at most TUNE_PERIOD_MAX (the editor folds a
 * file's degrees into the period and sorts them before it sends them) */
static int tune_check(uint32_t count, const int32_t *pitch)
{
    if (count < 1u || count > TUNE_DEGREES || pitch[0] != 0 || pitch[count] > TUNE_PERIOD_MAX) return 0;
    for (uint32_t i = 1; i <= count; i++)
        if (pitch[i] <= pitch[i - 1u]) return 0;
    return 1;
}
/* a text the lists can show: printable ASCII, upper case for the name; the rest 0 */
static int tune_text_ok(const char *s, uint32_t n)
{
    uint32_t i = 0;
    while (i < n && s[i]) {
        if (s[i] < 32 || s[i] > 126) return 0;
        i++;
    }
    while (i < n)
        if (s[i++]) return 0;
    return 1;
}
/* a whole slot's worth (name, title: 0-padded; count, pitch[0..count]) that tune_put takes: 1 */
static int tune_ok(const char *name, const char *title, uint32_t count, const int32_t *pitch)
{
    return tune_check(count, pitch) && name[0] && tune_text_ok(name, TUNE_NAME - 1u) && !name[TUNE_NAME - 1u] &&
           tune_text_ok(title, TUNE_TITLE - 1u) && !title[TUNE_TITLE - 1u];
}
/* slot k becomes the tuning; 1 done, 0 not a valid one (the slot kept). Empty first, so a voice that reads it
 * meanwhile meets an empty slot (SCL's scale), never half a table */
static int tune_put(uint32_t k, const char *name, const char *title, uint32_t count, const int32_t *pitch)
{
    if (k >= TUNE_SLOTS || !tune_ok(name, title, count, pitch)) return 0;
    tune_micro[k].count = 0;
    for (uint32_t i = 0; i < TUNE_NAME; i++)
        tune_slot[k].name[i] = name[i] >= 'a' && name[i] <= 'z' ? (char)(name[i] - 32) : name[i];
    memcpy(tune_slot[k].title, title, TUNE_TITLE);
    memcpy(tune_slot[k].pitch, pitch, (count + 1u) * sizeof pitch[0]);
    tune_micro[k].count = (uint8_t)count;
    return 1;
}
static void tune_clear(uint32_t k)
{
    if (k >= TUNE_SLOTS) return;
    tune_micro[k].count = 0;
    memset(&tune_slot[k], 0, sizeof tune_slot[k]);
}
static void tune_clear_all(void)
{
    for (uint32_t k = 0; k < TUNE_SLOTS; k++) tune_clear(k);
    tune_sel = 0;
}
