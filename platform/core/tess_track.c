/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* A track copied, pasted and moved (tess_core.h: tess_core_track_copy, _paste, _swap). Included by tess_core.c, so it
 * sees the firmware's statics. An 8-track build only (the plugin's, Andamento's): the FM-1 has none of it.
 *
 * A copy (TTK1) is the track whole, in the form XTRK keeps each of tracks 5..8 (project_tsp.c xt_trk_put): its sound,
 * patterns, the Tessera block and its own settings; then what the project keeps about it elsewhere: its references
 * (TWEAK FOLLOW, BRANCH TO, LOGIC WITH, QUAD LINK, SECTION PC), the mods onto it, its timed notes and (tracks 1..4)
 * its motion. Bytes, so a front end may hand it to another instance (the system clipboard): the same build reads it.
 *   "TTK1", version, NTRK, the track copied, u16 body length, the body; FOLLOW, BRANCH TO, LOGIC WITH, QUAD LINK,
 *   SECTION PC (0/1); the mods onto it: a count, then each one's SRC, SRC TRACK, DST, AMOUNT, OFFSET, UNI, HOLD clock,
 *   SCALE (0/1), CC number; the timed notes: a u16 count, then each one (its owner's track left out); the motion: a
 *   count, each event's place (its step), parameter, value (u16) and pattern, then MOTION on (0/1); its CHORD USER
 *   qualities (GEN_CH_USER_N bytes, gen.h uq), its cells of ENTRIES' grid (a byte: letter A's 2 bits low), its
 *   ATLAS BANK (a byte, 0 in a build without the banks) its FLOURISH WHERE, WHAT, HEAT (a byte each, 0 without) and its CHORD HIT when a chop (DOUBLE 4, BUBBLE 5;
 *   else 0: the record has the rest), then its generator's LANE and RANGE (a byte each, 0 without), then CHORD MOVE BY
 *   NOTES' IN (0 / 1) and its four offsets + 12 (0..24; IN 0, 12 without).
 * A paste of the TRACK keeps the target's MIDI OUT channel and the mods' and references' track numbers (Theo
 * 2026-11-26: a pasted track's mods that point at other tracks keep their numbers); one of the SOUND alone changes
 * what a user preset load does (the engine and the parameters param_kept leaves, the FM6, ORACLE and DRUM patches).
 * A swap moves two tracks whole and renumbers every reference to them, so the piece plays on as before. */

#if CORE_TRACKS > 4
#define TT_VER 7u                                        /* (2: the qualities and the grid's cells; 3: ATLAS's BANK;
                                                         * 4: FLOURISH's WHERE, WHAT, HEAT; 5: CHORD HIT's chop;
                                                         * 6: the generator's LANE and RANGE; 7: CHORD MOVE BY
                                                         * NOTES' IN and four offsets) */
#define TT_HEAD 9u                                       /* "TTK1", version, NTRK, the track, u16 body length */
#define TT_CORD 9u                                       /* a mod's bytes */
#define TT_MOT 5u                                        /* a motion event's bytes */

static uint8_t tt_tmp[2][TSP_MAX] CORE_BIG;
#if TESS_ATLAS_BANKS
#define TT_BANK(t) tessera.grids[t].bank
#else
#define TT_BANK(t) 0u
#endif
#if TESS_FLOURISH
#define TT_FL(f, t) tessera.f[t]
#else
#define TT_FL(f, t) 0u
#endif
#if TESS_GEN_FIT
#define TT_GN(f, t) tessera.f[t]
#else
#define TT_GN(f, t) 0u
#endif              /* (two tracks' bodies: a swap, a SOUND paste's own) */

/* track t's body into b (cap bytes): its length, 0 too big */
static uint32_t tt_body(uint8_t *b, uint32_t cap, uint32_t t)
{
    uint32_t keep = tsp_cap;
    tsp_w_t w = {b, 0, 0, 0, 1};
    tsp_cap = cap;
    xt_trk_put(&w, t);
    tsp_cap = keep;
    return w.ok ? w.pos : 0u;
}

/* a body read into track t (checked first: a bad one changes nothing): 1 done */
static int tt_body_load(const uint8_t *b, uint32_t len, uint32_t t)
{
    const uint8_t *p = b;
    if (!xt_trk_get(&p, b + len, t, 0) || p != b + len) return 0;
    p = b;
    TESS_IRQ_OFF();
    xt_trk_get(&p, b + len, t, 1);
    TESS_IRQ_ON();
    pat_last[t] = 0;
    pat_sig[t] = steps_sig(&trk[t]);
    return 1;
}

