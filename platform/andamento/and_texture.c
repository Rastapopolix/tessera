/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's texture engine: a small granular cloud, resampling the device's own finished output into a bed
 * under it. The plan (docs/design, andamento repo) names this class of effect after Clouds and Beads, Morphagene,
 * Arbhar and norns' glut and mangl as reference points, but the code here is Andamento's own: a clean-room design,
 * no Mutable Instruments source, no "Beads" anywhere a screen or label shows (CONTRACTS.md section 2, and
 * Emilie Gillet's own ask about Grids). "Texture" is the name on screen and in this file.
 *
 * and_texture_block() runs after and_master.c's master bus (the tilt EQ and compressor), so what it resamples is
 * the piece as it would otherwise be heard. Not on the FM-1.
 *
 *   capture   a 2 s mono ring (the stereo sum, halved), always a sample behind what just played; FREEZE stops it
 *             updating, so grains loop over whatever was last captured
 *   grains    up to AT_GRAINS at once (a fixed worst case, as the design rules ask): each starts at POS back from
 *             the write head, jittered by SPREAD, plays SIZE ms of the capture at PITCH semitones, panned by
 *             SPREAD, under a triangular window (its own simple shape, not a copy of anyone else's), and ends
 *   spawn     new grains at DENS a second, a little randomly spaced (SPREAD again) rather than metronomic, from
 *             this module's own seeded stream (gen_stream / gen_next, as gen.c's generators use), so a run repeats
 *   MIX       added on top of the dry signal (a bed under it, not a replacement); 0 is the default and leaves
 *             and_master_block's output untouched (and_texture_block returns at once: nothing captured either,
 *             so turning MIX up starts the bed from silence, filling over the next couple of seconds)
 *
 * Settings (AT_PID + AT_*) are a 3rd pid namespace, after and_ext.c's AE_PID and and_master.c's AM_PID, and_ui.c's
 * FX page's 3rd sub-page (press FX twice more). Not saved in a project yet, as the others aren't either.
 * Included by and_ui.c after and_master.c (it reuses am_clip16); and_texture_block() is called directly by the
 * platform (web/andamento/sim.c, later platform/daisy), right after and_master_block(). */

#define AT_PID 0xE200u                        /* settings' pids: AT_PID + AT_* (AE_PID 0xE000, AM_PID 0xE100) */
#define AT_BUF (FS * 2u)                      /* a 2 s mono capture */
#define AT_GRAINS 8u                          /* fixed: the worst case (8 grains' reads and a window each) is known */

enum { AT_SIZE, AT_DENS, AT_POS, AT_SPREAD, AT_PITCH, AT_FREEZE, AT_MIX, AT_NPRM };

static const param_desc_t AT_DESC[AT_NPRM] = {
    [AT_SIZE] = {"SIZE", F_INT, 10, 500, 80, 0, "ms"},
    [AT_DENS] = {"DENS", F_INT, 1, 100, 20, 0, "/s"},
    [AT_POS] = {"POS", F_PCT, 0, 100, 30, 0, 0},            /* how far back of the write head grains start */
    [AT_SPREAD] = {"SPREAD", F_PCT, 0, 100, 50, 0, 0},      /* jitters POS, the grain spacing and the pan */
    [AT_PITCH] = {"PITCH", F_SEMI, -24, 24, 0, 0, 0},
    [AT_FREEZE] = {"FREEZE", F_ENUM, 0, 1, 0, AE_ONOFF, 0}, /* and_ext.c's ON / OFF names */
    [AT_MIX] = {"MIX", F_PCT, 0, 100, 0, 0, 0},             /* 0: off, bit-identical (the default) */
};
static const param_desc_t AT_NONE_DESC = {"-", F_INT, 0, 0, 0, 0, 0};

typedef struct {
    uint32_t pos;                             /* Q16: the read position into the capture, a sample fraction too */
    uint32_t step;                            /* Q16: a sample's advance (PITCH as a ratio) */
    uint32_t len, age;                        /* total length and samples played, in samples (the window) */
    int32_t pan;                               /* -32768 (left) .. 32767 (right); 0 centred */
    uint8_t on;
} at_grain_t;

static struct {
    int16_t v[AT_NPRM];
    int16_t buf[AT_BUF];
    uint32_t w;                               /* the capture's next write position */
    at_grain_t g[AT_GRAINS];
    uint32_t rnd;                             /* this module's own stream (gen_stream / gen_next): independent
                                                * of the song's seed, so a run of it alone repeats the same way */
    uint32_t next_in;                         /* samples until a grain may start */
} and_texture = {.v = {[AT_SIZE] = 80, [AT_DENS] = 20, [AT_POS] = 30, [AT_SPREAD] = 50}, .rnd = 1};

static const param_desc_t *at_desc(uint32_t x) { return x < AT_NPRM ? &AT_DESC[x] : &AT_NONE_DESC; }
static int32_t at_get(uint32_t x) { return x < AT_NPRM ? and_texture.v[x] : 0; }
static void at_set(uint32_t x, int32_t v)
{
    if (x < AT_NPRM)
        and_texture.v[x] = (int16_t)clamp(v, AT_DESC[x].min, AT_DESC[x].max);
}
/* power-on (as and_ext_init): the capture, every grain and the stream reset, settings back to default, so a
 * run from here is the same every time */
