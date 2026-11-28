/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI's ENV CC and LFO CC (midi_out.c tm_mod_tick): the envelope rises with a note, holds at SUSTAIN and
 * falls once it ends, out as its CC from BASE by DEPTH; the LFO swings its CC around BASE; DEPTH 0 sends nothing;
 * no CC faster than every 12 blocks; an all-notes-off lets the envelope go. Built as Tessera MIDI by
 * tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>
static int lo[128], hi[128], cnt[128], last[128], minstep;
static uint32_t blk, at_last[128];
static void reset(void)
{
    for (int i = 0; i < 128; i++) lo[i] = 999, hi[i] = -1, cnt[i] = 0, last[i] = -1, at_last[i] = 0;
    minstep = 1 << 30;
}
static void drain(void)
{
    for (; mo_r != mo_w; mo_r++) {
        uint32_t e = midi_out_q[mo_r % MQ], st = (e >> 8) & 0xFFu, cc = (e >> 16) & 127u, v = e >> 24;
        if ((st & 0xF0u) != 0xB0u) continue;
        if (cnt[cc] && (int)(blk - at_last[cc]) < minstep) minstep = (int)(blk - at_last[cc]);
        at_last[cc] = blk;
        cnt[cc]++; last[cc] = (int)v;
        if ((int)v < lo[cc]) lo[cc] = (int)v;
        if ((int)v > hi[cc]) hi[cc] = (int)v;
    }
}
static void run(uint32_t blocks) { while (blocks--) { tm_mod_tick(32u); blk++; drain(); } }
static int mfails;
static void mcheck(const char *what, int ok) { printf("%-60s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }
int main(void)
{
    track_t *t = &trk[0];
    ui_power_on();
    usb.config = 1;
    settings_midi_out = MIDI_OUT_ALL;
    drain();
    reset();
    run(200);
    mcheck("DEPTH 0 (the default): no CC", !cnt[74] && !cnt[1]);
    /* ENV: ATK short, DEC short, SUS half, REL short; DEPTH 63 from 0 */
    t->p[P_ATK] = 0; t->p[P_DEC] = 70; t->p[P_SUS] = 64; t->p[P_REL] = 20;
    t->p[P_ECC] = 74; t->p[P_EDEPTH] = 63; t->p[P_EBASE] = 0;
    trk_note_on(t, 60, 100);
    run(400);
    mcheck("ENV CC 74: up to the top", hi[74] >= 124);
    mcheck("... and down to SUSTAIN, held", last[74] >= 60 && last[74] <= 66);
    mcheck("the mods' ENV follows it", t->m_env > 15000 && t->m_env < 17000);
    trk_note_off(t, 60);
    run(400);
    mcheck("note ended: back to BASE", last[74] == 0 && t->m_env == 0);
    mcheck("no CC closer than 12 blocks (but the top of an attack)", minstep >= 1);
    /* a negative DEPTH from 100: it dips */
    reset();
    t->p[P_EDEPTH] = -25; t->p[P_EBASE] = 100;
    trk_note_on(t, 62, 100);
    run(300);
    mcheck("DEPTH -25 from BASE 100: down to 50", lo[74] >= 49 && lo[74] <= 54 && hi[74] <= 100);
    trk_all_off(t);
    run(400);
    mcheck("all notes off: the envelope lets go", last[74] == 100 && t->m_env == 0);
    t->p[P_EDEPTH] = 0;
    /* LFO: DEPTH 32 around 64, a fast rate */
    reset();
    t->p[P_LCC] = 1; t->p[P_LDEPTH] = 32; t->p[P_LBASE] = 64; t->p[P_LRATE] = 110; t->p[P_LFADE] = 0;
    t->lfo_fade = 32767;
    run(3000);
    mcheck("LFO CC 1, DEPTH 32: 32 either side of 64, no closer than 12 blocks", cnt[1] > 10 && lo[1] <= 36 && hi[1] >= 92 && lo[1] >= 30 && hi[1] <= 98 && minstep >= 12);
    mcheck("ENV CC quiet at DEPTH 0", !cnt[74]);
    /* the track's MIDI OUT OFF, or muted: nothing */
    reset();
    t->p[P_MUTE] = 1;
    run(500);
    mcheck("muted: no CC", !cnt[1]);
    t->p[P_MUTE] = 0;
    settings_midi_out = MIDI_OUT_OFF;
    run(500);
    mcheck("MIDI OUT OFF: no CC", !cnt[1]);
    /* the mods' VEL and KEY follow the notes (sources a cord reads) */
    settings_midi_out = MIDI_OUT_ALL;
    trk_note_on(t, 72, 99);
    mcheck("a note sets the mods' VEL and KEY", t->m_vel == 99 && t->m_key == 72);
    trk_note_off(t, 72);
    /* MODS offers only what acts over MIDI: no voice, render, LFO DEPTH, send effect or GLIDE destination */
    {
        cord_t *c = (cord_t *)cord_get(0);
        cu_walk_t w;
        uint32_t i, n, bad = 0, gen = 0, cc = 0;
        c->dst = CD_OFF;
        n = cu_ndst(c);
        for (i = 1; i < n; i++) {
            const cord_dest_t *d;
            cu_walk(c, i, &w);
            d = cord_dest(w.code);
            if (d->kind == CK_VOICE || d->kind == CK_BLOCK || d->kind == CK_DEPTH || d->kind == CK_GLOBAL || w.code == CD_GLIDE)
                bad++;
            gen += w.code == CD_VELO; cc += w.code == CD_CC;
        }
        mcheck("MODS: no sound destinations listed", !bad);
        mcheck("MODS: VELO and CC still listed", gen && cc);
        c->dst = CD_CUTOFF; c->dst_trk = (uint8_t)(song.sel % NTRK);
        cu_walk(c, 0u, &w);
        mcheck("a saved mod onto CUTOFF still shows where it goes", w.at != 0u);
        c->dst = CD_OFF;
    }
    {                                                   /* FX's four pages: FX's alone, never GLO's (ui.c tm_fx_at) */
        uint8_t pg[64];
        uint32_t n = fam_pages(FAM_GLO, pg), k, bad = 0;
        for (k = 0; k < n; k++) bad += tm_fx_at(pg[k]) != 0;
        for (k = 0; k < 40u; k++) { open_family(FAM_GLO); bad += tm_fx_at(ui.page) != 0; }
        mcheck("GLO never reaches ECHO, FEEL, MIDI OUT, MIDI IN", !bad);
        open_family(FAM_FX);
        for (k = 0; k < 8u; k++) { page_scroll(1); bad += !tm_fx_at(ui.page); }
        mcheck("SELECT on FX's pages stays in the four", !bad);
    }
    printf(mfails ? "tm_mod_test FAILED\n" : "tm_mod_test passed\n");
    return mfails != 0;
}
