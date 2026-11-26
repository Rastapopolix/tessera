/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The TRACKS page: a second screen beside the FM-1's (a profile with TESS_SCR_COUNT > 1: the plugin's EXPANDED mode;
 * tessera-notes docs/design/PLUGIN_EXPANDED.md). Every track at once, a row each: its number, its role (PLCK, BASS,
 * DRUM ..), the engine, the pattern playing, the 16 steps of the bar it is in (a hit a block, the playhead boxed on each
 * row, each at its own step), its level, M and S. The selected track's row boxed, its number inverted; a muted row in
 * the dim ink, M inverted (blinking while MENU MUTES makes it wait); a soloed track's S in full ink.
 *
 * Drawn in cells of the Mono face (5 x 7, ui_mono.c's inks) from the size it is asked for, into a buffer of its own (not
 * the FM-1's screen): 480 x 240 at 2 px a cell (the plugin), or 128 x 64 at 1 (21 characters a row: number, role,
 * pattern, 8 steps, level; Andamento's dial screen). With TESS_HAS_POINTER it leaves zones (a row, M, S, the level)
 * that tess_core_tracks_do acts on. Without TESS_SCR_COUNT > 1 nothing here is built: the FM-1's image is unchanged.
 * Included by core_ui_sources.h after ui_layer.c */
#if TESS_SCR_COUNT > 1 && MELODEE_LOOK
#define T2_W_MAX 480
#define T2_H_MAX 240
#define T2_ZONES 72                                     /* (MIXER: 8 strips of eight; the rows: 8 of four; the tempo) */
static uint16_t t2_px[T2_W_MAX * T2_H_MAX];             /* RGB565 (ui_mono.c's inks as they are) */
static int32_t t2_w, t2_h, t2_sc;                       /* the size drawn, the pixels a cell */
static uz_t t2_z[T2_ZONES];
static uint32_t t2_zn;

static void t2_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* in cells */
{
    int32_t x0 = x * t2_sc, y0 = y * t2_sc, x1 = x0 + w * t2_sc, y1 = y0 + h * t2_sc, i, j;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > t2_w) x1 = t2_w;
    if (y1 > t2_h) y1 = t2_h;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++)
            t2_px[j * t2_w + i] = c;
}
static void t2_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    t2_fill(x, y, w, 1, c);
    t2_fill(x, y + h - 1, w, 1, c);
    t2_fill(x, y, 1, h, c);
    t2_fill(x + w - 1, y, 1, h, c);
}
static int32_t t2_text(const char *s, int32_t x, int32_t y, uint16_t c)
{
    for (; *s; s++, x += 6) {
        uint32_t ch = mono_ch((uint8_t)*s), r, b;
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if (MO_FONT[ch - 32u][r] & (16u >> b))
                    t2_fill(x + (int32_t)b, y + (int32_t)r, 1, 1, c);
    }
    return x;
}
static void t2_textr(const char *s, int32_t rx, int32_t y, uint16_t c) { t2_text(s, rx - mo_tw(s), y, c); }
static void t2_inv(const char *s, int32_t x, int32_t y, uint16_t box, uint16_t c)   /* on a filled box */
{
    t2_fill(x - 1, y - 1, mo_tw(s) + 2, 9, box);
    t2_text(s, x, y, c);
}
/* a zone of cells (TESS_HAS_POINTER): kind UZ_TRACK a row (a strip's head), T2_Z_MUTE, T2_Z_SOLO, T2_Z_LEVEL (role -1
 * the level, 0..2 FX SEND 1..3), T2_Z_PAN; row the track */
enum { T2_Z_MUTE = 5, T2_Z_SOLO, T2_Z_LEVEL, T2_Z_PAN = 9 };
static void t2_zone_r(uint32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t tr, const char *name,
                      const char *value, int32_t role)
{
#if TESS_HAS_POINTER
    uz_t *z;
    if (t2_zn >= T2_ZONES) return;
    z = &t2_z[t2_zn++];
    z->x = (int16_t)(x * t2_sc), z->y = (int16_t)(y * t2_sc), z->w = (int16_t)(w * t2_sc), z->h = (int16_t)(h * t2_sc);
    z->kind = (uint8_t)kind, z->group = UZ_G_TRACKS, z->role = (int8_t)role, z->vrole = -1, z->row = (int16_t)tr;
    str_cpy(z->name, name, sizeof z->name);
    str_cpy(z->value, value, sizeof z->value);
#else
    (void)kind, (void)x, (void)y, (void)w, (void)h, (void)tr, (void)name, (void)value, (void)role;
#endif
}
static void t2_zone(uint32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t tr, const char *name,
                    const char *value)
{
    t2_zone_r(kind, x, y, w, h, tr, name, value, -1);
}

