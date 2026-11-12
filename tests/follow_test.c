/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* FOLLOW's HOW and RHYTHM (tessera.c tess_follow_gate, tess_follow_how) through the real sequencer, read off MIDI OUT a
 * block at a time: a EUCLID track (2) follows a STEP melody (track 1) in C major. SAME / OWN, and HOW / RHYTHM with
 * FOLLOW OFF, play as before; RHYTHM SOURCE plays on the melody's steps, BOTH on its own and the melody's; CHORD a third,
 * fifth or seventh of the melody's note, never a leap past a tritone; CONTRA the other way; BASS an octave or two down on
 * beats 1 and 3; CANON2 the melody two notes late; the melody's own MIDI never changes; the same each PLAY. A muted
 * generator still leads: muted from PLAY or on the way, its follower plays as if it were heard, it sounds nothing
 * (no MIDI, no voice), and unmuted it comes back on its own notes, none doubled; a muted CHORD track still gives
 * QNT CHRD its chord. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

#define LOG_MAX 8192u
#define DIV_8TH 1u
static uint32_t log_pkt[LOG_MAX], log_blk[LOG_MAX], log_n, blk;

static void blocks(uint32_t n)
{
    while (n--) {
        events_block(CTL);
        blk++;
        while (mo_r != mo_w) {
            if (log_n < LOG_MAX) { log_blk[log_n] = blk; log_pkt[log_n++] = midi_out_q[mo_r % MQ]; }
            mo_r++;
        }
    }
}
#define EV_MAX 256u
typedef struct { uint32_t at[EV_MAX]; uint8_t note[EV_MAX]; uint32_t n; } run_t;

/* the melody, 16 eighths, 0 a rest: up and down C major */
static const uint8_t MEL[16] = {60, 62, 0, 64, 65, 0, 67, 0, 69, 67, 0, 65, 64, 0, 62, 0};

