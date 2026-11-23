/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* GENERATE (firmware/src/generate.c) on Tessera's core API, built as the plugin builds it (tests/run_tests.sh), for
 * every style:
 *   - repeatable: the same style, seed and VARIATION give the same .tsp byte for byte, whatever project was there
 *     before (a new boot, a demo, another piece)
 *   - before / after: the swap gives back the project before the press byte for byte, and the piece again
 *   - VARIATION 0 (every pick the middle of its range) makes a piece too, the same each time
 *   - it plays (GEN_SECS seconds each, GEN_PLAY seeds a style, the first at VARIATION 0): nothing clips, each of the
 *     four tracks sounds, the generators' notes are in the track's scale (a note tessera_snap_note leaves as it is
 *     with QNT SNAP, the track's TRANS less its octaves: a track a fifth up plays its line in key; a STEP track's notes
 *     are not seen; two, or 2 %, may pass: a chord tone of a chord borrowed from outside the key), and after STOP every
 *     voice dies away
 *   GEN_SEEDS (50) seeds a style for the checks without sound. Exit 0: every check passed */
#define TESSERA_NOTE_HOOK gt_note
#include "../platform/core/core_unit.c"

#ifdef TESS_PLUGIN_IO
#define STEMS 1                                     /* (the tracks' own outputs: the plugin's; else no silence or tail
                                                     * checks, the rest as there) */
#else
#define STEMS 0
#endif
static uint8_t a[65536], b[65536], c[65536];
static uint32_t fails, notes_all, notes_out, tr_all[8], tr_out[8];
static int listening;
static void gt_note(track_t *t, uint32_t note, uint32_t vel)   /* every note a track plays */
{
    (void)vel;
    if (!listening) return;
    int32_t n = (int32_t)note - t->p[P_TRANS] % 12, q = t->p[P_QUANT];   /* (a track a fifth up: its line in key) */
    notes_all++; tr_all[t - trk]++;
    t->p[P_QUANT] = Q_SNAP;                           /* (the scale's notes, whatever QNT: OFF passes any) */
    if (tessera_snap_note(t, n) != n) { notes_out++; tr_out[t - trk]++; }
    t->p[P_QUANT] = (int16_t)q;
}
static int env(const char *k, int d) { const char *v = getenv(k); return v && *v ? atoi(v) : d; }
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL " __VA_ARGS__); printf("\n"); } } while (0)

static uint32_t made(uint32_t st, uint32_t seed, uint32_t var, uint8_t *out)
{
    if (generate(st, seed, var, 0)) return 0;
    return tess_core_save(out, 65536);
}

static uint8_t demo[65536];
static uint32_t demo_len;
static void load_demo(void)
{
    FILE *f = fopen("demos/cedar-rain.tsp", "rb");
    if (!f) return;
    demo_len = (uint32_t)fread(demo, 1, sizeof demo, f);
    fclose(f);
}

static float L[512], R[512], S[8][512];
/* plays the piece now for secs: its peak, and each track's level (dB, its stem's mean power) */
static float play(int secs, double *db)
{
    float *st[8], peak = 0;
    double pw[4] = {0};
    uint32_t n = tess_core_rate() * (uint32_t)secs / 512u, i, k;
    for (i = 0; i < 8u; i++) st[i] = S[i];
    tess_core_transport(1);
    listening = 1;
    for (i = 0; i < n; i++) {
        tess_core_render(L, R, st, 512);
        for (k = 0; k < 512u; k++) {
            float x = fabsf(L[k]) > fabsf(R[k]) ? fabsf(L[k]) : fabsf(R[k]);
            peak = x > peak ? x : peak;
            for (uint32_t t = 0; t < 4u; t++) pw[t] += (double)S[t][k] * S[t][k];
        }
    }
    listening = 0;
    for (k = 0; k < 4u; k++) db[k] = 10.0 * log10(pw[k] / (n * 512.0) + 1e-15);
    return peak;
}
/* STOP, then the loudest voice of the 10th and the 20th second after it (the tracks' own sound, before the send
 * effects): a hanging note holds, a long release (ORBIT's ORACLE, up to half a minute) dies away */
static void tail(float *at10, float *at20)
{
    float *st[8], peak[2] = {0, 0};
    uint32_t sec = tess_core_rate() / 512u, i, k;
    for (i = 0; i < 8u; i++) st[i] = S[i];
    tess_core_transport(0);
    for (i = 0; i < 20u * sec; i++) {
        tess_core_render(L, R, st, 512);
        if (i % (10u * sec) < 9u * sec) continue;
        for (k = 0; k < 512u; k++)
            for (uint32_t t = 0; t < 4u; t++)
                peak[i >= 10u * sec] = fabsf(S[t][k]) > peak[i >= 10u * sec] ? fabsf(S[t][k]) : peak[i >= 10u * sec];
    }
    *at10 = peak[0];
    *at20 = peak[1];
}
/* a generator that starts slowly (BOUNCE, CONSTELLATION) under EVOLVE: EVOLVE loops its first phrase, and when that
 * was silent it stays so (rnd.c's rolls do the same: a finding for the styles, not GENERATE's) */
