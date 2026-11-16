/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* LANDSCAPE (GLO, the Mono look): rain on water at night, played by the music. A track's note is a drop: where it
 * lands across the water is its pitch (low at the left), how near is its track (T1 far, T4 near), how wide its rings
 * open its velocity. ENERGY's tide gathers the clouds and lowers them (full) or thins them (sparse); while playing they
 * drift. Stars come out as the music thins, all of them when nothing has played for a while; then the water is still
 * and nothing is drawn again. Drawn from code; the notes come from land_feed.c, read only while the page is open. The
 * whole page below the header (no dials: the knobs do nothing here). GROUND LIGHT: the same water by day, a sun in the
 * moon's place and no stars.
 * The layers each tell something, and only when switched on: a PIECE's SHAPE is the moon's phase (full at the peak), its
 * last tenth dusk by day and the stars coming out by night; each CHAIN
 * section has a shore of its own (the new one comes in from the right) and a shooting star marks the FORM coming round;
 * ENERGY's tide is the wind in the clouds (right as it builds, left as it thins, still at the turns), its rests a mist
 * on the water. A named project is a place of its own: its name seeds the hills, the stars, where the moon rests and a
 * torii on A's shore (unnamed: the shore as before). Nothing new moves on the water but the rain. Included by
 * ui_mono.c */
#define LAND_TOP 10                                     /* the scene's rows: LAND_TOP .. MO_PY1 - 1 */
#define LAND_H 66                                       /* the water's line */
#define LAND_RIP 12                                     /* rings at once (the oldest gives way) */
#define LAND_LIFE 1600u                                 /* ms a ring opens and fades */
#define LAND_STARS 28
#define LAND_MX 94                                      /* the moon (LENGTH FREE: it stays here) */
#define LAND_MY 22
#define LAND_ML 8                                       /* a planned piece: its arc, rising at the left, highest */
#define LAND_MR 111                                     /* half way, setting at the right */
#define LAND_MT (LAND_TOP + 7)
#define LAND_MB (LAND_H - 4)
#define LAND_FISH_MS 30000u                             /* a fish jumps once the page has been quiet this long */
#define LAND_FISH_T 900u                                /* ms in the air */
#define LAND_TRAV_MS 1800u                              /* ms a new section's shore takes to come in */
#define LAND_STAR_MS 900u                               /* ms a shooting star takes to cross */

/* a MOSAIC kind of palette: each part of the scene an ink of its four that suits it (Theo, beta3): the moon, the far
 * hills, the water (its line and the moon in it), as track ink 0..3, or 4 the palette's white; the stars white; on GROUND
 * LIGHT the sun in the moon's place (SUNSET's moon ink is CHARCOAL on paper: its sun CRIMSON). A one-ink
 * palette: its ink, as before (its track inks are the ink, its white the ink) */
static const struct { char name[7]; uint8_t moon, hills, water, sun, gate; } LAND_INKS[] = {
    {"MOSAIC", 0, 3, 1, 0, 2},                          /* yellow moon, green hills, blue water; by day a sepia sun; a pink gate */
    {"SUNSET", 3, 2, 0, 1, 1},                          /* yellow moon, lilac hills, orange water; a crimson sun and gate */
    {"OCEAN", 4, 2, 1, 4, 3},                           /* white moon, green hills, teal water; a charcoal sun */
    {"NEON", 3, 2, 0, 3, 1},                            /* orange moon, lime hills, cyan water; a sepia sun */
};
static uint16_t land_ink(uint32_t part, int dim)       /* part: 0 moon (by day the sun), 1 hills, 2 water, 3 the gate */
{
    uint32_t i, k = 4u;
    for (i = 0; i < sizeof LAND_INKS / sizeof LAND_INKS[0]; i++)
        if (str_eq(UI_PALETTES[ux.pal].name, LAND_INKS[i].name))
            k = part == 0u ? (ux.light ? LAND_INKS[i].sun : LAND_INKS[i].moon) : part == 1u ? LAND_INKS[i].hills
                                                                                 : part == 2u ? LAND_INKS[i].water
                                                                                              : LAND_INKS[i].gate;
    if (k > 3u) return dim ? MO_DIM : MO_HOT;
    return dim ? mo_tdim(k) : T_TRK(k);
}