/* a mod onto track t's (a track's destination, not a mod of a mod: those point at cords, not tracks) */
static int tt_cord_onto(const cord_t *c, uint32_t t)
{
    const cord_dest_t *d = cord_dest(c->dst);
    return !cord_empty(c) && d && d->kind < CK_GLOBAL && c->dst_trk == t &&
           !(c->dst >= CD_M1_AMT && c->dst <= CD_M16_AMT);
}

/* track t's cells of ENTRIES' arrangement grid, a byte (letter A low); and set from one */
static uint32_t tt_cells(uint32_t t)
{
    uint32_t s, c = 0;
    for (s = 0; s < 3u; s++) c |= (tessera.et_grid[s] >> (2u * t) & 3u) << (2u * s);
    return c;
}
static void tt_cells_set(uint32_t t, uint32_t c)
{
    for (uint32_t s = 0; s < 3u; s++)
        tessera.et_grid[s] = (uint16_t)((tessera.et_grid[s] & ~(3u << (2u * t))) | (c >> (2u * s) & 3u) << (2u * t));
}

static uint32_t tt_owner_trk(const recorded_note_t *r) { return (uint32_t)(r->owner & REC_OWNER_MASK) / NPAT; }

uint32_t tess_core_track_copy(uint32_t track, uint8_t *out, uint32_t cap)
{
    uint32_t n, i, pos, body, f;
    const tessera_evolve_t *ev;
    if (track >= NTRK || cap < TT_HEAD) return 0;
    body = tt_body(out + TT_HEAD, cap - TT_HEAD, track);
    if (!body || body > 0xFFFFu) return 0;
    memcpy(out, "TTK1", 4);
    out[4] = TT_VER; out[5] = NTRK; out[6] = (uint8_t)track; out[7] = (uint8_t)body; out[8] = (uint8_t)(body >> 8);
    pos = TT_HEAD + body;
#define TT_ROOM(k) do { if (cap - pos < (uint32_t)(k)) return 0; } while (0)
    ev = &tessera.evolve[track];
    TT_ROOM(5u);
    out[pos++] = ev->follow; out[pos++] = ev->br_to; out[pos++] = ev->lg_with;
    out[pos++] = tessera.quad[track].link; out[pos++] = (uint8_t)((tessera.sec_pc >> track) & 1u);
    for (i = n = 0; i < CORD_LIVE; i++) n += tt_cord_onto(&cords[i], track);
    TT_ROOM(1u + n * TT_CORD);
    out[pos++] = (uint8_t)n;
    for (i = 0; i < CORD_LIVE; i++)
        if (tt_cord_onto(&cords[i], track)) {
            const cord_t *c = &cords[i];
            uint8_t *o = out + pos;
            o[0] = c->src; o[1] = c->src_trk; o[2] = c->dst; o[3] = (uint8_t)c->amount; o[4] = (uint8_t)c->offset;
            o[5] = c->uni; o[6] = cord_hclk[i]; o[7] = (uint8_t)((cord_vmask >> i) & 1u); o[8] = cord_ccn[i];
            pos += TT_CORD;
        }
    for (i = n = 0; i < RECORD_MAX; i++) n += recording[i].vel && tt_owner_trk(&recording[i]) == track;
    TT_ROOM(2u + n * sizeof(recorded_note_t));
    out[pos++] = (uint8_t)n; out[pos++] = (uint8_t)(n >> 8);
    for (i = 0; i < RECORD_MAX; i++)
        if (recording[i].vel && tt_owner_trk(&recording[i]) == track) {
            recorded_note_t r = recording[i];
            r.owner = (uint8_t)(r.owner - track * NPAT);   /* (the bank alone, the flags above kept) */
            memcpy(out + pos, &r, sizeof r);
            pos += sizeof r;
        }
    f = motion_guard();
    for (i = n = 0; i < motion.count; i++) n += track < 4u && (motion.event[i].place >> 6) == track;
    TT_ROOM(2u + n * TT_MOT);
    out[pos++] = (uint8_t)n;
    for (i = 0; i < motion.count; i++)
        if (track < 4u && (motion.event[i].place >> 6) == track) {
            const motion_event_t *e = &motion.event[i];
            out[pos++] = e->place & 63u; out[pos++] = e->param;
            out[pos++] = (uint8_t)e->value; out[pos++] = (uint8_t)((uint16_t)e->value >> 8);
            out[pos++] = motion_pattern[i];
        }
    out[pos++] = (uint8_t)(track < 4u && ((motion.on >> track) & 1u));
    motion_unguard(f);
    TT_ROOM(GEN_CH_USER_N + 13u);
    memcpy(out + pos, tessera.chord[track].uq, GEN_CH_USER_N);
    pos += GEN_CH_USER_N;
    out[pos++] = (uint8_t)tt_cells(track);
    out[pos++] = TT_BANK(track);
    out[pos++] = TT_FL(fl_where, track);
    out[pos++] = TT_FL(fl_what, track);
    out[pos++] = TT_FL(fl_heat, track);
    out[pos++] = tessera.chord[track].hit >= GEN_CH_DOUBLE ? tessera.chord[track].hit : 0u;   /* (a chop: the record has
                                                         * its stand-in) */
    out[pos++] = TT_GN(gn_lane, track);
    out[pos++] = TT_GN(gn_range, track);
#if TESS_CH_NOTES
    out[pos++] = tessera.ch_odeg[track];
    for (uint32_t j = 0; j < 4u; j++) out[pos++] = (uint8_t)(tessera.ch_ofs[track][j] + 12);
#else
    out[pos++] = 0u;
    for (uint32_t j = 0; j < 4u; j++) out[pos++] = 12u;
#endif
#undef TT_ROOM
    return pos;
}

