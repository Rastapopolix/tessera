/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* USER scales (uscale.h): a 12-note mask picked on the keys on SCL > SCALES drives QNT and a generator as a built-in
 * mask does; the settings record keeps the four; a project that plays one carries them (save, load, a damaged part
 * refused) and a record from a firmware without the part, or one that does not know it, plays SCL underneath */
#include <assert.h>
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void verify(const char *what, int ok) { bad += check(what, ok); }
#define NEAPOL 0xAABu                                   /* C Db Eb F G A B: no built-in mask */

static track_t *setup(void)
{
    ui_power_on();
    tune_clear_all();
    for (uint32_t k = 0; k < USCALE_SLOTS; k++) uscale_mask[k] = USCALE_DEF;
    uscale_sel = 0;
    TSEL->p[P_SCALE] = 1; TSEL->p[P_QUANT] = Q_SNAP; TSEL->p[P_ROOT] = TSEL->p[P_TRANS] = 0;
    scale_share(TSEL);
    song.octave = 0;
    return TSEL;
}
static int built_in(uint32_t m)
{
    for (uint32_t i = 0; i < SCALE_LEGACY; i++) if (SCALE_MASK[i] == m) return 1;
    return 0;
}

/* SCALES, the USER family: USER1..USER4 always there; USER1 picked; the keys D# and C# in, D and E out; C stays */
static void keys(void)
{
    track_t *t = setup();
    go_page(GR_SCALE_PICKER);
    ui.scale_family = SCALE_FAM_USER;
    verify("USER: the four USER scales, nothing sent", scale_picker_count() == USCALE_SLOTS &&
           !strcmp(scale_pick_name(scale_picker_at(0)), "USER1") && scale_pick_degrees(USCALE_ID) == 7u &&
           !scale_pick_nonoct(USCALE_ID));
    ui.scale_family = 0;
    verify("ALL: the catalogue only", scale_picker_count() == SCALE_TOTAL);
    ui.scale_family = SCALE_FAM_USER;
    scale_picker_edit(0, 0);                            /* (SCL's MAJ not in USER: its first, USER1) */
    verify("USER1 picked: the project's, SCL kept, TUNING OFF", uscale_sel == 1u && tune_sel == 0u && t->p[P_SCALE] == 1 &&
           scale_picker_selected() == USCALE_ID && scale_mask(t) == USCALE_DEF && micro_scale(t) == 0);
    tap_key(8); tap_key(10); tap_key(9); tap_key(11);   /* C# D# in, D E out (keys 53 + k) */
    tap_key(7);                                         /* C: the root stays */
    verify("the keys made it: C Db Eb F G A B, a mask of its own", uscale_mask[0] == NEAPOL && !built_in(NEAPOL) &&
           scale_mask(t) == NEAPOL && scale_count(t) == 7u && uscale_mask[1] == USCALE_DEF);
    char val[8]; const char *unit = "";
    param_format(&TP[P_SCALE], t->p[P_SCALE], val, &unit);
    verify("SCL shows USER1", !strcmp(val, "USER1"));
    ui.page = (uint8_t)page_titled("SCL"); page_entered();
    tap_key(8);
    verify("off SCALES the keys only play", uscale_mask[0] == NEAPOL);

    /* QNT SNAP: every key rounded down into the USER scale, the root moving it */
    int snap = 1;
    for (uint32_t k = 0; k < 27u; k++) {
        uint32_t n = kb_map(t, k);
        snap &= n <= 53u + k && (NEAPOL >> (n % 12u)) & 1u;
    }
    snap &= kb_map(t, 9) == 61u && kb_map(t, 11) == 63u;    /* D -> Db, E -> Eb */
    t->p[P_ROOT] = 2;                                       /* in D: D Eb F G A B C# */
    snap &= kb_map(t, 9) == 62u && kb_map(t, 10) == 63u && kb_map(t, 11) == 63u && kb_map(t, 12) == 65u;
    t->p[P_ROOT] = 0;
    verify("QNT SNAP rounds every key into it; ROOT moves it", snap);

    /* the others take it back */
    scale_picker_select(5);                             /* a catalogue scale */
    verify("a catalogue scale picked: USER off", uscale_sel == 0u && scale_mask(t) == SCALE_MASK[5]);
    uscale_sel = 1;
    tessera_param_set(0, TX_TUNE, 2);
    verify("TUNING to a slot: USER off", uscale_sel == 0u);
}

/* a generator (EUCLID, track 2) in USER1: every note in the mask, and the notes MAJ would not play among them */
#define LOG_MAX 4096u
static uint32_t log_pkt[LOG_MAX], log_n;
static void blocks(uint32_t n)
{
    while (n--) {
        events_block(CTL);
        while (mo_r != mo_w) {
            if (log_n < LOG_MAX) log_pkt[log_n++] = midi_out_q[mo_r % MQ];
            mo_r++;
        }
    }
}
static uint32_t gen_pcs(uint32_t sel)                   /* bit p: a note of pitch class p played on track 2 */
{
    uint32_t pcs = 0, i;
    setup();
    for (i = 0; i < NTRK; i++) { memset(trk[i].step, 0, sizeof trk[i].step); trk[i].p[P_QUANT] = Q_SNAP; }
    tessera_reset();
    cord_clear();
    song.g[G_BPM] = 120;
    usb.config = 1;
    mi_r = mi_w = 0; mo_r = mo_w = 0;
    settings_midi_out = MIDI_OUT_ALL; settings_clock_out = 0; settings_drumch = 0;
    song.sel = 1;
    tessera_knob(0, TESS_ENG_TURING);
    trk[1].p[P_SCALE] = 1;
    scale_share(&trk[1]);
    tessera.seed[1] = 7;
    uscale_mask[0] = NEAPOL;
    uscale_sel = (uint8_t)sel;
    blocks(1);
    log_n = 0;
    transport_req = 1;
    blocks(16u * (beat_samples() / CTL));
    transport_req = 2;
    blocks(2);
    for (i = 0; i < log_n; i++)
        if (((log_pkt[i] >> 8) & 0xFFu) == 0x91u && (log_pkt[i] >> 24))
            pcs |= 1u << ((log_pkt[i] >> 16) & 0x7Fu) % 12u;
    return pcs;
}
static void generator(void)
{
    uint32_t maj = gen_pcs(0), user = gen_pcs(1);
    verify("TURING in MAJ: MAJ's notes", maj && !(maj & ~USCALE_DEF));
    verify("TURING in USER1: only its notes, Db or Eb among them", user && !(user & ~NEAPOL) && (user & 0x00Au));
}

