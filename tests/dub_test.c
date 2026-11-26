/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The dub moves (core.h TESS_DUB, an 8-track build's; RAM only): DROP (entries.c dub_drop_press) and THROW
 * (core.h dub_throw, fx.c dub_slots / fx_send_gain, mix.c). On a demo, T1 the ANCHOR: DROP while playing waits for the
 * bar line (the tracks to go blink: mute_waiting), then every track but T1 is out; a track muted by hand before stays
 * muted after; pressed again, the rest come back on the next bar. Stopped: at once. THROW: CHAIN mode opens the one
 * send in full; SPLIT opens the echo slot's send (none: slot 1's); THROW + REPEAT lifts the echo's REPEAT to
 * DUB_THROW_FB, never lowers it. A project load forgets both.
 *   dub_test DEMO.tsp      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_DUB
#error "dub_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static uint32_t muted(void)
{
    uint32_t m = 0, tr;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) m |= (trk[tr].p[P_MUTE] != 0) << tr;
    return m;
}
static void to_next_bar(void)
{
    uint32_t b = bar_n, i;
    for (i = 0; bar_n == b && i < 20000u; i++) tess_core_render(l, r, 0, 256);
    tess_core_render(l, r, 0, 256);                   /* (the line's tick done) */
}
static int load(const uint8_t *buf, uint32_t n)
{
    uint32_t tr;
    if (tess_core_load(buf, n)) return 0;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) trk[tr].p[P_MUTE] = 0, tessera.et_role[tr] = 0;
    tessera.et_role[0] = 5;                            /* (T1 ANCHOR) */
    tessera.et_role[4] = 1;                            /* (T5 BASS: not kept, an ANCHOR is there) */
    return 1;
}