/* a copy's parts: its body, the rest after it; 1 a copy this build reads (every part checked but the body's own) */
typedef struct { const uint8_t *body, *refs, *cords, *recs, *mot, *more; uint32_t blen, ncord, nrec, nmot; } tt_parts_t;
static int tt_parse(const uint8_t *in, uint32_t len, tt_parts_t *q)
{
    const uint8_t *p, *e = in + len;
    uint32_t i;
    if (len < TT_HEAD || memcmp(in, "TTK1", 4) || in[4] != TT_VER || in[5] != NTRK || in[6] >= NTRK) return 0;
    q->blen = in[7] | (uint32_t)in[8] << 8;
    if (len - TT_HEAD < q->blen) return 0;
    q->body = in + TT_HEAD;
    p = q->body + q->blen;
#define TT_NEED(k) do { if ((uint32_t)(e - p) < (uint32_t)(k)) return 0; } while (0)
    TT_NEED(6u);
    q->refs = p;
    if (p[0] > NTRK || p[1] > NTRK || p[2] > NTRK || p[3] > 15u || p[4] > 1u) return 0;
    q->ncord = p[5]; p += 6;
    TT_NEED(q->ncord * TT_CORD);
    q->cords = p;
    for (i = 0; i < q->ncord; i++, p += TT_CORD) {
        const cord_dest_t *d = cord_dest(p[2]);
        if (p[0] >= CS_N || p[1] >= NTRK || !d || d->kind >= CK_GLOBAL || (p[2] >= CD_M1_AMT && p[2] <= CD_M16_AMT) ||
            (int8_t)p[3] < -100 || (int8_t)p[3] > 100 || (int8_t)p[4] < -100 || (int8_t)p[4] > 100 || p[5] > CU_HOLD ||
            p[6] > 4u || p[7] > 1u || p[8] > 119u)
            return 0;
    }
    TT_NEED(2u);
    q->nrec = p[0] | (uint32_t)p[1] << 8; p += 2;
    if (q->nrec > RECORD_MAX) return 0;
    TT_NEED(q->nrec * sizeof(recorded_note_t));
    q->recs = p;
    for (i = 0; i < q->nrec; i++, p += sizeof(recorded_note_t)) {
        recorded_note_t r;
        memcpy(&r, p, sizeof r);
        if (!r.vel || (uint32_t)(r.owner & REC_OWNER_MASK) >= NPAT || !recording_valid(&r)) return 0;
    }
    TT_NEED(1u);
    q->nmot = *p++;
    if (q->nmot > MOTION_MAX) return 0;
    TT_NEED(q->nmot * TT_MOT + 1u);
    q->mot = p;
    for (i = 0; i < q->nmot; i++, p += TT_MOT)
        if (p[0] >= NSTEP || p[1] >= P_COUNT || p[4] >= NPAT) return 0;
    if (p[0] > 1u) return 0;
    p++;
    TT_NEED(GEN_CH_USER_N + 13u);
    q->more = p;
    for (i = 0; i < GEN_CH_USER_N; i++)
        if (p[i] >= CH_N) return 0;
    if (p[GEN_CH_USER_N] >> 6 || p[GEN_CH_USER_N + 1u] >= GEN_AB_N || p[GEN_CH_USER_N + 2u] > 3u ||
        p[GEN_CH_USER_N + 3u] > 3u || p[GEN_CH_USER_N + 4u] > 2u ||
        (p[GEN_CH_USER_N + 5u] && (p[GEN_CH_USER_N + 5u] < 4u || p[GEN_CH_USER_N + 5u] > 5u)) ||
        p[GEN_CH_USER_N + 6u] > 8u || p[GEN_CH_USER_N + 7u] > 3u || p[GEN_CH_USER_N + 8u] > 1u) return 0;
    for (i = 9u; i < 13u; i++)
        if (p[GEN_CH_USER_N + i] > 24u) return 0;
    p += GEN_CH_USER_N + 13u;
#undef TT_NEED
    return p == e;
}

