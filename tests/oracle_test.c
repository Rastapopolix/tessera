/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* ORACLE (eng_oracle.c, the Prophet engine back as its own, 2026-10-29), on the host:
 * 1. its sounds: a patch per factory preset, every byte in range, a category each (browse.c)
 * 2. each sound plays: a chord held and let go, sound and nothing but finite samples, quiet 8 s after the release
 * 3. the pages: a view turned writes the patch and reads back; out-of-range patches refused
 * 4. a project: each track's patch saved and loaded byte for byte (TSP1 part 21), a changed patch too
 *   build/host/oracle_test */
#define main hostsim_main
#include "hostsim.c"
#undef main

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
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    for (uint32_t i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0x856014u; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"

static int check(const char *what, int ok)
{
    printf("oracle: %-74s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

static void power_on(void)
{
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    memset(proj_bank_len, 0, sizeof proj_bank_len);
    up_cache_reset(); native_cache_reset();
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    panel = PANEL_DEFAULT;
    host_tracks_init();
    tessera_reset();
    fm1_ms = 0;
    transport_req = 0;
    flash_ok = 1;
}

/* ------------------------------------------------------------ 1. sounds --- */
static int test_sounds(void)
{
    uint8_t p[OR_N];
    uint32_t k, ok = 1, named = 1, cat = 1;
    const uint32_t n = ENG_ORACLE.npresets;
    for (k = 0; k < n; k++) {
        or_patch_init(p);
        or_sound(k, p);
        ok &= or_patch_ok(p);
        named &= ORACLE_PRESETS[k].name && ORACLE_PRESETS[k].name[0];
        cat &= preset_cat(ENGI_ORACLE, k) != CAT_OTHER || k == 0u;
    }
    return check("16..32 sounds, none of them another maker's bank", n >= 16u && n <= 32u && NELEM(OR_SOUNDS) == n) +
           check("each sound's patch in range (switches 0/1, KEY 0..2, the rest 0..127)", ok) +
           check("each sound named, a browser category each (INIT: OTHER)", named && cat) +
           check("the engine offered (ENGINE_ORDER, eng_ok) at its own ID 19", eng_ok(ENGI_ORACLE) && ENGI_ORACLE == 19u);
}

/* ------------------------------------------------------------ 2. playing --- */
static int play(uint32_t k, double *peak, double *tail)
{
    static const uint8_t CH[3] = {48, 55, 60};
    const uint32_t B = FS / 10u / CTL * CTL, frames = 100u * B;   /* 10 s: 1.5 s held, then let go */
    track_t *t = &trk[0];
    uint32_t f, i;
    int finite = 1;
    host_tracks_init();
    host_preset(t, ENGI_ORACLE, k);
    t->p[P_LEVEL] = 100;
    *peak = *tail = 0;
    for (f = 0; f < frames; f += CTL) {
        int32_t o[2 * CTL];
        if (f == B)
            for (i = 0; i < 3u; i++) trk_note_on(t, CH[i], 100);
        if (f == 16u * B)
            for (i = 0; i < 3u; i++) trk_note_off(t, CH[i]);
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) {
            double a = fabs((double)o[i]);
            finite &= o[i] == o[i];
            if (f < 16u * B && a > *peak) *peak = a;
            if (f >= 95u * B && a > *tail) *tail = a;
        }
    }
    return finite;
}
static int test_play(void)
{
    uint32_t k, sound = 1, finite = 1, quiet = 1;
    double peak, tail;
    for (k = 0; k < ENG_ORACLE.npresets; k++) {
        finite &= play(k, &peak, &tail);
        sound &= peak > 1000.0;
        quiet &= tail < peak * 0.15;          /* (DRONE and WIND: releases of many seconds) */
        if (peak <= 1000.0 || tail >= peak * 0.15)
            printf("  %s: peak %.0f, 8 s after the release %.0f\n", ORACLE_PRESETS[k].name, peak, tail);
    }
    return check("every sound plays a chord (finite, heard) and fades after its release", sound && finite && quiet);
}

/* ------------------------------------------------------------ 3. pages --- */
static int test_views(void)
{
    uint8_t p[OR_N], q[OR_N];
    uint32_t v, ok = 1;
    or_patch_init(p);
    for (v = 0; v < OV_COUNT; v++) {
        const param_desc_t *d = or_view_desc(p, v);
        memcpy(q, p, OR_N);
        or_view_put(q, v, d->max);
        ok &= or_view_get(q, v) == d->max && or_patch_ok(q);
        or_view_put(q, v, d->min);
        ok &= or_view_get(q, v) == d->min && or_patch_ok(q);
    }
    memset(q, 0, OR_N);
    q[OR_SYNC] = 2;
    return check("each view turned to its ends reads back, the patch stays in range", ok) +
           check("a patch with a byte out of range is refused (project, undo)", !or_set_patch(0, q));
}

/* ------------------------------------------------------------ 4. project --- */
static int test_project(void)
{
    uint8_t want[NTRK][OR_N];
    uint32_t tr, same = 1;
    power_on();
    memset(nor, 0xFF, sizeof nor);
    for (tr = 0; tr < NTRK; tr++) {
        host_preset(&trk[tr], ENGI_ORACLE, 3u + tr);
        memcpy(want[tr], or_patch_of(&trk[tr]), OR_N);
    }
    or_view_put(or_patch_of(&trk[1]), OV_CUT, 33);   /* a turned patch, not its preset's */
    or_view_put(or_patch_of(&trk[1]), OV_WAVE_B, 5);
    memcpy(want[1], or_patch_of(&trk[1]), OR_N);
    if (project_save_as(0, "ORACLE")) return check("the project saves", 0);
    power_on();
    memset(or_patch, 0, sizeof or_patch);
    persist_boot();
    project_load(0);
    for (tr = 0; tr < NTRK; tr++)
        same &= trk[tr].eng_req == ENGI_ORACLE && !memcmp(or_patch_of(&trk[tr]), want[tr], OR_N);
    return check("a project: every track's ORACLE patch back byte for byte (a turned one too)", proj_cur == 0u && same);
}

int main(void)
{
    int bad = 0;
    bad += test_sounds();
    bad += test_play();
    bad += test_views();
    bad += test_project();
    printf(bad ? "oracle: %d FAILED\n" : "oracle: all passed\n", bad);
    return bad != 0;
}
