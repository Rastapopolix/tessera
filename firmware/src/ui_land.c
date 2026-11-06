/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* LANDSCAPE (GLO, the Mono look): rain on water at night, played by the music. A track's note is a drop: where it
 * lands across the water is its pitch (low at the left), how near is its track (T1 far, T4 near), how wide its rings
 * open its velocity. ENERGY's tide gathers the clouds and lowers them (full) or thins them (sparse); while playing they
 * drift. Stars come out as the music thins, all of them when nothing has played for a while; then the water is still
 * and nothing is drawn again. Drawn from code; the notes come from land_feed.c, read only while the page is open. The
 * whole page below the header (no dials: the knobs do nothing here). Included by ui_mono.c */
#define LAND_TOP 10                                     /* the scene's rows: LAND_TOP .. MO_PY1 - 1 */
#define LAND_H 66                                       /* the water's line */
#define LAND_RIP 12                                     /* rings at once (the oldest gives way) */
#define LAND_LIFE 1600u                                 /* ms a ring opens and fades */
#define LAND_STARS 28
#define LAND_MX 94                                      /* the moon */
#define LAND_MY 22

/* a MOSAIC kind of palette: each part of the scene an ink of its four that suits it (Theo, beta3): the moon, the far
 * hills, the water (its line and the moon in it), as track ink 0..3, or 4 the palette's white; the stars white. A one-ink
 * palette: its ink, as before (its track inks are the ink, its white the ink) */
static const struct { char name[7]; uint8_t moon, hills, water; } LAND_INKS[] = {
    {"MOSAIC", 0, 3, 1},                                /* yellow moon, green hills, blue water */
    {"SUNSET", 3, 2, 0},                                /* yellow moon, lilac hills, orange water */
    {"OCEAN", 4, 2, 1},                                 /* white moon, green hills, teal water */
    {"NEON", 3, 2, 0},                                  /* orange moon, lime hills, cyan water */
};
static uint16_t land_ink(uint32_t part, int dim)       /* part: 0 moon, 1 hills, 2 water */
{
    uint32_t i, k = 4u;
    for (i = 0; i < sizeof LAND_INKS / sizeof LAND_INKS[0]; i++)
        if (str_eq(UI_PALETTES[ux.pal].name, LAND_INKS[i].name))
            k = part == 0u ? LAND_INKS[i].moon : part == 1u ? LAND_INKS[i].hills : LAND_INKS[i].water;
    if (k > 3u) return dim ? MO_DIM : MO_HOT;
    return dim ? mo_tdim(k) : T_TRK(k);
}

static struct {
    uint32_t t0[LAND_RIP];                              /* when the drop landed (fm1_ms) */
    uint8_t x[LAND_RIP], y[LAND_RIP], r[LAND_RIP], tr[LAND_RIP], vel[LAND_RIP], used[LAND_RIP];
    uint32_t last, drift, act, n;                       /* last frame (ms), drift (ms played), notes lately (Q8), drops */
    uint8_t rd;                                         /* the feed read up to here */
} land;

/* where a drop lands: across by pitch (C1 .. C7), the track a band deeper and nearer, a little apart from the last */
static void land_drop(uint32_t tr, uint32_t note, uint32_t vel)
{
    uint32_t i, k = 0, b = tr % 4u;
    int32_t x = 4 + ((int32_t)note - 24) * 112 / 72 + (int32_t)((note * 7u + land.n * 13u) % 5u) - 2;
    for (i = 0; i < LAND_RIP; i++) {                    /* a free slot, or the oldest */
        if (!land.used[i]) { k = i; break; }
        if (fm1_ms - land.t0[i] > fm1_ms - land.t0[k]) k = i;
    }
    land.used[k] = 1;
    land.t0[k] = fm1_ms;
    land.x[k] = (uint8_t)(x < 3 ? 3 : x > 116 ? 116 : x);
    land.y[k] = (uint8_t)(LAND_H + 6 + (int32_t)b * 9 + (int32_t)((note + land.n) % 3u));
    land.r[k] = (uint8_t)(5u + b * 2u + vel / 32u);
    land.tr[k] = (uint8_t)tr;
    land.vel[k] = (uint8_t)vel;
    land.n++;
    land.act += 256u;
}

/* a frame on: the drops since the last, the rings gone, how busy the music has been (falls back over ~4 s), the drift */
static void land_step(void)
{
    uint32_t at = land_ev_at, dt = fm1_ms - land.last, i;
    land.last = fm1_ms;
    if (!land_open) {                                   /* (just opened: what played before is not news) */
        land_open = 1;
        land.rd = (uint8_t)at;
        dt = 0;
    }
    if ((uint8_t)(at - land.rd) > LAND_EV)              /* (fallen behind: the newest ring's worth) */
        land.rd = (uint8_t)(at - LAND_EV);
    for (; land.rd != (uint8_t)at; land.rd++) {
        uint32_t e = land.rd & (LAND_EV - 1u);
        land_drop(land_ev_tr[e], land_ev_note[e], land_ev_vel[e]);
    }
    for (i = 0; i < LAND_RIP; i++)
        if (land.used[i] && fm1_ms - land.t0[i] >= LAND_LIFE)
            land.used[i] = 0;
    land.act = dt >= 4096u ? 0u : land.act - (land.act * dt >> 12);
    if (song.playing)
        land.drift += dt > 200u ? 200u : dt;
}
static int land_moving(void)
{
    for (uint32_t i = 0; i < LAND_RIP; i++)
        if (land.used[i]) return 1;
    return 0;
}
static uint32_t land_stars(void)                        /* none at four notes a second, all once quiet */
{
    uint32_t q = land.act >> 8;
    return q >= 16u ? 0u : LAND_STARS - q * LAND_STARS / 16u;
}
static uint32_t land_cover(void) { return (uint32_t)(energy_now() + 32767) >> 12; }   /* 0 sparse .. 15 full */

