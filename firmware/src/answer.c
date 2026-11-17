/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* ANSWER (after Zen-1's koi and petals, aceyprime/zen-1: the idea only, no code): the notes played on the track's keys
 * (or MIDI IN), up to 16, are its phrase, each with its velocity and its step; a bar with no key closes it. After a gap
 * (SPACE) it answers: the phrase's rhythm with its notes in ORDER (as played, reversed, shuffled), the answer moved up
 * to SHIFT scale degrees, and changes that come and go (vary.c: a note moved, an octave, dropped, swapped) at VARY.
 * Each answer is softer and more changed, until FADE's count is played and it forgets: it rests until the keys play
 * again. At the top of FADE it never forgets: its notes run as four loops of unequal lengths (LEN; Eno's tape loops),
 * so it never quite repeats. ECHO: a note now and then comes back once, softer, a few steps later, sometimes an octave
 * up. The phrase is saved (a project answers at PLAY); timing snaps to steps (SUB later). SOURCE (T1..T4, STEP as
 * seed's: tessera.sd_src) makes a STEP pattern the phrase instead, read again before each answer; a generator track's
 * SOURCE (BOUNCE, TURING ...) is heard instead: the notes it plays are the call, as the keys' are.
 * MODE BEND (after Qu-Bit's Data Bender: the idea only) does not wait for the phrase to end: each note comes back TIME
 * steps later while the keys still play, a delay that drifts (VARY) and breaks (BREAK): answer_bend_*. Included by
 * tessera.c */

static const uint8_t AN_LOOPS[TESS_AN_LENS][4] = {{5, 7, 11, 13}, {13, 17, 23, 29}, {23, 29, 31, 37}};

/* the answers FADE gives: 1 (0) .. 16 (99); 0 at 100: never forgets */
uint32_t tessera_answer_count(uint32_t tr)
{
    uint32_t f = tessera.answer[tr % TESSERA_TRACKS].fade;
    return f >= 100u ? 0u : 1u + f * 15u / 99u;
}

/* SOURCE another track with a generator: its notes heard as played (tessera_answer_heard), its STEP pattern not read
 * (Theo, 3.5-beta4: BOUNCE on T1 answered as T1's hidden pattern, one note) */
static int answer_gen_src(uint32_t tr)
{
    uint32_t s = tessera.sd_src[tr];
    return s && s <= TESSERA_TRACKS && s != tr + 1u && tessera.mode[s - 1u] != TESS_ENG_OFF;
}

/* SOURCE: the call is that track's STEP pattern playing now (this track's own too), its first 16 notes at their steps
 * (a chord its first note, a drum hit its lane's note); 0: none (OFF, or a pattern without notes: the keys' call) */
/* SOURCE the ANSWER track itself: its own STEP pattern is the call (shown OWN, not "T4", which read like a slip) */
static int answer_own(uint32_t tr)
{
    return tr < TESSERA_TRACKS && tessera.mode[tr] == TESS_ENG_ANSWER && tessera.sd_src[tr] == tr + 1u;
}
static int answer_source(uint32_t tr)
{
    uint32_t s = tessera.sd_src[tr], i, n = 0, note, vel, len;
    tessera_answer_t *a = &tessera.answer[tr];
    if (!s || s > TESSERA_TRACKS || tessera.mode[tr] != TESS_ENG_ANSWER) return 0;   /* (another mode's SOURCE: its own) */
    if (answer_gen_src(tr)) return 0;                  /* (a generator: heard, tessera_answer_heard) */
    len = sd_len(s - 1u);
    for (i = 0; i < len && i <= TESS_AN_AT_MAX && n < TESS_AN_NOTES; i++)
        if (sd_step(s - 1u, i, &note, &vel)) {
            a->note[n] = (uint8_t)note;
            a->vel[n] = (uint8_t)(vel > 127u ? 127u : vel);
            a->at[n++] = (uint8_t)i;
        }
    if (!n) return 0;
    a->n = (uint8_t)n;
    return 1;
}

/* the answers from the first again (PLAY, a mode change, a load, a new SOURCE): a phrase kept (or a SOURCE's) answers
 * after its gap, none rests */
static void answer_restart(uint32_t tr)
{
    answer_source(tr);
    if (answer_gen_src(tr)) tessera.answer[tr].n = 0;  /* (a generator's call: heard afresh) */
    tess_an.phase[tr] = tessera.answer[tr].n ? TESS_AN_GAP : TESS_AN_IDLE;
    tess_an.cnt[tr] = 0;
    tess_an.ans[tr] = 0;
    tess_an.slot[tr] = 0;
    tess_an.shift[tr] = 0;
    tess_an.rnd[tr] = gen_stream(tessera.seed[tr] * 31u + 7u);
    tess_an.hopen[tr] = 0;
    memset(tess_an.mut[tr], 0, TESS_AN_NOTES);
    memset(tess_an.e_left[tr], 0, TESS_AN_ECHOES);
    memset(tess_an.t_left[tr], 0, TESS_AN_TAPS);       /* (BEND: no repeats waiting, nothing remembered) */
    tess_an.mcnt[tr] = tess_an.mline[tr] = 0;
    tess_an.hclk[tr] = 0;
    for (uint32_t i = 0; i < TESS_AN_NOTES; i++) tess_an.ord[tr][i] = (uint8_t)i;
}

/* BEND: a repeat to wait left steps (1..255); full, the one repeated most makes room (at FADE's top, the oldest
 * material gives way to what the keys play now) */
static void answer_tap(uint32_t tr, uint32_t note, uint32_t vel, uint32_t left, uint32_t g, uint32_t ser)
{
    uint32_t k, use = 0, old = 0;
    for (k = 0; k < TESS_AN_TAPS; k++) {
        if (!tess_an.t_left[tr][k]) { use = k; break; }
        if (tess_an.t_gen[tr][k] >= old) old = tess_an.t_gen[tr][k], use = k;
    }
    tess_an.t_note[tr][use] = (uint8_t)note;
    tess_an.t_vel[tr][use] = (uint8_t)(vel ? vel : 1u);
    tess_an.t_left[tr][use] = (uint8_t)(left < 1u ? 1u : left > 255u ? 255u : left);
    tess_an.t_gen[tr][use] = (uint8_t)g;
    tess_an.t_ser[tr][use] = (uint8_t)ser;
}

/* BEND: the note heard back notes before the newest (0: the newest) + 1; 0 not remembered */
static uint32_t answer_mem(uint32_t tr, uint32_t back)
{
    if (back >= tess_an.mcnt[tr]) return 0;
    return tess_an.mem[tr][(uint8_t)(tess_an.mser[tr] - 1u - back) % TESS_AN_NOTES] + 1u;
}

/* BEND hears a note: remembered (a line: notes no more than 16 steps apart), and back TIME steps later (one past the
 * middle of its step: a step more, as REPLY counts it) */
static void answer_bend_hear(uint32_t tr, uint32_t note, uint32_t vel, uint32_t late)
{
    uint32_t s = tess_an.mser[tr];
    if (tess_an.hclk[tr] > TESS_AN_TIME_MAX) tess_an.mline[tr] = 0;
    tess_an.hclk[tr] = 0;
    tess_an.mem[tr][s % TESS_AN_NOTES] = (uint8_t)note;
    tess_an.mser[tr] = (uint8_t)(s + 1u);
    if (tess_an.mcnt[tr] < TESS_AN_NOTES) tess_an.mcnt[tr]++;
    if (tess_an.mline[tr] < TESS_AN_NOTES) tess_an.mline[tr]++;
    answer_tap(tr, note, vel, tessera.answer[tr].time + (late ? 1u : 0u), 0, s);
}

/* a note on the track's keys: the first after an answer (or a closed phrase) starts a new phrase; up to 16, a step
 * apart at least (keys while stopped: a step each). On the nearest step, as REC places a note: one played past the
 * middle of a step (late) counts on the next, so a note played a touch early is not a whole step early (Theo, 3.3.1:
 * the answers felt off the grid) */
void tessera_answer_hear(track_t *t, uint32_t note, uint32_t vel) { tessera_answer_hear_at(t, note, vel, 0); }
void tessera_answer_hear_at(track_t *t, uint32_t note, uint32_t vel, uint32_t late)
{
    uint32_t tr = trk_index(t), n, at;
    tessera_answer_t *a;
    if (tr >= TESSERA_TRACKS || !vel || note > 127u) return;
    if (tessera.mode[tr] != TESS_ENG_ANSWER) {         /* (a generator with SOURCE KEYS: its take, step_seed.c) */
        if (!sd_keys(tr) || !sd_mode(tr)) return;
    } else if (tessera.answer[tr].mode == TESS_AN_BEND) {   /* (BEND: every note, a STEP SOURCE or not) */
        answer_bend_hear(tr, note, vel > 127u ? 127u : vel, late);
        return;
    } else if (tessera.sd_src[tr] && answer_source(tr)) {
        return;                                         /* (a SOURCE's call: the keys just play) */
    }
    a = &tessera.answer[tr];
    if (tess_an.phase[tr] != TESS_AN_HEAR) {
        answer_restart(tr);
        a->n = 0;
        tess_an.phase[tr] = TESS_AN_HEAR;
        tess_an.hclk[tr] = 0;
        tess_an.hlate[tr] = late ? 1u : 0u;      /* (the phrase from its first note's step) */
    }
    n = a->n;
    if (n >= TESS_AN_NOTES) return;
    at = tess_an.hclk[tr] + (late ? 1u : 0u);
    at = at > tess_an.hlate[tr] ? at - tess_an.hlate[tr] : 0u;
    at = at > TESS_AN_AT_MAX ? TESS_AN_AT_MAX : at;
    if (n && (at < a->at[n - 1u] || (a->chord == TESS_AN_ARP && at == a->at[n - 1u])))
        at = a->at[n - 1u] + 1u;   /* ARP: a note at the same step as the last is pushed one on; TRUE: it shares it */
    if (at > TESS_AN_AT_MAX) return;
    a->note[n] = (uint8_t)note;
    a->vel[n] = (uint8_t)(vel > 127u ? 127u : vel);
    a->at[n] = (uint8_t)at;
    a->hold[n] = 0;                          /* (set on release, tessera_answer_release; 0: unheld, as before) */
    tess_an.hopen[tr] |= (uint16_t)(1u << n);
    a->n = (uint8_t)(n + 1u);
    tess_an.cnt[tr] = 0;
}

/* note's key (or MIDI note) is up: close the held place's hold, the step count it was held across (clamped
 * 1..15), so ANSWER can play it back that long (answer_tick, note_len.c's nl_on_for; LENGTH's own NL_MODE off
 * STEP still wins). Places not still open (never captured here, or already closed) are left alone */
static void answer_release(uint32_t tr, uint32_t note)
{
    tessera_answer_t *a = &tessera.answer[tr];
    uint32_t at;
    if (tr >= TESSERA_TRACKS || tessera.mode[tr] != TESS_ENG_ANSWER) return;
    for (uint32_t i = a->n; i-- > 0; )
        if ((tess_an.hopen[tr] & (1u << i)) && a->note[i] == note) {
            at = tess_an.hclk[tr] > tess_an.hlate[tr] ? tess_an.hclk[tr] - tess_an.hlate[tr] : 0u;
            at = at > a->at[i] ? at - a->at[i] : 0u;
            a->hold[i] = (uint8_t)(at < 1u ? 1u : at > 15u ? 15u : at);
            tess_an.hopen[tr] &= (uint16_t)~(1u << i);
            return;
        }
}
void tessera_answer_release(track_t *t, uint32_t note)
{
    uint32_t tr = trk_index(t);
    if (tr >= TESSERA_TRACKS) return;
    answer_release(tr, note);
    for (uint32_t f = 0; f < TESSERA_TRACKS; f++)
        if (f != tr && tessera.mode[f] == TESS_ENG_ANSWER && tessera.sd_src[f] == tr + 1u)
            answer_release(f, note);
}

/* track src (a generator) has just played note: each ANSWER with SOURCE src that listens or rests hears it, as a key
 * at velocity 100 (so a BOUNCE call: up to 16 notes, closed by a bar without one, or at the 16th's bar); one answering
 * (or LOOPS) goes on. Two ANSWERs each other's SOURCE: a conversation */
static void tessera_answer_heard(uint32_t src, uint32_t note)
{
    for (uint32_t f = 0; f < TESSERA_TRACKS; f++)
        if (tessera.mode[f] == TESS_ENG_ANSWER && tessera.sd_src[f] == src + 1u && f != src &&
            (tess_an.phase[f] == TESS_AN_HEAR || tess_an.phase[f] == TESS_AN_IDLE ||
             tessera.answer[f].mode == TESS_AN_BEND))
            tessera_answer_hear_at(&trk[f], note, 100u, 0);
}

/* a key (or MIDI IN) on track t: its own ANSWER's phrase, and the phrase of each ANSWER whose SOURCE is t, as on its
 * own keys (a reddit report, 3.5: T2 ANSWER with SOURCE T1 did not hear T1 played live). A STEP pattern on t still
 * wins (answer_source: the keys just play); not passed on (a ring of ANSWERs hears the keys once) */
void tessera_answer_key(track_t *t, uint32_t note, uint32_t vel, uint32_t late)
{
    uint32_t tr = trk_index(t);
    tessera_answer_hear_at(t, note, vel, late);
    if (tr >= TESSERA_TRACKS) return;
    for (uint32_t f = 0; f < TESSERA_TRACKS; f++)
        if (f != tr && tessera.mode[f] == TESS_ENG_ANSWER && tessera.sd_src[f] == tr + 1u)
            tessera_answer_hear_at(&trk[f], note, vel, late);
}

/* a new answer's (or loop round's) choices: the changes (vary.c, LOCK 50: half of them stay), the order, the SHIFT */
static void answer_round(uint32_t tr)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t n = a->n, cnt = tessera_answer_count(tr), amount = a->vary, i, *rnd = &tess_an.rnd[tr];
    uint8_t kind[TESS_AN_NOTES], *ord = tess_an.ord[tr];
    if (amount && cnt) amount += (100u - amount) * tess_an.ans[tr] / cnt;   /* (each answer more changed) */
    for (i = 0; i < n; i++) kind[i] = VK_NOTE;
    if (amount) vary_round(tess_an.mut[tr], kind, n, amount, 50u, rnd, 0);
    else memset(tess_an.mut[tr], 0, TESS_AN_NOTES);
    for (i = 0; i < n; i++) ord[i] = (uint8_t)(a->order == TESS_AN_REVERSE ? n - 1u - i : i);
    if (a->order == TESS_AN_SHUFFLE)
        for (i = n; i > 1u; i--) {
            uint32_t j = gen_rnd(rnd, i);
            uint8_t x = ord[i - 1u];
            ord[i - 1u] = ord[j], ord[j] = x;
        }
    tess_an.shift[tr] = (int8_t)(a->shift ? (int32_t)gen_rnd(rnd, 2u * a->shift + 1u) - (int32_t)a->shift : 0);
}

/* a sound into out: the first the step's note, two more besides it */
static void answer_emit(gen_out_t *out, uint32_t note, uint32_t vel)
{
    if (!out->gate) {
        out->note = (uint8_t)note, out->vel = (uint8_t)(vel ? vel : 1u), out->gate = 1;
    } else if (out->more_n < 2u && note != out->note) {
        out->more[out->more_n++] = (uint8_t)note;
    }
}

/* place i of the phrase as this answer plays it: its note (changed, shifted), velocity and hold (steps it was
 * held when heard, §2 docs/design/answer-note-length.md); 0 a rest */
static uint32_t answer_place(const track_t *t, uint32_t tr, uint32_t i, uint32_t *vel, uint32_t *hold)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t n = a->n, m = tess_an.mut[tr][i], src = vary_src(m), k = i, note;
    uint8_t kind[TESS_AN_NOTES];
    if (vary_dropped(m)) return 0;
    if (src != VARY_OWN) {                              /* (a SWAP's halves: the next or the last place's note) */
        for (uint32_t j = 0; j < n; j++) kind[j] = VK_NOTE;
        k = vary_find(kind, n, i, src == VARY_NEXT ? 1 : -1);
        if (k >= n) k = i;
    }
    note = sv_degrees(t, a->note[tess_an.ord[tr][k] % n], vary_move(m) + tess_an.shift[tr]);
    note = (uint32_t)((int32_t)note + 12 * vary_oct(m));
    *vel = a->vel[tess_an.ord[tr][k] % n];
    *hold = a->hold[tess_an.ord[tr][k] % n];
    return note > 127u ? 0u : note + 1u;
}

/* ECHO: the note back once, at half its velocity, 3 to 8 steps later; one in four an octave up */
static void answer_echo(uint32_t tr, uint32_t note, uint32_t vel)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t *rnd = &tess_an.rnd[tr];
    if (!a->echo || gen_rnd(rnd, 100u) >= a->echo) return;
    for (uint32_t e = 0; e < TESS_AN_ECHOES; e++) {
        if (tess_an.e_left[tr][e]) continue;
        if (!gen_rnd(rnd, 4u) && note + 12u <= 127u) note += 12u;
        tess_an.e_note[tr][e] = (uint8_t)note;
        tess_an.e_vel[tr][e] = (uint8_t)(vel / 2u ? vel / 2u : 1u);
        tess_an.e_left[tr][e] = (uint8_t)(3u + gen_rnd(rnd, 6u));
        return;
    }
}

/* place i plays now: its note into out (velocity scaled by sc / 256), maybe an echo. Its hold (held steps when
 * heard) rides with it only if this is the tick's main note, not a chord's "more": echoes and chords stay
 * out of scope for v1 (docs/design/answer-note-length.md §2-3) */
static void answer_play(const track_t *t, uint32_t tr, uint32_t i, uint32_t sc, gen_out_t *out)
{
    uint32_t vel = 0, hold = 0, n = answer_place(t, tr, i, &vel, &hold);
    uint32_t was_main = !out->gate;
    tess_an.slot[tr] = (uint8_t)(i + 1u);
    if (!n) return;
    vel = vel * sc / 256u;
    answer_emit(out, n - 1u, vel);
    if (was_main) out->hold = (uint8_t)hold;
    answer_echo(tr, n - 1u, vel);
}

/* BEND: repeat k plays now. Its first repeat in ORDER (the line reversed or shuffled); a repeat now and then moved by
 * the bar's SHIFT (TIME in spb of them: about once a bar, whatever the TIME), and changed at VARY (growing with each repeat, kept: the line drifts): a degree or two, an octave, silent
 * this once, its neighbour's note. BREAK, this once only: SLIP (the next repeat a step early or late), SKIP, STUTTER
 * (again a step later), JUMP (a note heard 2 to 5 notes before). Softer each repeat, FADE's count of them (at its top:
 * on until newer notes take the room). ECHO as REPLY's */
static void answer_bend_fire(const track_t *t, uint32_t tr, uint32_t k, uint32_t spb, gen_out_t *out)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t *rnd = &tess_an.rnd[tr], cnt = tessera_answer_count(tr), g = tess_an.t_gen[tr][k], amount = a->vary;
    uint32_t note = tess_an.t_note[tr][k], vel = tess_an.t_vel[tr][k], left = a->time, play, m, stutter = 0, sound = 1;
    uint32_t back = (uint8_t)(tess_an.mser[tr] - 1u - tess_an.t_ser[tr][k]), line = tess_an.mline[tr];
    int32_t d;
    if (!g && a->order != TESS_AN_ASPLAYED && back < line) {
        back = a->order == TESS_AN_REVERSE ? line - 1u - back : gen_rnd(rnd, line);
        note = answer_mem(tr, back) - 1u;
        tess_an.t_ser[tr][k] = (uint8_t)(tess_an.mser[tr] - 1u - back);
    }
    if (tess_an.shift[tr] && gen_rnd(rnd, spb) < a->time) note = sv_degrees(t, (int32_t)note, tess_an.shift[tr]);
    if (amount && cnt) amount += (100u - amount) * (g < cnt ? g : cnt) / cnt;
    if (amount && gen_rnd(rnd, 100u) < amount)
        switch (gen_rnd(rnd, 4u)) {
        case 0: d = 1 + (int32_t)gen_rnd(rnd, 2u); note = sv_degrees(t, (int32_t)note, gen_rnd(rnd, 2u) ? d : -d); break;
        case 1: note = gen_rnd(rnd, 2u) ? note + 12u : note - 12u; break;   /* (folded back below) */
        case 2: sound = 0; break;
        default:
            if ((m = answer_mem(tr, gen_rnd(rnd, 2u) ? back + 1u : back - 1u)) != 0) note = m - 1u;
            break;
        }
    while ((int32_t)note > 108) note -= 24u;            /* (drifted out of reach: two octaves back) */
    while ((int32_t)note < 24) note += 24u;
    play = note;
    if (a->brk && gen_rnd(rnd, 100u) < a->brk)
        switch (gen_rnd(rnd, 4u)) {
        case 0: left = gen_rnd(rnd, 2u) || left < 2u ? left + 1u : left - 1u; break;
        case 1: sound = 0; break;
        case 2: stutter = 1; break;
        default:
            if ((m = answer_mem(tr, back + 2u + gen_rnd(rnd, 4u))) != 0) play = m - 1u;
            break;
        }
    if (sound) {
        vel = vel * (cnt ? cnt - (g < cnt ? g : cnt - 1u) : 1u) / (cnt ? cnt : 1u);
        answer_emit(out, play, vel);
        answer_echo(tr, play, vel);
        for (uint32_t e = 0; stutter && e < TESS_AN_ECHOES; e++)   /* (STUTTER: an echo slot, a step on) */
            if (!tess_an.e_left[tr][e]) {
                tess_an.e_note[tr][e] = (uint8_t)play;
                tess_an.e_vel[tr][e] = (uint8_t)(vel * 3u / 4u ? vel * 3u / 4u : 1u);
                tess_an.e_left[tr][e] = 1;
                stutter = 0;
            }
    }
    if (++g > 254u) g = 254u;
    if (cnt && g >= cnt) return;                        /* (its last: the slot free, t_left 0) */
    tess_an.t_note[tr][k] = (uint8_t)note;
    tess_an.t_gen[tr][k] = (uint8_t)g;
    tess_an.t_left[tr][k] = (uint8_t)left;
}

