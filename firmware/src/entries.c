/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Entries (long form, slice 2): tracks drop out and come back on the project's phrase lines. The ENTRIES page (GLO):
 * ENTRIES 0..100 (0 off: every track as set), DWELL 1-8 phrases a track stays in or out at least, STYLE CUT / FADE and
 * FADE 1-4 bars; OCT+ held, KNOB k track k's ROLE (also OCT+ held on FORM, KNOB 4): AUTO, BASS, RHYTHM, HARMONY,
 * MELODY or ANCHOR (never taken out). AUTO: CHORD harmony; ATLAS or a drum sound rhythm; notes mostly below C3 bass;
 * else melody. OCT- held, KNOB k track k's ORDER: AUTO, 1ST..4TH (et_rank: the order it comes in).
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
 *   A track out is silent like a muted one (no notes, none to MIDI OUT; the mix's level faded or cut). Hand wins: a
 *   track muted by hand stays muted; its key on GLO brings an out track back, left alone until the next section; SOLO
 *   is heard whatever Entries says. Every draw is a hash of the SEEDs taken at PLAY, so a piece replays. Included by
 *   tessera.c after form_chain.c. */

enum { ET_AUTO, ET_BASS, ET_RHYTHM, ET_HARMONY, ET_MELODY, ET_ANCHOR, ET_ROLES };
#define ET_ORDERS 5u                                  /* ORDER: 0 AUTO, 1ST..4TH */
#define ET_RANK_LO 1u                                 /* the layering order, low (in first, out last) to high */
#define ET_RANK_MEL (4u + ET_MELODY)                  /* an AUTO melody's: the top (it waits for a bass or harmony) */
#define ET_RANK_HI ET_RANK_MEL
#define ET_SECTION 4u                                 /* FORM OFF: a section every 4 phrases */

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
} et;

static int32_t tess_flourish(const track_t *t, uint32_t tr);
static void mute_tick(void);                          /* (MENU MUTES, below) */

static uint32_t et_hash(uint32_t p, uint32_t salt)    /* a draw: the SEEDs, the phrase, what is drawn */
{
    uint32_t k = et.seed ^ (p * 0x9E3779B9u) ^ (salt * 0xC2B2AE35u);
    k = (k ^ (k >> 16)) * 0x7FEB352Du;
    k = (k ^ (k >> 15)) * 0x846CA68Bu;
    return k ^ (k >> 16);
}
static uint32_t et_count(uint32_t m) { uint32_t n = 0; for (; m; m &= m - 1u) n++; return n; }

/* track tr's role: as set, or AUTO's guess */
static uint32_t et_role(uint32_t tr)
{
    uint32_t r = tessera.et_role[tr];
    if (r && r < ET_ROLES) return r;
    if (tessera.mode[tr] == TESS_ENG_CHORD) return ET_HARMONY;
    if (tessera.mode[tr] == TESS_ENG_GRIDS || drum_track(&trk[tr])) return ET_RHYTHM;
    if (et.avg[tr] && et.avg[tr] < 48u * 16u) return ET_BASS;
    return ET_MELODY;
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

/* everything in, levels full (PLAY, STOP, ENTRIES 0) */
static void et_clear(void)
{
    uint32_t tr;
    et.out = et.pend_in = et.pend_out = et.lock = 0;
    et.want = et.final = 0;
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
}

/* what the CHAIN does at phrase p: a section begins there; it is a return to A; the next phrase is one, after two
 * phrases or more away (a single phrase away is no breakdown) */
static void et_chain(uint32_t p, uint32_t *sec, uint32_t *home, uint32_t *home_next)
{
    fc_st_t a, b, c;
    *sec = p % ET_SECTION == 0u;
    *home = *home_next = 0;
    if (!tessera.fc_form || !fc_len() || !p) return;
    a = *fc_at(p - 1u);                               /* (the phrase playing; the next two worked out on a copy) */
    b = a;
    fc_step(&b);
    c = b;
    fc_step(&c);
    *sec = b.moved & 1u;
    *home = *sec && !fc_letter_at(b.pos) && fc_letter_at(a.pos);
    *home_next = (c.moved & 1u) && !fc_letter_at(c.pos) && b.away >= 2u;   /* (not a lone phrase away) */
}

/* the changes for phrase p, a bar before its line: which tracks go out, which come in */
static void et_decide(uint32_t p)
{
    uint32_t sec, home, home_next, tr, i, r, n, h;
    uint32_t mute = et_hand_muted(), anchor = et_mask(ET_ANCHOR), lock = et.lock;
    uint32_t dwell = tessera.et_dwell < 1u ? 1u : tessera.et_dwell > 8u ? 8u : tessera.et_dwell;
    uint32_t can, ready = 0, play, sup = et_mask(ET_BASS) | et_mask(ET_HARMONY), mel = 0;
    uint32_t go_out = 0, come_in = 0, most, wanted;
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (et_rank(tr) == ET_RANK_MEL) mel |= 1u << tr;   /* (the melodies that wait for their ground: AUTO's) */
    et_chain(p, &sec, &home, &home_next);
    if (sec) lock = 0;                                /* (a new section: the hand's tracks are the rules' again) */
    can = et_present() & ~mute & ~anchor & ~lock;     /* (the tracks the rules may move) */
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if ((uint32_t)et.since[tr] + 1u >= dwell) ready |= 1u << tr;
    play = et_present() & ~mute & ~et.out;            /* (sounding now) */
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
        n = et_count(et.out & ~mute);
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
                    if (!((et.out & ~mute & ready) >> tr & 1u) || et_rank(tr) != r) continue;
                    if (r == ET_RANK_MEL && !(play & sup)) continue;   /* (an AUTO melody over a bass or harmony) */
                    come_in = 1u << tr;
                }
    }
    et.pend_in = (uint8_t)come_in;
    et.pend_out = (uint8_t)go_out;
    et.pend_sec = (uint8_t)sec;
    et.pend_p = p;
}