static struct {
    uint32_t t0[LAND_RIP];                              /* when the drop landed (fm1_ms) */
    uint8_t x[LAND_RIP], y[LAND_RIP], r[LAND_RIP], tr[LAND_RIP], vel[LAND_RIP], used[LAND_RIP];
    uint32_t last, drift, act, n;                       /* last frame (ms), drift (ms played), notes lately (Q8), drops */
    uint8_t rd;                                         /* the feed read up to here */
    int32_t mx, my;                                     /* the moon (1/16 cells) */
    uint8_t mset;                                       /* placed since the page opened */
    uint32_t quiet, ft0;                                /* ms since the last note (page open), the fish's jump */
    uint8_t fish, fx;                                   /* 0 none this silence, 1 in the air, 2 jumped; where from */
    uint32_t seed;                                      /* the place: the project name's hash (0 unnamed: as before) */
    uint32_t lp, tt0, st0, tk;                          /* the piece played (Q16), a travel's start, a star's, the 100 ms tick */
    int32_t cdr, ew;                                    /* the clouds' drift (ms, signed: the wind), ENERGY at the last look */
    uint16_t ph;                                        /* the moon's phase: the PIECE's SHAPE now (Q15) */
    int8_t wind;                                        /* -3 .. 3: ENERGY falling .. rising (the clouds' speed) */
    uint8_t sec, from, cpos, mist, wt;                  /* CHAIN's section, the one travelled from, its place; mist 0..16 */
    uint8_t gx, gp, wax;                                /* A's torii: its left column, its terrace's height; the moon waxing */
} land;

/* the moon: still while LENGTH is FREE; a planned piece's time played moves it on an arc, left to right (planner.c's
 * time, so a tempo change never jumps it), gliding where the plan is drawn again; END STOP: it sets behind the hills
 * with the tails; stopped: where it was (just opened: rising); a new start, LOOP's or NEW's: rising again at once */
static void land_moon(uint32_t dt)
{
    uint32_t p = 0, left;
    int32_t tx, ty, d;
    if (!pl_secs()) {                                   /* (a named place: the moon rests where its name says) */
        uint32_t h = land.seed * 2654435761u;
        land.mx = (land.seed ? 20 + (int32_t)((h >> 8 & 127u) * 80u >> 7) : LAND_MX) * 16;
        land.my = (land.seed ? 17 + (int32_t)(h >> 28) * 11 / 15 : LAND_MY) * 16, land.mset = 0, land.lp = 0;
        return;
    }
    if (song.playing) tessera_piece_get(&p, &left);
    else if (land.mset) return;
    if (p > 65536u) p = 65536u;
    land.lp = p;                                        /* (the phase: the SHAPE here, full at its peak) */
    left = (uint32_t)tessera_piece_shape(p);
    land.ph = (uint16_t)((int32_t)left < 0 ? 0 : left > 32767u ? 32767u : left);
    land.wax = p + 656u < 65536u && (int32_t)tessera_piece_shape(p + 656u) > (int32_t)left;   /* (the SHAPE rising) */
    tx = (LAND_ML + (int32_t)(p * (uint32_t)(LAND_MR - LAND_ML) >> 16)) * 16;
    ty = (LAND_MB * 16 - (LAND_MB - LAND_MT) * 16 * mo_sin((int32_t)(p >> 9)) / 127);
    if (song.playing && plan.ending) ty = (LAND_H + 8) * 16;
    if (!land.mset || (p < 1024u && land.mx > tx + 16 * 16)) {
        land.mx = tx, land.my = ty, land.mset = 1;
        return;
    }
    dt = dt > 500u ? 500u : dt;                         /* (a glide of about a second) */
    d = (tx - land.mx) * (int32_t)dt / 1000;
    land.mx += d ? d : (tx > land.mx) - (tx < land.mx);
    d = (ty - land.my) * (int32_t)dt / 1000;
    land.my += d ? d : (ty > land.my) - (ty < land.my);
}
/* a shore's height at x: section sec of the place (unnamed A: the shore as it always was; otherwise two waves and
 * sometimes a peak, from the seed, never above 15 so the moon is never buried; 0 or less: open water, so a place can
 * be islands) */
