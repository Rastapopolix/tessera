/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* PIECE (firmware/src/planner.c): the piece planner through the real sequencer and generators (hostsim.c).
 *   the ten SHAPEs: in range, where they peak, sparse ends but RISE's and FALL's, COUNT and PEAK;  LENGTH FREE with
 *   every other setting turned plays the same notes, block for block;  ENERGY's tide is the SHAPE (ARC its swing);
 *   Entries follows it: the intro layers in, the outro out, and the top has every track;  the CHAIN: A for good in the
 *   last 15 %, a new section's CONTRAST higher near the top;  END STOP (everything out, then stopped), LOOP (the same
 *   piece again), NEW (new SEEDs), FREE (unplanned on);  a tempo change moves the plan;  the PIECE knobs. */
#define TESSERA_NOTE_HOOK pt_hook
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
static void check(const char *what, int ok)
{
    printf("piece: %-100s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) fails++;
}

static uint32_t pt_blk, pt_hash, pt_n;
static void pt_hook(track_t *t, uint32_t note, uint32_t vel)   /* the sequencer's notes */
{
    if (!vel) return;
    pt_hash = (pt_hash ^ (pt_blk * 131u + note * 7u + trk_index(t))) * 16777619u;
    pt_n++;
}
static void sel(uint32_t tr) { song.sel = (uint8_t)tr; }
static void gen(uint32_t tr, uint32_t mode) { sel(tr); tessera_knob(0, (int32_t)mode); }
static void px(uint32_t tr, uint32_t x, int32_t v) { tessera_param_set(tr, x, v); }
enum { FM_OFF, FM_AB, FM_ABAC, FM_ABCB, FM_AABA, FM_ABCA, FM_AABC };

static void power_on(uint32_t seed)
{
    host_tracks_init();
    for (uint32_t k = 0; k < NTRK; k++) {
        memset(trk[k].v, 0, sizeof trk[k].v);
        trk[k].nmono = 0;
    }
    tessera_reset();
    song.g[G_BPM] = 120;
    rng_state = seed;
    pt_blk = 0;
}
static void play(void) { transport_req = 1; }
static void block(void) { events_block(CTL); cord_tick(); pt_blk++; }
static uint32_t bar_blocks(void) { return 4u * beat_samples() / CTL; }
static uint32_t bits(uint32_t m) { uint32_t n = 0; for (; m; m &= m - 1u) n++; return n; }

static void p_four(void)
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_ET_ROLE, ET_BASS);
    gen(1, TESS_ENG_EUCLID);
    px(1, TX_ET_ROLE, ET_RHYTHM);
    gen(2, TESS_ENG_CHORD);
    gen(3, TESS_ENG_MARKOV);
    px(3, TX_ET_ROLE, ET_MELODY);
}