static int32_t send_at(uint32_t tr, uint32_t step)   /* T(tr+1)'s FX SEND move on step, -1 none */
{
    for (uint32_t i = 0; i < motion.count; i++)
        if (motion.event[i].place == (tr << 6 | step) && motion.event[i].param == P_FXSEND) return motion.event[i].value;
    return -1;
}
int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint32_t all = (1u << TESSERA_TRACKS) - 1u, rest = all & ~1u, b;
    if (f) fclose(f);
    tess_core_boot();
    if (!n || !load(buf, (uint32_t)n)) {
        printf("dub_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }

    /* DROP while playing: on the bar line, and back on the next one */
    CHECK(dub_drop_keep() == 1u, "DROP keeps the ANCHOR only (%02x)", (unsigned)dub_drop_keep());
    trk[6].p[P_MUTE] = 1;                              /* (T7 muted by hand before) */
    tess_core_transport(1);
    to_next_bar();
    for (b = bar_n; bar_n == b && beat_n < 2u;) tess_core_render(l, r, 0, 256);   /* (mid-bar) */
    CHECK(dub_drop_press() == 1, "DROP pressed mid-bar: waiting");
    CHECK(dub_drop_state() == 2u && muted() == 1u << 6, "  nothing out before the line (state %u, muted %02x)",
          (unsigned)dub_drop_state(), (unsigned)muted());
    CHECK(mute_waiting() == (rest & ~(1u << 6)), "  the tracks to go blink (%02x)", (unsigned)mute_waiting());
    to_next_bar();
    CHECK(dub_drop_state() == 1u && muted() == rest, "  on the bar line all but T1 out (muted %02x)", (unsigned)muted());
    CHECK(dub_drop_press() == 0, "DROP pressed again: back next bar");
    CHECK(muted() == rest && mute_waiting() == (rest & ~(1u << 6)), "  still out until the line (waiting %02x)",
          (unsigned)mute_waiting());
    to_next_bar();
    CHECK(dub_drop_state() == 0u && muted() == 1u << 6, "  back on the one; T7's hand mute kept (muted %02x)",
          (unsigned)muted());
    CHECK(dub_drop_press() == 1 && dub_drop_press() == 0, "a press while one waits calls it off");
    to_next_bar();
    CHECK(dub_drop_state() == 0u && muted() == 1u << 6, "  nothing changed on the line (muted %02x)", (unsigned)muted());

    /* stopped: at once */
    tess_core_transport(0);
    trk[6].p[P_MUTE] = 0;
    tess_core_render(l, r, 0, 256);
    b = (uint32_t)dub_drop_press();
    CHECK(b == 1u && muted() == rest && dub_drop_state() == 1u, "stopped: DROP at once (%u, muted %02x, state %u, playing %d)",
          (unsigned)b, (unsigned)muted(), (unsigned)dub_drop_state(), (int)song.playing);
    CHECK(dub_drop_press() == 0 && muted() == 0u, "stopped: back at once");
    tessera.et_role[0] = 0;
    for (b = 0; b < TESSERA_TRACKS; b++) tessera.et_role[b] = b == 2u ? 2u : 3u;   /* (no ANCHOR: T3 RHYTHM kept) */
    CHECK(dub_drop_keep() == 1u << 2, "no ANCHOR: BASS and RHYTHM kept (%02x)", (unsigned)dub_drop_keep());
    for (b = 0; b < TESSERA_TRACKS; b++) tessera.et_role[b] = 3u;
    CHECK(dub_drop_press() == -1 && muted() == 0u && dub_drop_state() == 0u, "nothing to keep: nothing dropped (-1)");

    /* THROW */
    song.g[G_FXMODE] = 0;
    song.g[G_MFXTYPE] = 8;                             /* (REVERB, then ECHO in slot 2) */
    song.g[G_MFX2TYPE] = 6;
    song.g[G_MFX2CHAR] = 40;
    trk[1].p[P_FXSEND] = 10;
    trk[1].p[P_FXSEND2] = 0;
    trk[2].p[P_FXSEND] = 0;
    fx_bus_settings();
    CHECK(fx_send_gain(&trk[1]) == 10u * 512u, "no THROW: T2's send as set (%u)", (unsigned)fx_send_gain(&trk[1]));
    dub_throw = 1u << 1;
    fx_bus_settings();
    CHECK(dub_slots == 1u && fx_send_gain(&trk[1]) == 65536u, "CHAIN: THROW opens the one send in full");
    CHECK(fx_send_gain(&trk[2]) == 0u, "  only the thrown track's");
    CHECK(fx_bus_g[FX_PARAMS + 3u] == 40, "  THROW alone leaves REPEAT (%d)", fx_bus_g[FX_PARAMS + 3u]);
    song.g[G_FXMODE] = 1;
    fx_bus_settings();
    CHECK(dub_slots == 2u && fx_send_gain(&trk[1]) == 10u * 512u, "SPLIT: the echo slot's send, not slot 1's (slots %u)",
          (unsigned)dub_slots);
    song.g[G_MFX2TYPE] = 8;
    fx_bus_settings();
    CHECK(dub_slots == 1u, "SPLIT, no echo: slot 1's send");
    song.g[G_MFX2TYPE] = 6;
    dub_throw_fb = 1;
    fx_bus_settings();
    CHECK(fx_bus_g[FX_PARAMS + 3u] == DUB_THROW_FB && song.g[G_MFX2CHAR] == 40,
          "THROW + REPEAT lifts the echo's REPEAT for the block only (%d, kept %d)", fx_bus_g[FX_PARAMS + 3u],
          song.g[G_MFX2CHAR]);
    CHECK(fx_bus_g[3] == 0 || fx_bus_g[3] == song.g[G_MFXCHAR], "  the reverb's left alone");
    song.g[G_MFX2CHAR] = 127;
    fx_bus_settings();
    CHECK(fx_bus_g[FX_PARAMS + 3u] == 127, "  never lowers a higher REPEAT");
    tess_core_transport(1);
    for (b = 0; b < 200u; b++) tess_core_render(l, r, 0, 256);
    CHECK(trk[1].p[P_FXSEND2] == 0 && trk[1].p[P_FXSEND] == 10, "the track's own sends unchanged after playing thrown");

    /* send moves (NOTES' SEND lane; MOTION records the sends since step 4): played, saved and read back */
    {
        static uint8_t rec[TSP_MAX + 64];
        uint32_t len, i, hit = 0;
        tess_core_transport(0);
        for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
        dub_throw = 0, dub_throw_fb = 0;
        trk[0].p[P_FXSEND] = 0;
        motion_clear(&trk[0]);
        CHECK(motion_param(P_FXSEND) && motion_param(P_FXSEND2) && motion_param(P_FXSEND3),
              "MOTION takes FX SEND, SEND 2, SEND 3");
        CHECK(motion_set_event(&trk[0], 0, P_FXSEND, 127) == 0 && motion_set_event(&trk[0], 2, P_FXSEND, 0) == 0,
              "  T1: SEND 127 on step 1, 0 on step 3");
        tess_core_transport(1);
        for (i = 0; i < 800u && !hit; i++) {
            tess_core_render(l, r, 0, 256);
            hit = trk[0].p[P_FXSEND] == 127;
        }
        CHECK(hit, "  played: T1's send moves to 127");
        tess_core_transport(0);
        len = tess_core_save(rec, sizeof rec);
        motion_clear(&trk[0]);
        CHECK(len && !tess_core_load(rec, len) && send_at(0, 0) == 127 && send_at(0, 2) == 0,
              "  saved and read back: the two moves (%d, %d)", send_at(0, 0), send_at(0, 2));
    }

    /* a load forgets both */
    (void)dub_drop_press();
    CHECK(load(buf, (uint32_t)n) && dub_throw == 0u && !dub_throw_fb && dub_drop_state() == 0u,
          "a project load forgets DROP and THROW");

    printf("dub test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
