/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The engines the FM-1 leaves out for flash, built where they fit: SID (engine 3), DRUM's SID KIT and SID DIGI, and
 * MACRO (engine 21, Plaits). On in the plugin (platform/plugin/core_defs.txt) and the Daisy (platform/daisy, Andamento),
 * off on the FM-1 (core.h). Built with a target's options (tests/tsp_cross.sh: plugin at 44.1 kHz, daisy at 48 kHz),
 * with MELODEE_SID, MELODEE_SID_KIT and MELODEE_PLAITS on and Plaits' objects linked. For each: the engine is offered
 * (eng_ok, in the engine order), a track of it plays a held note (sounding, finite, within full scale) and stops after
 * its release, and a .tsp saved with it loads back with the same engine and sound, byte for byte.
 *   engines_extra        exit 0: passed */
#include "../platform/core/core_unit.c"

#if !MELODEE_SID || !MELODEE_SID_KIT || !MELODEE_PLAITS
#error "engines_extra.c: build with -DMELODEE_SID=1 -DMELODEE_SID_KIT=1 -DMELODEE_PLAITS=1"
#endif

static uint8_t tsp_a[65536], tsp_b[65536];
static float l[512], r[512];
static int fails;

static void check(int ok, const char *what)
{
    printf("engines_extra: %-72s %s\n", what, ok ? "ok" : "FAILED");
    fails += !ok;
}

/* frames of audio: the peak and the mean square of both sides; 0 when any sample is not finite */
static int play(uint32_t frames, double *peak, double *ms)
{
    uint32_t k, i;
    double s = 0, p = 0;
    for (k = 0; k < frames; k += 512u) {
        tess_core_render(l, r, 0, 512u);
        for (i = 0; i < 512u; i++) {
            if (!isfinite(l[i]) || !isfinite(r[i]))
                return 0;
            s += (double)l[i] * l[i] + (double)r[i] * r[i];
            if (fabs(l[i]) > p) p = fabs(l[i]);
            if (fabs(r[i]) > p) p = fabs(r[i]);
        }
    }
    *peak = p;
    *ms = s / (2.0 * frames);
    return 1;
}

static int offered(uint32_t e)
{
    uint32_t n;
    if (!eng_ok(e))
        return 0;
    for (n = 0; n < NENG_SHOWN; n++)
        if (ENGINE_ORDER[n] == e)
            return 1;
    return 0;
}

/* track tr on engine e, preset p: a note held 1 s, released, 3 s more; then a save and a load */
static void one(const char *name, uint32_t tr, uint32_t e, uint32_t p, uint32_t note)
{
    char what[96];
    double pk = 0, ms = 0, pk2 = 0, ms2 = 0;
    uint32_t i, na, nb, rate = tess_core_rate();
    track_t *t = &trk[tr];
    tess_core_boot();
    tess_core_transport(0);
    for (i = 0; i < NTRK; i++)                    /* (only this track sounds) */
        trk[i].p[P_MUTE] = i != tr;
    engine_load_of(t, e, p);
    play(rate / 4u, &pk, &ms);                    /* (the switch's fade) */
    snprintf(what, sizeof what, "%s: offered (in the engine list), the track's engine", name);
    check((e == ENGI_DRUM || offered(e)) && t->engine == e, what);
    trk_note_on(t, note, 100);
    int fin = play(rate, &pk, &ms);
    trk_note_off(t, note);
    fin &= play(3u * rate, &pk2, &ms2);
    pk2 = 0;
    fin &= play(rate / 2u, &pk2, &ms2);           /* (the last half second: the release is over) */
    printf("engines_extra:   %s: held note peak %.3f, rms %.4f; after its release peak %.5f\n", name, pk, sqrt(ms), pk2);
    snprintf(what, sizeof what, "%s: a held note sounds, finite, within full scale", name);
    check(fin && sqrt(ms) > 0.001 && pk <= 1.0, what);
    snprintf(what, sizeof what, "%s: silent after the note's release", name);
    check(pk2 < 0.001, what);
    na = tess_core_save(tsp_a, sizeof tsp_a);
    tess_core_boot();
    nb = !tess_core_load(tsp_a, na) && trk[tr].engine == e ? tess_core_save(tsp_b, sizeof tsp_b) : 0;
    snprintf(what, sizeof what, "%s: saved as .tsp (%u B), loaded: the same engine, the same bytes", name, (unsigned)na);
    if (getenv("EE_DUMP") && nb == na) for (i = 0; i < na; i++) if (tsp_a[i] != tsp_b[i]) printf("  diff at %u: %u %u\n", (unsigned)i, tsp_a[i], tsp_b[i]);
    check(na && nb == na && !memcmp(tsp_a, tsp_b, na), what);
}

int main(void)
{
    printf("engines_extra: rate %u\n", (unsigned)tess_core_rate());
    tess_core_boot();                             /* (a first boot's project saves a few bytes apart from what it loads as:
                                                   * a project round trip first, so the checks compare like with like) */
    tess_core_load(tsp_a, tess_core_save(tsp_a, sizeof tsp_a));
    one("FM6 (the reference)", 0, ENGI_FM6, 0, 60);
    one("SID (INIT)", 0, ENGI_SID, 0, 60);
    one("SID (preset 26)", 0, ENGI_SID, 26, 48);
    one("MACRO (INIT VA)", 0, ENGI_MACRO, 0, 57);
    one("DRUM SID KIT", 3, ENGI_DRUM, 2, 36);
    one("DRUM SID DIGI", 3, ENGI_DRUM, 3, 38);
    printf(fails ? "engines_extra: %d FAILED\n" : "engines_extra: all passed\n", fails);
    return fails != 0;
}