static int32_t land_sky0(int32_t x, uint32_t sec)
{
    uint32_t h;
    int32_t hh, pk;
    if (!land.seed && !sec) {
        hh = 3 + (mo_sin(x * 3) + mo_sin(x * 7 + 40) / 2) * 3 / 127;
        return hh < 1 ? 1 : hh;                         /* (the lake as it always was: one shore all along) */
    }
    h = ((land.seed ? land.seed : 0x6D2B79F5u) ^ (sec + 1u) * 0x9E3779B1u) * 0x2C1B3C6Du;
    h ^= h >> 15;
    hh = 2 + (int32_t)(h >> 16 & 3u) + (mo_sin(x * (2 + (int32_t)(h & 3u)) + (int32_t)(h >> 5 & 255u)) +
                                        mo_sin(x * (5 + (int32_t)(h >> 2 & 7u)) + (int32_t)(h >> 5 & 255u) * 3) / 2) *
                                           (5 + (int32_t)(h >> 13 & 3u) * 2) >> 7;
    pk = (mo_sin(x * 2 + (int32_t)(h >> 20 & 255u)) * (4 + (int32_t)(h >> 28 & 7u)) >> 7) - 2;
    if (!(h >> 18 & 3u) && pk > 0) hh += pk;
    return hh > 15 ? 15 : hh;
}
/* where A's torii stands: the flattest 11 columns of land, nearest where the seed would put it; its terrace is raised
 * to their top (at least 3: never water), so the land is only ever built up under it, never cut (once a name) */
static void land_gate_find(void)
{
    int32_t x, c, h, lo, hi, sc, best = 0x7FFFFFFF, want = 8 + (int32_t)((land.seed * 2246822519u) >> 9 & 127u) * 3 / 4;
    for (x = 4; x <= 105; x++) {
        for (lo = 99, hi = -99, c = 0; c < 11; c++)
            h = land_sky0(x + c, 0), lo = h < lo ? h : lo, hi = h > hi ? h : hi;
        sc = (hi - lo) * 64 + (lo < 3 ? 512 : 0) + (x > want ? x - want : want - x);
        if (sc < best) best = sc, land.gx = (uint8_t)x, land.gp = (uint8_t)(hi < 3 ? 3 : hi);
    }
}
static int32_t land_gate_x(void) { return land.gx; }
/* the shore: on a named place's A, the torii's terrace, flat under its 11 columns and sloping off three cells each
 * side, only ever above the land */
static int32_t land_sky(int32_t x, uint32_t sec)
{
    int32_t h = land_sky0(x, sec), d, g = land.gx;
    if (!land.seed || sec) return h;
    d = x < g ? g - x : x > g + 10 ? x - g - 10 : 0;
    return d > 3 || h > (int32_t)land.gp - d ? h : (int32_t)land.gp - d;
}
static int32_t land_toff(void)                          /* a travel: how far the new shore has come in (120: arrived) */
{
    uint32_t a = fm1_ms - land.tt0;
    return a < LAND_TRAV_MS ? (int32_t)(a * 120u / LAND_TRAV_MS) : 120;
}
static int32_t land_hill(int32_t x)                    /* the far shore's top at x (travelling: the old one going left) */
{
    int32_t w = x + land_toff(), h = w < 120 ? land_sky(w, land.from) : land_sky(w - 120, land.sec);
    return LAND_H - (h < 0 ? 0 : h);                    /* (0: open water, the water's line) */
}
static void land_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* mo_disc, behind the hills */
{
    for (int32_t y = -r; y <= r; y++) {
        int32_t w = 0, x;
        while ((w + 1) * (w + 1) + y * y <= r * r + r)
            w++;
        for (x = cx - w; x <= cx + w; x++)
            if (cy + y < land_hill(x)) mo_px(x, cy + y, c);
    }
}

/* a ring of its own (a fish's splash): no drop, no note, dim */
static void land_splash(int32_t x, int32_t y)
{
    uint32_t i, k = 0;
    for (i = 0; i < LAND_RIP; i++) {
        if (!land.used[i]) { k = i; break; }
        if (fm1_ms - land.t0[i] > fm1_ms - land.t0[k]) k = i;
    }
    land.used[k] = 1, land.t0[k] = fm1_ms - 180u;       /* (past the drop) */
    land.x[k] = (uint8_t)x, land.y[k] = (uint8_t)y, land.r[k] = 6, land.tr[k] = 4, land.vel[k] = 0;
}

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
    land.quiet = 0, land.fish = 0;
}