static void test_shapes(void)
{
    static const char *const NM[PL_SHAPES] = {"ARCH", "RISE", "FALL", "PLATEAU", "VALLEY", "WAVES", "BUILDS", "STEPS", "FLAT", "USER"};
    uint32_t sh, x, range_bad = 0, ends_bad = 0;
    power_on(1);
    for (sh = 0; sh < PL_SHAPES; sh++) {
        int32_t lo = 32767, hi = 0, y0, y1, ymax = -1;
        uint32_t xmax = 0;
        tessera.pl_shape = (uint8_t)sh;
        for (x = 0; x <= 65536u; x += 256u) {
            int32_t y = pl_shape(x);
            if (y < lo) lo = y;
            if (y > hi) hi = y;
            if (y > ymax) ymax = y, xmax = x;
        }
        y0 = pl_shape(0), y1 = pl_shape(65536u);
        printf("piece:   %-8s %5d..%5d, start %5d, end %5d, top at %u %%\n", NM[sh], lo, hi, y0, y1, xmax * 100u / 65536u);
        if (lo < 0 || hi > 32767 || hi < 16384) range_bad++;
        if (sh == PL_RISE ? !(y0 < 8192 && y1 > 30000) : sh == PL_FALL ? !(y0 > 30000 && y1 < 8192) :
            sh == PL_USER ? 0 : sh == PL_STEPS ? !(y0 < 16384 && y1 < 16384) : !(y0 < 4096 && y1 < 4096))
            ends_bad++;
    }
    check("every SHAPE inside 0..1 and reaching above a half", range_bad == 0u);
    check("each begins and ends sparse (the intro, the outro), but RISE (ends full) and FALL (opens full)", ends_bad == 0u);
    tessera.pl_shape = PL_ARCH;
    tessera.pl_peak = 30;
    {
        uint32_t best = 0;
        int32_t ym = -1;
        for (x = 0; x <= 65536u; x += 256u)
            if (pl_shape(x) > ym) ym = pl_shape(x), best = x;
        check("ARCH's top at its PEAK (30 %)", best * 100u / 65536u >= 28u && best * 100u / 65536u <= 32u);
    }
    tessera.pl_shape = PL_WAVES;
    for (uint32_t c = 2; c <= 8u; c += 3u) {
        uint32_t tops = 0;
        int32_t a = pl_shape(0), b2 = pl_shape(256);
        tessera.pl_count = (uint8_t)c;
        for (x = 512; x <= 65536u; x += 256u) {
            int32_t y = pl_shape(x);
            if (b2 > a && b2 >= y) tops++;
            a = b2, b2 = y;
        }
        printf("piece:   WAVES COUNT %u: %u swells\n", c, tops);
        check("WAVES: COUNT swells", tops == c);
    }
    tessera.pl_shape = PL_STEPS;
    tessera.pl_count = 4;
    {
        uint32_t levels = 0;
        int32_t seen[16], y;
        for (x = 0; x <= 65536u; x += 256u) {
            uint32_t i;
            y = pl_shape(x);
            for (i = 0; i < levels && seen[i] != y; i++)
                ;
            if (i == levels && levels < 16u) seen[levels++] = y;
        }
        check("STEPS 4: four terraces", levels == 4u);
    }
    tessera.pl_shape = PL_USER;
    px(0, TX_PL_USER3, 0);
    px(0, TX_PL_USER8, 100);
    check("USER: its levels, evenly along (point 3 at 2/7, point 8 at the end)", pl_shape(2u * 65536u / 7u) < 400 && pl_shape(65536u) == 32767);
}

static uint32_t notes_of(int turned, uint32_t *n)
{
    power_on(3);
    pt_hash = 2166136261u, pt_n = 0;
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_EN_ARC, 50);
    px(0, TX_ET_AMOUNT, 60);
    if (turned) {
        px(0, TX_PL_SHAPE, PL_WAVES);
        px(0, TX_PL_PEAK, 20);
        px(0, TX_PL_COUNT, 6);
        px(0, TX_PL_END, PL_END_NEW);
        px(0, TX_PL_USER2, 0);
    }
    play();
    for (uint32_t b = 0; b < 48u * bar_blocks(); b++) block();
    *n = pt_n;
    return pt_hash;
}
static void test_free(void)
{
    uint32_t n0, n1, h0 = notes_of(0, &n0), h1 = notes_of(1, &n1);
    printf("piece:   %u notes 0x%08x / %u notes 0x%08x\n", n0, h0, n1, h1);
    check("LENGTH FREE: every other PIECE setting turned plays the same notes, block for block", n0 > 100u && h0 == h1 && n0 == n1);
}

/* a planned piece of `mins` minutes at 120 BPM (8-bar phrases: 16 s), ENTRIES 100, ARCH at 50 %: the tracks out each
 * phrase, the CHAIN's letter, ENERGY */
