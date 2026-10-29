/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* LONG-FORM: eight hours of a patch left playing (ROADMAP phase 6, "Long-form ambient"), through the real sequencer
 * and generators (hostsim.c), at 120 BPM. It asks what an unattended FM-1 does overnight: does a track fall silent
 * for minutes, collapse to one note, freeze into a loop that never changes, or thin out hour by hour?
 *   build/host/longform_test [HOURS] [REPORT]       HOURS 8 by default; REPORT a table of every measure (text)
 *   LF_ONLY=name                                     one patch alone
 * Each patch runs in its own process from power-on state with a fixed seed. Notes only: the sequencer's block
 * (events_block, cord_tick) without the voices, which is what makes eight hours quick; the first check plays two
 * minutes of every patch both ways (mix_block, the whole audio path) and wants the same notes, block for block.
 * Measured in windows of 4 bars (8 s), per generator track and for the mix (docs/analysis/generators.md's probe):
 *   notes a bar, first hour and last;  silent windows and the longest silent run (a held note counts as sounding);
 *   one-pitch windows (a window whose notes are all one pitch);  frozen: the longest run of windows each the same
 *   as one of the 16 before it (a loop of up to 64 bars replayed note for note);  variety: distinct windows in the
 *   last hour;  the pitch range and the number of pitches used.
 * The limits are the guardrails the ROADMAP asks of long-form play; a patch whose automation is meant to rest
 * (ENERGY RESTS) is allowed its rests. Two roles get their own: a drum generator (GRIDS) plays its lanes, so no
 * pitch count; a harmony bed (CHORD) is meant to come round again, so it may replay its progression up to 15
 * minutes (a melodic track: about 4). Limits marked KNOWN are gaps the ROADMAP already
 * lists (risk 1 of generators.md: a FORM never develops once its sections exist): they are printed as KNOWN while
 * they fail and fail the test once they pass, so the limit is made strict when the gap closes. */
#include <sys/wait.h>
#include <unistd.h>
#define TESSERA_NOTE_HOOK lf_note
#define main hostsim_main
#include "hostsim.c"
#undef main

#define LF_BARS_WIN 4u                                /* a window: 4 bars */
#define LF_BACK 16u                                   /* frozen: the same as one of the 16 windows before (64 bars) */
#define LF_TR NTRK
#define LF_PROBE_SECS 120u                            /* the fast path against mix_block: two minutes */

/* ----------------------------------------------------------- the patches --- */
static void sel(uint32_t tr) { song.sel = (uint8_t)tr; }
static void gen(uint32_t tr, uint32_t mode) { sel(tr); tessera_knob(0, (int32_t)mode); }   /* from OFF */
static void px(uint32_t tr, uint32_t x, int32_t v) { tessera_param_set(tr, x, v); }
enum { EV_OFF, EV_1, EV_2, EV_4, EV_8, EV_16 };       /* TX_EV_EVERY */
enum { FM_OFF, FM_AB, FM_ABAC, FM_ABCB, FM_AABA, FM_ABCA, FM_AABC };   /* TX_FM_FORM */

static void p_turing(void)
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_EV_EVERY, EV_1);
    px(0, TX_EV_AMOUNT, 30);
}
static void p_euclid(void)
{
    gen(0, TESS_ENG_EUCLID);
    px(0, TX_EV_EVERY, EV_1);
    px(0, TX_EV_AMOUNT, 30);
}
static void p_markov(void)
{
    gen(0, TESS_ENG_MARKOV);
    px(0, TX_EV_EVERY, EV_2);
    px(0, TX_EV_LOCK, 60);
}
static void p_morph(void)
{
    gen(0, TESS_ENG_MORPH);
    px(0, TX_EV_EVERY, EV_1);
    px(0, TX_EV_AMOUNT, 40);
}
static void p_form(void)                              /* risk 1: once A, B and C exist every return is exact */
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_FM_FORM, FM_ABAC);
    px(0, TX_EV_EVERY, EV_1);
    px(0, TX_FM_CONTRAST, 60);
}
static void p_gravity(void)                           /* GRAVITY on EVOLVE: drifts and comes home */
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_EV_EVERY, EV_1);
    px(0, TX_EV_AMOUNT, 60);
    px(0, TX_SD_GRAVITY, 50);
    gen(1, TESS_ENG_AWAKE);
    px(1, TX_EV_EVERY, EV_2);
    px(1, TX_EV_AMOUNT, 50);
    px(1, TX_SD_GRAVITY, 50);
}
static void p_four(void)                              /* the 2-hour probe's four tracks (generators.md) */
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_TUR_PROB, 10);
    px(0, TX_EV_EVERY, EV_2);
    gen(1, TESS_ENG_EUCLID);
    px(1, TX_EV_EVERY, EV_4);
    gen(2, TESS_ENG_MARKOV);
    px(2, TX_EV_LOCK, 70);
    px(2, TX_FM_FORM, FM_ABAC);
    gen(3, TESS_ENG_MORPH);
    px(3, TX_EV_EVERY, EV_8);
    px(3, TX_TW_DROP, 20);
}
static void p_newer(void)                             /* the generators added since: Grids, Bounce, Constellation, Series */
{
    gen(0, TESS_ENG_GRIDS);
    px(0, TX_EV_EVERY, EV_4);
    px(0, TX_EV_AMOUNT, 50);
    gen(1, TESS_ENG_BOUNCE);
    px(1, TX_EV_EVERY, EV_2);
    gen(2, TESS_ENG_CONSTEL);
    px(2, TX_CS_AUTO, 5);
    gen(3, TESS_ENG_SERIES);
    px(3, TX_EV_EVERY, EV_4);
    px(3, TX_EV_AMOUNT, 50);
}
static void p_ambient(void)                           /* an ambient night: a CHORD drone, ENERGY's tide and rests */
{
    gen(0, TESS_ENG_CHORD);
    px(0, TX_CH_PROG, GEN_CH_WALK);
    px(0, TX_CH_HIT, GEN_CH_HOLD);
    px(0, TX_EV_EVERY, EV_8);
    gen(1, TESS_ENG_AWAKE);
    px(1, TX_EV_EVERY, EV_4);
    px(1, TX_TW_DROP, 30);
    gen(2, TESS_ENG_MARKOV);
    px(2, TX_MKV_DENS, 30);
    px(2, TX_EV_EVERY, EV_4);
    px(2, TX_EV_LOCK, 50);
    px(0, TX_EN_ARC, 60);
    px(0, TX_EN_RESTS, 40);
}

