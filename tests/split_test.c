/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* TIMING's SPLIT (firmware/src/seq_sub.c) through the real sequencer and generators, read off MIDI OUT a block at a time:
 * SPLIT 0 plays as before whatever HOW says; 100 % DOUBLE plays each generator note where it was and a neighbour
 * halfway, PUSH moves it halfway, MIX does either; never the bar's or the pattern's first step; the generator's own
 * notes the same throughout; the tosses replay, a new SEED gives new ones; a STEP track's second note goes toward the next
 * written note; drums and ATLAS repeat the hit; a ratchet, tie, slide, early Nudge, rest, FIXED's DOUBLE and recording
 * left as written; gates halved; a late Nudge and HUMAN add up; a cord brings SPLIT in. */
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
typedef struct { uint32_t at; uint8_t note; } ev_t;   /* a note-on, samples since PLAY */
#define EV_MAX 512u
typedef struct { ev_t e[EV_MAX]; uint32_t n; uint32_t off[EV_MAX]; } run_t;

static void reset(void)
{
    ui_power_on();
    for (uint32_t i = 0; i < NTRK; i++) {
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
    song.sel = 0; song.g[G_ROUTE] = 0;
    blocks(1);
    log_n = 0;
}
/* play beats, then STOP: track tr's note-ons into *r, each one's length in samples (0: to STOP) */
static void run(uint32_t tr, uint32_t beats, run_t *r)
{
    uint32_t i, j, t0, stop_at;
    log_n = 0;
    t0 = blk;
    transport_req = 1;
    blocks(beats * (beat_samples() / CTL) + 1u);
    stop_at = log_n;
    transport_req = 2;
    blocks(2);
    r->n = 0;
    for (i = 0; i < stop_at && r->n < EV_MAX; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        if (s != (0x90u | tr)) continue;
        r->e[r->n].at = (log_blk[i] - t0) * CTL;
        r->e[r->n].note = (uint8_t)d1;
        r->off[r->n] = 0;
        for (j = i + 1u; j < log_n; j++)
            if (((log_pkt[j] >> 8) & 0xFFu) == (0x80u | tr) && ((log_pkt[j] >> 16) & 0x7Fu) == d1) {
                r->off[r->n] = (log_blk[j] - log_blk[i]) * CTL;
                break;
            }
        r->n++;
    }
}
static int same(const run_t *a, const run_t *b)
{
    if (a->n != b->n) return 0;
    for (uint32_t i = 0; i < a->n; i++)
        if (a->e[i].at != b->e[i].at || a->e[i].note != b->e[i].note) return 0;
    return 1;
}
#define SLOP (3u * CTL)                                 /* (a note-on comes on a block's edge) */
static uint32_t span_of(uint32_t div) { return (uint32_t)((uint64_t)seq_div_samples(div) * beat_samples() / seq_div_samples(0)); }   /* (a step, samples: seq_div_samples counts its units) */
static uint32_t step_of(uint32_t at, uint32_t span) { return (at + SLOP) / span; }
static int on_line(uint32_t at, uint32_t span) { return (at + SLOP) % span <= 2u * SLOP; }
static int near(uint32_t a, uint32_t b) { return a + SLOP >= b && a <= b + SLOP; }
static uint32_t idiff(uint32_t a, uint32_t b) { return a > b ? a - b : b - a; }

static run_t base, r2, r3;

static void gen_setup(uint32_t mode, uint32_t split, uint32_t how, uint32_t div)
{
    reset();
    trk[0].p[P_SDIV] = (int16_t)div;
    tessera_knob(0, (int32_t)mode);
    trk[0].p[P_SDIV] = (int16_t)div;
    tessera.seed[0] = 7;
    tessera_param_set(0, TX_SUB_SPLIT, (int32_t)split);
    tessera_param_set(0, TX_SUB_SPLIT_HOW, (int32_t)how);
}

static int generator_test(void)
{
    int bad = 0, ok;
    uint32_t mode, i, j, span = span_of(DIV_8TH), firsts = 0, doubled = 0, pushed = 0;
    static const uint8_t MODES[3] = {TESS_ENG_TURING, TESS_ENG_EUCLID, TESS_ENG_MARKOV};
    for (mode = 0; mode < 3u; mode++) {
        gen_setup(MODES[mode], 0, 0, DIV_8TH);
        run(0, 16, &base);
        gen_setup(MODES[mode], 0, 0, DIV_8TH);
        run(0, 16, &r2);
        bad += check("a generator replays the same from PLAY (the test's ground)", base.n >= 4u && same(&base, &r2));
        gen_setup(MODES[mode], 0, TESS_SPLIT_PUSH, DIV_8TH);
        run(0, 16, &r2);
        bad += check("SPLIT 0: as before, whatever HOW says", same(&base, &r2));

        gen_setup(MODES[mode], 100, TESS_SPLIT_DOUBLE, DIV_8TH);
        run(0, 16, &r2);
        ok = 1; j = 0; firsts = 0;
        for (i = 0; i < base.n; i++) {
            uint32_t st = step_of(base.e[i].at, span);
            if (j >= r2.n || r2.e[j].at != base.e[i].at || r2.e[j].note != base.e[i].note) { ok = 0; break; }
            j++;
            if (!(st % 8u) || !(st % 16u)) { firsts++; continue; }
            if (j >= r2.n || !near(r2.e[j].at, base.e[i].at + span / 2u) || r2.e[j].note == base.e[i].note ||
                idiff(r2.e[j].note, base.e[i].note) > 4u) { ok = 0; break; }
            j++;
        }
        bad += check("100 % DOUBLE: every generator note where it was, a neighbour halfway; the bar's first alone",
                     ok && j == r2.n && r2.n == 2u * base.n - firsts);

        gen_setup(MODES[mode], 100, TESS_SPLIT_PUSH, DIV_8TH);
        run(0, 16, &r2);
        ok = base.n == r2.n;
        for (i = 0; ok && i < base.n; i++) {
            uint32_t st = step_of(base.e[i].at, span);
            ok = r2.e[i].note == base.e[i].note &&
                 (!(st % 8u) ? r2.e[i].at == base.e[i].at : near(r2.e[i].at, base.e[i].at + span / 2u));
        }
        bad += check("100 % PUSH: the same notes, each halfway through its step; the bar's first on its line", ok);

        gen_setup(MODES[mode], 100, TESS_SPLIT_MIX, DIV_8TH);
        run(0, 16, &r2);
        ok = 1; j = 0; doubled = pushed = 0;
        for (i = 0; ok && i < base.n; i++) {
            uint32_t st = step_of(base.e[i].at, span);
            if (j >= r2.n || r2.e[j].note != base.e[i].note) { ok = 0; break; }
            if (!(st % 8u)) { ok = r2.e[j++].at == base.e[i].at; continue; }
            if (near(r2.e[j].at, base.e[i].at + span / 2u)) { pushed++; j++; continue; }
            ok = r2.e[j].at == base.e[i].at && j + 1u < r2.n && near(r2.e[j + 1u].at, base.e[i].at + span / 2u);
            doubled++;
            j += 2u;
        }
        printf("    MIX: %u doubled, %u pushed\n", doubled, pushed);
        bad += check("100 % MIX: each step one or the other, some of both", ok && doubled && pushed);
    }

    gen_setup(TESS_ENG_TURING, 0, 0, DIV_8TH);
    tessera_param_set(0, TX_SUB_RATCH, 1);
    tessera_param_set(0, TX_SUB_CHANCE, 50);
    run(0, 8, &base);
    gen_setup(TESS_ENG_TURING, 100, TESS_SPLIT_MIX, DIV_8TH);
    tessera_param_set(0, TX_SUB_RATCH, 1);
    tessera_param_set(0, TX_SUB_CHANCE, 50);
    run(0, 8, &r2);
    bad += check("RATCH x2 at CHANCE 50: nothing split, its tosses and notes the same", base.n >= 2u && same(&base, &r2));

    gen_setup(TESS_ENG_TURING, 0, 0, DIV_8TH);
    tessera_param_set(0, TX_SUB_HUMAN, 40);
    run(0, 16, &base);
    gen_setup(TESS_ENG_TURING, 100, TESS_SPLIT_PUSH, DIV_8TH);
    tessera_param_set(0, TX_SUB_HUMAN, 40);
    run(0, 16, &r2);
    ok = base.n == r2.n;
    for (i = 0; ok && i < base.n; i++) {
        uint32_t st = step_of(base.e[i].at, span);
        ok = r2.e[i].note == base.e[i].note &&
             (!(st % 8u) ? r2.e[i].at == base.e[i].at : near(r2.e[i].at, base.e[i].at + span / 2u));
    }
    bad += check("PUSH with HUMAN TIME: HUMAN's own lateness on top, its stream the same", ok);

    gen_setup(TESS_ENG_TURING, 0, 0, DIV_8TH);
    tessera_param_set(0, TX_NL_MODE, NL_FIXED);
    run(0, 8, &base);
    gen_setup(TESS_ENG_TURING, 100, TESS_SPLIT_DOUBLE, DIV_8TH);
    tessera_param_set(0, TX_NL_MODE, NL_FIXED);
    run(0, 8, &r2);
    bad += check("NOTE LENGTH FIXED: DOUBLE left as written", base.n > 4u && same(&base, &r2));
    gen_setup(TESS_ENG_TURING, 100, TESS_SPLIT_PUSH, DIV_8TH);
    tessera_param_set(0, TX_NL_MODE, NL_FIXED);
    run(0, 8, &r2);
    ok = base.n == r2.n;
    for (i = 0; ok && i < base.n; i++)
        ok = !(step_of(base.e[i].at, span) % 8u) || near(r2.e[i].at, base.e[i].at + span / 2u);
    bad += check("NOTE LENGTH FIXED: PUSH still moves the note", ok);
    return bad;
}

/* a STEP track: 16 written notes, DIV div, SPLIT and HOW */
static void step_setup(uint32_t split, uint32_t how, uint32_t div)
{
    static const uint8_t N[8] = {60, 62, 64, 65, 67, 65, 64, 62};
    reset();
    trk[0].p[P_SDIV] = (int16_t)div;
    trk[0].p[P_SLEN] = 16;
    for (uint32_t i = 0; i < 16u; i++) trk[0].step[i] = (step_t){{N[i % 8u]}, 1, ST_NOTE, 0, 0, 100};
    tessera.seed[0] = 11;
    tessera_param_set(0, TX_SUB_SPLIT, (int32_t)split);
    tessera_param_set(0, TX_SUB_SPLIT_HOW, (int32_t)how);
}

static int step_track_test(void)
{
    static const uint8_t N[8] = {60, 62, 64, 65, 67, 65, 64, 62};
    static const uint8_t DIVS[3] = {DIV_8TH, 2u, 4u};  /* (1/8, 1/16, 8T) */
    int bad = 0, ok;
    uint32_t i, j, d, span, c30;
    for (d = 0; d < 3u; d++) {
        span = span_of(DIVS[d]);
        step_setup(0, 0, DIVS[d]);
        run(0, 8, &base);
        step_setup(100, TESS_SPLIT_DOUBLE, DIVS[d]);
        run(0, 8, &r2);
        ok = 1; j = 0;
        for (i = 0; ok && i < base.n; i++) {
            uint32_t st = step_of(base.e[i].at, span), idx = st % 16u, spb = (4u * beat_samples() + span / 2u) / span;
            int32_t next = N[(idx + 1u) % 8u], me = N[idx % 8u], second;
            ok = j < r2.n && r2.e[j].at == base.e[i].at && r2.e[j].note == base.e[i].note;
            j++;
            if (!ok || !idx || !(st % spb)) continue;
            ok = j < r2.n && near(r2.e[j].at, base.e[i].at + span / 2u);
            if (!ok) break;
            second = r2.e[j].note;
            ok = next < me ? second < me && second >= me - 2 : second > me && second <= me + 2;
            j++;
        }
        printf("    DIV %u: %u written, %u played\n", DIVS[d], base.n, r2.n);
        bad += check("STEP track, 100 % DOUBLE: the note, then a degree toward the next written note, at half the step (1/8, 1/16, 8T)",
                     ok && j == r2.n && r2.n > base.n);
        step_setup(100, TESS_SPLIT_PUSH, DIVS[d]);
        run(0, 8, &r2);
        ok = 1;
        for (i = j = 0; ok && i < r2.n; i++)
            ok = on_line(r2.e[i].at, span) ? !(step_of(r2.e[i].at, span) % 16u) || !(step_of(r2.e[i].at, span) % ((4u * beat_samples() + span / 2u) / span)) :
                 near(r2.e[i].at % span, span / 2u);
        bad += check("STEP track, 100 % PUSH: on the half, the bar's first on its line (1/8, 1/16, 8T)", ok && r2.n >= base.n - 1u);
    }
    span = span_of(DIV_8TH);

    step_setup(100, TESS_SPLIT_DOUBLE, DIV_8TH);        /* steps left as written: 2 x2, 3 slides into 4 tied, 5 early, 6 rest */
    step_set_ratch(&trk[0].step[2], 2);
    trk[0].step[3].flags |= SF_SLIDE;
    trk[0].step[4].time = ST_TIE;
    step_set_nudge(&trk[0].step[5], -4);
    trk[0].step[6].time = ST_REST;
    for (i = 0; i < 16u; i++) tessera_param_set(0, TX_SUB_SPLIT, 100);
    run(0, 8, &r2);
    tessera_param_set(0, TX_SUB_SPLIT, 0);
    run(0, 8, &base);
    ok = 1;
    {
        uint32_t a = 0, b = 0;
        for (;;) {                                       /* (the steps 2..6 of the loop, the same in both) */
            while (a < base.n && (step_of(base.e[a].at, span) % 16u < 2u || step_of(base.e[a].at, span) % 16u > 6u)) a++;
            while (b < r2.n && (step_of(r2.e[b].at, span) % 16u < 2u || step_of(r2.e[b].at, span) % 16u > 6u)) b++;
            if (a >= base.n || b >= r2.n) { ok = ok && a >= base.n && b >= r2.n; break; }
            ok = ok && base.e[a].at == r2.e[b].at && base.e[a].note == r2.e[b].note;
            a++; b++;
        }
    }
    bad += check("a ratchet, a slide, a tie, an early Nudge and a rest: left as written", ok);

    step_setup(100, TESS_SPLIT_PUSH, DIV_8TH);          /* a late Nudge: its 4/16 and the push add up */
    step_set_nudge(&trk[0].step[3], 4);
    run(0, 4, &r2);
    ok = 0;
    for (i = 0; i < r2.n; i++)
        if (step_of(r2.e[i].at, span) == 3u) ok = near(r2.e[i].at, 3u * span + span * 3u / 4u);
    bad += check("PUSH on a late Nudge: 4/16 and half a step", ok);
    step_setup(100, TESS_SPLIT_PUSH, DIV_8TH);
    step_set_nudge(&trk[0].step[3], 7);
    run(0, 4, &r2);
    ok = 0;
    for (i = 0; i < r2.n; i++)
        if (step_of(r2.e[i].at, span) == 3u) ok = near(r2.e[i].at, 3u * span + span * 15u / 16u);
    bad += check("PUSH on a late Nudge: no later than 15/16 of the step, its next line clear", ok);

    step_setup(100, TESS_SPLIT_DOUBLE, DIV_8TH);        /* a pattern of 5 steps: its first and the bar's stay */
    trk[0].p[P_SLEN] = 5;
    run(0, 8, &r2);
    ok = 1;
    for (i = 0; ok && i < r2.n; i++)
        if (!on_line(r2.e[i].at, span))
            ok = step_of(r2.e[i].at, span) % 5u && step_of(r2.e[i].at, span) % 8u;
    bad += check("the pattern's first step and the bar's first never split", ok && r2.n > 16u);

    step_setup(30, TESS_SPLIT_DOUBLE, DIV_8TH);
    run(0, 32, &r2);
    step_setup(30, TESS_SPLIT_DOUBLE, DIV_8TH);
    run(0, 32, &r3);
    bad += check("the tosses replay the same from PLAY", same(&r2, &r3));
    c30 = 0;
    for (i = 0; i < r2.n; i++) c30 += !on_line(r2.e[i].at, span);
    printf("    SPLIT 30: %u of 56 steps split\n", c30);
    bad += check("SPLIT 30: about that share of the steps", c30 >= 8u && c30 <= 26u);
    step_setup(30, TESS_SPLIT_DOUBLE, DIV_8TH);
    tessera.seed[0] = 12;
    run(0, 32, &r3);
    bad += check("another SEED: other tosses", !same(&r2, &r3));

    step_setup(0, 0, DIV_8TH);                          /* each half gets half the step's gate; PUSH too */
    trk[0].p[P_SGATE] = 64;
    run(0, 4, &base);
    step_setup(100, TESS_SPLIT_DOUBLE, DIV_8TH);
    trk[0].p[P_SGATE] = 64;
    run(0, 4, &r2);
    step_setup(100, TESS_SPLIT_PUSH, DIV_8TH);
    trk[0].p[P_SGATE] = 64;
    run(0, 4, &r3);
    printf("    gates: written %u, DOUBLE %u %u, PUSH %u\n", base.off[1], r2.off[1], r2.off[2], r3.off[1]);
    bad += check("gates: a split step's hits, and a pushed note, half the step's gate",
                 base.off[1] && near(r2.off[1], base.off[1] / 2u) && near(r2.off[2], base.off[1] / 2u) && near(r3.off[1], base.off[1] / 2u));

    step_setup(0, 0, DIV_8TH);
    run(0, 8, &base);
    step_setup(100, TESS_SPLIT_MIX, DIV_8TH);
    song.rec = 1;
    run(0, 8, &r2);
    song.rec = 0;
    bad += check("while the track records: left as written", same(&base, &r2));

    {
        cord_t c = {0};
        step_setup(0, TESS_SPLIT_PUSH, DIV_8TH);
        c.dst = CD_SUB_SPLIT; c.dst_trk = 0; c.offset = 100;   /* (a fixed mod: all the way) */
        cord_set(0, &c);
        run(0, 8, &r2);
        c30 = 0;
        for (i = 0; i < r2.n; i++) c30 += !on_line(r2.e[i].at, span);
        bad += check("a cord on SPLIT brings it in with the knob at 0", c30 > 8u && tessera.sub_split[0] == 0u);
        cord_clear();
        run(0, 8, &r2);
        bad += check("  and without the cord, nothing", same(&base, &r2));
    }
    return bad;
}

static int drum_test(void)
{
    int bad = 0, ok;
    uint32_t d = NTRK, i, j, span = span_of(DIV_8TH), found;
    reset();
    for (i = 0; i < NTRK; i++) if (drum_track(&trk[i])) { d = i; break; }
    if (d >= NTRK) return check("a drum track for the test", 0);
    reset();
    for (i = 0; i < 16u; i++) trk[d].step[i].hit = 1u, trk[d].step[i].time = ST_NOTE;
    tessera_param_set(d, TX_SUB_SPLIT, 100);
    run(d, 8, &r2);
    ok = r2.n > 16u;
    for (i = 1; ok && i < r2.n; i++) ok = r2.e[i].note == r2.e[0].note;
    bad += check("a drum hit: DOUBLE plays the same hit again", ok);

    song.sel = (uint8_t)d;
    reset();
    song.sel = (uint8_t)d;
    trk[d].p[P_SDIV] = DIV_8TH;
    tessera_knob(0, TESS_ENG_GRIDS);
    trk[d].p[P_SDIV] = DIV_8TH;
    tessera.seed[d] = 7;
    run(d, 8, &base);
    reset();
    song.sel = (uint8_t)d;
    trk[d].p[P_SDIV] = DIV_8TH;
    tessera_knob(0, TESS_ENG_GRIDS);
    trk[d].p[P_SDIV] = DIV_8TH;
    tessera.seed[d] = 7;
    tessera_param_set(d, TX_SUB_SPLIT, 100);
    run(d, 8, &r2);
    ok = r2.n > base.n && base.n > 4u;
    for (i = 0; ok && i < r2.n; i++) {                   /* (each note: as ATLAS played it, or that one halfway on) */
        found = 0;
        for (j = 0; j < base.n && !found; j++)
            found = base.e[j].note == r2.e[i].note &&
                    (base.e[j].at == r2.e[i].at || near(r2.e[i].at, base.e[j].at + span / 2u));
        ok = found;
    }
    bad += check("ATLAS: DOUBLE plays the same hits again, its pattern as it was", ok);
    return bad;
}

int main(void)
{
    int bad;
    reset();                                            /* (the tempo: span_of) */
    bad = generator_test() + step_track_test() + drum_test();
    printf("%s\n", bad ? "SPLIT TEST FAILED" : "SPLIT tests passed");
    return bad != 0;
}
