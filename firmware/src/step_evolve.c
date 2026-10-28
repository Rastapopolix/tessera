/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* STEP EVOLVE: a STEP track's EVOLVE page (LOCK, EVERY, AMOUNT, CLEAR) evolves the steps written by hand. The steps
 * stay as written; each step plays its change (vary.c), made a round at a time: every EVERY bars, at the pattern's
 * first step, each change kept at LOCK %, then new ones up to AMOUNT's share (100: half the steps). A MOVE goes by degrees of the track's scale
 * (CHR: semitones), drum hits never MOVE or jump an OCT. Tied and recorded steps are left alone. AMOUNT 0 or EVERY OFF:
 * the steps as written. REC held keeps the changes: they become the steps (ui_input.c capture_take). Not saved: a
 * loaded project's steps start as written. HOW (OCT+ held on the page, KNOB 1; saved) says what a round makes: DRIFT
 * (as before: moves, octaves, drops, swaps, ghosts), ADORN (ornaments, only adding: ratchets, octave jumps, neighbour
 * notes, passing notes in the rests) or SERIAL (the whole phrase at once, for the round: RETROGRADE, INVERSION around
 * its first note, ROTATION, AUGMENTATION, half the phrase at half speed; AMOUNT the chance a round brings one, LOCK
 * the chance it stays). Included by tessera.c */
_Static_assert(TESS_SV_STEPS == NSTEP && NSTEP <= VARY_MAX, "a change a step");

int tessera_step_on(uint32_t tr)
{
    return tr < TESSERA_TRACKS && tessera.mode[tr] == TESS_ENG_OFF && !trk[tr].p[P_AMODE];
}
static int sv_on(const track_t *t, uint32_t tr)
{
    (void)t;
    return tessera_step_on(tr) && tessera.sv_amount[tr] && tessera.sv_every[tr];
}
/* what step i holds (vary.c VK_*) */
static uint32_t sv_kind(const step_t *s)
{
    if (s->time == ST_TIE || (s->flags & SF_RECORDED)) return VK_KEEP;
    if (s->time == ST_REST || (!s->n && !s->hit)) return VK_EMPTY;
    return s->n ? VK_NOTE : VK_HITS;
}
/* note n moved d degrees of t's scale (CHR or none: semitones), 0..127 */
static uint32_t sv_degrees(const track_t *t, int32_t n, int32_t d)
{
    uint32_t m = scale_mask(t) & 0xFFFu, guard = 84;
    int32_t dir = d < 0 ? -1 : 1;
    if (!m) m = 0xFFFu;
    while (d && guard--) {
        n += dir;
        if ((m >> (uint32_t)((n - t->p[P_ROOT] % 12 + 1200) % 12)) & 1u) d -= dir;
    }
    return (uint32_t)(n < 0 ? 0 : n > 127 ? 127 : n);
}
/* the scale degrees from note a up to b (down: < 0), in t's scale (CHR or none: semitones) */
static int32_t sv_dist(const track_t *t, int32_t a, int32_t b)
{
    uint32_t m = scale_mask(t) & 0xFFFu;
    int32_t d = 0, dir = b < a ? -1 : 1;
    if (!m) m = 0xFFFu;
    while (a != b) {
        a += dir;
        if ((m >> (uint32_t)((a - t->p[P_ROOT] % 12 + 1200) % 12)) & 1u) d += dir;
    }
    return d;
}
/* step i of t as its change m plays it, into *out */
static void sv_apply(const track_t *t, uint32_t i, uint32_t m, step_t *out)
{
    const step_t *st = seq_steps(t), *s = &st[i];
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, src = vary_src(m), k;
    uint8_t kind[NSTEP];
    *out = *s;
    if (vary_dropped(m)) {
        out->time = ST_REST, out->n = 0, out->hit = 0, out->acc = 0;
        return;
    }
    if (src != VARY_OWN) {
        for (k = 0; k < len && k < NSTEP; k++) kind[k] = (uint8_t)sv_kind(&st[k]);
        k = vary_find(kind, len, i, src == VARY_NEXT ? 1 : -1);
        if (k < len) {
            const step_t *f = &st[k];
            memcpy(out->note, f->note, sizeof out->note);
            out->n = f->n, out->hit = f->hit, out->acc = f->acc, out->vel = f->vel;
            out->time = ST_NOTE;
            out->flags = (uint8_t)((s->flags & ~SF_ACCENT) | (f->flags & SF_ACCENT));
            if (sv_kind(s) == VK_EMPTY) {               /* a GHOST: softer, no accent */
                uint32_t v = f->vel ? f->vel : 96u;
                out->vel = (uint8_t)(v * 5u / 8u ? v * 5u / 8u : 1u);
                out->flags &= (uint8_t)~(SF_ACCENT | SF_SLIDE);
                out->acc = 0;
                out->probability = s->probability;
            }
        }
    }
    if (out->n && (vary_move(m) || vary_oct(m)))
        for (k = 0; k < out->n && k < 4u; k++)
            out->note[k] = (uint8_t)sv_degrees(t, (int32_t)out->note[k] + 12 * vary_oct(m), vary_move(m));
    if (out->n && vary_toward(m)) {                     /* (ORNAMENT's passing note: toward the next note) */
        uint8_t kind[NSTEP];
        for (k = 0; k < len && k < NSTEP; k++) kind[k] = (uint8_t)sv_kind(&st[k]);
        k = vary_find(kind, len, i, 1);
        if (k < len && st[k].n)
            out->note[0] = (uint8_t)sv_degrees(t, out->note[0], st[k].note[0] < out->note[0] ? -1 : 1);
    }
    if ((m & VARY_RATCH) && out->time == ST_NOTE && step_ratch(out) < 4u)
        step_set_ratch(out, step_ratch(out) + 1u);
}

