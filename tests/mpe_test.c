/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-note expression (TESS_MPE, the plugin): each note's own AT and SLIDE in the mods, through the real MIDI queue
 * and dispatch (seq.c, midi_control.c) and the matrix (mod.c mod_voice):
 *   MPE: a member channel's pressure and CC74 move only its own note; a channel's values before its note-on are the
 *   note's start; Poly AT and a CLAP note's BRIGHTNESS (code index 0) one key; a note not from MIDI the track's AT;
 *   AT -> CUTOFF and SLIDE -> SHAPE move each voice by its own; CC121 clears. Then what it costs: 16 voices with a
 *   mod from AT against the same from VEL (both per voice). Per-note bend: test_bend. Built with -DTESS_MPE=1
 *   -DCORE_TRACKS=8 */
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
    memset(trk[0].v, 0, sizeof trk[0].v);
    steal_pad(&trk[0]);                                /* (the same sound every run) */
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

/* per-note bend: a member channel's pitch bend moves its own note only, smoothed as the track's; MPE's MCM sets the
 * members' range to 48; a note-on takes its channel's bend at once; a CLAP note's TUNING adds to one key's */
static void test_bend(void)
{
    track_t *t = &trk[0];
    int32_t o[2 * CTL];
    voice_t *a, *b;
    host_tracks_init();
    t->p[P_VOICE] = V_POLY; t->p[P_SUS] = 127; t->p[P_AMODE] = 0;
    song.g[G_ROUTE] = 1; song.sel = 0;
    memset(midi_ch, 0, sizeof midi_ch);
    mq(0x91u, 60, 100, 9);
    mq(0x92u, 64, 100, 9);
    blk();
    a = voice_of(t, 60), b = voice_of(t, 64);
    if (!a || !b) { check("two notes for the bend", 0); return; }
    mq(0xE1u, 127, 127, 0xE);                           /* channel 2 fully up: 2 semitones (RPN 0's default) */
    for (uint32_t k = 0; k < 40u; k++) { blk(); mix_block(o, CTL); }
    printf("mpe:   bend: note 60 %d / %d Q8, note 64 %d\n", a->mbq8, a->mbt, b->mbq8);
    check("a member channel's bend moves its own note only, landing exactly (+2 semitones)", a->mbt == 512 &&
          a->mbq8 == 512 && b->mbq8 == 0 && b->mbt == 0);
    mq(0xB0u, 101, 0, 0xB); mq(0xB0u, 100, 6, 0xB); mq(0xB0u, 6, 15, 0xB);   /* MCM on channel 1: 15 members */
    blk();
    check("MPE's MCM: the members' bend range 48 semitones, the master's 2", midi_ch[1].semis == 48u &&
          midi_ch[15].semis == 48u && midi_ch[0].semis == 2u && a->mbt == 48 * 256);
    mq(0xE3u, 0, 96, 0xE);                              /* channel 4 at +0.5 before its note */
    mq(0x93u, 67, 100, 9);
    blk();
    {
        voice_t *c = voice_of(t, 67);
        check("a note-on takes its channel's bend at once (no glide in): +24 semitones", c && c->mbt == c->mbq8 &&
              c->mbt > 24 * 256 - 8 && c->mbt < 24 * 256 + 8);
    }
    mpe_tune_put(-1, 64, -300);                         /* a CLAP note's TUNING: key 64, -3 cents x 100 / 100 .. */
    blk();
    for (uint32_t k = 0; k < 40u; k++) { blk(); mix_block(o, CTL); }
    check("a CLAP note's TUNING: one key, added to its channel's bend", b->mtune == -300 && b->mbq8 == -300 &&
          a->mtune == 0);
    mq(0xE1u, 0, 64, 0xE);                              /* channel 2 back to the middle */
    for (uint32_t k = 0; k < 40u; k++) { blk(); mix_block(o, CTL); }
    check("bend back to the middle: the note back to 0", a->mbq8 == 0);
}

/* the bend reaches the sound: one note on channel 2, its channel bent or not: its zero crossings (the pitch) */
static uint32_t bent_crossings(uint32_t bend_msb)
{
    int32_t o[2 * CTL], was = 0;
    uint32_t n = 0;
    host_tracks_init();
    memset(trk[0].v, 0, sizeof trk[0].v);             /* (no voice left from before) */
    steal_pad(&trk[0]);                                /* (TRIO's plain triangle: its crossings are its pitch. Not FM6,
                                                        * which bends by its own DX7 range: voice.c's bend is not its) */
    trk[0].p[P_VOICE] = V_POLY; trk[0].p[P_SUS] = 127; trk[0].p[P_AMODE] = 0;
    song.g[G_ROUTE] = 1; song.sel = 0;
    memset(midi_ch, 0, sizeof midi_ch);
    mq(0xE1u, 0, bend_msb, 0xE);
    mq(0x91u, 48, 100, 9);
    for (uint32_t k = 0; k < 800u; k++) {
        blk(); mix_block(o, CTL);
        if (k >= 100u)                                  /* (past the attack) */
            for (uint32_t i = 0; i < CTL; i++) {
                if ((o[2 * i] >= 0) != (was >= 0)) n++;
                was = o[2 * i];
            }
    }
    mq(0x81u, 48, 0, 8);
    blk();
    return n;
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

    test_bend();
    {
        uint32_t z0 = bent_crossings(64), z1 = bent_crossings(100);   /* (100: +1.125 semitones, x 1.067) */
        printf("mpe:   zero crossings, 0.6 s: unbent %u, its channel bent +1.125 st %u (x %.3f)\n", z0, z1, z0 ? (double)z1 / z0 : 0.0);
        check("the bend reaches the sound: the bent note about 6.7 %% higher", z0 > 100u && z1 * 1000u > z0 * 1040u &&
              z1 * 1000u < z0 * 1100u);
    }
    {
        double c0 = cost_of(0), cv = cost_of(CS_VEL), ca2 = cost_of(CS_AT), cs = cost_of(CS_SLIDE);
        printf("mpe: cost, 16 voices held, a mod onto CUTOFF: none %.0f ns a block, VEL %.0f, AT %.0f, SLIDE %.0f "
               "(%.2f %% of a block's %.0f us)\n", c0, cv, ca2, cs, 100.0 * (ca2 - c0) / (1e9 * CTL / FS), 1e6 * CTL / FS);
        check("AT and SLIDE per voice cost about what VEL per voice does", ca2 < cv * 1.25 + 200 && cs < cv * 1.25 + 200);
    }
    printf(bad ? "mpe: %d FAILED\n" : "mpe: ok\n", bad);
    return bad != 0;
}