#define PT_MAXP 256u
static uint8_t ph_out[PT_MAXP], ph_letter[PT_MAXP], ph_moved[PT_MAXP], ph_extra[PT_MAXP];
static int32_t ph_energy[PT_MAXP];
static uint32_t pt_phrases, pt_stopped_at;
static void run_piece(uint32_t mins, uint32_t shape, uint32_t end, uint32_t extra_phrases)
{
    uint32_t bars = 8u, p, guard;
    power_on(5);
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 1);
    px(0, TX_EN_ARC, 100);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    px(0, TX_PL_SHAPE, (int32_t)shape);
    px(0, TX_PL_PEAK, 50);
    px(0, TX_PL_END, (int32_t)end);
    for (p = 0; p < PL_LENGTHS && PL_MIN[p] != mins; p++)
        ;
    px(0, TX_PL_LENGTH, (int32_t)p);
    play();
    pt_phrases = pl_beats() / 32u + extra_phrases;
    pt_stopped_at = 0;
    memset(ph_out, 0, sizeof ph_out);
    memset(ph_letter, 0, sizeof ph_letter);
    for (p = 0, guard = 0; p < pt_phrases && p < PT_MAXP && guard < pt_phrases * 8u * bar_blocks() + 4u * bar_blocks(); guard++) {
        uint32_t pb = bar_n;
        block();
        if (!song.playing) {
            if (!pt_stopped_at) pt_stopped_at = pt_blk;
            continue;
        }
        if (bar_n != pb && bar_n && bar_n % bars == 0u) {   /* (the phrase's downbeat: read in the block after the line) */
            const fc_st_t *s = fc_at(bar_n / bars);
            p = bar_n / bars > p ? bar_n / bars : p + 1u;   /* (counted on: a LOOP starts its bars again) */
            if (p < PT_MAXP) {
                ph_letter[p] = (uint8_t)fc_letter_at(s->pos);
                ph_moved[p] = (uint8_t)(s->moved & 1u);
                ph_extra[p] = s->extra;
            }
        }
        if (bar_n % bars == bars / 2u && beat_n == 0u && p < PT_MAXP) {   /* (mid-phrase: Entries and ENERGY settled) */
            ph_out[p] = (uint8_t)tessera_entries_get(0, 0);
            ph_energy[p] = energy_now();
        }
    }
}

static void test_arch(void)
{
    uint32_t p, n, top, home_bad = 0, intro_ok, outro_ok, top_full;
    run_piece(10, PL_ARCH, PL_END_FREE, 4);
    n = pt_phrases - 4u, top = n / 2u;
    printf("piece:   ARCH, 10 min: out ");
    for (p = 0; p < n + 4u; p++) printf("%u", bits(ph_out[p]));
    printf("\npiece:   letters    ");
    for (p = 0; p < n + 4u; p++) printf("%c", 'A' + ph_letter[p]);
    printf("\n");
    intro_ok = bits(ph_out[0]) >= 2u && bits(ph_out[1]) >= bits(ph_out[4]);
    top_full = bits(ph_out[top]) == 0u;
    outro_ok = bits(ph_out[n - 1u]) >= 2u;
    check("ARCH: the intro starts thin and layers in", intro_ok);
    check("ARCH: the top (PEAK 50 %) has every track", top_full);
    check("ARCH: the outro thins out again", outro_ok);
    for (p = n * 86u / 100u + 1u; p < n; p++) home_bad += ph_letter[p] != 0u;
    check("the CHAIN: A for good through the piece's last 15 %", home_bad == 0u);
    {                                                 /* a 2-hour piece: home its last 12 phrases */
        uint8_t len = tessera.pl_length, done = plan.done;
        uint32_t all;
        tessera.pl_length = 19u, plan.done = 0;
        all = pl_beats() / (fc_bars() * 4u);
        check("a long piece's home: its last 12 phrases, not its last 15 %",
              all > 200u && !planner_home(all - 13u) && planner_home(all - 12u) && planner_home(all - 1u));
        tessera.pl_length = len, plan.done = done;
    }
    check("ENERGY's tide is the SHAPE: low at the ends, high at the top",
          ph_energy[0] < -16000 && ph_energy[top] > 16000 && ph_energy[n - 1u] < -8000);
    check("END FREE: past the end, still playing, the tide unplanned", song.playing && pt_stopped_at == 0u);
    {
        uint32_t near = 0, far = 0, nn = 0, nf = 0;
        for (p = 1; p < n * 85u / 100u; p++)
            if (ph_moved[p] && ph_letter[p]) {
                if (p > top / 2u && p < top + top / 2u) near += ph_extra[p], nn++;
                else far += ph_extra[p], nf++;
            }
        check("new sections depart further near the top (CONTRAST's extra)", nn && nf && near * nf > far * nn);
    }
}