/* SERIAL's transform of the phrase (tess_sv.ser: TESS_SV_SER_*, then its argument): step i of t as it plays it, into
 * *out; a step tied or recorded, at i or where it comes from, plays as written. 1 = out differs from the step */
static int sv_serial(const track_t *t, uint32_t tr, uint32_t i, step_t *out)
{
    const step_t *st = seq_steps(t), *s = &st[i];
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, kind = tess_sv.ser[tr], arg = tess_sv.ser_arg[tr], j = i, k;
    *out = *s;
    if (sv_kind(s) == VK_KEEP) return 0;
    if (kind == TESS_SV_SER_RETRO) j = len - 1u - i;
    else if (kind == TESS_SV_SER_ROT) j = (i + arg) % len;
    else if (kind == TESS_SV_SER_AUG) {
        if (i & 1u) {                                   /* (between: a rest) */
            out->time = ST_REST, out->n = 0, out->hit = 0, out->acc = 0;
            return sv_kind(s) != VK_EMPTY;
        }
        j = (arg ? len / 2u : 0u) + i / 2u;
    } else if (kind == TESS_SV_SER_INV) {               /* (the notes mirrored, in the scale, about the first one) */
        uint32_t f;
        for (f = 0; f < len && sv_kind(&st[f]) != VK_NOTE; f++) {}
        if (f == len || !s->n) return 0;
        for (k = 0; k < s->n && k < 4u; k++)
            out->note[k] = (uint8_t)sv_degrees(t, st[f].note[0], -sv_dist(t, st[f].note[0], s->note[k]));
        return memcmp(out->note, s->note, sizeof s->note) != 0;
    } else {
        return 0;
    }
    if (j >= len || j == i || sv_kind(&st[j]) == VK_KEEP) return 0;
    *out = st[j];
    out->flags = (uint8_t)((st[j].flags & ~SF_RECORDED) | (s->flags & SF_RECORDED));
    return memcmp(out, s, sizeof *s) != 0;
}
/* SERIAL's round: the transform kept at LOCK %, else at AMOUNT % a new one (each kind as likely), else none */
static void sv_serial_round(const track_t *t, uint32_t tr)
{
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, *rnd = &tess_sv.rnd[tr];
    if (tess_sv.ser[tr] && gen_rnd(rnd, 100u) < tessera.sv_lock[tr]) return;
    tess_sv.ser[tr] = tess_sv.ser_arg[tr] = 0;
    if (len < 2u || gen_rnd(rnd, 100u) >= tessera.sv_amount[tr]) return;
    tess_sv.ser[tr] = (uint8_t)(TESS_SV_SER_RETRO + gen_rnd(rnd, 4u));
    if (tess_sv.ser[tr] == TESS_SV_SER_ROT) tess_sv.ser_arg[tr] = (uint8_t)(1u + gen_rnd(rnd, len - 1u));
    if (tess_sv.ser[tr] == TESS_SV_SER_AUG) tess_sv.ser_arg[tr] = (uint8_t)gen_rnd(rnd, 2u);
}

/* seq.c, a STEP track's step about to play (t->seq_idx): a round of changes when it is due, then the step as its
 * change plays it (out), or s itself */