/* the mods onto track t gone (and the mods of mods that turned them) */
static void tt_cords_off(uint32_t t)
{
    uint32_t i, gone = 0;
    for (i = 0; i < CORD_LIVE; i++)
        if (tt_cord_onto(&cords[i], t)) { memset(&cords[i], 0, sizeof cords[i]); gone |= 1u << i; }
    for (i = 0; i < CORD_LIVE; i++)
        if (cords[i].dst >= CD_M1_AMT && cords[i].dst <= CD_M16_AMT && ((gone >> (cords[i].dst - CD_M1_AMT)) & 1u))
            memset(&cords[i], 0, sizeof cords[i]);
}

/* the whole track from a copy into track t: its sound, patterns, settings, references, mods, timed notes, motion */
static void tt_paste_track(const tt_parts_t *q, uint32_t t)
{
    uint32_t i, at, keep_ch = tessera.midi_ch[t], f;
    const uint8_t *r = q->refs;
    tessera_evolve_t *ev = &tessera.evolve[t];
    tt_body_load(q->body, q->blen, t);
    tessera.midi_ch[t] = (uint8_t)keep_ch;                /* (its MIDI OUT channel stays the target's) */
    ev->follow = r[0] == t + 1u ? 0u : r[0];             /* (never itself) */
    ev->br_to = r[1] == t + 1u ? 0u : r[1];
    ev->lg_with = r[2] == t + 1u ? 0u : r[2];
    if (tessera.follow_hold[t] == t + 1u) tessera.follow_hold[t] = 0;
    if (tessera.sd_src[t] == t + 1u) tessera.sd_src[t] = 0;
    tessera.quad[t].link = (uint8_t)(r[3] & ~(1u << t));
    tessera.sec_pc = (uint8_t)((tessera.sec_pc & ~(1u << t)) | (uint32_t)r[4] << t);
    TESS_IRQ_OFF();
    memcpy(tessera.chord[t].uq, q->more, GEN_CH_USER_N);
    tt_cells_set(t, q->more[GEN_CH_USER_N]);
#if TESS_ATLAS_BANKS
    tessera.grids[t].bank = q->more[GEN_CH_USER_N + 1u];
#endif
#if TESS_CHOPS
    if (q->more[GEN_CH_USER_N + 5u]) tessera.chord[t].hit = q->more[GEN_CH_USER_N + 5u];
#endif
#if TESS_FLOURISH
    tessera.fl_where[t] = q->more[GEN_CH_USER_N + 2u];
    tessera.fl_what[t] = q->more[GEN_CH_USER_N + 3u];
    tessera.fl_heat[t] = q->more[GEN_CH_USER_N + 4u];
#endif
#if TESS_GEN_FIT
    tessera.gn_lane[t] = q->more[GEN_CH_USER_N + 6u];
    tessera.gn_range[t] = q->more[GEN_CH_USER_N + 7u];
#endif
#if TESS_CH_NOTES
    tessera.ch_odeg[t] = q->more[GEN_CH_USER_N + 8u];
    for (i = 0; i < 4u; i++) tessera.ch_ofs[t][i] = (int8_t)((int32_t)q->more[GEN_CH_USER_N + 9u + i] - 12);
#endif
    tt_cords_off(t);
    for (i = at = 0; i < q->ncord; i++) {                /* (no free cord left: the rest left out) */
        const uint8_t *c = q->cords + i * TT_CORD;
        while (at < CORD_LIVE && !cord_empty(&cords[at])) at++;
        if (at == CORD_LIVE) break;
        cords[at] = (cord_t){c[0], c[1], c[2], (uint8_t)t, (int8_t)c[3], (int8_t)c[4], c[5]};
        cord_hclk[at] = c[6];
        cord_vmask = (cord_mask_t)((cord_vmask & ~(1u << at)) | (uint32_t)c[7] << at);
        cord_ccn[at] = c[8];
    }
    cord_masks();
    TESS_IRQ_ON();
    for (i = 0; i < RECORD_MAX; i++)
        if (recording[i].vel && tt_owner_trk(&recording[i]) == t) memset(&recording[i], 0, sizeof recording[i]);
    for (i = at = 0; i < q->nrec; i++) {
        while (at < RECORD_MAX && recording[at].vel) at++;
        if (at == RECORD_MAX) break;                     /* (the ring full: the rest left out) */
        memcpy(&recording[at], q->recs + i * sizeof recording[at], sizeof recording[at]);
        recording[at].owner = (uint8_t)(recording[at].owner + t * NPAT);
    }
    recording_reindex();
    if (t < 4u) {                                        /* (motion: tracks 1..4's, motion.c) */
        uint32_t n = 0;
        f = motion_guard();
        for (i = 0; i < motion.count; i++)
            if ((motion.event[i].place >> 6) != t) { motion_pattern[n] = motion_pattern[i]; motion.event[n++] = motion.event[i]; }
        for (i = 0; i < q->nmot && n < MOTION_MAX; i++, n++) {
            const uint8_t *m = q->mot + i * TT_MOT;
            motion.event[n] = (motion_event_t){(uint8_t)(t << 6 | m[0]), m[1], (int16_t)(m[2] | m[3] << 8)};
            motion_pattern[n] = m[4];
        }
        motion.count = (uint8_t)n;
        motion.on = (uint8_t)((motion.on & ~(1u << t)) | (uint32_t)q->mot[q->nmot * TT_MOT] << t);
        motion_unguard(f);
    }
}

