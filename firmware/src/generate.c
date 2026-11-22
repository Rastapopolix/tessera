/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Thomas Bradley (LEMONADE SANDWICH, tessera-rnd: the rolls); Theo Dainis (GENERATE) */
/* GENERATE: one press, a whole piece. A style and a seed make the four tracks' sounds, generators, key, registers,
 * cords and tempo, and the long-form layers' settings; every one of them an ordinary Tessera setting (pid_set), so
 * the piece is a project like any other: saved, replayed, edited, every layer with its off position.
 *
 * The styles are LEMONADE SANDWICH's rolls (github.com/TheGrizard/LemonadeSandwich firmware/src/rnd.c, main 2098bf4,
 * Thomas Bradley, GPL-3.0-only), ported as they are: their roles, sounds, keys, registers, generator recipes, cords and
 * levels, by the measurements and listening their comments record. What changes (the design note "GENERATE: one
 * button, a whole piece", 2026-11-22):
 *   - repeatable: a piece is (style, seed, variation, this file's version); the draws come in a fixed order from the
 *     seed, every track's SEED from it too, and a press starts from a new project (project_new_blank), never from what
 *     was there, so the same four give the same .tsp byte for byte (tests/generate_test.c)
 *   - VARIATION 0..100: how wide each pick is in its range (100: the rolls' own; 0: the middle of every range, the
 *     heaviest choice of every list)
 *   - before / after: the project before the press, kept as its own .tsp in RAM (no list of fields to go stale), and
 *     swapped with what is there now (generate_swap)
 *   - left to the player: the transport (never started), the tempo when asked (GEN_KEEP_TEMPO: a DAW or MIDI clock
 *     drives), the voice BUDGET setting, the undo levels (a new project drops them: generate_swap is the undo). QNT is
 *     the rolls' WHITE (rnd_in_key): a piece starts from a new project, so it overrides no choice of the player's
 *   - the cost rules (rnd_cpu, rnd_tails: one FM6 and one ORACLE, short notes on fast tracks) only on a target with a
 *     voice budget (MELODEE_VOICE_BUDGET; the plugin has none). The CPU guard and DRIFT are not ported: the long-form
 *     layers move the piece (generate_layers), and DRIFT is to be decided after listening
 *   - MACRO is Plaits here, not Braids: the parts the rolls gave MACRO (SHORE's texture, WOZZA's DUST, RND's vowels)
 *     fall back as the rolls do on a build without it, until they are picked again by ear
 *   - not yet: the styles' own processes while they play (STATIC's build and drop, INTERLOCK's phasing, FORGE's and
 *     CHAOS's arc, COSMIC's new riffs), KEEP and re-generating one track, tracks 5..8 (they are left as a new project
 *     has them)
 * Three styles renamed (on-screen names, Theo 2026-11-22): FALL is AUTUMN (FALL is a PIECE shape and BOUNCE's), TIDE
 * SHORE (ORGANIC's TIDE; another maker's product), PHASE INTERLOCK (the PHASE engine). The code keeps rnd.c's names.
 *
 * In the build behind TESS_GENERATE (the plugin, core_defs.txt); the FM-1's image is unchanged. */
#ifndef TESS_GENERATE
#define TESS_GENERATE 0
#endif
#if TESS_GENERATE

#define RND_NT 4u                                   /* the tracks a style writes: the FM-1's four (5..8: later) */
#define MELODEE_BRAIDS 0                            /* (rnd.c's MACRO is Braids: none here, rnd_mc_ok) */
#ifndef MELODEE_FX_LEAN
#define MELODEE_FX_LEAN 0
#endif
#define RND_COST MELODEE_VOICE_BUDGET               /* the cost rules: only where voices share a budget */

/* the styles, in LEMONADE SANDWICH's menu order (rnd.c's codes; 0 its OFF) */
enum { RND_OFF, RND_FALL, RND_TIDE, RND_WOZZA, RND_ORBIT, RND_LOOPS, RND_HALO, RND_MIRROR, RND_PHASE, RND_CRATE,
       RND_COSMIC, RND_STATIC, RND_FORGE, RND_CHAOS, RND_FRACTURED, RND_FREE, RND_MODES };
static const char *const GEN_STYLE_NAME[RND_MODES - 1u] = {
    "AUTUMN", "SHORE", "WOZZA", "ORBIT", "LOOPS", "HALO", "MIRROR", "INTERLOCK", "CRATE", "COSMIC", "STATIC", "FORGE",
    "CHAOS", "FRACTURED", "RND"};
static uint8_t rnd_style = RND_FALL;                /* the style being made (rnd.c: MENU > REC BUTTON) */
static uint32_t rnd_setting(void) { return rnd_style; }

/* ------------------------------------------------------------ randomness --- */
/* xorshift32 (never 0) from the seed. A pick goes through rnd_u: its draw squeezed towards the middle by VARIATION */
static uint32_t rnd_state = 0x6D2B79F5u;
static uint32_t rnd_seed;                           /* the piece's seed */
static uint8_t rnd_var = 100u;                      /* VARIATION 0..100 */
static uint8_t rnd_nice;                            /* 1: a gentle style (rnd_roll) */
static uint8_t rnd_fall;                            /* 1: AUTUMN: every track TRIO on triangles */
static uint8_t rnd_loops;                           /* 1: LOOPS: four EUCLID loops of lengths apart */
static uint8_t rnd_orbit;                           /* 1: ORBIT: BOUNCE and CONSTELLATION only */
static uint32_t rnd_next(void)
{
    uint32_t x = rnd_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rnd_state = x;
}
static uint32_t rnd_u(uint32_t n)                   /* 0..n-1 (n <= 65536), VARIATION's: 0 the middle one */
{
    int32_t c = (int32_t)(rnd_next() >> 16) - 32768;
    uint32_t u = (uint32_t)(32768 + c * (int32_t)rnd_var / 100);
    return (uint32_t)(((uint64_t)u * n) >> 16);     /* (a multiply and a shift: no 64-bit division) */
}
static int32_t rnd_rr(int32_t lo, int32_t hi)       /* lo..hi, both ends */
{
    if (hi <= lo) return lo;
    return lo + (int32_t)rnd_u((uint32_t)(hi - lo + 1));
}
static int rnd_pct(uint32_t pct) { return rnd_u(100u) < pct; }
/* one of n codes, by weight (w[i]: 0 never) */
static uint32_t rnd_pick(const uint8_t *code, const uint8_t *w, uint32_t n)
{
    uint32_t total = 0, i, k;
    for (i = 0; i < n; i++) total += w[i];
    if (!total) return code[0];
    k = rnd_u(total);
    for (i = 0; i < n; i++) {
        if (k < w[i]) return code[i];
        k -= w[i];
    }
    return code[n - 1u];
}
/* a value between a and b, wild 0..100 of the way from a (the preset's) to a fresh one in lo..hi */
static int32_t rnd_push(int32_t a, int32_t lo, int32_t hi, uint32_t wild)
{
    int32_t b = rnd_rr(lo, hi);
    return a + (b - a) * (int32_t)wild / 100;
}
/* a track's SEED: any of them, whatever VARIATION (a squeezed draw would give every track the same) */
static int32_t rnd_seed_pick(void) { return 1 + (int32_t)((rnd_next() >> 8) % TESS_SEED_MAX); }

/* ------------------------------------------------------------ the roll: sound --- */
/* PHYS: in no roll (heard: its sounds skewed the tracks, cheap; the engine stays to play by hand) */
/* the engines a build leaves out never match (an engine number is 8 bits: the compiler drops what they guard; the
 * comparisons only, so a roll for a build without them costs nothing: room for COSMIC, 2026-11-10) */
#define RND_FM6 (MELODEE_FM6 ? ENGI_FM6 : 0x100u)
#define RND_PHYS (MELODEE_PHYS ? ENGI_PHYS : 0x100u)
static int rnd_phys_ok(void) { return 0; }
/* FM6 in no roll (asked 2026-11-07: its tines cheap; FM6 out of the device build too, MELODEE_FM6 0): ORACLE and TRIO
 * in its places. The host tests' build keeps FM6; the rolls are the device's either way */
static int rnd_fm6_ok(void) { return 0; }
/* MACRO (Braids, the device's: eng_braids.c) in STATIC, TIDE and RND (asked 2026-11-08: its textures and vowels;
 * the CPU fine). Its MODEL kept as the preset has it (a push would make a vowel a noise): rnd_mc_model */
static int rnd_mc_ok(void) { return eng_ok(ENGI_MACRO) && MELODEE_BRAIDS; }
/* a MACRO texture's level up to the TRIO layer it stands for (measured, the rolls' settings: TWIN PEAKS 7 dB under,
 * and the most uneven, its peaks narrow or wide; DUST 10, GRAIN CLOUD 5, the swarm and FB GROWL 4 to 8; a level step
 * half a dB) */
static void rnd_mc_level(uint32_t tr)
{
    static const uint8_t up[][2] = {{11, 14}, {14, 20}, {13, 10}, {10, 4}, {2, 16}, {8, 12}, {9, 12}};
    uint32_t k;
    if (trk[tr].eng_req != ENGI_MACRO || !rnd_mc_ok()) return;   /* (Braids' presets' numbers) */
    for (k = 0; k < NELEM(up); k++)
        if (trk[tr].preset == up[k][0]) pid_set(PID_TRACK(P_LEVEL), tr, clamp(trk[tr].p[P_LEVEL] + up[k][1], 0, 127));
}
static void rnd_mc_model(uint32_t tr)
{
#if MELODEE_BRAIDS
    if (trk[tr].eng_req == ENGI_MACRO) pid_set(PID_TRACK(P_E0), tr, MC_PRESETS[trk[tr].preset % NELEM(MC_PRESETS)].e[0]);
#else
    (void)tr;
#endif
}
static void rnd_tp(uint32_t tr, uint32_t id, int32_t v);
/* a track's knob, as a roll or DRIFT sets it. TRIO's INT2 and INT3: the nearest unison, octave or fifth (-24 -17 -12
 * -5 0 7 12 19 24), every roll and nudge (a semitone or a tritone off, heard: out of tune; DTN's cents kept) */
static void rnd_tp(uint32_t tr, uint32_t id, int32_t v)
{
    if (((id == P_E0 + 1u || id == P_E0 + 2u) && trk[tr].eng_req == 6u) ||
        ((id == P_E0 + 2u || id == P_E0 + 3u) && trk[tr].eng_req == ENGI_ORACLE)) {   /* (ORACLE's TUNE A, B too) */
        int32_t o = (v + 48) / 12 * 12 - 48, r = v - o;   /* (the octave below or at v, and how far over) */
        v = o + (r < 4 ? 0 : r < 10 ? 7 : 12);      /* (0..3: the octave; 4..9: its fifth; 10, 11: the next) */
        v = v > 24 ? 19 : v;
    }
    pid_set(PID_TRACK(id), tr, v);
}
static void rnd_tx(uint32_t tr, uint32_t id, int32_t v) { pid_set(PID_TESS(id), tr, v); }
/* the modulation a preset brought, off (WOZZA HALO CRATE: their own, set after): the envelope's pitch and shape, the
 * LFO's depths but pitch, its wave, sync, trigger and fade-in */
static void rnd_tp_still(uint32_t tr)
{
    static const uint8_t ids[] = {P_ED_PIT, P_ED_SHP, P_LD_FLT, P_LD_SHP, P_LD_AMP, P_LWAVE, P_LSYNC, P_LTRIG, P_LFADE};
    uint32_t i;
    for (i = 0; i < NELEM(ids); i++) rnd_tp(tr, ids[i], 0);
}
/* knobs at random in their ranges, in turn: {id, lo, hi} */
static void rnd_tp_rr(uint32_t tr, const int8_t (*t)[3], uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) rnd_tp(tr, (uint8_t)t[i][0], rnd_rr(t[i][1], t[i][2]));
}

/* the engines a roll may pick (built, offered, never DRUM): FM6, ORACLE and PHYS once each at most, and two of them
 * at most a roll (RND_HEAVY): each costs about twice TRIO, and on the FM-1 the rolls with all three were the ones past
 * 80 % CPU (host profile, 80 rolls: all of the heaviest held the three; the lightest two TRIO tracks) */
#define RND_HEAVY 2u
/* the roles of AMBIENT's four tracks (RND: RND_ROLE_FREE on each; GLITCH: two RND_ROLE_GLITCH, a drone, a melody) */
enum { RND_ROLE_FREE, RND_ROLE_DRONE, RND_ROLE_CHORDS, RND_ROLE_MELODY, RND_ROLE_GLITCH,
       RND_ROLE_LOWPERC, RND_ROLE_NOISE,           /* (PULSE: FM6's low percussion, TRIO's noise percussion) */
       RND_ROLE_OSTI,                              /* (WOZZA: two notes taking turns) */
       RND_ROLE_TINT, RND_ROLE_CANON,              /* (HALO: the tintinnabuli voice; the line slower, lower) */
       RND_ROLE_BASS, RND_ROLE_COUNTER };          /* (CRATE: the bass; a second voice answering) */
static uint8_t rnd_role_of[RND_NT];                  /* the last roll's roles (rnd_cpu, rnd_drift, the tests) */
static uint8_t rnd_wozza;                          /* 1: this roll is WOZZA's */
static uint8_t rnd_halo;                           /* 1: this roll is HALO's */
static uint8_t rnd_mirror;                         /* 1: this roll is MIRROR's */
static uint8_t rnd_tide;                           /* 1: this roll is TIDE's */
static uint8_t rnd_crate;                          /* 1: this roll is CRATE's */
static uint8_t rnd_phase;                          /* 1: this roll is PHASE's */
static uint8_t rnd_static;                         /* 1: this roll is STATIC's */
static uint8_t rnd_forge, rnd_chaos, rnd_cosmic, rnd_frac;               /* 1: this roll is FORGE's or CHAOS's (rnd_forge both, rnd_chaos
                                                     * CHAOS's) */
static uint8_t rnd_wz_lead = RND_NT;                 /* WOZZA: the first melody's track (a second one sparser, higher) */
static uint8_t rnd_want_e = 0xFFu, rnd_want_pi = 0xFFu;   /* PULSE: the engine and preset its role wants (0xFF: rolled) */
static int rnd_perc(uint32_t tr)                   /* GLITCH's glitch tracks, PULSE's percussion: each hit its attack */
{
    return rnd_role_of[tr] == RND_ROLE_GLITCH || rnd_role_of[tr] == RND_ROLE_LOWPERC || rnd_role_of[tr] == RND_ROLE_NOISE;
}
/* an engine's weight for a role (FM6 ORACLE PHYS TRIO; 2 for any other): a drone noisy (ORACLE's noise, TRIO's noise
 * waves, PHYS bowed), chords a pad, a melody struck */
static uint32_t rnd_engine_weight(uint32_t e, uint32_t role)
{
    /* FM6 ORACLE PHYS TRIO; GLITCH's: FM6's zaps and metal, PHYS struck, ORACLE's noise, TRIO's noisy waves */
    static const uint8_t W[5][4] = {{3, 3, 2, 2}, {2, 4, 3, 3}, {4, 3, 1, 3}, {3, 2, 3, 3}, {4, 3, 4, 3}};
    uint32_t i = e == RND_FM6 ? 0u : e == ENGI_ORACLE ? 1u : e == RND_PHYS ? 2u : e == 6u ? 3u : 4u;
    return i < 4u ? W[role < 5u ? role : 0u][i] : 2u;
}
static uint32_t rnd_engine(uint32_t *fm6s, uint32_t *phys, uint32_t *oracles, uint32_t role)
{
    uint8_t code[NENG_SHOWN], w[NENG_SHOWN];
    uint32_t n = 0, k, full = *fm6s + *phys + *oracles >= RND_HEAVY;
    if (rnd_fall && eng_ok(6u)) return 6u;          /* (FALL: TRIO on every track, triangles; the ORACLE lead asked for
                                                     * was TIDE's, 2026-11-08) */
    if (rnd_want_e != 0xFFu && eng_ok(rnd_want_e) && (rnd_want_e != RND_PHYS || rnd_phys_ok())) {   /* (a mode's
                                                     * own engine for the role) */
        *fm6s += rnd_want_e == RND_FM6; *phys += rnd_want_e == RND_PHYS; *oracles += rnd_want_e == ENGI_ORACLE;
        return rnd_want_e;
    }
    for (k = 0; k < NENG_SHOWN; k++) {
        uint32_t e = eng_vis(k), wt = rnd_engine_weight(e, role);
        if (!eng_ok(e) || e == ENGI_DRUM || !ENGINES[e]->npresets) continue;
        if (e == ENGI_PHASE || e == ENGI_SID || e == ENGI_MACRO) continue;   /* (GENERATE: the engines the rolls were
                                                     * never heard on, the FM-1 has none of them: picked by ear later) */
        if (e == RND_FM6 && (*fm6s || !rnd_fm6_ok())) wt = 0u;
        if (e == RND_PHYS && (*phys || !rnd_phys_ok())) wt = 0u;
        if (e == ENGI_ORACLE && *oracles) wt = 0u;      /* (one at most: as heavy as FM6) */
        if (full && (e == RND_FM6 || e == RND_PHYS || e == ENGI_ORACLE)) wt = 0u;
        code[n] = (uint8_t)e;
        w[n++] = (uint8_t)wt;
    }
    k = n ? rnd_pick(code, w, n) : ENGI_FALLBACK;
    if (k == RND_FM6) (*fm6s)++;
    if (k == RND_PHYS) (*phys)++;
    if (k == ENGI_ORACLE) (*oracles)++;
    return k;
}

/* ORACLE's own patch roughed up: waves and noise on, POLY MOD and sync often, its own envelopes slow or snapping */
static void rnd_oracle(uint32_t tr, uint32_t wild)
{
    uint8_t *p = or_patch[tr];
    uint32_t i;
    (void)or_patch_of(&trk[tr]);                    /* (its patch made, if it never was) */
    for (i = 0; i < OR_N; i++) {
        uint32_t m = or_byte_max(i);
        if (!rnd_pct(wild)) continue;
        p[i] = (uint8_t)rnd_rr(0, (int32_t)m);
    }
    if (!(p[OR_SAW_A] | p[OR_PULSE_A] | p[OR_SAW_B] | p[OR_TRI_B] | p[OR_PULSE_B]))
        p[rnd_pct(50) ? OR_SAW_A : OR_PULSE_B] = 1;    /* (something to hear) */
    if (p[OR_LEVEL_A] < 50u && p[OR_LEVEL_B] < 50u) p[OR_LEVEL_A] = (uint8_t)rnd_rr(70, 127);
    if (p[OR_CUTOFF] < 30u) p[OR_CUTOFF] = (uint8_t)rnd_rr(30, 110);
    if (p[OR_SUSTAIN_AMP] < 40u && p[OR_DECAY_AMP] < 40u) p[OR_SUSTAIN_AMP] = (uint8_t)rnd_rr(60, 127);
    p[OR_RELEASE_AMP] = (uint8_t)rnd_rr(40, 127);
    p[OR_NOISE] = (uint8_t)(rnd_pct(45) ? rnd_rr(20, 127) : p[OR_NOISE]);   /* (hiss and grit, often) */
}

static void rnd_role_sound(uint32_t tr, uint32_t e, uint32_t role);
static void rnd_sound(uint32_t tr, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles, uint32_t role)
{
    track_t *t = &trk[tr];
    uint32_t e = rnd_engine(fm6s, phys, oracles, role), k;
    uint32_t wild = (uint32_t)(rnd_nice ? rnd_rr(0, 20) : rnd_rr(40, 100));
    const engine_t *en = ENGINES[e];
    if (rnd_orbit && (e == 6u || e == ENGI_ORACLE)) {   /* (ORBIT: leads, keys, pads, brass; no bells, plucks or
                                                     * chimes: heard, terrible, 2026-11-08) */
        static const uint8_t ot[] = {1, 2, 5, 6, 7, 8, 10, 11, 13}, oo[] = {1, 2, 3, 4, 5, 11, 12, 17, 18, 19, 20, 21, 22};
        rnd_want_e = (uint8_t)e;
        rnd_want_pi = e == 6u ? ot[rnd_next() % NELEM(ot)] : oo[rnd_next() % NELEM(oo)];
    }
    engine_load_of(t, e, rnd_want_pi < en->npresets && e == rnd_want_e ? rnd_want_pi
                                                                     : (uint32_t)rnd_rr(0, (int32_t)en->npresets - 1));
    if (rnd_orbit) rnd_want_e = rnd_want_pi = 0xFFu;
    if ((e == 6u && t->preset == 3u) || (e == ENGI_ORACLE && t->preset == 10u) || (e == ENGI_MACRO && t->preset == 7u))
        engine_load_of(t, e, e == 6u ? 1u : e == ENGI_ORACLE ? 11u : 16u);   /* (no bells anywhere, asked 2026-11-08:
                                                     * TRIO's RING BELL, ORACLE's POLY BELLS, MACRO's FM BELL out of
                                                     * every pick: a lead, the keys, MORPH PAD) */
    for (k = 0; k < 8u; k++) {                      /* the engine's eight, pushed from the preset's */
        const param_desc_t *d = pid_desc(PID_TRACK(P_E0 + k), tr);
        if (!d || d->max <= d->min) continue;
        int32_t lo = d->min, hi = d->max;
        if (e == ENGI_ORACLE && k >= 6u) lo = -20;  /* (LVL A, LVL B: not both shut) */
        rnd_tp(tr, P_E0 + k, d->fmt == F_ENUM ? (!rnd_nice && rnd_pct(wild) ? rnd_rr(lo, hi) : t->p[P_E0 + k])
                                          : rnd_push(t->p[P_E0 + k], lo, hi, wild));   /* (NICE: the preset's kinds kept) */
    }
    if (e == ENGI_ORACLE && !rnd_nice) rnd_oracle(tr, wild * 2u / 3u);
    /* the envelope: slow swells or hard snaps, long tails for the room outside */
    rnd_tp(tr, P_ATK, rnd_pct(60) ? rnd_rr(30, 127) : rnd_rr(0, 20));
    {
        static const int8_t rg[][3] = {{P_DEC, 20, 127}, {P_SUS, 30, 127}, {P_REL, 50, 127}, {P_ED_FLT, -64, 63}};
        rnd_tp_rr(tr, rg, NELEM(rg));
    }
    rnd_tp(tr, P_ED_PIT, 0);                        /* (no pitch modulation in any roll: rnd_no_pitch) */
    rnd_tp(tr, P_ED_SHP, rnd_rr(-64, 63));
    /* the LFO: slow mostly, now and then fast enough to tear */
    rnd_tp(tr, P_LRATE, rnd_pct(75) ? rnd_rr(0, 60) : rnd_rr(60, 127));
    rnd_tp(tr, P_LWAVE, rnd_rr(0, 4));
    rnd_tp(tr, P_LFADE, rnd_rr(0, 127));
    rnd_tp(tr, P_LD_PIT, 0);
    rnd_tp(tr, P_LD_FLT, rnd_rr(-64, 63));
    rnd_tp(tr, P_LD_SHP, rnd_rr(-64, 63));
    rnd_tp(tr, P_LD_AMP, rnd_pct(40) ? rnd_rr(0, 90) : 0);
    rnd_tp(tr, P_LTRIG, rnd_rr(0, 1));
    rnd_tp(tr, P_LSYNC, rnd_pct(30) ? rnd_rr(1, 10) : 0);
    /* the voice */
    {
        static const uint8_t vm[] = {V_POLY, V_MONO, V_LEGATO}, vw[] = {55, 20, 25};   /* (no UNISON: the CPU) */
        rnd_tp(tr, P_VOICE, rnd_pick(vm, vw, 3));
    }
    rnd_tp(tr, P_GLIDE, rnd_pct(40) ? rnd_rr(10, 127) : 0);
    rnd_tp(tr, P_DETUNE, rnd_rr(0, 127));
    rnd_tp(tr, P_SPRD, rnd_rr(0, 127));
    rnd_tp(tr, P_CHRD, rnd_pct(8) && e != RND_FM6 ? 1 : 0);    /* (now and then a triad, DIA3; rnd_cpu keeps it to a
                                                                 * slow track) */
    for (k = 0; k < 4u; k++) rnd_tp(tr, P_M1SRC + 3u * k, 0);          /* (the routes: cords below) */
    /* the mix: all four in, spread */
    rnd_tp(tr, P_LEVEL, rnd_rr(72, 96));             /* (under the default 104: four rough tracks at once kept
                                                     * off the output limiter with MASTER turned up) */
    rnd_tp(tr, P_PAN, rnd_rr(-45, 45));
    rnd_tp(tr, P_MUTE, 0);
    rnd_tp(tr, P_AMODE, 0);
    if (role != RND_ROLE_FREE) rnd_role_sound(tr, e, role);
}

/* AMBIENT: what each role's sound needs on top of the roll's own. A drone swells in and never quite stops, low and
 * still in the middle, ORACLE with its noise up, TRIO on a noisy wave, PHYS bowed; chords a slow pad; a melody struck
 * and ringing, higher up. No tremolo (LFO -> AMP) anywhere: a chop is not ambient */
static void rnd_role_sound(uint32_t tr, uint32_t e, uint32_t role)
{
    if (role == RND_ROLE_DRONE) {
        static const uint8_t tw[] = {7, 8, 12, 13, 0, 2}, tww[] = {4, 4, 2, 2, 2, 1};   /* P+N NOIS RING RING3 SAW3 PPT */
        static const uint8_t nw[] = {0, 4, 5, 3, 2}, nww[] = {4, 4, 3, 2, 2};           /* NICE: SAW3 TRI3 S&T SST PPT */
        {
            static const int8_t rg[][3] = {{P_ATK, 70, 127}, {P_DEC, 90, 127}, {P_SUS, 100, 127}, {P_REL, 100, 127}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_ED_PIT, 0);
        rnd_tp(tr, P_LRATE, rnd_rr(0, 25));
        rnd_tp(tr, P_LD_PIT, 0);
        rnd_tp(tr, P_LTRIG, 1);
        rnd_tp(tr, P_LSYNC, 0);
        rnd_tp(tr, P_VOICE, V_LEGATO);
        rnd_tp(tr, P_GLIDE, rnd_rr(50, 127));
        rnd_tp(tr, P_LEVEL, rnd_rr(70, 84));
        rnd_tp(tr, P_PAN, rnd_rr(-10, 10));
        if (e == ENGI_ORACLE) {
            uint8_t *p = or_patch[tr];
            p[OR_NOISE] = (uint8_t)(rnd_nice ? rnd_rr(0, 15) : rnd_rr(50, 127));
            p[OR_ATTACK_AMP] = (uint8_t)rnd_rr(60, 127);
            p[OR_SUSTAIN_AMP] = (uint8_t)rnd_rr(100, 127);
            p[OR_RELEASE_AMP] = (uint8_t)rnd_rr(90, 127);
            p[OR_RELEASE_ON] = 1;
        } else if (e == 6u) {
            rnd_tp(tr, P_E0, (int32_t)(rnd_nice ? rnd_pick(nw, nww, NELEM(nw)) : rnd_pick(tw, tww, NELEM(tw))));   /* (WAVE) */
        } else if (e == RND_PHYS) {
            rnd_tp(tr, P_E0 + 6u, rnd_rr(60, 127));     /* (BOW) */
        } else if (e == RND_FM6) {
            rnd_tp(tr, P_E0 + 7u, rnd_rr(10, 26));      /* (AEG: slower carriers, rnd_cpu's cap) */
        }
    } else if (role == RND_ROLE_CHORDS) {
        {
            static const int8_t rg[][3] = {{P_ATK, 40, 110}, {P_DEC, 60, 127}, {P_SUS, 90, 127}, {P_REL, 70, 110}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_ED_PIT, 0);
        rnd_tp(tr, P_LD_PIT, 0);
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_GLIDE, 0);
        rnd_tp(tr, P_CHRD, 0);
        rnd_tp(tr, P_LEVEL, rnd_rr(72, 88));
        rnd_tp(tr, P_PAN, rnd_rr(-20, 20));
        if (e == ENGI_ORACLE) {
            uint8_t *p = or_patch[tr];
            if (p[OR_NOISE] > 40u) p[OR_NOISE] = (uint8_t)rnd_rr(0, 40);
            p[OR_ATTACK_AMP] = (uint8_t)rnd_rr(40, 100);
            p[OR_SUSTAIN_AMP] = (uint8_t)rnd_rr(100, 127);
            p[OR_RELEASE_AMP] = (uint8_t)rnd_rr(70, 110);
            p[OR_RELEASE_ON] = 1;
        }
    } else {
        {
            static const int8_t rg[][3] = {{P_ATK, 0, 40}, {P_SUS, 40, 110}, {P_REL, 60, 115}, {P_ED_PIT, 0, 0},
                                           {P_LRATE, 0, 50}, {P_LEVEL, 80, 96}, {P_PAN, -50, 50}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        if (rnd_pct(30)) rnd_tp(tr, P_VOICE, V_LEGATO);
    }
    rnd_tp(tr, P_LD_AMP, 0);
    if (rnd_nice) {                                 /* NICE: the patch near the preset, clean and still */
        static const uint8_t clean[] = {0, 1, 2, 3, 4, 5}, cw[] = {4, 3, 2, 2, 4, 3};   /* SAW3 PLS3 PPT SST TRI3 S&T */
        rnd_tp(tr, P_ED_FLT, rnd_rr(-10, 30));
        rnd_tp(tr, P_ED_PIT, 0);
        rnd_tp(tr, P_ED_SHP, rnd_rr(-15, 15));
        rnd_tp(tr, P_LRATE, rnd_rr(10, 50));
        rnd_tp(tr, P_LWAVE, 0);                     /* (a sine) */
        rnd_tp(tr, P_LD_PIT, 0);
        rnd_tp(tr, P_LD_FLT, rnd_rr(-15, 15));
        rnd_tp(tr, P_LD_SHP, rnd_rr(-15, 15));
        rnd_tp(tr, P_LSYNC, 0);
        rnd_tp(tr, P_DETUNE, rnd_rr(0, 40));
        rnd_tp(tr, P_SPRD, rnd_rr(30, 90));
        if (role == RND_ROLE_MELODY) rnd_tp(tr, P_GLIDE, rnd_pct(20) ? rnd_rr(10, 50) : 0);
        if (e == 6u && trk[tr].p[P_E0] > 5) rnd_tp(tr, P_E0, (int32_t)rnd_pick(clean, cw, NELEM(clean)));
        if (rnd_fall && e == 6u) {                  /* FALL: three triangles, consonant, soft */
            static const uint8_t i2[] = {24, 36, 31, 12, 43}, i2w[] = {3, 3, 2, 1, 1};   /* (+24: 0 +12 +7 -12 +19) */
            static const uint8_t i3[] = {12, 24, 36, 31}, i3w[] = {3, 2, 2, 1};    /* (-12 0 +12 +7: never -24, the drone
                                                     * two octaves under, heard: a rumble, mud under effects) */
            static const uint8_t d3[] = {24, 36, 31}, d3w[] = {3, 2, 2};    /* (the drone: 0 +12 +7, no octave under) */
            rnd_tp(tr, P_E0, 4);                    /* WAVE TRI3 */
            rnd_tp(tr, P_E0 + 1u, (int32_t)rnd_pick(i2, i2w, NELEM(i2)) - 24);   /* INT2 */
            rnd_tp(tr, P_E0 + 2u, role == RND_ROLE_DRONE ? (int32_t)rnd_pick(d3, d3w, NELEM(d3)) - 24 :
                                                          (int32_t)rnd_pick(i3, i3w, NELEM(i3)) - 24);   /* INT3 */
            rnd_tp(tr, P_E0 + 3u, role == RND_ROLE_MELODY ? rnd_rr(2, 10) : rnd_rr(6, 16));   /* DTN, cents: the pads
                                                     * chorused, warm (asked 2026-11-09: FALL warmer) */
            rnd_tp(tr, P_E0 + 4u, 0);               /* MODE LP */
            rnd_tp(tr, P_E0 + 5u, role == RND_ROLE_MELODY ? rnd_rr(66, 96) : rnd_rr(84, 112));   /* CUT: the leaves
                                                     * round, the pads full */
            rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 18));   /* RES */
            if (role == RND_ROLE_MELODY) {          /* (a leaf lands: struck, ringing out) */
                {
                    static const int8_t rg[][3] = {{P_ATK, 0, 10}, {P_DEC, 40, 90}, {P_SUS, 0, 50}, {P_REL, 60, 110}};
                    rnd_tp_rr(tr, rg, NELEM(rg));
                }
            }
        }
        if (e == ENGI_ORACLE && or_patch[tr][OR_NOISE] > 15u) or_patch[tr][OR_NOISE] = (uint8_t)rnd_rr(0, 15);
    }
}

/* ------------------------------------------------------------ the roll: effects within a CPU budget --- */
/* What each effect costs, in points (measured on the host, tests/ui_test.c test_rnd: four tracks rolled, one insert of
 * a kind on each, or one send effect; a point is about a twentieth of what a roll's engines cost), a send effect
 * RND_FX_BUS more (its bus: the tracks' sends summed, its output mixed back). On the FM-1 a roll's effects had cost as
 * much again as its engines (80 to 105 % CPU, the overload: late blocks are a digital crunch and the voices it sheds a
 * stutter), and the modes' three sends and inserts at 14 to 20 points 70 to 80 %, so a roll's effects share
 * RND_FX_POINTS, and only the light ones are rolled (rnd_fx_light: ECHO FILTER ROOM TAPE CHORUS; never REVERB,
 * FORMANT, WIDTH, PHASER or SPIT). Inserts: FX 1 only. */
#define RND_FX_BUS 2u
#define RND_ROOM (MELODEE_FX_LEAN ? FX_OFF : FX_ROOM)       /* (ROOM, CHORUS out of the device build 2026-11-12: none) */
#define RND_CHORUS (MELODEE_FX_LEAN ? FX_OFF : FX_CHORUS)
static int rnd_fx_light(uint32_t type)
{
    return type == FX_OFF || type == FX_ECHO || type == FX_FILTER || type == FX_ROOM || type == FX_TAPE ||
           type == FX_CHORUS;
}
static uint32_t rnd_fx_points(uint32_t type, int send)
{
    uint32_t bus = send ? RND_FX_BUS : 0u;          /* (no recursion: the build's stack audit) */
    switch (type) {
    case FX_OFF: return 0u;
    case FX_ECHO: case FX_FILTER: return bus + 1u;
    case FX_ROOM: return bus + 3u;
    case FX_SPIT: case FX_TAPE: case FX_CHORUS: return bus + 4u;
    case FX_PHASER: return bus + 5u;
    case FX_WIDTH: return bus + 7u;
    case FX_FORMANT: return bus + 9u;
    case FX_REVERB: return bus + 14u;
    default: return bus + 6u;
    }
}
#define RND_FX_POINTS 10u
/* 1: rolls pick effects (rnd_sends, rnd_inserts, rnd_fx_mode, rnd_fx_wozza); 0: every roll dry (rnd_fx_dry). Off since
 * tessera-rnd-19: with effects the FM-1 clicked now and then on NICE and the others, and ran at 70 to 80 % */
#define RND_FX_ROLLS 0
/* a roll without effects: the send effects and every track's three off, nothing sent */
static void rnd_fx_dry(void)                         /* (GENERATE: the inserts off; the send effects and the sends as a
                                                     * new project has them, the player's to turn up or down. rnd.c
                                                     * turned them off and the sends to 0, for the room outside) */
{
    uint32_t tr, s;
    for (tr = 0; tr < RND_NT; tr++)
        for (s = 0; s < 3u; s++) rnd_tp(tr, P_FXTYPE + s * 4u, FX_OFF);
}
#define RND_INSERT_TRACKS 2u

/* the send effects (SPLIT: a send to each): a space (ROOM) or an echo, now and then the other too; the third off.
 * -> the points they use */
static uint32_t rnd_sends(void)
{
    uint32_t used = 0, s;
    uint32_t type[3];
    type[0] = rnd_pct(65) ? RND_ROOM : FX_ECHO;
    type[1] = rnd_pct(50) ? (type[0] == RND_ROOM ? FX_ECHO : RND_ROOM) : FX_OFF;
    type[2] = FX_OFF;
    pid_set(PID_GLOBAL(G_FXMODE), 0, 1);
    for (s = 0; s < 3u; s++) {
        uint32_t base = G_MFXTYPE + s * 4u;
        pid_set(PID_GLOBAL(base), 0, (int32_t)type[s]);
        pid_set(PID_GLOBAL(base + 1u), 0, rnd_rr(60, 120));
        pid_set(PID_GLOBAL(base + 2u), 0, rnd_rr(0, 127));
        pid_set(PID_GLOBAL(base + 3u), 0, rnd_rr(20, 120));
        used += rnd_fx_points(type[s], 1);
    }
    pid_set(PID_GLOBAL(G_FXRETURN), 0, rnd_rr(60, 100));
    return used;
}

/* the inserts: FX 1 on RND_INSERT_TRACKS tracks at most, each kind if the points left allow it; FX 2 and 3 off */
static void rnd_inserts(uint32_t used)
{
    static const uint8_t fx[] = {FX_FILTER, FX_ECHO, FX_TAPE, RND_CHORUS};
    static const uint8_t fw[] = {18, 12, 8, 6};
    uint8_t ok[NELEM(fx)], tracks[RND_NT] = {0, 1, 2, 3};
    uint32_t tr, k, n = 0, want = (uint32_t)rnd_rr(0, (int32_t)RND_INSERT_TRACKS);
    for (k = RND_NT - 1u; k > 0u; k--) {    /* (which tracks: shuffled) */
        uint32_t j = rnd_next() % (k + 1u);
        uint8_t v = tracks[k]; tracks[k] = tracks[j]; tracks[j] = v;
    }
    for (tr = 0; tr < RND_NT; tr++) {
        uint32_t t = tracks[tr], type = FX_OFF, s;
        if (n < want) {
            for (k = 0; k < NELEM(fx); k++) ok[k] = used + rnd_fx_points(fx[k], 0) <= RND_FX_POINTS ? fw[k] : 0u;
            type = rnd_pick(fx, ok, NELEM(fx));
            if (used + rnd_fx_points(type, 0) > RND_FX_POINTS) type = FX_OFF;
            if (type != FX_OFF) { used += rnd_fx_points(type, 0); n++; }
        }
        for (s = 0; s < 3u; s++) rnd_tp(t, P_FXTYPE + s * 4u, s == 0u ? (int32_t)type : FX_OFF);
        if (type != FX_OFF) {
            rnd_tp(t, P_FXMIX, rnd_rr(40, 110));
            rnd_tp(t, P_FXRATE, rnd_rr(0, 127));
            rnd_tp(t, P_FXCHAR, type == FX_SPIT ? rnd_rr(0, 70) : rnd_rr(0, 127));   /* (SPIT: its BITE not flat out) */
        }
        rnd_tp(t, P_FXSEND, rnd_rr(20, 110));
        rnd_tp(t, P_FXSEND2, song.g[G_MFX2TYPE] != FX_OFF ? rnd_rr(0, 100) : 0);
        rnd_tp(t, P_FXSEND3, 0);
    }
}

/* ------------------------------------------------------------ the roll: what plays --- */
/* AMBIENT: a drone moves between two to four notes, one every bar to four bars, legato; chords a CHORD track's
 * progression held (or on the beat) over two or four bars; a melody one of the line generators, sparser and slower
 * than RND's (no CHORD, POLY or QUAD: one line) */
static void rnd_generator(uint32_t tr, uint32_t *chords);
static void rnd_role_generator(uint32_t tr, uint32_t role, uint32_t *chords)
{
    if (role == RND_ROLE_FREE) {
        rnd_generator(tr, chords);
        return;
    }
    rnd_generator(tr, chords);                          /* (EVOLVE, TWEAK, TIMING and the rest as RND's, then:) */
    rnd_tx(tr, TX_FM_FORM, 0);
    rnd_tx(tr, TX_SUB_RATCH, 0);
    rnd_tp(tr, P_SSWING, 0);
    if (role == RND_ROLE_DRONE) {
        static const uint8_t dv[] = {7, 8, 9}, dw[] = {2, 4, 4};           /* 1/1 2BAR 4BAR */
        rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
        rnd_tx(tr, TX_TUR_LEN, rnd_rr(2, 4));
        rnd_tx(tr, TX_TUR_PROB, rnd_rr(0, 15));
        rnd_tx(tr, TX_TUR_SPAN, 1);
        rnd_tp(tr, P_SDIV, rnd_pick(dv, dw, NELEM(dv)));
        rnd_tp(tr, P_SGATE, 127);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_EV_EVERY, rnd_pct(50) ? rnd_rr(4, 5) : 0);    /* (8 or 16 bars, or never) */
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 30));
        rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 20));
    } else if (role == RND_ROLE_CHORDS) {
        rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
        (*chords)++;
        rnd_tx(tr, TX_CH_PROG, rnd_rr(0, 6));
        rnd_tx(tr, TX_CH_BARS, rnd_pct(80) ? rnd_rr(2, 3) : 1);
        rnd_tx(tr, TX_CH_HIT, rnd_pct(75) ? 0 : 1);
        rnd_tx(tr, TX_CH_OCT, 0);                      /* (its register: rnd_registers) */
        rnd_tx(tr, TX_CH_BREATH, rnd_rr(0, 60));
        rnd_tp(tr, P_SDIV, 0);
        rnd_tp(tr, P_SGATE, 127);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tx(tr, TX_TW_DROP, 0);
    } else {
        static const uint8_t gm[] = {TESS_ENG_TURING, TESS_ENG_MARKOV, TESS_ENG_AWAKE, TESS_ENG_BOUNCE, TESS_ENG_CONSTEL,
                                     TESS_ENG_SERIES, TESS_ENG_MORPH, TESS_ENG_EUCLID};
        static const uint8_t gw[] = {12, 12, 12, 8, 12, 8, 6, 6};
        static const uint8_t dv[] = {0, 1, 4, 6, 2, 7}, dw[] = {24, 30, 14, 16, 8, 8};   /* 1/4 1/8 8T 1/2 1/16 1/1 */
        uint32_t tries = 0;
        while ((tessera.mode[tr] == TESS_ENG_CHORD || tessera.mode[tr] == TESS_ENG_POLY ||
                tessera.mode[tr] == TESS_ENG_QUAD) && tries++ < 8u) {   /* (a line generator, its settings rolled) */
            if (tessera.mode[tr] == TESS_ENG_CHORD && *chords) (*chords)--;
            rnd_generator(tr, chords);
            rnd_tx(tr, TX_FM_FORM, 0);
            rnd_tx(tr, TX_SUB_RATCH, 0);
            rnd_tp(tr, P_SSWING, 0);
        }
        if (tessera.mode[tr] == TESS_ENG_CHORD || tessera.mode[tr] == TESS_ENG_POLY || tessera.mode[tr] == TESS_ENG_QUAD) {
            if (tessera.mode[tr] == TESS_ENG_CHORD && *chords) (*chords)--;
            rnd_tx(tr, TX_MODE, (int32_t)rnd_pick(gm, gw, NELEM(gm)));
        }
        if (tessera.mode[tr] == TESS_ENG_MARKOV) rnd_tx(tr, TX_MKV_DENS, rnd_rr(15, 50));
        if (tessera.mode[tr] == TESS_ENG_CONSTEL) rnd_tx(tr, TX_CS_DENS, rnd_rr(10, 50));
        if (tessera.mode[tr] == TESS_ENG_AWAKE) rnd_tx(tr, TX_AWK_PROB, rnd_rr(20, 70));
        rnd_tp(tr, P_SDIV, rnd_pick(dv, dw, NELEM(dv)));
        rnd_tx(tr, TX_TW_DROP, rnd_pct(60) ? rnd_rr(10, 50) : 0);
        rnd_tx(tr, TX_NL_MODE, rnd_pct(30) ? 1 : 0);
        if (rnd_fall && rnd_pct(65)) {              /* FALL: BOUNCE's FALL (after ambalek's norns script): leaves */
            rnd_tx(tr, TX_MODE, TESS_ENG_BOUNCE);
            rnd_tx(tr, TX_BNC_BALLS, 5);
            rnd_tx(tr, TX_BNC_SPEED, rnd_rr(4, 20));
            rnd_tx(tr, TX_BNC_ANGLE, rnd_pct(60) ? rnd_rr(35, 55) : rnd_rr(15, 75));   /* (WIND: calm mostly) */
            rnd_tx(tr, TX_TW_DROP, rnd_pct(40) ? rnd_rr(5, 30) : 0);
            rnd_tp(tr, P_SDIV, rnd_pct(60) ? 1 : 0);   /* (1/8 or 1/4) */
        }
    }
}

static void rnd_generator(uint32_t tr, uint32_t *chords)
{
    static const uint8_t gm[] = {TESS_ENG_TURING, TESS_ENG_EUCLID, TESS_ENG_MARKOV, TESS_ENG_MORPH, TESS_ENG_BOUNCE,
                                 TESS_ENG_AWAKE, TESS_ENG_POLY, TESS_ENG_QUAD, TESS_ENG_CONSTEL, TESS_ENG_SERIES,
                                 TESS_ENG_CHORD};
    uint8_t gw[] = {12, 10, 12, 8, 8, 10, 8, 6, 10, 8, 8};
    uint32_t mode;
    if (*chords) gw[NELEM(gm) - 1u] = 0;            /* (one CHORD track a roll: QNT CHRD has one to follow) */
    mode = rnd_pick(gm, gw, NELEM(gm));
    if (mode == TESS_ENG_CHORD) (*chords)++;
    rnd_tx(tr, TX_MODE, (int32_t)mode);
    switch (mode) {
    case TESS_ENG_TURING:
        rnd_tx(tr, TX_TUR_LEN, rnd_rr(2, 16)); rnd_tx(tr, TX_TUR_PROB, rnd_rr(0, 60)); rnd_tx(tr, TX_TUR_SPAN, rnd_rr(1, 3)); break;
    case TESS_ENG_EUCLID: {
        int32_t st = rnd_rr(3, 32);
        rnd_tx(tr, TX_EUC_STEPS, st); rnd_tx(tr, TX_EUC_PULSES, rnd_rr(1, st * 3 / 5 > 1 ? st * 3 / 5 : 1));
        rnd_tx(tr, TX_EUC_ROT, rnd_rr(0, st - 1)); break;
    }
    case TESS_ENG_MARKOV:
        rnd_tx(tr, TX_MKV_LEAP, rnd_rr(0, 100)); rnd_tx(tr, TX_MKV_DENS, rnd_rr(15, 80)); rnd_tx(tr, TX_MKV_SPAN, rnd_rr(1, 3)); break;
    case TESS_ENG_MORPH:
        rnd_tx(tr, TX_MRP_POS, rnd_rr(0, 127)); rnd_tx(tr, TX_MRP_SNPA, rnd_rr(0, 7)); rnd_tx(tr, TX_MRP_SNPB, rnd_rr(0, 7)); break;
    case TESS_ENG_BOUNCE:
        rnd_tx(tr, TX_BNC_SPEED, rnd_rr(1, 40)); rnd_tx(tr, TX_BNC_ANGLE, rnd_rr(0, 90)); rnd_tx(tr, TX_BNC_BALLS, rnd_rr(1, 4)); break;
    case TESS_ENG_AWAKE:
        rnd_tx(tr, TX_AWK_LENA, rnd_rr(1, 16)); rnd_tx(tr, TX_AWK_LENB, rnd_rr(1, 16)); rnd_tx(tr, TX_AWK_PROB, rnd_rr(20, 90)); break;
    case TESS_ENG_POLY:
        rnd_tx(tr, TX_POL_DIV1, rnd_rr(1, 16)); rnd_tx(tr, TX_POL_DIV2, rnd_rr(0, 16));   /* (two or three rhythms:
                                                                                         * each one a voice) */
        rnd_tx(tr, TX_POL_DIV3, rnd_pct(40) ? rnd_rr(1, 16) : 0); rnd_tx(tr, TX_POL_DIV4, 0);
        rnd_tx(tr, TX_POL_SPAN, rnd_rr(1, 3)); break;
    case TESS_ENG_QUAD:
        rnd_tx(tr, TX_QD_DIR, rnd_rr(0, 3)); rnd_tx(tr, TX_QD_PROB, rnd_rr(20, 90)); rnd_tx(tr, TX_QD_LINK, 0); rnd_tx(tr, TX_QD_RANGE, rnd_rr(0, 3));
        break;
    case TESS_ENG_CONSTEL:
        rnd_tx(tr, TX_CS_Y, rnd_rr(0, 14)); rnd_tx(tr, TX_CS_DENS, rnd_rr(10, 80)); rnd_tx(tr, TX_CS_SIZE, rnd_rr(0, 7));
        rnd_tx(tr, TX_CS_AUTO, rnd_rr(0, 10)); rnd_tx(tr, TX_CS_LEN, rnd_rr(1, 16)); rnd_tx(tr, TX_CS_FILL, rnd_pct(25)); break;
    case TESS_ENG_SERIES:
        rnd_tx(tr, TX_SR_START, rnd_rr(0, 127)); rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 7)); rnd_tx(tr, TX_SR_LOOP, rnd_rr(0, 16)); break;
    case TESS_ENG_CHORD:
        rnd_tx(tr, TX_CH_PROG, rnd_rr(0, 6)); rnd_tx(tr, TX_CH_BARS, rnd_pct(70) ? rnd_rr(2, 3) : rnd_rr(0, 1));
        rnd_tx(tr, TX_CH_HIT, rnd_pct(60) ? 0 : rnd_rr(1, 3)); rnd_tx(tr, TX_CH_OCT, rnd_rr(-1, 1)); rnd_tx(tr, TX_CH_BREATH, rnd_rr(0, 100));
        break;
    default: break;
    }
    rnd_tx(tr, TX_EV_SEED, rnd_seed_pick());
    /* the phrase alive: EVOLVE often, a FORM now and then */
    rnd_tx(tr, TX_EV_EVERY, rnd_pct(60) ? rnd_rr(1, 5) : 0);
    rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 70));
    if (mode != TESS_ENG_TURING) rnd_tx(tr, TX_EV_LOCK, rnd_rr(20, 90));   /* (Turing's LOCK is its PROB, set above) */
    rnd_tx(tr, TX_FM_FORM, rnd_pct(25) ? rnd_rr(1, 6) : 0);
    rnd_tx(tr, TX_FM_CONTRAST, rnd_rr(10, 90));
    rnd_tx(tr, TX_FM_DEVELOP, rnd_rr(0, 60));
    rnd_tx(tr, TX_TW_SHIFT, 0);                         /* (the register: TRN above) */
    rnd_tx(tr, TX_TW_DROP, rnd_pct(50) ? rnd_rr(0, 45) : 0);
    rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 50));
    rnd_tx(tr, TX_TW_FOLLOW, 0);
    rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
    /* how long notes hold: the step's, or long fixed ones for drones */
    rnd_tx(tr, TX_NL_MODE, rnd_pct(40) ? 1 : 0);
    rnd_tx(tr, TX_NL_LEN, rnd_rr(2, 5));
    /* now and then a stutter */
    rnd_tx(tr, TX_SUB_RATCH, rnd_pct(15) ? rnd_rr(1, 5) : 0);
    rnd_tx(tr, TX_SUB_CHANCE, rnd_rr(30, 100));
    rnd_tx(tr, TX_SUB_SHAPE, 0);
    rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 25));
    /* the track's clock: slow by leaning */
    {
        static const uint8_t dv[] = {0, 1, 2, 4, 6, 7}, dw[] = {24, 26, 10, 8, 20, 12};   /* 1/4 1/8 1/16 8T 1/2 1/1 */
        rnd_tp(tr, P_SDIV, rnd_pick(dv, dw, NELEM(dv)));
    }
    rnd_tp(tr, P_SLEN, rnd_rr(8, 32));
    rnd_tp(tr, P_SGATE, rnd_rr(50, 127));
    rnd_tp(tr, P_SSWING, rnd_pct(30) ? rnd_rr(5, 40) : 0);
}

/* ------------------------------------------------------------ WOZZA MOZZA --- */
/* WOZZA MOZZA: minimal piano and synth, low-key and worn, from five reference tracks measured on the PC (their tempo,
 * keys, timing, timbre and lo-fi): slow (72-84 BPM), plain keys (major, the pentatonics, now and then minor, dorian or
 * lydian), dark (little above 2 kHz), off the grid, tape-worn. The BED (RND_ROLE_DRONE): a dark pad or organ moving
 * every bar to four, legato, now and then a slow tremolo. The OSTINATO (RND_ROLE_OSTI): two notes taking turns
 * (SERIES, two steps), a beat or two each. The MELODY: an itty bitty line on three to five notes, repeats, short phrases
 * and gaps (TURING short and nearly locked, AWAKE, MARKOV sparse; DROP), piano mostly (PHYS' stiff string hammered,
 * FM6's tines), the ostinato a low piano when one is left. The
 * TEXTURE (RND_ROLE_NOISE): NOISE's hiss, rumble and crackle held low under it all; or, now and then, a second melody
 * high up and sparser. The pitched voices wobble as worn tape (the LFO a sine at about 2.5 Hz on the pitch: the
 * references' wow); each step late by a player's amount (TIMING HUMAN). Effects: rnd_fx_wozza */
#define RND_WZ_WOW 75                               /* (LFO RATE: about 2.5 Hz) */
/* PHYS as a piano: the string model stiff (STRC past a quarter: dispersion, piano-like), hammered near its end, a
 * soft tone, a long decay, the hammer's thump a little (EXC); never bowed. A preset's knobs, set whatever it was */
static void rnd_wozza_piano(uint32_t tr)
{
    if (trk[tr].p[P_E0] != PM_STRING) return;       /* (VIBES, KALIMBA, MARIMBA: bars, as they are) */
    rnd_tp(tr, P_E0 + 1u, rnd_rr(70, 105));         /* STRC: stiff */
    rnd_tp(tr, P_E0 + 2u, rnd_rr(35, 65));          /* BRIT: soft, felt */
    rnd_tp(tr, P_E0 + 3u, rnd_rr(80, 102));         /* DAMP: rings on */
    rnd_tp(tr, P_E0 + 4u, rnd_rr(12, 22));          /* POS: near the end, as a hammer */
    rnd_tp(tr, P_E0 + 5u, rnd_rr(85, 110));         /* ACC */
    rnd_tp(tr, P_E0 + 6u, 0);                       /* BOW off */
    rnd_tp(tr, P_E0 + 7u, rnd_rr(5, 25));           /* EXC: the hammer */
}
static void rnd_wozza_sound(uint32_t tr, uint32_t role, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    uint8_t nice = rnd_nice;
    uint32_t e;
    if (role == RND_ROLE_NOISE) {                   /* (NOISE; else MACRO's DUST, a record's crackle: TRIO's hiss too loud
                                                     * in some rolls, heard 2026-11-16; else that hiss, quieter) */
        rnd_want_e = eng_ok(11u) ? 11u : rnd_mc_ok() ? ENGI_MACRO : 6u;
        rnd_want_pi = rnd_want_e == ENGI_MACRO ? 14u : 0u;   /* (DUST) */
    } else if (role == RND_ROLE_MELODY) {           /* ORACLE's synth leads and its keys, or TRIO's leads (no plucks,
                                                     * no tines, no FM6: heard, toy-like) */
        static const uint8_t op[] = {11, 17, 21, 5}, ow[] = {4, 3, 2, 1};   /* KEYS SQUARE LEAD FLUTE SYNC LEAD */
        static const uint8_t tp[] = {10, 11, 1}, tw[] = {4, 3, 2};          /* SOFT LEAD BREATH FLUTE ARP LEAD */
        if (*oracles < 2u && eng_ok(ENGI_ORACLE) && rnd_pct(70)) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pick(op, ow, NELEM(op));
        else rnd_want_e = 6u, rnd_want_pi = rnd_pick(tp, tw, NELEM(tp));
    } else if (role == RND_ROLE_OSTI) {             /* ORACLE's keys, low (a second ORACLE: WOZZA slow, light), else
                                                     * TRIO's pads */
        if (*oracles < 2u && eng_ok(ENGI_ORACLE) && rnd_pct(55)) rnd_want_e = ENGI_ORACLE, rnd_want_pi = 11;   /* KEYS */
        else rnd_want_e = 6u, rnd_want_pi = rnd_pct(50) ? 8 : 5;        /* (SOFT PAD, WARM PAD) */
    } else {                                        /* the bed: a dark pad (TRIO: the pianos keep FM6 and PHYS) */
        static const uint8_t tp[] = {5, 7, 8, 14, 6}, tw[] = {4, 3, 3, 2, 2};    /* WARM HOLLOW SOFT PAD, FIFTH DRONE, STRING MACH */
        if (rnd_pct(30) && eng_ok(7u)) rnd_want_e = 7u, rnd_want_pi = rnd_pct(60) ? 3 : 0;   /* (WHEEL, where built) */
        else rnd_want_e = 6u, rnd_want_pi = rnd_pick(tp, tw, NELEM(tp));
    }
    rnd_nice = 1;                                   /* (the presets' kinds kept, their knobs pushed a little) */
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    e = trk[tr].eng_req;
    rnd_tp_still(tr);                               /* (the LFO a sine: the wow) */
    rnd_tp(tr, P_ED_FLT, rnd_rr(-10, 20));
    rnd_tp(tr, P_LRATE, RND_WZ_WOW + rnd_rr(-3, 3));
    rnd_tp(tr, P_LD_PIT, role == RND_ROLE_NOISE ? 0 : rnd_pct(80) ? 1 : 0);   /* (a fifth of a semitone: worn tape) */
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_DETUNE, rnd_rr(0, 40));
    rnd_tp(tr, P_SPRD, rnd_rr(0, 60));
    if (role == RND_ROLE_NOISE) {                   /* hiss, rumble, crackle: held, low, dark */
        if (e == 11u) {
            rnd_tp(tr, P_E0, rnd_pct(80) ? 0 : 1);  /* MODE ANLG (DUST: the crackle alone) */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(70, 127)); /* COLR: pink to brown (the rumble) */
            rnd_tp(tr, P_E0 + 2u, rnd_rr(55, 85));  /* FREQ */
            rnd_tp(tr, P_E0 + 3u, rnd_rr(0, 25));   /* RES */
            rnd_tp(tr, P_E0 + 4u, 0);               /* TRK: the same whatever the note */
            rnd_tp(tr, P_E0 + 5u, rnd_pct(65) ? rnd_rr(10, 60) : 0);   /* DENS: crackle */
            rnd_tp(tr, P_E0 + 6u, rnd_rr(20, 70));  /* DRFT */
            rnd_tp(tr, P_E0 + 7u, rnd_pct(30) ? rnd_rr(5, 30) : 0);    /* CRSH */
        } else if (e == ENGI_MACRO) {               /* DUST: sparse, dark, a little crushed */
            rnd_tp(tr, P_E0, 14);
            rnd_tp(tr, P_E0 + 1u, rnd_rr(15, 45));  /* TIMBR: how often */
            rnd_tp(tr, P_E0 + 3u, rnd_rr(50, 80));  /* TONE: dark */
        } else {
            rnd_tp(tr, P_E0, 8);                    /* (TRIO: NOIS; NOISE is retired on the FM-1's build) */
            rnd_tp(tr, P_E0 + 4u, 2);               /* MODE HP: a hiss over it all (the rumble gone, 2026-11-09: mud) */
            rnd_tp(tr, P_E0 + 5u, rnd_rr(70, 92));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 20));
        }
        rnd_tp(tr, P_ATK, rnd_rr(90, 120));
        rnd_tp(tr, P_DEC, 127);
        rnd_tp(tr, P_SUS, 127);
        rnd_tp(tr, P_REL, rnd_rr(90, 110));
        rnd_tp(tr, P_VOICE, V_LEGATO);
        rnd_tp(tr, P_LEVEL, e == 6u ? rnd_rr(40, 54) : rnd_rr(50, 66));   /* (TRIO's hiss 6 dB down, 2026-11-16) */
        rnd_tp(tr, P_PAN, 0);
        rnd_tp(tr, P_SPRD, rnd_rr(60, 127));
        return;
    }
    if (role == RND_ROLE_DRONE) {                   /* the bed: swells, holds, never quite stops */
        {
            static const int8_t rg[][3] = {{P_ATK, 70, 115}, {P_DEC, 90, 127}, {P_SUS, 100, 127}, {P_REL, 90, 115}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, V_LEGATO);
        rnd_tp(tr, P_GLIDE, rnd_pct(30) ? rnd_rr(20, 60) : 0);
        rnd_tp(tr, P_LEVEL, rnd_rr(74, 88));
        rnd_tp(tr, P_PAN, rnd_rr(-10, 10));
        if (rnd_pct(35)) {                          /* (a slow tremolo: 1.3 to 5.5 Hz, the references') */
            rnd_tp(tr, P_LD_PIT, 0);
            rnd_tp(tr, P_LRATE, rnd_rr(66, 84));
            rnd_tp(tr, P_LD_AMP, rnd_rr(25, 60));
        }
        if (e == 6u) rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 70));   /* (TRIO's CUT: dark) */
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 7u, rnd_rr(10, 24));
        if (e == 7u) { rnd_tp(tr, P_E0 + 3u, rnd_rr(0, 30)); rnd_tp(tr, P_E0 + 5u, 0); }   /* (WHEEL: TOP low, no CLICK) */
        return;
    }
    if (role == RND_ROLE_OSTI) {                    /* soft, round, a little held */
        {
            static const int8_t rg[][3] = {{P_ATK, 2, 30}, {P_DEC, 60, 100}, {P_SUS, 30, 90}, {P_REL, 50, 85}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, rnd_pct(60) ? V_MONO : V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(72, 86));
        rnd_tp(tr, P_PAN, rnd_rr(-25, 25));
        if (e == 6u) rnd_tp(tr, P_E0 + 5u, rnd_rr(35, 65));
        if (e == 7u) { rnd_tp(tr, P_E0 + 3u, rnd_rr(0, 20)); rnd_tp(tr, P_E0 + 5u, 0); }
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 5));   /* (MLVL: mellow) */
        if (e == RND_PHYS) {                       /* (the low piano: struck, ringing) */
            rnd_wozza_piano(tr);
            rnd_tp(tr, P_ATK, 0);
            rnd_tp(tr, P_DEC, rnd_rr(80, 110));
            rnd_tp(tr, P_SUS, rnd_rr(0, 30));
            rnd_tp(tr, P_VOICE, V_POLY);
        }
        return;
    }
    /* the melody: a soft synth lead or keys (ORACLE: its own envelopes) */
    {
        static const int8_t rg[][3] = {{P_ATK, 6, 30}, {P_DEC, 80, 115}, {P_SUS, 60, 100}, {P_REL, 60, 90}};
        rnd_tp_rr(tr, rg, NELEM(rg));
    }
    rnd_tp(tr, P_VOICE, e == ENGI_ORACLE && trk[tr].preset == 11u ? V_POLY : rnd_pct(60) ? V_MONO : V_LEGATO);
    rnd_tp(tr, P_LEVEL, rnd_rr(76, 92));
    rnd_tp(tr, P_PAN, rnd_rr(-35, 35));
    if (e == RND_FM6) { rnd_tp(tr, P_E0 + 2u, rnd_rr(-25, 5)); rnd_tp(tr, P_E0 + 7u, rnd_rr(0, 20)); }   /* (MLVL mellow, AEG
                                                     * ringing) */
    if (e == RND_PHYS) rnd_wozza_piano(tr);
    if (e == 6u) rnd_tp(tr, P_E0 + 5u, rnd_rr(45, 70));
}
/* an itty bitty line: a few notes, repeats, phrases and gaps, a player's timing */
static void rnd_wozza_line(uint32_t tr, int sparse)
{
    static const uint8_t dv[] = {1, 2, 4, 0}, dw[] = {55, 20, 10, 15};   /* 1/8 1/16 8T 1/4 */
    uint32_t k = (uint32_t)rnd_rr(0, 99);
    if (k < 50u) {                                  /* (TURING: a short loop, nearly locked, on an octave) */
        rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
        rnd_tx(tr, TX_TUR_LEN, rnd_rr(3, 7));
        rnd_tx(tr, TX_TUR_PROB, rnd_rr(0, 12));
        rnd_tx(tr, TX_TUR_SPAN, 1);
    } else if (k < 80u) {                           /* (AWAKE: two loops, gaps) */
        rnd_tx(tr, TX_MODE, TESS_ENG_AWAKE);
        rnd_tx(tr, TX_AWK_LENA, rnd_rr(3, 6));
        rnd_tx(tr, TX_AWK_LENB, rnd_rr(2, 5));
        rnd_tx(tr, TX_AWK_PROB, rnd_rr(50, 85));
    } else {                                        /* (MARKOV: sparse, near steps) */
        rnd_tx(tr, TX_MODE, TESS_ENG_MARKOV);
        rnd_tx(tr, TX_MKV_LEAP, rnd_rr(5, 35));
        rnd_tx(tr, TX_MKV_DENS, rnd_rr(40, 70));
        rnd_tx(tr, TX_MKV_SPAN, 1);
    }
    rnd_tp(tr, P_SDIV, rnd_pick(dv, dw, NELEM(dv)));
    rnd_tx(tr, TX_TW_DROP, sparse ? rnd_rr(45, 70) : trk[tr].p[P_SDIV] == 2 ? rnd_rr(35, 60) : rnd_rr(5, 35));
    rnd_tx(tr, TX_EV_EVERY, rnd_pct(70) ? rnd_rr(3, 5) : 0);   /* (now and then the line turns: 4 to 16 bars) */
    rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 30));
    rnd_tx(tr, TX_TW_HUMAN, rnd_rr(30, 70));
    rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(25, (int32_t)TESS_HUMAN_MS));
    rnd_tx(tr, TX_NL_MODE, 0);
    rnd_tp(tr, P_SGATE, rnd_rr(40, 90));
    if (rnd_pct(40)) rnd_tp(tr, P_SLEN, rnd_rr(5, 14));   /* (a loop its own length) */
    if (trk[tr].p[P_SDIV] == 0 && rnd_pct(60)) {     /* (on the beat: a note now and then repeated, two or three
                                                     * quick strikes: the references' repeats) */
        rnd_tx(tr, TX_SUB_RATCH, rnd_rr(1, 2));
        rnd_tx(tr, TX_SUB_CHANCE, rnd_rr(20, 50));
        rnd_tx(tr, TX_SUB_SHAPE, 0);
    }
}
static void rnd_wozza_generator(uint32_t tr, uint32_t role, uint32_t *chords)
{
    rnd_generator(tr, chords);                      /* (EVOLVE, TWEAK, TIMING and the rest as RND's, then:) */
    if (tessera.mode[tr] == TESS_ENG_CHORD && *chords) (*chords)--;
    rnd_tx(tr, TX_FM_FORM, 0);
    rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
    rnd_tx(tr, TX_TW_SHIFT, 0);
    rnd_tx(tr, TX_SUB_RATCH, 0);
    rnd_tp(tr, P_SSWING, 0);
    rnd_tp(tr, P_SLEN, 16);
    if (role == RND_ROLE_DRONE || role == RND_ROLE_NOISE) {   /* the bed: two to four notes, a bar to four each,
                                                     * held (SERIES: never a rest, as TURING's 0 bits are); the
                                                     * texture: one note, held */
        static const uint8_t dv[] = {7, 8, 9}, dw[] = {2, 4, 4};   /* 1/1 2BAR 4BAR */
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
        rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
        rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 3));
        rnd_tx(tr, TX_SR_LOOP, role == RND_ROLE_NOISE ? 1 : rnd_rr(2, 4));
        rnd_tp(tr, P_SDIV, role == RND_ROLE_NOISE ? 9 : rnd_pick(dv, dw, NELEM(dv)));
        rnd_tp(tr, P_SGATE, 127);
        rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_EV_EVERY, role == RND_ROLE_DRONE && rnd_pct(50) ? rnd_rr(4, 5) : 0);   /* (8 or 16 bars, or
                                                     * never) */
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 30));
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 20));
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 40));
        rnd_tx(tr, TX_NL_MODE, 0);
        return;
    }
    if (role == RND_ROLE_OSTI) {                    /* two notes taking turns, a beat or two each */
        static const uint8_t dv[] = {0, 6, 1}, dw[] = {55, 30, 15};   /* 1/4 1/2 1/8 */
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
        rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
        rnd_tx(tr, TX_SR_INTV, rnd_rr(2, 4));       /* (a third, a fourth, a fifth apart in the scale) */
        rnd_tx(tr, TX_SR_LOOP, 2);
        rnd_tp(tr, P_SDIV, rnd_pick(dv, dw, NELEM(dv)));
        rnd_tp(tr, P_SGATE, rnd_rr(60, 110));
        rnd_tx(tr, TX_TW_DROP, rnd_pct(50) ? rnd_rr(5, 25) : 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(20, 50));
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(20, (int32_t)TESS_HUMAN_MS));
        rnd_tx(tr, TX_NL_MODE, 0);
        return;
    }
    rnd_wozza_line(tr, rnd_role_of[tr] == RND_ROLE_MELODY && tr != rnd_wz_lead);
}
/* the registers: the bed low, in the middle now and then; the ostinato apart from it; the melody above; a second
 * melody higher; the texture where it is. And the tempo: slow */
static void rnd_wozza_registers(const uint8_t *role)
{
    int32_t bed = rnd_pct(50) ? -24 : -12, osti = bed + 12;   /* (each an octave over the one under, 2026-11-09: they
                                                     * shared octaves, muddy under effects) */
    int32_t mel = osti + 24;                        /* (two over: the keys play low, measured near the ostinato) */
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_TRANS, role[tr] == RND_ROLE_DRONE ? bed : role[tr] == RND_ROLE_OSTI ? osti :
                            role[tr] == RND_ROLE_NOISE ? 0 : tr == rnd_wz_lead ? mel : mel < 24 ? mel + 12 : 24);
        rnd_tp(tr, P_SSWING, 0);
    }
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(72, 84));
}
/* the key: plain (MAJ, the pentatonics; MIN, DOR, LYD now and then) */
static void rnd_key_wozza(void)
{
    static const uint8_t sc[] = {5, 1, 6, 2, 3, 9}, sw[] = {7, 5, 3, 2, 2, 1};   /* PEN MAJ MPEN MIN DOR LYD */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
/* the four: a bed, an ostinato, a melody, and the texture (a second melody in a quarter of rolls), in a random order */
static void rnd_roles_wozza(uint8_t *role)
{
    uint32_t k;
    role[0] = RND_ROLE_DRONE; role[1] = RND_ROLE_OSTI; role[2] = RND_ROLE_MELODY;
    role[3] = rnd_pct(75) ? RND_ROLE_NOISE : RND_ROLE_MELODY;
    for (k = RND_NT - 1u; k > 0u; k--) {
        uint32_t j = rnd_next() % (k + 1u);
        uint8_t v = role[k]; role[k] = role[j]; role[j] = v;
    }
    for (k = 0, rnd_wz_lead = RND_NT; k < RND_NT; k++)
        if (role[k] == RND_ROLE_MELODY && rnd_wz_lead == RND_NT) rnd_wz_lead = (uint8_t)k;
}

/* ------------------------------------------------------------ HALO --- */
/* HALO: after Arvo Pärt's tintinnabuli (Spiegel im Spiegel, Für Alina; the canons of Cantus): warm, hazy, slow (60 to
 * 72 BPM). The HALO itself (RND_ROLE_CHORDS): a pad holding the tonic triad (CHORD: DRONE, now and then TWO), breathing
 * slowly. The M-VOICE (RND_ROLE_MELODY): a slow line, mostly by step (SERIES, its steps one degree; FALL's leaves; a
 * loose TURING), on FALL's three triangles, warmer and hazier. The T-VOICE (RND_ROLE_TINT): bells
 * that follow the M-voice (FOLLOW) onto the nearest note of the triad (QNT CHRD): Pärt's tintinnabuli voice, an octave
 * above it or below (beside it, a high line's). The CANON (RND_ROLE_CANON): the M-voice again, half as fast, an octave
 * down (a high line's mostly in its own octave) (FOLLOW on a slower clock: a mensuration canon's slow voice). A
 * follower's TRANS is from the line's own note (FOLLOW takes it transposed), on the triangles or FM6's soft pads. Every voice detuned and spread; the line's and the halo's pitch drifting a hair
 * (the LFO slow on the pitch: haze). Dry, as every roll */
static uint8_t rnd_hl_m = RND_NT;                     /* HALO: the M-voice's track (the others follow it) */
static void rnd_halo_tri(uint32_t tr)               /* FALL's three triangles, warmer, hazier */
{
    static const uint8_t i2[] = {24, 36, 31, 12, 43}, i2w[] = {3, 3, 2, 1, 1};   /* (+24: 0 +12 +7 -12 +19) */
    static const uint8_t i3[] = {12, 24, 36, 0, 31}, i3w[] = {3, 2, 2, 1, 1};    /* (-12 0 +12 -24 +7) */
    rnd_tp(tr, P_E0, 4);                            /* WAVE TRI3 */
    rnd_tp(tr, P_E0 + 1u, (int32_t)rnd_pick(i2, i2w, NELEM(i2)) - 24);   /* INT2 */
    rnd_tp(tr, P_E0 + 2u, (int32_t)rnd_pick(i3, i3w, NELEM(i3)) - 24);   /* INT3 */
    rnd_tp(tr, P_E0 + 3u, rnd_rr(3, 10));           /* DTN, cents: the haze (22 beat like a wobble, heard) */
    rnd_tp(tr, P_E0 + 4u, 0);                       /* MODE LP */
    rnd_tp(tr, P_E0 + 5u, rnd_rr(48, 80));          /* CUT: warm */
    rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 20));           /* RES */
}
static void rnd_halo_sound(uint32_t tr, uint32_t role, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    uint8_t nice = rnd_nice;
    int ph = !*phys && rnd_phys_ok(), fm = !*fm6s && rnd_fm6_ok();
    uint32_t e;
    if (role == RND_ROLE_CHORDS) {                  /* the halo: a warm pad */
        static const uint8_t tp[] = {5, 7, 8, 6}, tw[] = {4, 3, 3, 2};   /* WARM HOLLOW SOFT PAD, STRING MACH */
        rnd_want_e = 6u, rnd_want_pi = rnd_pick(tp, tw, NELEM(tp));
    } else if (role == RND_ROLE_MELODY) {           /* the M-voice: FALL's triangles (a bowed string in the middle
                                                     * was a hiss, measured) */
        rnd_want_e = 6u, rnd_want_pi = 8;
    } else if (role == RND_ROLE_TINT) {             /* the T-voice: a soft flute or voice (no bells anywhere, asked
                                                     * 2026-11-08): ORACLE's FLUTE or VOX, TRIO's BREATH FLUTE or
                                                     * SOFT LEAD */
        int orc = !*oracles && eng_ok(ENGI_ORACLE);
        (void)fm; (void)ph;
        if (orc && rnd_pct(55)) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pct(60) ? 21 : 18;
        else rnd_want_e = 6u, rnd_want_pi = rnd_pct(55) ? 11 : 10;
    } else {                                        /* the canon: triangles or a soft pad, low (PHYS bowed: a
                                                     * hiss as often as not, measured) */
        if (fm && rnd_pct(35)) rnd_want_e = ENGI_FM6, rnd_want_pi = rnd_pct(50) ? 25 : 26;   /* SOFT PAD, SOFT SWELL */
        else rnd_want_e = 6u, rnd_want_pi = 8;
    }
    rnd_nice = 1;                                   /* (the presets' kinds kept, their knobs pushed a little) */
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    e = trk[tr].eng_req;
    rnd_tp_still(tr);
    rnd_tp(tr, P_ED_FLT, rnd_rr(0, 15));
    rnd_tp(tr, P_LRATE, rnd_rr(40, 56));            /* (0.4 to 1 Hz: a slow sway) */
    rnd_tp(tr, P_LD_PIT, 0);                        /* (no sway on the pitch: heard, far too much) */
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_DETUNE, rnd_rr(40, 100));
    rnd_tp(tr, P_SPRD, rnd_rr(60, 127));
    if (e == 6u && role != RND_ROLE_TINT && role != RND_ROLE_CHORDS) rnd_halo_tri(tr);
    if (role == RND_ROLE_CHORDS) {                  /* the halo: swells in, holds, breathes */
        if (e == 6u) { rnd_tp(tr, P_E0 + 3u, rnd_rr(4, 12)); rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 65)); }   /* (DTN, CUT) */
        rnd_tp(tr, P_ATK, rnd_rr(90, 120));
        rnd_tp(tr, P_DEC, 127);
        rnd_tp(tr, P_SUS, rnd_rr(110, 127));
        rnd_tp(tr, P_REL, rnd_rr(105, 125));
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(58, 72));
        rnd_tp(tr, P_PAN, 0);
        if (!trk[tr].p[P_LD_PIT]) {                 /* (no sway on the pitch: a breath on the filter) */
            rnd_tp(tr, P_LRATE, rnd_rr(20, 40));
            rnd_tp(tr, P_LD_FLT, rnd_rr(8, 20));
        }
    } else if (role == RND_ROLE_MELODY) {
        {
            static const int8_t rg[][3] = {{P_ATK, 15, 50}, {P_DEC, 90, 120}, {P_SUS, 60, 110}, {P_REL, 80, 110}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, rnd_pct(50) ? V_LEGATO : V_MONO);
        rnd_tp(tr, P_LEVEL, rnd_rr(76, 90));
        rnd_tp(tr, P_PAN, rnd_rr(-20, 20));
    } else if (role == RND_ROLE_TINT) {             /* soft, a step aside in the stereo (no bell's strike) */
        {
            static const int8_t rg[][3] = {{P_ATK, 10, 35}, {P_DEC, 75, 105}, {P_SUS, 40, 80}, {P_REL, 65, 95}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(62, 76));
        rnd_tp(tr, P_PAN, rnd_pct(50) ? rnd_rr(20, 45) : -rnd_rr(20, 45));
        if (e == RND_FM6) { rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 0)); rnd_tp(tr, P_E0 + 7u, rnd_rr(0, 14)); }   /* (MLVL, AEG) */
        if (e == RND_PHYS) {                       /* (struck, never bowed; quieter than FM6's bells: up) */
            rnd_tp(tr, P_E0 + 2u, rnd_rr(50, 80));
            rnd_tp(tr, P_E0 + 6u, 0);
            rnd_tp(tr, P_LEVEL, rnd_rr(88, 100));
        }
    } else {                                        /* the canon: slow, low, held */
        {
            static const int8_t rg[][3] = {{P_ATK, 50, 100}, {P_DEC, 110, 127}, {P_SUS, 100, 127}, {P_REL, 100, 120}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, V_LEGATO);
        rnd_tp(tr, P_LEVEL, rnd_rr(70, 84));
        rnd_tp(tr, P_PAN, rnd_rr(-15, 15));
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 7u, rnd_rr(10, 24));
    }
}
/* HALO's generators: the halo's triad, the M-voice's line, then the two that follow it (the T-voice on the M-voice's
 * clock, the canon on one twice as slow) */
static void rnd_halo_generators(const uint8_t *role)
{
    static const uint8_t dv[] = {0, 6, 1}, dw[] = {55, 30, 15};   /* 1/4 1/2 1/8 */
    uint32_t tr, chords = 1, mdiv = 0, k;
    for (tr = 0, rnd_hl_m = RND_NT; tr < RND_NT; tr++) if (role[tr] == RND_ROLE_MELODY) rnd_hl_m = (uint8_t)tr;
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] == RND_ROLE_CHORDS) continue;
        rnd_generator(tr, &chords);                 /* (EVOLVE, TWEAK, TIMING and the rest as RND's; never CHORD) */
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SLEN, 16);
    }
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] != RND_ROLE_CHORDS) continue;  /* the halo: the tonic triad held, four bars a chord */
        rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
        rnd_tx(tr, TX_CH_PROG, rnd_pct(70) ? 5 : 4);   /* DRONE (I alone), TWO (I IV) */
        rnd_tx(tr, TX_CH_BARS, 3);
        rnd_tx(tr, TX_CH_HIT, 0);
        rnd_tx(tr, TX_CH_OCT, 0);
        rnd_tx(tr, TX_CH_BREATH, rnd_rr(0, 40));
        rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tx(tr, TX_SUB_HUMAN, 0);
        rnd_tp(tr, P_SDIV, 0);
        rnd_tp(tr, P_SGATE, 127);
    }
    if (rnd_hl_m < RND_NT) {                          /* the M-voice: slow, mostly by step */
        tr = rnd_hl_m;
        k = (uint32_t)rnd_rr(0, 99);
        if (k < 65u) {
            rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
            rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
            rnd_tx(tr, TX_SR_INTV, 1);
            rnd_tx(tr, TX_SR_LOOP, rnd_pct(50) ? 0 : rnd_rr(6, 16));
        } else if (k < 85u) {                       /* (FALL's leaves) */
            rnd_tx(tr, TX_MODE, TESS_ENG_BOUNCE);
            rnd_tx(tr, TX_BNC_BALLS, 5);
            rnd_tx(tr, TX_BNC_SPEED, rnd_rr(4, 20));
            rnd_tx(tr, TX_BNC_ANGLE, rnd_rr(35, 55));
        } else {
            rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
            rnd_tx(tr, TX_TUR_LEN, rnd_rr(4, 8));
            rnd_tx(tr, TX_TUR_PROB, rnd_rr(5, 20));
            rnd_tx(tr, TX_TUR_SPAN, 1);
        }
        rnd_tp(tr, P_SDIV, tessera.mode[tr] == TESS_ENG_BOUNCE ? (rnd_pct(60) ? 1 : 0) : rnd_pick(dv, dw, NELEM(dv)));   /* (the
                                                     * leaves fall sparsely: on 1/8 or 1/4, as FALL's) */
        rnd_tp(tr, P_SGATE, rnd_rr(80, 120));
        rnd_tx(tr, TX_TW_DROP, rnd_rr(0, 20));
        rnd_tx(tr, TX_EV_EVERY, rnd_pct(50) ? rnd_rr(4, 5) : 0);
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 25));
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(10, 40));
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(10, 40));
        mdiv = (uint32_t)trk[tr].p[P_SDIV];
    }
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] != RND_ROLE_TINT && role[tr] != RND_ROLE_CANON) continue;
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);       /* (one note a step: the pitch is the M-voice's) */
        rnd_tx(tr, TX_SR_START, 0);
        rnd_tx(tr, TX_SR_INTV, 1);
        rnd_tx(tr, TX_SR_LOOP, 1);
        rnd_tx(tr, TX_TW_FOLLOW, rnd_hl_m < RND_NT ? (int32_t)rnd_hl_m + 1 : 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        if (role[tr] == RND_ROLE_TINT) {            /* on the M-voice's clock, onto the triad */
            rnd_tp(tr, P_SDIV, (int32_t)mdiv);
            if (mdiv == 1u) rnd_tp(tr, P_VOICE, V_MONO);   /* (on 1/8: one bell at a time) */
            rnd_tp(tr, P_SGATE, rnd_rr(50, 100));
            rnd_tp(tr, P_QUANT, Q_CHRD);
            rnd_tx(tr, TX_TW_DROP, rnd_rr(0, 30));
            rnd_tx(tr, TX_TW_HUMAN, rnd_rr(20, 50));
            rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(10, 40));
        } else {                                    /* twice as slow, held */
            rnd_tp(tr, P_SDIV, mdiv == 1u ? 0 : mdiv == 0u ? 6 : 7);   /* 1/8 -> 1/4 -> 1/2 -> 1/1 */
            rnd_tp(tr, P_SGATE, 127);
            rnd_tx(tr, TX_TW_DROP, 0);
            rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 20));
            rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 30));
        }
    }
}
/* the registers: the halo in the middle or below, the M-voice above it, the bells an octave over the line (or under),
 * the canon an octave under (two, from a high line); and the tempo: slow */
static void rnd_halo_registers(const uint8_t *role)
{
    int32_t m = 12, halo = m - 12, t = 12;          /* (each its own octave, 2026-11-09: the canon under all, the halo,
                                                     * the line, the voice over it; the line lower, the canon and the
                                                     * halo met, measured. The voice's and the canon's TRANS are from the
                                                     * line's notes: they FOLLOW it) */   /* (the bells
                                                     * never past +12: higher, a glare) */
    int32_t c = -24;                                /* (two octaves under the line: under the halo) */   /* (never below -12: lower, a rumble) */
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++)
        rnd_tp(tr, P_TRANS, role[tr] == RND_ROLE_CHORDS ? halo : role[tr] == RND_ROLE_MELODY ? m :
                            role[tr] == RND_ROLE_TINT ? t : c);
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(100, 120));   /* (100 to 120, asked 2026-11-09: was 60 to 72) */
}
/* the key: MIN and MAJ mostly (Pärt's), DOR LYD MIX now and then */
static void rnd_key_halo(void)
{
    static const uint8_t sc[] = {2, 1, 3, 9, 4}, sw[] = {7, 5, 2, 2, 2};   /* MIN MAJ DOR LYD MIX */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
/* HALO's cords: slow wanderers only: the halo's colour, the line's colour, the bells' place */
static void rnd_cords_halo(const uint8_t *role)
{
    uint32_t i = 0, tr;
    cord_t c;
    for (tr = 0; tr < RND_NT && i < CORD_LIVE; tr++) {
        memset(&c, 0, sizeof c);
        c.src_trk = c.dst_trk = (uint8_t)tr;
        c.src = role[tr] == RND_ROLE_CHORDS ? CS_DRIFT : role[tr] == RND_ROLE_TINT ? CS_TORPOR : CS_INERTIA;
        c.dst = role[tr] == RND_ROLE_TINT ? CD_PAN : CD_CUTOFF;
        c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, 25) : -rnd_rr(10, 25));
        if (cord_set(i, &c)) i++;
    }
    for (memset(&c, 0, sizeof c); i < CORD_LIVE; i++) cord_set(i, &c);
}
/* the four: the halo, the M-voice, the T-voice, the canon, in a random order (a T-voice before its M-voice plays the
 * line a step behind: an echo) */
static void rnd_roles_halo(uint8_t *role)
{
    uint32_t k;
    role[0] = RND_ROLE_CHORDS; role[1] = RND_ROLE_MELODY; role[2] = RND_ROLE_TINT; role[3] = RND_ROLE_CANON;
    for (k = RND_NT - 1u; k > 0u; k--) {
        uint32_t j = rnd_next() % (k + 1u);
        uint8_t v = role[k]; role[k] = role[j]; role[j] = v;
    }
}

/* ------------------------------------------------------------ MIRROR --- */
/* MIRROR: HALO's mirror (Pärt's Spiegel im Spiegel: the mirror in the mirror): its four parts, glassy and digital,
 * the rules loosened. 70 to 100 BPM; a key in twelve notes (LYD WHOLE PEN MAJ DOR DIMHW) or, two rolls in five, off
 * them (SLENDRO, PELOG, 19 and 22 EDO, HARM16, JI-MAJ, Bohlen-Pierce). The HALO (RND_ROLE_CHORDS): glass, held or
 * pulsing: in twelve notes a CHORD track (DRONE, TWO, CIRCLE, WALK, 50S; HOLD, BEAT or OFFBEAT), off them a cluster
 * (SERIES on two or three notes, held). The LINE (RND_ROLE_MELODY): a digital line (SID's keys and plucks, FM6's
 * mallets, TRIO's sharper waves), by step or not (SERIES, TURING, BOUNCE, MARKOV, AWAKE), 1/16 to 1/4. The MIRROR
 * (RND_ROLE_TINT): glass bells following the line (FOLLOW) onto the halo's chord (QNT CHRD: the tintinnabuli voice) or
 * in parallel, a few degrees off (SHIFT: off twelve notes always), on the line's clock or twice as fast, now and then
 * stuttering (ratchets). The CANON (RND_ROLE_CANON): the line again, low, half as fast or across it in triplets. Bright
 * and wide, no wow; S&H and LFO cords on the shapes and the bells' place. DRIFT moves it as HALO's (rnd_mirror_move) */
static int rnd_mr_micro(void)                       /* (off twelve notes: no chords to snap to) */
{
    return trk[0].p[P_SCALE] >= 16;
}
static void rnd_mirror_sound(uint32_t tr, uint32_t role, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    /* MIRROR: HALO's opposite, cold, glassy and digital, in key (asked 2026-11-07): no warm pad; ORACLE's glass and
     * blips, TRIO's thin, sharp waves (pulses, ring and sync on octaves and fifths) through a high-pass, struck
     * (MACRO's wavetables out with their table, 2026-11-18: ORACLE's or TRIO's in their place) */
    uint8_t nice = rnd_nice;
    int orc = *oracles < 2u && eng_ok(ENGI_ORACLE);
    uint32_t e, k = (uint32_t)rnd_rr(0, 99);
    if (role == RND_ROLE_CHORDS) {                  /* glass: ORACLE's GLASS PAD or TRIO's thin pulses, high-passed */
        if (orc && k < 60u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = 2;
        else rnd_want_e = 6u, rnd_want_pi = 8;      /* (SOFT PAD, made thin and bright below) */
    } else if (role == RND_ROLE_MELODY) {           /* digital: ORACLE's SEQ BLIP or SYNC LEAD, TRIO's ring and sync */
        if (orc && k < 55u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pct(60) ? 20 : 5;
        else rnd_want_e = 6u, rnd_want_pi = rnd_pct(50) ? 1 : 2;   /* ARP LEAD, SYNC LEAD (their waves below) */
    } else if (role == RND_ROLE_TINT) {             /* glass bells */
        if (orc && k < 60u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pct(50) ? 20 : 2;   /* SEQ BLIP, GLASS PAD (no
                                                     * bells anywhere, 2026-11-08) */
        else rnd_want_e = 6u, rnd_want_pi = 1;      /* ARP LEAD (thin, high-passed below) */
    } else {                                        /* the canon: low, digital, struck (not HALO's held pad) */
        if (orc && k < 45u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = 20;   /* SEQ BLIP */
        else rnd_want_e = 6u, rnd_want_pi = 1;      /* ARP LEAD */
    }
    (void)fm6s; (void)phys;
    rnd_nice = 1;                                   /* (the presets' kinds kept, their knobs pushed a little: pushed
                                                     * further, SID's, FM6's and TRIO's turned to a hiss, measured) */
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    rnd_mc_model(tr);
    e = trk[tr].eng_req;
    rnd_tp(tr, P_ED_PIT, 0);
    rnd_tp(tr, P_ED_SHP, rnd_pct(30) ? rnd_rr(-30, 30) : 0);
    rnd_tp(tr, P_LD_PIT, 0);                        /* (no wow: glass) */
    rnd_tp(tr, P_LD_AMP, 0);
    rnd_tp(tr, P_LD_FLT, rnd_pct(40) ? rnd_rr(5, 25) : 0);
    rnd_tp(tr, P_LD_SHP, rnd_pct(50) ? rnd_rr(10, 40) : 0);   /* (the shape shimmering: PW, the wave) */
    rnd_tp(tr, P_LWAVE, rnd_pct(70) ? 0 : 4);       /* (a sine; or S&H: digital steps) */
    rnd_tp(tr, P_LSYNC, 0);
    rnd_tp(tr, P_LTRIG, 0);
    rnd_tp(tr, P_LFADE, 0);
    rnd_tp(tr, P_LRATE, rnd_rr(55, 90));
    rnd_tp(tr, P_GLIDE, role == RND_ROLE_MELODY && rnd_pct(20) ? rnd_rr(5, 30) : 0);
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_DETUNE, rnd_rr(0, 30));
    rnd_tp(tr, P_SPRD, rnd_rr(80, 127));
    if (e == 6u) {                                  /* (TRIO: thin and sharp; the glass and the canon pulses, the line
                                                     * and the bells ring and sync too; high-passed, bright) */
        static const uint8_t wv[] = {1, 3, 12, 9, 13, 10}, ww[] = {3, 2, 2, 2, 1, 1};   /* PLS3 SST RING SYNC RING3 SYNCP */
        rnd_tp(tr, P_E0, role == RND_ROLE_CHORDS || role == RND_ROLE_CANON ? (rnd_pct(60) ? 1 : 3) :
                         (int32_t)rnd_pick(wv, ww, NELEM(wv)));
        rnd_tp(tr, P_E0 + 1u, rnd_pct(50) ? 12 : 0);
        rnd_tp(tr, P_E0 + 2u, rnd_pct(40) ? 12 : rnd_pct(50) ? 7 : 24);
        rnd_tp(tr, P_E0 + 3u, rnd_rr(0, 4));        /* (DTN: still, glass) */
        rnd_tp(tr, P_E0 + 4u, role == RND_ROLE_CANON ? 0 : rnd_pct(65) ? 2 : 1);   /* HP or BP (the canon LP) */
        rnd_tp(tr, P_E0 + 5u, role == RND_ROLE_CANON ? rnd_rr(60, 85) : rnd_rr(30, 60));   /* (HP's cut: thin) */
        rnd_tp(tr, P_E0 + 6u, rnd_rr(10, 50));
        rnd_tp(tr, P_E0 + 7u, rnd_rr(10, 40));      /* PW: narrow */
    }
    if (role == RND_ROLE_CHORDS) {
        /* (slower: a chord gone before it swells, measured) */
        {
            static const int8_t rg[][3] = {{P_ATK, 10, 40}, {P_DEC, 80, 110}, {P_SUS, 60, 100}, {P_REL, 70, 100}};
            rnd_tp_rr(tr, rg, NELEM(rg));           /* (glass: it speaks at once, not HALO's swell) */
        }
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(56, 70));
        rnd_tp(tr, P_PAN, 0);
    } else if (role == RND_ROLE_MELODY) {
        static const uint8_t vv[] = {V_MONO, V_LEGATO, V_POLY}, vw[] = {4, 2, 3};
        {
            static const int8_t rg[][3] = {{P_ATK, 0, 4}, {P_DEC, 50, 85}, {P_SUS, 0, 40}, {P_REL, 40, 80}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, rnd_pick(vv, vw, NELEM(vv)));
        rnd_tp(tr, P_LEVEL, rnd_rr(76, 90));
        rnd_tp(tr, P_PAN, rnd_rr(-25, 25));
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, rnd_rr(-25, 0));   /* (MLVL: the tines' clang kept in) */
    } else if (role == RND_ROLE_TINT) {
        {
            static const int8_t rg[][3] = {{P_ATK, 0, 3}, {P_DEC, 70, 110}, {P_SUS, 0, 20}, {P_REL, 60, 95}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(64, 80));
        rnd_tp(tr, P_PAN, rnd_pct(50) ? rnd_rr(30, 60) : -rnd_rr(30, 60));
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 7u, rnd_rr(0, 14));
        if (e == RND_PHYS) { rnd_tp(tr, P_E0 + 6u, 0); rnd_tp(tr, P_LEVEL, rnd_rr(84, 98)); }
    } else {
        {
            static const int8_t rg[][3] = {{P_ATK, 0, 6}, {P_DEC, 60, 95}, {P_SUS, 20, 60}, {P_REL, 50, 85}};
            rnd_tp_rr(tr, rg, NELEM(rg));           /* (struck, not held) */
        }
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_LEVEL, rnd_rr(68, 82));
        rnd_tp(tr, P_PAN, rnd_rr(-15, 15));
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 7u, rnd_rr(10, 24));
    }
    if (e == 3u && trk[tr].p[P_E0 + 1u] == 3) rnd_tp(tr, P_E0 + 1u, 2);   /* (SID's NOISE wave pushed in: PULSE) */
    if (e == 3u || e == ENGI_ORACLE)                /* (SID, ORACLE: quieter than the others, measured: up) */
        rnd_tp(tr, P_LEVEL, (int32_t)trk[tr].p[P_LEVEL] + (e == 3u ? 22 : 12) > 118 ? 118 :
                            (int32_t)trk[tr].p[P_LEVEL] + (e == 3u ? 22 : 12));
    rnd_mc_level(tr);
}
/* the canon's division from the line's: half as fast, or across it (3:2 in triplets); "cross" 0 or 1 */
static int32_t rnd_mr_canon_div(int32_t d, int cross)
{
    switch (d) {
    case 2: return cross ? 4 : 1;                   /* 1/16: 1/8, or 8T (4:3 against it) */
    case 1: return cross ? 4 : 0;                   /* 1/8: 1/4, or 8T (3:2) */
    case 4: return cross ? 1 : 0;                   /* 8T: 1/4, or 1/8 (2:3) */
    case 0: return cross ? 4 : 6;                   /* 1/4: 1/2, or 8T (3:1 over it) */
    default: return 7;                              /* 1/2: 1/1 */
    }
}
static int rnd_mr_heavy(uint32_t tr)                /* (FM6, PHYS, ORACLE: never on a fast clock, rnd_cpu) */
{
    uint32_t e = trk[tr].eng_req;
    return e == RND_FM6 || e == RND_PHYS || e == ENGI_ORACLE;
}
/* the mirror's way of following the line: onto the halo's chord, or in parallel SHIFT degrees off it */
static void rnd_mr_tint_mode(uint32_t tr, int chord)
{
    static const uint8_t sh[] = {26, 28, 22, 21, 27, 29}, shw[] = {4, 3, 3, 2, 2, 1};   /* (+24: +2 +4 -2 -3 +3 +5) */
    if (chord && !rnd_mr_micro()) {
        rnd_tp(tr, P_QUANT, Q_CHRD);
        rnd_tx(tr, TX_TW_SHIFT, 0);
    } else {
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
        rnd_tx(tr, TX_TW_SHIFT, (int32_t)rnd_pick(sh, shw, NELEM(sh)) - 24);
    }
}
static void rnd_mirror_generators(const uint8_t *role)
{
    static const uint8_t dv[] = {1, 0, 4, 2}, dw[] = {40, 30, 15, 15};   /* 1/8 1/4 8T 1/16 */
    uint32_t tr, chords = 1, k;
    int32_t mdiv = 0;
    for (tr = 0, rnd_hl_m = RND_NT; tr < RND_NT; tr++) if (role[tr] == RND_ROLE_MELODY) rnd_hl_m = (uint8_t)tr;
    for (tr = 0; tr < RND_NT; tr++) {       /* (the clean slate, as HALO's) */
        if (role[tr] == RND_ROLE_CHORDS && !rnd_mr_micro()) continue;
        rnd_generator(tr, &chords);
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SLEN, 16);
    }
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] != RND_ROLE_CHORDS) continue;
        if (!rnd_mr_micro()) {                      /* the halo: chords, held or pulsing */
            static const uint8_t pg[] = {5, 4, 3, 6, 1}, pgw[] = {3, 3, 1, 1, 1};   /* DRONE TWO CIRCLE WALK 50S */
            static const uint8_t ht[] = {0, 1, 3}, htw[] = {6, 3, 1};               /* HOLD BEAT OFFBEAT */
            rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
            rnd_tx(tr, TX_CH_PROG, rnd_pick(pg, pgw, NELEM(pg)));
            rnd_tx(tr, TX_CH_BARS, rnd_rr(1, 3));
            rnd_tx(tr, TX_CH_HIT, rnd_pick(ht, htw, NELEM(ht)));
            rnd_tx(tr, TX_CH_OCT, 0);
            rnd_tx(tr, TX_CH_BREATH, rnd_rr(0, 40));
            rnd_tp(tr, P_SDIV, 0);
            if (pid_get(PID_TESS(TX_CH_HIT), tr)) { /* (pulsing: struck, short, as glass) */
                rnd_tp(tr, P_SGATE, rnd_rr(30, 80));
                rnd_tp(tr, P_ATK, rnd_rr(0, 20));
                rnd_tp(tr, P_REL, rnd_rr(50, 80));
            } else rnd_tp(tr, P_SGATE, 127);
        } else {                                    /* off twelve notes: a cluster, held, its notes taking turns */
            rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
            rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
            rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 3));
            rnd_tx(tr, TX_SR_LOOP, rnd_rr(2, 3));
            rnd_tp(tr, P_SDIV, rnd_pct(50) ? 7 : 8);   /* 1/1, 2BAR */
            rnd_tp(tr, P_SGATE, 127);
        }
        rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tx(tr, TX_SUB_HUMAN, 0);
    }
    if (rnd_hl_m < RND_NT) {                          /* the line */
        tr = rnd_hl_m;
        k = (uint32_t)rnd_rr(0, 99);
        if (k < 35u) {
            rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
            rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
            rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 2));
            rnd_tx(tr, TX_SR_LOOP, rnd_pct(50) ? 0 : rnd_rr(5, 16));
        } else if (k < 60u) {
            rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
            rnd_tx(tr, TX_TUR_LEN, rnd_rr(5, 12));
            rnd_tx(tr, TX_TUR_PROB, rnd_rr(5, 30));
            rnd_tx(tr, TX_TUR_SPAN, rnd_rr(1, 2));
        } else if (k < 75u) {
            rnd_tx(tr, TX_MODE, TESS_ENG_BOUNCE);
            rnd_tx(tr, TX_BNC_BALLS, rnd_rr(3, 7));
            rnd_tx(tr, TX_BNC_SPEED, rnd_rr(10, 40));
            rnd_tx(tr, TX_BNC_ANGLE, rnd_rr(20, 70));
        } else if (k < 90u) {
            rnd_tx(tr, TX_MODE, TESS_ENG_MARKOV);
            rnd_tx(tr, TX_MKV_LEAP, rnd_rr(20, 70));
            rnd_tx(tr, TX_MKV_DENS, rnd_rr(50, 90));
            rnd_tx(tr, TX_MKV_SPAN, rnd_rr(1, 2));
        } else {
            rnd_tx(tr, TX_MODE, TESS_ENG_AWAKE);
            rnd_tx(tr, TX_AWK_LENA, rnd_rr(3, 8));
            rnd_tx(tr, TX_AWK_LENB, rnd_rr(3, 7));
            rnd_tx(tr, TX_AWK_PROB, rnd_rr(50, 90));
        }
        mdiv = (int32_t)rnd_pick(dv, dw, NELEM(dv));
        for (k = 0; k < RND_NT; k++)                  /* (FM6 PHYS ORACLE never fast, rnd_cpu: the line, or the mirror
                                                     * on its clock, one of them: 1/8) */
            if ((k == tr || role[k] == RND_ROLE_TINT) && rnd_mr_heavy(k) && (mdiv == 2 || mdiv == 4)) mdiv = 1;
        rnd_tp(tr, P_SDIV, mdiv);
        rnd_tp(tr, P_SGATE, rnd_rr(30, 100));
        rnd_tx(tr, TX_TW_DROP, rnd_rr(0, 35));
        rnd_tx(tr, TX_EV_EVERY, rnd_pct(50) ? rnd_rr(3, 5) : 0);
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(15, 40));
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 30));
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 10));    /* (tight: digital) */
    }
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] != RND_ROLE_TINT && role[tr] != RND_ROLE_CANON) continue;
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);       /* (one note a step: the pitch is the line's) */
        rnd_tx(tr, TX_SR_START, 0);
        rnd_tx(tr, TX_SR_INTV, 1);
        rnd_tx(tr, TX_SR_LOOP, 1);
        rnd_tx(tr, TX_TW_FOLLOW, rnd_hl_m < RND_NT ? (int32_t)rnd_hl_m + 1 : 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 10));
        if (role[tr] == RND_ROLE_TINT) {            /* on the line's clock, or twice as fast (light engines) */
            int32_t d = mdiv;
            if (rnd_pct(25) && !rnd_mr_heavy(tr) && (mdiv == 1 || mdiv == 0)) d = mdiv == 1 ? 2 : 1;
            rnd_tp(tr, P_SDIV, d);
            rnd_tp(tr, P_SGATE, rnd_rr(30, 90));
            rnd_tp(tr, P_VOICE, d == 2 || d == 4 ? V_MONO : V_POLY);
            rnd_tx(tr, TX_TW_DROP, rnd_rr(0, 40));
            rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 30));
            if ((d == 0 || d == 6) && rnd_pct(30)) {   /* (on a slow clock: now and then a stutter) */
                rnd_tx(tr, TX_SUB_RATCH, rnd_rr(1, 2));
                rnd_tx(tr, TX_SUB_CHANCE, rnd_rr(20, 50));
                rnd_tx(tr, TX_SUB_SHAPE, 0);
            }
            rnd_mr_tint_mode(tr, rnd_pct(65));
        } else {                                    /* half as fast, or across in triplets */
            int32_t d = rnd_mr_canon_div(mdiv, rnd_pct(35));
            if (rnd_mr_heavy(tr) && (d == 2 || d == 4)) d = rnd_mr_canon_div(mdiv, 0);
            rnd_tp(tr, P_SDIV, d);
            rnd_tp(tr, P_SGATE, 127);
            rnd_tx(tr, TX_TW_DROP, 0);
            rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 20));
        }
    }
}
/* the registers: as HALO's (a follower's TRANS from the line's note): the halo in the middle or below, the line in the
 * middle or above, the mirror an octave over it or under (beside a high line), the canon an octave under; the tempo */
static void rnd_mirror_registers(const uint8_t *role)
{
    int32_t m = rnd_pct(50) ? 0 : 12, halo = m - 12, t = 12;   /* (each its own octave, 2026-11-09; the mirror's and
                                                     * the canon's TRANS from the line's notes: they FOLLOW it) */
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++)
        rnd_tp(tr, P_TRANS, role[tr] == RND_ROLE_CHORDS ? halo : role[tr] == RND_ROLE_MELODY ? m :
                            role[tr] == RND_ROLE_TINT ? t : -24);
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(100, 120));   /* (100 to 120, asked 2026-11-09: was 70 to 100) */
}
/* the key: twelve notes (LYD WHOLE PEN MAJ DOR DIMHW), or off them (SLENDRO PELOG 19EDO HARM16 JI-MAJ 22EDO BP-ET) */
static void rnd_key_mirror(void)
{
    static const uint8_t sc[] = {9, 13, 5, 1, 3, 14}, sw[] = {4, 2, 2, 2, 1, 1};
    static const uint8_t mc[] = {60, 61, 26, 46, 40, 29, 62}, mw[] = {3, 3, 2, 2, 2, 1, 1};
    int32_t scale = rnd_pct(60) ? (int32_t)rnd_pick(sc, sw, NELEM(sc)) : (int32_t)rnd_pick(mc, mw, NELEM(mc));
    int32_t root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
/* MIRROR's cords: S&H on the line's shape (digital steps), an LFO on the bells' place, slow ones on the halo and the
 * canon's colour */
static void rnd_cords_mirror(const uint8_t *role)
{
    uint32_t i = 0, tr;
    cord_t c;
    for (tr = 0; tr < RND_NT && i < CORD_LIVE; tr++) {
        memset(&c, 0, sizeof c);
        c.src_trk = c.dst_trk = (uint8_t)tr;
        if (role[tr] == RND_ROLE_MELODY) c.src = CS_SH, c.dst = CD_SHAPE;
        else if (role[tr] == RND_ROLE_TINT) c.src = CS_LFO, c.dst = CD_PAN;
        else c.src = rnd_pct(50) ? CS_DRIFT : CS_INERTIA, c.dst = CD_CUTOFF;
        c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, 35) : -rnd_rr(10, 35));
        if (cord_set(i, &c)) i++;
    }
    for (memset(&c, 0, sizeof c); i < CORD_LIVE; i++) cord_set(i, &c);
}
static void rnd_roles_mirror(uint8_t *role)
{
    rnd_roles_halo(role);                           /* (the same four, in a random order) */
}

/* ------------------------------------------------------------ TIDE --- */
/* TIDE: slow (64 to 84 BPM), always the same four, on the same tracks (to mix and mute by heart): track 1 the SUB
 * GROOVE (RND_ROLE_LOWPERC): a synthesized sub kick, a soft low boom (TRIO's triangles filtered to near a sine, the
 * pitch falling a little or not, a long decay), its pitch the bed's lowest note two octaves down (FOLLOW), its rhythm across the
 * bar mostly (EUCLID on 5, 7, 9, 10, 11 or 13 steps, or 12 in threes; 8T now and then) and now and then four on the
 * floor; track 2 the MELODY: FALL's leaves (BOUNCE) or now and then a line by step (SERIES, HALO's), half the rolls
 * a lead (a saw, pulse or triangle filtered low), legato where its notes come close (they tie and glide), apart after
 * its gaps; half soft and struck; dark both, onto the
 * chord's notes now and then (QNT CHRD), in the middle or an octave up; track 3 the BED (RND_ROLE_CHORDS): TRIO's
 * triangles holding the chords (CHORD: DRONE, TWO, CIRCLE or WALK), in the middle; track 4 the TEXTURE: TRIO's
 * noise mostly (or a pulse with noise, a saw, a triangle) through its low-pass or band-pass, low, held, soft, its
 * colour, place and LFO pace on slow random walks. The melody and the noise move a lot and wander the stereo (the LFO
 * on the filter and the place, slow chaos, the note's height on the place); DRIFT moves the groove (another rhythm
 * across, or four on the floor, or back), the melody's pace, and the key to its relative and home (rnd_tide_move) */
#define RND_TD_KICK 0u
#define RND_TD_MEL 1u
#define RND_TD_BED 2u
#define RND_TD_NOISE 3u
static uint8_t rnd_td_lead;                        /* TIDE: 1: the melody a filtered lead, legato in places (else
                                                     * struck leaves) */
/* a lead's notes held two steps (FALL's leaves, landing apart: four) (NOTE LENGTH FIXED, one key): the next note
 * within them ties on, gliding (LEGATO); after a gap or a rest, a note of its own. Runs, then a breath: measured, a
 * line by step about four notes in five tied (FALL's leaves a fifth or so) */
static void rnd_tide_nl(uint32_t tr)
{
    int32_t d = trk[tr].p[P_SDIV];
    if (!rnd_td_lead) { rnd_tx(tr, TX_NL_MODE, 0); return; }
    rnd_tx(tr, TX_NL_MODE, 1);                      /* FIXED */
    if (tessera.mode[tr] == TESS_ENG_BOUNCE)        /* (the leaves land apart: held four steps, so some still tie) */
        rnd_tx(tr, TX_NL_LEN, d == 1 ? 4 : 5);      /* (1/8 -> 1/2, 1/4 -> 1/1) */
    else rnd_tx(tr, TX_NL_LEN, d == 1 ? 3 : d == 0 ? 4 : 5);   /* (two steps: 1/8 -> 1/4, 1/4 -> 1/2, 1/2 -> 1/1) */
    rnd_tx(tr, TX_NL_KEYS, 1);
}
static void rnd_tide_tri(uint32_t tr, int pure)     /* TRIO's three triangles: pure (one), or FALL's */
{
    rnd_tp(tr, P_E0, 4);                            /* WAVE TRI3 */
    rnd_tp(tr, P_E0 + 1u, pure ? 0 : rnd_pct(60) ? 12 : 7);   /* INT2 */
    rnd_tp(tr, P_E0 + 2u, pure ? 0 : rnd_pct(60) ? -12 : 0);  /* INT3 */
    rnd_tp(tr, P_E0 + 3u, pure ? 0 : rnd_rr(4, 16)); /* DTN, cents */
    rnd_tp(tr, P_E0 + 4u, 0);                       /* MODE LP */
}
static void rnd_tide_sound(uint32_t tr, uint32_t role, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    uint8_t nice = rnd_nice;
    uint32_t e;
    int fmkick = 0;                                 /* (FM6's FM BASS, SOLID BASS, DROP: held notes, a sub bass more than
                                                     * a kick, measured: TRIO's alone) */
    rnd_want_e = fmkick ? ENGI_FM6 : 6u;
    rnd_want_pi = fmkick ? (rnd_pct(40) ? 2 : rnd_pct(50) ? 10 : 28) :   /* (FM BASS, SOLID BASS, DROP: the low ones) */
                  role == RND_ROLE_MELODY ? (rnd_pct(50) ? 8 : 10) : role == RND_ROLE_CHORDS ? 5 : 8;   /* (SOFT PAD,
                                                     * SOFT LEAD; WARM PAD; the rest made below) */
    if (role == RND_ROLE_MELODY && eng_ok(ENGI_ORACLE) && rnd_pct(75)) {   /* (the melody ORACLE's mostly, asked
                                                     * 2026-11-08: its leads, keys, flute and voice; legato, dark) */
        static const uint8_t op[] = {17, 21, 5, 11, 18}, ow[] = {3, 3, 2, 2, 2};   /* SQUARE LEAD FLUTE SYNC LEAD KEYS VOX */
        rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pick(op, ow, NELEM(op));
    }
    if (role == RND_ROLE_NOISE && rnd_mc_ok() && rnd_pct(50))   /* (the texture on MACRO half the time: WIND, DUST,
                                                     * TWIN PEAKS) */
        rnd_want_e = ENGI_MACRO, rnd_want_pi = rnd_pct(40) ? 10 : rnd_pct(50) ? 14 : 11;
    rnd_nice = 1;
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    rnd_mc_model(tr);
    e = trk[tr].eng_req;
    rnd_tp(tr, P_ED_SHP, 0);
    rnd_tp(tr, P_LD_PIT, 0);
    rnd_tp(tr, P_LD_SHP, 0);
    rnd_tp(tr, P_LD_AMP, 0);
    rnd_tp(tr, P_LD_FLT, 0);
    rnd_tp(tr, P_LWAVE, 0);
    rnd_tp(tr, P_LSYNC, 0);
    rnd_tp(tr, P_LTRIG, 1);                         /* FREE: the slow wander runs on (NOTE started it again at each
                                                     * phrase, the note before still ringing: its filter jumped, a click,
                                                     * heard and measured) */
    rnd_tp(tr, P_LFADE, 0);
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_CHRD, 0);
    if (role == RND_ROLE_LOWPERC) {                 /* the sub: a soft low boom, near a sine (a punchy thump on
                                                     * every roll was too much, heard): a breath of attack, the pitch
                                                     * falling a little or not, a long decay */
        int boom = rnd_pct(70);
        if (e == 6u) {
            rnd_tide_tri(tr, 1);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(10, 26));  /* CUT: the triangle's overtones gone: a sine, near enough */
            rnd_tp(tr, P_E0 + 6u, 0);
            rnd_tp(tr, P_ED_PIT, boom ? rnd_rr(0, 8) : rnd_rr(8, 16));   /* (a boom: barely a fall; a soft thump) */
            rnd_tp(tr, P_ED_FLT, 0);
        } else rnd_tp(tr, P_ED_PIT, 0);
        rnd_tp(tr, P_ATK, rnd_rr(3, 12));           /* (no click) */
        rnd_tp(tr, P_DEC, boom ? rnd_rr(70, 100) : rnd_rr(55, 75));
        rnd_tp(tr, P_SUS, boom ? rnd_rr(0, 30) : 0);
        rnd_tp(tr, P_REL, boom ? rnd_rr(55, 85) : rnd_rr(40, 60));
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_LEVEL, rnd_rr(96, 108));
        rnd_tp(tr, P_PAN, 0);
        rnd_tp(tr, P_DETUNE, 0);
        rnd_tp(tr, P_SPRD, 0);
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 7u, rnd_rr(0, 12));
        return;
    }
    rnd_tp(tr, P_ED_PIT, 0);
    rnd_tp(tr, P_ED_FLT, rnd_rr(0, 15));
    if (role == RND_ROLE_MELODY) {                  /* half the rolls a legato lead, filtered; half soft leaves,
                                                     * struck; dark both (bright before, heard); the filter and the
                                                     * place moving */
        static const uint8_t wv[] = {4, 5, 2}, ww[] = {4, 3, 1};   /* TRI3, S&T, PPT */
        static const uint8_t lv[] = {0, 1, 4, 5}, lw[] = {4, 3, 2, 2};   /* SAW3, PLS3, TRI3, S&T */
        rnd_td_lead = (uint8_t)rnd_pct(50);
        if (e == 6u) {
            rnd_tp(tr, P_E0, rnd_td_lead ? rnd_pick(lv, lw, NELEM(lv)) : rnd_pick(wv, ww, NELEM(wv)));
            rnd_tp(tr, P_E0 + 1u, rnd_pct(60) ? 12 : 0);
            rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : 7);
            rnd_tp(tr, P_E0 + 3u, rnd_rr(4, 14));
            rnd_tp(tr, P_E0 + 4u, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_td_lead ? rnd_rr(30, 52) : rnd_rr(40, 62));   /* CUT: low */
            rnd_tp(tr, P_E0 + 6u, rnd_td_lead ? rnd_rr(20, 55) : rnd_rr(0, 25));    /* RES: a lead's a little vocal */
        } else if (e == ENGI_ORACLE) {              /* (ORACLE: its own envelopes, made as the lead's or the leaves';
                                                     * darker than the preset) */
            uint8_t *q = or_patch_of(&trk[tr]);
            q[OR_ATTACK_AMP] = (uint8_t)(rnd_td_lead ? rnd_rr(30, 60) : rnd_rr(0, 8));
            q[OR_DECAY_AMP] = (uint8_t)(rnd_td_lead ? rnd_rr(70, 100) : rnd_rr(45, 75));
            q[OR_SUSTAIN_AMP] = (uint8_t)(rnd_td_lead ? rnd_rr(80, 115) : rnd_rr(10, 50));
            q[OR_RELEASE_AMP] = (uint8_t)rnd_rr(60, 90);
            q[OR_RELEASE_ON] = 1;
            rnd_tp(tr, P_E0, trk[tr].p[P_E0] - rnd_rr(5, 20));   /* CUT */
        }
        if (rnd_td_lead) {                          /* the lead: swelling, gliding where notes tie (rnd_tide_nl), its
                                                     * filter opening a little */
            rnd_tp(tr, P_ATK, rnd_rr(45, 75));      /* (a phrase's first note swells in: a tied one keeps the swell's
                                                     * level, LEGATO) */
            rnd_tp(tr, P_DEC, rnd_rr(80, 110));
            rnd_tp(tr, P_SUS, rnd_rr(80, 115));
            rnd_tp(tr, P_REL, rnd_rr(70, 100));
            rnd_tp(tr, P_VOICE, V_LEGATO);
            rnd_tp(tr, P_GLIDE, rnd_rr(15, 45));
            rnd_tp(tr, P_ED_FLT, rnd_rr(10, 30));
        } else {
            {
                static const int8_t rg[][3] = {{P_ATK, 0, 12}, {P_DEC, 55, 95}, {P_SUS, 10, 50}, {P_REL, 70, 105}};
                rnd_tp_rr(tr, rg, NELEM(rg));
            }
            rnd_tp(tr, P_VOICE, V_POLY);
        }
        rnd_tp(tr, P_LEVEL, rnd_td_lead ? rnd_rr(74, 86) : rnd_rr(86, 98));   /* (the voice over the bed, not on top of
                                                     * it: 5 dB down, heard 2026-11-08; a lead's saw louder than the
                                                     * leaves, measured) */
        rnd_tp(tr, P_LRATE, rnd_rr(30, 60));        /* (0.2 to 0.8 Hz) */
        rnd_tp(tr, P_LD_FLT, rnd_rr(15, 40));
        rnd_tp(tr, P_LWAVE, rnd_pct(70) ? 0 : 1);   /* (sine or triangle) */
        rnd_tp(tr, P_DETUNE, rnd_rr(20, 60));
        rnd_tp(tr, P_SPRD, rnd_rr(60, 127));
        rnd_tp(tr, P_PAN, 0);
    } else if (role == RND_ROLE_CHORDS) {           /* the bed: triangles, held, warm */
        if (e == 6u) {
            rnd_tide_tri(tr, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(45, 75));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 20));
        }
        rnd_tp(tr, P_ATK, rnd_rr(70, 110));
        rnd_tp(tr, P_DEC, 127);
        rnd_tp(tr, P_SUS, rnd_rr(105, 127));
        rnd_tp(tr, P_REL, rnd_rr(95, 120));
        rnd_tp(tr, P_VOICE, V_POLY);
        /* (under the melody) */
        {
            static const int8_t rg[][3] = {{P_LEVEL, 52, 64}, {P_LRATE, 15, 35}, {P_LD_FLT, 5, 15}, {P_DETUNE, 30, 80},
                                           {P_SPRD, 40, 90}};
            rnd_tp_rr(tr, rg, NELEM(rg));
        }
        rnd_tp(tr, P_PAN, 0);
    } else {                                        /* the texture: noise mostly, or a pulse with noise, a saw, a
                                                     * triangle; low-pass or band-pass, low; held, soft, wandering
                                                     * slowly at random (softer and slower than before, heard) */
        static const uint8_t wv[] = {8, 7, 0, 4, 5}, ww[] = {10, 4, 2, 2, 2};   /* NOIS, P+N, SAW3, TRI3, S&T */
        int bp = rnd_pct(50);
        if (e == 6u) {
            rnd_tp(tr, P_E0, rnd_pick(wv, ww, NELEM(wv)));
            rnd_tp(tr, P_E0 + 1u, rnd_pct(50) ? 0 : 12);   /* INT2, INT3: the tones in octaves */
            rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : -12);
            rnd_tp(tr, P_E0 + 3u, rnd_rr(5, 25));   /* DTN: a tone's slow beating */
            rnd_tp(tr, P_E0 + 4u, bp ? 1 : 2);      /* MODE BP or HP (2026-11-09: up, over the bed and the melody) */
            rnd_tp(tr, P_E0 + 5u, bp ? rnd_rr(72, 92) : rnd_rr(80, 100));   /* CUT: high, air */
            rnd_tp(tr, P_E0 + 6u, bp ? rnd_rr(20, 40) : rnd_rr(0, 20));    /* RES: a band-pass sings a little */
        } else if (e == ENGI_MACRO) {               /* (MACRO: dark, soft) */
            rnd_tp(tr, P_E0 + 3u, rnd_rr(80, 105)); /* TONE (80 Hz x 2^(TONE/16): 2.6 to 7 kHz) */
            rnd_tp(tr, P_E0 + 7u, 0);
        }
        rnd_tp(tr, P_ATK, rnd_rr(90, 118));
        rnd_tp(tr, P_DEC, 127);
        rnd_tp(tr, P_SUS, 127);
        rnd_tp(tr, P_REL, rnd_rr(90, 115));
        rnd_tp(tr, P_VOICE, V_LEGATO);
        rnd_tp(tr, P_LEVEL, rnd_rr(54, 68));
        rnd_mc_level(tr);
        if (e == 6u)                                /* (TRIO's noise -33 to -29 dB on the level, asked 2026-11-09: at
                                                     * -20 too loud in the mix, heard; its tones, quieter, -30 to -27;
                                                     * MACRO's noises fine as they were) */
            rnd_tp(tr, P_LEVEL, trk[tr].p[P_E0] == 7 || trk[tr].p[P_E0] == 8 ? rnd_rr(46, 54) : rnd_rr(52, 58));
        rnd_tp(tr, P_LWAVE, rnd_pct(50) ? 4 : 0);   /* (S&H: a random step now and then; or a sine) */
        rnd_tp(tr, P_LRATE, rnd_rr(5, 28));         /* (slow: 0.05 to 0.2 Hz) */
        rnd_tp(tr, P_LD_FLT, rnd_rr(10, 30));
        rnd_tp(tr, P_DETUNE, 0);
        rnd_tp(tr, P_SPRD, rnd_rr(90, 127));
        rnd_tp(tr, P_PAN, 0);
    }
}
/* the groove: across the bar mostly (EUCLID on odd lengths; 12 in threes; on 8T now and then), four on the floor now
 * and then */
static void rnd_tide_groove(uint32_t tr, int four)
{
    static const uint8_t ln[] = {5, 7, 9, 10, 11, 13, 12}, lw[] = {3, 4, 2, 2, 2, 1, 2};
    int32_t st = (int32_t)rnd_pick(ln, lw, NELEM(ln));
    rnd_tx(tr, TX_MODE, TESS_ENG_EUCLID);
    if (four) {
        rnd_tx(tr, TX_EUC_STEPS, 4);
        rnd_tx(tr, TX_EUC_PULSES, 4);
        rnd_tx(tr, TX_EUC_ROT, 0);
        rnd_tp(tr, P_SDIV, 0);                      /* 1/4 */
        return;
    }
    rnd_tx(tr, TX_EUC_STEPS, st);
    rnd_tx(tr, TX_EUC_PULSES, st == 12 ? 4 : rnd_rr(2, st >= 9 ? 4 : 3));   /* (12 in threes: dotted quarters) */
    rnd_tx(tr, TX_EUC_ROT, st == 12 ? 0 : rnd_rr(0, st - 1));
    rnd_tp(tr, P_SDIV, rnd_pct(20) ? 4 : 1);        /* 8T or 1/8 */
}
static void rnd_tide_generators(void)
{
    static const uint8_t pg[] = {5, 4, 3, 6}, pgw[] = {3, 4, 1, 2};   /* DRONE TWO CIRCLE WALK */
    uint32_t tr, chords = 1;
    for (tr = 0; tr < RND_NT; tr++) {
        if (tr == RND_TD_BED) continue;
        rnd_generator(tr, &chords);
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_EV_EVERY, 0);
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SLEN, 16);
    }
    tr = RND_TD_BED;                                /* the bed: the chords, held */
    rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
    rnd_tx(tr, TX_CH_PROG, rnd_pick(pg, pgw, NELEM(pg)));
    rnd_tx(tr, TX_CH_BARS, rnd_pct(60) ? 3 : 2);    /* (4 or 2 bars a chord) */
    rnd_tx(tr, TX_CH_HIT, 0);
    rnd_tx(tr, TX_CH_OCT, 0);
    rnd_tx(tr, TX_CH_BREATH, rnd_rr(0, 30));
    rnd_tx(tr, TX_TW_DROP, 0);
    rnd_tx(tr, TX_EV_EVERY, 0);
    rnd_tx(tr, TX_SUB_HUMAN, 0);
    rnd_tp(tr, P_SDIV, 0);
    rnd_tp(tr, P_SGATE, 127);
    tr = RND_TD_KICK;                               /* the sub groove: on the bed's lowest note */
    rnd_tide_groove(tr, rnd_pct(15));
    rnd_tx(tr, TX_TW_FOLLOW, (int32_t)RND_TD_BED + 1);
    rnd_tx(tr, TX_TW_DROP, rnd_pct(40) ? rnd_rr(5, 20) : 0);
    rnd_tx(tr, TX_TW_HUMAN, rnd_rr(10, 40));
    rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 20));
    rnd_tp(tr, P_SGATE, rnd_rr(50, 90));            /* (the boom let ring) */
    tr = RND_TD_MEL;                                /* the melody: FALL's leaves, or a line by step (a lead: runs by
                                                     * step mostly) */
    if (rnd_pct(rnd_td_lead ? 30 : 75)) {
        rnd_tx(tr, TX_MODE, TESS_ENG_BOUNCE);
        rnd_tx(tr, TX_BNC_BALLS, 5);
        rnd_tx(tr, TX_BNC_SPEED, rnd_rr(9, 24));   /* (not FALL's slowest: on 1/4 its first leaf took 20 s and
                                                     * more, measured) */
        rnd_tx(tr, TX_BNC_ANGLE, rnd_pct(60) ? rnd_rr(35, 55) : rnd_rr(15, 75));
        rnd_tp(tr, P_SDIV, rnd_pct(60) ? 1 : 0);    /* 1/8 or 1/4 */
    } else {
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
        rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
        rnd_tx(tr, TX_SR_INTV, 1);
        rnd_tx(tr, TX_SR_LOOP, rnd_pct(50) ? 0 : rnd_rr(6, 16));
        rnd_tp(tr, P_SDIV, rnd_td_lead ? (rnd_pct(60) ? 1 : 0) : rnd_pct(60) ? 0 : 6);   /* (a lead's runs: 1/8 or 1/4;
                                                     * else 1/4 or 1/2) */
    }
    rnd_tp(tr, P_SGATE, rnd_rr(60, 110));
    rnd_tx(tr, TX_TW_DROP, !rnd_td_lead ? (rnd_pct(50) ? rnd_rr(5, 30) : 0) :   /* (a lead's gaps: where its legato
                                                     * breaks; a line by step, every note there, more of them) */
                           tessera.mode[tr] == TESS_ENG_BOUNCE ? rnd_rr(15, 30) : rnd_rr(30, 45));   /* (runs of a
                                                     * few notes tied, then a breath: two rests running) */
    rnd_tx(tr, TX_TW_HUMAN, rnd_rr(10, 40));
    rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(5, 30));
    rnd_tx(tr, TX_EV_EVERY, rnd_pct(50) ? rnd_rr(4, 5) : 0);
    rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 25));
    if (rnd_pct(40)) rnd_tp(tr, P_QUANT, Q_CHRD);   /* (onto the chord's notes now and then) */
    rnd_tide_nl(tr);
    tr = RND_TD_NOISE;                              /* the noise: one note, held */
    rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
    rnd_tx(tr, TX_SR_START, 0);
    rnd_tx(tr, TX_SR_INTV, 1);
    rnd_tx(tr, TX_SR_LOOP, 1);
    rnd_tp(tr, P_SDIV, 9);                          /* 4BAR */
    rnd_tp(tr, P_SGATE, 127);
    rnd_tx(tr, TX_TW_DROP, 0);
    rnd_tx(tr, TX_SUB_HUMAN, 0);
}
/* the registers: the sub two octaves under the bed's lowest note (the bed in the middle), the melody an octave up, the
 * noise where it is; the tempo: slow */
static void rnd_tide_registers(void)
{
    rnd_tp(RND_TD_KICK, P_TRANS, -24);              /* (FOLLOW: from the bed's note: C1 to B1) */
    rnd_tp(RND_TD_MEL, P_TRANS, 12);                /* (over the bed, 2026-11-09: they shared 200-500 Hz, mud) */
    rnd_tp(RND_TD_BED, P_TRANS, 0);
    rnd_tp(RND_TD_NOISE, P_TRANS, 24);              /* (the texture over them all) */
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(64, 84));
}
static void rnd_key_tide(void)
{
    static const uint8_t sc[] = {2, 3, 6, 5, 1, 9, 4}, sw[] = {4, 3, 2, 2, 1, 1, 1};   /* MIN DOR MPEN PEN MAJ LYD MIX */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
/* TIDE's cords: the melody's place on a slow LFO and on the note's height, its colour wandering; the noise's place on
 * slow chaos, its colour on another; the bed's colour slowly */
static void rnd_cords_tide(void)
{
    static const uint8_t chaos[] = {CS_OCHD1, CS_OCHD3, CS_DRIFT, CS_TORPOR};
    cord_t c[8];
    uint32_t i, n = 0;
    memset(c, 0, sizeof c);
    c[n].src_trk = c[n].dst_trk = RND_TD_MEL; c[n].src = CS_LFO; c[n].dst = CD_PAN; c[n].amount = (int8_t)rnd_rr(35, 70); n++;
    c[n].src_trk = c[n].dst_trk = RND_TD_MEL; c[n].src = CS_NOTE; c[n].dst = CD_PAN;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(15, 35) : -rnd_rr(15, 35)); n++;
    c[n].src_trk = c[n].dst_trk = RND_TD_MEL; c[n].src = CS_DRIFT; c[n].dst = CD_CUTOFF;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(15, 35) : -rnd_rr(15, 35)); n++;
    c[n].src_trk = c[n].dst_trk = RND_TD_NOISE; c[n].src = chaos[rnd_next() % NELEM(chaos)]; c[n].dst = CD_PAN;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(45, 80) : -rnd_rr(45, 80)); n++;
    c[n].src_trk = c[n].dst_trk = RND_TD_NOISE; c[n].src = rnd_pct(50) ? CS_INERTIA : CS_DRIFT; c[n].dst = CD_CUTOFF;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(20, 40) : -rnd_rr(20, 40)); n++;   /* (random walks: the colour) */
    c[n].src_trk = c[n].dst_trk = RND_TD_NOISE; c[n].src = CS_APATHY; c[n].dst = CD_LFO_RATE;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, 25) : -rnd_rr(10, 25)); n++;   /* (the LFO's own pace wandering) */
    c[n].src_trk = c[n].dst_trk = RND_TD_BED; c[n].src = CS_DRIFT; c[n].dst = CD_CUTOFF;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, 25) : -rnd_rr(10, 25)); n++;
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t z;
        memset(&z, 0, sizeof z);
        cord_set(i, i < n ? &c[i] : &z);
    }
}
static void rnd_roles_tide(uint8_t *role)
{
    role[RND_TD_KICK] = RND_ROLE_LOWPERC;
    role[RND_TD_MEL] = RND_ROLE_MELODY;
    role[RND_TD_BED] = RND_ROLE_CHORDS;
    role[RND_TD_NOISE] = RND_ROLE_NOISE;
}

/* ------------------------------------------------------------ CRATE --- */
/* CRATE: crate digging. Each roll a little record to sample: an old funk or soul side without drums (the drums,
 * the dust and the chops are yours, on another machine), the same four on the same tracks, composed rather than
 * generated: a song of sections that come back exactly (FORM AABA, ABAC or ABCB; A the verse, B the chorus or the
 * bridge, C the bridge; 4 or 8 bars each, CONTRAST and DEVELOP off: each section written once, at the roll), so it
 * loops and chops like a record. Track 1 the KEYS (RND_ROLE_CHORDS): diatonic sevenths mostly (CHORD WALK, CHRD
 * DIA7), four chords a section from a book of vamps and soul turns (i IV, I vi IV V, IV iii ii I ...), stabbed on
 * the beat, off it or on the eighths and sixteenths (a clavinet, the electric pianos) or held (an organ, strings).
 * Track 2 the BASS (RND_ROLE_BASS): onto the chord's notes (QNT CHRD), a locked line of its own each section, funk's
 * sixteenths jumping octaves or soul's eighths, on FM6's bass, PHYS' plucked string or TRIO low. Track 3 the HEAD
 * (RND_ROLE_MELODY): the hook, a locked TURING per section, its rests its own (no DROP: the same each time round), on
 * horns, a flute, vibes, or TRIO's flute or muted horn. Track 4 the STRINGS (RND_ROLE_COUNTER), mostly: a lush,
 * slow line by step onto the chord's notes (SERIES), swelling in, wide; else a sparse second voice answering in the
 * gaps. 80 to 100 BPM, the sixteenths and eighths lightly swung; a minor, dorian, major or mixolydian key. Clean: no
 * wow, no dust */
#define RND_CR_KEYS 0u
#define RND_CR_BASS 1u
#define RND_CR_HEAD 2u
#define RND_CR_COUNTER 3u
/* the book: four degrees a progression (0 = I); verses (vamps), choruses, bridges */
static const uint8_t RND_CR_VERSE[][4] = {{0, 3, 0, 3}, {0, 0, 3, 3}, {0, 5, 3, 4}, {0, 3, 4, 3}, {0, 0, 0, 3},
                                          {0, 2, 3, 2}};
static const uint8_t RND_CR_CHORUS[][4] = {{3, 4, 0, 0}, {5, 3, 0, 4}, {3, 2, 1, 0}, {3, 0, 3, 4}, {0, 4, 5, 3}};
static const uint8_t RND_CR_BRIDGE[][4] = {{5, 4, 3, 4}, {3, 3, 0, 0}, {5, 5, 3, 3}, {1, 4, 1, 4}, {3, 4, 5, 5}};
static uint8_t rnd_cr_lush;                         /* bit 0: the keys held (organ, strings); bit 1: track 4 the strings */
static void rnd_crate_sound(uint32_t tr, uint32_t role, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    uint8_t nice = rnd_nice;
    uint32_t e, k = (uint32_t)rnd_rr(0, 99);
    int ph = !*phys && rnd_phys_ok(), fm = !*fm6s && rnd_fm6_ok(), orc = !*oracles && eng_ok(ENGI_ORACLE);
    int orc2 = *oracles < 2u && eng_ok(ENGI_ORACLE), mc = rnd_mc_ok();   /* (a second ORACLE: FM6 gone, the CPU there) */
    int str = 0;
    if (role == RND_ROLE_CHORDS) {                  /* the keys: ORACLE's keys, clavinet, stabs, brass, a warm pad;
                                                     * TRIO's organ or string machine (more of them, asked 2026-11-08) */
        static const uint8_t fp[] = {13, 0, 8, 14, 6, 15}, fw[] = {6, 6, 2, 3, 2, 3};   /* CLAVINET, TINE EP, DX TINE,
                                                     * DRAWBARS, ORGAN, STRINGS */
        static const uint8_t op[] = {11, 12, 22, 4, 1}, ow[] = {5, 4, 2, 2, 2};   /* KEYS CLAV FAT POLY BRASS WARM PAD */
        if (fm && k < 80u) rnd_want_e = ENGI_FM6, rnd_want_pi = rnd_pick(fp, fw, NELEM(fp));
        else if (ph && k < 94u) rnd_want_e = ENGI_PHYS, rnd_want_pi = 9;   /* VIBES */
        else if (orc && k < 76u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pick(op, ow, NELEM(op));
        else rnd_want_e = 6u, rnd_want_pi = k < 90u ? 7 : 6;   /* (TRIO's organ, made from HOLLOW PAD below; STRING MACH) */
        rnd_cr_lush = rnd_want_e == 6u || (rnd_want_e == ENGI_ORACLE && rnd_want_pi == 1u) ||
                      (rnd_want_e == RND_FM6 && (rnd_want_pi == 14u || rnd_want_pi == 6u || rnd_want_pi == 15u));   /* (held,
                                                     * not stabbed) */
    } else if (role == RND_ROLE_BASS) {             /* TRIO's, a square or a triangle (asked: not PHYS' pluck, not FM6) */
        rnd_want_e = 6u, rnd_want_pi = 8;
    } else if (role == RND_ROLE_COUNTER && k < 75u) {   /* the strings: lush, slow, wide */
        static const uint8_t tp[] = {6, 5, 7}, tw[] = {5, 3, 2};   /* STRING MACH, WARM PAD, HOLLOW PAD */
        str = 1;
        rnd_cr_lush |= 2u;
        if (fm && k < 30u) rnd_want_e = ENGI_FM6, rnd_want_pi = rnd_pct(60) ? 15 : 25;   /* STRINGS, SOFT PAD */
        else if (orc2 && k < 22u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = 3;   /* SOFT STRINGS */
        else if (mc && k < 40u) rnd_want_e = ENGI_MACRO, rnd_want_pi = 6;   /* CHOIR FOF: the backing singers' oohs */
        else rnd_want_e = 6u, rnd_want_pi = rnd_pick(tp, tw, NELEM(tp));
    } else {                                        /* the head (or a second voice): ORACLE's flute, leads, brass, voice;
                                                     * MACRO's sung vowels (a vocal chop); TRIO's flute or muted horn */
        static const uint8_t op[] = {21, 17, 4, 18, 5}, ow[] = {4, 3, 3, 2, 1};   /* FLUTE SQUARE LEAD BRASS VOX SYNC LEAD */
        if (fm && k < 45u) rnd_want_e = ENGI_FM6, rnd_want_pi = rnd_pct(55) ? 9 : 20;   /* BRASS SECT, FLUTE */
        else if (ph && k < 70u) rnd_want_e = ENGI_PHYS, rnd_want_pi = 9;   /* VIBES */
        else if (orc2 && k < 60u) rnd_want_e = ENGI_ORACLE, rnd_want_pi = rnd_pick(op, ow, NELEM(op));
        else if (mc && k < 74u) rnd_want_e = ENGI_MACRO, rnd_want_pi = rnd_pct(60) ? 5 : 4;   /* VOWEL AH, VOSIM VOX */
        else rnd_want_e = 6u, rnd_want_pi = 10;     /* (TRIO's SOFT LEAD: made a flute or a muted horn below) */
    }
    rnd_nice = 1;
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    rnd_mc_model(tr);
    e = trk[tr].eng_req;
    if (e == ENGI_MACRO) {                          /* MACRO: a voice, warm, a consonant a note (the singers: none) */
        rnd_tp(tr, P_E0 + 1u, rnd_rr(10, 90));      /* TIMBR, COLOR: the vowel, the formants */
        rnd_tp(tr, P_E0 + 2u, rnd_rr(30, 90));
        rnd_tp(tr, P_E0 + 3u, rnd_rr(84, 102));     /* TONE: an old record's top, dull */
        rnd_tp(tr, P_E0 + 4u, 0); rnd_tp(tr, P_E0 + 5u, 0);
        rnd_tp(tr, P_E0 + 6u, str ? 0 : 1);
        rnd_tp(tr, P_E0 + 7u, 0);
    }
    if (e == 6u)                                    /* TRIO: INT2, INT3 the unison or an octave, CRATE's (rnd_tp's
                                                     * fifths too thick for its strings and keys, asked) */
        for (k = 1u; k < 3u; k++)
            rnd_tp(tr, P_E0 + k, trk[tr].p[P_E0 + k] >= 6 ? 12 : trk[tr].p[P_E0 + k] <= -6 ? -12 : 0);
    if (e == RND_FM6)                              /* FM6: the preset's algorithm, feedback, ratios and vibrato (the
                                                     * push swapped a tine's ALG for another, clanging; MLVL, MEG, DTUN,
                                                     * AEG: its tone, kept) */
        for (k = 0; k < 6u; k++)
            if (k != 2u && k != 4u) rnd_tp(tr, P_E0 + k, 0);
    rnd_tp_still(tr);
    rnd_tp(tr, P_LD_PIT, 0);
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_DETUNE, rnd_rr(0, 25));
    rnd_tp(tr, P_SPRD, rnd_rr(10, 50));             /* (a combo: near the middle, as an old record) */
    if (role == RND_ROLE_CHORDS) {
        static const int8_t keys_r[][3] = {{P_ATK, 0, 10}, {P_DEC, 70, 105}, {P_SUS, 40, 90}, {P_REL, 55, 85},
                                           {P_LEVEL, 74, 88}, {P_PAN, -20, 20}};
        rnd_tp_rr(tr, keys_r, NELEM(keys_r));
        rnd_tp(tr, P_VOICE, V_POLY);
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 0));   /* (MLVL: mellow) */
        if (trk[tr].preset == 6 || trk[tr].preset == 14) rnd_tp(tr, P_LD_AMP, 0);   /* (an organ held) */
        if (e == ENGI_ORACLE && trk[tr].preset == 1) rnd_tp(tr, P_ATK, rnd_rr(20, 45));   /* (the warm pad: in softly) */
        if (e == 6u && trk[tr].preset == 7) {       /* (TRIO's organ: squares on octaves, drawbars; at once, held) */
            rnd_tp(tr, P_E0, 1);                    /* PLS3 */
            rnd_tp(tr, P_E0 + 1u, 12); rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : -12); rnd_tp(tr, P_E0 + 3u, rnd_rr(2, 8));
            rnd_tp(tr, P_E0 + 4u, 0); rnd_tp(tr, P_E0 + 5u, rnd_rr(68, 86)); rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 15));
            rnd_tp(tr, P_E0 + 7u, rnd_rr(50, 64));  /* (PW: near square, hollow) */
            rnd_tp(tr, P_ATK, rnd_rr(0, 6)); rnd_tp(tr, P_SUS, rnd_rr(100, 120));
            rnd_tp(tr, P_LD_AMP, 0);
        } else if (e == 6u || (e == RND_FM6 && trk[tr].preset == 15)) {   /* (strings: swelling in, wide) */
            rnd_tp(tr, P_ATK, rnd_rr(30, 60));
            rnd_tp(tr, P_SPRD, rnd_rr(60, 90));
            if (e == 6u) rnd_tp(tr, P_E0 + 5u, rnd_rr(80, 92));   /* (TRIO: open enough to be heard, measured) */
        }
    } else if (role == RND_ROLE_BASS) {
        if (e == 6u) {                              /* (TRIO: a round bass, a triangle and a pulse, low-passed) */
            rnd_tp(tr, P_E0, rnd_pct(60) ? 4 : 1);
            rnd_tp(tr, P_E0 + 1u, 0); rnd_tp(tr, P_E0 + 2u, -12); rnd_tp(tr, P_E0 + 3u, 0);
            rnd_tp(tr, P_E0 + 4u, 0); rnd_tp(tr, P_E0 + 5u, rnd_rr(30, 50)); rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 20));
        }
        if (e == RND_PHYS) { rnd_tp(tr, P_E0 + 2u, rnd_rr(25, 50)); rnd_tp(tr, P_E0 + 6u, 0); }   /* (BRIT: thumbed) */
        static const int8_t bass_r[][3] = {{P_ATK, 0, 6}, {P_DEC, 60, 90}, {P_SUS, 20, 60}, {P_REL, 35, 60},
                                           {P_LEVEL, 92, 106}};
        rnd_tp_rr(tr, bass_r, NELEM(bass_r));
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_PAN, 0);
        rnd_tp(tr, P_SPRD, 0);
    } else if (str) {                               /* the strings: a slow swell, held long, a slight vibrato, wide */
        static const int8_t sr[][3] = {{P_ATK, 55, 85}, {P_DEC, 100, 127}, {P_SUS, 100, 127}, {P_REL, 85, 110},
                                       {P_LEVEL, 90, 102}, {P_PAN, -15, 15}, {P_SPRD, 70, 100}, {P_DETUNE, 20, 45},
                                       {P_LRATE, 66, 74}, {P_LFADE, 60, 90}};
        rnd_tp_rr(tr, sr, NELEM(sr));
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LD_PIT, 0);                    /* (no vibrato: rnd_no_pitch) */
        if (e == 6u) rnd_tp(tr, P_E0 + 5u, rnd_rr(74, 88));   /* (TRIO: warm, the top soft, an old record's; lower,
                                                     * measured: lost) */
        else if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 0));
        if (e == ENGI_MACRO) rnd_tp(tr, P_DETUNE, rnd_rr(0, 15));   /* (the singers: together) */
    } else {                                        /* the head, the counter */
        int horn = rnd_pct(40);
        if (e == 6u) {                              /* TRIO: a flute (triangles, breathy) or a muted horn (a pulse
                                                     * through a band-pass) */
            rnd_tp(tr, P_E0, horn ? 1 : 4);
            rnd_tp(tr, P_E0 + 1u, horn ? 0 : 12); rnd_tp(tr, P_E0 + 2u, 0); rnd_tp(tr, P_E0 + 3u, rnd_rr(3, 10));
            rnd_tp(tr, P_E0 + 4u, horn ? 1 : 0);
            rnd_tp(tr, P_E0 + 5u, horn ? rnd_rr(55, 75) : rnd_rr(50, 75));
            rnd_tp(tr, P_E0 + 6u, horn ? rnd_rr(15, 35) : rnd_rr(0, 20));   /* (the horn: nasal, not a whistle) */
            rnd_tp(tr, P_LRATE, rnd_rr(68, 78));    /* (a player's vibrato, slight) */
            rnd_tp(tr, P_LD_PIT, 0);
            rnd_tp(tr, P_LFADE, rnd_rr(40, 80));
        }
        rnd_tp(tr, P_ATK, e == 6u ? rnd_rr(5, 25) : rnd_rr(0, 6));
        rnd_tp(tr, P_DEC, rnd_rr(65, 100));
        rnd_tp(tr, P_SUS, e == 6u ? rnd_rr(60, 100) : rnd_rr(10, 50));
        rnd_tp(tr, P_REL, rnd_rr(50, 85));
        rnd_tp(tr, P_VOICE, e == 6u ? V_MONO : V_POLY);
        rnd_tp(tr, P_LEVEL, role == RND_ROLE_MELODY ? rnd_rr(84, 98) : rnd_rr(66, 80));
        rnd_tp(tr, P_PAN, role == RND_ROLE_MELODY ? rnd_rr(-15, 15) : rnd_pct(50) ? rnd_rr(25, 45) : -rnd_rr(25, 45));
        if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 0));
        if (e == RND_PHYS) rnd_tp(tr, P_E0 + 6u, 0);
    }
    if (e == RND_PHYS && role != RND_ROLE_BASS) rnd_tp(tr, P_LEVEL, trk[tr].p[P_LEVEL] + 14);   /* (the vibes: quieter than
                                                     * the rest at a level, measured, 15 dB) */
    {                                               /* (the levels against CRATE's others, measured, half a dB a step:
                                                     * TRIO's organ 7 dB over the keys, the clavinet's stabs 9 under;
                                                     * MACRO's voices 4 to 9 under) */
        int32_t d = e == 6u && trk[tr].preset == 7 && role == RND_ROLE_CHORDS ? -14 :
                    e == ENGI_ORACLE && trk[tr].preset == 12 ? 10 :
                    e == ENGI_MACRO ? (trk[tr].preset == 4 ? 18 : trk[tr].preset == 5 ? 12 : 18) : 0;
        if (d) rnd_tp(tr, P_LEVEL, clamp(trk[tr].p[P_LEVEL] + d, 0, 120));
    }
}
/* a section of a track's FORM written now (its generator's settings as tess_save_sec keeps them), as if it had been
 * played and kept: FORM recalls it, never makes it (CONTRAST does not touch it) */
static void rnd_crate_sec(uint32_t tr, uint32_t sec)
{
    tessera_evolve_t *ev = &tessera.evolve[tr];
    tessera_sec_t *s = &tessera.sec[tr][sec % 3u];
    gen_sec_t g = {s->reg, s->p0, s->p1};
    tess_gen_save(tr, &g, s->note);
    s->reg = g.reg; s->p0 = g.p0; s->p1 = g.p1;
    ev->visited |= (uint8_t)(1u << (sec % 3u));
}
static void rnd_crate_generators(void)
{
    static const uint8_t fm[] = {4, 2, 3}, fw[] = {4, 3, 3};   /* AABA, ABAC, ABCB */
    uint32_t tr, chords = 1, sec;
    int32_t form = (int32_t)rnd_pick(fm, fw, NELEM(fm)), every = rnd_pct(60) ? 4 : 3;   /* (8 bars a section, or 4) */
    int32_t hits[3], bars = rnd_pct(70) ? 1 : 2;    /* (a chord a bar, or two) */
    const uint8_t *prog[3];
    static const uint8_t ht[] = {0, 1, 3, 2}, htw[] = {1, 2, 4, 4};   /* HOLD BEAT OFFBEAT STEP: stabbed, mostly */
    int32_t funk = rnd_pct(55);                     /* (the bass on sixteenths, jumping; else soul's eighths) */
    int32_t mdiv = rnd_pct(75) ? 1 : 0;             /* (the head on eighths, or quarters) */
    prog[0] = RND_CR_VERSE[rnd_next() % NELEM(RND_CR_VERSE)];
    prog[1] = form == 4 ? RND_CR_BRIDGE[rnd_next() % NELEM(RND_CR_BRIDGE)] : RND_CR_CHORUS[rnd_next() % NELEM(RND_CR_CHORUS)];
    prog[2] = RND_CR_BRIDGE[rnd_next() % NELEM(RND_CR_BRIDGE)];
    hits[0] = (int32_t)rnd_pick(ht, htw, NELEM(ht));
    hits[1] = rnd_pct(50) ? hits[0] : (int32_t)rnd_pick(ht, htw, NELEM(ht));   /* (the chorus comped its own way, now and
                                                     * then) */
    hits[2] = rnd_pct(60) ? 0 : hits[0];            /* (the bridge held, mostly) */
    if (rnd_cr_lush & 1u) hits[0] = hits[1] = hits[2] = 0;   /* (an organ, strings: held) */
    for (tr = 0; tr < RND_NT; tr++) {       /* (the clean slate) */
        if (tr != RND_CR_KEYS) rnd_generator(tr, &chords);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_TW_DROP, 0);                  /* (no gaps at random: each time round the same) */
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SLEN, 16);
    }
    tr = RND_CR_KEYS;                               /* the keys: diatonic sevenths, four a section */
    rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
    rnd_tx(tr, TX_CH_PROG, 6);                      /* WALK: its four degrees ours */
    rnd_tx(tr, TX_CH_BARS, bars);
    rnd_tx(tr, TX_CH_HIT, hits[0]);
    rnd_tx(tr, TX_CH_OCT, 0);
    rnd_tx(tr, TX_CH_BREATH, 0);
    rnd_tp(tr, P_CHRD, rnd_pct(80) ? 2 : 1);        /* DIA7 (DIA3 now and then: plainer) */
    rnd_tp(tr, P_SDIV, hits[0] == 2 ? (rnd_pct(50) ? 2 : 1) : 0);   /* (STEP: on sixteenths or eighths) */
    rnd_tp(tr, P_SGATE, hits[0] ? rnd_rr(25, 55) : 100);   /* (stabs short) */
    tr = RND_CR_BASS;                               /* the bass: a locked line of its own */
    rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
    rnd_tx(tr, TX_TUR_LEN, 16);
    rnd_tx(tr, TX_TUR_PROB, 0);                     /* (locked) */
    rnd_tx(tr, TX_TUR_SPAN, 1);                     /* (2, octaves at random: leaping about, heard) */
    rnd_tp(tr, P_SDIV, funk ? 2 : 1);
    rnd_tp(tr, P_SGATE, funk ? rnd_rr(30, 60) : rnd_rr(55, 85));
    rnd_tp(tr, P_QUANT, Q_CHRD);
    tr = RND_CR_HEAD;                               /* the head: a locked TURING, its own rests */
    rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
    rnd_tx(tr, TX_TUR_LEN, mdiv == 1 ? 16 : 8);     /* (two bars a phrase) */
    rnd_tx(tr, TX_TUR_PROB, 0);
    rnd_tx(tr, TX_TUR_SPAN, rnd_pct(70) ? 1 : 2);
    rnd_tp(tr, P_SDIV, mdiv);
    rnd_tp(tr, P_SGATE, rnd_rr(60, 100));
    if (rnd_pct(35)) rnd_tp(tr, P_QUANT, Q_CHRD);   /* (onto the chord's notes now and then: inside) */
    tr = RND_CR_COUNTER;                            /* the strings: by step, slow, held; or a counter, sparse */
    if (rnd_cr_lush & 2u) {
        rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
        rnd_tx(tr, TX_SR_INTV, 1);
        rnd_tx(tr, TX_SR_LOOP, rnd_pct(60) ? 4 : 8);
        rnd_tp(tr, P_SDIV, rnd_pct(65) ? 6 : 7);    /* 1/2, 1/1 */
        rnd_tp(tr, P_SGATE, 120);                   /* (each note into the next) */
    } else {
        rnd_tx(tr, TX_MODE, TESS_ENG_TURING);
        rnd_tx(tr, TX_TUR_LEN, 16);
        rnd_tx(tr, TX_TUR_PROB, 0);
        rnd_tx(tr, TX_TUR_SPAN, 1);
        rnd_tp(tr, P_SDIV, rnd_pct(60) ? 0 : 1);
        rnd_tp(tr, P_SGATE, rnd_rr(50, 90));
    }
    rnd_tp(tr, P_QUANT, Q_CHRD);
    for (tr = 0; tr < RND_NT; tr++) {       /* the eighths swung, a player's timing, the form */
        int32_t d = trk[tr].p[P_SDIV];
        rnd_tp(tr, P_SSWING, d == 2 ? rnd_rr(10, 30) : d == 1 ? rnd_rr(20, 40) : 0);   /* (the pocket: light) */
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(10, 35));
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(5, 25));
        rnd_tx(tr, TX_EV_EVERY, every);
        rnd_tx(tr, TX_EV_AMOUNT, 0);
        rnd_tx(tr, TX_FM_CONTRAST, 0);
        rnd_tx(tr, TX_FM_DEVELOP, 0);
        rnd_tx(tr, TX_FM_FORM, form);               /* (restarts the form: A the phrase now) */
        tess_roll_apply(tr);                        /* (a seed rnd_generator left waiting, applied now: on the next bar
                                                     * line it would start the form again, the sections written below
                                                     * forgotten, B and C made from A, measured) */
    }
    for (sec = 0; sec < 3u; sec++) {                /* each section written: A, B, C */
        uint32_t i;
        tr = RND_CR_KEYS;
        for (i = 0; i < GEN_CH_WALK_N; i++) tessera.chord[tr].walk[i] = prog[sec][i];
        tessera.chord[tr].rot = 0;
        tessera.chord[tr].hit = (uint8_t)hits[sec];
        rnd_crate_sec(tr, sec);
        for (tr = 1; tr < RND_NT; tr++) {   /* (the others: a fresh phrase each section) */
            if (tessera.mode[tr] == TESS_ENG_TURING) {
                tessera.turing[tr].reg = (uint16_t)rnd_next();
                tessera.evolve[tr].base_reg = tessera.turing[tr].reg;   /* (what a section keeps: tess_gen_save) */
            } else if (tessera.mode[tr] == TESS_ENG_SERIES) tessera.series[tr].start = (uint8_t)(rnd_next() & 127u);
            rnd_crate_sec(tr, sec);
        }
    }
    for (tr = 0; tr < RND_NT; tr++) {       /* (A, to start) */
        tessera.evolve[tr].sec = 0;
        tess_load_sec(tr);
        if (tessera.mode[tr] == TESS_ENG_TURING) tessera.evolve[tr].base_reg = tessera.turing[tr].reg;
    }
}
static void rnd_crate_registers(void)
{
    rnd_tp(RND_CR_KEYS, P_TRANS, 0);
    rnd_tp(RND_CR_BASS, P_TRANS, 0);                /* (-12: 23 to 74 Hz, measured, felt more than heard: mud) */
    rnd_tp(RND_CR_HEAD, P_TRANS, 12);               /* (the hook over the keys, the strings an octave under it: +24 and
                                                     * +12 were bright, heard 2026-11-09, a third of it over 2 kHz) */
    rnd_tp(RND_CR_COUNTER, P_TRANS, trk[RND_CR_HEAD].p[P_TRANS] - 12);
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(80, 100));
}
static void rnd_key_crate(void)
{
    static const uint8_t sc[] = {1, 2, 3, 4}, sw[] = {2, 3, 3, 2};   /* MAJ MIN DOR MIX */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
static void rnd_cords_crate(void)
{
    uint32_t i;
    cord_t z;
    memset(&z, 0, sizeof z);
    for (i = 0; i < CORD_LIVE; i++) cord_set(i, &z);  /* (none: a record, played as it was) */
}
static void rnd_roles_crate(uint8_t *role)
{
    role[RND_CR_KEYS] = RND_ROLE_CHORDS;
    role[RND_CR_BASS] = RND_ROLE_BASS;
    role[RND_CR_HEAD] = RND_ROLE_MELODY;
    role[RND_CR_COUNTER] = RND_ROLE_COUNTER;
}

/* ------------------------------------------------------------ STATIC --- */
/* STATIC: a wall of noise as ambient, after Tim Hecker, Fennesz and Ben Frost: not slow and melodic but dense and
 * atonal (the key chromatic, whole tone, diminished, locrian, 13 EDO or Bohlen-Pierce), a system overloading. Four
 * sustained layers: the BED (RND_ST_BED: CHORD held, four bars a chord, clusters on those scales, breathing; ORACLE's
 * pads or TRIO's detuned saws), the NOISE (never a rest, on the bed's chord: MACRO's noises or TRIO's noise through a
 * resonant filter, whistling), the FIZZ (off the chord, wandering, stuttering fast: TRIO's ring and sync waves or
 * MACRO's grit, through SPIT's soft-clip drive; its S&H fast on the shape) and the BASS (RND_ST_DRIVE, asked
 * 2026-11-08: a low brooding Reese, TRIO's saws in unison slowly beating, low-passed, C1 to C3; or ORACLE's dark
 * drone; the chord's notes a bar or two each, never a rest). No drums. rnd_static_move builds the noise and the fizz up
 * over the bed and the bass to a wall (SPIT's drive at its most), holds it, drops back to the bed, the bass and the
 * noise, then builds again. Inserts: SPIT on the fizz, and SPIT on the noise or TAPE on the bed: two, within the
 * effects' points (rnd_fx_points). DRIFT does not touch it (its own process; FAST builds twice as fast) */
#define RND_ST_BED 0u
#define RND_ST_NOISE 1u
#define RND_ST_FIZZ 2u
#define RND_ST_DRIVE 3u
enum { RND_ST_BUILD, RND_ST_WALL, RND_ST_LOW };
static struct {
    uint8_t stage, base[RND_NT], put[RND_NT], spit[RND_NT];
    uint16_t x, from, to;                           /* the density 0..1000: now, this stage's start and end */
    uint32_t t0, len;                               /* the stage's start and length, in 64ths */
} rnd_st;
static const uint16_t RND_ST_TH[RND_NT] = {0, 0, 220, 0};   /* (where each layer comes in: the noise with the bed, from
                                                     * the start, asked 2026-11-08: more noise throughout) */
static void rnd_static_sound(uint32_t tr, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    static const uint8_t bo[] = {1, 3, 22, 19, 2}, bow[] = {3, 2, 2, 2, 1};   /* WARM PAD SOFT STRINGS FAT POLY SWEEP PAD
                                                     * GLASS PAD */
    static const uint8_t fw[] = {12, 13, 14, 15, 9, 10, 11}, fww[] = {3, 3, 2, 2, 2, 2, 1};   /* RING RING3 R+S R+SP SYNC
                                                     * SYNCP SYNC3 */
    static const uint8_t fp[] = {4, 16, 27, 26, 9}, fpw[] = {3, 2, 2, 2, 1};   /* PAD GLASS PAD DEEP PAD SOFT SWELL BRASS
                                                     * SECT */
    track_t *t = &trk[tr];
    (void)phys;
    if (tr == RND_ST_BED) {                         /* buried: a pad, slow */
        if (eng_ok(ENGI_ORACLE) && rnd_pct(55)) {
            uint8_t *q;
            engine_load_of(t, ENGI_ORACLE, rnd_pick(bo, bow, NELEM(bo)));
            q = or_patch_of(t);
            q[OR_ATTACK_AMP] = (uint8_t)rnd_rr(60, 96);
            q[OR_RELEASE_AMP] = (uint8_t)rnd_rr(70, 90);
            q[OR_RELEASE_ON] = 1;
            q[OR_NOISE] = (uint8_t)rnd_rr(40, 90);   /* (a hiss in it, up: noise throughout) */
            (*oracles)++;
        } else {
            engine_load_of(t, 6u, 15u);             /* (SWEEP PAD, made below) */
            rnd_tp(tr, P_E0, 0);                    /* SAW3 */
            rnd_tp(tr, P_E0 + 1u, rnd_pct(50) ? 12 : 0);
            rnd_tp(tr, P_E0 + 2u, rnd_pct(40) ? -12 : rnd_pct(50) ? 7 : 0);
            rnd_tp(tr, P_E0 + 3u, rnd_rr(24, 50));  /* DTN: wide */
            rnd_tp(tr, P_E0 + 4u, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 62));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(10, 35));
            {
                static const int8_t env[][3] = {{P_ATK, 70, 100}, {P_DEC, 100, 127}, {P_SUS, 100, 127}, {P_REL, 70, 90}};
                rnd_tp_rr(tr, env, NELEM(env));
            }
        }
    } else if ((tr == RND_ST_NOISE && rnd_mc_ok() && rnd_pct(70)) || (tr == RND_ST_FIZZ && rnd_mc_ok() && rnd_pct(40))) {
        static const uint8_t np[] = {13, 10, 11, 14, 12, 15}, nw[] = {3, 2, 2, 2, 1, 1};   /* GRAIN CLOUD WIND TWIN PEAKS
                                                     * DUST BIT NOISE QPSK DATA */
        static const uint8_t zp[] = {2, 8, 9, 12, 15}, zw[] = {2, 2, 1, 2, 1};   /* SAW SWARM FB GROWL CHAOS FM BIT NOISE
                                                     * QPSK DATA */
        engine_load_of(t, ENGI_MACRO, tr == RND_ST_NOISE ? rnd_pick(np, nw, NELEM(np)) : rnd_pick(zp, zw, NELEM(zp)));
        rnd_tp(tr, P_E0 + 1u, clamp(t->p[P_E0 + 1u] + rnd_rr(-20, 20), 0, 127));   /* TIMBR, COLOR: near the preset's */
        rnd_tp(tr, P_E0 + 2u, clamp(t->p[P_E0 + 2u] + rnd_rr(-20, 20), 0, 127));
        rnd_tp(tr, P_E0 + 3u, rnd_rr(96, 127));   /* TONE: open-ish (a noise's energy is high up: 70 was 15 dB
                                                     * down, measured) */
        rnd_tp(tr, P_E0 + 4u, rnd_pct(30) ? rnd_rr(1, 3) : 0);   /* BITS, RATE: crushed now and then */
        rnd_tp(tr, P_E0 + 5u, rnd_pct(25) ? rnd_rr(1, 2) : 0);
        rnd_tp(tr, P_E0 + 7u, rnd_rr(0, 40));       /* DRIVE */
        {
            static const int8_t env[][3] = {{P_ATK, 50, 90}, {P_DEC, 100, 127}, {P_SUS, 100, 127}, {P_REL, 60, 86}};
            rnd_tp_rr(tr, env, NELEM(env));
        }
    } else if (tr == RND_ST_FIZZ && rnd_fm6_ok() && rnd_pct(25)) {   /* FM6, its feedback up: grit */
        uint32_t k;
        engine_load_of(t, ENGI_FM6, rnd_pick(fp, fpw, NELEM(fp)));
        for (k = 0; k < 6u; k++) rnd_tp(tr, P_E0 + k, 0);
        rnd_tp(tr, P_E0 + 1u, rnd_rr(3, 7));       /* FB */
        rnd_tp(tr, P_E0 + 2u, rnd_rr(10, 30));      /* MLVL */
        (*fm6s)++;
    } else if (tr == RND_ST_DRIVE && !*oracles && eng_ok(ENGI_ORACLE) && rnd_pct(35)) {   /* the bass: ORACLE, a dark
                                                     * drone (its saws low, closed, a growl of noise) */
        uint8_t *q;
        uint8_t tune[3];
        engine_load_of(t, ENGI_ORACLE, rnd_pct(50) ? 22u : 1u);   /* FAT POLY, WARM PAD */
        q = or_patch_of(t);
        tune[0] = q[OR_FREQ_A]; tune[1] = q[OR_FREQ_B]; tune[2] = q[OR_FINE_B];
        rnd_oracle(tr, 25u);
        q[OR_FREQ_A] = tune[0]; q[OR_FREQ_B] = tune[1]; q[OR_FINE_B] = tune[2];   /* (in tune: the preset's) */
        q[OR_SAW_A] = q[OR_SAW_B] = 1;
        q[OR_LEVEL_A] = q[OR_LEVEL_B] = 110;
        q[OR_GLIDE] = 0;
        q[OR_NOISE] = (uint8_t)rnd_rr(10, 40);
        q[OR_CUTOFF] = (uint8_t)rnd_rr(28, 48);
        q[OR_RESONANCE] = (uint8_t)rnd_rr(20, 60);
        q[OR_ATTACK_AMP] = (uint8_t)rnd_rr(60, 90);
        q[OR_SUSTAIN_AMP] = 127;
        q[OR_RELEASE_AMP] = (uint8_t)rnd_rr(70, 90);
        q[OR_RELEASE_ON] = 1;
        (*oracles)++;
    } else {                                        /* TRIO: noise, ring and sync, stacked saws */
        engine_load_of(t, 6u, 15u);
        if (tr == RND_ST_NOISE) {
            rnd_tp(tr, P_E0, rnd_pct(60) ? 8 : 7);  /* NOIS P+N */
            rnd_tp(tr, P_E0 + 4u, rnd_pct(55) ? 1 : rnd_pct(50) ? 2 : 0);   /* BP HP LP */
            rnd_tp(tr, P_E0 + 5u, rnd_rr(50, 92));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(55, 100)); /* RES: it whistles */
        } else if (tr == RND_ST_FIZZ) {
            rnd_tp(tr, P_E0, (int32_t)rnd_pick(fw, fww, NELEM(fw)));
            rnd_tp(tr, P_E0 + 4u, rnd_pct(70) ? 0 : 1);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(60, 90));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(30, 75));
        } else {                                    /* the bass: a Reese (asked 2026-11-08), saws in unison, slowly
                                                     * beating against each other, an octave up now and then, low-passed */
            rnd_tp(tr, P_E0, 0);                    /* SAW3 */
            rnd_tp(tr, P_E0 + 4u, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(30, 52));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(10, 40));
        }
        rnd_tp(tr, P_E0 + 1u, tr == RND_ST_DRIVE ? 0 : rnd_pct(50) ? 12 : 0);
        rnd_tp(tr, P_E0 + 2u, tr == RND_ST_DRIVE ? (rnd_pct(50) ? 12 : 0) : rnd_pct(50) ? -12 : rnd_pct(50) ? 7 : 12);   /* (the
                                                     * bass: no sub, it is low already; an octave up for its edge) */
        rnd_tp(tr, P_E0 + 3u, tr == RND_ST_DRIVE ? rnd_rr(16, 40) : rnd_rr(10, 45));   /* (DTN: the Reese's beating) */
        {
            static const int8_t env[][3] = {{P_ATK, 50, 90}, {P_DEC, 100, 127}, {P_SUS, 100, 127}, {P_REL, 60, 86}};
            rnd_tp_rr(tr, env, NELEM(env));
        }
    }
    rnd_tp_still(tr);
    rnd_tp(tr, P_LD_PIT, 0);
    rnd_tp(tr, P_GLIDE, 0);                         /* (no slides between the chords' notes: heard, silly) */
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_MUTE, 0);
    rnd_tp(tr, P_AMODE, 0);
    rnd_tp(tr, P_VOICE, tr == RND_ST_BED ? V_POLY : V_LEGATO);
    rnd_tp(tr, P_LRATE, tr == RND_ST_BED ? rnd_rr(10, 30) : rnd_rr(96, 122));   /* (the S&H fast: fizzing) */
    rnd_tp(tr, P_SPRD, rnd_rr(50, 100));
    rnd_tp(tr, P_DETUNE, 0);
    if (tr == RND_ST_DRIVE) {                       /* (the bass: centred, narrow, slow in) */
        rnd_tp(tr, P_SPRD, rnd_rr(0, 25));
        rnd_tp(tr, P_ATK, rnd_rr(40, 75));
        rnd_tp(tr, P_SUS, 127);
        rnd_tp(tr, P_REL, rnd_rr(60, 85));
    }
    rnd_tp(tr, P_LEVEL, tr == RND_ST_BED ? rnd_rr(84, 94) : tr == RND_ST_NOISE ? rnd_rr(100, 110) :
                        tr == RND_ST_DRIVE ? rnd_rr(82, 94) : rnd_rr(92, 104));   /* (the bass 5 dB under the others:
                                                     * 9 over the bed, measured) */   /* (6 dB
                                                     * up, 2026-11-08: a wall, 9 dB under CRATE before) */
    rnd_mc_level(tr);
}
static void rnd_static_generators(void)
{
    static const uint8_t pg[] = {3, 0, 2, 6}, pw[] = {3, 2, 1, 2};   /* CIRCLE POP 2-5-1 WALK */
    static const uint8_t dv[] = {6, 7, 8}, dw[] = {2, 3, 2};   /* 1/2 1/1 2BAR */
    uint32_t tr, chords = 1;
    for (tr = 0; tr < RND_NT; tr++) {       /* (the clean slate) */
        rnd_generator(tr, &chords);
        rnd_tx(tr, TX_MODE, TESS_ENG_TURING);       /* the layers: a few notes, locked, held */
        rnd_tx(tr, TX_TUR_LEN, rnd_rr(2, 5));
        rnd_tx(tr, TX_TUR_PROB, rnd_rr(0, 12));
        rnd_tx(tr, TX_TUR_SPAN, rnd_rr(1, 2));
        rnd_tx(tr, TX_EV_EVERY, 0); rnd_tx(tr, TX_EV_LOCK, 0);
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0); rnd_tx(tr, TX_TW_FOLLOW, 0); rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_TW_HUMAN, 0); rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 30));
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SLEN, 16);
        rnd_tp(tr, P_SDIV, (int32_t)rnd_pick(dv, dw, NELEM(dv)));
        rnd_tp(tr, P_SGATE, 127);
        rnd_tp(tr, P_QUANT, Q_CHRD);                /* (onto the bed's chord) */
    }
    tr = RND_ST_BED;                                /* the progression, buried: four bars a chord, held */
    rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
    rnd_tx(tr, TX_CH_PROG, (int32_t)rnd_pick(pg, pw, NELEM(pg)));
    rnd_tx(tr, TX_CH_BARS, 3);
    rnd_tx(tr, TX_CH_HIT, 0);
    rnd_tx(tr, TX_CH_OCT, 0);
    rnd_tx(tr, TX_CH_BREATH, rnd_rr(20, 50));
    rnd_tp(tr, P_CHRD, rnd_pct(60) ? 2 : 1);        /* DIA7, DIA3 (on STATIC's scales: clusters, rnd_key_static) */
    rnd_tp(tr, P_SDIV, 0);
    rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    tr = RND_ST_NOISE;                              /* the noise: never a rest (SERIES; TURING rests on its 0 bits,
                                                     * heard: a bar or more gone), held, on the chord */
    rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
    rnd_tx(tr, TX_SR_START, rnd_rr(0, 127));
    rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 3));
    rnd_tx(tr, TX_SR_LOOP, rnd_rr(2, 4));
    rnd_tp(tr, P_SDIV, rnd_pct(50) ? 7 : 8);        /* 1/1 2BAR */
    tr = RND_ST_FIZZ;                               /* the fizz: off the chord, wandering (atonal), stuttering */
    {
        static const uint8_t fd[] = {2, 1, 4}, fdw[] = {3, 2, 2};   /* 1/16 1/8 8T */
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
        rnd_tx(tr, TX_TUR_PROB, rnd_rr(30, 70));
        rnd_tx(tr, TX_TUR_SPAN, rnd_rr(2, 3));
        rnd_tx(tr, TX_TUR_LEN, rnd_rr(5, 11));
        rnd_tp(tr, P_SDIV, (int32_t)rnd_pick(fd, fdw, NELEM(fd)));
        rnd_tp(tr, P_SGATE, rnd_rr(30, 110));
    }
    tr = RND_ST_DRIVE;                              /* the bass: low, brooding, the bed's chord's notes, never a rest,
                                                     * a note a bar or two */
    rnd_tx(tr, TX_MODE, TESS_ENG_SERIES);
    rnd_tx(tr, TX_SR_START, rnd_rr(0, 40));
    rnd_tx(tr, TX_SR_INTV, rnd_rr(1, 2));
    rnd_tx(tr, TX_SR_LOOP, rnd_rr(2, 4));
    rnd_tp(tr, P_SDIV, rnd_pct(60) ? 7 : 8);        /* 1/1 2BAR */
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(100, 120));   /* (100 to 120, asked 2026-11-09: was 60 to 80) */
}
static void rnd_static_registers(void)
{
    rnd_tp(RND_ST_BED, P_TRANS, 0);                 /* (over the bass: each its own space, 2026-11-09) */
    rnd_tp(RND_ST_NOISE, P_TRANS, 24);
    rnd_tp(RND_ST_FIZZ, P_TRANS, 12);
    rnd_tp(RND_ST_DRIVE, P_TRANS, -24);             /* (the bass: SERIES' C3 and two octaves above, two down: C1 to C3,
                                                     * 33 to 130 Hz) */
    rnd_tp(RND_ST_BED, P_PAN, 0);
    rnd_tp(RND_ST_NOISE, P_PAN, rnd_rr(-30, 30));
    rnd_tp(RND_ST_FIZZ, P_PAN, rnd_pct(50) ? -rnd_rr(15, 35) : rnd_rr(15, 35));
    rnd_tp(RND_ST_DRIVE, P_PAN, 0);
}
static void rnd_key_static(void)
{
    static const uint8_t sc[] = {0, 13, 14, 10, 20, 62}, sw[] = {4, 2, 2, 1, 1, 1};   /* CHR WHOLE DIMHW LOC 13EDO BP-ET
                                                     * (atonal, asked 2026-11-08: not the modes) */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
    }
    scale_share(&trk[0]);
}
/* the effects: SPIT's drive on the drive layer always (its band-pass jumping slowly, the soft clip hard), then one of
 * SPIT on the noise or TAPE on the bed now and then; no sends. The cords: the S&H fast on the noise's cutoff and the
 * fizz's shape, the drive's cutoff on a slow wander */
static void rnd_static_fx(void)
{
    uint32_t tr = RND_ST_FIZZ;                      /* (SPIT on the fizz: its band-pass took the bass's low end) */
    rnd_fx_dry();
    rnd_tp(tr, P_FXTYPE, FX_SPIT);
    rnd_tp(tr, P_FXMIX, rnd_rr(70, 110));
    rnd_tp(tr, P_FXRATE, rnd_rr(4, 30));
    rnd_tp(tr, P_FXCHAR, rnd_rr(70, 110));
    {                                               /* (always a second: the noise through SPIT mostly) */
        tr = rnd_pct(75) ? RND_ST_NOISE : RND_ST_BED;
        rnd_tp(tr, P_FXTYPE, tr == RND_ST_NOISE ? FX_SPIT : FX_TAPE);
        rnd_tp(tr, P_FXMIX, rnd_rr(50, 90));
        rnd_tp(tr, P_FXRATE, rnd_rr(10, 50));
        rnd_tp(tr, P_FXCHAR, rnd_rr(50, 100));
    }
    for (tr = 0; tr < RND_NT; tr++) rnd_st.spit[tr] = (uint8_t)trk[tr].p[P_FXCHAR];
}
static void rnd_cords_static(void)
{
    cord_t c[6];
    uint32_t i, n = 0;
    memset(c, 0, sizeof c);
    c[n].src_trk = c[n].dst_trk = RND_ST_NOISE; c[n].src = CS_SH; c[n].dst = CD_CUTOFF; c[n].amount = (int8_t)rnd_rr(15, 35); n++;
    c[n].src_trk = c[n].dst_trk = RND_ST_FIZZ; c[n].src = CS_SH; c[n].dst = CD_SHAPE; c[n].amount = (int8_t)rnd_rr(25, 55); n++;
    c[n].src_trk = c[n].dst_trk = RND_ST_FIZZ; c[n].src = CS_LFO; c[n].dst = CD_CUTOFF; c[n].amount = (int8_t)rnd_rr(8, 20); n++;
    c[n].src_trk = c[n].dst_trk = RND_ST_DRIVE; c[n].src = CS_DRIFT; c[n].dst = CD_CUTOFF;
    c[n].amount = (int8_t)(rnd_pct(50) ? rnd_rr(8, 20) : -rnd_rr(8, 20)); n++;   /* (the bass's filter wandering, a little) */
    c[n].src_trk = c[n].dst_trk = RND_ST_BED; c[n].src = CS_INERTIA; c[n].dst = CD_CUTOFF; c[n].amount = (int8_t)rnd_rr(10, 25); n++;
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t z;
        memset(&z, 0, sizeof z);
        cord_set(i, i < n ? &c[i] : &z);
    }
}
static void rnd_roles_static(uint8_t *role)
{
    role[RND_ST_BED] = RND_ROLE_CHORDS;             /* (the bed's engine first, then the fizz's, the noise's, the drive's) */
    role[RND_ST_NOISE] = RND_ROLE_NOISE;
    role[RND_ST_FIZZ] = RND_ROLE_GLITCH;
    role[RND_ST_DRIVE] = RND_ROLE_TINT;
}

/* ------------------------------------------------------------ PHASE --- */
/* PHASE: after Steve Reich's phasing (Piano Phase, Clapping Music, Drumming, Music for 18 Musicians). One short bright
 * cell, EUCLID's (its pitches its seed's, a step's each: two tracks on one seed, length and rotation play one melody),
 * dense, on two tracks with one sound, panned apart (RND_PH_A, RND_PH_B). They start in unison; every few bars B moves
 * a step ahead (rnd_phase_move: its EUCLID playhead), holds there while the new interlock sounds, and so on round until
 * the two meet again, when a new cell begins. Track C a third copy moving the other way, half as often, or a low pulse
 * under the cell; track D a soft pulsing chord bed, one or two chords, slow. The cell on TRIO's bright pulses, FM6's
 * mallets or ORACLE's plucks; no PHYS, no FORM, nothing dropped, no human timing: the machine's evenness is the point.
 * A pentatonic or modal key, dry, no cords. DRIFT does not touch it (its own process; FAST holds half as long) */
#define RND_PH_A 0u
#define RND_PH_B 1u
#define RND_PH_C 2u
#define RND_PH_BED 3u
static struct { uint8_t third, steps, moved_b, moved_c, hold, wait_b, wait_c; uint32_t bar; } rnd_ph;
/* a track the sound of another (the pair's: one sound, B as A) */
static void rnd_ph_copy(uint32_t dst, uint32_t src)
{
    track_t *d = &trk[dst];
    const track_t *t = &trk[src];
    uint32_t i;
    engine_load_of(d, t->eng_req, t->preset);
    fm1_irq_off();
    for (i = 0; i < P_COUNT; i++)
        if (i != P_PAN && i != P_TRANS) d->p[i] = t->p[i];
    memcpy(or_patch_of(d), or_patch[src], OR_N);
    fm1_irq_on();
    if (t->eng_req == RND_FM6) fm6_set_patch(dst, fm6_patch[src]);
}
static void rnd_phase_sound(uint32_t tr, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    static const uint8_t fp[] = {5, 12, 29, 19, 0, 8}, fw[] = {4, 3, 3, 2, 1, 1};   /* MARIMBA DX MARIMBA MALLET KALIMBA,
                                                     * TINE EP DX TINE */
    static const uint8_t op[] = {9, 20, 11}, ow[] = {4, 3, 2};   /* PLUCK SEQ BLIP KEYS (no POLY BELLS: no bells) */
    static const uint8_t tw[] = {1, 3, 0, 4, 6}, tww[] = {3, 2, 2, 2, 1};   /* PLS3 SST SAW3 TRI3 P&S: bright */
    track_t *t = &trk[tr];
    (void)phys;
    if (tr == RND_PH_C && trk[RND_PH_A].eng_req != 6u) rnd_ph.third = 0;   /* (three FM6s or ORACLEs on sixteenths: past
                                                     * the voices the FM-1 has, measured; TRIO's copies light) */
    if (tr == RND_PH_B || (tr == RND_PH_C && rnd_ph.third)) {
        rnd_ph_copy(tr, RND_PH_A);                  /* (the copies: A's sound) */
        return;
    }
    if (tr == RND_PH_A) {                           /* the cell: struck, short, bright */
        uint32_t k = (uint32_t)rnd_rr(0, 99);
        if (k < 30u && rnd_fm6_ok()) {
            engine_load_of(t, ENGI_FM6, rnd_pick(fp, fw, NELEM(fp)));
            for (k = 0; k < 6u; k++) rnd_tp(tr, P_E0 + k, 0);   /* (the preset's algorithm, feedback, ratios) */
            rnd_tp(tr, P_E0 + 2u, rnd_rr(-12, 4));  /* MLVL */
            rnd_tp(tr, P_E0 + 7u, rnd_rr(-30, -12));   /* AEG: shorter than the preset's (the voices free
                                                     * before the cell comes round) */
            (*fm6s)++;
        } else if (k < 55u && eng_ok(ENGI_ORACLE)) {
            uint8_t *q;
            engine_load_of(t, ENGI_ORACLE, rnd_pick(op, ow, NELEM(op)));
            q = or_patch_of(t);
            q[OR_ATTACK_AMP] = 0;
            q[OR_DECAY_AMP] = (uint8_t)rnd_rr(34, 52);
            q[OR_SUSTAIN_AMP] = 0;
            q[OR_RELEASE_AMP] = (uint8_t)rnd_rr(24, 40);
            q[OR_RELEASE_ON] = 1;
            (*oracles)++;
        } else {
            engine_load_of(t, 6u, 1u);              /* (TRIO's ARP LEAD, made below) */
            rnd_tp(tr, P_E0, (int32_t)rnd_pick(tw, tww, NELEM(tw)));   /* WAVE */
            rnd_tp(tr, P_E0 + 1u, rnd_pct(60) ? 0 : 12);   /* INT2 */
            rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 12 : rnd_pct(50) ? 0 : 7);   /* INT3 */
            rnd_tp(tr, P_E0 + 3u, rnd_rr(2, 8));    /* DTN */
            rnd_tp(tr, P_E0 + 4u, 0);               /* LP */
            rnd_tp(tr, P_E0 + 5u, rnd_rr(58, 80));  /* CUT, the envelope opening it */
            rnd_tp(tr, P_E0 + 6u, rnd_rr(8, 30));   /* RES */
            {
                static const int8_t env[][3] = {{P_ATK, 0, 3}, {P_DEC, 30, 50}, {P_SUS, 0, 8}, {P_REL, 18, 34},
                                                {P_ED_FLT, 22, 44}};
                rnd_tp_rr(tr, env, NELEM(env));
            }
        }
    } else if (tr == RND_PH_C) {                    /* the low pulse: round, plain */
        if (trk[RND_PH_A].eng_req != RND_FM6 && rnd_fm6_ok() && rnd_pct(40)) {
            engine_load_of(t, ENGI_FM6, 6u);        /* ORGAN, a bass */
            (*fm6s)++;
        } else {
            engine_load_of(t, 6u, 9u);              /* SUB BASS */
            rnd_tp(tr, P_E0, rnd_pct(50) ? 4 : 1);  /* TRI3 PLS3 */
            rnd_tp(tr, P_E0 + 1u, 0); rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : 12);
            rnd_tp(tr, P_E0 + 3u, rnd_rr(0, 4));
            rnd_tp(tr, P_E0 + 4u, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 60));
            rnd_tp(tr, P_E0 + 6u, rnd_rr(0, 15));
            {
                static const int8_t env[][3] = {{P_ATK, 0, 6}, {P_DEC, 50, 70}, {P_SUS, 40, 70}, {P_REL, 30, 46}};
                rnd_tp_rr(tr, env, NELEM(env));
            }
        }
        rnd_tp(tr, P_VOICE, V_MONO);
    } else {                                        /* the bed: soft, pulsing */
        static const uint8_t bo[] = {3, 1, 18}, bow[] = {3, 2, 2};   /* SOFT STRINGS WARM PAD VOX */
        static const uint8_t bt[] = {8, 7, 5, 6}, btw[] = {3, 2, 2, 2};   /* SOFT PAD HOLLOW PAD WARM PAD STRING MACH */
        if (!*oracles && eng_ok(ENGI_ORACLE) && rnd_pct(55)) {
            uint8_t *q;
            engine_load_of(t, ENGI_ORACLE, rnd_pick(bo, bow, NELEM(bo)));
            q = or_patch_of(t);
            q[OR_ATTACK_AMP] = (uint8_t)rnd_rr(26, 48);
            q[OR_RELEASE_AMP] = (uint8_t)rnd_rr(34, 50);
            q[OR_RELEASE_ON] = 1;
            (*oracles)++;
        } else {
            engine_load_of(t, 6u, rnd_pick(bt, btw, NELEM(bt)));
            rnd_tp(tr, P_E0 + 1u, rnd_pct(50) ? 0 : 12);
            rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : -12);
            {
                static const int8_t env[][3] = {{P_ATK, 26, 48}, {P_DEC, 70, 96}, {P_SUS, 70, 96}, {P_REL, 34, 50}};
                rnd_tp_rr(tr, env, NELEM(env));
            }
        }
        rnd_tp(tr, P_VOICE, V_POLY);
    }
    rnd_tp_still(tr);
    rnd_tp(tr, P_LD_PIT, 0);
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_DETUNE, 0);
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_MUTE, 0);
    rnd_tp(tr, P_AMODE, 0);
    rnd_tp(tr, P_LEVEL, tr == RND_PH_BED ? rnd_rr(76, 82) : tr == RND_PH_C ? rnd_rr(82, 88) : rnd_rr(84, 92));   /* (the
                                                     * pulse and the bed -12 to -15 dB, rnd_balance's lift with it,
                                                     * asked 2026-11-09: at -20 the cells' plucks buried them) */
    if (tr == RND_PH_A) {
        static const uint8_t lf[][2] = {{5, 20}, {12, 24}, {29, 14}, {19, 14}, {8, 12}},   /* FM6's: up, as loud as */
                             lo[][2] = {{9, 24}, {10, 20}, {11, 12}, {20, 24}};          /* TINE EP; ORACLE's */
        uint32_t k;
        if (t->eng_req == 6u) rnd_tp(tr, P_LEVEL, t->p[P_LEVEL] + 20);   /* (measured: the cell 12 dB over the bed
                                                     * and the pulse, a dB a level step of two) */
        for (k = 0; k < NELEM(lf); k++)
            if (t->eng_req == RND_FM6 && t->preset == lf[k][0]) rnd_tp(tr, P_LEVEL, t->p[P_LEVEL] + lf[k][1]);
        for (k = 0; k < NELEM(lo); k++)
            if (t->eng_req == ENGI_ORACLE && t->preset == lo[k][0]) rnd_tp(tr, P_LEVEL, t->p[P_LEVEL] + lo[k][1]);
        rnd_tp(tr, P_VOICE, t->eng_req == RND_FM6 ? V_MONO : V_POLY);   /* (FM6's mallets ring past the next note:
                                                     * one voice each, the next takes it, measured) */
        rnd_tp(tr, P_SPRD, 0);
    } else if (tr == RND_PH_BED) {
        rnd_tp(tr, P_SPRD, rnd_rr(40, 70));
    }
}
/* the cell, its copies, the pulse and the bed; the tempo by the cell's clock (FM6's on 1/8 at twice the tempo: the
 * same pace, rnd_cpu's rule) */
static void rnd_phase_cell(int32_t seed)
{
    static const uint8_t st[] = {12, 8, 10, 16}, sw[] = {5, 2, 2, 1};
    uint32_t tr;
    int32_t n = (int32_t)rnd_pick(st, sw, NELEM(st));
    int32_t hits = rnd_pct(30) ? n * 2 / 3 : n - rnd_rr(0, n / 4), rot = rnd_rr(0, n - 1);
    if (seed == tessera.seed[RND_PH_A]) seed = seed % (int32_t)TESS_SEED_MAX + 1;   /* (another: it restarts the copies
                                                     * together, tess_realign) */
    rnd_ph.steps = (uint8_t)n;
    for (tr = 0; tr < RND_NT; tr++) {
        if (tr == RND_PH_BED || (tr == RND_PH_C && !rnd_ph.third)) continue;
        rnd_tx(tr, TX_EUC_STEPS, n);
        rnd_tx(tr, TX_EUC_PULSES, hits);
        rnd_tx(tr, TX_EUC_ROT, rot);
        rnd_tx(tr, TX_EV_SEED, seed);               /* (last: the playheads on step 1 together, tess_realign; playing,
                                                     * on the next bar line, the copies' one) */
    }
    rnd_ph.moved_b = rnd_ph.moved_c = 0;
    rnd_ph.wait_b = rnd_ph.hold;                    /* (in unison a while first) */
    rnd_ph.wait_c = (uint8_t)(rnd_ph.hold * 2u);
}
static void rnd_phase_generators(void)
{
    uint32_t tr, chords = 1, fm = trk[RND_PH_A].eng_req == RND_FM6;
    int32_t div = fm ? 1 : 2, bpm = fm ? rnd_rr(160, 184) : rnd_rr(88, 104);
    static const uint8_t pg[] = {4, 5, 3, 0}, pw[] = {4, 2, 1, 1};   /* TWO DRONE CIRCLE POP */
    for (tr = 0; tr < RND_NT; tr++) {       /* (the clean slate: even, as written) */
        rnd_generator(tr, &chords);
        rnd_tx(tr, TX_MODE, TESS_ENG_EUCLID);
        rnd_tx(tr, TX_EV_EVERY, 0); rnd_tx(tr, TX_EV_LOCK, 0);
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0); rnd_tx(tr, TX_TW_FOLLOW, 0); rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_TW_HUMAN, 0); rnd_tx(tr, TX_SUB_HUMAN, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SLEN, 16);
        rnd_tp(tr, P_SDIV, div);
        rnd_tp(tr, P_SGATE, rnd_rr(40, 64));
    }
    rnd_tp(RND_PH_B, P_SGATE, trk[RND_PH_A].p[P_SGATE]);
    pid_set(PID_GLOBAL(G_BPM), 0, bpm);
    rnd_ph.hold = (uint8_t)(rnd_rr(10, 16) * bpm / 240 / (0 ? 2 : 1));   /* (about 10
                                                     * to 16 s a place, bars of four beats; DRIFT FAST, 2: half) */
    if (rnd_ph.hold < 2u) rnd_ph.hold = 2u;
    rnd_phase_cell(rnd_seed_pick());
    for (tr = 0; tr < RND_PH_BED; tr++) tess_roll_apply(tr);   /* (the roll's at once: the copies together now, not
                                                     * apart until the bar line, a roll before's places kept) */
    rnd_role_of[RND_PH_C] = rnd_ph.third ? RND_ROLE_CANON : RND_ROLE_BASS;   /* (a heavy cell's: the pulse) */
    if (rnd_ph.third) rnd_tp(RND_PH_C, P_SGATE, trk[RND_PH_A].p[P_SGATE]);
    else {                                          /* the pulse: a few hits on the cell's length */
        tr = RND_PH_C;
        rnd_tx(tr, TX_EUC_STEPS, rnd_ph.steps);
        rnd_tx(tr, TX_EUC_PULSES, rnd_rr(2, rnd_ph.steps > 8u ? 4 : 3));
        rnd_tx(tr, TX_EUC_ROT, 0);
        rnd_tp(tr, P_SGATE, rnd_rr(60, 90));
    }
    tr = RND_PH_BED;                                /* the bed: a chord or two, slow, pulsing or breathing */
    rnd_tx(tr, TX_MODE, TESS_ENG_CHORD);
    rnd_tx(tr, TX_CH_PROG, (int32_t)rnd_pick(pg, pw, NELEM(pg)));
    rnd_tx(tr, TX_CH_BARS, rnd_pct(60) ? 3 : 2);
    rnd_tx(tr, TX_CH_OCT, 0);
    rnd_tx(tr, TX_EV_LOCK, 0);
    if (rnd_pct(65)) {                              /* (pulsing: every step of a slower clock) */
        rnd_tx(tr, TX_CH_HIT, 2);
        rnd_tx(tr, TX_CH_BREATH, 0);
        rnd_tp(tr, P_SDIV, div - 1);
        rnd_tp(tr, P_SGATE, rnd_rr(50, 76));
    } else {                                        /* (breathing: held, each voice struck again softer) */
        rnd_tx(tr, TX_CH_HIT, 0);
        rnd_tx(tr, TX_CH_BREATH, rnd_rr(40, 80));
        rnd_tp(tr, P_SDIV, 0);
        rnd_tp(tr, P_SGATE, 127);
    }
    rnd_tp(tr, P_CHRD, rnd_pct(70) ? 1 : 2);        /* DIA3, DIA7 now and then */
}
/* the registers: the cell above middle C (its pitches C3 to B3 before; FM6's now and then at them), its third copy with
 * it or an octave over, the pulse at C2 to B2, the bed an octave under the cell; the pair panned apart */
static void rnd_phase_registers(void)
{
    int32_t cell = trk[RND_PH_A].eng_req == RND_FM6 && rnd_pct(40) ? 0 : 12;
    rnd_tp(RND_PH_A, P_TRANS, cell);
    rnd_tp(RND_PH_B, P_TRANS, cell);
    rnd_tp(RND_PH_C, P_TRANS, rnd_ph.third ? cell + (rnd_pct(30) ? 12 : 0) : -12);   /* (the pulse C2 to B2: lower,
                                                     * felt more than heard) */
    rnd_tp(RND_PH_BED, P_TRANS, cell - 12);
    rnd_tp(RND_PH_A, P_PAN, -rnd_rr(26, 40));
    rnd_tp(RND_PH_B, P_PAN, rnd_rr(26, 40));
    rnd_tp(RND_PH_C, P_PAN, rnd_rr(-8, 8));
    rnd_tp(RND_PH_BED, P_PAN, 0);
}
static void rnd_key_phase(void)
{
    static const uint8_t sc[] = {5, 6, 1, 3, 4, 9}, sw[] = {3, 2, 1, 2, 1, 1};   /* PEN MPEN MAJ DOR MIX LYD */
    int32_t scale = (int32_t)rnd_pick(sc, sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
static void rnd_roles_phase(uint8_t *role)
{
    rnd_ph.third = (uint8_t)rnd_pct(60);            /* (a TRIO cell's, rnd_phase_sound) */
    role[RND_PH_A] = RND_ROLE_MELODY;
    role[RND_PH_B] = RND_ROLE_CANON;
    role[RND_PH_C] = rnd_ph.third ? RND_ROLE_CANON : RND_ROLE_BASS;
    role[RND_PH_BED] = RND_ROLE_COUNTER;           /* (the bed last: its engine after the cell's, rnd_roll's order) */
}

/* FORGE and CHAOS (asked 2026-11-09): techno at 132 to 148 BPM, dark, hypnotic and exhilarating, no four on the floor.
 * The kick a pattern with an identity the ear learns, one of eight ideas a roll (RND_FG_IDEA): broken and syncopated,
 * the kick and the bass one rhythm, kicks missing from a four, an odd-length loop sliding against the bar, triplets, the
 * kick answering the hats, three over four, the kick an accent over a rolling low tom. The hats a steady sixteenth grid
 * (the tempo); the percussion a loop of an odd length against it (polymeter: 5, 7, 9, 11 or 13 steps); the bass stabs in
 * the kick's gaps. All on STEP, the tracks' own steps written (the patterns exact; held REC brings yours back:
 * rnd_snap.step). FORGE: dark techno: Braids' 808-like kick or a two-op FM kick, an acid or sub bass, hats, toms or FM
 * percussion. CHAOS: noisier, synthetic: crushed and driven FM kicks, metal (feedback and chaotic FM), clocked and data
 * noise for hats, ratchets and chance. The drums struck at the key's root (MACRO's STRIK DRUM) with a pitch drop: the
 * one place a roll moves pitch, drum hits. DRIFT OFF: the loops as rolled, for ever (hypnotic); SLOW: a move every 16
 * bars, FAST every 8, the arc of a set (kicks missing, a breakdown and the slam back, a fill, the percussion turned,
 * the hats opened): rnd_forge_move */
#define RND_FG_KICK 0u
#define RND_FG_BASS 1u                              /* (CHAOS: the metal, half its rolls) */
#define RND_FG_HAT 2u
#define RND_FG_PERC 3u
enum { FG_BROKEN, FG_LOCK, FG_MISSING, FG_ODD, FG_TRIPLET, FG_ANSWER, FG_THREE, FG_ACCENT, FG_IDEAS };
static struct { uint8_t idea, metal, phrase, down, fill, len[RND_NT], hits[RND_NT], rot[RND_NT]; uint16_t mask[RND_NT]; uint32_t bar; } rnd_fg;
static uint16_t rnd_fg_euclid(uint32_t len, uint32_t hits, uint32_t rot)
{
    uint32_t i, m = 0;
    for (i = 0; i < len; i++) {
        uint32_t k = (i + rot) % len;
        if ((k + 1u) * hits / len > k * hits / len) m |= 1u << i;
    }
    return (uint16_t)m;
}
/* the kick's steps from its mask: a hit where the bit is (its note C2: STRIK DRUM plays the key's root), rests else;
 * accents on acc's bits; ratchets on rat's (CHAOS) */
static void rnd_fg_write(uint32_t tr, uint32_t mask, uint32_t acc, uint32_t rat, const uint8_t *notes, uint32_t nn)
{
    track_t *t = &trk[tr];
    uint32_t i, len = rnd_fg.len[tr];
    fm1_irq_off();
    for (i = 0; i < NSTEP; i++) {
        step_t *st = &t->step[i];
        memset(st, 0, sizeof *st);
        st->time = ST_REST;
        if (i >= len || !(mask >> i & 1u)) continue;
        st->time = ST_NOTE;
        st->n = 1;
        st->note[0] = notes ? notes[i % nn] : (uint8_t)(36u + (uint32_t)t->p[P_ROOT] % 12u);   /* (GENERATE: the key's
                                                     * root, for a kick that is not Braids' drum) */
        st->vel = (uint8_t)(acc >> i & 1u ? 124u : 104u);
        if (acc >> i & 1u) st->flags |= SF_ACCENT;
        if (rat >> i & 1u) step_set_ratch(st, rnd_chaos ? 3u : 2u);
    }
    fm1_irq_on();
    rnd_tp(tr, P_SLEN, (int32_t)len);
}
static void rnd_fg_drum(uint32_t tr, uint32_t pi, int32_t dec)
{
    engine_load_of(&trk[tr], ENGI_MACRO, pi);
    rnd_tp(tr, P_E0 + 6u, 2);                       /* STRIK DRUM: at the key's root */
    rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0);
    rnd_tp(tr, P_DEC, dec); rnd_tp(tr, P_REL, dec / 2);
    rnd_tp(tr, P_VOICE, V_MONO);
}
static void rnd_forge_sound(uint32_t tr, uint32_t *oracles)
{
    track_t *t = &trk[tr];
    int mc = rnd_mc_ok(), ch = rnd_chaos | rnd_frac;   /* (FRACTURED: crushed and FM as CHAOS's) */
    rnd_tp_still(tr);
    rnd_tp(tr, P_GLIDE, 0); rnd_tp(tr, P_DETUNE, 0); rnd_tp(tr, P_CHRD, 0); rnd_tp(tr, P_MUTE, 0);
    rnd_tp(tr, P_SPRD, 0); rnd_tp(tr, P_ED_PIT, 0); rnd_tp(tr, P_ED_FLT, 0); rnd_tp(tr, P_AMODE, 0);
    if (tr == RND_FG_KICK) {                        /* the kick: Braids' 808-like, or two-op FM with a pitch drop */
        if (mc && rnd_pct(ch ? 50 : 75)) {          /* (deep, asked 2026-11-09: a long decay, dark, no click) */
            rnd_fg_drum(tr, 19, rnd_rr(80, 100));   /* BD ANALOG */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(85, 115)); /* TIMBR: its decay, long */
            rnd_tp(tr, P_E0 + 2u, rnd_rr(0, 35));   /* COLOR: its tone, dark */
        } else if (mc) {
            rnd_fg_drum(tr, 20, rnd_rr(56, 72));    /* BD DIGI: two-op FM */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(5, ch ? 45 : 22));
            rnd_tp(tr, P_E0 + 2u, rnd_rr(0, 10));
            rnd_tp(tr, P_ED_PIT, rnd_rr(26, 40));   /* (the drop: the hit's thump, deep) */
        } else {                                    /* (no MACRO: TRIO's triangles near a sine, TIDE's thump) */
            engine_load_of(t, 6u, 9u);
            rnd_tide_tri(tr, 1);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(14, 30));
            rnd_tp(tr, P_ED_PIT, rnd_rr(20, 34));
            rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0); rnd_tp(tr, P_DEC, rnd_rr(48, 62)); rnd_tp(tr, P_REL, 30);
            rnd_tp(tr, P_VOICE, V_MONO);
        }
        if (t->eng_req == ENGI_MACRO) {
            rnd_tp(tr, P_E0 + 3u, ch ? rnd_rr(84, 110) : rnd_rr(70, 94));   /* TONE: the click soft, the body */
            rnd_tp(tr, P_E0 + 4u, ch && rnd_pct(60) ? rnd_rr(2, 4) : 0);   /* BITS: CHAOS crushed */
            rnd_tp(tr, P_E0 + 7u, ch ? rnd_rr(40, 100) : rnd_rr(0, 25));   /* DRIVE */
        }
        rnd_tp(tr, P_LEVEL, rnd_rr(121, 127));      /* (louder, asked 2026-11-10: the rest 3 dB up, the kick 7) */
    } else if (tr == RND_FG_BASS && rnd_fg.metal && mc) {   /* CHAOS's metal: feedback or chaotic FM, struck, short
                                                     * (GENERATE: Braids' only; else the bass) */
        engine_load_of(t, ENGI_MACRO, rnd_pct(50) ? 8u : 9u);   /* FB GROWL, CHAOS FM */
        rnd_tp(tr, P_E0 + 1u, rnd_rr(50, 110)); rnd_tp(tr, P_E0 + 2u, rnd_rr(30, 120));
        rnd_tp(tr, P_E0 + 3u, rnd_rr(90, 120)); rnd_tp(tr, P_E0 + 4u, rnd_pct(40) ? rnd_rr(1, 3) : 0);
        rnd_tp(tr, P_E0 + 6u, 1); rnd_tp(tr, P_E0 + 7u, rnd_rr(20, 70));
        rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0); rnd_tp(tr, P_DEC, rnd_rr(25, 45)); rnd_tp(tr, P_REL, 20);
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_LEVEL, rnd_rr(100, 110));
    } else if (tr == RND_FG_BASS && rnd_frac) {     /* FRACTURED's pad: FALL's, TRIO's triangles, warm, slow in, wide */
        engine_load_of(t, 6u, 5u);                  /* WARM PAD */
        rnd_tide_tri(tr, 0);
        rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 62)); rnd_tp(tr, P_E0 + 6u, rnd_rr(5, 25));
        rnd_tp(tr, P_ATK, rnd_rr(60, 90)); rnd_tp(tr, P_DEC, 100); rnd_tp(tr, P_SUS, rnd_rr(110, 124));
        rnd_tp(tr, P_REL, rnd_rr(90, 110)); rnd_tp(tr, P_SPRD, rnd_rr(60, 100));
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(76, 84));        /* (10 dB down, asked 2026-11-16) */
    } else if (tr == RND_FG_BASS) {                 /* the bass: stabs, short, dark, its filter's envelope */
        if (*oracles < 1u && eng_ok(ENGI_ORACLE) && rnd_pct(45)) {
            engine_load_of(t, ENGI_ORACLE, rnd_pct(50) ? 8u : rnd_pct(50) ? 6u : 7u);   /* ACID LINE, FUNK BASS, SUB BASS */
            (*oracles)++;
        } else {
            engine_load_of(t, 6u, rnd_pct(50) ? 0u : 9u);   /* FAT BASS, SUB BASS */
            rnd_tp(tr, P_E0, rnd_pct(60) ? 0 : 1);  /* SAW3, PLS3 */
            rnd_tp(tr, P_E0 + 1u, 0); rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? -12 : 0);
            rnd_tp(tr, P_E0 + 3u, rnd_rr(4, 14)); rnd_tp(tr, P_E0 + 4u, 0);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(24, 46)); rnd_tp(tr, P_E0 + 6u, rnd_rr(20, 70));
            rnd_tp(tr, P_ED_FLT, rnd_rr(30, 70));   /* (the stab's bite) */
        }
        rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, rnd_rr(0, 30)); rnd_tp(tr, P_DEC, rnd_rr(35, 55)); rnd_tp(tr, P_REL, 25);
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_LEVEL, rnd_rr(96, 104));       /* (the levels, measured by their peaks: the kick about -6 dB, the
                                                     * bass 10 under, the percussion 12, the hats 16) */
    } else if (tr == RND_FG_HAT) {                  /* the hats: Braids' cymbal short, clocked or data noise, TRIO's */
        uint32_t k = (uint32_t)rnd_rr(0, 99);
        if (mc && k < (ch ? 35u : 60u)) {
            rnd_fg_drum(tr, 22, rnd_rr(14, 30));    /* HAT */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(60, 110)); rnd_tp(tr, P_E0 + 2u, rnd_rr(60, 120));
        } else if (mc) {
            rnd_fg_drum(tr, rnd_pct(60) ? 12u : 15u, rnd_rr(10, 24));   /* BIT NOISE, QPSK DATA */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(60, 127)); rnd_tp(tr, P_E0 + 2u, rnd_rr(40, 127));
            rnd_tp(tr, P_E0 + 4u, ch && rnd_pct(50) ? rnd_rr(1, 3) : 0);
        } else {
            engine_load_of(t, 6u, 12u);             /* (WIND: TRIO's noise, high-passed, short) */
            rnd_tp(tr, P_E0 + 4u, 2); rnd_tp(tr, P_E0 + 5u, rnd_rr(96, 118));
            rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0); rnd_tp(tr, P_DEC, rnd_rr(12, 26)); rnd_tp(tr, P_REL, 10);
            rnd_tp(tr, P_VOICE, V_MONO);
        }
        if (t->eng_req == ENGI_MACRO) rnd_tp(tr, P_E0 + 3u, rnd_rr(110, 127));
        rnd_tp(tr, P_LEVEL, rnd_rr(98, 106) + (t->eng_req == ENGI_MACRO && t->p[P_E0] != 21 ? 10 : 0));   /* (the noises
                                                     * 5 dB under the cymbal, measured) */
        rnd_tp(tr, P_PAN, rnd_rr(-12, 12));
    } else {                                        /* the percussion: FORGE's low tom answering the kick (asked
                                                     * 2026-11-09), CHAOS's half the rolls FM metal */
        uint32_t k = (uint32_t)rnd_rr(0, 99);
        if (mc && (!ch || k < 50u)) {
            rnd_fg_drum(tr, 21, rnd_rr(56, 80));    /* TOM: low, at the key's root (STRIK DRUM) */
            rnd_tp(tr, P_E0 + 1u, rnd_rr(45, 85)); rnd_tp(tr, P_E0 + 2u, rnd_rr(20, 70));
            rnd_tp(tr, P_ED_PIT, rnd_rr(8, 18));
        } else if (!mc && !ch) {                    /* (no MACRO: TRIO's triangles, a thump) */
            engine_load_of(t, 6u, 9u);
            rnd_tide_tri(tr, 1);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(30, 50));
            rnd_tp(tr, P_ED_PIT, rnd_rr(10, 20));
            rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0); rnd_tp(tr, P_DEC, rnd_rr(50, 64)); rnd_tp(tr, P_REL, 25);
            rnd_tp(tr, P_VOICE, V_MONO);
        } else if (mc) {
            rnd_fg_drum(tr, ch && rnd_pct(50) ? 9u : 23u, rnd_rr(22, 42));   /* CHAOS FM, FM PERC */
            rnd_tp(tr, P_E0 + 6u, 1);
            rnd_tp(tr, P_E0 + 1u, rnd_rr(50, 120)); rnd_tp(tr, P_E0 + 2u, rnd_rr(40, 127));
            rnd_tp(tr, P_E0 + 4u, ch && rnd_pct(40) ? rnd_rr(1, 4) : 0);
            rnd_tp(tr, P_E0 + 7u, ch ? rnd_rr(20, 80) : 0);
        } else {
            engine_load_of(t, 6u, 1u);              /* (TRIO: a pulse, short, in key) */
            rnd_tp(tr, P_E0, 1); rnd_tp(tr, P_E0 + 5u, rnd_rr(50, 80));
            rnd_tp(tr, P_ATK, 0); rnd_tp(tr, P_SUS, 0); rnd_tp(tr, P_DEC, rnd_rr(24, 40)); rnd_tp(tr, P_REL, 15);
            rnd_tp(tr, P_VOICE, V_MONO);
        }
        if (t->eng_req == ENGI_MACRO) rnd_tp(tr, P_E0 + 3u, t->p[P_E0] == 20 ? rnd_rr(84, 108) : rnd_rr(96, 124));
        rnd_tp(tr, P_LEVEL, t->p[P_E0] == 20 ? rnd_rr(92, 102) : rnd_rr(112, 122));   /* (the low tom 4 dB under the kick,
                                                     * FM's level the tom's, measured) */
        rnd_tp(tr, P_PAN, t->p[P_E0] == 20 ? rnd_rr(-12, 12) : rnd_pct(50) ? rnd_rr(18, 34) : -rnd_rr(18, 34));
    }
    if (tr != RND_FG_KICK) {                        /* the movement (asked 2026-11-09): a slow LFO, free, sweeping
                                                     * TIMBR and TONE (or the bass's cutoff); the cords pan the hats and
                                                     * sweep COLOR, rnd_cords_forge. The kick still, centred */
        rnd_tp(tr, P_LWAVE, rnd_pct(60) ? 1 : 0);   /* TRI, SIN */
        rnd_tp(tr, P_LSYNC, tr == RND_FG_HAT ? (rnd_pct(50) ? 3 : 2) : rnd_pct(50) ? 1 : 2);   /* (the hats a bar or two,
                                                     * the rest two or four) */
        rnd_tp(tr, P_LTRIG, 1); rnd_tp(tr, P_LFADE, 0); rnd_tp(tr, P_LPOL, 0);   /* (FREE: on NOTE each short hit
                                                     * restarted it, heard: no sweep, no pan) */
        rnd_tp(tr, P_LPHASE, rnd_rr(0, 127));
        rnd_tp(tr, P_LD_FLT, tr == RND_FG_BASS && !rnd_fg.metal ? rnd_rr(18, 36) : rnd_rr(8, 22));
        if (t->eng_req == ENGI_MACRO) rnd_tp(tr, P_LD_SHP, rnd_rr(14, 34));
    } else rnd_tp(tr, P_PAN, 0);
}
/* the cords: the hats panned by their LFO (left to right and back), COLOR swept slowly on the hats, the percussion
 * and the metal (their own cycles, OCHD, apart) */
static void rnd_cords_forge(void)
{
    cord_t c[4];
    uint32_t i, n = 0;
    memset(c, 0, sizeof c);
    c[n].src_trk = c[n].dst_trk = RND_FG_HAT; c[n].src = CS_LFO; c[n].dst = CD_PAN; c[n].amount = (int8_t)rnd_rr(18, 34); n++;   /* (side to side,
                                                     * not hard left and right, measured) */
    c[n].src_trk = c[n].dst_trk = RND_FG_HAT; c[n].src = CS_OCHD3; c[n].dst = CD_E3; c[n].amount = (int8_t)rnd_rr(20, 45); n++;
    if (trk[RND_FG_PERC].eng_req == ENGI_MACRO) {
        c[n].src_trk = c[n].dst_trk = RND_FG_PERC; c[n].src = CS_OCHD5; c[n].dst = CD_E3; c[n].amount = (int8_t)rnd_rr(15, 35); n++;
    }
    if (trk[RND_FG_BASS].eng_req == ENGI_MACRO) {
        c[n].src_trk = c[n].dst_trk = RND_FG_BASS; c[n].src = CS_OCHD4; c[n].dst = CD_E3; c[n].amount = (int8_t)rnd_rr(20, 45); n++;
    }
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t z;
        memset(&z, 0, sizeof z);
        cord_set(i, i < n ? &c[i] : &z);
    }
}
/* the patterns (asked 2026-11-09: the kick a written pattern holding the groove, the rest Euclidean): the kick's idea
 * on STEP; the bass, the hats and the percussion on EUCLID, each its steps, hits and turn (rnd_fg_euc) */
static const uint16_t FG_BROKEN_M[] = {0x2149, 0x90A1, 0x8201, 0x0945, 0x4209};   /* (as asked, 2026-11-09:) K--K --K-
                                                     * K--- -K--; A broken driving K--- -K-K ---- K--K; B sparse,
                                                     * ominous K--- ---- -K-- ---K; C rolling K-K- --K- K--K ----;
                                                     * D weird but usable K--K ---- -K-- --K- */
static const uint8_t FG_BROKEN_W[] = {1, 2, 3, 1, 3};   /* (the sparse ones most: less busy, heard) */
/* the gaps' best turn: a Euclidean loop of n steps, hits hits, meeting the mask least (a mask of 16 only) */
static uint32_t rnd_fg_gap(uint32_t n, uint32_t hits, uint32_t mask)
{
    uint32_t r, best = 0, over = 99;
    for (r = 0; r < n; r++) {
        uint32_t m = rnd_fg_euclid(n, hits, r) & mask, o = 0;
        while (m) o += m & 1u, m >>= 1;
        if (o < over || (o == over && rnd_pct(30))) over = o, best = r;
    }
    return best;
}
static void rnd_fg_euc(uint32_t tr, uint32_t n, uint32_t hits, uint32_t rot)
{
    rnd_fg.len[tr] = (uint8_t)n;
    rnd_fg.hits[tr] = (uint8_t)hits;
    rnd_fg.rot[tr] = (uint8_t)rot;
    rnd_fg.mask[tr] = rnd_fg_euclid(n, hits, rot);
    rnd_tx(tr, TX_EUC_STEPS, (int32_t)n);
    rnd_tx(tr, TX_EUC_PULSES, (int32_t)hits);
    rnd_tx(tr, TX_EUC_ROT, (int32_t)rot);
}
static void rnd_forge_patterns(void)
{
    uint32_t kick, krat = 0, n, h, best, ch = rnd_chaos | rnd_frac;
    static const uint8_t pl[] = {5, 7, 9, 11, 13}, plw[] = {2, 3, 2, 2, 1};
    rnd_fg.len[RND_FG_KICK] = 16;
    switch (rnd_fg.idea) {                          /* (the patterns asked for, 2026-11-09, and no busier) */
    case FG_BROKEN: kick = FG_BROKEN_M[rnd_pick((const uint8_t[]){0, 1, 2, 3, 4}, FG_BROKEN_W, NELEM(FG_BROKEN_W))]; break;
    case FG_LOCK: kick = 0x1041u; break;            /* K--- --K- ---- K--- (the bass: -B-- B--- --B- ..: 5, 10, 14) */
    case FG_MISSING: kick = rnd_pct(50) ? 0x1011u : 0x0501u; break;       /* K--- K--- ---- K---; K--- ---- K-K- ---- */
    case FG_ODD:                                    /* K--K--- (7), or 9, 11, 13 steps, two or three kicks */
        n = rnd_pick(pl, plw, NELEM(pl));
        if (n < 7u) n = 7u;
        rnd_fg.len[RND_FG_KICK] = (uint8_t)n;
        kick = n == 7u ? 0x09u : rnd_fg_euclid(n, n < 11u ? 2u : 3u, 0);
        break;
    case FG_TRIPLET: rnd_fg.len[RND_FG_KICK] = 12; kick = 0x0449u; break;   /* K..K..K...K. on triplets */
    case FG_ANSWER: kick = 0x2492u; break;          /* -K-- K--K --K- -K-- */
    case FG_THREE: rnd_fg.len[RND_FG_KICK] = 3; kick = 1u; break;   /* K-- on eighths: three over four, a dotted quarter */
    default: kick = 0x0001u; break;                 /* (the accent: once a bar, the low tom rolling under) */
    }
    if (ch && rnd_pct(50)) krat = kick & (0u - kick);   /* (CHAOS, FRACTURED: a ratchet on the first kick now and then) */
    rnd_fg.mask[RND_FG_KICK] = (uint16_t)kick;
    rnd_fg_write(RND_FG_KICK, kick, kick & 1u, krat, 0, 1);
    rnd_fg.mask[RND_FG_BASS] = 0;
    if (rnd_frac) ;                                 /* (FRACTURED: the pad, its chords: rnd_forge_generators) */
    else if (rnd_fg.metal) {                        /* (CHAOS's metal: a short loop of its own against the bar) */
        n = rnd_pct(50) ? 5u : 7u;
        rnd_fg_euc(RND_FG_BASS, n, 2u + rnd_next() % 2u, rnd_next() % n);
    } else {                                        /* the bass: in the kick's gaps, the turn that meets it least */
        h = (uint32_t)rnd_rr(3, rnd_fg.idea == FG_ACCENT ? 6 : 5);
        best = rnd_fg_gap(16, h, rnd_fg.len[RND_FG_KICK] == 16u ? kick : 0x5555u);   /* (a kick of its own length: the
                                                     * offbeats) */
        if (rnd_fg.idea == FG_LOCK) h = 3, best = rnd_fg_gap(16, 3, 0xFFFFu & ~0x4420u);   /* (the bass on 5, 10, 14, near) */
        rnd_fg_euc(RND_FG_BASS, 16, h, best);
    }
    if (rnd_frac) {                                 /* (FRACTURED: the hats a loop of 11, 13 or 15, sliding) */
        n = 11u + 2u * (rnd_next() % 3u);
        rnd_fg_euc(RND_FG_HAT, n, (uint32_t)rnd_rr((int32_t)n / 2, (int32_t)n - 3), rnd_next() % n);
    } else if (rnd_chaos) rnd_fg_euc(RND_FG_HAT, rnd_pct(50) ? 16u : 12u, (uint32_t)rnd_rr(9, 12), rnd_next() % 4u);   /* (broken) */
    else if (rnd_pct(65)) rnd_fg_euc(RND_FG_HAT, 16, 16, 0);   /* (every sixteenth: the tempo) */
    else rnd_fg_euc(RND_FG_HAT, 16, 8, 1);          /* (the offbeats) */
    if (trk[RND_FG_PERC].eng_req == ENGI_MACRO && trk[RND_FG_PERC].p[P_E0] == 20 && rnd_pct(rnd_fg.idea == FG_ACCENT ? 100 : 65)) {
        h = rnd_fg.idea == FG_ACCENT ? (uint32_t)rnd_rr(5, 7) : (uint32_t)rnd_rr(2, 4);   /* the low tom: in the kick's gaps,
                                                     * answering it (rolling, under an accent kick) */
        rnd_fg_euc(RND_FG_PERC, 16, h, rnd_fg_gap(16, h, (rnd_fg.len[RND_FG_KICK] == 16u ? kick : 0x1111u) | rnd_fg.mask[RND_FG_BASS]));
    } else {                                        /* (or a loop of its own length: polymeter) */
        n = rnd_pick(pl, plw, NELEM(pl));
        if (n == rnd_fg.len[RND_FG_KICK] || n == rnd_fg.len[RND_FG_BASS]) n = n == 7u ? 9u : 7u;
        rnd_fg_euc(RND_FG_PERC, n, (uint32_t)rnd_rr((int32_t)n / 3, (int32_t)n / 2 + 1), rnd_next() % n);
    }
}
static void rnd_forge_generators(void)
{
    uint32_t tr, chords = 1;
    for (tr = 0; tr < RND_NT; tr++) {       /* (the clean slate: the kick STEP, the rest EUCLID) */
        rnd_generator(tr, &chords);
        rnd_tx(tr, TX_MODE, tr == RND_FG_KICK ? TESS_ENG_OFF : TESS_ENG_EUCLID);
        rnd_tx(tr, TX_EV_EVERY, 0); rnd_tx(tr, TX_EV_LOCK, 0); rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0); rnd_tx(tr, TX_TW_FOLLOW, 0); rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0); rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_TW_HUMAN, 0); rnd_tx(tr, TX_SUB_HUMAN, 0);
        rnd_tp(tr, P_SDIV, 2);                      /* 1/16 */
        rnd_tp(tr, P_SSWING, 0);
        rnd_tp(tr, P_SGATE, tr == RND_FG_BASS ? rnd_rr(30, 60) : rnd_rr(24, 40));
    }
    {                                               /* the idea: the sparse ones most (less busy, asked 2026-11-09) */
        static const uint8_t iw[FG_IDEAS] = {4, 3, 2, 2, 1, 1, 1, 1};   /* BROKEN LOCK MISSING ODD TRIPLET ANSWER THREE ACCENT */
        static const uint8_t fw[FG_IDEAS] = {2, 0, 2, 4, 3, 1, 3, 0};   /* (FRACTURED: the odd, the broken, the triplets) */
        static const uint8_t ix[FG_IDEAS] = {0, 1, 2, 3, 4, 5, 6, 7};
        rnd_fg.idea = (uint8_t)rnd_pick(ix, rnd_frac ? fw : iw, FG_IDEAS);
    }
    rnd_forge_patterns();
    if (rnd_chaos | rnd_frac) rnd_tx(RND_FG_PERC, TX_SUB_CHANCE, rnd_rr(20, 50)), rnd_tx(RND_FG_PERC, TX_SUB_RATCH, rnd_rr(2, 3));
                                                    /* (CHAOS, FRACTURED: the percussion rattling now and then) */
    if (rnd_frac) {                                 /* FRACTURED (asked 2026-11-12): glitchy, Autechre's way: the hats
                                                     * stuttering and dropping hits, the percussion off the grid; the pad
                                                     * FALL's, slow minor chords (the bass's track) */
        rnd_tx(RND_FG_HAT, TX_SUB_CHANCE, rnd_rr(15, 40)); rnd_tx(RND_FG_HAT, TX_SUB_RATCH, rnd_rr(2, 4));
        rnd_tx(RND_FG_HAT, TX_TW_DROP, rnd_rr(10, 30)); rnd_tx(RND_FG_PERC, TX_TW_DROP, rnd_rr(10, 30));
        rnd_tp(RND_FG_PERC, P_SSWING, rnd_rr(0, 40));
        tr = 0;
        rnd_role_generator(RND_FG_BASS, RND_ROLE_CHORDS, &tr);
        rnd_tx(RND_FG_BASS, TX_CH_PROG, rnd_rr(0, 2)); rnd_tx(RND_FG_BASS, TX_CH_HIT, 0);
    }
    if (rnd_fg.idea == FG_TRIPLET) rnd_tp(RND_FG_KICK, P_SDIV, 5);   /* 16T: the kick's triplets, the hats straight */
    if (rnd_fg.idea == FG_THREE) rnd_tp(RND_FG_KICK, P_SDIV, 1);     /* 1/8: three of them over four */
    if (rnd_fg.idea == FG_TRIPLET || (!rnd_chaos && rnd_pct(25))) rnd_tp(RND_FG_HAT, P_SSWING, rnd_rr(18, 34));
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_frac ? rnd_rr(130, 150) : rnd_chaos ? rnd_rr(130, 134) : rnd_rr(128, 132));   /* (about
                                                     * 130, 2026-11-09; FRACTURED 130 to 150, asked 2026-11-16) */
    for (tr = 0; tr < RND_NT; tr++) tess_roll_apply(tr);   /* (the loops together from now) */
    rnd_fg.phrase = rnd_fg.down = rnd_fg.fill = 0;
    rnd_fg.bar = bar_n;
}
static void rnd_forge_registers(void)
{
    rnd_tp(RND_FG_KICK, P_TRANS, -12);              /* (the drums' root C2 + TRANS: the kick C1 to B1, 33 to 62 Hz) */
    rnd_tp(RND_FG_BASS, P_TRANS, rnd_fg.metal ? 12 : 0);   /* (the bass C2 up, over the kick) */
    rnd_tp(RND_FG_HAT, P_TRANS, 36);
    rnd_tp(RND_FG_PERC, P_TRANS, trk[RND_FG_PERC].p[P_E0] == 20 ? 0 : 12);   /* (the toms low-mid, FM higher) */
}
static void rnd_key_forge(void)
{
    static const uint8_t sc[] = {2, 3, 8, 10}, sw[] = {4, 2, 3, 1};   /* MIN DOR PHRY LOC; COSMIC: MIN (its phrase
                                                     * composed on it) */
    int32_t scale = rnd_cosmic ? 2 : (int32_t)rnd_pick(sc, sw, rnd_chaos ? 4u : 3u);
    int32_t root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
static void rnd_roles_forge(uint8_t *role)
{
    rnd_fg.metal = (uint8_t)(rnd_chaos && rnd_pct(50));
    role[RND_FG_KICK] = RND_ROLE_LOWPERC;
    role[RND_FG_BASS] = rnd_frac ? RND_ROLE_CHORDS : rnd_fg.metal ? RND_ROLE_GLITCH : RND_ROLE_BASS;
    role[RND_FG_HAT] = RND_ROLE_NOISE;
    role[RND_FG_PERC] = RND_ROLE_GLITCH;
}

/* COSMIC (asked 2026-11-10): instrumental synthwave after Com Truise and 80s Stallone: warm, driving, 108 to 118 BPM,
 * a minor key; no drums (the player's own). Its phrase composed (rnd_cosmic_generators), four bars on STEP: 1 the
 * bass, deep, driving eighths on each chord's root; 2 the pad, a seventh a bar, spread, held; 3 the lead, melodic, on
 * the chord's notes, its filter swept slowly; 4 the arp, the chord up and down late in the phrase. Locked, they
 * repeat. DRIFT SLOW or FAST: the arp comes and goes every 16 or 8 bars (OFF: as rolled) */
#define RND_CS_BASS 0u
#define RND_CS_PAD 1u
#define RND_CS_LEAD 2u
#define RND_CS_ARP 3u
static void rnd_roles_cosmic(uint8_t *role)
{
    role[RND_CS_BASS] = RND_ROLE_BASS; role[RND_CS_PAD] = RND_ROLE_CHORDS;
    role[RND_CS_LEAD] = RND_ROLE_MELODY; role[RND_CS_ARP] = RND_ROLE_TINT;
}
static void rnd_cosmic_sound(uint32_t tr, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles, uint32_t role)
{
    static const uint8_t op[3] = {1, 19, 22};       /* ORACLE (the pad only): WARM SWEEP PAD, FAT POLY. (Its own
                                                     * envelopes: the bass and the lead plucked, its blips jolly, heard
                                                     * 2026-11-10: TRIO's, held, and dark) */
    static const uint8_t tp[4][3] = {{0, 0, 9}, {5, 6, 8}, {10, 13, 10}, {1, 1, 0}};   /* TRIO: FAT, SUB BASS; WARM PAD,
                                                     * STRING MACH, SOFT PAD; SOFT LEAD, ANALOG BRASS; ARP LEAD, FAT */
    int orc = tr == RND_CS_PAD && eng_ok(ENGI_ORACLE) && rnd_pct(80);
    uint8_t nice = rnd_nice;
    rnd_want_e = orc ? ENGI_ORACLE : 6u;
    rnd_want_pi = orc ? op[rnd_next() % 3u] : tp[tr][rnd_next() % 3u];
    rnd_nice = 1;
    rnd_sound(tr, fm6s, phys, oracles, role);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    rnd_tp_still(tr);
    if (tr == RND_CS_BASS) {                        /* (deep and held: a sub octave under it, the filter low, barely
                                                     * opened by its envelope, sustained) */
        rnd_tp(tr, P_E0, rnd_pct(60) ? 0 : 1);      /* SAW3, PLS3 */
        rnd_tp(tr, P_E0 + 1u, rnd_pct(40) ? 12 : 0); rnd_tp(tr, P_E0 + 2u, 0);   /* (an octave over it now and then for
                                                     * its edge; the references' bass 40 to 90 Hz, measured 2026-11-10) */
        rnd_tp(tr, P_E0 + 4u, 0); rnd_tp(tr, P_E0 + 5u, rnd_rr(40, 54)); rnd_tp(tr, P_E0 + 6u, rnd_rr(10, 30));
        rnd_tp(tr, P_ED_FLT, rnd_rr(6, 16));
        rnd_tp(tr, P_VOICE, V_MONO);
        rnd_tp(tr, P_ATK, 2); rnd_tp(tr, P_DEC, rnd_rr(70, 90)); rnd_tp(tr, P_SUS, rnd_rr(100, 118)); rnd_tp(tr, P_REL, 30);
        rnd_tp(tr, P_LEVEL, rnd_rr(96, 104));
    } else if (tr == RND_CS_PAD) {                  /* (warm, wide, slow in) */
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_ATK, rnd_rr(50, 80)); rnd_tp(tr, P_SUS, 120); rnd_tp(tr, P_REL, rnd_rr(80, 100));
        rnd_tp(tr, P_SPRD, rnd_rr(80, 120)); rnd_tp(tr, P_DETUNE, rnd_rr(20, 50));
        rnd_tp(tr, P_LEVEL, rnd_rr(104, 112));
    } else {                                        /* the lead: slow, its filter swept; the arp: short, panned */
        int lead = tr == RND_CS_LEAD;
        rnd_tp(tr, P_E0 + 1u, rnd_pct(50) ? 0 : 12); rnd_tp(tr, P_E0 + 2u, rnd_pct(50) ? 0 : -12);
        rnd_tp(tr, P_E0 + 3u, rnd_rr(8, 16)); rnd_tp(tr, P_E0 + 4u, 0);
        if (lead) {                                 /* (held, soft in: unison or an octave, the filter half open, no
                                                     * envelope on it) */
            rnd_tp(tr, P_E0 + 5u, rnd_rr(76, 94)); rnd_tp(tr, P_E0 + 6u, rnd_rr(15, 35));   /* (brighter: the
                                                     * references' saws) */
            rnd_tp(tr, P_ED_FLT, 0);
        } else {                                    /* (the arp: a saw or pulse, dark, resonant, squelched) */
            rnd_tp(tr, P_E0, rnd_pct(50) ? 0 : 1);
            rnd_tp(tr, P_E0 + 5u, rnd_rr(58, 76)); rnd_tp(tr, P_E0 + 6u, rnd_rr(55, 80));
            rnd_tp(tr, P_ED_FLT, rnd_rr(16, 30));
        }
        rnd_tp(tr, P_VOICE, lead ? V_LEGATO : V_MONO);
        rnd_tp(tr, P_ATK, lead ? rnd_rr(45, 70) : 0); rnd_tp(tr, P_SUS, lead ? 120 : 0);
        rnd_tp(tr, P_DEC, lead ? 100 : rnd_rr(25, 40)); rnd_tp(tr, P_REL, lead ? rnd_rr(60, 80) : rnd_rr(20, 40));
        rnd_tp(tr, P_LSYNC, rnd_pct(50) ? 1 : 2); rnd_tp(tr, P_LTRIG, 1); rnd_tp(tr, P_LWAVE, 1);   /* (4 or 2 bars, free) */
        rnd_tp(tr, P_LD_FLT, rnd_rr(15, 30));
        rnd_tp(tr, P_PAN, lead ? 0 : rnd_pct(50) ? rnd_rr(16, 30) : -rnd_rr(16, 30));
        rnd_tp(tr, P_LEVEL, lead ? rnd_rr(80, 88) : rnd_rr(100, 108));
    }
}
/* COSMIC's phrase, composed (heard 2026-11-10: the generators' notes, at random among the chord's, aimless): four bars
 * of 16ths on the STEP sequencer, a chord a bar of a minor progression, every part on it: the bass its root in a
 * slow-funk riff (octaves, fifths, the next chord's root a 16th early), the pad its seventh voiced near the last, the
 * lead long notes landing on the chord's notes and stepping between them, the arp its notes up and down, late in the
 * phrase. Notes as played (STEP: TRANS not added) */
static const uint8_t RND_CS_MIN[7] = {0, 2, 3, 5, 7, 8, 10};
static const uint16_t RND_CS_PROG[] = {0x0526, 0x0656, 0x0354, 0x5600, 0x0056, 0x0534, 0x3056, 0x0256};   /* degrees,
                                 * bar 1 the top nibble: i VI III VII, i VII VI VII, i iv VI v, VI VII i i, i i VI VII,
                                 * i VI iv v, iv i VI VII, i III VI VII */
static const char RND_CS_BASSR[][17] = {             /* R root, O its octave, F fifth, N the next chord's root, - held,
                                                     * . rest; the driving eighths of 80s Stallone (asked 2026-11-10) */
    "R.R.R.R.R.R.R.O.", "R.O.R.O.R.O.R.O.", "R.RRR.RRR.RRR.RR", "R.R.O.R.R.R.F.O.", "R.R.R.R.O.O.R.R.",
    "R.R.R.R.R.R.O.RN", "R.R.O.R.R.O.R.RN", "R--O--R--F--R-O-"};
static const char RND_CS_LEADR[][17] = {
    "N---------------", "N-------N-------", "N-----N-----N---", "N-----------N---", "........N-------",
    "N-------....N---", "N-N-N---N-N-N---", "N---N-N-N-------"};
static int32_t rnd_cs_deg(int32_t root, int32_t d) { return root + 12 * (d / 7) + RND_CS_MIN[d % 7]; }
static int32_t rnd_cs_in(int32_t pc, int32_t lo) { return lo + ((pc - lo) % 12 + 12) % 12; }   /* pc in lo..lo+11 */
static void rnd_cs_step(uint32_t tr, uint32_t i, const int32_t *n, uint32_t k, uint32_t vel)
{
    step_t *st = &trk[tr].step[i];
    uint32_t j;
    st->time = ST_NOTE;
    st->n = (uint8_t)k;
    for (j = 0; j < k; j++) st->note[j] = (uint8_t)n[j];
    st->vel = (uint8_t)vel;
}
static struct { uint16_t prog; uint8_t br[2], lr[3], ao, ab, ag, rs; } rnd_cs;   /* the phrase's choices (DRIFT changes
                                                     * one, rnd_cs_write writes those parts again) */
static void rnd_cs_write(uint32_t parts)           /* bit tr: track tr written again from rnd_cs */
{
    static const uint8_t arpo[][8] = {{0, 1, 2, 3, 0, 1, 2, 3}, {0, 1, 2, 3, 4, 3, 2, 1}, {0, 2, 1, 3, 0, 2, 1, 3},
                                      {4, 3, 2, 1, 4, 3, 2, 1}};
    static const uint16_t arpg[] = {0xFFFF, 0xFFFF, 0x7777, 0xDBDB};
    uint32_t tr, b, i, prog = rnd_cs.prog, ag = arpg[rnd_cs.ag];
    int32_t root = trk[0].p[P_ROOT] % 12, cur = 40 + rnd_rr(0, 2);   /* (the lead: degrees 38..44 of the key) */
    fm1_irq_off();
    for (tr = 0; tr < RND_NT; tr++)
        for (i = 0; (parts >> tr & 1u) && i < NSTEP; i++) {
            memset(&trk[tr].step[i], 0, sizeof(step_t));
            trk[tr].step[i].time = ST_REST;
        }
    for (b = 0; b < 4u; b++) {
        int32_t d = (int32_t)(prog >> (12u - 4u * b) & 0xFu), nd = (int32_t)(prog >> (12u - 4u * ((b + 1u) & 3u)) & 0xFu);
        int32_t ch[5], r = rnd_cs_in(rnd_cs_deg(root, d), 28), k, t;
        const char *bp = RND_CS_BASSR[rnd_cs.br[b == 3u]];
        const char *lp = RND_CS_LEADR[b == 3u || (b == 2u && rnd_cs.lr[2] == 0u) ? rnd_cs.lr[1] : rnd_cs.lr[0]];
        uint32_t base = 16u * b;
        t = rnd_cs_in(rnd_cs_deg(root, d), 48) - rnd_cs_deg(root, d);   /* (the pad: its seventh spread, 1 5 7 and the
                                                     * third an octave up, the root 48..59, over the bass) */
        for (k = 0; k < 4; k++) ch[k] = rnd_cs_deg(root, d + "\0\4\6\11"[k]) + t;
        if (parts >> RND_CS_PAD & 1u) {
            rnd_cs_step(RND_CS_PAD, base, ch, 4, 100);
            if (rnd_cs.rs) rnd_cs_step(RND_CS_PAD, base + 10u, ch, 4, 84);
            for (i = 1; i < 16u; i++)
                if (trk[RND_CS_PAD].step[base + i].time == ST_REST) trk[RND_CS_PAD].step[base + i].time = ST_TIE;
        }
        for (k = 0; k < 4; k++) ch[k] = rnd_cs_in(ch[k], 67);   /* (the arp: 67..78, in order, the root again on top) */
        for (k = 1; k < 4; k++)
            for (i = (uint32_t)k; i > 0u && ch[i - 1u] > ch[i]; i--) { t = ch[i]; ch[i] = ch[i - 1u]; ch[i - 1u] = t; }
        ch[4] = ch[0] + 12;
        for (i = 0; i < 16u; i++) {
            char c = bp[i];
            if (!(parts >> RND_CS_BASS & 1u)) ;
            else if (c == '-' || c == '.') trk[RND_CS_BASS].step[base + i].time = c == '-' ? ST_TIE : ST_REST;
            else {
                t = c == 'O' ? r + 12 : c == 'F' ? rnd_cs_in(rnd_cs_deg(root, d + 4), r) :
                    c == 'N' ? rnd_cs_in(rnd_cs_deg(root, nd), 28) : r;
                rnd_cs_step(RND_CS_BASS, base + i, &t, 1, i ? 96u : 116u);
            }
            if ((parts >> RND_CS_ARP & 1u) && (rnd_cs.ab >> b & 1u) && (ag >> i & 1u))
                rnd_cs_step(RND_CS_ARP, base + i, &ch[arpo[rnd_cs.ao][i & 7u]], 1, i & 3u ? 88u : 104u);
            c = lp[i];
            if (!(parts >> RND_CS_LEAD & 1u)) ;
            else if (c == '-') trk[RND_CS_LEAD].step[base + i].time = ST_TIE;
            else if (c == 'N') {                    /* (the bar's first: the chord's note nearest, else a step or two) */
                int32_t x = cur, o, dir = rnd_pct(50) ? 1 : -1;
#define RND_CS_CT(x) ((((x) - d) % 7 + 7) % 7 % 2 == 0)   /* (a degree of the chord: its 1 3 5 7) */
                if (i && lp[i - 1u] != '.') x = cur + (rnd_pct(70) ? 1 : 2) * (cur > 42 ? -1 : cur < 40 ? 1 : dir);
                else if (!(RND_CS_CT(cur) && rnd_pct(40)))
                    for (o = 1; o <= 3; o++) {
                        if (RND_CS_CT(cur + dir * o)) { x = cur + dir * o; break; }
                        if (RND_CS_CT(cur - dir * o)) { x = cur - dir * o; break; }
                    }
                cur = x > 44 ? x - 7 : x < 38 ? x + 7 : x;
                t = rnd_cs_deg(root, cur) - (root >= 7 ? 12 : 0);   /* (about the same height in every key, over the
                                                     * pad: 60..81) */
                rnd_cs_step(RND_CS_LEAD, base + i, &t, 1, 100);
            }
        }
    }
    fm1_irq_on();
}
static void rnd_cs_pick(uint32_t what)             /* bit 0 the progression, 1 the bass's riffs, 2 the lead's rhythms,
                                                     * 3 the arp */
{
    static const uint8_t arpb[] = {0xC, 0x8, 0xA, 0xF}, bars_w[] = {4, 2, 2, 1};
    if (what & 1u) rnd_cs.prog = RND_CS_PROG[rnd_next() % NELEM(RND_CS_PROG)];
    if (what & 2u) { rnd_cs.br[0] = (uint8_t)(rnd_next() % NELEM(RND_CS_BASSR)); rnd_cs.br[1] = (uint8_t)(rnd_next() % NELEM(RND_CS_BASSR)); }
    if (what & 4u) {
        rnd_cs.lr[0] = (uint8_t)(rnd_next() % NELEM(RND_CS_LEADR)); rnd_cs.lr[1] = (uint8_t)(rnd_next() % NELEM(RND_CS_LEADR));
        rnd_cs.lr[2] = (uint8_t)(rnd_next() % 4u);
    }
    if (what & 8u) {
        rnd_cs.ao = (uint8_t)(rnd_next() % 4u); rnd_cs.ab = (uint8_t)rnd_pick(arpb, bars_w, 4u);
        rnd_cs.ag = (uint8_t)(rnd_next() % 4u);
    }
}
static void rnd_cosmic_generators(const uint8_t *role)
{
    uint32_t tr;
    (void)role;
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tx(tr, TX_MODE, TESS_ENG_OFF);
        rnd_tx(tr, TX_EV_EVERY, 0); rnd_tx(tr, TX_TW_DROP, 0); rnd_tx(tr, TX_TW_HUMAN, 0); rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tp(tr, P_SDIV, 2); rnd_tp(tr, P_SLEN, 64); rnd_tp(tr, P_TRANS, 0); rnd_tp(tr, P_CHRD, 0);
        rnd_tp(tr, P_SGATE, tr == RND_CS_ARP ? rnd_rr(35, 55) : tr == RND_CS_BASS ? rnd_rr(70, 90) : 100);
    }
    rnd_cs_pick(15u);
    rnd_cs.rs = (uint8_t)rnd_pct(30);
    rnd_cs_write(15u);
    pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(108, 118));   /* (driving, not too fast: 80s Stallone 115 to 126) */
    for (tr = 0; tr < RND_NT; tr++) tess_roll_apply(tr);
    rnd_fg.bar = bar_n;
    rnd_fg.phrase = 0;
}

/* a CHORD track's chord known at once, before it first strikes (its lowest note for FOLLOW, its notes for QNT CHRD):
 * else a track before it in the order (TIDE's sub, a HALO bell) takes the last roll's for a step, measured (a sub on
 * B3). As tessera.c tess_chord_play keeps them */
static void rnd_chord_prime(void)
{
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++) {
        const track_t *t = &trk[tr];
        uint8_t out[CHORD_MAX];
        int32_t r, x;
        uint16_t mask;
        uint32_t n;
        if (tessera.mode[tr] != TESS_ENG_CHORD) continue;
        n = tess_chord_build(t, gen_chord_degree(&tessera.chord[tr], tessera.chord[tr].idx), out, &r, &mask);
        if (!n) continue;
        x = (int32_t)out[0] + 12 * tessera.chord[tr].oct + t->p[P_TRANS] + tessera.evolve[tr].shift;
        tessera.last_note[tr] = (uint8_t)(x < 0 ? 0 : x > 127 ? 127 : x);
        r += t->p[P_TRANS] + tessera.evolve[tr].shift;
        mask = mask ? mask : 1u;
        tessera.chord_pc[tr] = (uint16_t)(((uint32_t)mask << ((r % 12 + 12) % 12) |
                                           (uint32_t)mask >> (12 - (r % 12 + 12) % 12)) & 0xFFFu);
    }
}

/* ------------------------------------------------------------ the roll: what the FM-1 can play --- */
/* BUDGET (GLO > GLOBAL, the voices' shared pool, 16 by default) during rolls: 12 leaves the FM-1 headroom (MANUAL "How
 * many voices at once"). Only ever lowered, and saved only when it changes: a BUDGET set lower stays */
#define RND_BUDGET 12
enum { RND_SLOW, RND_MID, RND_FAST };
static uint32_t rnd_speed(uint32_t div)            /* P_SDIV: 1/4 1/8 1/16 1/32 8T 16T 1/2 1/1 2BAR 4BAR */
{
    return div == 2u || div == 3u || div == 4u || div == 5u ? RND_FAST : div == 1u ? RND_MID : RND_SLOW;
}
static void rnd_cap(uint32_t tr, uint32_t id, int32_t top)
{
    if (trk[tr].p[id] > top) rnd_tp(tr, id, top);
}
/* a track's tail (how long a note rings after its gate), 0..127, wherever its engine keeps it: FM6 and ORACLE have
 * their own envelopes (ownenv) and leave the track's ENV RELEASE alone. FM6: AEG (+ slower carriers' release, -64..63);
 * ORACLE: its patch's amp release; the others: the track's RELEASE */
static int32_t rnd_tail(uint32_t tr)
{
    const track_t *t = &trk[tr];
    if (t->eng_req == RND_FM6) return t->p[P_E0 + 7u] + 64;
    if (t->eng_req == ENGI_ORACLE) return or_patch[tr][OR_RELEASE_AMP];
    return t->p[P_REL];
}
static void rnd_tail_cap(uint32_t tr, int32_t top)
{
    track_t *t = &trk[tr];
    if (t->eng_req == RND_FM6) {
        rnd_cap(tr, P_E0 + 7u, top - 64);
    } else if (t->eng_req == ENGI_ORACLE) {
        uint8_t *p = or_patch[tr];
        if (p[OR_RELEASE_AMP] > top) p[OR_RELEASE_AMP] = (uint8_t)top;
        if (p[OR_RELEASE_FILTER] > top) p[OR_RELEASE_FILTER] = (uint8_t)top;
    } else {
        rnd_cap(tr, P_REL, top);
    }
}
/* a track after its sound and generator: the faster it plays, the shorter its notes, so its voices are free again
 * before the next ones (a voice ringing in its release still holds a place in the pool) */
static void rnd_cpu(uint32_t tr)
{
    track_t *t = &trk[tr];
    uint32_t e = t->eng_req, sp;
    if (!RND_COST) return;                          /* (GENERATE: a target without a voice budget plays them all) */
    if (e == RND_FM6 && rnd_speed((uint32_t)t->p[P_SDIV]) == RND_FAST)   /* (FM6 is the heaviest: 1/8 at most) */
        rnd_tp(tr, P_SDIV, rnd_pct(50) || rnd_perc(tr) ? 1 : 0);
    sp = rnd_speed((uint32_t)t->p[P_SDIV]);
    if (sp != RND_SLOW) {
        if (!rnd_loops) rnd_tail_cap(tr, sp == RND_FAST ? 55 : 85);   /* (LOOPS: its tails long; few notes, measured
                                                     * 74 a minute on all four, BUDGET keeping the voices) */
        rnd_cap(tr, P_SGATE, sp == RND_FAST ? 90 : 127);
        rnd_tx(tr, TX_SUB_RATCH, 0);                /* (ratchets only on a slow track) */
        if (t->p[P_CHRD] && !((rnd_crate || rnd_cosmic) && tessera.mode[tr] == TESS_ENG_CHORD))
            rnd_tp(tr, P_CHRD, 0);                  /* (a chord: three voices a note; CRATE's keys' CHRD: their
                                                     * sevenths, a voice more, not a chord on every note) */
        if (tessera.mode[tr] == TESS_ENG_CHORD && pid_get(PID_TESS(TX_CH_HIT), tr) == 2)
            rnd_tx(tr, TX_CH_HIT, 1);               /* (CHORD's hits: a beat's, not every step's) */
        if (sp == RND_FAST)
            rnd_tx(tr, TX_NL_MODE, 0);              /* (LENGTH: the step's, not long FIXED notes) */
        else if (pid_get(PID_TESS(TX_NL_LEN), tr) > 3)
            rnd_tx(tr, TX_NL_LEN, 3);               /* (FIXED at most a 1/4) */
    }
    if (tessera.mode[tr] == TESS_ENG_CHORD)          /* (a chord holds three or four voices: a shorter tail) */
        rnd_tail_cap(tr, 75);
    else if (sp == RND_FAST && tessera.mode[tr] != TESS_ENG_POLY && tessera.mode[tr] != TESS_ENG_QUAD && rnd_pct(50))
        rnd_tp(tr, P_VOICE, rnd_pct(50) || rnd_perc(tr) ? V_MONO : V_LEGATO);   /* (a line on one
                                                     * voice: nothing overlaps; GLITCH's hits each their attack) */
    if (e == RND_FM6) {
        rnd_tail_cap(tr, 90);                       /* (AEG at most +26: FM6's pads ring, but not for ever) */
        if (t->p[P_CHRD] && !(rnd_crate && tessera.mode[tr] == TESS_ENG_CHORD)) rnd_tp(tr, P_CHRD, 0);
    }
}
/* very long releases (past RND_REL_LONG) on two tracks at most: the rest of the pool stays free for new notes */
#define RND_REL_LONG 100
static void rnd_tails(void)
{
    uint32_t tr, n = 0;
    if (!RND_COST) return;
    for (tr = 0; tr < RND_NT; tr++) {
        track_t *t = &trk[tr];
        (void)t;
        if (rnd_loops || rnd_tail(tr) <= RND_REL_LONG || ++n <= 2u) continue;   /* (LOOPS: all four long) */
        rnd_tail_cap(tr, rnd_rr(50, RND_REL_LONG));
    }
}

/* one key and scale for all four: modes mostly, often something stranger (EDOs, JI, gamelan, maqam) */
static void rnd_key(void)
{
    static const uint8_t modal[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    const param_desc_t *d = pid_desc(PID_TRACK(P_SCALE), 0);
    int32_t top = d ? d->max : 14, scale, root = rnd_rr(0, 11);
    uint32_t tr;
    scale = rnd_pct(55) ? modal[rnd_next() % NELEM(modal)] : rnd_rr(16, top);
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
    }
    scale_share(&trk[0]);
}

/* each track an octave of its own (TRN), dealt at random whatever its role: four of -24 -12 0 +12 +24, the one left
 * out most often +24 (a high drone or pad shrill), shuffled over the tracks; now and then one a fifth up from its
 * octave. The voices spread over the registers and none covers another */
/* FALL's registers, by role (asked 2026-11-09: each its own space, warmer, no rumble): the drone an octave over the
 * old low one (its body about 110 Hz), the chords where they were, the two melodies an octave over the chords (one of
 * them two, now and then) */
static void rnd_fall_registers(const uint8_t *role)
{
    uint32_t tr, mels = 0;
    for (tr = 0; tr < RND_NT; tr++)
        rnd_tp(tr, P_TRANS, role[tr] == RND_ROLE_DRONE || role[tr] == RND_ROLE_CHORDS ? 0 :
                            mels++ && rnd_pct(30) ? 24 : 12);
}
static void rnd_registers(void)
{
    static const uint8_t nice_w[] = {0, 0, 0, 0, 1};    /* (NICE: +24 never) */
    const uint8_t *drop_w = rnd_orbit ? (const uint8_t[]){1, 0, 0, 0, 0} : rnd_nice ? nice_w : (const uint8_t[]){2, 1, 1, 1, 5};   /* (ORBIT:
                                                     * -24 never, the low one muddy, 2026-11-09) */
    static const uint8_t idx[] = {0, 1, 2, 3, 4};
    int8_t oct[5] = {-24, -12, 0, 12, 24}, deal[RND_NT];
    uint32_t drop = rnd_pick(idx, drop_w, 5), k, n = 0;
    for (k = 0; k < 5u; k++) if (k != drop && n < RND_NT) deal[n++] = oct[k];
    for (k = RND_NT - 1u; k > 0u; k--) {
        uint32_t j = rnd_next() % (k + 1u);
        int8_t v = deal[k]; deal[k] = deal[j]; deal[j] = v;
    }
    for (k = 0; k < RND_NT; k++) rnd_tp(k, P_TRANS, deal[k] + (rnd_pct(10) && deal[k] < 24 ? 7 : 0));
}

/* AMBIENT's key: modes mostly (MIN DOR MIX PEN MPEN PHRY LYD MAJ), now and then a stranger one; QNT CHRD on every
 * track, so the drone and the melodies follow the chords */
static void rnd_key_ambient(void)
{
    static const uint8_t sc[] = {2, 3, 4, 5, 6, 8, 9, 1}, sw[] = {4, 5, 3, 3, 4, 2, 3, 2};
    const param_desc_t *d = pid_desc(PID_TRACK(P_SCALE), 0);
    int32_t top = d ? d->max : 14, scale = rnd_nice || rnd_pct(80) ? (int32_t)rnd_pick(sc, sw, NELEM(sc)) : rnd_rr(16, top);
    int32_t root = rnd_rr(0, 11);                       /* (NICE: the modes always, no stranger tuning) */
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, Q_CHRD);
    }
    scale_share(&trk[0]);
}
/* LOOPS' key: the pentatonics (PEN MPEN); ORBIT's the bright modes (MAJ LYD PEN MIX DOR MPEN MIN); the generators' own
 * notes in it (QNT as by default) */
static void rnd_key_loops(void)
{
    static const uint8_t sc[] = {1, 9, 5, 4, 3, 6, 2}, sw[] = {4, 4, 4, 2, 2, 2, 1};
    static const uint8_t lw[] = {0, 0, 3, 0, 0, 2, 0};   /* (LOOPS: the pentatonics only, PEN MPEN: nothing clashes) */
    int32_t scale = (int32_t)rnd_pick(sc, rnd_loops ? lw : sw, NELEM(sc)), root = rnd_rr(0, 11);
    uint32_t tr;
    rnd_tx(0, TX_KEY_LINK, 1);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tp(tr, P_SCALE, scale);
        rnd_tp(tr, P_ROOT, root);
        rnd_tp(tr, P_QUANT, TP[P_QUANT].def);
    }
    scale_share(&trk[0]);
}
/* LOOPS (after Eno's Music for Airports): each track a EUCLID loop of a length of its own (7 9 11 13 15 17 19 23: none
 * a multiple of another but 9 15), one to three notes in it, all on one slow DIV, so the loops drift against each other
 * and seldom line up the same way; the seed's pitches, the loop as it is (EVOLVE now and then, every 16 bars, a little);
 * nothing dropped, no ratchet, swing or FORM. The first loop starts on its first step (no long silence) */
static void rnd_loops_generators(void)
{
    static const uint8_t lens[] = {7, 9, 11, 13, 15, 17, 19, 23};
    static const uint8_t dv[] = {0, 1, 6}, dw[] = {5, 3, 3};   /* 1/4 1/8 1/2 */
    uint8_t pool[NELEM(lens)];
    uint32_t tr, k, n = NELEM(lens), div = rnd_pick(dv, dw, NELEM(dv));
    memcpy(pool, lens, sizeof pool);
    for (tr = 0; tr < RND_NT; tr++) {
        int32_t len;
        k = rnd_next() % n;
        len = pool[k];
        pool[k] = pool[--n];
        rnd_tx(tr, TX_MODE, TESS_ENG_EUCLID);
        rnd_tx(tr, TX_EUC_STEPS, len);
        rnd_tx(tr, TX_EUC_PULSES, rnd_rr(1, 3));
        rnd_tx(tr, TX_EUC_ROT, tr == 0u ? 0 : rnd_rr(0, len - 1));
        rnd_tx(tr, TX_EV_SEED, rnd_seed_pick());
        rnd_tx(tr, TX_EV_EVERY, rnd_pct(30) ? 5 : 0);
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 20));
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_DROP, 0);
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 20));
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 15));
        rnd_tp(tr, P_SDIV, (int32_t)div);
        rnd_tp(tr, P_SGATE, rnd_rr(100, 127));
        rnd_tp(tr, P_SSWING, 0);
    }
}
/* ORBIT: BOUNCE or CONSTELLATION on each track, at a middling density (busier than LOOPS): BOUNCE two or three balls at
 * a moderate SPEED (one in five FALL's leaves instead), CONSTELLATION a middling sky, its ship steered by AUTO; 1/8
 * mostly; EVOLVE now and then, a little dropped now and then, no ratchet, swing or FORM */
static void rnd_orbit_generators(void)
{
    static const uint8_t dv[] = {1, 2, 0}, dw[] = {6, 3, 1};   /* 1/8 1/16 1/4 */
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++) {
        if (rnd_pct(50)) {
            rnd_tx(tr, TX_MODE, TESS_ENG_BOUNCE);
            rnd_tx(tr, TX_BNC_BALLS, rnd_pct(20) ? 5 : rnd_rr(2, 3));
            rnd_tx(tr, TX_BNC_SPEED, rnd_rr(16, 40));
            rnd_tx(tr, TX_BNC_ANGLE, rnd_rr(15, 75));
        } else {
            rnd_tx(tr, TX_MODE, TESS_ENG_CONSTEL);
            rnd_tx(tr, TX_CS_Y, rnd_rr(3, 11));
            rnd_tx(tr, TX_CS_DENS, rnd_rr(45, 75));
            rnd_tx(tr, TX_CS_SIZE, rnd_rr(3, 6));
            rnd_tx(tr, TX_CS_AUTO, rnd_rr(1, 10));
            rnd_tx(tr, TX_CS_LEN, rnd_rr(8, 16));
            rnd_tx(tr, TX_CS_FILL, rnd_pct(25));
        }
        rnd_tx(tr, TX_EV_SEED, rnd_seed_pick());
        rnd_tx(tr, TX_EV_EVERY, rnd_pct(50) ? rnd_rr(2, 4) : 0);
        rnd_tx(tr, TX_EV_AMOUNT, rnd_rr(10, 30));
        rnd_tx(tr, TX_EV_LOCK, rnd_rr(40, 90));
        rnd_tx(tr, TX_FM_FORM, 0);
        rnd_tx(tr, TX_TW_SHIFT, 0);
        rnd_tx(tr, TX_TW_DROP, rnd_pct(20) ? rnd_rr(5, 10) : 0);
        rnd_tx(tr, TX_TW_HUMAN, rnd_rr(0, 25));
        rnd_tx(tr, TX_TW_FOLLOW, 0);
        rnd_tx(tr, TX_TW_BRANCH, 0); rnd_tx(tr, TX_TW_BR_TO, 0); rnd_tx(tr, TX_TW_LOGIC, 0); rnd_tx(tr, TX_TW_COND, 0);
        rnd_tx(tr, TX_NL_MODE, 0);
        rnd_tx(tr, TX_SUB_RATCH, 0);
        rnd_tx(tr, TX_SUB_HUMAN, rnd_rr(0, 15));
        rnd_tp(tr, P_SDIV, (int32_t)rnd_pick(dv, dw, NELEM(dv)));
        rnd_tp(tr, P_SGATE, rnd_rr(80, 127));
        rnd_tp(tr, P_SSWING, 0);
    }
}

/* LOOPS' voices, after Music for Airports: a felt piano (PHYS' hammered string as WOZZA's, rnd_wozza_piano; FM6's
 * tines without PHYS), two warm pads (HALO's triangles, rnd_halo_tri) and a glass (FALL's triangles, struck, high),
 * panned apart; their envelopes long and soft (rnd_loops_sound), their registers set (rnd_loops_registers). (The stranger
 * waves and the PHYS mix before: random and harsh, heard) */
static uint8_t rnd_lp_phys, rnd_lp_glass;           /* the piano's track, the glass' (rnd_roll) */
static void rnd_loops_voice(uint32_t tr, uint32_t *fm6s, uint32_t *phys, uint32_t *oracles)
{
    uint8_t nice = rnd_nice;
    int piano = tr == rnd_lp_phys, ph = piano && !*phys && rnd_phys_ok();
    int orc = piano && !ph && !*oracles && eng_ok(ENGI_ORACLE), fm = piano && !ph && !orc && !*fm6s && rnd_fm6_ok();
    rnd_want_e = ph ? ENGI_PHYS : orc ? ENGI_ORACLE : fm ? ENGI_FM6 : 6u;
    rnd_want_pi = ph ? 12u : orc ? 9u : fm ? 0u : 5u;   /* (the hammered string; ORACLE's PLUCK, ringing; TINE EP;
                                                     * TRIO's WARM PAD, made below) */
    rnd_nice = 1;
    rnd_sound(tr, fm6s, phys, oracles, RND_ROLE_FREE);
    rnd_nice = nice;
    rnd_want_e = rnd_want_pi = 0xFFu;
    rnd_tp_still(tr);
    rnd_tp(tr, P_LTRIG, 1);                         /* (FREE: never started again under a ringing note) */
    rnd_tp(tr, P_PAN, rnd_rr(-45, 45));
    rnd_tp(tr, P_SPRD, rnd_rr(30, 70));
    rnd_tp(tr, P_CHRD, 0);
    rnd_tp(tr, P_LD_PIT, 0);                        /* (no vibrato: rnd_no_pitch. Before, the least there was: +-2 at NICE's rate
                                                     * was a wobble of 40 cents, heard: far too much) */
    rnd_tp(tr, P_LRATE, rnd_rr(2, 12));
    if (trk[tr].eng_req == RND_PHYS) rnd_wozza_piano(tr);
    else if (trk[tr].eng_req == RND_FM6) {
        uint32_t k;
        for (k = 0; k < 6u; k++)
            if (k != 2u && k != 4u) rnd_tp(tr, P_E0 + k, 0);   /* (the preset's algorithm, feedback, ratios: CRATE's rule) */
        rnd_tp(tr, P_E0 + 2u, rnd_rr(-20, 0));      /* MLVL: mellow */
    } else if (trk[tr].eng_req == 6u) {
        rnd_halo_tri(tr);                           /* (TRI3, warm, a little haze) */
        rnd_tp(tr, P_E0 + 3u, rnd_rr(3, 10));       /* (DTN: half HALO's, its beating a wobble too) */
        if (tr == rnd_lp_glass) rnd_tp(tr, P_E0 + 5u, rnd_rr(70, 90));   /* (the glass: brighter, struck) */
    }
}
/* LOOPS' registers: a pad low (-12), the piano in the middle, a pad above it (+12), the glass high (+24) */
static void rnd_loops_registers(void)
{
    uint32_t tr, pads = 0;
    for (tr = 0; tr < RND_NT; tr++)
        rnd_tp(tr, P_TRANS, tr == rnd_lp_glass ? 24 : tr == rnd_lp_phys ? 0 : pads++ ? 12 : -12);
}
/* LOOPS' envelopes long and soft, sustained (ORBIT's: a soft onset and a long tail), POLY, no glide */
static void rnd_loops_sound(uint32_t tr)
{
    track_t *t = &trk[tr];
    if (rnd_loops) {
        static const int8_t pad[][3] = {{P_ATK, 60, 90}, {P_DEC, 105, 127}, {P_SUS, 95, 120}, {P_REL, 108, 127},
                                        {P_LEVEL, 80, 92}};   /* (the pads: a slow swell, held, a long tail) */
        static const int8_t hit[][3] = {{P_ATK, 28, 48}, {P_DEC, 105, 127}, {P_SUS, 70, 105}, {P_REL, 108, 127},
                                        {P_LEVEL, 84, 98}};   /* (the piano, the glass: a soft onset, ringing long) */
        int struck = tr == rnd_lp_phys || tr == rnd_lp_glass;
        rnd_tp_rr(tr, struck ? hit : pad, struck ? NELEM(hit) : NELEM(pad));
        if (t->eng_req == RND_PHYS) rnd_tp(tr, P_LEVEL, t->p[P_LEVEL] + 10);   /* (quieter at a level, measured) */
        rnd_tp(tr, P_VOICE, V_POLY);
        rnd_tp(tr, P_GLIDE, 0);
        return;
    }
    {
        static const int8_t rg[][3] = {{P_ATK, 10, 60}, {P_DEC, 60, 127}, {P_SUS, 40, 110}, {P_REL, 90, 127}};
        rnd_tp_rr(tr, rg, NELEM(rg));
    }
    rnd_tp(tr, P_VOICE, V_POLY);
    rnd_tp(tr, P_GLIDE, 0);
    rnd_tp(tr, P_LEVEL, rnd_rr(80, 96));
    if (t->eng_req == ENGI_ORACLE) {
        uint8_t *p = or_patch[tr];
        p[OR_ATTACK_AMP] = (uint8_t)rnd_rr(10, 60);
        p[OR_RELEASE_AMP] = (uint8_t)rnd_rr(90, 127);
        p[OR_RELEASE_ON] = 1;
    }
}

/* The effects of the modes after RND (AMBIENT .. PULSE), the light ones only (rnd_fx_light): send effect 1 always,
 * send 2 most rolls (SPLIT: a send to each; the two never alike), send 3 off, then FX 1 on two tracks at most (the
 * tracks in a random order) while points are left, each of a kind by the mode's weights among those its points allow.
 * The points as RND's (rnd_fx_points, a send's bus counted), the mode's by what its engines leave (rnd_fx_mode_points:
 * AMBIENT and NICE heavy before any effect, PULSE and GLITCH light; the three sends and four inserts at 14 to 20 points
 * had taken the FM-1 to 70 or 80 %); FX 2 and 3 off. Cords modulate them: about half the inserts and up to two of the
 * send effects, a slow source in the gentle modes, a fast one (S&H, the gate, the envelope, fast chaos) in GLITCH and
 * PULSE. The guard (rnd_guard) trims them as RND's if the FM-1 runs hot. */
#define RND_FX_MODE_POINTS 13u                     /* (the most: WOZZA's) */
static uint32_t rnd_fx_mode_points(void)
{
    return rnd_wozza ? 13u : rnd_fall ? 10u : rnd_loops || rnd_orbit ? 9u : 8u;   /* (NICE: 8; WOZZA: its own,
                                                     * rnd_fx_wozza) */
}
static int rnd_cord_add(const cord_t *c)           /* (the first free cord) */
{
    uint32_t i;
    for (i = 0; i < CORD_LIVE; i++)
        if (!cord_get(i)->amount && cord_get(i)->dst == CD_OFF) return cord_set(i, c);
    return 0;
}
static void rnd_fx_mode(void)
{
    static const uint8_t fx[] = {FX_ECHO, RND_ROOM, FX_TAPE, RND_CHORUS, FX_FILTER};
    /*                                   ECH ROO TAP CHO FIL */
    static const uint8_t gs[5] = {8, 10, 3, 4, 0}, gi[5] = {5, 0, 6, 6, 6};    /* gentle */
    static const uint8_t es[5] = {10, 6, 3, 1, 3}, ei[5] = {6, 0, 4, 2, 10};   /* GLITCH PULSE */
    static const uint8_t ssrc[] = {CS_DRIFT, CS_TORPOR, CS_APATHY, CS_INERTIA, CS_LFO, CS_OCHD1, CS_OCHD3};
    static const uint8_t fsrc[] = {CS_SH, CS_LFO, CS_GATE, CS_ENV, CS_OCHD5, CS_OCHD7, CS_OCHD8, CS_NOTE};
    const int el = 0;                               /* (GLITCH, PULSE, the fast sources' modes: gone) */
    const uint8_t *ws = el ? es : gs, *wi = el ? ei : gi;
    uint8_t w[5], tracks[RND_NT] = {0, 1, 2, 3};
    uint32_t used = 0, s, k, tr, n_sendmods = 0, n_ins = 0, pts = rnd_fx_mode_points();
    pid_set(PID_GLOBAL(G_FXMODE), 0, 1);
    for (s = 0; s < 3u; s++) {                      /* the send effects */
        uint32_t base = G_MFXTYPE + s * 4u, type = FX_OFF, j;
        if (s == 0u || (s == 1u && rnd_pct(70))) {
            for (k = 0, j = 0; k < 5u; k++) {
                w[k] = ws[k];
                if (s && song.g[G_MFXTYPE] == fx[k]) w[k] = 0;   /* (never alike) */
                if (used + rnd_fx_points(fx[k], 1) > pts) w[k] = 0;
                j += w[k];
            }
            type = j ? rnd_pick(fx, w, 5u) : FX_OFF;   /* (nothing the points allow: off) */
        }
        pid_set(PID_GLOBAL(base), 0, (int32_t)type);
        pid_set(PID_GLOBAL(base + 1u), 0, rnd_rr(60, 120));
        pid_set(PID_GLOBAL(base + 2u), 0, rnd_rr(0, 127));
        pid_set(PID_GLOBAL(base + 3u), 0, rnd_rr(20, 120));
        used += rnd_fx_points(type, 1);
        if (type != FX_OFF && n_sendmods < 2u && rnd_pct(50)) {   /* (a cord on it) */
            cord_t c;
            memset(&c, 0, sizeof c);
            c.src_trk = c.dst_trk = (uint8_t)(rnd_next() % RND_NT);
            c.src = el ? fsrc[rnd_next() % NELEM(fsrc)] : ssrc[rnd_next() % NELEM(ssrc)];
            c.dst = (uint8_t)(CD_SF1_MIX + s * 3u + (uint32_t)rnd_rr(0, 2));
            c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(el ? 25 : 15, el ? 70 : 45) : -rnd_rr(el ? 25 : 15, el ? 70 : 45));
            if (rnd_cord_add(&c)) n_sendmods++;
        }
    }
    pid_set(PID_GLOBAL(G_FXRETURN), 0, rnd_rr(60, 100));
    for (k = RND_NT - 1u; k > 0u; k--) {              /* (the inserts: the tracks in a random order) */
        uint32_t j = rnd_next() % (k + 1u);
        uint8_t v = tracks[k]; tracks[k] = tracks[j]; tracks[j] = v;
    }
    for (tr = 0; tr < RND_NT; tr++) {
        uint32_t t = tracks[tr], type, j;
        for (k = 0, j = 0; k < 5u; k++) {
            w[k] = used + rnd_fx_points(fx[k], 0) <= pts ? wi[k] : 0u;
            j += w[k];
        }
        type = j && n_ins < RND_INSERT_TRACKS ? rnd_pick(fx, w, 5u) : FX_OFF;   /* (two tracks at most, as RND's) */
        used += rnd_fx_points(type, 0);
        n_ins += type != FX_OFF;
        for (s = 0; s < 3u; s++) rnd_tp(t, P_FXTYPE + s * 4u, s == 0u ? (int32_t)type : FX_OFF);
        if (type != FX_OFF) {
            rnd_tp(t, P_FXMIX, el ? rnd_rr(40, 110) : rnd_rr(30, 90));
            rnd_tp(t, P_FXRATE, rnd_rr(0, 127));
            rnd_tp(t, P_FXCHAR, rnd_rr(0, 127));
            if (rnd_pct(50)) {                      /* (a cord on it) */
                cord_t c;
                memset(&c, 0, sizeof c);
                c.src_trk = c.dst_trk = (uint8_t)t;
                c.src = el ? fsrc[rnd_next() % NELEM(fsrc)] : ssrc[rnd_next() % NELEM(ssrc)];
                c.dst = (uint8_t)(CD_FX1_MIX + (uint32_t)rnd_rr(0, 2));
                c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(el ? 25 : 15, el ? 70 : 45) : -rnd_rr(el ? 25 : 15, el ? 70 : 45));
                rnd_cord_add(&c);
            }
        }
        rnd_tp(t, P_FXSEND, rnd_rr(el ? 10 : 30, el ? 80 : 110));
        rnd_tp(t, P_FXSEND2, song.g[G_MFX2TYPE] != FX_OFF ? rnd_rr(0, el ? 90 : 80) : 0);
        rnd_tp(t, P_FXSEND3, 0);
    }
}
/* WOZZA's effects, the light ones: a ROOM always (send 1, every track in it), an ECHO now and then (send 2, slow, low),
 * TAPE on the bed or the ostinato (its colour wandering), a low-pass FILTER on the melody (dark: the references hold
 * little above 2 kHz); within rnd_fx_mode_points' 13. FX 2 and 3 off */
static void rnd_fx_wozza(const uint8_t *role)
{
    uint32_t tr, s, tape = RND_NT, mel = RND_NT, echo = (uint32_t)rnd_pct(45);
    pid_set(PID_GLOBAL(G_FXMODE), 0, 1);
    for (s = 0; s < 3u; s++) {
        uint32_t base = G_MFXTYPE + s * 4u;
        pid_set(PID_GLOBAL(base), 0, s == 0u ? RND_ROOM : s == 1u && echo ? FX_ECHO : FX_OFF);
        pid_set(PID_GLOBAL(base + 1u), 0, s == 0u ? rnd_rr(70, 110) : rnd_rr(40, 75));
        pid_set(PID_GLOBAL(base + 2u), 0, s == 0u ? rnd_rr(60, 115) : rnd_rr(20, 70));
        pid_set(PID_GLOBAL(base + 3u), 0, s == 0u ? rnd_rr(40, 100) : rnd_rr(20, 60));
    }
    pid_set(PID_GLOBAL(G_FXRETURN), 0, rnd_rr(70, 100));
    for (tr = 0; tr < RND_NT; tr++) {
        if (role[tr] == RND_ROLE_MELODY && mel == RND_NT) mel = tr;
        if ((role[tr] == RND_ROLE_DRONE || role[tr] == RND_ROLE_OSTI) && (tape == RND_NT || rnd_pct(50))) tape = tr;
    }
    for (tr = 0; tr < RND_NT; tr++) {
        uint32_t type = tr == tape ? FX_TAPE : tr == mel ? FX_FILTER : FX_OFF;
        for (s = 0; s < 3u; s++) rnd_tp(tr, P_FXTYPE + s * 4u, s == 0u ? (int32_t)type : FX_OFF);
        if (type == FX_TAPE) {
            rnd_tp(tr, P_FXMIX, rnd_rr(80, 127));
            rnd_tp(tr, P_FXRATE, rnd_rr(30, 75));
            rnd_tp(tr, P_FXCHAR, rnd_rr(50, 110));
        } else if (type == FX_FILTER) {
            rnd_tp(tr, P_FXMIX, 127);
            rnd_tp(tr, P_FXRATE, rnd_rr(74, 90));   /* (1.2 to 2.8 kHz) */
            rnd_tp(tr, P_FXCHAR, rnd_rr(0, 30));
        }
        rnd_tp(tr, P_FXSEND, role[tr] == RND_ROLE_NOISE ? rnd_rr(15, 45) : rnd_rr(50, 110));
        rnd_tp(tr, P_FXSEND2, echo && role[tr] == RND_ROLE_MELODY ? rnd_rr(30, 80) : echo ? rnd_rr(0, 30) : 0);
        rnd_tp(tr, P_FXSEND3, 0);
    }
}
/* WOZZA's cords: slow wanderers on the bed's and the texture's colour, the melody's pan; now and then the melody's level
 * dipping at random (a tape's dropouts), the tape's colour wandering */
static void rnd_cords_wozza(const uint8_t *role)
{
    uint32_t i = 0, tr;
    cord_t c;
    for (tr = 0; tr < RND_NT && i < CORD_LIVE; tr++) {
        memset(&c, 0, sizeof c);
        c.src_trk = c.dst_trk = (uint8_t)tr;
        if (role[tr] == RND_ROLE_DRONE || role[tr] == RND_ROLE_NOISE) {
            c.src = rnd_pct(60) ? CS_DRIFT : CS_TORPOR;
            c.dst = CD_CUTOFF;
            c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, 30) : -rnd_rr(10, 30));
        } else if (role[tr] == RND_ROLE_MELODY) {
            c.src = CS_TORPOR;
            c.dst = CD_PAN;
            c.amount = (int8_t)rnd_rr(10, 30);
        } else continue;
        if (cord_set(i, &c)) i++;
        if (role[tr] == RND_ROLE_MELODY && rnd_pct(35) && i < CORD_LIVE) {
            memset(&c, 0, sizeof c);
            c.src_trk = c.dst_trk = (uint8_t)tr;
            c.src = CS_SH;
            c.dst = CD_GAIN;
            c.amount = (int8_t)-rnd_rr(15, 35);
            c.uni = 1;
            if (cord_set(i, &c)) i++;
        }
        if (trk[tr].p[P_FXTYPE] == FX_TAPE && i < CORD_LIVE) {
            memset(&c, 0, sizeof c);
            c.src_trk = c.dst_trk = (uint8_t)tr;
            c.src = CS_DRIFT;
            c.dst = CD_FX1_CHAR;
            c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(15, 40) : -rnd_rr(15, 40));
            if (cord_set(i, &c)) i++;
        }
    }
    for (memset(&c, 0, sizeof c); i < CORD_LIVE; i++) cord_set(i, &c);
}
static uint32_t rnd_gen_dest(uint32_t tr);
/* AMBIENT's cords: two to five, slow sources only, into the sound (never its pitch) and the melodies' generators */
static void rnd_cords_ambient(void)
{
    static const uint8_t src[] = {CS_LFO, CS_DRIFT, CS_TORPOR, CS_APATHY, CS_INERTIA, CS_OCHD1, CS_OCHD3, CS_SLEW};
    static const uint8_t sw[] = {6, 12, 6, 6, 6, 5, 5, 4};
    static const uint8_t dst[] = {CD_CUTOFF, CD_SHAPE, CD_PAN, CD_SPREAD, CD_LFO_DEPTH, CD_E1, CD_E2, CD_E3, CD_E4, CD_E5,
                                  CD_E6, 0xFF};
    static const uint8_t dw[] = {14, 10, 6, 4, 4, 5, 5, 5, 5, 4, 4, 8};
    static const uint8_t nw[] = {14, 10, 6, 4, 4, 0, 0, 0, 0, 0, 0, 8};   /* NICE: not the engines' own eight */
    uint32_t i, n = (uint32_t)rnd_rr(2, 5);
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t c;
        memset(&c, 0, sizeof c);
        if (i < n) {
            c.src = (uint8_t)rnd_pick(src, sw, NELEM(src));
            c.src_trk = (uint8_t)rnd_rr(0, RND_NT - 1);
            c.dst_trk = (uint8_t)(rnd_pct(60) ? c.src_trk : (uint32_t)rnd_rr(0, RND_NT - 1));
            c.dst = (uint8_t)rnd_pick(dst, rnd_nice ? nw : dw, NELEM(dst) - (rnd_loops ? 1u : 0u));   /* (LOOPS: the
                                                     * loops left as they are: not the generators') */
            if (c.dst == 0xFFu) c.dst = (uint8_t)(c.dst_trk >= 2u ? rnd_gen_dest(c.dst_trk) : CD_CUTOFF);
            c.amount = (int8_t)(rnd_pct(50) ? rnd_rr(10, rnd_nice ? 30 : 50) : -rnd_rr(10, rnd_nice ? 30 : 50));
            c.uni = (uint8_t)rnd_pct(30);
        }
        if (!cord_set(i, &c)) { memset(&c, 0, sizeof c); cord_set(i, &c); }
    }
}

/* cords: a handful, slow wanderers mostly, into the sound, the effects and the generators' own knobs */
static uint32_t rnd_gen_dest(uint32_t tr)
{
    switch (tessera.mode[tr]) {
    case TESS_ENG_TURING: return rnd_pct(50) ? CD_TUR_PROB : CD_TUR_LEN;
    case TESS_ENG_EUCLID: return rnd_pct(50) ? CD_EUC_PULSES : CD_EUC_ROT;
    case TESS_ENG_MARKOV: return rnd_pct(50) ? CD_MKV_LEAP : CD_MKV_DENS;
    case TESS_ENG_MORPH: return CD_MRP_POS;
    case TESS_ENG_BOUNCE: return rnd_pct(50) ? CD_BNC_SPEED : CD_BNC_ANGLE;
    case TESS_ENG_AWAKE: return CD_AWK_PROB;
    case TESS_ENG_QUAD: return CD_QD_PROB;
    case TESS_ENG_CONSTEL: return rnd_pct(50) ? CD_CS_DENS : CD_CS_Y;
    case TESS_ENG_SERIES: return CD_SR_START;
    case TESS_ENG_CHORD: return CD_CH_PROG;
    default: return CD_TW_DROP;
    }
}
static void rnd_cords(void)
{
    static const uint8_t src[] = {CS_LFO, CS_RAND, CS_SH, CS_SLEW, CS_CV, CS_NOTE, CS_DRIFT, CS_TORPOR, CS_APATHY,
                                  CS_INERTIA, CS_OCHD1, CS_OCHD3, CS_OCHD5, CS_OCHD8, CS_ENV, CS_GATE};
    static const uint8_t sw[] = {8, 4, 8, 6, 6, 4, 12, 6, 6, 6, 4, 4, 4, 4, 4, 4};
    static const uint8_t dst[] = {CD_CUTOFF, CD_SHAPE, CD_PITCH, CD_PAN, CD_SPREAD, CD_LFO_RATE, CD_LFO_DEPTH,
                                  CD_FX_SEND, CD_E1, CD_E2, CD_E3, CD_E4, CD_E5, CD_E6, CD_E7, CD_E8,
                                  CD_FX1_MIX, CD_FX1_RATE, CD_FX1_CHAR, CD_FX2_CHAR, CD_RELEASE, CD_ENV_FLT,
                                  CD_EV_AMOUNT, CD_SF1_MIX, CD_SF2_CHAR, 0xFF /* the generator's own */};
    static const uint8_t dw[] = {14, 10, 0, 5, 4, 5, 4, 5, 6, 6, 6, 6, 6, 6, 4, 4, 5, 5, 6, 4, 3, 4, 3, 3, 3, 12};   /* (PITCH
                                                     * none: no pitch modulation, rnd_no_pitch) */
    uint32_t i, n = (uint32_t)rnd_rr(3, 9);
    for (i = 0; i < CORD_LIVE; i++) {
        cord_t c;
        memset(&c, 0, sizeof c);
        if (i < n) {
            c.src = (uint8_t)rnd_pick(src, sw, NELEM(src));
            c.src_trk = (uint8_t)rnd_rr(0, RND_NT - 1);
            c.dst_trk = (uint8_t)(rnd_pct(60) ? c.src_trk : (uint32_t)rnd_rr(0, RND_NT - 1));
            c.dst = (uint8_t)rnd_pick(dst, dw, NELEM(dst));
            if (c.dst == 0xFFu) c.dst = (uint8_t)rnd_gen_dest(c.dst_trk);
            c.amount = (int8_t)(c.dst == CD_PITCH ? (rnd_pct(50) ? rnd_rr(2, 12) : -rnd_rr(2, 12)) : rnd_rr(-70, 70));
            c.uni = (uint8_t)rnd_pct(30);
            if (!c.amount) c.amount = 20;
        }
        if (!cord_set(i, &c)) { memset(&c, 0, sizeof c); cord_set(i, &c); }
    }
}


/* the roles of the four tracks: RND free on each; AMBIENT a drone, chords and two melodies, in a random order (so the
 * drone is not always track 1) */
static void rnd_roles(uint8_t *role, uint32_t ambient)
{
    uint32_t k;
    role[0] = RND_ROLE_DRONE; role[1] = RND_ROLE_CHORDS; role[2] = role[3] = RND_ROLE_MELODY;
    if (!ambient) {
        for (k = 0; k < RND_NT; k++) role[k] = RND_ROLE_FREE;
        return;
    }
    for (k = RND_NT - 1u; k > 0u; k--) {
        uint32_t j = rnd_next() % (k + 1u);
        uint8_t v = role[k]; role[k] = role[j]; role[j] = v;
    }
}

/* expression: cords from the track's own VEL and KEY to its CUTOFF, each voice its own (mod.c mod_voice): a harder
 * note brighter, a higher one brighter; sh, VEL to SHAPE too (FM6: its feedback, a hard note grittier; TRIO, ORACLE:
 * the pulse's width). The cutoff (TRIO's CUT, ORACLE's CUT, FM6's MLVL) down by about the lift a usual velocity brings;
 * FM6's VMOD up (its own: harder, brighter); TWEAK HUMAN at least hu, the velocities a player's. vel, key, sh: cord
 * percentages (a cord a slot free: VEL first, KEY after) */
static void rnd_expr_one(uint32_t tr, int32_t vel, int32_t key, int32_t sh, int32_t hu)
{
    track_t *t = &trk[tr];
    uint32_t e = t->eng_req;
    int32_t low = vel * 64 / 100 * 3 / 5;               /* (60 % of the lift at full velocity: the mean one, near) */
    cord_t c;
    memset(&c, 0, sizeof c);
    c.src_trk = c.dst_trk = (uint8_t)tr;
    c.src = CS_VEL; c.dst = CD_CUTOFF; c.amount = (int8_t)vel;
    if (vel && rnd_cord_add(&c)) {
        if (e == 6u) rnd_tp(tr, P_E0 + 5u, t->p[P_E0 + 5u] - low);
        else if (e == ENGI_ORACLE) rnd_tp(tr, P_E0, t->p[P_E0] - low);
        else if (e == RND_FM6) rnd_tp(tr, P_E0 + 2u, t->p[P_E0 + 2u] - low * 2 / 3);   /* (a CUT step: 2/3 of MLVL's) */
        else if (e == ENGI_MACRO) rnd_tp(tr, P_E0 + 3u, t->p[P_E0 + 3u] - low);   /* (MACRO's TONE) */
    }
    c.src = CS_KEY; c.amount = (int8_t)key;
    if (key) rnd_cord_add(&c);
    c.src = CS_VEL; c.dst = CD_SHAPE; c.amount = (int8_t)sh;
    if (sh) rnd_cord_add(&c);
    if (e == RND_FM6) rnd_tp(tr, P_E0 + 5u, rnd_rr(2, 4));
    if (pid_get(PID_TESS(TX_TW_HUMAN), tr) < hu) rnd_tx(tr, TX_TW_HUMAN, hu);
}
/* ORACLE's PLUCK and POLY BELLS where PHYS' piano and vibes were (WOZZA, LOOPS, HALO, MIRROR): up, as loud as the
 * part was (measured: 15 to 20 dB under the mix at the level PHYS' had) */
static void rnd_orc_levels(void)
{
    uint32_t tr;
    for (tr = 0; tr < RND_NT && rnd_tide; tr++)       /* (TIDE's ORACLE lead, measured: VOX 12 dB under SQUARE LEAD, FLUTE
                                                     * and KEYS 4) */
        if (trk[tr].eng_req == ENGI_ORACLE && tr == RND_TD_MEL)
            rnd_tp(tr, P_LEVEL, trk[tr].p[P_LEVEL] + (trk[tr].preset == 18u ? 24 : trk[tr].preset == 17u ? 0 : 8));
    if (!rnd_wozza && !rnd_loops && !rnd_halo && !rnd_mirror) return;
    for (tr = 0; tr < RND_NT; tr++)
        if (trk[tr].eng_req == ENGI_ORACLE && (trk[tr].preset == 9u || trk[tr].preset == 10u))
            rnd_tp(tr, P_LEVEL, trk[tr].p[P_LEVEL] + 24);
}
/* RND's vowels (asked 2026-11-08: more of them): one track a roll on MACRO's VOWEL, CHOIR FOF or VOSIM, two now and
 * then, its vowel and formants anywhere (TIMBR, COLOR), a note a consonant (STRIK), crushed now and then: chopped
 * speech among the rest; the track's generator, envelope and register as RND rolled them */
static void rnd_vowels(void)
{
    static const uint8_t vp[] = {5, 6, 4}, vw[] = {4, 3, 2};   /* VOWEL AH, CHOIR FOF, VOSIM VOX */
    uint32_t n, i, used = 0;
    if (!rnd_mc_ok() || rnd_setting() != RND_FREE) return;
    n = rnd_pct(85) ? (rnd_pct(30) ? 2u : 1u) : 0u;   /* (two MACRO tracks at most; the wavetable track out with their
                                                     * table, 2026-11-18) */
    for (i = 0; i < n; i++) {
        uint32_t tr = rnd_next() % RND_NT, j;
        track_t *t;
        int16_t keep[4];
        for (j = 0; j < RND_NT; j++)                  /* (a track MACRO already first: as many MACRO tracks as before) */
            if (!(used & (1u << ((tr + j) % RND_NT))) && trk[(tr + j) % RND_NT].eng_req == ENGI_MACRO) break;
        tr = j < RND_NT ? (tr + j) % RND_NT : tr;
        while (used & (1u << tr)) tr = (tr + 1u) % RND_NT;
        used |= 1u << tr;
        t = &trk[tr];
        memcpy(keep, &t->p[P_ATK], sizeof keep);    /* (RND's envelope kept: ATK DEC SUS REL) */
        engine_load_of(t, ENGI_MACRO, rnd_pick(vp, vw, NELEM(vp)));
        memcpy(&t->p[P_ATK], keep, sizeof keep);
        rnd_tp(tr, P_E0 + 1u, rnd_rr(0, 127));      /* TIMBR, COLOR: the vowel, the formants */
        rnd_tp(tr, P_E0 + 2u, rnd_rr(0, 127));
        rnd_tp(tr, P_E0 + 3u, rnd_rr(100, 127));    /* TONE: open-ish (the formants high up) */
        rnd_tp(tr, P_E0 + 4u, rnd_pct(30) ? rnd_rr(1, 4) : 0);   /* BITS, RATE: crushed now and then */
        rnd_tp(tr, P_E0 + 5u, rnd_pct(25) ? rnd_rr(1, 3) : 0);
        rnd_tp(tr, P_E0 + 6u, 1);                   /* STRIK: a consonant a note */
        rnd_tp(tr, P_E0 + 7u, rnd_pct(40) ? rnd_rr(10, 50) : 0);
        rnd_tp(tr, P_VOICE, rnd_pct(60) ? V_MONO : V_POLY);
        rnd_tp(tr, P_LEVEL, rnd_rr(84, 100));
        rnd_mc_level(tr);
    }
}
/* no pitch modulation in any roll (heard: far too much, silly): no vibrato (the track's LFO, FM6's PMD and PMS,
 * ORACLE's LFO on its oscillators; its Poly-Mod to the filter or width, never FREQ A), no pitch envelope (TIDE's kick its thump kept), no cord to the pitch. WOZZA's tape
 * wow (the track LFO a fifth of a semitone) its one exception. Expression stays on the cutoff and FM6's feedback */
static void rnd_no_pitch(void)
{
    uint32_t tr, i;
    for (tr = 0; tr < RND_NT; tr++) {
        track_t *t = &trk[tr];
        if (!rnd_wozza) rnd_tp(tr, P_LD_PIT, 0);
        if (!(rnd_tide && tr == RND_TD_KICK) && !(rnd_forge && tr != RND_FG_BASS)) rnd_tp(tr, P_ED_PIT, 0);   /* (the
                                                     * drums' pitch drop kept: FORGE, CHAOS) */
        if (t->eng_req == ENGI_ORACLE) {
            uint8_t *q = or_patch_of(t);
            q[OR_WHEEL_FREQ_A] = q[OR_WHEEL_FREQ_B] = 0;
            if (q[OR_POLY_FREQ]) {                  /* (Poly-Mod's FENV and OSC B to the filter or the width, not
                                                     * oscillator A's pitch: its swoop the pitch modulation heard) */
                q[OR_POLY_FREQ] = 0;
                if (!q[OR_POLY_PW] && !q[OR_POLY_FILTER])   /* (the width: oscillator A a pulse alone; else the filter.
                                                     * Decided by the patch, so PHASE's copies stay alike) */
                    q[q[OR_PULSE_A] && !q[OR_SAW_A] ? OR_POLY_PW : OR_POLY_FILTER] = 1;
            }
        }
        if (t->eng_req == RND_FM6 && (fm6_patch[tr][FP_LPMD] || fm6_patch[tr][FP_LPMS])) {
            uint8_t v[FP_SIZE + 1u];
            memcpy(v, fm6_patch[tr], FP_SIZE);
            v[FP_LPMD] = v[FP_LPMS] = 0;
            fm6_set_patch(tr, v);
        }
    }
    for (i = 0; i < CORD_LIVE; i++) {
        const cord_t *c = cord_get(i);
        if (c->amount && (c->dst == CD_PITCH || c->dst == CD_LFO_PIT || c->dst == CD_ENV_PIT)) {
            cord_t z;
            memset(&z, 0, sizeof z);
            cord_set(i, &z);
        }
    }
}
/* the modes' expression: WOZZA soft, CRATE's keys biting, ORBIT lively, RND anywhere; FALL's and TIDE's lines only
 * (their drones, pads, kick and noise even). The others none */
static void rnd_expression(void)
{
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++) {
        uint8_t r = rnd_role_of[tr];
        if (rnd_wozza) rnd_expr_one(tr, rnd_rr(25, 35), rnd_rr(25, 40), 0, rnd_rr(20, 35));
        else if (rnd_crate)
            rnd_expr_one(tr, tr == RND_CR_KEYS ? rnd_rr(50, 65) : tr == RND_CR_BASS ? rnd_rr(25, 35) :
                         tr == RND_CR_HEAD ? rnd_rr(35, 45) : rnd_rr(15, 22),
                         tr == RND_CR_BASS ? rnd_rr(8, 15) : rnd_rr(25, 35), 0, 0);   /* (its HUMAN its own) */
        else if (rnd_orbit)
            rnd_expr_one(tr, rnd_rr(35, 50), rnd_rr(35, 50),
                         trk[tr].eng_req == RND_FM6 && rnd_pct(50) ? rnd_rr(15, 25) : 0, rnd_rr(25, 40));
        else if (rnd_setting() == RND_FREE)
            rnd_expr_one(tr, rnd_rr(20, 70), rnd_rr(15, 70), rnd_pct(40) ? rnd_rr(10, 40) : 0, rnd_rr(15, 50));
        else if ((rnd_fall && r == RND_ROLE_MELODY) || (rnd_tide && tr == RND_TD_MEL))
            rnd_expr_one(tr, rnd_rr(22, 32), rnd_rr(25, 35), 0, rnd_rr(15, 25));
    }
}
/* the balance (asked 2026-11-09: the pads a bit quiet in every roll, the leads a bit loud, FALL and TIDE most): the
 * pads, beds and strings up 3 dB (FALL, TIDE 4), the leads down 2 (FALL, TIDE 3); a level step half a dB */
static void rnd_balance(void)
{
    uint32_t tr;
    int32_t up = rnd_fall || rnd_tide ? 8 : 6, down = rnd_fall || rnd_tide ? -6 : -4;
    for (tr = 0; tr < RND_NT; tr++) {
        uint8_t r = rnd_role_of[tr];
        int32_t d = 0;
        if (rnd_fall || rnd_wozza || rnd_halo || rnd_mirror)
            d = r == RND_ROLE_CHORDS || r == RND_ROLE_DRONE ? up : r == RND_ROLE_MELODY ? down : 0;
        else if (rnd_tide) d = tr == RND_TD_BED ? up : tr == RND_TD_MEL ? down : 0;
        else if (rnd_crate) d = tr == RND_CR_KEYS ? up : tr == RND_CR_HEAD ? down : tr == RND_CR_COUNTER ? -6 : 0;   /* (the
                                                     * keys up, the strings down: they were 10 dB over the keys, measured) */
        else if (rnd_static) d = tr == RND_ST_BED ? up : 0;
        else if (rnd_phase) d = tr == RND_NT - 1u ? up : 0;
        else if (rnd_loops) d = tr != rnd_lp_phys && tr != rnd_lp_glass ? up : 0;
        if (d) rnd_tp(tr, P_LEVEL, clamp(trk[tr].p[P_LEVEL] + d, 0, 127));
    }
}
/* the tempo of the rolls that kept yours (asked 2026-11-09: every roll starts at its own): FALL slow, LOOPS slow,
 * ORBIT 100 to 120, RND anywhere from 80 to 130. (The others set theirs: TIDE 64-84, WOZZA 72-84, HALO MIRROR STATIC
 * 100-120, CRATE 80-100, PHASE by its cell.) Held REC brings yours back */
/* every track's QNT WHITE but the chords' (QNT CHRD) */
static void rnd_in_key(void)
{
    uint32_t tr;
    for (tr = 0; tr < RND_NT; tr++)
        if (trk[tr].p[P_QUANT] != Q_CHRD) rnd_tp(tr, P_QUANT, Q_WHITE);
}
static void rnd_tempo(void)
{
    if (rnd_fall) pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(64, 80));
    else if (rnd_loops) pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(72, 90));
    else if (rnd_orbit) pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(100, 120));
    else if (rnd_setting() == RND_FREE) pid_set(PID_GLOBAL(G_BPM), 0, rnd_rr(80, 130));
}
static void rnd_roll(void)                           /* (rnd.c's, from a new project and the seed) */
{
    uint32_t tr, fm6s = 0, phys = 0, oracles = 0, chords = 0;
    uint32_t ambient = rnd_setting() == RND_FALL || rnd_setting() == RND_LOOPS || rnd_setting() == RND_ORBIT;   /* (the
                                                     * ambient roles' modes) */
    uint8_t role[RND_NT];
    rnd_state = rnd_seed ? rnd_seed : 0x2545F491u;  /* (the draws from the seed, in a fixed order) */
    rnd_nice = (uint8_t)(rnd_setting() == RND_FALL);   /* (FALL: the ambient roles, the gentle sounds, on TRIO's
                                                     * triangles; the others after, theirs) */
    rnd_fall = (uint8_t)(rnd_setting() == RND_FALL);
    rnd_loops = (uint8_t)(rnd_setting() == RND_LOOPS);
    rnd_orbit = (uint8_t)(rnd_setting() == RND_ORBIT);
    rnd_wozza = (uint8_t)(rnd_setting() == RND_WOZZA);
    rnd_halo = (uint8_t)(rnd_setting() == RND_HALO);
    rnd_mirror = (uint8_t)(rnd_setting() == RND_MIRROR);
    rnd_tide = (uint8_t)(rnd_setting() == RND_TIDE);
    rnd_crate = (uint8_t)(rnd_setting() == RND_CRATE);
    rnd_phase = (uint8_t)(rnd_setting() == RND_PHASE);
    rnd_static = (uint8_t)(rnd_setting() == RND_STATIC);
    rnd_chaos = (uint8_t)(rnd_setting() == RND_CHAOS);
    rnd_frac = (uint8_t)(rnd_setting() == RND_FRACTURED);
    rnd_forge = (uint8_t)(rnd_setting() == RND_FORGE || rnd_chaos || rnd_frac);
    rnd_cosmic = (uint8_t)(rnd_setting() == RND_COSMIC);
    if (rnd_wozza || rnd_halo || rnd_mirror || rnd_tide || rnd_crate || rnd_phase || rnd_static || rnd_forge || rnd_cosmic)
        rnd_nice = 1;   /* (WOZZA HALO MIRROR TIDE: NICE's gentle
                                                     * drift) */
    if (rnd_loops || rnd_orbit) rnd_nice = 1;       /* (LOOPS, ORBIT: NICE's gentle voices too) */
    rnd_roles(role, ambient && !rnd_loops && !rnd_orbit);
    if (rnd_loops || rnd_orbit)                     /* (LOOPS, ORBIT: four voices alike) */
        for (tr = 0; tr < RND_NT; tr++) role[tr] = RND_ROLE_MELODY;
    if (rnd_wozza) rnd_roles_wozza(role);
    if (rnd_halo) rnd_roles_halo(role);
    if (rnd_mirror) rnd_roles_mirror(role);
    if (rnd_tide) rnd_roles_tide(role);
    if (rnd_crate) rnd_roles_crate(role);
    if (rnd_phase) rnd_roles_phase(role);
    if (rnd_static) rnd_roles_static(role);
    if (rnd_forge) rnd_roles_forge(role);
    if (rnd_cosmic) rnd_roles_cosmic(role);
    memcpy(rnd_role_of, role, sizeof rnd_role_of);
    rnd_lp_phys = rnd_lp_glass = RND_NT;
    if (rnd_loops) {                                /* (LOOPS: the piano's track, the glass') */
        rnd_lp_phys = (uint8_t)(rnd_next() % RND_NT);
        rnd_lp_glass = (uint8_t)((rnd_lp_phys + 1u + rnd_next() % (RND_NT - 1u)) % RND_NT);
    }
    undo_depth++;                                   /* (a roll is not four sound loads to undo one by one) */
    {                                               /* (the drone and the chords pick their engines first) */
        static const uint8_t order[] = {RND_ROLE_LOWPERC, RND_ROLE_DRONE, RND_ROLE_CHORDS, RND_ROLE_MELODY,
                                        RND_ROLE_GLITCH, RND_ROLE_NOISE, RND_ROLE_OSTI, RND_ROLE_TINT,
                                        RND_ROLE_CANON, RND_ROLE_BASS, RND_ROLE_COUNTER, RND_ROLE_FREE};   /* (PULSE:
                                                     * FM6 first) */
        uint32_t o;
        for (o = 0; o < NELEM(order); o++)
            for (tr = 0; tr < RND_NT; tr++)
                if (role[tr] != order[o]) continue;
                else if (rnd_wozza) rnd_wozza_sound(tr, role[tr], &fm6s, &phys, &oracles);
                else if (rnd_halo) rnd_halo_sound(tr, role[tr], &fm6s, &phys, &oracles);
                else if (rnd_mirror) rnd_mirror_sound(tr, role[tr], &fm6s, &phys, &oracles);
                else if (rnd_tide) rnd_tide_sound(tr, role[tr], &fm6s, &phys, &oracles);
                else if (rnd_crate) rnd_crate_sound(tr, role[tr], &fm6s, &phys, &oracles);
                else if (rnd_phase) rnd_phase_sound(tr, &fm6s, &phys, &oracles);
                else if (rnd_static) rnd_static_sound(tr, &fm6s, &phys, &oracles);
                else if (rnd_forge) rnd_forge_sound(tr, &oracles);
                else if (rnd_cosmic) rnd_cosmic_sound(tr, &fm6s, &phys, &oracles, role[tr]);
                else if (rnd_loops) rnd_loops_voice(tr, &fm6s, &phys, &oracles);
                else rnd_sound(tr, &fm6s, &phys, &oracles, role[tr]);
    }
    undo_depth--;
    if (rnd_wozza) rnd_wozza_registers(role);
    else if (rnd_halo) rnd_halo_registers(role);
    else if (rnd_mirror) rnd_mirror_registers(role);
    else if (rnd_tide) rnd_tide_registers();
    else if (rnd_crate) rnd_crate_registers();
    else if (rnd_phase) ;                           /* (PHASE: after its generators, rnd_phase_registers) */
    else if (rnd_static) rnd_static_registers();
    else if (rnd_forge) rnd_forge_registers();
    else if (!rnd_cosmic) rnd_registers();          /* (COSMIC: its generators set them) */
    if (rnd_fall) rnd_fall_registers(role);
    if (rnd_loops) rnd_loops_registers();           /* (LOOPS: spread wider, -24 to +24) */
    if (rnd_wozza) rnd_key_wozza();
    else if (rnd_halo) rnd_key_halo();
    else if (rnd_mirror) rnd_key_mirror();
    else if (rnd_tide) rnd_key_tide();
    else if (rnd_crate) rnd_key_crate();
    else if (rnd_phase) rnd_key_phase();
    else if (rnd_static) rnd_key_static();
    else if (rnd_forge || rnd_cosmic) rnd_key_forge();
    else if (rnd_loops || rnd_orbit) rnd_key_loops();
    else if (ambient) rnd_key_ambient();
    else rnd_key();
    if (rnd_wozza) {
        for (tr = 0; tr < RND_NT; tr++) {
            rnd_wozza_generator(tr, role[tr], &chords);
            rnd_cpu(tr);
        }
    } else if (rnd_phase) {                         /* (its own caps: the copies one sound, as written) */
        rnd_phase_generators();
        rnd_phase_registers();
    } else if (rnd_static) {
        rnd_static_generators();
        for (tr = 0; tr < RND_NT; tr++) rnd_cpu(tr);
    } else if (rnd_forge) {
        rnd_forge_generators();
        for (tr = 0; tr < RND_NT; tr++) rnd_cpu(tr);
    } else if (rnd_cosmic) {
        rnd_cosmic_generators(role);
        for (tr = 0; tr < RND_NT; tr++) rnd_cpu(tr);
    } else if (rnd_halo || rnd_mirror || rnd_tide || rnd_crate) {
        if (rnd_halo) rnd_halo_generators(role);
        else if (rnd_mirror) rnd_mirror_generators(role);
        else if (rnd_tide) rnd_tide_generators();
        else rnd_crate_generators();
        for (tr = 0; tr < RND_NT; tr++) rnd_cpu(tr);
    } else if (rnd_loops || rnd_orbit) {
        if (rnd_loops) rnd_loops_generators();
        else rnd_orbit_generators();
        for (tr = 0; tr < RND_NT; tr++) {
            rnd_loops_sound(tr);
            rnd_cpu(tr);
        }
    } else {
        for (tr = 0; tr < RND_NT; tr++) {
            rnd_role_generator(tr, role[tr], &chords);
            rnd_cpu(tr);
        }
    }
    rnd_tails();                                    /* (rnd.c lowered BUDGET to 12 here: the player's setting, left) */
    if (rnd_halo || rnd_mirror || rnd_tide || rnd_crate || rnd_phase || rnd_forge || rnd_cosmic) {
        rnd_fx_dry();
        if (rnd_halo) rnd_cords_halo(role);
        else if (rnd_mirror) rnd_cords_mirror(role);
        else if (rnd_tide) rnd_cords_tide();
        else if (rnd_forge) rnd_cords_forge();
        else rnd_cords_crate();                     /* (CRATE, PHASE, COSMIC: none) */
    } else if (rnd_static) {                        /* (its own: SPIT's drive, the fizz) */
        rnd_static_fx();
        rnd_cords_static();
    } else if (rnd_wozza) {                         /* (its own effects, then its cords: one on the TAPE) */
        if (RND_FX_ROLLS) rnd_fx_wozza(role);
        else rnd_fx_dry();
        rnd_cords_wozza(role);
    } else if (ambient) {
        rnd_cords_ambient();
        if (RND_FX_ROLLS) rnd_fx_mode();
        else rnd_fx_dry();
    } else {
        if (RND_FX_ROLLS) rnd_inserts(rnd_sends());
        else rnd_fx_dry();
        rnd_cords();
    }
    rnd_vowels();
    rnd_orc_levels();
    rnd_no_pitch();
    rnd_expression();
    rnd_in_key();                                   /* (QNT WHITE: the rolls' key; a new project's QNT is OFF, which
                                                     * lets a SERIES or a FOLLOW step off the scale, measured) */
    rnd_chord_prime();
    rnd_balance();
    rnd_tempo();                                    /* (GEN_KEEP_TEMPO: generate puts the tempo back) */
}


/* ------------------------------------------------------------ GENERATE: the long-form layers --- */
/* What moves the piece over time, in place of rnd.c's DRIFT: Tessera's own layers, set per style (the design note's
 * 4.2). ENTRIES takes parts out and back on the phrase lines, by the roles (a track another FOLLOWs an ANCHOR, so its
 * followers keep their source); ENERGY's tide thins and fills; EVOLVE's GRAVITY brings an evolving line home; PIECE
 * gets the style's arc as its USER shape, with LENGTH FREE (nothing steered until a length is asked for). First
 * guesses, for Theo's ears: 0 is the layer's off */
static const struct {
    uint8_t entries, dwell, arc, rests, gravity;
    uint8_t arc_user[8];                            /* PIECE USER's eight levels, 0..100 */
} GEN_LF[RND_MODES - 1u] = {
    /* AUTUMN    */ {45, 2, 30, 10, 40, {20, 40, 60, 75, 85, 70, 45, 20}},
    /* SHORE     */ {35, 2, 30, 0, 40, {30, 50, 70, 85, 90, 80, 55, 25}},
    /* WOZZA     */ {35, 2, 20, 10, 50, {30, 45, 60, 70, 75, 65, 45, 25}},
    /* ORBIT     */ {45, 2, 40, 0, 40, {40, 60, 80, 95, 90, 75, 55, 30}},
    /* LOOPS     */ {0, 2, 20, 0, 60, {40, 50, 60, 65, 65, 60, 50, 40}},   /* (the loops' drift is the piece) */
    /* HALO      */ {0, 2, 20, 0, 50, {25, 40, 55, 70, 80, 70, 50, 25}},   /* (the T-voice and the canon FOLLOW) */
    /* MIRROR    */ {0, 2, 25, 0, 50, {25, 40, 55, 70, 80, 70, 50, 25}},
    /* INTERLOCK */ {0, 2, 0, 0, 0, {60, 60, 60, 60, 60, 60, 60, 60}},     /* (the machine's evenness is the point) */
    /* CRATE     */ {0, 2, 0, 0, 0, {70, 70, 70, 70, 70, 70, 70, 70}},     /* (a record to sample: sections exact) */
    /* COSMIC    */ {35, 2, 30, 0, 30, {40, 60, 80, 95, 85, 95, 70, 35}},
    /* STATIC    */ {0, 2, 50, 0, 0, {30, 55, 80, 100, 100, 35, 75, 20}},  /* (its build and drop: ENERGY for now) */
    /* FORGE     */ {40, 2, 40, 0, 30, {40, 60, 80, 100, 50, 90, 100, 40}},
    /* CHAOS     */ {40, 2, 45, 0, 20, {50, 70, 90, 100, 60, 95, 100, 40}},
    /* FRACTURED */ {45, 2, 45, 10, 20, {40, 70, 50, 90, 40, 100, 70, 30}},
    /* RND       */ {30, 2, 30, 0, 30, {30, 50, 70, 85, 90, 75, 50, 25}},
};
static uint32_t gen_et_role(uint32_t tr)            /* rnd.c's role -> ENTRIES' ROLE (entries.c ET_*) */
{
    uint32_t k;
    for (k = 0; k < RND_NT; k++)                    /* (a source: never out under its followers) */
        if (k != tr && (pid_get(PID_TESS(TX_TW_FOLLOW), k) == (int32_t)tr + 1 || pid_get(PID_TESS(TX_TW_HOLD), k) == (int32_t)tr + 1))
            return ET_ANCHOR;
    switch (rnd_role_of[tr]) {
    case RND_ROLE_DRONE: return ET_ANCHOR;
    case RND_ROLE_CHORDS: case RND_ROLE_OSTI: return ET_HARMONY;
    case RND_ROLE_BASS: return ET_BASS;
    case RND_ROLE_LOWPERC: case RND_ROLE_GLITCH: return ET_RHYTHM;
    case RND_ROLE_NOISE: return ET_ANCHOR;          /* (a texture under it all) */
    case RND_ROLE_MELODY: case RND_ROLE_TINT: case RND_ROLE_CANON: case RND_ROLE_COUNTER: return ET_MELODY;
    default: return ET_AUTO;
    }
}
static void generate_layers(void)
{
    uint32_t s = rnd_style - 1u, tr, k;
    rnd_tx(0, TX_ET_AMOUNT, GEN_LF[s].entries);
    rnd_tx(0, TX_ET_DWELL, GEN_LF[s].dwell);
    rnd_tx(0, TX_EN_ARC, GEN_LF[s].arc);
    rnd_tx(0, TX_EN_RESTS, GEN_LF[s].rests);
    rnd_tx(0, TX_PL_LENGTH, 0);                     /* FREE: the arc waits for a LENGTH */
    rnd_tx(0, TX_PL_SHAPE, 9);                      /* USER */
    for (k = 0; k < 8u; k++) rnd_tx(0, TX_PL_USER1 + k, GEN_LF[s].arc_user[k]);
    for (tr = 0; tr < RND_NT; tr++) {
        rnd_tx(tr, TX_ET_ROLE, (int32_t)gen_et_role(tr));
        if (pid_get(PID_TESS(TX_EV_EVERY), tr)) rnd_tx(tr, TX_SD_GRAVITY, GEN_LF[s].gravity);
    }
}

/* ------------------------------------------------------------ GENERATE: before / after --- */
/* The project before the last press, as its own .tsp (never a list of fields: it cannot go stale), and swapping it
 * with what is there now (as a project load: tess_core.c tess_core_load) */
static uint8_t gen_before[TSP_MAX];
static uint32_t gen_before_len;
static struct { uint8_t style, var, made; uint32_t seed; } gen_last;
static uint32_t gen_pack_now(void)                  /* the project now, into proj_wire_u: its length (0 failed) */
{
    project_capture(&proj_scratch);
    proj_wire_gen++;
    return tsp_pack(proj_wire_u.raw, &proj_scratch, 1);
}
static int gen_load(const uint8_t *raw, uint32_t len)   /* 0 loaded */
{
    if (!tsp_valid(raw, len)) return 1;
    if (project_restore_runtime(&proj_scratch)) return 1;
    tsp_restore(raw);
    ui.force = 1;
    return 0;
}

/* the native patches of the tracks not on their engine, as a new one (FM6's INIT VOICE, ORACLE's init), and FM6's
 * function settings: a new project leaves whatever the last one had there, and the record keeps them, so the same
 * seed would save other bytes */
static void gen_patches_reset(void)
{
    uint32_t k;
    uint8_t v[FP_SIZE + 1u];
    fm6_fn_reset();                                 /* (FM6's function settings: Dexed's, as a new project's comment
                                                     * says; project_new_blank leaves them, and the core's boot zero) */
    for (k = 0; k < PROJ_NTRK; k++) {
        if (trk[k].eng_req != ENGI_FM6) {
            fm6_unpack(FM6_INIT, v);
            fm6_put_patch(k, v, 1);
        }
        if (trk[k].eng_req != ENGI_ORACLE) {
            or_patch_init(or_patch[k]);
            or_ready[k] = 1;
        }
    }
}

/* tracks 5..8 (an 8-track build) as a new project's: a load and a new project keep their state (project.c PROJ_NTRK)
 * and a piece has no part for them yet, so the last project's would play on under it. Every track's steps empty */
static void gen_tracks_rest(void)
{
    uint32_t k;
    undo_depth++;
    for (k = PROJ_NTRK; k < NTRK; k++) {
        track_t *t = &trk[k];
        track_defaults(t);
        set_engine_of(t, TRK_DEF[k][0]);
        apply_preset_to(t, TRK_DEF[k][1]);
        if (TRK_DEF[k][0] != ENGI_DRUM) TRK_DEF_SCALE_OF(t);
        track_defaults_steps(t);
    }
    undo_depth--;
    for (k = 0; k < NTRK; k++)                      /* (step_clear leaves a step's notes, which the record keeps: the
                                                     * last project's would be saved with the piece) */
        for (uint32_t i = 0; i < NSTEP; i++) {
            memset(&trk[k].step[i], 0, sizeof trk[k].step[i]);
            step_clear(&trk[k].step[i]);
        }
}

/* ------------------------------------------------------------ GENERATE: the API --- */
enum { GEN_KEEP_TEMPO = 1u };                       /* generate's flags: the tempo as it is (a DAW or MIDI clock drives) */
static uint32_t generate_styles(void) { return RND_MODES - 1u; }
static const char *generate_style_name(uint32_t s) { return s < RND_MODES - 1u ? GEN_STYLE_NAME[s] : ""; }
/* a new piece: style 0..generate_styles()-1, seed (0 reads as 1), VARIATION 0..100. The
 * project before it is kept for generate_swap. Playing, it plays on; stopped, it stays stopped. 0 made, 1 refused */
static uint32_t generate(uint32_t style, uint32_t seed, uint32_t variation, uint32_t flags)
{
    uint32_t len, tr, playing = song.playing;
    int16_t bpm = song.g[G_BPM];
    if (style >= RND_MODES - 1u) return 1;
    len = gen_pack_now();
    if (!len) return 1;
    memcpy(gen_before, proj_wire_u.raw, len);
    gen_before_len = len;
    rnd_style = (uint8_t)(style + 1u);
    rnd_seed = seed ? seed : 1u;
    rnd_var = (uint8_t)(variation > 100u ? 100u : variation);
    project_new_blank();                            /* (from nothing: the same seed, the same piece) */
    gen_tracks_rest();
    rnd_roll();
    gen_patches_reset();
    generate_layers();
    for (tr = 0; tr < RND_NT; tr++) {               /* every track's SEED from the piece's: PLAY replays it */
        int32_t sd = rnd_seed_pick();
        if (!pid_get(PID_TESS(TX_EV_SEED), tr)) rnd_tx(tr, TX_EV_SEED, sd);
    }
    if (flags & GEN_KEEP_TEMPO) song.g[G_BPM] = bpm;
    sync_reload = 1;
    gen_last.style = (uint8_t)style; gen_last.seed = rnd_seed; gen_last.var = rnd_var; gen_last.made = 1;
    if (playing) transport_req = 1;                 /* (a new project stops; it was playing: on again) */
    ui.force = 1;
    return 0;
}
/* before / after: the project before the last press and what is there now change places. 0 done, 1 nothing kept */
static uint32_t generate_swap(void)
{
    uint32_t len, playing = song.playing;
    if (!gen_before_len) return 1;
    len = gen_pack_now();                           /* (now, in proj_wire_u: gen_load reads gen_before) */
    if (!len || gen_load(gen_before, gen_before_len)) return 1;
    memcpy(gen_before, proj_wire_u.raw, len);
    gen_before_len = len;
    if (playing) transport_req = 1;
    return 0;
}
/* the last piece made: its style, seed and VARIATION. 0 none yet */
static uint32_t generate_last(uint32_t *style, uint32_t *seed, uint32_t *variation)
{
    if (!gen_last.made) return 0;
    if (style) *style = gen_last.style;
    if (seed) *seed = gen_last.seed;
    if (variation) *variation = gen_last.var;
    return 1;
}

#endif /* TESS_GENERATE */