typedef struct {
    const char *name;
    uint32_t seed;
    void (*setup)(void);
    uint32_t rests;                                   /* ENERGY RESTS on: the mix may rest (8 to 18 s) */
    uint32_t known_frozen;                            /* tracks (bits) KNOWN to freeze (generators.md risk 1) */
    const char *why;
} patch_t;
static const patch_t PATCH[] = {
    {"turing_evolve1_amount30", 0x4C01u, p_turing, 0, 0, "Turing, EVOLVE every bar"},
    {"euclid_evolve1_amount30", 0x4C02u, p_euclid, 0, 0, "Euclid: PULSES a random walk (risk 2)"},
    {"markov_evolve2_lock60", 0x4C03u, p_markov, 0, 0, "Markov with LOCK (déjà vu)"},
    {"morph_evolve1_amount40", 0x4C04u, p_morph, 0, 0, "Morph: rhythm A losing its bits (risk 2)"},
    {"form_turing_abac_contrast60", 0x4C05u, p_form, 0, 1u, "FORM: sections recalled exactly (risk 1)"},
    {"gravity50_turing_awake", 0x4C06u, p_gravity, 0, 0, "GRAVITY on EVOLVE pulling home (risk 4)"},
    {"four_tracks_probe", 0x4C07u, p_four, 0, 0, "the 2-hour probe's four tracks, Euclid at EVERY 4"},
    {"grids_bounce_constel_series", 0x4C08u, p_newer, 0, 0, "the newer generators together"},
    {"ambient_chord_energy_rests", 0x4C09u, p_ambient, 1, 0, "CHORD drone, ARC 60, RESTS 40"},
};
#define NPATCH (sizeof PATCH / sizeof PATCH[0])

