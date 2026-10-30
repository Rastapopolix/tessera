/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* STEP as seed (2.1; tessera-notes docs/design/STEP_SEED.md): a TURING, EUCLID, MARKOV or MORPH track plays from a STEP
 * pattern, SOURCE's (OFF, T1..T4: that track's pattern playing now, this track's own too), which it never changes.
 * TURING loads the source's first LEN steps into a loop and drifts it: each step, at PROB %, the step about to play
 * changes and stays so (a Turing machine's flipped bit). KEEP says what may change: FREE the hit or the pitch, RHYTHM
 * only the pitch (another note of the loop's, or a degree away), PITCH only the hit (a rest taking the last note
 * before it). PROB 0 plays the source as written. EUCLID plays the source's notes in order on its hits (FREE, PITCH);
 * KEEP RHYTHM plays the source's own rhythm on Euclid's pitches. MARKOV learns from the source what follows what: each
 * step plays what followed the step it played last somewhere in the source (LEAP %: any of its steps); KEEP RHYTHM
 * the source's rhythm, the chain over its notes alone; KEEP PITCH the chain's rhythm, the notes in written order.
 * MORPH plays A, the source as written (its first 16 steps), against B, a variation of it made at PLAY (about half
 * its steps changed as TURING's are, KEEP's way; EVOLVE changes a few more); MORPH's position picks B's share.
 * GRAVITY pulls each back: at GRAVITY % a step plays the source as written at its place (TURING's changed step goes
 * back to it; MARKOV's chain goes on from there). The notes keep their written pitch, then this track's scale, TRN
 * and SHIFT. A source with no notes: as SOURCE OFF. Reloaded at PLAY and when SOURCE or LEN turn. Draws from the
 * track's stream, so it replays with its SEED. Included by tessera.c */
#define SD_LOOP 16u                                     /* TURING's LEN at most */
#define SD_MORPH 0xFFu                                  /* tess_sd.len: the loop is MORPH's B */
static struct {
    uint8_t loop[TESSERA_TRACKS][SD_LOOP];              /* TURING's loop: bit 7 a hit, bits 0..6 its note */
    uint8_t pos[TESSERA_TRACKS];                        /* the next step of the loop, or of the source */
    uint16_t n[TESSERA_TRACKS];                         /* EUCLID: the hits played since the phrase began; MARKOV: its
                                                         * chain's step (KEEP PITCH: n's low byte, the notes in order
                                                         * the high one) */
    uint8_t load[TESSERA_TRACKS];                       /* 1: TURING's loop to fill from the source first */
    uint8_t len[TESSERA_TRACKS];                        /* the LEN it was filled for (another LEN: filled again), or
                                                         * SD_MORPH */
} tess_sd __attribute__((section(".pool")));

/* step i of track s's pattern playing: 1 a hit (*note its note, a drum hit's lane note; *vel its velocity) */
static int sd_step(uint32_t s, uint32_t i, uint32_t *note, uint32_t *vel)
{
    const track_t *t = &trk[s];
    const step_t *st = &t->step[i % (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP)];
    if (st->time != ST_NOTE || (!st->n && !st->hit)) return 0;
    *note = st->n ? st->note[0] : DRUM_LANE_NOTE[__builtin_ctz(st->hit) % NLANE];
    *vel = (st->flags & SF_ACCENT) ? 127u : st->vel ? st->vel : 100u;
    return 1;
}
static uint32_t sd_len(uint32_t s) { return (uint32_t)clamp(trk[s].p[P_SLEN], 1, NSTEP); }
/* tr's generator can play from a SOURCE */
static int sd_mode(uint32_t tr)
{
    uint32_t m = tessera.mode[tr];
    return m == TESS_ENG_TURING || m == TESS_ENG_EUCLID || m == TESS_ENG_MARKOV || m == TESS_ENG_MORPH;
}
/* the source track of tr, + 1; 0: none (SOURCE OFF, not a generator with a SOURCE, or a source without notes) */
static uint32_t sd_source(uint32_t tr)
{
    uint32_t s = tessera.sd_src[tr], i, note, vel;
    if (!s || s > TESSERA_TRACKS || !sd_mode(tr)) return 0;
    for (i = 0; i < sd_len(s - 1u); i++)
        if (sd_step(s - 1u, i, &note, &vel)) return s;
    return 0;
}
void tessera_sd_play(void)                              /* seq.c, PLAY: the loops filled again, from step 1 */
{
    for (uint32_t tr = 0; tr < TESSERA_TRACKS; tr++) {
        tess_sd.load[tr] = 1;
        tess_sd.pos[tr] = tess_sd.n[tr] = 0;
    }
}
static void sd_realign(uint32_t tr)                     /* a phrase boundary (tess_realign): from step 1 again */
{
    tess_sd.pos[tr] = tess_sd.n[tr] = 0;
}
static void sd_fill(uint32_t tr, uint32_t s)
{
    uint32_t i, note, vel, last = 0;
    for (i = 0; i < sd_len(s); i++)                     /* (a rest before the first hit: the pattern's last note) */
        if (sd_step(s, i, &note, &vel)) last = note;
    for (i = 0; i < SD_LOOP; i++) {
        int hit = sd_step(s, i, &note, &vel);
        if (hit) last = note;
        tess_sd.loop[tr][i] = (uint8_t)((hit ? 0x80u : 0u) | (last & 0x7Fu));
    }
    tess_sd.load[tr] = 0;
    tess_sd.len[tr] = tessera.turing[tr].len;
}
/* TURING's change to step p of its loop: KEEP RHYTHM a new pitch, KEEP PITCH the hit flipped, FREE either */
static void sd_drift(const track_t *t, uint32_t tr, uint32_t p, uint32_t len)
{
    uint8_t *e = &tess_sd.loop[tr][p];
    uint32_t keep = tessera.sd_keep[tr], pitch = keep == TESS_SD_RHYTHM ? 1u : keep == TESS_SD_PITCH ? 0u :
                    gen_rnd(&tessera.rnd[tr], 2u);
    if (!pitch) {
        *e ^= 0x80u;
        return;
    }
    if (!(*e & 0x80u)) return;                          /* (a rest has no pitch to change) */
    {
        uint32_t i = gen_rnd(&tessera.rnd[tr], len), k, note = *e & 0x7Fu;
        for (k = 0; k < len; k++, i = (i + 1u) % len)   /* another note of the loop's .. */
            if ((tess_sd.loop[tr][i] & 0x80u) && (tess_sd.loop[tr][i] & 0x7Fu) != note) break;
        if (k < len) note = tess_sd.loop[tr][i] & 0x7Fu;
        else note = (uint32_t)clamp(tess_scale_steps(t, (int32_t)note, gen_rnd(&tessera.rnd[tr], 2u) ? 1 : -1), 0, 127);
        *e = (uint8_t)(0x80u | note);                   /* .. or, the loop all one note, a degree away */
    }
}
/* GRAVITY's pull this step: 1 = it plays the source as written (no draw at 0) */
static int sd_pull(uint32_t tr)
{
    return tessera.sd_grav[tr] && gen_rnd(&tessera.rnd[tr], 100u) < tessera.sd_grav[tr];
}
/* a generator's raw note playing the source's note n (tessera_clock_step adds the key back, then snaps) */
static uint8_t sd_raw(const track_t *t, uint32_t tr, uint32_t n)
{
    return (uint8_t)clamp((int32_t)n - tess_key(t, tr), 0, 127);
}
/* TURING with a SOURCE: this step from its loop (instead of gen_turing_tick); 0 = no source */
static int sd_turing(const track_t *t, uint32_t tr, gen_out_t *out)
{
    uint32_t s = sd_source(tr), len = tessera.turing[tr].len, p, note, vel;
    if (!s) return 0;
    len = len < 2u ? 2u : len > SD_LOOP ? SD_LOOP : len;
    if (tess_sd.load[tr] || tess_sd.len[tr] != tessera.turing[tr].len) sd_fill(tr, s - 1u);
    p = tess_sd.pos[tr] % len;
    tess_sd.pos[tr] = (uint8_t)((p + 1u) % len);
    if (sd_pull(tr)) {                                  /* GRAVITY: the step as written again (a rest keeps its pitch) */
        uint8_t *e = &tess_sd.loop[tr][p];
        *e = sd_step(s - 1u, p, &note, &vel) ? (uint8_t)(0x80u | (note & 0x7Fu)) : (uint8_t)(*e & 0x7Fu);
    } else if (tessera.turing[tr].prob && gen_rnd(&tessera.rnd[tr], 100u) < tessera.turing[tr].prob) {
        sd_drift(t, tr, p, len);
    }
    note = tess_sd.loop[tr][p] & 0x7Fu;
    out->cv = (int16_t)(note * 32767u / 127u);
    if (!(tess_sd.loop[tr][p] & 0x80u)) return 1;
    out->gate = 1;
    out->note = sd_raw(t, tr, note);
    out->vel = (uint8_t)(sd_step(s - 1u, p, &note, &vel) ? vel : 100u);
    return 1;
}
/* EUCLID with a SOURCE, after gen_euclid_tick: its hit plays the source's next note (FREE, PITCH); RHYTHM: the source's
 * hit plays Euclid's pitch for the step; GRAVITY: the source's step at its place, as written */
static void sd_euclid(const track_t *t, uint32_t tr, gen_out_t *out)
{
    uint32_t s = sd_source(tr), note, vel, i, k, len, p;
    if (!s) return;
    s--;
    len = sd_len(s);
    p = tess_sd.pos[tr] % len;
    tess_sd.pos[tr] = (uint8_t)((p + 1u) % len);
    if (sd_pull(tr)) {
        out->gate = (uint8_t)sd_step(s, p, &note, &vel);
        if (!out->gate) return;
        out->note = sd_raw(t, tr, note);
        out->vel = (uint8_t)vel;
        return;
    }
    if (tessera.sd_keep[tr] == TESS_SD_RHYTHM) {
        out->gate = (uint8_t)sd_step(s, p, &note, &vel);
        if (!out->gate) return;
        out->note = (uint8_t)(48u + gen_euclid_degree(tessera.euclid[tr].pitch_seed, p));
        out->vel = (uint8_t)vel;
        return;
    }
    if (!out->gate) return;
    for (i = k = 0; i < len; i++)                       /* (the source's notes, counted) */
        k += (uint32_t)sd_step(s, i, &note, &vel);
    k = tess_sd.n[tr]++ % k;
    for (i = 0; i < len; i++)
        if (sd_step(s, i, &note, &vel) && !k--) break;
    out->note = sd_raw(t, tr, note);
    out->vel = (uint8_t)vel;
}

/* MARKOV's chain: after place c of len places (place j holds ev[map[j]], no map: ev[j]; 0 a rest, 0x80 | note a hit),
 * the place it goes to: what followed place c's at one of the places holding the same (at random among them), or at
 * LEAP % any place */
static uint32_t sd_chain(const uint8_t *ev, const uint8_t *map, uint32_t len, uint32_t c, uint32_t tr)
{
    uint32_t i, k = 0;
    uint8_t e = ev[map ? map[c] : c];
    if (gen_rnd(&tessera.rnd[tr], 100u) < tessera.markov[tr].tension) return gen_rnd(&tessera.rnd[tr], len);
    for (i = 0; i < len; i++) k += ev[map ? map[i] : i] == e;
    k = gen_rnd(&tessera.rnd[tr], k);
    for (i = 0; i < len; i++)
        if (ev[map ? map[i] : i] == e && !k--) break;
    return (i + 1u) % len;
}
/* MARKOV with a SOURCE: this step from its chain (instead of gen_markov_tick); 0 = no source */
static int sd_markov(const track_t *t, uint32_t tr, gen_out_t *out)
{
    uint32_t s = sd_source(tr), len, p, i, m = 0, c, note, vel, keep = tessera.sd_keep[tr];
    uint8_t ev[NSTEP], at[NSTEP];                       /* (the source's places; at: the notes' places, KEEP RHYTHM) */
    if (!s) return 0;
    s--;
    len = sd_len(s);
    for (i = 0; i < len; i++) {
        ev[i] = sd_step(s, i, &note, &vel) ? (uint8_t)(0x80u | (note & 0x7Fu)) : 0u;
        if (ev[i]) at[m++] = (uint8_t)i;
    }
    p = tess_sd.pos[tr] % len;
    tess_sd.pos[tr] = (uint8_t)((p + 1u) % len);
    if (keep == TESS_SD_RHYTHM) {                       /* the source's hit at p, the chain over its notes alone */
        if (!ev[p]) return 1;
        c = tess_sd.n[tr] % m;
        if (sd_pull(tr))
            for (c = 0; c < m && at[c] != p; c++) {}
        i = at[c];
        tess_sd.n[tr] = (uint16_t)sd_chain(ev, at, m, c, tr);
    } else {
        c = (tess_sd.n[tr] & 0xFFu) % len;
        if (sd_pull(tr)) c = p;
        i = c;
        tess_sd.n[tr] = (uint16_t)((tess_sd.n[tr] & 0xFF00u) | sd_chain(ev, 0, len, c, tr));
        if (!ev[i]) return 1;
        if (keep == TESS_SD_PITCH) {                    /* (the chain's rhythm, the notes in written order) */
            uint32_t k = (tess_sd.n[tr] >> 8) % m;
            tess_sd.n[tr] = (uint16_t)((tess_sd.n[tr] & 0xFFu) | (((k + 1u) % m) << 8));
            i = at[k];
        }
    }
    sd_step(s, i, &note, &vel);
    out->cv = (int16_t)(note * 32767u / 127u);
    out->gate = 1;
    out->note = sd_raw(t, tr, note);
    out->vel = (uint8_t)vel;
    return 1;
}

/* MORPH's B: the source's first 16 steps, about half of them changed as TURING's are (n of them for EVOLVE) */
static void sd_morph_vary(const track_t *t, uint32_t tr, uint32_t n)
{
    uint32_t s = sd_source(tr), len, i;
    if (!s) return;
    len = sd_len(s - 1u);
    len = len > SD_LOOP ? SD_LOOP : len;
    if (!n) {
        sd_fill(tr, s - 1u);
        tess_sd.len[tr] = SD_MORPH;
        for (i = 0; i < len; i++)
            if (gen_rnd(&tessera.rnd[tr], 2u)) sd_drift(t, tr, i, len);
        return;
    }
    while (n--) sd_drift(t, tr, gen_rnd(&tessera.rnd[tr], len), len);
}
static void sd_evolve(uint32_t tr, uint32_t amt)        /* tessera_mutate, MORPH: 1..3 more of B's steps changed */
{
    if (sd_source(tr) && !tess_sd.load[tr] && tess_sd.len[tr] == SD_MORPH) sd_morph_vary(&trk[tr], tr, 1u + amt / 34u);
}
/* MORPH with a SOURCE: this step of A (the source) or of B (instead of gen_morph_tick); 0 = no source */
static int sd_morph(const track_t *t, uint32_t tr, gen_out_t *out)
{
    gen_morph_t *g = &tessera.morph[tr];
    uint32_t s = sd_source(tr), len, p, note, vel, e;
    if (!s) return 0;
    if (tess_sd.load[tr] || tess_sd.len[tr] != SD_MORPH) sd_morph_vary(t, tr, 0);   /* (or TURING's loop) */
    len = sd_len(s - 1u);
    len = len > SD_LOOP ? SD_LOOP : len;
    p = tess_sd.pos[tr] % len;
    tess_sd.pos[tr] = (uint8_t)((p + 1u) % len);
    g->step_idx = (uint8_t)((g->step_idx & 0xF0u) | p);   /* (its picture's playhead) */
    out->cv = (int16_t)(p * 32767u / 15u);
    if (!sd_pull(tr) && gen_rnd(&tessera.rnd[tr], 128u) < g->morph_pos) {
        e = tess_sd.loop[tr][p];                        /* (B) */
        if (!(e & 0x80u)) return 1;
        note = e & 0x7Fu;
        if (!sd_step(s - 1u, p, &e, &vel)) vel = 100u;
    } else if (!sd_step(s - 1u, p, &note, &vel)) {      /* (A) */
        return 1;
    }
    out->gate = 1;
    out->note = sd_raw(t, tr, note);
    out->vel = (uint8_t)vel;
    return 1;
}

/* TURING or MORPH playing from a SOURCE, for its pictures (ui_graph.c): the loop's steps (0: not from one), into now[]
 * as they play now (MORPH: B) and home[] as written in the source (bit 7 a hit, bits 0..6 its note; a rest 0); *at
 * the step playing (the last one, stopped) */
uint32_t tessera_sd_loop(uint32_t tr, uint8_t *now, uint8_t *home, uint32_t *at)
{
    uint32_t s = tr < TESSERA_TRACKS ? sd_source(tr) : 0u, m, len, i, note, vel, live;
    if (!s) return 0;
    m = tessera.mode[tr];
    if (m == TESS_ENG_TURING) {
        len = tessera.turing[tr].len;
        len = len < 2u ? 2u : len > SD_LOOP ? SD_LOOP : len;
        live = tess_sd.len[tr] == tessera.turing[tr].len;
    } else if (m == TESS_ENG_MORPH) {
        len = sd_len(s - 1u);
        len = len > SD_LOOP ? SD_LOOP : len;
        live = tess_sd.len[tr] == SD_MORPH;
    } else {
        return 0;
    }
    live = live && !tess_sd.load[tr];                   /* (not filled yet: as written) */
    for (i = 0; i < len; i++) {
        home[i] = (uint8_t)(sd_step(s - 1u, i, &note, &vel) ? 0x80u | (note & 0x7Fu) : 0u);
        now[i] = live ? (tess_sd.loop[tr][i] & 0x80u ? tess_sd.loop[tr][i] : 0u) : home[i];
    }
    *at = (tess_sd.pos[tr] + len - 1u) % len;
    return len;
}

/* OCT+ held on GEN, TURING, EUCLID, MARKOV or MORPH: KNOB 1 SOURCE, KNOB 2 KEEP, KNOB 3 GRAVITY (ui_input.c) */
void tessera_sd_layer_knob(uint32_t k, int32_t s)
{
    uint32_t tr = song.sel;
    int32_t d = s > 0 ? 1 : -1;
    tessera_ensure_init();
    if (tr >= TESSERA_TRACKS || !s) return;
    TESS_IRQ_OFF();
    if (k == 0u) tess_param_set_body(tr, TX_SD_SOURCE, (int32_t)tessera.sd_src[tr] + d);
    else if (k == 1u) tess_param_set_body(tr, TX_SD_KEEP, (int32_t)tessera.sd_keep[tr] + d);
    else if (k == 2u) tess_param_set_body(tr, TX_SD_GRAVITY, (int32_t)tessera.sd_grav[tr] + s);
    TESS_IRQ_ON();
}