/* BEND's step: a new bar's SHIFT (up or down, the same for all its repeats: the line climbs or sinks about that
 * many degrees a bar), the repeats one step nearer and those due played; the CV the share of the room in use */
static void answer_bend_tick(const track_t *t, uint32_t tr, uint32_t spb, gen_out_t *out)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t k, live = 0;
    if (tess_an.hclk[tr] < 0xFFFFu) tess_an.hclk[tr]++;
    if (!(tess_an.cnt[tr] % spb))
        tess_an.shift[tr] = (int8_t)(a->shift ? (int32_t)gen_rnd(&tess_an.rnd[tr], 2u * a->shift + 1u) - (int32_t)a->shift : 0);
    tess_an.cnt[tr] = tess_an.cnt[tr] + 1u < 0x7FFFFFFFu ? tess_an.cnt[tr] + 1u : 0u;
    for (k = 0; k < TESS_AN_TAPS; k++)
        if (tess_an.t_left[tr][k] && !--tess_an.t_left[tr][k]) answer_bend_fire(t, tr, k, spb, out);
    for (k = 0; k < TESS_AN_TAPS; k++) live += tess_an.t_left[tr][k] != 0u;
    out->cv = (int16_t)(live * 32767u / TESS_AN_TAPS);
}

static void answer_tick(const track_t *t, uint32_t tr, const gen_clock_t *clk, gen_out_t *out)
{
    const tessera_answer_t *a = &tessera.answer[tr];
    uint32_t spb = clk->steps_per_bar ? clk->steps_per_bar : 16u, n = a->n, cnt = tessera_answer_count(tr), i;
    uint32_t c = tess_an.cnt[tr];
    for (uint32_t e = 0; e < TESS_AN_ECHOES; e++)       /* the echoes waiting: one step nearer */
        if (tess_an.e_left[tr][e] && !--tess_an.e_left[tr][e])
            answer_emit(out, tess_an.e_note[tr][e], tess_an.e_vel[tr][e]);
    if (a->mode == TESS_AN_BEND) {
        answer_bend_tick(t, tr, spb, out);
        return;
    }
    out->cv = (int16_t)(n ? (uint32_t)tess_an.slot[tr] * 32767u / n : 0u);
    switch (tess_an.phase[tr]) {
    case TESS_AN_HEAR:                                  /* listening: a bar with no key closes the phrase */
        if (tess_an.hclk[tr] < 0xFFFFu) tess_an.hclk[tr]++;
        if (++tess_an.cnt[tr] >= spb) {
            tess_an.phase[tr] = n ? TESS_AN_GAP : TESS_AN_IDLE;
            tess_an.cnt[tr] = 0;
            tess_an.ans[tr] = 0;
        }
        return;
    case TESS_AN_GAP:                                   /* SPACE: 0 .. 4 bars before an answer */
        if (!n) { tess_an.phase[tr] = TESS_AN_IDLE; return; }
        if (c < (uint32_t)a->space * spb * 4u / 100u) {
            tess_an.cnt[tr] = c + 1u;
            return;
        }
        tess_an.phase[tr] = cnt ? TESS_AN_PLAY : TESS_AN_LOOP;
        tess_an.cnt[tr] = c = 0;
        answer_source(tr);                              /* (a SOURCE: the pattern as it is now) */
        n = a->n;
        answer_round(tr);
        if (!cnt) break;
        /* fall through */
    case TESS_AN_PLAY: {                                /* an answer: the phrase's rhythm, softer each time */
        uint32_t span = n ? (uint32_t)a->at[n - 1u] + 1u : 1u, sc = cnt ? 256u * (cnt - tess_an.ans[tr]) / cnt : 256u;
        if (!n) { tess_an.phase[tr] = TESS_AN_IDLE; return; }
        for (i = 0; i < n; i++)
            if (a->at[i] == c) answer_play(t, tr, i, sc, out);
        tess_an.cnt[tr] = ++c;
        if (c < span) return;
        tess_an.cnt[tr] = 0;
        if (!cnt) {                                     /* (FADE turned to the top: on as the loops) */
            tess_an.phase[tr] = TESS_AN_LOOP;
            return;
        }
        if (++tess_an.ans[tr] >= cnt) {                 /* forgotten: rest until the keys play again */
            tess_an.phase[tr] = TESS_AN_IDLE;
            tess_an.slot[tr] = 0;
            return;
        }
        tess_an.phase[tr] = TESS_AN_GAP;
        return; }
    case TESS_AN_IDLE:                                  /* forgotten: a SOURCE calls again (its track plays on) */
        if (answer_source(tr)) {
            tess_an.phase[tr] = TESS_AN_GAP;
            tess_an.cnt[tr] = tess_an.ans[tr] = 0;
        }
        return;
    default:
        break;
    }
    if (tess_an.phase[tr] != TESS_AN_LOOP) return;
    if (!n || cnt) {                                    /* (FADE turned down: answers again, after a gap) */
        tess_an.phase[tr] = n ? TESS_AN_GAP : TESS_AN_IDLE;
        tess_an.cnt[tr] = 0;
        return;
    }
    {   /* the four loops: place i on loop i % 4, sounding every LEN of it from its own step; a new round each turn of
         * the first loop */
        const uint8_t *L = AN_LOOPS[a->len % TESS_AN_LENS];
        if (c && !(c % L[0])) {
            answer_source(tr);
            n = a->n;
            answer_round(tr);
        }
        for (i = 0; i < n; i++)
            if (c >= a->at[i] && !((c - a->at[i]) % L[i & 3u])) answer_play(t, tr, i, 256u, out);
        tess_an.cnt[tr] = c + 1u < 0x7FFFFFFFu ? c + 1u : 0u;
    }
}

/* OCT- held on GEN in ANSWER (behind RANDOM, ui_input.c gen_dn_knob): KNOB 2 SOURCE, 3 ORDER, 4 SHIFT (SPACE and
 * CHORD: the GEN sheet, OCT+ held, ui_popup.c; LEN: FADE's top, tessera_knob) */
void tessera_answer_layer_knob(uint32_t k, int32_t s)
{
    uint32_t tr = song.sel;
    const tessera_answer_t *a;
    int32_t d = s > 0 ? 1 : -1;
    tessera_ensure_init();
    if (tr >= TESSERA_TRACKS || !s) return;
    a = &tessera.answer[tr];
    TESS_IRQ_OFF();
    if (k == 0u && (int32_t)tessera.sd_src[tr] + d <= (int32_t)TESSERA_TRACKS)   /* (KEYS: a generator's; ANSWER hears them) */
        tess_param_set_body(tr, TX_SD_SOURCE, (int32_t)tessera.sd_src[tr] + d);
    else if (k == 1u) tess_param_set_body(tr, TX_AN_ORDER, (int32_t)a->order + d);
    else if (k == 2u) tess_param_set_body(tr, TX_AN_SHIFT, (int32_t)a->shift + d);
    TESS_IRQ_ON();
}
