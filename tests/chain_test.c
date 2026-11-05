/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* FORM CHAIN (firmware/src/form_chain.c): the project's FORM on the bar clock, through the real sequencer and
 * generators (hostsim.c), notes only (events_block, as tests/longform_test.c).
 *   the order of the letters and the REPEAT counts, WANDER 0;  the change on the phrase's first step, the same block
 *   for tracks of different DIVs;  FOLLOW / OWN and its default;  an OWN track plays as without the CHAIN;  SKIP holds a
 *   phrase, never two;  WANDER: the same piece from PLAY, A comes back, a new letter only after A, CONTRAST's extra;
 *   OFF with every other CHAIN setting turned: the notes the same as without, block for block. */
#define TESSERA_NOTE_HOOK ct_note
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
static void check(const char *what, int ok)
{
    printf("chain: %-100s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) fails++;
}

static uint32_t ct_hash, ct_n, ct_blk, ct_track_hash[NTRK];
#define CT_REC 4096u                                   /* (KEY B / C: every note, its block and bar) */
typedef struct { uint32_t blk, bar; uint8_t tr, note; } ct_rec_t;
static ct_rec_t ct_rec[CT_REC];
static uint32_t ct_rec_n;
static void ct_note(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t tr = trk_index(t);
    if (!vel) return;
    ct_hash = (ct_hash ^ (ct_blk * 131u + note * 7u + tr)) * 16777619u;
    if (tr < NTRK) ct_track_hash[tr] = (ct_track_hash[tr] ^ (ct_blk * 131u + note)) * 16777619u;
    if (ct_rec_n < CT_REC) ct_rec[ct_rec_n++] = (ct_rec_t){ct_blk, bar_n, (uint8_t)tr, (uint8_t)note};
    ct_n++;
}

static void sel(uint32_t tr) { song.sel = (uint8_t)tr; }
static void gen(uint32_t tr, uint32_t mode) { sel(tr); tessera_knob(0, (int32_t)mode); }
static void px(uint32_t tr, uint32_t x, int32_t v) { tessera_param_set(tr, x, v); }
enum { FM_OFF, FM_AB, FM_ABAC, FM_ABCB, FM_AABA, FM_ABCA, FM_AABC };

static void power_on(uint32_t seed)
{
    host_tracks_init();
    tessera_reset();
    song.g[G_BPM] = 120;
    rng_state = seed;
    ct_hash = 2166136261u, ct_n = 0, ct_blk = 0, ct_rec_n = 0;
    memset(ct_track_hash, 0, sizeof ct_track_hash);
}
static void play(void) { transport_req = 1; }
static void block(void) { events_block(CTL); cord_tick(); ct_blk++; }
static uint32_t bar_blocks(void) { return 4u * beat_samples() / CTL; }

/* each phrase's section of the tracks in mask, read in the block the phrase's first step plays in (the bar line falls
 * at a block's end, its step in the next), for `phrases` phrases of `bars` bars; *stray: a section changed in any
 * other block */
#define CT_MAXP 64u
static uint8_t ct_sec[CT_MAXP][TESSERA_TRACKS];
static void run_phrases(uint32_t phrases, uint32_t bars, uint32_t mask, uint32_t *stray)
{
    uint32_t tr, was[TESSERA_TRACKS], first, pbar, ppos;
    *stray = 0;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) was[tr] = tessera.evolve[tr].sec;
    while (1) {
        pbar = bar_n, ppos = beat_pos;
        block();
        if (bar_n / bars >= phrases) break;
        /* (the block the bar's first step plays in: the line crossed in it, or it began on the line) */
        first = bar_n % bars == 0u && beat_n == 0u && (pbar != bar_n || ppos == 0u);
        for (tr = 0; tr < TESSERA_TRACKS; tr++) {
            if (!(mask >> tr & 1u)) continue;
            if (tessera.evolve[tr].sec != was[tr] && !first) (*stray)++;
            was[tr] = tessera.evolve[tr].sec;
        }
        if (first && bar_n / bars < CT_MAXP)
            for (tr = 0; tr < TESSERA_TRACKS; tr++) ct_sec[bar_n / bars][tr] = tessera.evolve[tr].sec;
    }
}
static int seq_is(uint32_t tr, const char *want)      /* "AABAC...": the phrases' letters from phrase 0 */
{
    for (uint32_t p = 0; want[p]; p++)
        if (ct_sec[p][tr] != (uint8_t)(want[p] - 'A')) {
            printf("chain:   track %u phrase %u: %c, want %c\n", tr + 1u, p, 'A' + ct_sec[p][tr], want[p]);
            return 0;
        }
    return 1;
}