/* track k's role, four letters, as HOME's column has it (ui_mono.c mo_stage_col): REC armed, its sound's category, else
 * T<k> */
static void t2_role(uint32_t k, char *a)
{
    uint32_t src, kk, cat;
    stage_entry(k, &src, &kk);
    cat = entry_cat(src, kk) % CAT_N;
    if ((song.rec >> k) & 1u) str_cpy(a, "REC", 5);
    else if (cat == CAT_DRUM) str_cpy(a, "DRUM", 5);
    else if (cat == CAT_KEYS) str_cpy(a, "CHRD", 5);
    else if (cat && cat != CAT_OTHER) mo_abbr(a, CAT_NAME[cat]);
    else a[0] = 'T', a[1] = (char)('1' + k), a[2] = 0;
    a[4] = 0;
}
static void t2_level(uint32_t k, char *a)               /* dB, or OFF */
{
    uint32_t lvl = (uint32_t)trk[k].p[P_LEVEL] & 127u;
    if (lvl) {
        int32_t d = level_db_x10(lvl);
        fmt_int(a, (d + (d < 0 ? -5 : 5)) / 10);
    } else {
        str_cpy(a, "OFF", 4);
    }
}

static uint32_t t2_meter(uint32_t k);
static uint8_t t2_hot[NTRK];                            /* a note's flash: draws left (voice.c t2_note) */

/* one row at y (cells): x of the steps, how many, their pitch; compact: Andamento's 21 characters. Live motion (Cosmati
 * audit G8): at the left edge a meter (the loudest voice's envelope now), and the role inverted for a few draws when a
 * note starts */
static void t2_row(uint32_t k, int32_t y, int32_t rh, int compact, uint32_t blink)
{
    const track_t *t = &trk[k];
    uint32_t sel = k == song.sel, mute = t->p[P_MUTE] != 0u, wait = (mute_waiting() >> k) & 1u;
    uint32_t solo = (track_solo >> k) & 1u, len = (uint32_t)t->p[P_SLEN], n = compact ? 8u : 16u, i;
    uint32_t ph = song.playing && t->seq_idx < len ? t->seq_idx : 0xFFFFu, first = ph < len ? ph / n * n : 0u;
    uint16_t ink = mute ? mo_tdim(k) : MO_TRK(k), dim = mo_tdim(k);
    int32_t sx = compact ? 56 : 88, ty = y + (rh - 7) / 2, lx = compact ? 127 : 217;
    char b[16], nm[8], lv[8];
    b[0] = (char)('1' + k), b[1] = 0;
    if (sel) {
        t2_box(0, y, compact ? t2_w / t2_sc : 240, rh, MO_TRK(k));
        t2_inv(b, compact ? 1 : 4, ty, MO_TRK(k), MO_BG);
    } else {
        t2_text(b, compact ? 1 : 4, ty, ink);
    }
    t2_role(k, nm);
    if (t2_note[k])
        t2_note[k] = 0, t2_hot[k] = 3;
    if (t2_hot[k] && !compact) {
        t2_hot[k]--;
        t2_inv(nm, 13, ty, ink, MO_BG);
    } else {
        t2_text(nm, compact ? 12 : 13, ty, ink);
    }
    if (!compact) {                                     /* the meter: two cells wide, up from the row's foot */
        int32_t mh = (int32_t)t2_meter(k) * (rh - 2) / 127;
        if (mh > 0)
            t2_fill(1, y + rh - 1 - mh, 2, mh, ink);
    }
    if (!compact) {
        str_cpy(b, eng_abbr(ENGINES[eng_idx(t->eng_req)]->name), 6);
        t2_text(b, 40, ty, dim);
    }
    b[0] = 'P', b[1] = (char)('1' + t->pattern % NPAT), b[2] = 0;
    t2_text(b, compact ? 40 : 74, ty, ink);
    for (i = 0; i < n; i++) {                           /* the steps of the window the playhead is in */
        uint32_t si = first + i;
        int32_t x = sx + (int32_t)i * 6 + (compact ? 0 : 2 * (int32_t)(i / 4u)), cy = y + rh / 2;   /* (16: a clear
                                                                 * gap before the level, "-INF" too) */
        if (si >= len)
            break;
        if (step_on(&seq_steps(t)[si % NSTEP])) t2_fill(x, cy - 3, 5, 6, ink);
        else t2_fill(x + 1, cy + 2, 3, 1, dim);
        if (si == ph) t2_box(x - 1, cy - 4, 7, 8, MO_TRK(k));
    }
    t2_level(k, lv);
    t2_textr(lv, lx, ty, ink);
    if (!compact) {
        if (mute && !(wait && blink)) t2_inv("M", 222, ty, dim, MO_BG);
        else if (wait && blink) t2_inv("M", 222, ty, MO_TRK(k), MO_BG);
        else t2_text("M", 222, ty, dim);
        t2_text("S", 231, ty, solo ? MO_INK : dim);
        b[0] = 'T', b[1] = (char)('1' + k), b[2] = 0;
        t2_zone(T2_Z_MUTE, 220, y, 9, rh, k, "MUTE", mute ? "ON" : "OFF");
        t2_zone(T2_Z_SOLO, 229, y, 9, rh, k, "SOLO", solo ? "ON" : "OFF");
        t2_zone(T2_Z_LEVEL, lx - 20, y, 21, rh, k, "LEVEL", lv);
        trk_short_name(k, b);
        t2_zone(UZ_TRACK, 0, y, 220, rh, k, nm, b);
    }
}

