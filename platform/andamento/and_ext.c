/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's tracks 5..8: the external tracks. Tessera's core keeps its 4 tracks (NTRK, the FM-1's, and Andamento's
 * 4 internal voices); these 4 more each run a generator of Tessera's own (gen.c) on the song's clock, in the song's
 * scale, and play out of the box rather than through a voice: a MIDI channel (DIN / TRS / USB on the board, Web MIDI in
 * the browser) and a CV / gate pair (the jack strip's 4 CV and 4 gates, track 5 the first pair). Included by and_ui.c
 * before the pages, so it runs the same in the browser (web/andamento) and on the Daisy Seed; nothing of the FM-1's
 * build changes (the core is read, never written: the scale, the clock's position, ENERGY).
 *
 *   the clock   the sequencer's own position (beat_pos, beat_n, bar_n: seq.c), so the tracks keep time with tracks 1..4
 *               at any tempo or clock source; PLAY starts each from its first step and its SEED
 *   the notes   a generator's raw note (C minor, from C2..C4) moved to ROOT and snapped to track 1's scale (one scale for
 *               the song), then OCT octaves; one note at a time (a mono line: CV and gate are one voice); GATE % of
 *               the step long; VEL scales the generator's velocity
 *   ENERGY      as Tessera's tracks feel it (energy.c energy_hit): sparse leaves steps out (up to 70 %) and softens,
 *               full plays louder, a rest silences; DEPTH how much (0: an anchor, ENERGY passes it by). Their own
 *               random stream, so tracks 1..4 play exactly as they would without them
 *   the outputs MIDI as 3-byte messages in a ring (and_ext_midi_take: the page sends them on, the board's UART later);
 *               CV in millivolts, 1 V an octave, 0 V at C2 (MIDI 36), 0..10 V (the DAC's range on the board: to
 *               settle with the circuit); the gate high while a note sounds
 *
 * The settings (a pid of its own, 0xE0xx, for and_ui.c's pages: GEN, the generator's three, SEED, DIV, OCT, GATE;
 * OUT: CHAN, VEL, DEPTH, MUTE) are not saved in a project yet: a new project format part is for later. */

#define AE_N 4u                                  /* external tracks: Andamento's 5..8 */
#define AE_PID 0xE000u                           /* their settings' pids: AE_PID + AE_* */
#define AE_CV_MAX 10000                          /* mV */
#define AE_MIDI_RING 256u                        /* messages */

enum { AE_OFF, AE_TURING, AE_EUCLID, AE_MARKOV, AE_AWAKE, AE_POLY, AE_SERIES, AE_GENS };
enum { AE_GEN, AE_P1, AE_P2, AE_P3, AE_SEED, AE_DIV, AE_OCT, AE_GATE, AE_VEL, AE_CHAN, AE_DEPTH, AE_MUTE, AE_NPRM };

static const char *const AE_GEN_NAME[AE_GENS] = {"OFF", "TURING", "EUCLID", "MARKOV", "AWAKE", "POLY", "SERIES"};
static const char *const AE_DIV_NAME[6] = {"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"};
static const uint8_t AE_DIV_N[6] = {1, 2, 3, 4, 6, 8};             /* steps a beat */
static const char *const AE_CHAN_NAME[17] = {"OFF", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
                                             "13", "14", "15", "16"};
static const char *const AE_ONOFF[2] = {"OFF", "ON"};

/* each generator's three settings: label, range, default (kept per generator, so a change of GEN keeps them) */
static const param_desc_t AE_GP[AE_GENS][3] = {
    [AE_TURING] = {{"LEN", F_INT, 2, 16, 8, 0, 0}, {"PROB", F_PCT, 0, 100, 10, 0, 0}, {"SPAN", F_INT, 1, 3, 2, 0, 0}},
    [AE_EUCLID] = {{"STEPS", F_INT, 1, 32, 16, 0, 0}, {"HITS", F_INT, 0, 32, 5, 0, 0}, {"ROT", F_INT, 0, 31, 0, 0, 0}},
    [AE_MARKOV] = {{"TENS", F_PCT, 0, 100, 25, 0, 0}, {"DENS", F_PCT, 1, 100, 70, 0, 0}, {"SPAN", F_INT, 1, 3, 2, 0, 0}},
    [AE_AWAKE] = {{"LEN A", F_INT, 1, 16, 8, 0, 0}, {"LEN B", F_INT, 1, 16, 7, 0, 0}, {"PROB", F_PCT, 0, 100, 100, 0, 0}},
    [AE_POLY] = {{"DIV 1", F_INT, 0, 16, 4, 0, 0}, {"DIV 2", F_INT, 0, 16, 3, 0, 0}, {"SPAN", F_INT, 1, 3, 1, 0, 0}},
    [AE_SERIES] = {{"START", F_INT, 0, 127, 0, 0, 0}, {"INTV", F_INT, 1, 7, 1, 0, 0}, {"LOOP", F_INT, 0, 16, 0, 0, 0}},
};
static const param_desc_t AE_DESC[AE_NPRM] = {
    [AE_GEN] = {"GEN", F_ENUM, 0, AE_GENS - 1, AE_TURING, AE_GEN_NAME, 0},
    [AE_SEED] = {"SEED", F_INT, 0, 255, 1, 0, 0},
    [AE_DIV] = {"DIV", F_ENUM, 0, 5, 3, AE_DIV_NAME, 0},
    [AE_OCT] = {"OCT", F_INT, -3, 3, 0, 0, 0},
    [AE_GATE] = {"GATE", F_PCT, 5, 100, 50, 0, 0},
    [AE_VEL] = {"VEL", F_PCT, 0, 100, 100, 0, 0},
    [AE_CHAN] = {"CHAN", F_ENUM, 0, 16, 5, AE_CHAN_NAME, 0},
    [AE_DEPTH] = {"DEPTH", F_PCT, 0, 100, 100, 0, 0},
    [AE_MUTE] = {"MUTE", F_ENUM, 0, 1, 0, AE_ONOFF, 0},
};
static const param_desc_t AE_NONE_DESC = {"-", F_INT, 0, 0, 0, 0, 0};

typedef struct {
    uint8_t gen, seed, div, gate, vel, chan, depth, mute;
    int8_t oct;
    uint8_t gp[AE_GENS][3];                      /* each generator's three */
    gen_turing_t turing;
    gen_euclid_t euclid;
    gen_markov_t markov;
    gen_awake_t awake;
    gen_poly_t poly;
    gen_series_t series;
    uint32_t rnd, en_rnd;                        /* the generator's stream, ENERGY's */
    uint32_t step;                               /* the next step to play, counted from PLAY */
    uint32_t off_at;                             /* the clock's position its note ends at (0: none due) */
    uint8_t note, sent_ch, sounding, key;        /* the note sounding and the channel it went out on; a key holds it */
    int32_t cv_mv;                               /* the CV now, mV */
    uint32_t note_ms;                            /* when its last note began (the lights, the lanes) */
} ae_track_t;

static struct {
    ae_track_t t[AE_N];
    uint8_t playing;
    uint32_t last;                               /* the clock's position at the last block */
    uint8_t midi[AE_MIDI_RING][3];
    uint32_t midi_w, midi_r;
} and_ext;

/* ---------------------------------------------------------- settings --- */
static const param_desc_t *ae_desc(uint32_t x, uint32_t tr)
{
    const ae_track_t *a = &and_ext.t[tr % AE_N];
    if (x >= AE_P1 && x <= AE_P3)
        return a->gen ? &AE_GP[a->gen][x - AE_P1] : &AE_NONE_DESC;
    return x < AE_NPRM ? &AE_DESC[x] : &AE_NONE_DESC;
}
static int32_t ae_get(uint32_t x, uint32_t tr)
{
    const ae_track_t *a = &and_ext.t[tr % AE_N];
    switch (x) {
    case AE_GEN: return a->gen;
    case AE_P1: case AE_P2: case AE_P3: return a->gp[a->gen][x - AE_P1];
    case AE_SEED: return a->seed;
    case AE_DIV: return a->div;
    case AE_OCT: return a->oct;
    case AE_GATE: return a->gate;
    case AE_VEL: return a->vel;
    case AE_CHAN: return a->chan;
    case AE_DEPTH: return a->depth;
    case AE_MUTE: return a->mute;
    }
    return 0;
}
static void ae_note_off(ae_track_t *a);
static void ae_set(uint32_t x, uint32_t tr, int32_t v)
{
    ae_track_t *a = &and_ext.t[tr % AE_N];
    const param_desc_t *d = ae_desc(x, tr);
    v = v < d->min ? d->min : v > d->max ? d->max : v;
    switch (x) {
    case AE_GEN: a->gen = (uint8_t)v; break;
    case AE_P1: case AE_P2: case AE_P3: a->gp[a->gen][x - AE_P1] = (uint8_t)v; break;
    case AE_SEED: a->seed = (uint8_t)v; break;
    case AE_DIV: a->div = (uint8_t)v; break;
    case AE_OCT: a->oct = (int8_t)v; break;
    case AE_GATE: a->gate = (uint8_t)v; break;
    case AE_VEL: a->vel = (uint8_t)v; break;
    case AE_CHAN:                                /* (the note sounding ends on the channel it began on) */
        ae_note_off(a);
        a->chan = (uint8_t)v;
        break;
    case AE_DEPTH: a->depth = (uint8_t)v; break;
    case AE_MUTE:
        a->mute = (uint8_t)v;
        if (v)
            ae_note_off(a);
        break;
    }
}

/* the defaults: TURING on 5, EUCLID on 6, MARKOV on 7, AWAKE on 8; MIDI channels 5..8 */
static void and_ext_init(void)
{
    uint32_t tr, g, k;
    static const uint8_t GEN[AE_N] = {AE_TURING, AE_EUCLID, AE_MARKOV, AE_AWAKE};
    memset(&and_ext, 0, sizeof and_ext);
    for (tr = 0; tr < AE_N; tr++) {
        ae_track_t *a = &and_ext.t[tr];
        for (k = 0; k < AE_NPRM; k++)
            if (k < AE_P1 || k > AE_P3)
                ae_set(k, tr, AE_DESC[k].def);
        for (g = 1; g < AE_GENS; g++)
            for (k = 0; k < 3u; k++)
                a->gp[g][k] = (uint8_t)AE_GP[g][k].def;
        a->gen = GEN[tr];
        a->seed = (uint8_t)(tr * 37u + 11u);
        a->chan = (uint8_t)(5u + tr);
        a->div = tr == 3u ? 1u : 3u;             /* (AWAKE in eighths, the others in sixteenths) */
    }
}

/* -------------------------------------------------------------- MIDI --- */
static void ae_midi(uint32_t s, uint32_t d1, uint32_t d2)
{
    uint8_t *m;
    if (and_ext.midi_w - and_ext.midi_r >= AE_MIDI_RING)
        and_ext.midi_r++;                        /* (full: the oldest goes; the page reads every 2.7 ms) */
    m = and_ext.midi[and_ext.midi_w++ % AE_MIDI_RING];
    m[0] = (uint8_t)s;
    m[1] = (uint8_t)(d1 & 127u);
    m[2] = (uint8_t)(d2 & 127u);
}
/* up to max messages, 3 bytes each, into out; returns how many */
static uint32_t and_ext_midi_take(uint8_t *out, uint32_t max)
{
    uint32_t n = 0;
    while (n < max && and_ext.midi_r != and_ext.midi_w) {
        memcpy(out + 3u * n, and_ext.midi[and_ext.midi_r++ % AE_MIDI_RING], 3);
        n++;
    }
    return n;
}

/* -------------------------------------------------------------- notes --- */
static void ae_note_off(ae_track_t *a)
{
    if (!a->sounding)
        return;
    if (a->sent_ch)
        ae_midi(0x80u | (a->sent_ch - 1u), a->note, 0);
    a->sounding = 0;
    a->key = 0;
    a->off_at = 0;
}
static void ae_note_on(ae_track_t *a, uint32_t note, uint32_t vel)
{
    int32_t mv = ((int32_t)note - 36) * 1000 / 12;
    ae_note_off(a);
    a->note = (uint8_t)note;
    a->sent_ch = a->chan;
    if (a->chan)
        ae_midi(0x90u | (a->chan - 1u), note, vel ? vel : 1u);
    a->cv_mv = mv < 0 ? 0 : mv > AE_CV_MAX ? AE_CV_MAX : mv;
    a->sounding = 1;
    a->note_ms = fm1_ms | 1u;
}
/* the gate and the CV of track tr (0..3, Andamento's 5..8): the jack strip's pair tr */
static int and_ext_gate(uint32_t tr) { return and_ext.t[tr % AE_N].sounding; }
static int32_t and_ext_cv(uint32_t tr) { return and_ext.t[tr % AE_N].cv_mv; }

/* a key (and_keys.c) plays track tr's line: on, then off when it is let go */
static void and_ext_key(uint32_t tr, uint32_t note, uint32_t vel)
{
    ae_track_t *a = &and_ext.t[tr % AE_N];
    if (vel) {
        ae_note_on(a, note, vel);
        a->key = 1;
    } else if (a->sounding && a->key && a->note == note) {
        ae_note_off(a);
    }
}

/* the key's raw note into the song's scale: ROOT (MARKOV adds its own), the snap to track 1's scale, OCT */
static uint32_t ae_pitch(const ae_track_t *a, uint32_t raw)
{
    const track_t *t = &trk[0];
    int32_t n = (int32_t)raw + (a->gen == AE_MARKOV ? 0 : (int32_t)(t->p[P_ROOT] % 12));
    n = tessera_snap_note(t, n) + 12 * a->oct;
    return n < 0 ? 0u : n > 127 ? 127u : (uint32_t)n;
}

/* ENERGY on a step (energy.c energy_hit, with the track's own stream and DEPTH): 1 plays, *vel moved */
static int ae_energy(ae_track_t *a, uint32_t *vel)
{
    int32_t e;
    uint32_t s;
    if (!a->depth)
        return 1;
    if (energy_resting())
        return 0;
    e = energy_now() * (int32_t)a->depth / 100;
    if (e > 0) {
        *vel += (uint32_t)(e * 12 / 32767);
        if (*vel > 127u)
            *vel = 127u;
        return 1;
    }
    if (!(s = (uint32_t)-e))
        return 1;
    if (gen_rnd(&a->en_rnd, 32768u) < s * 7u / 10u)
        return 0;
    s = s * 16u / 32767u;
    *vel = *vel > s + 1u ? *vel - s : 1u;
    return 1;
}

/* PLAY: each generator from its first step and its SEED (as tessera.c's tess_seed_apply does a track's) */
static void ae_start(ae_track_t *a, uint32_t tr)
{
    uint32_t sd = a->seed;
    a->rnd = gen_stream(sd * 131u + tr);
    a->en_rnd = gen_stream(sd * 7u + 0x454Eu + tr);
    a->step = 0;
    memset(&a->turing, 0, sizeof a->turing);
    a->turing.reg = gen_turing_pick(sd + 1u);
    memset(&a->euclid, 0, sizeof a->euclid);
    a->euclid.pitch_seed = (uint8_t)sd;
    memset(&a->markov, 0, sizeof a->markov);
    memset(&a->awake, 0, sizeof a->awake);
    gen_awake_fill(&a->awake, sd);
    memset(&a->poly, 0, sizeof a->poly);
    a->poly.span = a->gp[AE_POLY][2];
    gen_poly_fill(&a->poly, sd);
    a->poly.pos = 3;
    memset(&a->series, 0, sizeof a->series);
    gen_series_fill(&a->series, sd);
}

/* one step of track tr's generator: its settings in, a note out (or none) */
static void ae_step(ae_track_t *a, uint32_t tr, uint32_t at, uint32_t len)
{
    gen_out_t o = {0, 60, 100, 0, 0, 0, {0, 0}, 0, 0};
    gen_clock_t clk = {(uint8_t)(a->step & 15u), (uint8_t)(4u * AE_DIV_N[a->div % 6u]), (uint8_t)(trk[0].p[P_ROOT] % 12)};
    const uint8_t *p = a->gp[a->gen];
    uint32_t vel;
    (void)tr;
    switch (a->gen) {
    case AE_TURING:
        a->turing.len = p[0];
        a->turing.prob = p[1];
        a->turing.oct_span = p[2];
        gen_turing_tick(&a->turing, &clk, &a->rnd, &o);
        break;
    case AE_EUCLID:
        a->euclid.steps = p[0];
        a->euclid.pulses = p[1] > p[0] ? p[0] : p[1];
        a->euclid.rot = p[2];
        if (a->euclid.step_idx >= p[0])
            a->euclid.step_idx = 0;
        gen_euclid_tick(&a->euclid, &clk, &a->rnd, &o);
        break;
    case AE_MARKOV:
        a->markov.tension = p[0];
        a->markov.density = p[1];
        a->markov.oct_span = p[2];
        gen_markov_tick(&a->markov, &clk, &a->rnd, &o);
        break;
    case AE_AWAKE:
        a->awake.len_a = p[0];
        a->awake.len_b = p[1];
        a->awake.prob = p[2];
        gen_awake_tick(&a->awake, &clk, &a->rnd, &o);
        break;
    case AE_POLY:
        a->poly.div[0] = p[0];
        a->poly.div[1] = p[1];
        a->poly.span = p[2];
        gen_poly_tick(&a->poly, &clk, &a->rnd, &o);
        break;
    case AE_SERIES:
        a->series.start = p[0];
        a->series.intv = p[1];
        a->series.loop = p[2];
        gen_series_tick(&a->series, &o);
        break;
    default:
        break;
    }
    a->step++;
    if (!o.gate || a->mute || a->key)            /* (a key held: the player has the line) */
        return;
    vel = o.vel;
    if (!ae_energy(a, &vel))
        return;
    vel = vel * a->vel / 100u;
    ae_note_on(a, ae_pitch(a, o.note), vel);
    a->off_at = at + len * a->gate / 100u;
    if (!a->off_at)
        a->off_at = 1;
}

/* the sequencer's position since PLAY, in its units (MIDI_BEAT_UNITS a beat: 24,576) */
static uint32_t ae_pos(void) { return (bar_n * 4u + beat_n) * MIDI_BEAT_UNITS + beat_pos; }

/* every audio block (after mix_block): the steps due, the notes ending */
static void and_ext_block(void)
{
    uint32_t tr, now;
    if (!song.playing) {
        if (and_ext.playing)                     /* STOP: every note off */
            for (tr = 0; tr < AE_N; tr++)
                if (!and_ext.t[tr].key)
                    ae_note_off(&and_ext.t[tr]);
        and_ext.playing = 0;
        return;
    }
    now = ae_pos();
    if (!and_ext.playing || now < and_ext.last) {   /* PLAY (or PLAY again while playing: from the top) */
        for (tr = 0; tr < AE_N; tr++) {
            if (!and_ext.t[tr].key)
                ae_note_off(&and_ext.t[tr]);
            ae_start(&and_ext.t[tr], tr);
        }
        and_ext.playing = 1;
    }
    and_ext.last = now;
    for (tr = 0; tr < AE_N; tr++) {
        ae_track_t *a = &and_ext.t[tr];
        uint32_t len = MIDI_BEAT_UNITS / AE_DIV_N[a->div % 6u], at;
        if (a->sounding && !a->key && a->off_at && now >= a->off_at)
            ae_note_off(a);
        if (a->step < now / len)                 /* (a step missed, a jump: from where the clock is) */
            a->step = now / len;
        while ((at = a->step * len) <= now) {
            if (a->sounding && !a->key)          /* (a GATE of 100: the line tied into the next note) */
                ae_note_off(a);
            if (!a->gen) {
                a->step++;
                continue;
            }
            ae_step(a, tr, at, len);
        }
    }
}