/* the SOUND alone from a copy into track t (as a user preset load): the engine, the parameters param_kept leaves, the
 * FM6 / ORACLE / DRUM patches; the rest of the track as it was */
static int tt_paste_sound(const tt_parts_t *q, uint32_t t)
{
    track_t *k = &trk[t];
    uint32_t own = tt_body(tt_tmp[0], sizeof tt_tmp[0], t), i;
    int16_t p[P_COUNT];
    uint8_t eng, preset, fm[FP_SIZE + 1u], fn[FM6_NFN], orc[OR_N];
    drum_patch_t dp;
    if (!own || !tt_body_load(q->body, q->blen, t)) return 0;
    eng = k->eng_req; preset = k->preset;
    memcpy(p, k->p, sizeof p);
    memcpy(fm, fm6_patch[t], sizeof fm);
    memcpy(fn, fm6_fn[t], sizeof fn);
    memcpy(orc, or_patch_of(k), sizeof orc);
    dp = drum_patch[t];
    tt_body_load(tt_tmp[0], own, t);                     /* (the track as it was, then its sound replaced) */
    TESS_IRQ_OFF();
    k->eng_req = eng; k->preset = preset; k->user = 0; k->user_native = 0;
    for (i = 0; i < P_COUNT; i++)
        if (!param_kept(i)) k->p[i] = param_reserved(i) ? 0 : p[i];
    memcpy(fm6_fn[t], fn, sizeof fn);
    drum_patch[t] = dp;
    TESS_IRQ_ON();
    fm6_set_patch(t, fm);
    if (!or_set_patch(t, orc)) or_track_loaded(k);
    return 1;
}

