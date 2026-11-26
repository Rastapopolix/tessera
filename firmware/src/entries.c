/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Entries (long form, slice 2): tracks drop out and come back on the project's phrase lines. The ENTRIES page (GLO):
 * ENTRIES 0..100 (0 off: every track as set), DWELL 1-8 phrases a track stays in or out at least, STYLE CUT / FADE and
 * FADE 1-4 bars; OCT- held, KNOB k track k's ROLE (also OCT- held on FORM, KNOB 4): AUTO, BASS, RHYTHM, HARMONY,
 * MELODY or ANCHOR (never taken out). AUTO (et_guess_all): CHORD or chords harmony; GRIDS, a kit or NOISE rhythm; notes
 * below C3, or the lowest track a fifth below the rest, bass; else melody (from the settings until heard; a guess
 * holds through a section). OCT- held, KNOB k track k's ORDER: AUTO, 1ST..4TH (et_rank: the order it comes in).
 *   The phrase is the CHAIN's (PHRASE, counted from PLAY, its FORM on or OFF). A section begins where the CHAIN moves
 *   on to its next place (FORM OFF: every 4 phrases). There a number of tracks wanted out is drawn (ENTRIES: how many
 *   may be at most, of the tracks that have played, not ANCHOR, not muted by hand); one more on leaving A. Exits come
 *   on a section's line, all at once, MELODY first, then HARMONY, RHYTHM, BASS; entries one a phrase the other way
 *   round, a melody only over a bass or harmony playing. ORDER set: the numbered tracks come in first, 1ST to 4TH, then
 *   the AUTO ones by ROLE; out the other way round (last in, first out); only an AUTO melody waits for its ground. With ENTRIES above 50 the phrase before a return to A (after two
 *   phrases away or more) drops to the anchors (or the one track lowest in that order), and A comes back with
 *   everything. At least one track always
 *   plays. Each change is decided a bar ahead: the track's mute key on GLO blinks quickly through that bar, and a track
 *   with FLOURISH set comes in on its flourish (the bar's last beat).
 *   A track out is silent like a muted one (no notes, none to MIDI OUT; the mix's level faded or cut; a FADE
 *   fades MIDI OUT's velocity too, entries_vel). Hand wins: a
 *   track muted by hand stays muted; its key on GLO brings an out track back, left alone until the next section; SOLO
 *   is heard whatever Entries says. Every draw is a hash of the SEEDs taken at PLAY, so a piece replays. Included by
 *   tessera.c after form_chain.c.
 *   The arrangement grid (an 8-track build's: Cosmati's ENTRIES grid, Andamento's; tessera.et_grid): a cell a track
 *   and FORM letter (A, B, C; FORM OFF: A's the whole piece). AUTO: Entries decides, as above; IN: plays there; OUT:
 *   never; FADE: in, fading in over the section's first phrase (when it was out) and out over its last (when the next
 *   section has it AUTO or OUT). The rules draw among the AUTO cells only; OUT wins over ANCHOR; hand mutes still win,
 *   and one track still plays. The grid works with ENTRIES 0 too (its AUTO cells then play). Every cell AUTO: no
 *   change at all. */

enum { ET_AUTO, ET_BASS, ET_RHYTHM, ET_HARMONY, ET_MELODY, ET_ANCHOR,
#if TESSERA_TRACKS > 4
       ET_TURNS,                                      /* (tracks 5..8: takes turns with the track four below, et_heard_out) */
#endif
       ET_ROLES };
#define ET_ORDERS 5u                                  /* ORDER: 0 AUTO, 1ST..4TH */
#define ET_RANK_LO 1u                                 /* the layering order, low (in first, out last) to high */
#define ET_RANK_MEL (4u + ET_MELODY)                  /* an AUTO melody's: the top (it waits for a bass or harmony) */
#define ET_RANK_HI ET_RANK_MEL
#define ET_SECTION 4u                                 /* FORM OFF: a section every 4 phrases */
#if TESSERA_TRACKS > 4
enum { ET_G_AUTO, ET_G_IN, ET_G_OUT, ET_G_FADE };    /* the grid's cells (saved: TSP_EGRID) */
#define ET_G_SECS 3u                                  /* the FORM's letters: A, B, C */
#endif

static struct {
    uint32_t seed;                                    /* the SEEDs' hash, at PLAY */
    uint32_t bar;                                     /* the bar last seen (0xFFFFFFFF: none yet) */
    uint32_t pend_p;                                  /* the phrase the pending changes are for */
    uint8_t out;                                      /* the tracks taken out */
    uint8_t pend_in, pend_out;                        /* the changes decided for the next phrase line (the warning) */
    uint8_t lock;                                     /* brought back by hand: left alone until the next section */
    uint8_t heard;                                    /* the tracks the sequencer gave notes since PLAY */
    uint8_t want;                                     /* how many tracks the rules want out now */
    uint8_t pend_sec;                                 /* the line pending is a section's */
    uint8_t final;                                    /* a planned piece's END STOP: every track out, the ANCHORs too */
    uint8_t since[TESSERA_TRACKS];                    /* phrases a track has been in or out since it changed, up to 255 */
    uint16_t avg[TESSERA_TRACKS];                     /* its notes' average, x16 (AUTO's bass), 0 none yet */
    uint8_t guess[TESSERA_TRACKS];                    /* AUTO's guess, held while playing (et_guess_all) */
#if TESSERA_TRACKS > 4
    uint8_t alts;                                     /* TURNS: the tracks 5..8 taking turns (et_alts, at PLAY and each line) */
    uint8_t turn, pend_turn;                          /* TURNS: those heard in place of their partner; on the next line */
    uint8_t fin, fout, gout;                          /* the grid, this phrase: fading in, fading out; its OUT cells */
    uint8_t pend_fin, pend_fout, pend_gout;           /* the same, the next phrase's */
#endif
} et;
#if TESSERA_TRACKS > 4
#define ET_ALTS ((uint32_t)et.alts)
#else
#define ET_ALTS 0u                                    /* (four tracks: no TURNS) */
#endif

/* the grid: the tracks whose cell in letter sec is `kind`; 1 if a cell is not AUTO */
#if TESSERA_TRACKS > 4
static uint32_t et_cells(uint32_t sec, uint32_t kind)
{
    uint32_t tr, m = 0, g = tessera.et_grid[sec < ET_G_SECS ? sec : 0u];
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if ((g >> (2u * tr) & 3u) == kind) m |= 1u << tr;
    return m;
}
static int et_grid_on(void) { return (tessera.et_grid[0] | tessera.et_grid[1] | tessera.et_grid[2]) != 0; }
#else
#define et_grid_on() 0
#endif

static int32_t tess_flourish(const track_t *t, uint32_t tr);
static int32_t tess_trn(const track_t *t, uint32_t tr);
static int32_t tess_key(const track_t *t, uint32_t tr);
static void mute_tick(void);                          /* (MENU MUTES, below) */

static uint32_t et_hash(uint32_t p, uint32_t salt)    /* a draw: the SEEDs, the phrase, what is drawn */
{
    uint32_t k = et.seed ^ (p * 0x9E3779B9u) ^ (salt * 0xC2B2AE35u);
    k = (k ^ (k >> 16)) * 0x7FEB352Du;
    k = (k ^ (k >> 15)) * 0x846CA68Bu;
    return k ^ (k >> 16);
}
static uint32_t et_count(uint32_t m) { uint32_t n = 0; for (; m; m &= m - 1u) n++; return n; }

/* track tr's register, x16: the notes heard since PLAY; else what its settings say: a generator's middle (C3 and a
 * fifth, its ROOT; ANSWER the keys' C4) and TRN, a STEP pattern its notes' average; 0 none (an empty pattern) */
static uint32_t et_pitch(uint32_t tr)
{
    const track_t *t = &trk[tr];
    uint32_t i, j, n = 0, sum = 0, len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
    if (et.avg[tr]) return et.avg[tr];
    if (tessera.mode[tr] != TESS_ENG_OFF) {
        int32_t p = (tessera.mode[tr] == TESS_ENG_ANSWER ? 60 : 55 + (int32_t)(t->p[P_ROOT] % 12)) + tess_trn(t, tr);
        return (uint32_t)(p < 1 ? 1 : p > 127 ? 127 : p) << 4;
    }
    for (i = 0; i < len && i < NSTEP; i++)
        if (t->step[i].time != ST_REST)
            for (j = 0; j < t->step[i].n && j < 4u; j++) sum += t->step[i].note[j], n++;
    return n ? (sum << 4) / n : 0u;
}
/* 1: a STEP track that plays chords: its keys' CHRD on, or most of its pattern's notes in steps of three or more */
static int et_chords(uint32_t tr)
{
    const track_t *t = &trk[tr];
    uint32_t i, k = 0, c = 0, len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
    if (tessera.mode[tr] != TESS_ENG_OFF) return 0;
    if (t->p[P_CHRD]) return 1;
    for (i = 0; i < len && i < NSTEP; i++)
        if (t->step[i].time != ST_REST && t->step[i].n) k++, c += t->step[i].n >= 3u;
    return k && c * 2u > k;
}
/* 1: a track of hits rather than notes: GRIDS, a kit (DRUM; SLICE's slices: the engine's own key map) or NOISE */
static int et_hits(uint32_t tr)
{
    const track_t *t = &trk[tr];
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return tessera.mode[tr] == TESS_ENG_GRIDS || drum_track(t) || eng_idx(t->eng_req) == ENGI_NOISE ||
           (e->keys && e->keys(t, 0) >= 0);
}
/* AUTO's guesses, the tracks judged together (et.guess): CHORD or a STEP track of chords HARMONY; hits RHYTHM; the
 * rest by register: below C3 BASS, and the lowest a fifth or more below every other track with notes (a HARMONY's
 * too) BASS; else MELODY. Worked out at PLAY and on each section's line, so a guess holds through a section */
static void et_guess_all(void)
{
    uint32_t tr, p[TESSERA_TRACKS], lo = TESSERA_TRACKS, next = 0xFFFFu;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) {
        uint32_t g = tessera.mode[tr] == TESS_ENG_CHORD || et_chords(tr) ? ET_HARMONY : et_hits(tr) ? ET_RHYTHM : ET_MELODY;
        p[tr] = g == ET_RHYTHM ? 0u : et_pitch(tr);
        if (g == ET_MELODY && p[tr] && p[tr] < 48u * 16u) g = ET_BASS;
        et.guess[tr] = (uint8_t)g;
        if (g == ET_MELODY && p[tr] && (lo == TESSERA_TRACKS || p[tr] < p[lo])) lo = tr;
    }
    if (lo == TESSERA_TRACKS) return;
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (tr != lo && p[tr] && p[tr] < next) next = p[tr];
    if (next != 0xFFFFu && p[lo] + 7u * 16u <= next) et.guess[lo] = ET_BASS;
}
/* track tr's role: as set, or AUTO's guess */
static uint32_t et_role(uint32_t tr)
{
    uint32_t r = tessera.et_role[tr];
#if TESSERA_TRACKS > 4
    if (r == ET_TURNS && tr < 4u) r = ET_AUTO;        /* (TURNS: a track 5..8's; on 1..4, AUTO) */
#endif
    if (r && r < ET_ROLES) return r;
    return et.guess[tr] ? et.guess[tr] : ET_MELODY;
}
/* track tr's place in the layering order, ET_RANK_LO..ET_RANK_HI: an ORDER set, 1..4; AUTO, after them by ROLE, 4 +
 * BASS..MELODY (no ORDER set anywhere: the role order, as before ORDER); ANCHOR none (ET_RANK_HI + 1: never moved).
 * Tracks of one place take turns as the draw says */
static uint32_t et_rank(uint32_t tr)
{
    uint32_t o = tessera.et_order[tr], r = et_role(tr);
    if (r == ET_ANCHOR) return ET_RANK_HI + 1u;
    return o && o < ET_ORDERS ? o : 4u + r;
}
static uint32_t et_mask(uint32_t role)                /* the tracks of a role */
{
    uint32_t tr, m = 0;
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (et_role(tr) == role) m |= 1u << tr;
    return m;
}
/* the tracks with something to play: a generator, or a STEP pattern with a note (or heard since PLAY) */
static uint32_t et_present(void)
{
    uint32_t tr, i, m = et.heard;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) {
        uint32_t len = trk[tr].p[P_SLEN] > 0 ? (uint32_t)trk[tr].p[P_SLEN] : 1u;
        if (tessera.mode[tr] != TESS_ENG_OFF) m |= 1u << tr;
        for (i = 0; i < len && i < NSTEP && !(m >> tr & 1u); i++)
            if (trk[tr].step[i].time != ST_REST) m |= 1u << tr;
    }
    return m;
}
static uint32_t et_hand_muted(void)
{
    uint32_t tr, m = 0;
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (trk[tr].p[P_MUTE]) m |= 1u << tr;
    return m;
}

/* TURNS (ROLE on tracks 5..8): the track and the one four below take turns. The pair is one place to the rules (the
 * lower track's: in or out as Entries says); in it one of the two is heard, the one drawn on each section's line (the
 * lower one from PLAY and on every return to A). Both need something to play; else the TURNS track plays on, never moved. An
 * 8-track build's (the plugin: GENERATE's alternates); the FM-1 has four tracks and none of this */
#if TESSERA_TRACKS > 4
static uint32_t et_alts(void)
{
    uint32_t tr, m = 0, pr = et_present();
    for (tr = 4; tr < TESSERA_TRACKS; tr++)
        if (tessera.et_role[tr] == ET_TURNS && (pr >> tr & 1u) && (pr >> (tr - 4u) & 1u)) m |= 1u << tr;
    return m;
}
#endif
/* the tracks out as heard: the rules' (et.out), and of each TURNS pair the one not having its turn */
#if TESSERA_TRACKS > 4
static uint32_t et_heard_out(void)
{
    uint32_t o = et.out;
    uint32_t tr;
    for (tr = 4; tr < TESSERA_TRACKS; tr++) {
        uint32_t q = tr - 4u;
        if (!(et.alts >> tr & 1u)) continue;
        if ((et.out >> q & 1u) || !(et.turn >> tr & 1u)) o |= 1u << tr;
        else o &= ~(1u << tr);
        if (et.turn >> tr & 1u) o |= 1u << q;
    }
    return o;
}
#else
#define et_heard_out() ((uint32_t)et.out)
#endif

/* everything in, levels full (PLAY, STOP, ENTRIES 0) */
static void et_clear(void)
{
    uint32_t tr;
    et.out = et.pend_in = et.pend_out = et.lock = 0;
    et.want = et.final = 0;
#if TESSERA_TRACKS > 4
    et.turn = et.pend_turn = 0;
    et.fin = et.fout = et.gout = et.pend_fin = et.pend_fout = et.pend_gout = 0;
#endif
    et.pend_p = 0xFFFFFFFFu;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) {
        et.since[tr] = 0;
        entry_gain[tr] = 32768;
    }
}