/* ------------------------------------------------------------- the notes --- */
static uint32_t lf_blk;                               /* blocks since the start */
static int32_t lf_open_note[LF_TR];                   /* the track's note still held, -1 none */
static uint32_t lf_probe_on, lf_probe_hash, lf_probe_n;
/* the window being filled, per track (index LF_TR: the mix) */
typedef struct {
    uint32_t notes, hash, lo, hi;
    uint32_t pitch[4];                                /* the pitches played, a bit each */
    uint8_t sounded;                                  /* a note started or held in it */
} lf_win_t;
static lf_win_t lf_w[LF_TR + 1];
static uint32_t lf_win_start;                         /* the block the window began at */

static uint32_t fnv(uint32_t h, uint32_t v)
{
    for (uint32_t i = 0; i < 4u; i++) {
        h ^= (v >> (8u * i)) & 0xFFu;
        h *= 16777619u;
    }
    return h;
}

static void lf_put(lf_win_t *w, uint32_t tr, uint32_t note, uint32_t at)
{
    w->notes++;
    w->hash = fnv(fnv(w->hash, at), note | tr << 8);
    w->pitch[note >> 5] |= 1u << (note & 31u);
    if (note < w->lo) w->lo = note;
    if (note > w->hi) w->hi = note;
    w->sounded = 1;
}

static void lf_note(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t tr = trk_index(t), at = lf_blk - lf_win_start;
    if (tr >= LF_TR || !vel) return;
    if (lf_probe_on) {
        lf_probe_hash = fnv(fnv(lf_probe_hash, lf_blk), note | tr << 8 | vel << 16);
        lf_probe_n++;
        return;
    }
    lf_open_note[tr] = (int32_t)note;
    lf_put(&lf_w[tr], tr, note, at);
    lf_put(&lf_w[LF_TR], tr, note, at);
}

static void lf_held(void)                             /* a note let go: no longer sounding */
{
    for (uint32_t tr = 0; tr < LF_TR; tr++)
        if (lf_open_note[tr] >= 0 && !trk[tr].seq_n && !(nl_active & (1u << tr))) lf_open_note[tr] = -1;
}

/* ------------------------------------------------------------- measures --- */
typedef struct {
    uint32_t wins, silent, silent_run, silent_longest, one_pitch, frozen_run, frozen_longest;
    uint32_t notes_h1, notes_hlast, wins_h1, wins_hlast;
    uint32_t lo, hi, pitch[4];
    uint32_t back[LF_BACK], nback;                    /* the windows before (their hashes), the newest last */
    uint32_t last_hash[512], nlast;                   /* the last hour's windows (distinct count) */
} lf_meas_t;
static lf_meas_t lf_m[LF_TR + 1];
static uint8_t lf_active[LF_TR];                      /* a generator track: its MODE + 1, 0 a STEP track */

static uint32_t popc(uint32_t v)
{
    uint32_t n = 0;
    for (; v; v &= v - 1u) n++;
    return n;
}