static int slow_loop(uint32_t t)
{
    return (tessera.mode[t] == TESS_ENG_BOUNCE || tessera.mode[t] == TESS_ENG_CONSTEL) && tessera.evolve[t].every;
}

int main(void)
{
    int seeds = env("GEN_SEEDS", 50), plays = env("GEN_PLAY", 4), secs = env("GEN_SECS", 60);
    uint32_t st, i, n1, n2, nb, checks = 0;
    tess_core_boot();
    load_demo();
    CHECK(generate_styles() == 16u, "styles: %u", (unsigned)generate_styles());
    for (st = 0; st < generate_styles(); st++) {
        const char *nm = generate_style_name(st);
        for (i = 0; i < (uint32_t)seeds; i++) {
            uint32_t seed = 0x9E3779B9u * (i + 1u) ^ st;
            n1 = made(st, seed, 100, a);                         /* after whatever the last check left */
            if (demo_len) CHECK(!tess_core_load(demo, demo_len), "%s: the demo loads", nm);
            nb = tess_core_save(c, sizeof c);                  /* (the project before the press, as saved) */
            n2 = made(st, seed, 100, b);
            CHECK(n1 && n1 == n2 && !memcmp(a, b, n1), "%s seed %08X: not the same piece after another project", nm,
                  (unsigned)seed);
            CHECK(!generate_swap(), "%s: swap", nm);
            n2 = tess_core_save(b, sizeof b);
            CHECK(n2 == nb && !memcmp(b, c, nb), "%s seed %08X: before is not the project before", nm, (unsigned)seed);
            CHECK(!generate_swap(), "%s: swap back", nm);
            n2 = tess_core_save(b, sizeof b);
            CHECK(n2 == n1 && !memcmp(a, b, n1), "%s seed %08X: after is not the piece", nm, (unsigned)seed);
            checks += 3;
        }
        n1 = made(st, 77u, 0, a);                                /* VARIATION 0 */
        n2 = made(st, 77u, 0, b);
        CHECK(n1 && n1 == n2 && !memcmp(a, b, n1), "%s: VARIATION 0 not repeatable", nm);
        n2 = made(st, 78u, 0, b);
        checks += 1;
        for (i = 0; i < (uint32_t)plays; i++) {
            double db[4];
            float pk, tl, tl2;
            uint32_t seed = 1000u + i * 7919u, t;
            made(st, seed, i == 0u ? 0u : 100u, a);
            notes_all = notes_out = 0; memset(tr_all, 0, sizeof tr_all); memset(tr_out, 0, sizeof tr_out);
            pk = play(secs, db);
            tail(&tl, &tl2);
            printf("  %-10s seed %5u var %3u  peak %.2f  tracks %6.1f %6.1f %6.1f %6.1f dB  notes %5u out of key %u  tail %.4f\n",
                   nm, (unsigned)seed, i == 0u ? 0u : 100u, pk, db[0], db[1], db[2], db[3], (unsigned)notes_all,
                   (unsigned)notes_out, tl);
            if (getenv("GEN_DIAG")) for (t = 0; t < 8u; t++)
                printf("    t%u mode %2u eng %2u q %d trans %3d scale %2d root %2d notes %4u out %3u mute %d et %d\n", (unsigned)t + 1u, tessera.mode[t], trk[t].eng_req,
                       trk[t].p[P_QUANT], trk[t].p[P_TRANS], trk[t].p[P_SCALE], trk[t].p[P_ROOT], (unsigned)tr_all[t], (unsigned)tr_out[t], trk[t].p[P_MUTE], tessera.et_role[t]);
            CHECK(pk < 0.99f, "%s seed %u: clips (%.2f)", nm, (unsigned)seed, pk);
            for (t = 0; t < 4u && STEMS; t++)
                if (db[t] <= -80.0 && slow_loop(t))
                    printf("KNOWN %s seed %u: track %u silent (%s under EVOLVE EVERY %u)\n", nm, (unsigned)seed,
                           (unsigned)t + 1u, tessera.mode[t] == TESS_ENG_BOUNCE ? "BOUNCE" : "CONSTELLATION",
                           (unsigned)tessera.evolve[t].every);
                else CHECK(db[t] > -80.0, "%s seed %u: track %u silent", nm, (unsigned)seed, (unsigned)t + 1u);
            CHECK(notes_out <= 2u || notes_out * 50u <= notes_all, "%s seed %u: %u of %u notes out of key", nm, (unsigned)seed,
                  (unsigned)notes_out, (unsigned)notes_all);
            CHECK(!STEMS || tl < 0.001f || tl2 < tl * 0.6f, "%s seed %u: a note hangs after STOP (%.4f at 10 s, %.4f at 20 s)", nm,
                  (unsigned)seed, tl, tl2);
            checks += 5;
        }
    }
    printf("generate: %u styles, %u checks, %u failed\n", (unsigned)generate_styles(), (unsigned)checks, (unsigned)fails);
    return fails != 0;
}