/* seq.c seq_start: from the top, the draws from the project's SEEDs */
void entries_play(void)
{
    uint32_t tr, h = 0x454E5452u;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) h = (h ^ tessera.seed[tr]) * 16777619u;
    et.seed = h;
    et.bar = 0xFFFFFFFFu;
    et.heard = 0;
    memset(et.avg, 0, sizeof et.avg);
    et_clear();
    et_guess_all();                                   /* (AUTO's roles from the settings, for the first section) */
#if TESSERA_TRACKS > 4
    et.alts = (uint8_t)et_alts();
#endif
}

/* what the CHAIN does at phrase p: a section begins there; it is a return to A; the next phrase is one, after two
 * phrases or more away (a single phrase away is no breakdown); *let phrase p's letter, *last 1 when the next phrase
 * begins a section, *nlet the next phrase's letter (FORM OFF: A) */
static void et_chain(uint32_t p, uint32_t *sec, uint32_t *home, uint32_t *home_next, uint32_t *let, uint32_t *last,
                     uint32_t *nlet)
{
    fc_st_t a, b, c;
    *sec = p % ET_SECTION == 0u;
    *last = (p + 1u) % ET_SECTION == 0u;
    *home = *home_next = *let = *nlet = 0;
    if (!tessera.fc_form || !fc_len() || !p) return;
    a = *fc_at(p - 1u);                               /* (the phrase playing; the next two worked out on a copy) */
    b = a;
    fc_step(&b);
    c = b;
    fc_step(&c);
    *sec = b.moved & 1u;
    *home = *sec && !fc_letter_at(b.pos) && fc_letter_at(a.pos);
    *home_next = (c.moved & 1u) && !fc_letter_at(c.pos) && b.away >= 2u;   /* (not a lone phrase away) */
    *let = fc_letter_at(b.pos);
    *nlet = fc_letter_at(c.pos);
    *last = c.moved & 1u;
}