/* what the scene shows, for mo_signature: drawn again only when it changes (a ring opening: 25 frames a second) */
static uint32_t land_sig(void)
{
    land_step();
    return (land_moving() ? fm1_ms / 40u + 1u : 0u) ^ land_stars() << 20 ^ land_cover() << 26 ^ (land.drift / 700u) << 8;
}

static void land_ring(int32_t x, int32_t y, int32_t rx, int32_t ry, int dashed, uint16_t c)
{
    int32_t n = 12 + rx * 3, j, px = x + rx, py = y;     /* (dashed: every other side of the polygon) */
    for (j = 1; j <= n; j++) {
        int32_t a = j * 256 / n, qx = x + rx * mo_cos(a) / 127, qy = y + ry * mo_sin(a) / 127;
        if (!(dashed && (j & 1)) && py > LAND_H && qy > LAND_H && py < MO_PY1 && qy < MO_PY1)
            mo_line(px, py, qx, qy, c);
        px = qx, py = qy;
    }
}

static void mo_pic_land(void)
{
    uint32_t i, s = 0x2545F491u, stars = land_stars(), cover = land_cover(), clouds = 1u + cover / 4u;
    uint16_t moon = land_ink(0, 0), hills = land_ink(1, 1), water = land_ink(2, 0), deep = land_ink(2, 1);
    int32_t x, y;
    for (i = 0; i < LAND_STARS; i++) {                  /* the stars, always in the same places */
        s = s * 1103515245u + 12345u;
        x = 2 + (int32_t)((s >> 8) % 116u);
        y = LAND_TOP + 2 + (int32_t)((s >> 20) % (uint32_t)(LAND_H - LAND_TOP - 12));
        if (i < stars && ((x - LAND_MX) * (x - LAND_MX) + (y - LAND_MY) * (y - LAND_MY) > 81))
            mo_px(x, y, (s >> 30) ? MO_HOT : MO_DIM);
    }
    mo_disc(LAND_MX, LAND_MY, 5, moon);                 /* a crescent */
    mo_disc(LAND_MX + 3, LAND_MY - 2, 4, MO_BG);
    for (i = 0; i < clouds; i++) {                      /* the clouds: bands of dashes, lower and wider when full */
        int32_t w = 18 + (int32_t)cover + (int32_t)(i * 7u % 9u), cy = LAND_TOP + 6 + (int32_t)(i * 11u % 23u) +
                    (int32_t)cover / 2, cx = (int32_t)((i * 37u + land.drift / 700u) % 150u) - 15, r;
        for (r = 0; r < 3; r++) {
            int32_t rw = w * (2 + r) / 4, rx = cx - rw / 2;
            mo_hl(rx, cy + r * 2 - 1, rw, MO_BG);
            mo_hl(rx, cy + r * 2, rw, MO_BG);
            for (x = rx + (r & 1); x < rx + rw; x += 2)
                if (x >= 0 && x < 120) mo_px(x, cy + r * 2, MO_DIM);
        }
    }
    for (x = 0; x < 120; x++) {                         /* the far shore: low hills */
        int32_t h = 3 + (mo_sin(x * 3) + mo_sin(x * 7 + 40) / 2) * 3 / 127, y0 = LAND_H - (h < 1 ? 1 : h);
        mo_px(x, y0, hills);
        for (y = y0 + 1 + ((x ^ y0) & 1); y < LAND_H; y += 2) mo_px(x, y, hills);   /* (dithered) */
    }
    mo_hl(0, LAND_H, 120, water);
    for (y = LAND_H + 3, i = 0; y < MO_PY1 - 2; y += 4, i++) {   /* the moon in the water (it shivers under rings) */
        int32_t w = 7 - (int32_t)i, sh = land_moving() ? (int32_t)((fm1_ms / 80u + i) % 3u) - 1 : 0;
        if (w < 2) break;
        for (x = 0; x < w; x += 2) mo_px(LAND_MX - w / 2 + x + sh, y, deep);
    }
    for (i = 0; i < LAND_RIP; i++) {                    /* the rings: open fast, slow down, go to dashes and fade */
        uint32_t a, e;
        int32_t r, b;
        if (!land.used[i]) continue;
        a = fm1_ms - land.t0[i];
        e = LAND_LIFE - a;                              /* (ease out: 1 - (1 - a)^2, in 1/LIFE) */
        r = 1 + (int32_t)((uint32_t)(land.r[i] - 1u) * (LAND_LIFE * LAND_LIFE - e * e) / (LAND_LIFE * LAND_LIFE));
        b = (int32_t)land.y[i] - LAND_H;
        if (a < 90u) mo_vl(land.x[i], land.y[i] - 7 + (int32_t)a / 15, 3, MO_TRK(land.tr[i]));   /* (the drop) */
        else if (a < 180u) mo_px(land.x[i] - 1, land.y[i] - 2, MO_INK), mo_px(land.x[i] + 1, land.y[i] - 2, MO_INK);
        land_ring(land.x[i], land.y[i], r, 1 + r * (b + 12) / 100, a > LAND_LIFE / 2u,
                  a > LAND_LIFE * 2u / 3u ? MO_DIM : MO_TRK(land.tr[i]));
        if (land.vel[i] > 80u && a > 200u && a < LAND_LIFE * 2u / 3u && r > 5)
            land_ring(land.x[i], land.y[i], r / 2, 1 + r / 2 * (b + 12) / 100, 0, MO_DIM);
    }
}