const step_t *tessera_step_vary(track_t *t, const step_t *s, step_t *out)
{
    uint32_t tr = trk_index(t), i = t->seq_idx, len, k, m;
    if (!sv_on(t, tr)) return s;
    len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    if (tess_sv.flash[tr]) tess_sv.flash[tr]--;
    if (!i && tess_sv.steps[tr] >= tessera_every_bars(tessera.sv_every[tr]) * tess_steps_per_bar(t)) {
        uint8_t kind[NSTEP];
        const step_t *st = seq_steps(t);
        for (k = 0; k < len && k < NSTEP; k++) kind[k] = (uint8_t)sv_kind(&st[k]);
        if (!tess_sv.rnd[tr]) tess_sv.rnd[tr] = gen_stream(0x5EEDu + tr);
        if (tessera.sv_how[tr] == TESS_SV_SERIAL) sv_serial_round(t, tr);
        else vary_round(tess_sv.mut[tr], kind, len, tessera.sv_amount[tr], tessera.sv_lock[tr], &tess_sv.rnd[tr],
                        tessera.sv_how[tr] == TESS_SV_ORNAMENT);
        tess_sv.steps[tr] = 0;
        tess_sv.flash[tr] = 4;
    }
    tess_sv.steps[tr]++;
    if (tessera.sv_how[tr] == TESS_SV_SERIAL) return i < len && tess_sv.ser[tr] && sv_serial(t, tr, i, out) ? out : s;
    m = i < NSTEP ? tess_sv.mut[tr][i] : 0u;
    if (!m || i >= len || sv_kind(s) == VK_KEEP) return s;
    sv_apply(t, i, m, out);
    return out;
}

/* the steps of track tr playing a change now (CLEAR's count; REC held keeps them) */
uint32_t tessera_step_varied(uint32_t tr)
{
    uint32_t n = 0, len;
    if (tr >= TESSERA_TRACKS || !sv_on(&trk[tr], tr)) return 0;
    len = trk[tr].p[P_SLEN] ? (uint32_t)trk[tr].p[P_SLEN] : 1u;
    if (tessera.sv_how[tr] == TESS_SV_SERIAL) {         /* (SERIAL: the steps its transform changes) */
        step_t v;
        for (uint32_t i = 0; i < len && i < NSTEP && tess_sv.ser[tr]; i++) n += (uint32_t)sv_serial(&trk[tr], tr, i, &v);
        return n;
    }
    for (uint32_t i = 0; i < len && i < NSTEP; i++) n += tess_sv.mut[tr][i] != 0u;
    return n;
}
/* the changes back to as written, from the next round on afresh (CLEAR; a pattern load) */
void tessera_step_clear(uint32_t tr)
{
    if (tr >= TESSERA_TRACKS) return;
    memset(tess_sv.mut[tr], 0, sizeof tess_sv.mut[tr]);
    tess_sv.ser[tr] = tess_sv.ser_arg[tr] = 0;
    tess_sv.steps[tr] = 0;
}
/* REC held on a STEP track with changes: they become its steps (the caller: an undo level, the interrupt off, the
 * NOTES view rebuilt); the steps written that way played as before. Returns the steps changed */
uint32_t tessera_step_keep(uint32_t tr)
{
    track_t *t;
    step_t v[NSTEP];
    uint32_t n = 0, len, i;
    if (tr >= TESSERA_TRACKS) return 0;
    t = &trk[tr];
    len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    if (tessera.sv_how[tr] == TESS_SV_SERIAL) {         /* (SERIAL: the transform's steps, from the steps as written) */
        uint64_t ch = 0;
        for (i = 0; i < len && i < NSTEP && tess_sv.ser[tr]; i++)
            if (sv_serial(t, tr, i, &v[i])) ch |= 1ull << i;
        for (i = 0; i < len && i < NSTEP; i++)
            if ((ch >> i) & 1u) {
                t->step[i] = v[i];
                n++;
            }
        tessera_step_clear(tr);
        return n;
    }
    for (i = 0; i < len && i < NSTEP; i++)              /* (each from the steps as written: then written over) */
        if (tess_sv.mut[tr][i] && sv_kind(&t->step[i]) != VK_KEEP) sv_apply(t, i, tess_sv.mut[tr][i], &v[i]);
    for (i = 0; i < len && i < NSTEP; i++)
        if (tess_sv.mut[tr][i] && sv_kind(&t->step[i]) != VK_KEEP) {
            t->step[i] = v[i];
            n++;
        }
    tessera_step_clear(tr);
    return n;
}

/* OCT+ held on a STEP track's EVOLVE page: KNOB 1 HOW (ui_input.c) */
void tessera_sv_layer_knob(uint32_t k, int32_t s)
{
    uint32_t tr = song.sel;
    tessera_ensure_init();
    if (tr >= TESSERA_TRACKS || !s || k) return;
    TESS_IRQ_OFF();
    tess_param_set_body(tr, TX_SV_HOW, (int32_t)tessera.sv_how[tr] + (s > 0 ? 1 : -1));
    TESS_IRQ_ON();
}