/* the changes for phrase p, a bar before its line: which tracks go out, which come in */
static void et_decide(uint32_t p)
{
    uint32_t sec, home, home_next, tr, i, r, n, h;
    uint32_t mute = et_hand_muted(), anchor = et_mask(ET_ANCHOR), lock = et.lock;
    uint32_t dwell = tessera.et_dwell < 1u ? 1u : tessera.et_dwell > 8u ? 8u : tessera.et_dwell;
    uint32_t can, ready = 0, play, sup, mel = 0;
    uint32_t go_out = 0, come_in = 0, most, wanted, let, last, nlet, gx = 0;
#if TESSERA_TRACKS > 4
    uint32_t g_in, g_out, g_fade;
#endif
    et_chain(p, &sec, &home, &home_next, &let, &last, &nlet);
    if (sec) et_guess_all();                          /* (a new section: AUTO looks again, by what it has heard) */
    sup = et_mask(ET_BASS) | et_mask(ET_HARMONY);
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (et_rank(tr) == ET_RANK_MEL) mel |= 1u << tr;   /* (the melodies that wait for their ground: AUTO's) */
    if (sec) lock = 0;                                /* (a new section: the hand's tracks are the rules' again) */
#if TESSERA_TRACKS > 4
    et.alts = (uint8_t)et_alts();
#endif
    can = et_present() & ~mute & ~anchor & ~lock & ~ET_ALTS;   /* (the tracks the rules may move; TURNS:
                                                       * the pair's lower track stands for both) */
#if TESSERA_TRACKS > 4
    g_fade = et_cells(let, ET_G_FADE) & ~mute;         /* (the grid: its IN, OUT and FADE cells, out of the rules' */
    g_in = (et_cells(let, ET_G_IN) & ~mute) | g_fade;  /* hands) */
    g_out = et_cells(let, ET_G_OUT) & ~mute & ~lock;
    gx = g_in | g_out;
    can &= ~gx;
#endif
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if ((uint32_t)et.since[tr] + 1u >= dwell) ready |= 1u << tr;
    play = et_present() & ~mute & ~et.out & ~ET_ALTS;   /* (sounding now, as the rules count) */
#if TESSERA_TRACKS > 4
    play &= ~g_out;                                   /* (the grid's OUT: as gone) */
#endif
    most = (et_count(can) * tessera.et_amount + 50u) / 100u;
    if (!(play & anchor) && most >= et_count(can)) most = et_count(can) ? et_count(can) - 1u : 0u;   /* (one plays) */
    h = et_hash(p, 1u);
    if (home && tessera.et_amount > 50u && !pl_on()) {   /* A comes back with everything (planned: the SHAPE says) */
        et.want = 0;
        come_in = et.out & ~mute;
    } else if (home_next && tessera.et_amount > 50u && !pl_on()) {   /* the phrase before A: down to the anchors */
        uint32_t keep = play & anchor;
        if (!keep)                                    /* (none: the one lowest in the layering order plays on) */
            for (r = ET_RANK_LO; r <= ET_RANK_HI && !keep; r++)
                for (tr = 0; tr < TESSERA_TRACKS && !keep; tr++)
                    if (((play & can) >> tr & 1u) && et_rank(tr) == r) keep = 1u << tr;
        go_out = play & can & ~keep;
    } else {
        if (pl_on()) {                                /* a planned piece: as many out as its SHAPE is low */
            et.want = (uint8_t)planner_out(p, most);
        } else if (sec) {                             /* a section: how many out, a step up or down */
            int32_t w = (int32_t)et.want;
            if (home) w--;
            else if (!tessera.fc_form || !fc_len()) w += (int32_t)(h % 3u) - 1;
            else w += h % 4u ? 1 : -1;                /* (away from A: mostly thinner) */
            et.want = (uint8_t)(w < 0 ? 0 : w > (int32_t)most ? most : (uint32_t)w);
        } else if (et.want > most) {
            et.want = (uint8_t)most;
        }
        wanted = et.want;
        n = et_count(et.out & ~mute & ~gx);
        if (sec && n == wanted && wanted && (h >> 8) % 2u) {   /* (as many as wanted out already: now and then one swaps) */
            for (r = ET_RANK_LO; r <= ET_RANK_HI && !come_in; r++)
                for (i = 0; i < TESSERA_TRACKS && !come_in; i++) {
                    tr = (i + h) % TESSERA_TRACKS;
                    if ((et.out & can & ready) >> tr & 1u && et_rank(tr) == r && (r != ET_RANK_MEL || (play & sup)))
                        come_in = 1u << tr;
                }
            if (come_in) n--;
        }
        if (n < wanted && (sec || pl_on()))           /* exits: on a section's line (planned: one a phrase between),
                                                       * the top of the order first */
            for (r = ET_RANK_HI; r >= ET_RANK_LO && n < wanted && (sec || !go_out); r--)
                for (i = 0; i < TESSERA_TRACKS && n < wanted && (sec || !go_out); i++) {
                    uint32_t left;
                    tr = (i + (h >> 4)) % TESSERA_TRACKS;
                    if (!((play & can & ready & ~come_in) >> tr & 1u) || et_rank(tr) != r) continue;
                    left = (play | come_in) & ~go_out & ~(1u << tr);
                    if (!left) continue;              /* (one always plays) */
                    if ((sup >> tr & 1u) && (left & mel) && !(left & sup)) continue;   /* (a melody keeps its ground) */
                    go_out |= 1u << tr;
                    n++;
                }
        if (n > wanted && !come_in)                   /* entries: one a phrase, the ground first */
            for (r = ET_RANK_LO; r <= ET_RANK_HI && !come_in; r++)
                for (i = 0; i < TESSERA_TRACKS && !come_in; i++) {
                    tr = (i + (h >> 12)) % TESSERA_TRACKS;
                    if (!((et.out & ~mute & ~gx & ready) >> tr & 1u) || et_rank(tr) != r) continue;
                    if (r == ET_RANK_MEL && !(play & sup)) continue;   /* (an AUTO melody over a bass or harmony) */
                    come_in = 1u << tr;
                }
    }
#if TESSERA_TRACKS > 4
    if (gx | et.fout) {                               /* the grid: IN and FADE in, OUT out, a faded track out */
        uint32_t after;
        come_in = (come_in | (g_in & et.out)) & ~g_out;
        go_out = (go_out | (g_out & ~et.out) | (et.fout & ~et.out & ~mute & ~g_in)) & ~g_in;
        after = et_present() & ~mute & ~ET_ALTS & ~((et.out | go_out) & ~come_in);
        if (!after && go_out) go_out &= go_out - 1u;   /* (one still plays: the lowest track) */
    }
    et.pend_fin = (uint8_t)(sec ? g_fade & (et.out | et.fout) : 0u);
    et.pend_fout = (uint8_t)(last ? g_fade & ~et.pend_fin & ~et_cells(nlet, ET_G_IN) & ~et_cells(nlet, ET_G_FADE) : 0u);
    et.pend_gout = (uint8_t)g_out;
#else
    (void)gx, (void)let, (void)last, (void)nlet;
#endif
    et.pend_in = (uint8_t)come_in;
    et.pend_out = (uint8_t)go_out;
    et.pend_sec = (uint8_t)sec;
    et.pend_p = p;
#if TESSERA_TRACKS > 4
    et.pend_turn = et.turn;
    if (sec)                                          /* TURNS: whose turn, each section (A: the lower track's) */
        for (tr = 4; tr < TESSERA_TRACKS; tr++)
            if (!(et.alts >> tr & 1u) || home) et.pend_turn &= (uint8_t)~(1u << tr);
            else if (et_hash(p, 16u + tr) >> 7 & 1u) et.pend_turn |= (uint8_t)(1u << tr);
            else et.pend_turn &= (uint8_t)~(1u << tr);
#endif
}