static void lf_close_window(uint32_t win, uint32_t wins_hour, uint32_t wins_total)
{
    for (uint32_t k = 0; k <= LF_TR; k++) {
        lf_win_t *w = &lf_w[k];
        lf_meas_t *m = &lf_m[k];
        uint32_t i, rep = 0, held = 0;
        if (k < LF_TR) held = lf_open_note[k] >= 0;
        else
            for (i = 0; i < LF_TR; i++) held |= lf_open_note[i] >= 0;
        m->wins++;
        if (!w->sounded && !held) {
            m->silent++;
            if (++m->silent_run > m->silent_longest) m->silent_longest = m->silent_run;
        } else
            m->silent_run = 0;
        if (w->notes && popc(w->pitch[0]) + popc(w->pitch[1]) + popc(w->pitch[2]) + popc(w->pitch[3]) == 1u)
            m->one_pitch++;
        if (w->notes)
            for (i = 0; i < m->nback; i++) rep |= m->back[i] == w->hash;
        if (rep) {
            if (++m->frozen_run > m->frozen_longest) m->frozen_longest = m->frozen_run;
        } else
            m->frozen_run = 0;
        if (m->nback == LF_BACK) memmove(m->back, m->back + 1, (LF_BACK - 1u) * sizeof m->back[0]), m->nback--;
        m->back[m->nback++] = w->notes ? w->hash : 0u;
        if (win < wins_hour) m->notes_h1 += w->notes, m->wins_h1++;
        if (win >= wins_total - wins_hour) {
            m->notes_hlast += w->notes, m->wins_hlast++;
            if (m->nlast < 512u && w->notes) m->last_hash[m->nlast++] = w->hash;
        }
        if (w->notes) {
            if (w->lo < m->lo) m->lo = w->lo;
            if (w->hi > m->hi) m->hi = w->hi;
            for (i = 0; i < 4u; i++) m->pitch[i] |= w->pitch[i];
        }
        memset(w, 0, sizeof *w);
        w->hash = 2166136261u;
        w->lo = 127;
    }
}

static uint32_t distinct(const uint32_t *h, uint32_t n)
{
    uint32_t d = 0;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t j;
        for (j = 0; j < i && h[j] != h[i]; j++)
            ;
        d += j == i;
    }
    return d;
}

/* ---------------------------------------------------------------- a run --- */
static void lf_power_on(const patch_t *p)
{
    host_tracks_init();
    tessera_reset();
    song.g[G_BPM] = 120;
    for (uint32_t tr = 0; tr < LF_TR; tr++) lf_open_note[tr] = -1;
    p->setup();
    for (uint32_t tr = 0; tr < LF_TR; tr++) lf_active[tr] = tr < TESSERA_TRACKS && tessera.mode[tr] != TESS_ENG_OFF ? (uint8_t)(tessera.mode[tr] + 1u) : 0u;
    rng_state = p->seed;
    transport_req = 1;
}

static void lf_fast_block(void)                       /* mix_block without the voices: the sequencer's half */
{
    events_block(CTL);
    cord_tick();
}

/* the first LF_PROBE_SECS: the note hash through mix_block (full) or the fast block */
static uint32_t lf_probe(const patch_t *p, int full, uint32_t *n)
{
    int32_t o[2 * CTL];
    uint32_t blocks = LF_PROBE_SECS * FS / CTL;
    lf_power_on(p);
    lf_probe_on = 1, lf_probe_hash = 2166136261u, lf_probe_n = 0, lf_blk = 0;
    for (; lf_blk < blocks; lf_blk++) {
        if (full) mix_block(o, CTL);
        else lf_fast_block();
    }
    lf_probe_on = 0;
    *n = lf_probe_n;
    return lf_probe_hash;
}

