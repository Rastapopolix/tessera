/* SPDX-License-Identifier: GPL-3.0-only */
/* PAT GEN (Tessera, issue #440): each of a track's 8 patterns keeps its own generator setup, so switching pattern (by
 * hand, PATTERNS, SONG) switches what the generator plays. A slot holds MODE, EVOLVE's EVERY, AMOUNT, LOCK, SEED and
 * the knobs of its MODE, as bytes of the saved track record (tessera.c tessera_pack, version 19): stored by packing
 * the track and taking those bytes, loaded by packing the track, putting them back and loading the record through
 * its own range checks. FORM, TWEAK, TWEAK 2, NOTE LENGTH, SUB, FOLLOW and the rest stay the track's. Included by
 * melodee.c after tessera.c (docs/design/gen-per-pattern.md in tessera-notes) */

typedef struct { uint8_t at, n; } pg_range_t;
#define PG_COMMON 3u                                     /* MODE, EVERY AMOUNT LOCK, SEED: every slot's */
static const pg_range_t pg_common[PG_COMMON] = {{0, 1}, {20, 3}, {TESS_PK_TRACK_V1, 2}};
/* each MODE's knobs, at most 3 ranges (offsets: the TESS_PK_TRACK_V* parts) */
static const pg_range_t pg_mode[TESS_ENG_COUNT][3] = {
    [TESS_ENG_TURING]  = {{1, 5}},
    [TESS_ENG_EUCLID]  = {{6, 4}},
    [TESS_ENG_MARKOV]  = {{10, 3}},
    [TESS_ENG_MORPH]   = {{13, 7}},
    [TESS_ENG_GRIDS]   = {{TESS_PK_TRACK_V2, 4}, {TESS_PK_TRACK_V12, 4}, {TESS_PK_TRACK_V14, 1}},
    [TESS_ENG_BOUNCE]  = {{TESS_PK_TRACK_V3, 3}},
    [TESS_ENG_AWAKE]   = {{TESS_PK_TRACK_V3 + 3, 19}},
    [TESS_ENG_POLY]    = {{TESS_PK_TRACK_V3 + 22, 7}},
    [TESS_ENG_QUAD]    = {{TESS_PK_TRACK_V5, 7}, {TESS_PK_TRACK_V9, 1}},   /* (not the grid's column: shared) */
    [TESS_ENG_CONSTEL] = {{TESS_PK_TRACK_V7, 22}, {TESS_PK_TRACK_V10, 1}},
    [TESS_ENG_SERIES]  = {{TESS_PK_TRACK_V8, 3}},
    [TESS_ENG_CHORD]   = {{TESS_PK_TRACK_V11, 5}, {TESS_PK_TRACK_V12 + 4, 4}, {TESS_PK_TRACK_V16, 1}},
    [TESS_ENG_ANSWER]  = {{TESS_PK_TRACK_V17, 32}},
};
_Static_assert(TESS_PK_TRACK_V19 == TESS_PK_TRACK_V17 + 32u, "PAT GEN's slots are version 19's bytes (pg_mode)");
static uint8_t pg_slot[TESSERA_TRACKS][NPAT][TESS_PG_BYTES] __attribute__((section(".pool")));

/* the slot's bytes <-> the track record's: dir 1 record -> slot, 0 slot -> record */
static void pg_move(uint8_t *rec, uint8_t *slot, uint32_t mode, int dir)
{
    uint32_t p = 0, i;
    for (i = 0; i < PG_COMMON + 3u; i++) {
        const pg_range_t *r = i < PG_COMMON ? &pg_common[i] : &pg_mode[mode][i - PG_COMMON];
        if (!r->n) break;
        if (dir) memcpy(slot + p, rec + r->at, r->n);
        else memcpy(rec + r->at, slot + p, r->n);
        p += r->n;
    }
}

/* the track's settings now into pattern b's slot */
void tessera_pg_store(uint32_t tr, uint32_t b)
{
    uint8_t rec[TESS_PK_TRACK];
    if (tr >= TESSERA_TRACKS || b >= NPAT) return;
    tess_pack_track(rec, tr);
    memset(pg_slot[tr][b], 0, TESS_PG_BYTES);
    pg_move(rec, pg_slot[tr][b], rec[0] < TESS_ENG_COUNT ? rec[0] : 0u, 1);
}

/* slot s on track tr's record now: 1 it passes the record's range checks (nothing changed), 0 not */
static int pg_record(uint32_t tr, const uint8_t *s, uint8_t *rec)
{
    if (s[0] >= TESS_ENG_COUNT) return 0;
    tess_pack_track(rec, tr);
    pg_move(rec, (uint8_t *)s, s[0], 0);
    return tess_pk_track(rec, tr, 0);
}
int tessera_pg_check(uint32_t tr, const uint8_t *s)
{
    uint8_t rec[TESS_PK_TRACK];
    return tr < TESSERA_TRACKS && pg_record(tr, s, rec);
}

/* pattern b's settings into the track (PAT GEN on), the generator from the start, as a MODE change does */
void tessera_pg_load(uint32_t tr, uint32_t b)
{
    uint8_t rec[TESS_PK_TRACK];
    if (tr >= TESSERA_TRACKS || b >= NPAT || !tessera.pat_gen[tr]) return;
    if (!pg_record(tr, pg_slot[tr][b], rec)) return;
    tessera.active_gate[tr] = 0;
    tess_pk_track(rec, tr, 1);
    tessera.awake[tr].edit = 0;
    if (tessera.seed[tr]) tessera.rnd[tr] = gen_stream(tessera.seed[tr]);   /* (its knobs as stored: not the seed's) */
    else if (tessera.mode[tr] != TESS_ENG_OFF) tess_seed_fresh(tr);
    tess_atlas_new &= (uint8_t)~(1u << tr);                 /* (ATLAS's point is the pattern's) */
    if (tess_has_form(tr)) tess_form_restart(tr);
    tess_rearm(tr);
    answer_restart(tr);
    tess_join_due |= (uint8_t)(1u << tr);
}

void tessera_pg_copy(uint32_t tr, uint32_t from, uint32_t to)
{
    if (tr < TESSERA_TRACKS && from < NPAT && to < NPAT) memcpy(pg_slot[tr][to], pg_slot[tr][from], TESS_PG_BYTES);
}
/* PAT GEN turned on: every pattern gets the settings as they are */
static void pg_fill(uint32_t tr)
{
    for (uint32_t b = 0; b < NPAT; b++) tessera_pg_store(tr, b);
}
/* the MODE pattern b plays (PAT GEN on), the track's otherwise */
uint32_t tessera_pg_mode(uint32_t tr, uint32_t b)
{
    if (tr >= TESSERA_TRACKS) return TESS_ENG_OFF;
    if (!tessera.pat_gen[tr] || b >= NPAT || b == trk[tr].pattern) return tessera.mode[tr];
    return pg_slot[tr][b][0] < TESS_ENG_COUNT ? pg_slot[tr][b][0] : TESS_ENG_OFF;
}
/* the record's PGEN part (project_tsp.c) */
const uint8_t *tessera_pg_slots(uint32_t tr) { return pg_slot[tr % TESSERA_TRACKS][0]; }
void tessera_pg_put(uint32_t tr, const uint8_t *slots) { memcpy(pg_slot[tr % TESSERA_TRACKS], slots, NPAT * TESS_PG_BYTES); }