/* phrase p's line: the decided changes happen */
static void et_apply(uint32_t p)
{
    uint32_t tr;
    if (et.pend_p == p) {
        et.out = (uint8_t)((et.out | et.pend_out) & ~et.pend_in);
#if TESSERA_TRACKS > 4
        et.turn = et.pend_turn;
        et.fin = et.pend_fin, et.fout = et.pend_fout, et.gout = et.pend_gout;
        for (tr = 0; tr < TESSERA_TRACKS; tr++)
            if (et.fin >> tr & 1u) entry_gain[tr] = 0;   /* (a FADE in: from silence) */
#endif
        if (et.pend_sec) et.lock = 0;
    }
#if TESSERA_TRACKS > 4
    else
        et.fin = et.fout = 0;                         /* (a fade: one phrase) */
#endif
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (et.pend_p == p && ((et.pend_in | et.pend_out) >> tr & 1u)) et.since[tr] = 0;
        else if (et.since[tr] < 255u) et.since[tr]++;
    et.pend_in = et.pend_out = 0;
#if TESSERA_TRACKS > 4
    et.pend_fin = et.pend_fout = 0;
#endif
    et.pend_p = 0xFFFFFFFFu;
}

/* seq.c seq_advance, each block before the steps: the phrase lines (a bar line falls at a block's end, so the
 * downbeat's steps play after this) and the levels' fades */