uint32_t tess_core_track_paste(uint32_t track, uint32_t what, const uint8_t *in, uint32_t len)
{
    tt_parts_t q;
    const uint8_t *p;
    if (track >= NTRK || what > TESS_PASTE_SOUND || !tt_parse(in, len, &q)) return 1;
    p = q.body;
    if (!xt_trk_get(&p, q.body + q.blen, track, 0) || p != q.body + q.blen) return 1;
    if (what == TESS_PASTE_SOUND) { if (!tt_paste_sound(&q, track)) return 1; }
    else tt_paste_track(&q, track);
    panic_req |= (uint8_t)(1u << track);
    sync_reload = 1;
    ui.force = 1;
    return 0;
}

/* a track number (1-based, 0 none) after a swap of a and b (0-based) */
static uint8_t tt_swap1(uint32_t v, uint32_t a, uint32_t b) { return (uint8_t)(v == a + 1u ? b + 1u : v == b + 1u ? a + 1u : v); }
static uint32_t tt_swapbit(uint32_t m, uint32_t a, uint32_t b)
{
    uint32_t ba = (m >> a) & 1u, bb = (m >> b) & 1u;
    return (m & ~(1u << a | 1u << b)) | bb << a | ba << b;
}

uint32_t tess_core_track_swap(uint32_t a, uint32_t b)
{
    uint32_t la, lb, i, t, f;
    if (a >= NTRK || b >= NTRK) return 1;
    if (a == b) return 0;
#if TESS_CHOPS
    uint8_t ha = tessera.chord[a].hit, hb = tessera.chord[b].hit;   /* (the bodies hold a chop's stand-in) */
#endif
    la = tt_body(tt_tmp[0], sizeof tt_tmp[0], a);
    lb = tt_body(tt_tmp[1], sizeof tt_tmp[1], b);
    if (!la || !lb || !tt_body_load(tt_tmp[1], lb, a) || !tt_body_load(tt_tmp[0], la, b)) return 1;
    {                                                    /* (the references each keeps outside its body) */
        uint8_t fa = tessera.evolve[a].follow, ba = tessera.evolve[a].br_to, ga = tessera.evolve[a].lg_with;
        uint8_t qa = tessera.quad[a].link;
        tessera.evolve[a].follow = tessera.evolve[b].follow; tessera.evolve[a].br_to = tessera.evolve[b].br_to;
        tessera.evolve[a].lg_with = tessera.evolve[b].lg_with; tessera.quad[a].link = tessera.quad[b].link;
        tessera.evolve[b].follow = fa; tessera.evolve[b].br_to = ba; tessera.evolve[b].lg_with = ga; tessera.quad[b].link = qa;
    }
    tessera.sec_pc = (uint8_t)tt_swapbit(tessera.sec_pc, a, b);
    {                                                    /* (CHORD USER's qualities, ENTRIES' grid cells) */
        uint8_t uq[GEN_CH_USER_N];
        uint32_t ca = tt_cells(a), cb = tt_cells(b);
        TESS_IRQ_OFF();
        memcpy(uq, tessera.chord[a].uq, sizeof uq);
        memcpy(tessera.chord[a].uq, tessera.chord[b].uq, sizeof uq);
        memcpy(tessera.chord[b].uq, uq, sizeof uq);
        tt_cells_set(a, cb);
        tt_cells_set(b, ca);
#if TESS_ATLAS_BANKS
        {
            uint8_t k = tessera.grids[a].bank;
            tessera.grids[a].bank = tessera.grids[b].bank; tessera.grids[b].bank = k;
        }
#endif
#if TESS_CHOPS
        if (ha >= GEN_CH_DOUBLE) tessera.chord[b].hit = ha;   /* (the chops: the bodies held their stand-ins) */
        if (hb >= GEN_CH_DOUBLE) tessera.chord[a].hit = hb;
#endif
#if TESS_FLOURISH
        {
            uint8_t k;
            k = tessera.fl_where[a], tessera.fl_where[a] = tessera.fl_where[b], tessera.fl_where[b] = k;
            k = tessera.fl_what[a], tessera.fl_what[a] = tessera.fl_what[b], tessera.fl_what[b] = k;
            k = tessera.fl_heat[a], tessera.fl_heat[a] = tessera.fl_heat[b], tessera.fl_heat[b] = k;
        }
#endif
#if TESS_GEN_FIT
        {
            uint8_t k;
            k = tessera.gn_lane[a], tessera.gn_lane[a] = tessera.gn_lane[b], tessera.gn_lane[b] = k;
            k = tessera.gn_range[a], tessera.gn_range[a] = tessera.gn_range[b], tessera.gn_range[b] = k;
        }
#endif
#if TESS_CH_NOTES
        {
            int8_t o[4];
            uint8_t k = tessera.ch_odeg[a];
            tessera.ch_odeg[a] = tessera.ch_odeg[b], tessera.ch_odeg[b] = k;
            memcpy(o, tessera.ch_ofs[a], 4); memcpy(tessera.ch_ofs[a], tessera.ch_ofs[b], 4); memcpy(tessera.ch_ofs[b], o, 4);
        }
#endif
        TESS_IRQ_ON();
    }
    for (t = 0; t < NTRK; t++) {                         /* (every reference to a or b: the other) */
        tessera_evolve_t *ev = &tessera.evolve[t];
        ev->follow = tt_swap1(ev->follow, a, b); ev->br_to = tt_swap1(ev->br_to, a, b); ev->lg_with = tt_swap1(ev->lg_with, a, b);
        tessera.follow_hold[t] = tt_swap1(tessera.follow_hold[t], a, b);
        if (tessera.sd_src[t] != TESS_SD_KEYS) tessera.sd_src[t] = tt_swap1(tessera.sd_src[t], a, b);
        /* QUAD LINK: tracks 1..4's (a bit each); one past 4 has none, so a bit of a track moving there goes */
        tessera.quad[t].link = (uint8_t)(tt_swapbit(tessera.quad[t].link, a, b) & 15u & ~(1u << t));
    }
    TESS_IRQ_OFF();
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t *c = &cords[i];
        const cord_dest_t *d = cord_dest(c->dst);
        if (cord_empty(c)) continue;
        if (c->src && !CS_PROJECT(c->src)) c->src_trk = (uint8_t)(tt_swap1(c->src_trk + 1u, a, b) - 1u);
        if (d && d->kind < CK_GLOBAL && !(c->dst >= CD_M1_AMT && c->dst <= CD_M16_AMT))
            c->dst_trk = (uint8_t)(tt_swap1(c->dst_trk + 1u, a, b) - 1u);
        if (cord_hclk[i]) {                              /* (a HOLD by a track's notes: 1..4 only) */
            uint32_t h = tt_swap1(cord_hclk[i], a, b);
            cord_hclk[i] = (uint8_t)(h > 4u ? 0u : h);
        }
    }
    cord_masks();
    TESS_IRQ_ON();
    for (i = 0; i < RECORD_MAX; i++)
        if (recording[i].vel) {
            uint32_t o = recording[i].owner & REC_OWNER_MASK, tr = o / NPAT;
            if (tr == a || tr == b)
                recording[i].owner = (uint8_t)((recording[i].owner & ~REC_OWNER_MASK) | ((tr == a ? b : a) * NPAT + o % NPAT));
        }
    recording_reindex();
    f = motion_guard();                                  /* (motion: tracks 1..4's; one moving past 4 loses its) */
    {
        uint32_t n = 0, on = motion.on;
        for (i = 0; i < motion.count; i++) {
            uint32_t tr = motion.event[i].place >> 6, to = tr == a ? b : tr == b ? a : tr;
            if (to >= 4u) continue;
            motion_pattern[n] = motion_pattern[i];
            motion.event[n] = motion.event[i];
            motion.event[n++].place = (uint8_t)(to << 6 | (motion.event[i].place & 63u));
        }
        motion.count = (uint8_t)n;
        on = tt_swapbit(on, a, b);
        motion.on = (uint8_t)(on & 15u);
    }
    motion_unguard(f);
    if (song.sel == a) song.sel = (uint8_t)b;            /* (the selected track goes where it went) */
    else if (song.sel == b) song.sel = (uint8_t)a;
    undo_clear();                                        /* (the undo levels name tracks by number) */
    panic_req |= (uint8_t)(1u << a | 1u << b);
    sync_reload = 1;
    ui.force = 1;
    return 0;
}
#else
uint32_t tess_core_track_copy(uint32_t track, uint8_t *out, uint32_t cap)
{
    (void)track; (void)out; (void)cap;
    return 0;
}
uint32_t tess_core_track_paste(uint32_t track, uint32_t what, const uint8_t *in, uint32_t len)
{
    (void)track; (void)what; (void)in; (void)len;
    return 1;
}
uint32_t tess_core_track_swap(uint32_t a, uint32_t b)
{
    (void)a; (void)b;
    return 1;
}
#endif