/* the notes of `bars` bars from PLAY */
static uint32_t notes_of(void (*setup)(void), uint32_t bars, uint32_t *n, uint32_t tr_hash[NTRK])
{
    power_on(0xC4A1u);
    setup();
    play();
    for (uint32_t b = 0; b < bars * bar_blocks(); b++) block();
    *n = ct_n;
    if (tr_hash) memcpy(tr_hash, ct_track_hash, sizeof ct_track_hash);
    return ct_hash;
}

static void p_base(void)
{
    gen(0, TESS_ENG_TURING);
    gen(1, TESS_ENG_EUCLID);
    px(1, TX_EV_EVERY, 3);
    gen(2, TESS_ENG_MARKOV);
    px(2, TX_FM_FORM, FM_ABAC);
    px(2, TX_EV_EVERY, 2);
    gen(3, TESS_ENG_MORPH);
    px(3, TX_FM_DEVELOP, 30);
}
static void p_off_turned(void)                        /* FORM OFF, everything else turned */
{
    p_base();
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_WANDER, 80);
    px(0, TX_FC_REP1, 3);
    px(0, TX_FC_REP3, 5);
    for (uint32_t tr = 0; tr < TESSERA_TRACKS; tr++) {
        px(tr, TX_FC_SKIP, 50);
        px(tr, TX_FC_FOLLOW, tr & 1u);
    }
}
static void p_chain_on(void)                          /* the CHAIN on: track 3 (own FORM) is OWN by default */
{
    p_base();
    px(0, TX_FC_FORM, FM_ABCB);
    px(0, TX_FC_PHRASE, 0);
}

static void test_off(void)
{
    uint32_t n0, n1, h0 = notes_of(p_base, 96, &n0, 0), h1 = notes_of(p_off_turned, 96, &n1, 0);
    printf("chain:   %u notes, 0x%08x / %u notes, 0x%08x\n", n0, h0, n1, h1);
    check("FORM OFF: every other CHAIN setting turned plays the same notes, block for block", n0 > 100u && h0 == h1 && n0 == n1);
}

static void test_own_untouched(void)
{
    uint32_t n0, n1, a[NTRK], b[NTRK];
    notes_of(p_base, 96, &n0, a);
    notes_of(p_chain_on, 96, &n1, b);
    check("an OWN track (its own FORM, the default) plays as without the CHAIN", a[2] == b[2] && a[2]);
    check("the following tracks play otherwise", a[0] != b[0] && a[3] != b[3]);
}

static void test_follow_default(void)
{
    power_on(1);
    p_base();
    check("FORM OFF: nobody follows", !fc_follows(0) && !fc_follows(2));
    px(0, TX_FC_FORM, FM_ABAC);
    check("FOLLOW not set: a generator with its own FORM OFF follows, one with a FORM is OWN, a STEP track neither",
          fc_follows(0) && fc_follows(1) && !fc_follows(2) && fc_follows(3) && tessera_param_get(2, TX_FC_FOLLOW) == 1 &&
          tessera_param_get(0, TX_FC_FOLLOW) == 0);
    px(2, TX_FC_FOLLOW, 0);
    px(0, TX_FC_FOLLOW, 1);
    check("set by hand: FOLLOW over its own FORM, OWN without one", fc_follows(2) && !fc_follows(0));
    px(0, TX_MODE, TESS_ENG_OFF);
    px(0, TX_FC_FOLLOW, 0);
    check("a STEP track never follows", !fc_follows(0));
}

/* WANDER 0, the counts: ABAC with A x2, C x3 */
static void p_order(void)
{
    gen(0, TESS_ENG_TURING);
    gen(1, TESS_ENG_EUCLID);
    trk[1].p[P_SDIV] = (int16_t)(trk[1].p[P_SDIV] + 1);   /* (another DIV: its own steps a bar) */
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_REP1, 2);
    px(0, TX_FC_REP4, 3);
}
static void test_order(void)
{
    uint32_t stray;
    power_on(2);
    p_order();
    check("the two tracks count different steps a bar", tess_steps_per_bar(&trk[0]) != tess_steps_per_bar(&trk[1]));
    play();
    run_phrases(16, 4, 3u, &stray);
    check("ABAC, A x2, C x3, 4-bar phrases: AABACCC round and round", seq_is(0, "AABACCCAABACCCAA"));
    check("a track of another DIV changes with it", seq_is(1, "AABACCCAABACCCAA"));
    check("each change on the phrase's first step (the block its downbeat plays in), never between", stray == 0u);
    check("the sections are the track's own: B and C made once, A recalled", (tessera.evolve[0].visited & 7u) == 7u);
}