/* a planned piece's start: as many tracks out as its SHAPE is low there, at once (the intro layers in from them) */
static void et_intro(void)
{
    uint32_t mute = et_hand_muted(), can = et_present() & ~mute & ~et_mask(ET_ANCHOR) & ~ET_ALTS,
             play = et_present() & ~mute & ~ET_ALTS;
    uint32_t most = (et_count(can) * tessera.et_amount + 50u) / 100u, want, r, tr, n = 0;
    if (!(play & et_mask(ET_ANCHOR)) && most >= et_count(can)) most = et_count(can) ? et_count(can) - 1u : 0u;
    want = planner_out(0, most);
    for (r = ET_RANK_HI; r >= ET_RANK_LO && n < want; r--)
        for (tr = 0; tr < TESSERA_TRACKS && n < want; tr++)
            if ((can >> tr & 1u) && et_rank(tr) == r) {
                et.out |= (uint8_t)(1u << tr);
                entry_gain[tr] = 0;
                n++;
            }
    et.want = (uint8_t)want;
}

/* PLAY: the grid's A cells for the first phrase (OUT out at once, FADE in from silence) */
#if TESSERA_TRACKS > 4
static void et_grid_start(void)
{
    uint32_t tr, mute = et_hand_muted(), o = et_cells(0, ET_G_OUT) & ~mute;
    if (!(et_present() & ~mute & ~ET_ALTS & ~(et.out | o))) o &= o - 1u;   /* (one plays) */
    et.out |= (uint8_t)o;
    et.gout = (uint8_t)o;
    et.fin = (uint8_t)(et_cells(0, ET_G_FADE) & ~mute);
    et.out &= (uint8_t)~(et_cells(0, ET_G_IN) | et.fin);
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if ((o | et.fin) >> tr & 1u) entry_gain[tr] = 0;
}
#endif