/* MIXER (Cosmati audit I1): track k's strip, 30 cells wide from x. Its number (inverted when selected) and role; a
 * fader (the level, 0..127, a bar from the bottom) and beside it a meter (the loudest voice's envelope now); the level in
 * dB; PAN as a marker on a line; the FX sends as bars (one in CHAIN, three in SPLIT); M and S. Zones: the head selects,
 * the fader and the sends turn (dragged, the wheel), PAN too, M and S click */
static uint32_t t2_meter(uint32_t k)
{
    uint32_t i, m = 0;
    for (i = 0; i < NVOICE; i++) {
        const voice_t *v = &trk[k].v[i];
        uint32_t e = v->active && v->env_out > 0 ? (uint32_t)v->env_out >> 8 : 0u;
        if (e > m) m = e;
    }
    return m > 127u ? 127u : m;
}
static void t2_strip(uint32_t k, int32_t x, uint32_t blink)
{
    static const char *const SN[3] = {"SEND 1", "SEND 2", "SEND 3"};
    const track_t *t = &trk[k];
    uint32_t sel = k == song.sel, mute = t->p[P_MUTE] != 0u, wait = (mute_waiting() >> k) & 1u;
    uint32_t solo = (track_solo >> k) & 1u, lvl = (uint32_t)t->p[P_LEVEL] & 127u, mt = t2_meter(k);
    uint32_t ns = song.g[G_FXMODE] ? FX_SLOTS : 1u, i;
    uint16_t ink = mute ? mo_tdim(k) : MO_TRK(k), dim = mo_tdim(k);
    int32_t fh, pan = t->p[P_PAN], cx = x + 15;
    char b[16], nm[8], lv[8];
    b[0] = (char)('1' + k), b[1] = 0;                   /* the head: number, role */
    if (sel) t2_box(x + 1, 13, 28, 93, MO_TRK(k));   /* (round M and S too, a clear cell under them) */
    if (sel) t2_inv(b, cx - 2, 15, MO_TRK(k), MO_BG);
    else t2_text(b, cx - 2, 15, ink);
    t2_role(k, nm);
    t2_text(nm, cx - mo_tw(nm) / 2, 24, dim);
    trk_short_name(k, b);
    t2_zone(UZ_TRACK, x, 13, 30, 19, k, nm, b);
    t2_fill(x + 9, 33, 1, 40, dim);                     /* the fader: its slot, the level from the bottom, a cap */
    fh = (int32_t)(lvl * 40u / 127u);
    t2_fill(x + 7, 73 - fh, 5, fh, ink);
    t2_fill(x + 5, 72 - fh, 9, 1, MO_INK);
    t2_box(x + 17, 33, 5, 40, dim);                     /* the meter */
    fh = (int32_t)(mt * 38u / 127u);
    if (fh) t2_fill(x + 18, 72 - fh, 3, fh, mt > 120u ? MO_INK : ink);
    t2_level(k, lv);
    t2_zone(T2_Z_LEVEL, x + 3, 32, 24, 42, k, "LEVEL", lv);
    t2_text(lv, cx - mo_tw(lv) / 2, 75, ink);
    t2_fill(x + 3, 85, 24, 1, dim);                     /* PAN: a line, its centre, a marker */
    t2_fill(cx, 84, 1, 3, dim);
    t2_fill(cx + (int32_t)(pan * 12 / 64) - 1, 83, 3, 5, ink);
    fmt_int(b, pan);
    t2_zone(T2_Z_PAN, x + 2, 82, 26, 7, k, "PAN", b);
    for (i = 0; i < ns; i++) {                          /* the sends: a bar each, two cells high */
        int32_t y = 90 + (int32_t)i * 3, sv = t->p[P_FXSEND + i];
        t2_fill(x + 3, y + 1, 24, 1, dim);
        if (sv) t2_fill(x + 3, y, sv * 24 / 127 < 1 ? 1 : sv * 24 / 127, 2, ink);
        fmt_int(b, sv * 100 / 127);
        t2_zone_r(T2_Z_LEVEL, x + 2, y - 1, 26, 3, k, ns > 1u ? SN[i] : "FX SEND", b, (int32_t)i);
    }
    if (mute && !(wait && blink)) t2_inv("M", x + 7, 99, dim, MO_BG);   /* M and S */
    else if (wait && blink) t2_inv("M", x + 7, 99, MO_TRK(k), MO_BG);
    else t2_text("M", x + 7, 99, dim);
    t2_text("S", x + 19, 99, solo ? MO_INK : dim);
    t2_zone(T2_Z_MUTE, x + 3, 98, 12, 8, k, "MUTE", mute ? "ON" : "OFF");
    t2_zone(T2_Z_SOLO, x + 15, 98, 12, 8, k, "SOLO", solo ? "ON" : "OFF");
}