/* the patch for HOURS: the measures in lf_m */
static void lf_run(const patch_t *p, uint32_t hours)
{
    uint32_t bar_blocks = 4u * seq_beat_samples() / CTL, win_blocks = LF_BARS_WIN * bar_blocks;
    uint32_t wins_total = hours * 3600u * FS / CTL / win_blocks, wins_hour = 3600u * FS / CTL / win_blocks, win;
    lf_power_on(p);
    memset(lf_m, 0, sizeof lf_m);
    for (uint32_t k = 0; k <= LF_TR; k++) {
        memset(&lf_w[k], 0, sizeof lf_w[k]);
        lf_w[k].hash = 2166136261u, lf_w[k].lo = 127;
        lf_m[k].lo = 127;
    }
    lf_blk = 0;
    for (win = 0; win < wins_total; win++) {
        lf_win_start = lf_blk;
        for (uint32_t b = 0; b < win_blocks; b++, lf_blk++) {
            lf_fast_block();
            lf_held();
        }
        lf_close_window(win, wins_hour, wins_total);
    }
}

/* ------------------------------------------------------------- results --- */
/* a patch's result, one line a track and the mix: "k wins silent silent_longest one_pitch frozen_longest notes_h1
 * notes_hlast wins_h1 wins_hlast lo hi pitches distinct_last nlast active" */
typedef struct {
    uint32_t k, wins, silent, silent_longest, one_pitch, frozen_longest, notes_h1, notes_hlast, wins_h1, wins_hlast;
    uint32_t lo, hi, pitches, distinct_last, nlast, active;
} lf_res_t;

static int lf_child(const patch_t *p, uint32_t hours, int fd)
{
    char line[256];
    uint32_t nf, nm, hf = lf_probe(p, 1, &nf), hm = lf_probe(p, 0, &nm);
    int len = snprintf(line, sizeof line, "probe %u %u %u %u\n", hf, nf, hm, nm);
    if (write(fd, line, (size_t)len) < 0) return 1;
    lf_run(p, hours);
    for (uint32_t k = 0; k <= LF_TR; k++) {
        lf_meas_t *m = &lf_m[k];
        uint32_t pitches = popc(m->pitch[0]) + popc(m->pitch[1]) + popc(m->pitch[2]) + popc(m->pitch[3]);
        uint32_t act = k < LF_TR ? lf_active[k] : 1u;
        len = snprintf(line, sizeof line, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", k, m->wins, m->silent,
                       m->silent_longest, m->one_pitch, m->frozen_longest, m->notes_h1, m->notes_hlast, m->wins_h1,
                       m->wins_hlast, m->lo, m->hi, pitches, distinct(m->last_hash, m->nlast), m->nlast, act);
        if (write(fd, line, (size_t)len) < 0) return 1;
    }
    return 0;
}

typedef struct {
    pid_t pid;
    int fd;
    char buf[4096];
    size_t n;
} lf_job_t;

static int lf_bad, lf_known;
static int check(const char *what, int ok)
{
    printf("longform: %-100s %s\n", what, ok ? "ok" : "FAILED");
    return !ok;
}