/* planner.c, a planned piece's END STOP: every track out, the ANCHORs too (its FADE, then the tails) */
static void entries_all_out(void)
{
    et.out = (uint8_t)((1u << TESSERA_TRACKS) - 1u);
    et.pend_in = et.pend_out = 0;
    et.final = 1;
}

void entries_tick(uint32_t n)
{
    uint32_t tr, b = bar_n, bars = fc_bars(), step, heard;
    mute_tick();                                      /* (MENU MUTES: before the downbeat's steps) */
    planner_tick(n);
    fc_pc_tick();                                     /* (SECTION PC: form_chain.c, on the same phrase line) */
    if (et.final && song.playing) {
        /* (the end: everything fades out, nothing decided) */
    } else if (!song.playing || (!tessera.et_amount && !et_grid_on())) {
        if (et.out || et.pend_in || et.pend_out || et.lock || et_heard_out()) {   /* (off or stopped: everything
                                                       * back in) */
            et.out = et.pend_in = et.pend_out = et.lock = 0;
#if TESSERA_TRACKS > 4
            et.turn = et.pend_turn = 0;
            et.fin = et.fout = et.gout = 0;
#endif
            et.pend_p = 0xFFFFFFFFu;
        }
        et.bar = song.playing ? b : 0xFFFFFFFFu;
        if (!song.playing) {
            for (tr = 0; tr < TESSERA_TRACKS; tr++) entry_gain[tr] = 32768;
            return;
        }
    } else if (b != et.bar) {
        if (et.bar == 0xFFFFFFFFu && !b) {
            if (pl_on()) et_intro();
#if TESSERA_TRACKS > 4
            if (et_grid_on()) et_grid_start();
#endif
        } else if (et.bar != 0xFFFFFFFFu && b == et.bar + 1u) {
            if (b % bars == bars - 1u) et_decide(b / bars + 1u);   /* (the phrase coming: decided, warned a bar ahead) */
            else if (b % bars == 0u) et_apply(b / bars);
        }
        et.bar = b;
    }
#if TESSERA_TRACKS > 4
    if (!et.final) et.out &= (uint8_t)~(et_mask(ET_ANCHOR) & ~et.gout);   /* (an ANCHOR never drops, set so now: back at
                                                       * once; but where the grid has it OUT) */
#else
    if (!et.final) et.out &= (uint8_t)~et_mask(ET_ANCHOR);   /* (an ANCHOR never drops, set so now: back at once) */
#endif
    step = tessera.et_style ? 32768u * n / ((tessera.et_fade ? tessera.et_fade : 1u) * 4u * seq_beat_samples() + 1u) + 1u
                            : 32768u;
    heard = et_heard_out();
#if TESSERA_TRACKS > 4
    if (song.playing && (et.fin | et.fout)) {          /* the grid's FADEs: the level across the phrase */
        uint32_t bars = fc_bars(), beat = seq_beat_samples(), part;
        part = ((b % bars * 4u + beat_n) * beat + beat_pos % beat) / ((bars * 4u * beat >> 10) + 1u);
        part = part > 1024u ? 1024u : part;
        for (tr = 0; tr < TESSERA_TRACKS; tr++)
            if (!(heard >> tr & 1u) && ((et.fin | et.fout) >> tr & 1u))
                entry_gain[tr] = (int32_t)((et.fin >> tr & 1u) ? part : 1024u - part) * 32;
    }
#endif
    for (tr = 0; tr < TESSERA_TRACKS; tr++) {
        int32_t g = entry_gain[tr], to = (heard >> tr & 1u) ? 0 : 32768, d = to - g;
#if TESSERA_TRACKS > 4
        if ((et.fin | et.fout) >> tr & 1u && !(heard >> tr & 1u)) continue;   /* (set above) */
#endif
        if (d > (int32_t)step) d = (int32_t)step;
        else if (d < -(int32_t)step) d = -(int32_t)step;
        entry_gain[tr] = g + d;
    }
}