static void test_skip(void)
{
    uint32_t stray;
    power_on(3);
    gen(0, TESS_ENG_TURING);
    gen(1, TESS_ENG_TURING);
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(1, TX_FC_SKIP, 100);
    play();
    run_phrases(12, 4, 3u, &stray);
    check("SKIP 0 moves with the CHAIN", seq_is(0, "ABACABACABAC"));
    {
        uint32_t p, held = 0, twice = 0, behind = 0;
        for (p = 1; p < 12u; p++) {
            int lag = ct_sec[p][1] != ct_sec[p][0];
            held += lag;
            if (lag && p > 1u && ct_sec[p - 1u][1] != ct_sec[p - 1u][0]) twice++;
            if (lag && ct_sec[p][1] != ct_sec[p - 1u][0]) behind++;   /* (a lag is the letter before) */
        }
        printf("chain:   SKIP 100: %u phrases held of 11\n", held);
        check("SKIP 100: half the moves held (every other), each one the letter before, none twice running",
              held >= 5u && held <= 6u && !twice && !behind && stray == 0u);
    }
}

/* the letters the project plays at WANDER w, from fc_at (the same as the tracks: see test_wander_tracks) */
static void letters(uint32_t form, uint32_t w, uint32_t seed, uint32_t n, uint8_t *out, uint8_t *extra)
{
    power_on(seed);
    gen(0, TESS_ENG_TURING);
    tessera.seed[0] = (uint16_t)seed;
    px(0, TX_FC_FORM, (int32_t)form);
    px(0, TX_FC_WANDER, (int32_t)w);
    fchain_play();
    for (uint32_t p = 0; p < n; p++) {
        const fc_st_t *s = fc_at(p);
        out[p] = (uint8_t)fc_letter_at(s->pos);
        if (extra) extra[p] = (s->moved & 1u) ? s->extra : 0u;
    }
}
static void test_wander(void)
{
    enum { N = 2000 };
    static uint8_t a[N], b[N], w0[N], x[N], c[N];
    uint32_t p, same = 1, gapmax = 0, gap = 0, newbad = 0, hold_bad = 0, drawn_diff = 0, maxextra = 0, extra_on_a = 0;
    uint32_t home = 1, counts[3] = {0, 0, 0}, seeds_differ = 0;
    letters(FM_ABAC, 60, 7, N, a, x);
    letters(FM_ABAC, 60, 7, N, b, 0);
    letters(FM_ABAC, 0, 7, N, w0, 0);
    letters(FM_ABAC, 60, 8, N, c, 0);
    for (p = 0; p < N; p++) {
        same &= a[p] == b[p];
        drawn_diff += a[p] != w0[p];
        seeds_differ += a[p] != c[p];
        counts[a[p]]++;
        gap = a[p] ? gap + 1u : 0u;
        if (gap > gapmax) gapmax = gap;
        if (p && a[p] == a[p - 1u]) hold_bad++;       /* (ABAC repeats no letter, counts 1: never twice) */
        if (x[p] > maxextra) maxextra = x[p];
        if (!a[p] && x[p]) extra_on_a++;
    }
    {   /* a letter not heard yet only after a return to A since the last new one */
        uint32_t heard = 1, ok_new = 1;
        for (p = 1; p < N; p++) {
            if (!(heard >> a[p] & 1u)) {
                if (!ok_new) newbad++;
                heard |= 1u << a[p], ok_new = 0;
            }
            if (!a[p] && a[p - 1u]) ok_new = 1;
        }
    }
    for (p = 0; p < 8u; p++) home &= w0[p] == (uint8_t)"ABAC"[p % 4u] - 'A';
    printf("chain:   WANDER 60, %u phrases: A %u, B %u, C %u; longest away %u; %u differ from WANDER 0\n", N,
           counts[0], counts[1], counts[2], gapmax, drawn_diff);
    check("WANDER 0 reads the FORM", home);
    check("WANDER 60: the same letters from PLAY each time (seeded)", same);
    check("WANDER 60 draws (not the written order), another SEED another path", drawn_diff > N / 10u && seeds_differ > N / 10u);
    check("WANDER 60: A comes back (never 8 phrases away), every letter played", gapmax < 8u && counts[1] && counts[2]);
    check("WANDER 60: no letter twice where the FORM never repeats it", hold_bad == 0u);
    check("WANDER 60: a new letter only after a return to A", newbad == 0u);
    check("CONTRAST's extra: up to 30, none on A", maxextra <= 30u && !extra_on_a);
    {   /* AABC: A may repeat, C after B is a further phrase away: +10 */
        uint32_t aa = 0, ten = 0;
        letters(FM_AABC, 0, 7, 16, a, x);
        for (p = 0; p < 16u; p++) ten += a[p] == 2u && x[p] == 10u;
        letters(FM_AABC, 60, 7, N, b, 0);
        for (p = 1; p < N; p++) aa += !b[p] && !b[p - 1u];
        check("AABC: C, a phrase further from A, gets +10; with WANDER, A may repeat", ten == 4u && aa > 0u);
    }
}

