/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* User tunings (tune.h): the slots' checks, the pitch math against the catalogue (a slot holding a built-in table plays
 * exactly as SCL does, 12-EDO as CHR), TUNING over SCL, the SCALES picker's USER family, and the project's TUNE part
 * (a save and load, a record without it, a damaged one refused) */
#include <assert.h>
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void verify(const char *what, int ok) { bad += check(what, ok); }
static uint32_t scale_id(const char *name)
{
    for (uint32_t i = 0; i < SCALE_TOTAL; i++) if (!strcmp(N_SCALE[i], name)) return i;
    assert(0); return 0;
}
/* tune_put with name and title 0-padded as the slots keep them (a longer one cut: still refused) */
static int put(uint32_t k, const char *name, const char *title, uint32_t count, const int32_t *pitch)
{
    char n[TUNE_NAME + 4u] = {0}, t[TUNE_TITLE + 16u] = {0};
    memcpy(n, name, strlen(name) < sizeof n ? strlen(name) : sizeof n);
    memcpy(t, title, strlen(title) < sizeof t ? strlen(title) : sizeof t);
    return tune_put(k, n, t, count, pitch);
}
static int put_table(uint32_t k, const char *name, const micro_scale_t *s)
{
    return put(k, name, "From the catalogue", s->count, s->pitch);
}
static track_t *setup(void)
{
    ui_power_on();
    tune_clear_all();
    TSEL->p[P_SCALE] = 0; TSEL->p[P_QUANT] = Q_ALL; TSEL->p[P_ROOT] = TSEL->p[P_TRANS] = 0;
    scale_share(TSEL);
    return TSEL;
}

static void checks(void)
{
    static const int32_t EDO12[13] = {0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000, 120000};
    int32_t p[TUNE_DEGREES + 2u];
    setup();
    verify("a 12-EDO table goes into slot 1", put(0, "edo12", "Twelve equal", 12, EDO12) && tune_used(0) &&
           !strcmp(tune_name(0), "EDO12") && !strcmp(tune_title(0), "Twelve equal"));
    p[0] = 0; p[1] = 50000; p[2] = 30000; p[3] = 120000;
    verify("unsorted degrees refused, the slot kept", !put(0, "BAD", "", 3, p) && tune_micro[0].count == 12u);
    p[0] = 0; p[1] = 50000; p[2] = 50000; p[3] = 120000;
    verify("a repeated degree refused", !put(1, "BAD", "", 3, p) && !tune_used(1));
    p[0] = 0; p[1] = 50000; p[2] = TUNE_PERIOD_MAX + 1;
    verify("a period past four octaves refused", !put(1, "BAD", "", 2, p));
    p[0] = 5; p[1] = 50000; p[2] = 120000;
    verify("a root other than 0 refused", !put(1, "BAD", "", 2, p));
    for (uint32_t i = 0; i <= TUNE_DEGREES + 1u; i++) p[i] = (int32_t)i * 1000;
    verify("64 degrees taken, 65 refused", put(1, "BIG", "", TUNE_DEGREES, p) && !put(2, "BIG", "", TUNE_DEGREES + 1u, p) &&
           !put(2, "BIG", "", 0, p));
    verify("an empty name, a long one or a control character refused",
           !put(2, "", "", 12, EDO12) && !put(2, "EIGHTCHR", "", 12, EDO12) && !put(2, "A\tB", "", 12, EDO12) &&
           !put(2, "A", "a title of more than 23 characters", 12, EDO12));
    tune_clear(1);
    verify("a cleared slot is empty", !tune_used(1) && !tune_name(1)[0]);
}

static void pitch(void)
{
    track_t *t = setup();
    int same = 1, chr = 1, over = 1;
    static const int32_t EDO12[13] = {0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000, 120000};
    for (uint32_t s = SCALE_LEGACY; s < SCALE_TOTAL; s++) {   /* every catalogue table, as a tuning, plays the same */
        const micro_scale_t *ms = &MICRO_SCALE[s - SCALE_LEGACY];
        int32_t want[128];
        uint32_t map[80];
        t->p[P_SCALE] = (int16_t)s; tune_sel = 0;
        for (int32_t n = 0; n < 128; n++) want[n] = micro_pitch(t, n);
        for (int32_t d = -40; d < 40; d++) map[d + 40] = scale_degree_map(t, d, 0, 1);
        assert(put_table(2, N_SCALE[s], ms));
        tune_sel = 3; t->p[P_SCALE] = 1;                       /* (SCL under it: MAJ, overridden) */
        same &= micro_active(t) && micro_scale(t) == &tune_micro[2] && scale_count(t) == ms->count;
        for (int32_t n = 0; n < 128; n++) same &= micro_pitch(t, n) == want[n];
        for (int32_t d = -40; d < 40; d++) same &= scale_degree_map(t, d, 0, 1) == map[d + 40];
    }
    verify("every catalogue table in a slot plays as SCL plays it (pitch, degrees)", same);

    t->p[P_SCALE] = 0; tune_sel = 0;                           /* CHR */
    uint32_t want16[128], fine[128];
    for (uint32_t n = 0; n < 128; n++) { voice_t v = {0}; voice_scale_pitch(t, &v, n); want16[n] = v.pitch16; fine[n] = v.scale_fine; }
    assert(put(0, "EDO12", "", 12, EDO12)); tune_sel = 1;
    for (uint32_t n = 0; n < 128; n++) {
        voice_t v = {0}; voice_scale_pitch(t, &v, n);
        chr &= v.pitch16 == want16[n] && v.scale_fine == fine[n];
    }
    verify("a 12-EDO tuning sounds exactly as CHR, every note", chr);
    t->p[P_ROOT] = 2; t->p[P_TRANS] = -3;
    over &= micro_pitch(t, 60) == 590000 && micro_pitch(t, 72) == 710000 && micro_pitch(t, 47) == 460000;
    t->p[P_ROOT] = t->p[P_TRANS] = 0;
    t->p[P_QUANT] = Q_OFF;
    over &= !micro_active(t);
    t->p[P_QUANT] = Q_ALL; t->eng_req = 10;
    over &= !micro_active(t);
    t->eng_req = trk[(song.sel + 1u) % NTRK].eng_req;
    tune_sel = 2;                                              /* an empty slot: SCL's own scale */
    over &= micro_scale(t) == 0 && scale_count(t) == 12u;
    verify("ROOT and TRN move it, QNT OFF and a drum track bypass it, an empty slot plays SCL", over);
}