/* phrase p's line: the decided changes happen */
static void et_apply(uint32_t p)
{
    uint32_t tr;
    if (et.pend_p == p) {
        et.out = (uint8_t)((et.out | et.pend_out) & ~et.pend_in);
        if (et.pend_sec) et.lock = 0;
    }
    for (tr = 0; tr < TESSERA_TRACKS; tr++)
        if (et.pend_p == p && ((et.pend_in | et.pend_out) >> tr & 1u)) et.since[tr] = 0;
        else if (et.since[tr] < 255u) et.since[tr]++;
    et.pend_in = et.pend_out = 0;
    et.pend_p = 0xFFFFFFFFu;
}

/* seq.c seq_advance, each block before the steps: the phrase lines (a bar line falls at a block's end, so the
 * downbeat's steps play after this) and the levels' fades */
/* a planned piece's start: as many tracks out as its SHAPE is low there, at once (the intro layers in from them) */
static void et_intro(void)
{
    uint32_t mute = et_hand_muted(), can = et_present() & ~mute & ~et_mask(ET_ANCHOR), play = et_present() & ~mute;
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

/* planner.c, a planned piece's END STOP: every track out, the ANCHORs too (its FADE, then the tails) */
static void entries_all_out(void)
{
    et.out = (uint8_t)((1u << TESSERA_TRACKS) - 1u);
    et.pend_in = et.pend_out = 0;
    et.final = 1;
}

void entries_tick(uint32_t n)
{
    uint32_t tr, b = bar_n, bars = fc_bars(), step;
    mute_tick();                                      /* (MENU MUTES: before the downbeat's steps) */
    planner_tick(n);
    if (et.final && song.playing) {
        /* (the end: everything fades out, nothing decided) */
    } else if (!song.playing || !tessera.et_amount) {
        if (et.out || et.pend_in || et.pend_out || et.lock) {   /* (off or stopped: everything back in) */
            et.out = et.pend_in = et.pend_out = et.lock = 0;
            et.pend_p = 0xFFFFFFFFu;
        }
        et.bar = song.playing ? b : 0xFFFFFFFFu;
        if (!song.playing) {
            for (tr = 0; tr < TESSERA_TRACKS; tr++) entry_gain[tr] = 32768;
            return;
        }
    } else if (b != et.bar) {
        if (et.bar == 0xFFFFFFFFu && !b && pl_on()) et_intro();
        else if (et.bar != 0xFFFFFFFFu && b == et.bar + 1u) {
            if (b % bars == bars - 1u) et_decide(b / bars + 1u);   /* (the phrase coming: decided, warned a bar ahead) */
            else if (b % bars == 0u) et_apply(b / bars);
        }
        et.bar = b;
    }
    if (!et.final) et.out &= (uint8_t)~et_mask(ET_ANCHOR);   /* (an ANCHOR never drops, set so now: back at once) */
    step = tessera.et_style ? 32768u * n / ((tessera.et_fade ? tessera.et_fade : 1u) * 4u * seq_beat_samples() + 1u) + 1u
                            : 32768u;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) {
        int32_t g = entry_gain[tr], to = (et.out >> tr & 1u) ? 0 : 32768, d = to - g;
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
    if (!(et.out & b) || (track_solo & b)) return 1;
    if ((et.pend_in & b) && tess_flourish(t, tr)) {   /* (its FLOURISH leads it in) */
        et.out &= (uint8_t)~b;
        entry_gain[tr] = 32768;
        return 1;
    }
    return entry_gain[tr] > 0;
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
    if (tr >= TESSERA_TRACKS || !(et.out & b)) return 0;
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
    if (warn) *warn = song.playing && tessera.et_amount ? (uint32_t)(et.pend_in | et.pend_out) : 0u;
    if (role)
        for (tr = 0; tr < TESSERA_TRACKS; tr++) role[tr] = (uint8_t)et_role(tr);
    return song.playing && (tessera.et_amount || et.final) ? et.out : 0u;
}