/* the settings record keeps the four; a slot never set (0) or damaged reads MAJ */
static void record(void)
{
    persist_t p;
    setup();
    uscale_mask[0] = NEAPOL; uscale_mask[2] = 0x001u;
    memset(&p, 0, sizeof p);
    settings_export(&p);
    const uint8_t *u = p.favorites.factory[8] + 8;
    verify("saved in row 8 bytes 8..15, a slot as MAJ 0", u[0] == (NEAPOL & 0xFFu) && u[1] == NEAPOL >> 8 && !u[2] && !u[3] &&
           u[4] == 1u && !u[5] && !u[6] && !u[7] && p.favorites.factory[8][16] == favorites.factory[8][16]);
    for (uint32_t k = 0; k < USCALE_SLOTS; k++) uscale_mask[k] = 0x111u;
    p.favorites.factory[8][14] = 0x10;                  /* (slot 4: above 12 bits) */
    verify("loaded back", settings_import(&p, (int)sizeof p) && uscale_mask[0] == NEAPOL && uscale_mask[1] == USCALE_DEF &&
           uscale_mask[2] == 1u && uscale_mask[3] == USCALE_DEF);
    p.favorites.factory[8][8] = 0xAA;                   /* (slot 1 without its root) */
    (void)settings_import(&p, (int)sizeof p);
    verify("a mask without the root: MAJ", uscale_mask[0] == USCALE_DEF);
}

/* the project: saved with the USER scale it plays and all four; a record of before (no part) or a firmware that does
 * not know the part (its id unknown: skipped) plays SCL underneath and keeps the device's USER scales */
static void project(void)
{
    track_t *t = setup();
    uscale_mask[0] = NEAPOL; uscale_mask[3] = 0x91u;
    uscale_sel = 1;
    stop_transport();
    verify("a project playing USER1 saves", !project_save(1));
    uint8_t *raw = proj_bank_slot[1];
    uint32_t len = tsp_len(raw);
    verify("its USCALE part 39: USER1, then the four", len && tsp_valid(raw, len) && tsp_seen.at[TSP_USCALE] &&
           tsp_seen.len[TSP_USCALE] == 9u && raw[tsp_seen.at[TSP_USCALE]] == 1u);
    static uint8_t rec[TSP_MAX], old[TSP_MAX];          /* (the record as saved: slot 1 may be saved over) */
    uint32_t at = tsp_seen.at[TSP_USCALE];
    memcpy(rec, raw, len);
    uscale_sel = 0;
    stop_transport();
    verify("a project in SCL saves without it", !project_save(2) && tsp_valid(proj_bank_slot[2], tsp_len(proj_bank_slot[2])) &&
           !tsp_seen.at[TSP_USCALE]);
    for (uint32_t k = 0; k < USCALE_SLOTS; k++) uscale_mask[k] = USCALE_DEF;     /* (another FM-1's) */
    t->p[P_SCALE] = 3;
    memcpy(raw, rec, len);
    project_load(1);
    verify("loaded on another FM-1: USER1 plays its mask, the four with it, SCL under it", uscale_sel == 1u &&
           uscale_mask[0] == NEAPOL && uscale_mask[3] == 0x91u && uscale_mask[1] == USCALE_DEF && TSEL->p[P_SCALE] == 1 &&
           scale_mask(TSEL) == NEAPOL);
    project_load(2);
    verify("a project in SCL: USER off, the device's USER scales kept", uscale_sel == 0u && uscale_mask[0] == NEAPOL &&
           scale_mask(TSEL) == SCALE_MASK[1]);

    memcpy(old, rec, len);                              /* a firmware without part 39: its id unknown, skipped */
    old[at - 4u] = 200;
    tsp_resum(old);
    verify("a firmware that does not know the part reads the record", tsp_valid(old, len) && !tsp_seen.at[TSP_USCALE]);
    memcpy(raw, old, len);
    uscale_sel = 2;
    project_load(1);
    verify("and plays SCL's scale, MAJ (the scale saved under USER1)", uscale_sel == 0u && scale_mask(TSEL) == SCALE_MASK[1]);

    memcpy(old, rec, len);                              /* a damaged part: the record refused */
    old[at + 1u] = 0x10;                                /* (USER1 without its root) */
    tsp_resum(old);
    verify("a USER scale without its root: refused", !tsp_valid(old, len));
    memcpy(old, rec, len);
    old[at] = 5;
    tsp_resum(old);
    verify("a USER scale past 4: refused", !tsp_valid(old, len));
}

int main(void)
{
    keys();
    generator();
    record();
    project();
    puts(bad ? "uscale: FAILED" : "uscale: USER scales ok");
    return bad != 0;
}