static void test_end(void)
{
    uint32_t seed0;
    run_piece(2, PL_ARCH, PL_END_STOP, 6);
    check("END STOP: the piece stops after its LENGTH and the tails", pt_stopped_at != 0u &&
          pt_stopped_at > 2u * 60u * FS / CTL && pt_stopped_at < 2u * 60u * FS / CTL + 12u * bar_blocks());
    run_piece(2, PL_ARCH, PL_END_LOOP, 3);
    check("END LOOP: plays on from the top (the bar count again from 0)", song.playing && bar_n < 30u);
    power_on(6);
    seed0 = tessera.seed[0];
    run_piece(2, PL_ARCH, PL_END_NEW, 3);
    check("END NEW: new SEEDs, from the top", song.playing && bar_n < 30u && tessera.seed[0] != seed0);
}

static void test_tempo(void)
{
    uint32_t x120, x60, l;
    power_on(7);
    p_four();
    px(0, TX_PL_LENGTH, 5);                           /* (5 minutes) */
    play();
    for (uint32_t b = 0; b < 16u * bar_blocks(); b++) block();   /* (16 bars at 120: 32 s) */
    (void)tessera_piece_get(&x120, &l);
    song.g[G_BPM] = 60;
    block();
    (void)tessera_piece_get(&x60, &l);
    printf("piece:   at bar 16: %u %% at 120 BPM, %u %% at 60\n", x120 * 100u / 65536u, x60 * 100u / 65536u);
    check("a tempo change moves the plan (the same bars, twice the time at half the tempo)", x60 > x120 * 3u / 2u);
}

static void test_knobs(void)
{
    power_on(8);
    check("defaults: FREE, ARCH, PEAK 62, COUNT 3, STOP", !tessera.pl_length && tessera.pl_shape == PL_ARCH && tessera.pl_peak == 62u &&
          tessera.pl_count == 3u && tessera.pl_end == PL_END_STOP);
    tessera_piece_knob(0, 3, 0);
    tessera_piece_knob(1, 1, 0);                      /* (RISE: a step a detent) */
    tessera_piece_knob(2, 1, 0);
    tessera_piece_knob(3, -1, 0);
    check("PIECE knobs: LENGTH, SHAPE a step a detent, KNOB 3 PEAK, END",
          PL_MIN[tessera.pl_length] == 3u && tessera.pl_shape == PL_RISE && tessera.pl_peak == 63u && tessera.pl_end == PL_END_STOP);
    tessera_piece_knob(1, 4, 0);
    tessera_piece_knob(2, 1, 0);
    check("KNOB 3 is COUNT on WAVES", tessera.pl_shape == PL_WAVES && tessera.pl_count == 4u);
    tessera_piece_knob(1, 1, 2);
    tessera_piece_knob(3, -5, 1);
    check("OCT- held: USER 5-8; OCT+ held: USER 1-4", tessera.pl_user[5] == 77u && tessera.pl_user[3] == 70u);
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    test_knobs();
    test_shapes();
    test_free();
    test_arch();
    test_end();
    test_tempo();
    if (fails) {
        printf("piece: %d FAILED\n", fails);
        return 1;
    }
    printf("piece tests passed\n");
    return 0;
}