/* the page, w x h pixels (at most 480 x 240): 2 px a cell from 240 high, else 1. blink: MENU MUTES' wait, on or off */
static void tracks_draw(int32_t w, int32_t h, uint32_t blink)
{
    uint32_t k;
    int32_t i, cw, ch, compact, mixer;
    char b[24];
    if (w > T2_W_MAX) w = T2_W_MAX;
    if (h > T2_H_MAX) h = T2_H_MAX;
    t2_w = w, t2_h = h, t2_sc = h >= 240 ? 2 : 1, t2_zn = 0;
    cw = w / t2_sc, ch = h / t2_sc, compact = cw < 200;
    for (i = 0; i < w * h; i++) t2_px[i] = MO_BG;
    if (compact) {                                      /* eight rows of 8 cells */
        for (k = 0; k < NTRK; k++) t2_row(k, (int32_t)k * (ch / 8), ch / 8, 1, blink);
        return;
    }
    mixer = !ui.home && cur_page()->graph == GR_TRK;    /* (MIXER the page: the strips) */
    str_cpy(b, mixer ? NTRK > 4u ? "MIXER 1-8" : "MIXER 1-4" : NTRK > 4u ? "TRACKS 1-8" : "TRACKS 1-4", sizeof b);
    t2_text(b, 4, 2, MO_INK);
    if (song.playing) {                                 /* PLAY a triangle, STOP a square; the tempo */
        int32_t r;
        for (r = 0; r < 7; r++) t2_fill(100 + r / 2, 2 + r / 2, 1, 7 - r / 2 * 2, MO_INK);
    } else {
        t2_fill(100, 3, 5, 5, MO_INK);
    }
    fmt_int(b, song.g[G_BPM]);
    t2_text(b, 108, 2, MO_DIM);
    t2_zone(UZ_TEMPO, 98, 0, 30, 11, 0, "TEMPO", b);    /* (dragged, the wheel: the BPM, as the FM-1 screen's) */
    if (song.playing) {
        char n[12];
        str_cpy(b, "BAR ", sizeof b);
        fmt_int(n, (int32_t)bar_n + 1);
        str_cpy(b + 4, n, sizeof b - 4);
        k = str_len(b);
        b[k++] = '.', b[k++] = (char)('1' + beat_n % 4u), b[k] = 0;
        t2_textr(b, 236, 2, MO_DIM);
    }
    t2_fill(0, 11, 240, 1, MO_DIM);
    if (mixer) {                                        /* MIXER: a strip a track */
        for (k = 0; k < NTRK; k++)
            t2_strip(k, (int32_t)k * (240 / (int32_t)NTRK) + (240 / (int32_t)NTRK - 30) / 2, blink);
    } else {
        for (k = 0; k < NTRK; k++)                      /* the rows */
            t2_row(k, 14 + (int32_t)k * 11, 11, 0, blink);
    }
    t2_fill(0, 107, 240, 1, MO_DIM);                    /* the selected track's sound; the hint */
    b[0] = 'T', b[1] = (char)('1' + song.sel % NTRK), b[2] = ' ';
    trk_short_name(song.sel % NTRK, b + 3);
    t2_text(b, 4, 111, MO_INK);
#if TESS_HAS_POINTER
    t2_textr(mixer ? "DRAG A FADER, CLICK M OR S" : "CLICK A ROW TO SELECT", 236, 111,
             MO_DIM);
#endif
}

#endif
