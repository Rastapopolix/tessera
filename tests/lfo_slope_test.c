/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* SLOPE (core.h TESS_LFO_SLOPE: the plugin, the Daisy): the track LFO as a function generator. Built twice by
 * run_tests.sh:
 *   with TESS_LFO_SLOPE=1: the SLOPE, EXP and LOG waves and their SKEW (voice.c lfo_slope), TRIG ONCE (one cycle,
 *   held at its end, EOC once, FADE running on), the sources LFO 90 and LFO X2 (cord.c), the destination LFO RST
 *   (an edge past half starts the LFO again, a held gate once), a code this build lacks refused;
 *   without (the FM-1): a project with WAVE SLOPE / EXP / LOG and TRIG ONCE loads as TRI and NOTE (project.c), the
 *   other values as saved */
static unsigned char host_samples[3][0x14000];
#define SMP_USER_XIP(k) host_samples[k]
#define main hostsim_main
#include "hostsim.c"
#undef main
/* (as engine_switch_test: the UI and storage stubs, so project.c's load runs on the host) */
static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define MELODEE_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static void panel_setup(void) {}
static uint8_t nor[0x100000], flash_ok = 1;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { memcpy(nor + off, src, n); return 0; }
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
static project_t pj, base;

static int bad;
static void check(const char *what, int ok)
{
    printf("lfo slope: %-90s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

#if TESS_LFO_SLOPE
static int32_t at(track_t *t, uint32_t wave, uint32_t skew, uint32_t ph16)
{
    t->p[P_LWAVE] = (int16_t)wave;
    t->p[P_LPHASE] = (int16_t)skew;
    return lfo_wave(t, ph16 << 16);
}
static cord_t C(uint32_t src, uint32_t st, uint32_t dst, uint32_t dt, int32_t amount, int32_t offset)
{
    cord_t c = {(uint8_t)src, (uint8_t)st, (uint8_t)dst, (uint8_t)dt, (int8_t)amount, (int8_t)offset};
    return c;
}
#endif

int main(void)
{
    host_tracks_init();
    track_t *t = &trk[1];
#if TESS_LFO_SLOPE
    uint32_t i, eoc = 0, held;
    int32_t a, b, c, prev, mono = 1;

    /* the shapes */
    check("SKEW 0: the top at the start, a straight fall to the bottom", at(t, LW_SLOPE, 0, 0) > 32700 &&
          at(t, LW_SLOPE, 0, 65535) < -32700 && at(t, LW_SLOPE, 0, 32768) > -200 && at(t, LW_SLOPE, 0, 32768) < 200);
    check("SKEW 64: a triangle, the top in the middle", at(t, LW_SLOPE, 64, 0) < -32700 &&
          at(t, LW_SLOPE, 64, 33024) > 32600 && at(t, LW_SLOPE, 64, 65535) < -32600);
    check("SKEW 127: a long rise, the top at the end", at(t, LW_SLOPE, 127, 65000) > 32000 && at(t, LW_SLOPE, 127, 0) < -32700);
    for (i = 0, prev = 99999; i < 65536u; i += 97u) {     /* SKEW 0: every wave falls all the way, never back up */
        a = at(t, LW_SLOPE + 1u, 0, i);
        if (a > prev) mono = 0;
        prev = a;
    }
    check("EXP's fall never rises again", mono);
    a = at(t, LW_SLOPE, 64, 16512), b = at(t, LW_SLOPE + 1u, 64, 16512), c = at(t, LW_SLOPE + 2u, 64, 16512);
    check("half way up: EXP under the straight slope, LOG over it", b < a - 4000 && c > a + 4000);
    check("the ends the same whatever the curve", at(t, LW_SLOPE + 1u, 64, 33024) > 32600 && at(t, LW_SLOPE + 2u, 64, 0) < -32700);
    check("a SLOPE's PHS is SKEW; SIN's still PHS", (t->p[P_LWAVE] = LW_SLOPE, str_eq(track_desc(t, P_LPHASE)->label, "SKEW")) &&
          (t->p[P_LWAVE] = 0, str_eq(track_desc(t, P_LPHASE)->label, "PHS")));
    t->p[P_LWAVE] = LW_SLOPE, t->p[P_LPHASE] = 90;
    check("a slope's cycle starts at 0, whatever its SKEW (LFO_PH0)", LFO_PH0(t) == 0u);

    /* ONCE */
    t->p[P_LWAVE] = LW_SLOPE, t->p[P_LPHASE] = 0, t->p[P_LTRIG] = LT_ONCE, t->p[P_LSYNC] = 0, t->p[P_LRATE] = 100;
    t->p[P_LPOL] = 0, t->p[P_LFADE] = 127;
    t->lfo_ph = 0, t->lfo_fade = 0, lfo_eoc = 0;
    for (i = 0; i < 200000u && t->lfo_ph != 0xFFFFFFFFu; i++) {
        track_lfo_tick(t);
        eoc += lfo_eoc_now(t);
    }
    check("ONCE: one cycle, then held at its end", t->lfo_ph == 0xFFFFFFFFu && i < 200000u);
    check("ONCE: EOC at its end", eoc == 1u && lfo_eoc_now(t));
    held = t->lfo_ph, a = t->lfo_val;
    for (i = 0, eoc = 0; i < 5000u; i++) {
        track_lfo_tick(t);
        eoc += lfo_eoc_now(t);
    }
    check("ONCE: held (its value and phase stay; EOC not again)", t->lfo_ph == held && t->lfo_val == a && eoc == 0u);
    check("ONCE: the FADE runs on while held", t->lfo_fade > 0);
    check("ONCE: the bottom at the end (SKEW 0's decay)", a < -32000);
    t->p[P_LTRIG] = 1;                                    /* FREE: wraps and runs on */
    t->lfo_ph = 0xFFFFFF00u;
    track_lfo_tick(t);
    check("FREE wraps as before", t->lfo_ph < 0x10000000u);
    check("LFO_FREE: FREE only (NOTE and ONCE start again)", LFO_FREE(t) && (t->p[P_LTRIG] = LT_ONCE, !LFO_FREE(t)) &&
          (t->p[P_LTRIG] = 0, !LFO_FREE(t)));

    /* the sources */
    t->p[P_LWAVE] = 1, t->p[P_LTRIG] = 1, t->lfo_fade = 32767, t->lfo_ph = 0x12345678u;
    check("LFO 90: the wave a quarter cycle on", cord_src(CS_LFOQ, 1) == mulq15(lfo_wave(t, 0x52345678u), 32767));
    check("LFO X2: the wave at twice the phase", cord_src(CS_LFO2X, 1) == mulq15(lfo_wave(t, 0x2468ACF0u), 32767));
    check("both bipolar, track sources", cord_bipolar(CS_LFOQ, 1) && cord_bipolar(CS_LFO2X, 1) && !CS_PROJECT(CS_LFOQ) &&
          !CS_PROJECT(CS_LFO2X));
    check("named LFO 90, LFO X2", str_eq(cord_src_name(CS_LFOQ), "LFO 90") && str_eq(cord_src_name(CS_LFO2X), "LFO X2"));

    /* LFO RST: a fixed cord at 100 % is past half from its first block */
    cord_reset();
    t->p[P_LWAVE] = LW_SLOPE, t->p[P_LTRIG] = LT_ONCE, t->lfo_ph = 0xFFFFFFFFu;
    {
        cord_t k = C(CS_OFF, 0, CD_LFO_RST, 1, 0, 100);
        check("LFO RST: a cord onto it is taken", cord_set(0, &k));
        cord_tick();
        check("LFO RST: its edge starts a held ONCE again", t->lfo_ph == 0u);
        t->lfo_ph = 0x40000000u;
        cord_tick();
        check("LFO RST: held past half, no new start", t->lfo_ph == 0x40000000u);
        k.offset = 0;
        cord_set(0, &k);
        cord_tick();
        k.offset = 100;
        cord_set(0, &k);
        cord_tick();
        check("LFO RST: under half and back past it, started again", t->lfo_ph == 0u);
        trk[2].lfo_ph = 0x30000000u;
        cord_tick();
        check("LFO RST: other tracks left alone", trk[2].lfo_ph == 0x30000000u);
        trk[1].lfo_ph = 0x40000000u;
        k.offset = 0, cord_set(0, &k), cord_tick();
        k.offset = 40, cord_set(0, &k), cord_tick();
        check("LFO RST: under half, nothing", t->lfo_ph == 0x40000000u);
    }
    {
        cord_t g = C(36u, 0, CD_PITCH, 0, 50, 0);          /* (BUS C: claimed, not built here) */
        check("a source code this build lacks is refused", !cord_set(1, &g) && !cord_src_name(36u));
    }
    cord_reset();
#else
    /* the FM-1: a project with SLOPE waves and ONCE (saved by the plugin) */
    static const int16_t W[] = {0, 1, 2, 3, 4, 5, 6, 7}, WANT[] = {0, 1, 2, 3, 4, 1, 1, 1};
    static const int16_t G[] = {0, 1, 2}, GWANT[] = {0, 1, 0};
    uint32_t k;
    project_capture(&base);                             /* (once: the project as it starts, each load from it) */
    for (k = 0; k < 8u; k++) {
        pj = base;
        pj.t[1].p[P_LWAVE] = W[k];
        pj.t[1].p[P_LTRIG] = G[k % 3u];
        pj.sum = proj_sum(&pj);                         /* (as a project saved by the plugin) */
        if (project_restore_runtime(&pj)) bad++, printf("lfo slope: project %u not restored\n", k);
        if (trk[1].p[P_LWAVE] != WANT[k] || trk[1].p[P_LTRIG] != GWANT[k % 3u]) {
            printf("lfo slope: WAVE %d TRIG %d loaded as %d %d\n", W[k], G[k % 3u], trk[1].p[P_LWAVE], trk[1].p[P_LTRIG]);
            bad++;
        }
    }
    check("the FM-1 loads SLOPE / EXP / LOG as TRI and ONCE as NOTE, the rest as saved", 1);
    check("the FM-1's WAVE ends at S&H, TRIG at FREE (no SLOPE here)", TP[P_LWAVE].max == 4 && TP[P_LTRIG].max == 1);
    (void)t;
#endif
    printf("lfo slope: %s\n", bad ? "FAILED" : "all passed");
    return bad != 0;
}