static void setup(uint32_t follow, uint32_t how, uint32_t rhy, int silent)
{
    uint32_t i;
    ui_power_on();
    for (i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = DIV_8TH;
    }
    tessera_reset();
    cord_clear();
    song.g[G_BPM] = 120;
    usb.config = 1;
    mi_r = mi_w = 0; mo_r = mo_w = 0;
    settings_midi_out = MIDI_OUT_ALL; settings_clock_out = 0; settings_drumch = 0;
    song.g[G_ROUTE] = 0;
    for (i = 0; i < 16u; i++)
        trk[0].step[i] = MEL[i] && !silent ? (step_t){{MEL[i]}, 1, ST_NOTE, 0, 0, 100} : (step_t){{60}, 0, ST_REST, 0, 0, 100};
    song.sel = 1;
    tessera_knob(0, TESS_ENG_EUCLID);
    trk[1].p[P_SDIV] = DIV_8TH;
    trk[1].p[P_SCALE] = 1;                              /* (major) */
    tessera.seed[1] = 5;
    tessera_param_set(1, TX_TW_FOLLOW, (int32_t)follow);
    tessera_param_set(1, TX_TW_FHOW, (int32_t)how);
    tessera_param_set(1, TX_TW_FRHY, (int32_t)rhy);
    song.sel = 0;
    blocks(1);
    log_n = 0;
}
/* play beats, then STOP: the melody's note-ons into *m, the follower's into *f */
static void play(uint32_t beats, run_t *m, run_t *f)
{
    uint32_t i, t0 = blk, stop_at;
    log_n = 0;
    transport_req = 1;
    blocks(beats * (beat_samples() / CTL) + 1u);
    stop_at = log_n;
    transport_req = 2;
    blocks(2);
    m->n = f->n = 0;
    for (i = 0; i < stop_at; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        run_t *r = s == 0x90u ? m : s == 0x91u ? f : 0;
        if (!r || r->n >= EV_MAX) continue;
        r->at[r->n] = (log_blk[i] - t0) * CTL;
        r->note[r->n++] = (uint8_t)d1;
    }
}
static int same(const run_t *a, const run_t *b)
{
    if (a->n != b->n) return 0;
    for (uint32_t i = 0; i < a->n; i++)
        if (a->at[i] != b->at[i] || a->note[i] != b->note[i]) return 0;
    return 1;
}
#define SLOP (3u * CTL)
static uint32_t span8(void) { return (uint32_t)((uint64_t)seq_div_samples(DIV_8TH) * beat_samples() / seq_div_samples(0)); }
static uint32_t step_of(uint32_t at) { return (at + SLOP) / span8(); }
static int deg(int32_t n)                               /* a C major note's degree, counted from C -1 */
{
    static const int8_t D[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    return D[n % 12] < 0 ? -1000 : (n / 12) * 7 + D[n % 12];
}
/* the follower's note-on on step st, -1 none */
static int32_t at_step(const run_t *r, uint32_t st)
{
    for (uint32_t i = 0; i < r->n; i++) if (step_of(r->at[i]) == st) return r->note[i];
    return -1;
}


/* the source a generator (TURING, track 1) instead of the melody; muted from PLAY when pre, the MIXER's mute key
 * (MUTES NOW) pressed at blocks mute_at and unmute_at after PLAY (~0u: never); *gated: a voice of it ever opened while
 * muted */
static void setup_gen(uint32_t how, uint32_t rhy, int pre)
{
    setup(1, how, rhy, 1);
    tessera_knob(0, TESS_ENG_TURING);
    trk[0].p[P_SDIV] = DIV_8TH;
    trk[0].p[P_SCALE] = 1;
    tessera.seed[0] = 9;
    tessera.rnd[0] = gen_stream(9);
    settings_mute_q = 0;
    trk[0].p[P_MUTE] = (int16_t)(pre != 0);
    blocks(1);
    log_n = 0;
}
static void play_mute(uint32_t beats, run_t *a, run_t *b, uint32_t mute_at, uint32_t unmute_at, int *gated)
{
    uint32_t i, j, t0 = blk, stop_at, n = beats * (beat_samples() / CTL) + 1u;
    log_n = 0;
    *gated = 0;
    transport_req = 1;
    for (j = 0; j < n; j++) {
        if (j == mute_at || j == unmute_at) mute_press(0, 0);
        blocks(1);
        if (trk[0].p[P_MUTE])
            for (i = 0; i < NVOICE; i++) *gated |= trk[0].v[i].gate != 0;
    }
    stop_at = log_n;
    transport_req = 2;
    blocks(2);
    trk[0].p[P_MUTE] = 0;
    a->n = b->n = 0;
    for (i = 0; i < stop_at; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        run_t *r = s == 0x90u ? a : s == 0x91u ? b : 0;
        if (!r || r->n >= EV_MAX) continue;
        r->at[r->n] = (log_blk[i] - t0) * CTL;
        r->note[r->n++] = (uint8_t)d1;
    }
}
/* r's note-ons from sample at on, as a run of their own */
static void from(const run_t *r, uint32_t at, run_t *o)
{
    o->n = 0;
    for (uint32_t i = 0; i < r->n; i++)
        if (r->at[i] >= at) { o->at[o->n] = r->at[i]; o->note[o->n++] = r->note[i]; }
}

static run_t mel0, own0, alone, m, f, f2, g0, gf0, ga, gb;

int main(void)
{
    int bad = 0, ok;
    uint32_t i, k, st, beats = 16, steps = 32;          /* (two passes of the melody) */

    setup(1, TESS_FH_SAME, TESS_FR_OWN, 0);
    play(beats, &mel0, &own0);
    setup(1, TESS_FH_SAME, TESS_FR_OWN, 0);
    play(beats, &m, &f);
    bad += check("the ground: the same each PLAY, a melody and a follower with rests", same(&own0, &f) && same(&mel0, &m) &&
                 mel0.n == 20u && own0.n > 4u && own0.n < steps);
    setup(1, TESS_FH_CHORD, TESS_FR_BOTH, 0);
    tessera_param_set(1, TX_TW_FHOW, TESS_FH_SAME);
    tessera_param_set(1, TX_TW_FRHY, TESS_FR_OWN);
    play(beats, &m, &f);
    bad += check("SAME, OWN: as before (turned away and back)", same(&own0, &f));
    setup(0, TESS_FH_SAME, TESS_FR_OWN, 0);
    play(beats, &m, &alone);
    setup(0, TESS_FH_CANON + 1u, TESS_FR_SOURCE, 0);
    play(beats, &m, &f);
    bad += check("FOLLOW OFF: HOW and RHYTHM change nothing", same(&alone, &f));

    setup(1, TESS_FH_SAME, TESS_FR_SOURCE, 0);
    play(beats, &m, &f);
    ok = f.n == mel0.n && same(&mel0, &m);
    for (i = 0; ok && i < f.n; i++) ok = step_of(f.at[i]) == step_of(mel0.at[i]) && f.note[i] == mel0.note[i];
    bad += check("RHYTHM SOURCE: on the melody's steps, its notes (SAME, SHIFT 0)", ok);

    setup(1, TESS_FH_SAME, TESS_FR_BOTH, 0);
    play(beats, &m, &f);
    ok = 1;
    for (st = 0; st < steps; st++) ok &= (at_step(&f, st) >= 0) == (at_step(&own0, st) >= 0 || at_step(&mel0, st) >= 0);
    bad += check("RHYTHM BOTH: its own steps and the melody's", ok && f.n > own0.n && f.n > mel0.n);

    setup(1, TESS_FH_SAME, TESS_FR_SOURCE, 1);
    play(beats, &m, &f);
    bad += check("RHYTHM SOURCE on a silent melody: silent", m.n == 0u && f.n == 0u);

    setup(1, TESS_FH_CHORD, TESS_FR_SOURCE, 0);
    play(beats, &m, &f);
    ok = f.n == mel0.n && same(&mel0, &m);
    for (i = 0; ok && i < f.n; i++) {
        int d = ((deg(f.note[i]) - deg(mel0.note[i])) % 7 + 7) % 7;
        ok = deg(f.note[i]) >= 0 && (d == 2 || d == 4 || d == 6) && (!i || abs((int)f.note[i] - (int)f.note[i - 1]) <= 6);
    }
    bad += check("CHORD: a third, fifth or seventh of the melody's note, in the scale, no leap past a tritone", ok);
    setup(1, TESS_FH_CHORD, TESS_FR_SOURCE, 0);
    play(beats, &m, &f2);
    bad += check("  the same each PLAY", same(&f, &f2));

    setup(1, TESS_FH_CONTRA, TESS_FR_SOURCE, 0);
    play(beats, &m, &f);
    ok = f.n == mel0.n && f.n && f.note[0] == mel0.note[0];
    for (i = 1; ok && i < f.n; i++) {
        int ds = (int)mel0.note[i] - (int)mel0.note[i - 1], df = (int)f.note[i] - (int)f.note[i - 1];
        ok = deg(f.note[i]) >= 0 && (ds > 0 ? df < 0 : ds < 0 ? df > 0 : df == 0);
    }
    bad += check("CONTRA: from the melody's first note, every step the other way", ok);

    setup(1, TESS_FH_BASS, TESS_FR_SOURCE, 0);
    play(beats, &m, &f);
    ok = f.n == steps / 4u;                             /* (the melody plays on every beat 1 and 3) */
    for (i = 0; ok && i < f.n; i++) {
        int32_t s = -1;
        st = step_of(f.at[i]);
        for (k = 0; k < mel0.n; k++) if (step_of(mel0.at[k]) <= st) s = mel0.note[k];
        ok = !(st % 4u) && s >= 0 && (f.note[i] == s - 12 || f.note[i] == s - 24) && f.note[i] < 48;
    }
    bad += check("BASS, SOURCE: on beats 1 and 3, the melody's note an octave or two down", ok);
    setup(1, TESS_FH_BASS, TESS_FR_OWN, 0);
    play(beats, &m, &f);
    ok = f.n > 0u && f.n < own0.n;
    for (i = 0; ok && i < f.n; i++) ok = !(step_of(f.at[i]) % 4u) && at_step(&own0, step_of(f.at[i])) >= 0;
    bad += check("BASS, OWN: its own hits on beats 1 and 3 only", ok);

    setup(1, TESS_FH_CANON + 1u, TESS_FR_SOURCE, 0);   /* (CANON2) */
    play(beats, &m, &f);
    ok = f.n + 2u == mel0.n;
    for (i = 0; ok && i < f.n; i++) ok = step_of(f.at[i]) == step_of(mel0.at[i + 2u]) && f.note[i] == mel0.note[i];
    bad += check("CANON2, SOURCE: the melody two notes late, silent until it has played two", ok);
    setup(1, TESS_FH_CANON + 7u, TESS_FR_OWN, 0);      /* (CANON8, its own rhythm: nothing before the melody's 9th) */
    play(beats, &m, &f);
    ok = f.n > 0u;
    for (i = 0; ok && i < f.n; i++) ok = f.at[i] >= mel0.at[8] && at_step(&own0, step_of(f.at[i])) >= 0;
    bad += check("CANON8, OWN: its own steps, from the melody's ninth note on", ok);


    {   /* MUTE (Theo 2026-11-10: B following A, A muted on the MIXER, B went silent): A's notes still lead */
        uint32_t bb = beat_samples() / CTL, um = 8u * bb + span8() / CTL / 2u;   /* (unmuted mid-step) */
        int gated;
        static const uint8_t HOW[3] = {TESS_FH_SAME, TESS_FH_CHORD, TESS_FH_CANON + 1u};
        for (k = 0; k < 3u; k++) {
            setup_gen(HOW[k], TESS_FR_SOURCE, 0);
            play_mute(beats, &g0, &gf0, ~0u, ~0u, &gated);
            ok = g0.n > 4u && gf0.n > 2u;
            setup_gen(HOW[k], TESS_FR_SOURCE, 1);
            play_mute(beats, &ga, &gb, ~0u, ~0u, &gated);
            ok &= same(&gf0, &gb) && ga.n == 0u && !gated;
            setup_gen(HOW[k], TESS_FR_SOURCE, 0);
            play_mute(beats, &ga, &gb, 4u * bb, ~0u, &gated);
            from(&g0, 4u * bb * CTL, &f);
            ok &= same(&gf0, &gb) && ga.n + f.n == g0.n && !gated;
            setup_gen(HOW[k], TESS_FR_SOURCE, 1);
            play_mute(beats, &ga, &gb, ~0u, um, &gated);
            from(&g0, um * CTL, &f);
            ok &= same(&gf0, &gb) && same(&f, &ga) && !gated;
            bad += check(k == 0u ? "MUTE: a muted TURING still leads (SAME): its follower as when heard; it silent; "
                                   "unmuted, its own notes on, none doubled" :
                         k == 1u ? "  CHORD" : "  CANON2", ok);
        }
        setup_gen(TESS_FH_SAME, TESS_FR_SOURCE, 0);    /* QNT CHRD's chord: from a muted CHORD track too */
        song.sel = 2;
        tessera_knob(0, TESS_ENG_CHORD);
        song.sel = 0;
        trk[2].p[P_MUTE] = 1;
        transport_req = 1;
        blocks(4u * bb);
        ok = tessera_chord_pcs() != 0u;
        for (i = 0; i < NVOICE; i++) ok &= !trk[2].v[i].gate;
        for (i = 0; i < log_n; i++) ok &= ((log_pkt[i] >> 8) & 0xFFu) != 0x92u;
        transport_req = 2;
        blocks(2);
        trk[2].p[P_MUTE] = 0;
        bad += check("  a muted CHORD track still gives QNT CHRD its chord, silent", ok);
    }
    {                                                   /* the parameters: ranges, never past their ends */
        tessera_param_set(1, TX_TW_FHOW, 99);
        tessera_param_set(1, TX_TW_FRHY, -3);
        ok = tessera_param_get(1, TX_TW_FHOW) == TESS_FH_COUNT - 1 && tessera_param_get(1, TX_TW_FRHY) == 0;
        song.sel = 1;
        tessera_tw_layer_knob(1, -1); tessera_tw_layer_knob(2, 1); tessera_tw_layer_knob(2, 1); tessera_tw_layer_knob(2, 1);
        ok &= tessera.follow_how[1] == TESS_FH_COUNT - 2u && tessera.follow_rhy[1] == TESS_FR_BOTH;
        tessera_tw_layer_knob(0, 1);
        ok &= tessera.follow_how[1] == TESS_FH_COUNT - 2u && tessera.follow_rhy[1] == TESS_FR_BOTH;
        song.sel = 0;
        bad += check("HOW and RHYTHM: clamped at their ends; OCT+ KNOB 2 and 3 step them, KNOB 1 nothing", ok);
    }
    printf("%s\n", bad ? "FOLLOW TEST FAILED" : "FOLLOW tests passed");
    return bad != 0;
}