/* voice.c trk_note_on: 1 plays the note, 0 leaves it out (the track is out and faded) */
static int entries_note(const track_t *t, uint32_t note)
{
    uint32_t tr = (uint32_t)(t - trk), b;
    if (tr >= TESSERA_TRACKS || !midi_out_seq) return 1;   /* (the keys, MIDI IN: as played) */
    b = 1u << tr;
    et.heard |= (uint8_t)b;
    et.avg[tr] = (uint16_t)(et.avg[tr] ? (int32_t)et.avg[tr] + ((int32_t)(note << 4) - (int32_t)et.avg[tr]) / 8 : (int32_t)(note << 4));
    if (!(et_heard_out() & b) || (track_solo & b)) return 1;
    if ((et.pend_in & b) && tess_flourish(t, tr)) {   /* (its FLOURISH leads it in) */
        et.out &= (uint8_t)~b;
        entry_gain[tr] = 32768;
        return 1;
    }
    return entry_gain[tr] > 0;
}

/* voice.c trk_note_on: the velocity a sequencer note goes to MIDI OUT with. While a FADE takes a track out or brings
 * it in (its level under full), the velocity is scaled by that level, so a synth on MIDI OUT fades with the FM-1's own
 * sound (one that ignores velocity still cuts). Never under 1: a note-on of 0 is a note-off, and a note the FM-1 still
 * plays, faintly, goes out faintly too, so every note-on keeps its note-off (no floor that drops notes). CUT (the level
 * 0 or full), a level full, SOLO (heard full), the keys and MIDI IN: as played. The FM-1's own voice and LANDSCAPE
 * keep the velocity (the mix fades the level already) */
static uint32_t entries_vel(const track_t *t, uint32_t vel)
{
    uint32_t tr = (uint32_t)(t - trk), g;
    if (!vel || tr >= TESSERA_TRACKS || !midi_out_seq || (track_solo >> tr & 1u) ||
        (g = (uint32_t)entry_gain[tr]) >= 32768u)
        return vel;
    vel = (vel > 127u ? 127u : vel) * g >> 15;        /* (as MIDI OUT would send it, 1..127, then scaled) */
    return vel ? vel : 1u;
}

/* MENU MUTES (hand mutes on the beat): GLO's mute keys and C4's UNMUTE ALL, while playing, wait for the next bar line
 * (BAR) or phrase line (PHRASE, the CHAIN's); the key blinks until then, and pressed again it is called off. NOW (0):
 * at once, as before. Stopped: at once. */