static void picker(void)
{
    static const int32_t P[6] = {0, 23100, 47400, 71700, 95500, 120800};
    track_t *t = setup();
    ui.scale_family = SCALE_FAM_USER;
    verify("USER with nothing sent: no scales", scale_picker_count() == 0u);
    ui.scale_family = 0;
    uint32_t all = scale_picker_count();
    assert(put(3, "SLEN-2", "My slendro", 5, P));
    verify("a tuning sent: listed in ALL, after the catalogue", scale_picker_count() == all + 1u &&
           scale_picker_at(all) == SCALE_TOTAL + 3u);
    ui.scale_family = SCALE_FAM_USER;
    scale_picker_edit(1, 1);
    verify("picked in USER: TUNING is its slot, SCL kept", scale_picker_count() == 1u && tune_sel == 4u &&
           t->p[P_SCALE] == 0 && scale_picker_selected() == SCALE_TOTAL + 3u && scale_count(t) == 5u);
    char val[8]; const char *unit = "";
    param_format(&TP[P_SCALE], t->p[P_SCALE], val, &unit);
    verify("the SCL value shows the tuning's name (cut to 5, as every value)", !strcmp(val, "SLEN-") && !strcmp(scale_pick_title(SCALE_TOTAL + 3u), "My slendro") &&
           scale_pick_degrees(SCALE_TOTAL + 3u) == 5u && scale_pick_nonoct(SCALE_TOTAL + 3u) && !scale_pick_fav(SCALE_TOTAL + 3u));
    scale_picker_mark(1);
    verify("a tuning takes no star (SCL's scale not marked either)", !scale_favorite(0));
    ui.scale_family = 1;                                       /* WESTERN */
    scale_picker_edit(0, 0);
    scale_picker_edit(1, 2);
    verify("a catalogue scale picked: TUNING OFF", tune_sel == 0u && t->p[P_SCALE] != 0);
    tune_sel = 4;
    ui.scale_family = SCALE_FAM_USER;
    scale_picker_edit(0, -1);                                  /* (to FAV: nothing marked, the first of none) */
    scale_picker_edit(0, -9);                                  /* ALL */
    verify("families clamp at ALL and USER", ui.scale_family == 0u);
    scale_picker_edit(0, 99);
    verify("families clamp at USER", ui.scale_family == SCALE_FAM_USER);
    tessera_param_set(0, TX_TUNE, 9);
    verify("TUNING through its id: clamped to the slots", tessera_param_get(2, TX_TUNE) == (int32_t)TUNE_SLOTS);
}

static void project(void)
{
    static const int32_t A[4] = {0, 38631, 70196, 120000}, B[3] = {0, 70196, 190196};
    track_t *t = setup();
    assert(put(0, "TRIAD", "Just triad", 3, A) && put(3, "BP", "Tritave", 2, B));
    tune_sel = 4;
    stop_transport();
    verify("a project with tunings saves", !project_save(1));
    tune_clear_all();
    assert(put(1, "OTHER", "", 3, A));
    project_load(1);
    verify("loaded: its tunings and TUNING, nothing else", tune_sel == 4u && tune_used(0) && !tune_used(1) && !tune_used(2) &&
           tune_used(3) && !strcmp(tune_name(3), "BP") && !strcmp(tune_title(0), "Just triad") &&
           tune_micro[3].count == 2u && tune_slot[3].pitch[2] == 190196 && micro_scale(t) == &tune_micro[3]);
    uint8_t *raw = proj_bank_slot[1];
    uint32_t len = tsp_len(raw);
    verify("the record checks out, its TUNE part 25 found", len && tsp_valid(raw, len) && tsp_seen.at[TSP_TUNE] &&
           tsp_seen.len[TSP_TUNE] == 1u + TUNE_SLOTS + 2u * (TUNE_NAME + TUNE_TITLE) + 4u * (3u + 2u));
    raw[tsp_seen.at[TSP_TUNE] + 1u + 1u + TUNE_NAME + TUNE_TITLE + 4u] = 0;   /* slot 1's second degree: 0 */
    raw[tsp_seen.at[TSP_TUNE] + 1u + 1u + TUNE_NAME + TUNE_TITLE + 5u] = 0;
    raw[tsp_seen.at[TSP_TUNE] + 1u + 1u + TUNE_NAME + TUNE_TITLE + 6u] = 0;
    tsp_resum(raw);
    verify("a part with unsorted degrees: the record refused", !tsp_valid(raw, len));
    tune_clear_all();
    stop_transport();
    verify("a project without tunings saves", !project_save(2));
    assert(put(2, "STAYS?", "", 3, A)); tune_sel = 3;
    project_load(2);
    verify("loading it empties the slots, TUNING OFF", !tune_used(2) && tune_sel == 0u);
}

int main(void)
{
    checks();
    pitch();
    picker();
    project();
    puts(bad ? "tune: FAILED" : "tune: user tunings ok");
    return bad != 0;
}
