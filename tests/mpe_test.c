/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-note expression (TESS_MPE, the plugin): each note's own AT and SLIDE in the mods, through the real MIDI queue
 * and dispatch (seq.c, midi_control.c) and the matrix (mod.c mod_voice):
 *   MPE: a member channel's pressure and CC74 move only its own note; a channel's values before its note-on are the
 *   note's start; Poly AT and a CLAP note's BRIGHTNESS (code index 0) one key; a note not from MIDI the track's AT;
 *   AT -> CUTOFF and SLIDE -> SHAPE move each voice by its own; CC121 clears. Then what it costs: 16 voices with a
 *   mod from AT against the same from VEL (both per voice). Built with -DTESS_MPE=1 -DCORE_TRACKS=8 */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("mpe: %-96s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) bad++;
}
/* a message into the core's queue as tess_core_midi puts it (cin: its code index, 0 for one key's slide) */
static void mq(uint32_t status, uint32_t d1, uint32_t d2, uint32_t cin)
{
    midi_in_q[mi_w % MQ] = cin | status << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24;
    mi_w++;
}
static void blk(void) { events_block(CTL); }
static voice_t *voice_of(track_t *t, uint32_t key)
{
    for (uint32_t k = 0; k < NVOICE; k++)
        if (t->v[k].active && t->v[k].gate && t->v[k].mkey == key) return &t->v[k];
    return 0;
}
static voice_t *voice_note(track_t *t, uint32_t note)
{
    for (uint32_t k = 0; k < NVOICE; k++)
        if (t->v[k].active && t->v[k].gate && t->v[k].note == note) return &t->v[k];
    return 0;
}
static void mod_to(uint32_t i, uint32_t src, uint32_t dst)
{
    cord_t c;
    memset(&c, 0, sizeof c);
    c.src = (uint8_t)src; c.src_trk = 0; c.dst = (uint8_t)dst; c.dst_trk = 0; c.amount = 100;
    if (!cord_set(i, &c)) check("a mod set", 0);
}
/* voice v's CUTOFF and SHAPE as the block's mods make them, from a plain vmod_t */
static void moved(track_t *t, voice_t *v, int32_t *cut, int32_t *shp)
{
    vmod_t m;
    memset(&m, 0, sizeof m);
    m.amp1 = 32767;
    m.pitch16 = 60 * 16;
    *cut = *shp = -1;
    if (!mod_begin(t)) return;
    mod_voice(t, v, &m, 0);
    mod_end(t);
    *cut = m.cutoff, *shp = m.shape;
}

static double cost_of(uint32_t src)                     /* ns a block: track 1, 16 voices held, a mod src -> CUTOFF */
{
    static const uint8_t NOTES[16] = {36, 40, 43, 47, 48, 52, 55, 59, 60, 64, 67, 71, 72, 76, 79, 83};
    uint64_t best = ~0ull;
    int32_t o[2 * CTL];
    host_tracks_init();
    for (uint32_t i = 0; i < CORD_MAX; i++) { cord_t z; memset(&z, 0, sizeof z); cord_set(i, &z); }
    trk[0].p[P_VOICE] = V_POLY; trk[0].p[P_SUS] = 127; trk[0].p[P_AMODE] = 0;
    if (src) mod_to(0, src, CD_CUTOFF);
    song.g[G_ROUTE] = 1; song.sel = 0;
    for (uint32_t i = 0; i < 16u; i++) mq(0x90u | (1u + i % 15u), NOTES[i], 100, 9);   /* (MPE: a channel a note) */
    for (uint32_t k = 0; k < 64u; k++) { mq(0xD0u | (1u + k % 15u), k & 127u, 0, 0xD); blk(); mix_block(o, CTL); }
    for (uint32_t r = 0; r < 5u; r++) {
        uint64_t t0 = now_ns();
        for (uint32_t k = 0; k < 2000u; k++) { blk(); mix_block(o, CTL); }
        uint64_t ns = now_ns() - t0;
        best = ns < best ? ns : best;
    }
    return (double)best / 2000.0;
}