static uint8_t mute_pend;                             /* the tracks whose mute key waits for the line */
static uint32_t mute_bar = 0xFFFFFFFFu;               /* the bar last seen playing */
static void mute_now(uint32_t tr)                     /* the mute key's change: muted, or back in */
{
    track_t *t = &trk[tr];
    if (t->p[P_MUTE]) {
        t->p[P_MUTE] = 0;
        (void)entries_hand_in(tr);                    /* (out by Entries meanwhile: back in too) */
    } else if (!entries_hand_in(tr)) {                /* (out by Entries: back in, left alone a section) */
        t->p[P_MUTE] = 1;
        tessera_kill(t);
    }
}
/* GLO's black key tr (all: C4's UNMUTE ALL): now, or on the line MUTES says */
void mute_press(uint32_t tr, int all)
{
    uint32_t i, m = 0;
    for (i = 0; i < TESSERA_TRACKS; i++) m |= (trk[i].p[P_MUTE] != 0) << i;
    if (song.playing && settings_mute_q) {
        mute_pend = all ? (uint8_t)m : (uint8_t)(mute_pend ^ (tr < TESSERA_TRACKS ? 1u << tr : 0u));
    } else if (all) {
        for (i = 0; i < TESSERA_TRACKS; i++) trk[i].p[P_MUTE] = 0;
        mute_pend = 0;
    } else if (tr < TESSERA_TRACKS) {
        mute_now(tr);
    }
}
uint32_t mute_waiting(void) { return song.playing ? mute_pend : 0u; }   /* the keys that blink */
/* MIXER's underline under a waiting MUTE (ui_mono.c mo_strip): how much of the bar or phrase before its line has gone,
 * in w parts (0..w-1; the bars counted from PLAY, as mute_tick counts them) */
uint32_t mute_wait_part(uint32_t w)
{
    uint32_t n = settings_mute_q < 2u ? 1u : fc_bars(), beat = seq_beat_samples();
    return ((bar_n % n * 4u + beat_n) * beat + beat_pos % beat) * w / (n * 4u * beat);
}
static void mute_tick(void)                           /* entries_tick: a waiting key's line */
{
    uint32_t b = bar_n, tr;
    if (mute_pend && (!song.playing || !settings_mute_q ||
                      (mute_bar != 0xFFFFFFFFu && b != mute_bar && (settings_mute_q < 2u || b % fc_bars() == 0u)))) {
        for (tr = 0; tr < TESSERA_TRACKS; tr++)
            if (mute_pend >> tr & 1u) mute_now(tr);
        mute_pend = 0;
    }
    mute_bar = song.playing ? b : 0xFFFFFFFFu;
}

/* GLO's mute key on track tr while it is out: back in now, left alone until the next section. 1 if it was out */
int entries_hand_in(uint32_t tr)
{
    uint32_t b = 1u << tr;
    if (tr >= TESSERA_TRACKS || !(et_heard_out() & b)) return 0;
#if TESSERA_TRACKS > 4
    if (tr >= 4u && (et.alts >> tr & 1u)) et.turn |= (uint8_t)b, et.pend_turn |= (uint8_t)b;   /* (TURNS: its turn) */
    if (tr < 4u && (et.alts >> (tr + 4u) & 1u))
        et.turn &= (uint8_t)~(b << 4), et.pend_turn &= (uint8_t)~(b << 4);
    if (!(et.out & b)) { entry_gain[tr] = 32768; return 1; }
#endif
    et.out &= (uint8_t)~b;
    et.pend_out &= (uint8_t)~b;
    et.lock |= (uint8_t)b;
    et.since[tr] = 0;
    entry_gain[tr] = 32768;
    return 1;
}

/* the ENTRIES page and GLO's keys: the tracks out, those to change on the next line (the warning), each one's role
 * (AUTO's guess shown) */
uint32_t tessera_entries_get(uint32_t *warn, uint8_t role[TESSERA_TRACKS])
{
    uint32_t tr;
    if (warn) {
        uint32_t w = (uint32_t)(et.pend_in | et.pend_out);
#if TESSERA_TRACKS > 4
        uint32_t x = (uint32_t)(et.turn ^ et.pend_turn) & 0xF0u;
        w |= x | x >> 4;                              /* (TURNS: the pair changing hands) */
#endif
        *warn = song.playing && (tessera.et_amount || et_grid_on()) ? w : 0u;
    }
    if (role && !song.playing) {                      /* (stopped: AUTO's guess from the settings as they are now) */
        memset(et.avg, 0, sizeof et.avg);
        et_guess_all();
    }
    if (role)
        for (tr = 0; tr < TESSERA_TRACKS; tr++) role[tr] = (uint8_t)et_role(tr);
    return song.playing && (tessera.et_amount || et.final || et_grid_on()) ? et_heard_out() : 0u;
}