/* the guardrails: each patch's tracks and its mix */
static void lf_judge(const patch_t *p, const char *out, FILE *rep, uint32_t hours)
{
    lf_res_t r[LF_TR + 1];
    unsigned hf = 0, nf = 0, hm = 0, nm = 0;
    char line[200];
    const char *s = out;
    uint32_t k, bar_s = 2u;                           /* (a bar at 120 BPM: 2 s) */
    memset(r, 0, sizeof r);
    if (sscanf(s, "probe %u %u %u %u", &hf, &nf, &hm, &nm) != 4) {
        snprintf(line, sizeof line, "%s: runs", p->name);
        lf_bad += check(line, 0);
        return;
    }
    snprintf(line, sizeof line, "%s: the fast path plays mix_block's notes (%u in %u s)", p->name, nf, LF_PROBE_SECS);
    lf_bad += check(line, hf == hm && nf == nm && nf > 0);
    for (k = 0; k <= LF_TR; k++) {
        lf_res_t *x = &r[k];
        s = strchr(s, '\n');
        if (!s || sscanf(++s, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", &x->k, &x->wins, &x->silent,
                         &x->silent_longest, &x->one_pitch, &x->frozen_longest, &x->notes_h1, &x->notes_hlast,
                         &x->wins_h1, &x->wins_hlast, &x->lo, &x->hi, &x->pitches, &x->distinct_last, &x->nlast,
                         &x->active) != 16) {
            snprintf(line, sizeof line, "%s: all %u measures", p->name, LF_TR + 1u);
            lf_bad += check(line, 0);
            return;
        }
    }
    if (rep) {
        fprintf(rep, "%s (%s), %u h at 120 BPM, windows of %u bars\n", p->name, p->why, hours, LF_BARS_WIN);
        fprintf(rep, "  %-5s %9s %9s %8s %9s %9s %9s %7s %8s\n", "", "notes/bar", "silent", "longest", "one-pitch",
                "frozen", "variety", "range", "pitches");
        fprintf(rep, "  %-5s %9s %9s %8s %9s %9s %9s %7s %8s\n", "", "h1 / hN", "windows", "silence", "windows",
                "longest", "last h", "", "");
    }
    for (k = 0; k <= LF_TR; k++) {
        lf_res_t *x = &r[k];
        char who[48];
        double bar1 = x->wins_h1 ? (double)x->notes_h1 / (x->wins_h1 * LF_BARS_WIN) : 0;
        double barN = x->wins_hlast ? (double)x->notes_hlast / (x->wins_hlast * LF_BARS_WIN) : 0;
        uint32_t silent_bars = x->silent_longest * LF_BARS_WIN, frozen_bars = x->frozen_longest * LF_BARS_WIN;
        uint32_t frozen_ok, known = k < LF_TR && (p->known_frozen >> k & 1u), mode = x->active - 1u;
        uint32_t drums = k < LF_TR && mode == TESS_ENG_GRIDS, bed = k < LF_TR && mode == TESS_ENG_CHORD;
        if (!x->active) continue;
        if (k < LF_TR) snprintf(who, sizeof who, "%s T%u", p->name, k + 1u);
        else snprintf(who, sizeof who, "%s mix", p->name);
        if (rep)
            fprintf(rep, "  %-5s %4.1f/%4.1f %8.1f%% %5u b %8.1f%% %7u b %4u/%-4u %3u-%-3u %8u\n",
                    k < LF_TR ? (k == 0 ? "T1" : k == 1 ? "T2" : k == 2 ? "T3" : "T4") : "mix", bar1, barN,
                    100.0 * x->silent / x->wins, silent_bars, 100.0 * x->one_pitch / x->wins, frozen_bars,
                    x->distinct_last, x->nlast, x->lo, x->hi, x->pitches);
        /* 1. still playing: the last hour has at least a quarter of the first hour's notes, and some */
        snprintf(line, sizeof line, "%s: still playing in hour %u (%.1f notes a bar, hour 1 %.1f)", who, hours, barN,
                 bar1);
        lf_bad += check(line, x->notes_hlast > 0u && x->notes_hlast * 4u >= x->notes_h1);
        /* 2. no long silence: the mix never silent longer than 32 bars (64 s) unless RESTS asks (a rest: up to 18 s,
         *    so 3 windows with the bars round it); a track: 64 bars (a sparse part may lie out a while) */
        if (k == LF_TR) {
            uint32_t most = p->rests ? 12u : 32u;
            snprintf(line, sizeof line, "%s: no silence longer than %u bars (longest %u, %u s)", who, most, silent_bars,
                     silent_bars * bar_s);
            lf_bad += check(line, silent_bars <= most);
        } else {
            snprintf(line, sizeof line, "%s: never out longer than 64 bars (longest %u)", who, silent_bars);
            lf_bad += check(line, silent_bars <= 64u);
        }
        /* 3. not one note: at most 1 window in 10 a single pitch, and more than 3 pitches over the night (a drum
         *    generator: its lanes, any number) */
        if (drums) {
            snprintf(line, sizeof line, "%s: one-pitch windows %.1f%% (at most 10%%; drums: %u lanes)", who,
                     100.0 * x->one_pitch / x->wins, x->pitches);
            lf_bad += check(line, x->one_pitch * 10u <= x->wins);
        } else {
            snprintf(line, sizeof line, "%s: one-pitch windows %.1f%% (at most 10%%), %u pitches (more than 3)", who,
                     100.0 * x->one_pitch / x->wins, x->pitches);
            lf_bad += check(line, x->one_pitch * 10u <= x->wins && x->pitches > 3u);
        }
        /* 4. not frozen: with EVOLVE on, no loop (up to 64 bars) replayed note for note for more than about 4
         *    minutes (112 bars), and the last hour at least 1 window in 5 not heard before in it (a vocabulary that
         *    comes round is fine; a handful of windows over and over is not). A harmony bed: 15 minutes (448 bars),
         *    1 in 10 */
        frozen_ok = bed ? frozen_bars <= 448u && x->distinct_last * 10u >= x->nlast
                        : frozen_bars <= 112u && x->distinct_last * 5u >= x->nlast;
        if (known) {
            snprintf(line, sizeof line, "%s: KNOWN frozen (risk 1, FORM recalls sections exactly): longest %u bars, "
                                        "%u/%u distinct", who, frozen_bars, x->distinct_last, x->nlast);
            printf("longform: %-100s %s\n", line, frozen_ok ? "FIXED: make it strict" : "KNOWN");
            lf_bad += frozen_ok;
            lf_known += !frozen_ok;
        } else if (k < LF_TR) {
            snprintf(line, sizeof line, "%s: not frozen%s (longest replay %u bars, last hour %u/%u windows distinct)",
                     who, bed ? " (harmony bed)" : "", frozen_bars, x->distinct_last, x->nlast);
            lf_bad += check(line, frozen_ok);
        }
    }
    if (rep) fputc('\n', rep);
}