int main(void)
{
    track_t *t = &trk[0];
    voice_t *a, *b, *s;
    int32_t ca, sa, cb, sb;
    host_tracks_init();
    t->p[P_VOICE] = V_POLY; t->p[P_SUS] = 127; t->p[P_AMODE] = 0;
    song.g[G_ROUTE] = 1;                                /* (SEL: every channel plays the selected track, as MPE wants) */
    song.sel = 0;
    mod_to(0, CS_AT, CD_CUTOFF);
    mod_to(1, CS_SLIDE, CD_SHAPE);
    check("SLIDE is cord source 35, a track's", CS_SLIDE == 35 && !CS_PROJECT(CS_SLIDE) && !strcmp(cord_src_name(CS_SLIDE), "SLIDE"));

    mq(0xD3u, 90, 0, 0xD);                              /* (channel 4's pressure before its note: the note's start) */
    mq(0xB3u, 74, 30, 0xB);
    mq(0x91u, 60, 100, 9);                              /* member channels 2 and 3, and 4 */
    mq(0x92u, 64, 100, 9);
    mq(0x93u, 67, 100, 9);
    blk();
    a = voice_of(t, 60), b = voice_of(t, 64), s = voice_of(t, 67);
    check("three notes on three channels, each voice knows its channel and key", a && b && s && a->mch == 2u &&
          b->mch == 3u && s->mch == 4u && a->mkey == 60u);
    if (!a || !b || !s) return 1;
    check("a channel's pressure and CC74 before its note-on are the note's start", s->mat == 90u && s->msl == 30u &&
          !a->mat && !b->mat);

    mq(0xD1u, 100, 0, 0xD);                             /* channel 2's pressure */
    mq(0xB2u, 74, 120, 0xB);                            /* channel 3's slide */
    blk();
    check("a member channel's pressure moves its own note only", a->mat == 100u && !b->mat && s->mat == 90u);
    check("a member channel's CC74 slides its own note only", b->msl == 120u && !a->msl && s->msl == 30u);
    moved(t, a, &ca, &sa);
    moved(t, b, &cb, &sb);
    printf("mpe:   AT -> CUTOFF, SLIDE -> SHAPE: note 60 (AT 100) cut %d shape %d; note 64 (SLIDE 120) cut %d shape %d\n",
           ca, sa, cb, sb);
    check("AT -> CUTOFF, per voice: the pressed note opens, the other not", ca > 0 && cb == 0);
    check("SLIDE -> SHAPE, per voice: the slid note moves, the other not", sb > 0 && sa == 0);

    mq(0xA2u, 64, 50, 0xA);                             /* Poly AT on channel 3, key 64 */
    mq(0xA1u, 64, 77, 0xA);                             /* .. key 64 on channel 2: no such note there */
    mq(0xA3u, 67, 11, 0);                               /* one key's slide (a CLAP BRIGHTNESS: code index 0) */
    blk();
    check("Poly AT: that key on that channel only", b->mat == 50u && a->mat == 100u);
    check("a CLAP note's BRIGHTNESS (code index 0): that key's slide, not its pressure", s->msl == 11u && s->mat == 90u);

    trk_note_on(t, 72, 100);                            /* (a note not from MIDI: the sequencer's, a generator's) */
    {
        voice_t *q = voice_note(t, 72);
        int32_t cq, sq;
        check("a note not from MIDI has no channel", q && !q->mch);
        moved(t, q, &cq, &sq);
        check("it reads the track's AT (the last channel pressure that reached the track)", q && t->at == 100u && cq == ca);
    }

    mq(0xB1u, 121, 0, 0xB);                             /* CC121 on channel 2: the next note there starts from 0 */
    mq(0x81u, 60, 0, 8);
    mq(0x91u, 61, 100, 9);
    blk();
    a = voice_of(t, 61);
    check("after CC121 a channel's next note starts at 0", a && !a->mat && !a->msl && a->mch == 2u);

    {
        double c0 = cost_of(0), cv = cost_of(CS_VEL), ca2 = cost_of(CS_AT), cs = cost_of(CS_SLIDE);
        printf("mpe: cost, 16 voices held, a mod onto CUTOFF: none %.0f ns a block, VEL %.0f, AT %.0f, SLIDE %.0f "
               "(%.2f %% of a block's %.0f us)\n", c0, cv, ca2, cs, 100.0 * (ca2 - c0) / (1e9 * CTL / FS), 1e6 * CTL / FS);
        check("AT and SLIDE per voice cost about what VEL per voice does", ca2 < cv * 1.25 + 200 && cs < cv * 1.25 + 200);
    }
    printf(bad ? "mpe: %d FAILED\n" : "mpe: ok\n", bad);
    return bad != 0;
}