static void test_wander_tracks(void)
{
    uint32_t stray, p, ok = 1;
    uint8_t want[24];
    letters(FM_ABCA, 70, 9, 24, want, 0);
    power_on(9);
    gen(0, TESS_ENG_TURING);
    tessera.seed[0] = 9u;
    px(0, TX_FC_FORM, FM_ABCA);
    px(0, TX_FC_WANDER, 70);
    px(0, TX_FC_PHRASE, 0);
    play();
    run_phrases(24, 4, 1u, &stray);
    for (p = 0; p < 24u; p++) ok &= ct_sec[p][0] == want[p];
    check("WANDER 70: the following track plays the project's letters, on the phrase lines", ok && stray == 0u);
}

static void test_flourish(void)
{
    power_on(4);
    gen(0, TESS_ENG_TURING);
    px(0, TX_FC_FORM, FM_AB);
    px(0, TX_FC_PHRASE, 1);
    px(0, TX_FM_FLOURISH, 5);
    play();
    {
        uint32_t b, hit_last = 0, hit_other = 0;
        for (b = 0; b < 24u * bar_blocks(); b++) {
            block();
            if (tess_flourish(&trk[0], 0)) {
                if (bar_n % 8u == 7u || (bar_n % 8u == 0u && beat_n == 0u)) hit_last++;   /* (or the line's block) */
                else hit_other++;
            }
        }
        check("FLOURISH while following: the last beat of the CHAIN's phrase (8 bars), EVERY OFF", hit_last && !hit_other);
    }
}

static void test_stop_restart(void)
{
    uint32_t stray;
    power_on(5);
    gen(0, TESS_ENG_TURING);
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_WANDER, 50);
    px(0, TX_FC_SKIP, 30);
    play();
    run_phrases(6, 4, 1u, &stray);
    seq_stop();
    check("STOP: the follower back on A", tessera.evolve[0].sec == 0u);
    {
        uint8_t first[6];
        uint32_t p, same = 1;
        for (p = 0; p < 6u; p++) first[p] = ct_sec[p][0];
        block();
        play();
        run_phrases(6, 4, 1u, &stray);
        for (p = 0; p < 6u; p++) same &= ct_sec[p][0] == first[p];
        check("PLAY again: the same letters (WANDER, SKIP)", same);
    }
    px(0, TX_FC_FORM, FM_AB);
    check("FORM changed playing: the follower's live phrase is A again", tessera.evolve[0].sec == 0u &&
          tessera.evolve[0].visited == 1u);
}

/* KEY B / KEY C: the notes of an AB.. FORM with the keys set against the same piece with none: the same notes in the
 * same blocks, those of a B phrase (C) KEY B (KEY C) semitones up or down, A's as they were; on a following generator
 * (TURING, CHORD: its chord too), a melodic STEP track; an OWN generator never moves; the keys 0: nothing changes */