int main(int argc, char **argv)
{
    uint32_t hours = argc > 1 ? (uint32_t)atoi(argv[1]) : 8u, i, running = 0, next = 0;
    const char *only = getenv("LF_ONLY");
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    FILE *rep = argc > 2 ? fopen(argv[2], "w") : 0;
    lf_job_t job[NPATCH];
    if (!hours) hours = 8u;
    if (cpus < 1) cpus = 1;
    if (getenv("LF_JOBS")) cpus = atoi(getenv("LF_JOBS"));
    memset(job, 0, sizeof job);
    /* each patch a child (power-on state), as many at once as there are CPUs */
    while (next < NPATCH || running) {
        while (next < NPATCH && running < (uint32_t)cpus) {
            int fd[2];
            if (only && strcmp(only, PATCH[next].name)) { next++; continue; }
            if (pipe(fd)) return 1;
            job[next].pid = fork();
            if (!job[next].pid) {
                close(fd[0]);
                _exit(lf_child(&PATCH[next], hours, fd[1]));
            }
            close(fd[1]);
            job[next].fd = fd[0];
            running++, next++;
        }
        for (i = 0; i < next; i++) {                  /* read the children's lines as they come */
            ssize_t got;
            if (job[i].fd <= 0) continue;
            got = read(job[i].fd, job[i].buf + job[i].n, sizeof job[i].buf - 1u - job[i].n);
            if (got > 0) { job[i].n += (size_t)got; continue; }
            close(job[i].fd);
            job[i].fd = -1;
            waitpid(job[i].pid, 0, 0);
            job[i].buf[job[i].n] = 0;
            running--;
        }
    }
    for (i = 0; i < NPATCH; i++)
        if (job[i].fd == -1) lf_judge(&PATCH[i], job[i].buf, rep, hours);
    if (rep) fclose(rep);
    printf("longform: %u patches, %u h each: %d failed, %d known gaps\n", (unsigned)NPATCH, hours, lf_bad, lf_known);
    return lf_bad != 0;
}