/* a frame on: the drops since the last, the rings gone, how busy the music has been (falls back over ~4 s), the drift */
static void land_step(void)
{
    uint32_t at = land_ev_at, dt = fm1_ms - land.last, i;
    land.last = fm1_ms;
    uint8_t seq[4];
    uint32_t pos, left, n = tessera_chain_get(seq, &pos, &left), sec = n ? seq[pos & 3u] : 0u;
    char nm[13];                                        /* (project_cur_name: 12 + its end) */
    uint32_t was = land.seed;
    project_cur_name(nm);                               /* (the place: FNV-1a of the name) */
    for (land.seed = 2166136261u, i = 0; nm[i]; i++) land.seed = (land.seed ^ (uint8_t)nm[i]) * 16777619u;
    if (!nm[0]) land.seed = 0;
    if (land.seed && (land.seed != was || !land_open)) land_gate_find();
    if (!land_open) {                                   /* (just opened: what played before is not news) */
        land_open = 1;
        land.rd = (uint8_t)at;
        land.mset = 0;
        land.quiet = 0, land.fish = 0;
        dt = 0;
        land.sec = land.from = (uint8_t)sec, land.cpos = (uint8_t)pos, land.tt0 = fm1_ms - LAND_TRAV_MS;
        land.st0 = fm1_ms - LAND_STAR_MS, land.tk = fm1_ms, land.ew = energy_now(), land.mist = 0;
        land.wind = tessera.en_arc ? 0 : 1;
    }
    if (sec != land.sec)                                /* a new section: travel there */
        land.from = land.sec, land.sec = (uint8_t)sec, land.tt0 = fm1_ms;
    if (n > 1u && song.playing && !pos && land.cpos == n - 1u)   /* the FORM came round: a shooting star */
        land.st0 = fm1_ms;
    land.cpos = (uint8_t)pos;
    if (fm1_ms - land.tk > 2000u) land.tk = fm1_ms - 100u;   /* (long away: no catching up) */
    for (; fm1_ms - land.tk >= 100u; land.tk += 100u) { /* each 0.1 s: the mist toward a rest; each second: the wind */
        if (energy_resting() && song.playing) { if (land.mist < 16u) land.mist++; }
        else if (land.mist) land.mist--;
        if (++land.wt < 10u) continue;
        land.wt = 0;
        if (tessera.en_arc) {                           /* (the tide's slope, per second: about +-700 at most) */
            int32_t e = energy_now(), w = (e - land.ew) / 200;
            land.wind = (int8_t)(energy_resting() ? 0 : w > 3 ? 3 : w < -3 ? -3 : w), land.ew = e;
        } else land.wind = 1;                           /* (no tide: the clouds drift as they always did) */
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
        land.drift += dt > 200u ? 200u : dt, land.cdr += (int32_t)(dt > 200u ? 200u : dt) * land.wind;
    land_moon(dt);
    if (land.quiet < LAND_FISH_MS) land.quiet += dt;
    if (!land.fish && land.quiet >= LAND_FISH_MS) {     /* a long silence: a fish, somewhere along the near water */
        land.fish = 1, land.ft0 = fm1_ms;
        land.fx = (uint8_t)(16u + (land.n * 37u + land.drift / 16u) % 72u);
        land_splash(land.fx, LAND_H + 5);
    } else if (land.fish == 1u && fm1_ms - land.ft0 >= LAND_FISH_T) {
        land.fish = 2;
        land_splash(land.fx + 18, LAND_H + 5);
    }
}
static int land_moving(void)
{
    for (uint32_t i = 0; i < LAND_RIP; i++)
        if (land.used[i]) return 1;
    return land.fish == 1u;
}
static uint32_t land_stars(void)                        /* none at four notes a second, all once quiet */
{
    uint32_t q = land.act >> 8;
    return q >= 16u ? 0u : LAND_STARS - q * LAND_STARS / 16u;
}
static uint32_t land_cover(void) { return (uint32_t)(energy_now() + 32767) >> 12; }   /* 0 sparse .. 15 full */

/* what the scene shows, for mo_signature: drawn again only when it changes (a ring opening: 25 frames a second) */
static uint32_t land_dusk(void)                         /* 0 .. 16 over a PIECE's last tenth */
{
    return pl_secs() && land.lp > 58982u ? (land.lp - 58982u) * 5u >> 11 : 0u;
}
static uint32_t land_sig(void)
{
    uint32_t anim;
    land_step();
    anim = land_toff() < 120 || fm1_ms - land.st0 < LAND_STAR_MS || (land.mist && land.mist < 16u);
    return (land_moving() || anim ? fm1_ms / 40u + 1u : 0u) ^ land_stars() << 20 ^ land_cover() << 26 ^
           (uint32_t)(land.cdr / 700) << 8 ^ (uint32_t)(land.mx >> 4) << 12 ^ (uint32_t)(land.my >> 4) << 19 ^
           (uint32_t)land.ph >> 11 << 24 ^ land_dusk() << 16 ^ land.seed ^ land.sec << 28 ^ (land.mist ? fm1_ms / 350u : 0u) << 5;
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

static const uint8_t LAND_BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
#define LAND_BY(x, y) LAND_BAYER[((y) & 3) * 4 + ((x) & 3)]
/* a place's torii: the top beam's ends turned up, the tie beam, the pillars (11 x 10; bit 10 the left column) */
static const uint16_t LAND_GATE[10] = {0x401, 0x7FF, 0x3FE, 0x124, 0x7FF, 0x104, 0x104, 0x104, 0x104, 0x38E};
static void land_temple(int32_t gx, uint16_t hills)    /* on A's shore at gx, standing on the ground under its middle */
{
    int32_t r, c, y, gy = land_hill(gx + 5);
    uint16_t ink = land_ink(3, 0);                      /* (an ink of its own: MOSAIC pink, apart from the hills) */
    gy = (gy > LAND_H - 1 ? LAND_H - 1 : gy) + 1 - 10;
    for (r = 0; r < 10; r++)
        for (c = 0; c < 11; c++)
            if (LAND_GATE[r] >> (10 - c) & 1u) mo_px(gx + c, gy + r, ink);
    for (c = 0; c < 11; c++)                            /* (the ground under it, where the slope falls away) */
        for (y = gy + 10; y < land_hill(gx + c); y++) mo_px(gx + c, y, hills);
}

static void mo_pic_land(void)
{
    uint32_t i, s = land.seed ? land.seed : 0x2545F491u, stars = land_stars(), cover = land_cover(), clouds = 1u + cover / 4u,
                dusk = land_dusk();
    uint16_t moon = land_ink(0, 0), hills = land_ink(1, 1), water = land_ink(2, 0), deep = land_ink(2, 1);
    int32_t x, y, mx = land.mx >> 4, my = land.my >> 4, toff = land_toff();
    if (!ux.light && dusk * LAND_STARS / 16u > stars) stars = dusk * LAND_STARS / 16u;   /* (night: the outro's stars) */
    if (ux.light && dusk)                                /* by day the outro: dusk, the sky thicker toward the hills */
        for (y = LAND_TOP; y < LAND_H; y++)
            for (x = 0; x < 120; x++)
                if ((uint32_t)(LAND_BY(x, y) + 1u) * 4u * 56u * 56u < dusk * (uint32_t)((y - LAND_TOP) * (y - LAND_TOP)) * (uint32_t)(y - LAND_TOP) / 56u)
                    mo_px(x, y, ux_mix(T_BG, moon, 45));
    for (i = 0; i < LAND_STARS && !ux.light; i++) {    /* the stars, always in the same places (by day none) */
        s = s * 1103515245u + 12345u;
        x = 2 + (int32_t)((s >> 8) % 116u);
        y = LAND_TOP + 2 + (int32_t)((s >> 20) % (uint32_t)(LAND_H - LAND_TOP - 12));
        if (i < stars && ((x - mx) * (x - mx) + (y - my) * (y - my) > 81))
            mo_px(x, y, (s >> 30) ? MO_HOT : MO_DIM);
    }
    if (ux.light) {                                     /* by day the sun: a ring, eight short rays */
        land_disc(mx, my, 4, moon);
        land_disc(mx, my, 3, MO_BG);
        for (i = 0; i < 16u; i++) {                     /* (each ray two cells: 6 and 7 out) */
            int32_t r = 6 + (int32_t)(i & 1u), a = (int32_t)(i >> 1) * 32;
            x = mx + r * mo_cos(a) / 127, y = my + r * mo_sin(a) / 127;
            if (y < land_hill(x)) mo_px(x, y, moon);
        }
    } else {
        land_disc(mx, my, 5, moon);                     /* a crescent; a PIECE's: as full as its SHAPE here, waxing as it rises */
        if (!pl_secs()) land_disc(mx + 3, my - 2, 4, MO_BG);
        else if (land.ph < 31784u)
            land_disc(mx + (land.wax ? 1 : -1) * (2 + (((int32_t)land.ph * 8 + 16384) >> 15)),   /* (waxing: lit on the left, */
                      my - ((2 * (32767 - (int32_t)land.ph) + 16384) >> 15), 4, MO_BG);       /* waning: on the right: */
                                                                                               /* facing into the scene) */
    }
    for (i = 0; i < clouds; i++) {                      /* the clouds: bands of dashes, lower and wider when full */
        int32_t w = 18 + (int32_t)cover + (int32_t)((i * 7u + (land.seed >> 27)) % 9u), cy = LAND_TOP + 6 + (int32_t)(i * 11u % 23u) +
                    (int32_t)cover / 2, cx = ((int32_t)(i * 37u) + land.cdr / 700) % 150, r;
        cx = (cx < 0 ? cx + 150 : cx) - 15;
        for (r = 0; r < 3; r++) {
            int32_t rw = w * (2 + r) / 4, rx = cx - rw / 2;
            mo_hl(rx, cy + r * 2 - 1, rw, MO_BG);
            mo_hl(rx, cy + r * 2, rw, MO_BG);
            for (x = rx + (r & 1); x < rx + rw; x += 2)
                if (x >= 0 && x < 120) mo_px(x, cy + r * 2, MO_DIM);
        }
    }
    for (x = 0; x < 120; x++) {                         /* the far shore: low hills */
        int32_t y0 = land_hill(x);
        mo_px(x, y0, hills);
        for (y = y0 + 1 + ((x ^ y0) & 1); y < LAND_H; y += 2) mo_px(x, y, hills);   /* (dithered) */
    }
    if (!ux.light && fm1_ms - land.st0 < LAND_STAR_MS) {   /* the FORM came round: a star across, down to the left (night) */
        uint32_t a = fm1_ms - land.st0;
        int32_t hx = 96 + (int32_t)(land.seed & 15u) - (int32_t)(a * 46u / LAND_STAR_MS), hy = LAND_TOP + 6 + (int32_t)(a * 18u / LAND_STAR_MS);
        for (i = 5; i >= 1u; i--) mo_px(hx + (int32_t)(i * 5u / 2u), hy - (int32_t)i, i > 2u ? MO_DIM : MO_HOT);
        mo_px(hx, hy, MO_HOT);
    }
    if (land.seed && land.from == 0u && toff < 120)     /* the torii: only on A's shore, going with it and back; */
        land_temple(land_gate_x() - toff, hills);
    if (land.seed && land.sec == 0u)
        land_temple(land_gate_x() + 120 - toff, hills);
    mo_hl(0, LAND_H, 120, water);
    for (y = LAND_H + 3, i = 0; my - 5 < land_hill(mx) && y < MO_PY1 - 2 && i < 6u - land.mist * 6u / 16u; y += 4, i++) {   /* the moon in the water (it shivers under rings) */
        int32_t w = 7 - (int32_t)i, sh = land_moving() ? (int32_t)((fm1_ms / 80u + i) % 3u) - 1 : 0;
        if (w < 2) break;
        for (x = 0; x < w; x += 2) mo_px(mx - w / 2 + x + sh, y, deep);
    }
    if (land.mist)                                      /* a rest: mist on the water, drifting */
        for (y = LAND_H - 5; y < LAND_H + 8; y++) {
            int32_t d = y - LAND_H - 1, sh = (int32_t)(fm1_ms / 350u);
            d = 8 - (d < 0 ? -d : d);
            for (x = 0; x < 120 && d > 0; x++)
                if ((uint32_t)LAND_BY(x + sh + (y & 1) * 2, y) * 32u < land.mist * (uint32_t)d) mo_px(x, y, deep);
        }
    if (land.fish == 1u) {                              /* the fish: out, over and back, nose first */
        uint32_t a = fm1_ms - land.ft0;
        int32_t fx = land.fx + (int32_t)(a * 18u / LAND_FISH_T), s = a * 2u < LAND_FISH_T ? -1 : 1,
                fy = LAND_H - 1 - 10 * mo_sin((int32_t)(a * 128u / LAND_FISH_T)) / 127;
        mo_px(fx + 1, fy + s, MO_INK), mo_px(fx, fy, MO_INK), mo_px(fx - 1, fy - s, MO_INK);
        mo_px(fx - 2, fy - s - 1, MO_INK), mo_px(fx - 2, fy - s + 1, MO_INK);
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
                  a > LAND_LIFE * 2u / 3u || land.tr[i] > 3u ? MO_DIM : MO_TRK(land.tr[i]));
        if (land.vel[i] > 80u && a > 200u && a < LAND_LIFE * 2u / 3u && r > 5)
            land_ring(land.x[i], land.y[i], r / 2, 1 + r / 2 * (b + 12) / 100, 0, MO_DIM);
    }
}
