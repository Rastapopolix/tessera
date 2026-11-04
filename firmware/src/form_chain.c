/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Form chaining (long form, slice 1): one FORM for the whole project, on the project's bar clock. The CHAIN page
 * (GLO): FORM (OFF, AB .. AABC, the per-track FORM's shapes), PHRASE (4 / 8 / 16 bars a phrase, counted from PLAY),
 * WANDER 0..100 and a REPEAT count of 1-8 phrases for each place in the FORM. Each phrase boundary the project moves on
 * (after its place's count of phrases); every generator track that FOLLOWs moves to the same letter at the same moment,
 * using its own A / B / C sections as a track's FORM does (recalled, DEVELOPed, or made new by its CONTRAST). OWN
 * tracks keep their own FORM. A track FOLLOWs unless set otherwise where its own FORM is OFF, so no older project
 * changes; FORM OFF (the default) changes nothing at all.
 *   WANDER: at each change of letter, with chance WANDER the next letter is drawn instead of read: from the FORM's
 *   letters; the letter now only if the FORM repeats it; a letter not heard yet only after a return to A since the last
 *   new one; A weighted more the longer the music has been away (A is home). A drawn letter takes its count from its
 *   first place in the FORM. Each further phrase away from A adds 10 to the CONTRAST of the next new section (up to 30),
 *   back to 0 on A.
 *   SKIP (a track's, 0..100): where the project moves on, the chance the track holds its section for one more phrase;
 *   never twice running.
 * Every draw is a hash of the project's SEEDs (taken at PLAY, as ENERGY's), the phrase's number and what is drawn, so
 * a piece replays from PLAY, and one track's SKIP never moves another's or WANDER's path. Included by tessera.c. */

#define FC_PHRASES 3u                                 /* PHRASE: 4, 8, 16 bars */
static const uint8_t FC_PHRASE_BARS[FC_PHRASES] = {4, 8, 16};
#define FC_HIST 16u                                   /* phrases of moves remembered (SKIP looks back, never further) */

typedef struct {
    uint32_t p;                                       /* the phrase this is: since PLAY */
    uint8_t pos, left;                                /* the place in the FORM, the phrases it still holds after this */
    uint8_t away;                                     /* phrases played away from A, up to this one */
    uint8_t fresh_ok, heard;                          /* a new letter allowed (back on A since the last); letters heard */
    uint8_t extra;                                    /* the CONTRAST this phrase's move adds to a new section */
    uint16_t moved;                                   /* bit i: the project moved on at phrase p - i */
} fc_st_t;
static struct {
    fc_st_t now, before;                              /* the newest phrase worked out, and the one before it */
    uint32_t seed;                                    /* the SEEDs' hash, at PLAY */
    uint8_t ok;                                       /* now / before hold */
} fchain __attribute__((section(".pool")));

static uint32_t fc_bars(void) { return FC_PHRASE_BARS[tessera.fc_phrase < FC_PHRASES ? tessera.fc_phrase : 1u]; }
static uint32_t fc_len(void) { return TESS_FORM_LEN[tessera.fc_form > 6u ? 0u : tessera.fc_form]; }
static uint32_t fc_letter_at(uint32_t pos) { return TESS_FORMS[tessera.fc_form > 6u ? 0u : tessera.fc_form][pos & 3u]; }
static uint32_t fc_rep(uint32_t pos) { uint32_t r = tessera.fc_rep[pos & 3u]; return r < 1u ? 1u : r > 8u ? 8u : r; }

static uint32_t fc_hash(uint32_t p, uint32_t salt)    /* a draw: the SEEDs, the phrase, what is drawn */
{
    uint32_t k = fchain.seed ^ (p * 0x9E3779B9u) ^ (salt * 0x85EBCA6Bu);
    k = (k ^ (k >> 16)) * 0x7FEB352Du;
    k = (k ^ (k >> 15)) * 0x846CA68Bu;
    return k ^ (k >> 16);
}

/* the FORM plays letter c twice running somewhere (AABA; ABCA round its end) */
static int fc_repeats(uint32_t c)
{
    uint32_t i, n = fc_len();
    for (i = 0; i < n; i++)
        if (fc_letter_at(i) == c && fc_letter_at((i + 1u) % n) == c) return 1;
    return 0;
}
static uint32_t fc_first(uint32_t c)                  /* letter c's first place in the FORM */
{
    uint32_t i, n = fc_len();
    for (i = 0; i < n; i++)
        if (fc_letter_at(i) == c) return i;
    return 0;
}

static void fc_begin(fc_st_t *s, uint32_t p)          /* phrase p is A, the FORM's first place */
{
    memset(s, 0, sizeof *s);
    s->p = p;
    s->left = (uint8_t)(fc_rep(0) - 1u);
    s->fresh_ok = 1;
    s->heard = 1;
}

static int planner_home(uint32_t p);                 /* (planner.c: the piece's last 15 %, A for good) */
static uint32_t planner_contrast(uint32_t p);         /* (planner.c: a new section's extra CONTRAST near the top) */
/* from phrase s->p to the next */
static void fc_step(fc_st_t *s)
{
    uint32_t n = fc_len(), cur, nx, c, q = s->p + 1u;
    s->p = q;
    s->moved = (uint16_t)(s->moved << 1);
    if (!n) return;
    if (s->pos >= n) s->pos = 0;                      /* (FORM shorter since) */
    cur = fc_letter_at(s->pos);
    if (planner_home(q)) {                            /* (a planned piece's end: A, held) */
        if (cur) {
            s->pos = 0;
            s->moved |= 1u;
            s->extra = 0;
            s->away = 0;
        }
        s->left = 0;
        return;
    }
    if (s->left && s->left >= fc_rep(s->pos)) s->left = (uint8_t)(fc_rep(s->pos) - 1u);   /* (a count lowered since) */
    if (s->left) {                                    /* the letter holds */
        s->left--;
        if (cur) s->away++;
        return;
    }
    nx = (s->pos + 1u) % n;
    if (tessera.fc_wander && fc_hash(q, 1u) % 100u < tessera.fc_wander) {
        uint32_t w[3] = {0, 0, 0}, sum = 0, r;
        for (c = 0; c < 3u; c++) {
            if (fc_first(c) == 0u && fc_letter_at(0) != c) continue;   /* (not in the FORM) */
            if (c == cur ? !fc_repeats(c) : !(s->heard >> c & 1u) && !s->fresh_ok) continue;
            w[c] = c ? 1u : 1u + s->away;             /* (A: home, the more wanted the longer away) */
            sum += w[c];
        }
        if (sum) {                                    /* (nothing allowed: as written) */
            r = fc_hash(q, 2u) % sum;
            for (c = 0; r >= w[c]; c++) r -= w[c];
            nx = fc_first(c);
        }
    }
    c = fc_letter_at(nx);
    s->pos = (uint8_t)nx;
    s->left = (uint8_t)(fc_rep(nx) - 1u);
    s->moved |= 1u;
    s->extra = (uint8_t)((c && s->away < 3u ? 10u * s->away : c ? 30u : 0u) + (c ? planner_contrast(q) : 0u));
    if (!(s->heard >> c & 1u)) {
        s->heard |= (uint8_t)(1u << c);
        s->fresh_ok = 0;
    }
    if (!c) {
        if (cur) s->fresh_ok = 1;                     /* (back home: a new letter may come next) */
        s->away = 0;
    } else if (s->away < 255u) {
        s->away++;
    }
}

/* the project at phrase p (since PLAY): the newest worked out, the one before, or on from them */
static const fc_st_t *fc_at(uint32_t p)
{
    if (!fchain.ok || p < fchain.before.p) {          /* (back in time: Song Position) from PLAY */
        fc_begin(&fchain.now, 0);
        fchain.before = fchain.now;
        fchain.ok = 1;
    }
    if (p + 1u == fchain.now.p) return &fchain.before;
    while (fchain.now.p < p) {
        fchain.before = fchain.now;
        fc_step(&fchain.now);
    }
    return &fchain.now;
}

/* seq.c seq_start: from the top, the draws from the project's SEEDs */
void fchain_play(void)
{
    uint32_t tr, h = 0x43484149u;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) h = (h ^ tessera.seed[tr]) * 16777619u;
    fchain.seed = h;
    fchain.ok = 0;
}

/* track tr FOLLOWs the project's FORM: it is on, the track plays a generator, and it is set to FOLLOW (or not set,
 * with its own FORM OFF) */
static int fc_follows(uint32_t tr)
{
    uint32_t f = tessera.fc_follow[tr];
    return tessera.fc_form && tessera.mode[tr] != TESS_ENG_OFF && (f == 1u || (!f && !tessera.evolve[tr].form));
}

/* track tr held its section at phrase p (SKIP): the project moved on there and its draw said hold, unless it held the
 * phrase before (never twice running) */
static int fc_skips(const fc_st_t *s, uint32_t p, uint32_t tr)
{
    uint32_t skip = tessera.fc_skip[tr], i, run = 0;
    if (!skip) return 0;
    for (i = 0; i < FC_HIST && i <= p && (s->moved >> (s->p - p + i) & 1u) && fc_hash(p - i, 16u + tr) % 100u < skip; i++)
        run++;
    return run & 1u;                                  /* (a run of hits: hold, go, hold, ...) */
}

static void tess_sec_go(uint32_t tr, uint32_t sec, uint32_t extra);
/* a following track's step: on the project's phrase line its section changes with the project's (or holds, SKIP; or
 * catches up, behind for any reason) */
static void fc_track_step(const track_t *t, uint32_t tr)
{
    tessera_evolve_t *ev = &tessera.evolve[tr];
    uint32_t k = seq_run_step[tr], spb = tess_steps_per_bar(t), bars = fc_bars(), bar, p;
    const fc_st_t *s;
    if (!song.playing || k == 0xFFFFFFFFu || k % spb) return;
    bar = k / spb;
    ev->bar_cnt = (uint8_t)(bar % bars);              /* (FLOURISH: the phrase's last bar) */
    if (bar % bars) return;
    p = bar / bars;
    s = fc_at(p);
    if ((s->moved & 1u) && fc_skips(s, p, tr)) return;
    if ((s->moved & 1u) || ev->sec != fc_letter_at(s->pos)) tess_sec_go(tr, fc_letter_at(s->pos), s->extra);
}

/* the CHAIN page's picture: the FORM's letters (n, 0 OFF), the place playing and the phrases left of it (stopped: the
 * top) */
uint32_t tessera_chain_get(uint8_t seq[4], uint32_t *pos, uint32_t *left)
{
    uint32_t n = fc_len(), i;
    for (i = 0; i < 4u; i++) seq[i] = (uint8_t)fc_letter_at(i);
    *pos = 0;
    *left = fc_rep(0) - 1u;
    if (n && song.playing && fchain.ok) {
        *pos = fchain.now.pos < n ? fchain.now.pos : 0u;
        *left = fchain.now.left;
    }
    return n;
}

/* a setting of the CHAIN page changed (tess_param_set_body): FORM changed, the live phrase is A again for the project
 * and every follower (the per-track FORM's rule); on to OFF, the tracks stay where they are. PHRASE: the phrases
 * counted on its new length from PLAY, the place kept */
static void fc_changed(uint32_t x, uint32_t was)
{
    uint32_t tr;
    if (x == TX_FC_FORM) {
        if (tessera.fc_form == was) return;
        if (song.playing && fchain.ok) fc_begin(&fchain.now, fchain.now.p), fchain.before = fchain.now;
        if (!tessera.fc_form) return;
        for (tr = 0; tr < TESSERA_TRACKS; tr++)
            if (fc_follows(tr)) tess_form_restart(tr);
    } else if (x == TX_FC_PHRASE && song.playing && fchain.ok) {
        fchain.now.p = bar_n / fc_bars();
        fchain.before = fchain.now;
        if (fchain.before.p) fchain.before.p--;
    }
}