static const uint8_t KT_STEP[8] = {60, 0, 63, 65, 0, 67, 70, 0};
static void kt_setup(int32_t kb, int32_t kc)
{
    power_on(0x4B45u);
    gen(0, TESS_ENG_TURING);
    gen(2, TESS_ENG_CHORD);
    gen(3, TESS_ENG_MARKOV);
    px(3, TX_FC_FOLLOW, 1);                           /* (set: OWN) */
    for (uint32_t i = 0; i < 8u; i++) {               /* track 2: a STEP melody */
        step_t *st = &trk[1].step[i];
        st->n = KT_STEP[i] ? 1u : 0u;
        st->note[0] = KT_STEP[i];
        st->time = KT_STEP[i] ? ST_NOTE : ST_REST;
        st->vel = 100;
    }
    trk[1].p[P_SLEN] = 8;
    px(0, TX_FC_FORM, FM_ABCA);
    px(0, TX_FC_PHRASE, 0);                           /* (4 bars) */
    px(0, TX_FC_KEYB, kb);
    px(0, TX_FC_KEYC, kc);
    play();
}
static uint32_t kt_run(int32_t kb, int32_t kc, ct_rec_t *out)
{
    kt_setup(kb, kc);
    uint32_t was = 0xFFFFFFFFu;
    for (uint32_t b = 0; b < 16u * bar_blocks(); b++) {   /* (ABCA: 4 phrases of 4 bars) */
        block();
        if (seq_run_step[1] != was && trk[1].seq_n && ct_rec_n < CT_REC) {   /* (a STEP track's notes: not hooked) */
            was = seq_run_step[1];
            ct_rec[ct_rec_n++] = (ct_rec_t){ct_blk, seq_run_step[1] / tess_steps_per_bar(&trk[1]), 1u, trk[1].seq_notes[0]};
        }
    }
    memcpy(out, ct_rec, ct_rec_n * sizeof *ct_rec);
    return ct_rec_n;
}
static void test_keys(void)
{
    static ct_rec_t a[CT_REC], b[CT_REC], z[CT_REC];
    uint32_t na = kt_run(0, 0, a), nb = kt_run(5, -3, b), i, bad = 0, moved[4] = {0, 0, 0, 0}, own_moved = 0;
    check("KEY B / C: a STEP track's step plays it (not a drum track)", !drum_track(&trk[1]));
    check("KEY B / C set: as many notes, in the same blocks", na == nb && na > 100u);
    for (i = 0; i < na && i < nb; i++) {
        uint32_t ph = a[i].bar / 4u, want;
        int32_t k = ph == 1u ? 5 : ph == 2u ? -3 : 0;        /* (ABCA: B on phrase 1, C on 2) */
        if (a[i].tr == 3u) k = 0;                     /* (OWN) */
        want = (uint32_t)((int32_t)a[i].note + k);
        if (b[i].blk != a[i].blk || b[i].tr != a[i].tr || b[i].note != want) {
            if (bad++ < 4u) printf("chain:   note %u (block %u, bar %u, track %u): %u, want %u\n", i, a[i].blk, a[i].bar,
                                   a[i].tr + 1u, b[i].note, want);
        }
        if (b[i].note != a[i].note) { if (a[i].tr == 3u) own_moved++; else moved[a[i].tr]++; }
    }
    printf("chain:   KEY: moved notes T1 %u, T2 (STEP) %u, T3 (CHORD) %u\n", moved[0], moved[1], moved[2]);
    check("KEY B +5, KEY C -3: B's notes 5 up, C's 3 down, A's as before, on the phrase's first step", !bad);
    check("KEY: the following TURING, the STEP track and the CHORD all move; the OWN MARKOV never",
          moved[0] && moved[1] && moved[2] && !own_moved);
    {
        uint32_t nz = kt_run(0, 0, z), same = nz == na;
        for (i = 0; i < nz && same; i++) same = z[i].blk == a[i].blk && z[i].note == a[i].note;
        check("KEY B / C 0: the same piece", same);
    }
    kt_setup(7, 0);
    for (i = 0; i < 6u * bar_blocks(); i++) block();
    check("KEY: playing B, the saved key is untouched (ROOT, TRN as set)", trk[0].p[P_ROOT] == 0 && trk[0].p[P_TRANS] == 0 &&
          trk[1].p[P_ROOT] == 0 && tessera_key_now(0) == 7 && tessera_key_now(1) == 7 && tessera_key_now(3) == 0);
    seq_stop();
    check("KEY: stopped, no key moves", tessera_key_now(0) == 0 && tessera_key_now(1) == 0);
}

int main(void)
{
    test_off();
    test_own_untouched();
    test_follow_default();
    test_order();
    test_skip();
    test_wander();
    test_wander_tracks();
    test_flourish();
    test_stop_restart();
    test_keys();
    printf(fails ? "chain: %d FAILED\n" : "chain tests passed\n", fails);
    return fails != 0;
}