static void and_texture_init(void)
{
    uint32_t k;
    memset(&and_texture, 0, sizeof and_texture);
    for (k = 0; k < AT_NPRM; k++)
        at_set(k, AT_DESC[k].def);
    and_texture.rnd = 1;
}

/* a free grain slot, or AT_GRAINS if all 8 are sounding (the spawn is skipped: a known, bounded worst case,
 * not a bigger pool or a steal) */
static uint32_t at_free_grain(void)
{
    uint32_t i;
    for (i = 0; i < AT_GRAINS; i++)
        if (!and_texture.g[i].on)
            return i;
    return AT_GRAINS;
}

static void at_spawn(void)
{
    uint32_t i = at_free_grain(), back, len;
    int32_t spread, jitter;
    at_grain_t *g;
    float semi, ratio;
    if (i >= AT_GRAINS)
        return;
    g = &and_texture.g[i];
    spread = and_texture.v[AT_SPREAD];
    len = (uint32_t)and_texture.v[AT_SIZE] * FS / 1000u;
    if (len >= AT_BUF)
        len = AT_BUF - 1u;
    back = (uint32_t)and_texture.v[AT_POS] * (AT_BUF / 100u);
    if (spread) {                              /* SPREAD jitters how far back this grain starts, +/- half of it */
        jitter = (int32_t)gen_rnd(&and_texture.rnd, (uint32_t)spread * (AT_BUF / 100u)) - (int32_t)spread * (int32_t)(AT_BUF / 200u);
        back = (uint32_t)clamp((int32_t)back + jitter, 0, (int32_t)AT_BUF - 1);
    }
    g->pos = ((and_texture.w + AT_BUF - back % AT_BUF) % AT_BUF) << 16;
    semi = (float)and_texture.v[AT_PITCH];
    ratio = powf(2.0f, semi / 12.0f);
    g->step = (uint32_t)(ratio * 65536.0f);
    g->len = len ? len : 1u;
    g->age = 0;
    g->pan = spread ? (int32_t)gen_rnd(&and_texture.rnd, (uint32_t)spread * 655u) - (int32_t)spread * 327 : 0;
    g->on = 1;
}

/* the capture buffer, linearly interpolated at a Q16 position */
static inline float at_read(uint32_t pos_q16)
{
    uint32_t idx = (pos_q16 >> 16) % AT_BUF, frac = pos_q16 & 0xFFFFu;
    float a = (float)and_texture.buf[idx], b = (float)and_texture.buf[(idx + 1u) % AT_BUF];
    return a + (b - a) * ((float)frac * (1.0f / 65536.0f));
}

/* and_master_block's n samples (int32, interleaved L/R), processed in place: a bed added on top, not a
 * replacement. Called once a control block, after and_master_block. */
static void and_texture_block(int32_t *out, uint32_t n)
{
    uint32_t i, g, spawn_every;
    float mix;
    if (!and_texture.v[AT_MIX])                           /* the default: nothing captured, nothing added */
        return;
    mix = (float)and_texture.v[AT_MIX] * (0.01f * 0.25f);  /* 0.25: headroom for several overlapping grains */
    spawn_every = and_texture.v[AT_DENS] ? FS / (uint32_t)and_texture.v[AT_DENS] : FS;
    for (i = 0; i < n; i++) {
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        float wl = 0, wr = 0;
        int16_t mono = (int16_t)clamp((l + r) >> 1, -32768, 32767);
        if (!and_texture.v[AT_FREEZE]) {
            and_texture.buf[and_texture.w] = mono;
            and_texture.w = (and_texture.w + 1u) % AT_BUF;
        }
        if (and_texture.next_in)
            and_texture.next_in--;
        else {
            at_spawn();
            and_texture.next_in = and_texture.v[AT_SPREAD]
                ? spawn_every / 2u + gen_rnd(&and_texture.rnd, spawn_every)   /* SPREAD: half to 1.5x apart */
                : spawn_every;
        }
        for (g = 0; g < AT_GRAINS; g++) {
            at_grain_t *gr = &and_texture.g[g];
            float s, win, amp_l, amp_r;
            if (!gr->on)
                continue;
            s = at_read(gr->pos);
            win = gr->age < gr->len / 2u ? (float)gr->age / (float)(gr->len / 2u)
                                          : (float)(gr->len - gr->age) / (float)(gr->len / 2u);   /* a triangle */
            amp_l = 0.5f - (float)gr->pan * (0.5f / 32768.0f);
            amp_r = 0.5f + (float)gr->pan * (0.5f / 32768.0f);
            wl += s * win * amp_l;
            wr += s * win * amp_r;
            gr->pos += gr->step;
            if (++gr->age >= gr->len)
                gr->on = 0;
        }
        out[2u * i] = am_clip16(l + (int32_t)(wl * mix));
        out[2u * i + 1u] = am_clip16(r + (int32_t)(wr * mix));
    }
}
