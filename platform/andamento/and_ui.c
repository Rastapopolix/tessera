/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's panel UI over Tessera's core: a 128 x 64 screen with 16 levels (the yellow NHD-2.7-12864WDY3-M, so
 * Tessera's default Mono ink), 8 data encoders in 2 rows of 4, ENERGY, the 6 page buttons (GEN SOUND MOD FX PIECE
 * MIX), FUNC NO YES and the arrows, 16 keys in 2 rows of 8, REC PLAY STOP. First stage, for the virtual prototype
 * (web/andamento) and later the Daisy build: each page is a 2 x 4 grid of the selected track's settings, taken from
 * Tessera's central parameter list (pid.c), one per encoder. Included after the firmware (a unity build: it uses
 * pid_*, param_format, song, trk, transport_req, fm1_in and the Mono look's 5 x 7 face, MO_FONT).
 *
 * Controls (first stage):
 *   page button      that page; pressed again, its next sub-page (also UP / DOWN)
 *   LEFT / RIGHT     the selected track; FUNC + one of the first keys picks a track directly
 *   encoder 1..8     the cell's setting; pushed, back to its default
 *   ENERGY           ENERGY LEVEL on every page
 *   keys             the selected track's scale (bottom row low, top row high), as the FM-1's 16 white keys play it
 *   PLAY / STOP      transport (PLAY while playing restarts) */

#define AND_W 128
#define AND_H 64
#define AND_NENC 8

enum { AB_GEN, AB_SOUND, AB_MOD, AB_FX, AB_PIECE, AB_MIX, AB_FUNC, AB_NO, AB_YES, AB_UP, AB_DOWN, AB_LEFT, AB_RIGHT,
       AB_REC, AB_PLAY, AB_STOP, AB_COUNT };

static uint8_t and_fb[AND_H][AND_W];             /* one level 0..15 a pixel (the SSD1322's 4 bits) */
static struct {
    uint8_t page, sub[6], held[AB_COUNT];
    uint16_t cell[AND_NENC];                     /* this page's settings (PID_NONE: an empty cell) */
    int8_t hot;                                  /* the cell turned last (-1: none), shown until hot_ms + 1200 */
    uint32_t hot_ms, keys;
    uint16_t hot_pid;
} and_ui = {.hot = -1};

/* the FM-1's 16 white keys (F3 .. G5, seq.c key_black): what its keyboard plays as scale degrees */
static const uint8_t AND_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

/* ------------------------------------------------------------ drawing --- */
static void and_px(int32_t x, int32_t y, uint32_t l)
{
    if ((uint32_t)x < AND_W && (uint32_t)y < AND_H)
        and_fb[y][x] = (uint8_t)l;
}
static void and_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t l)
{
    int32_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            and_px(i, j, l);
}
/* text in the 5 x 7 face, 6 px a character; at most n characters; the width drawn */
static int32_t and_text(int32_t x, int32_t y, const char *s, uint32_t l, uint32_t n)
{
    int32_t x0 = x;
    for (; *s && n; s++, n--, x += 6) {
        uint32_t c = (uint8_t)*s, r, b;
        if (c >= 'a' && c <= 'z')
            c -= 32;                             /* (one case on this screen) */
        if (c < ' ' || c > 'z')
            c = '?';
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if ((MO_FONT[c - ' '][r] >> (4u - b)) & 1u)
                    and_px(x + (int32_t)b, y + (int32_t)r, l);
    }
    return x - x0;
}

/* ------------------------------------------------------------- pages --- */
static const char *const AND_PAGE[6] = {"GEN", "SOUND", "MOD", "FX", "PIECE", "MIX"};
static const uint8_t AND_SUBS[6] = {2, 2, 2, 1, 2, 1};
static const uint16_t AND_FIXED[6][2][AND_NENC] = {
    [0][1] = {0x20f, 0x20e, 0x20d, 0x210, 0x211, 0x212, 0x26e, 0x26a},   /* EVOLVE AMT EVERY LOCK ROLL, FORM, GRAVITY */
    [1][0] = {0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076},   /* the engine's 8 */
    [1][1] = {0x001, 0x002, 0x003, 0x004, 0x005, 0x026, 0x027, 0x000},   /* ADSR, ENV > FILTER, GLIDE, PAN, LEVEL */
    [2][0] = {0x009, 0x00a, 0x00d, 0x00e, 0x00f, 0x010, 0x05c, 0x00c},   /* the LFO */
    [2][1] = {0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x28d, 0x28e},   /* MOD 1, MOD 2, MACRO 1, 2 */
    [3][0] = {0x06c, 0x06d, 0x06e, 0x11b, 0x11c, 0x11f, 0x120, 0x127},   /* sends, send FX 1 and 2, return */
    [4][0] = {0x280, 0x281, 0x282, 0x284, 0x259, 0x25a, 0x27b, 0x27d},   /* PIECE, ENERGY ARC RESTS, ENTRIES */
    [4][1] = {0x272, 0x273, 0x274, 0x27c, 0x27e, 0x26d, 0x283, 0x279},   /* CHAIN, ENTRIES, DEPTH, COUNT, FOLLOW */
    [5][0] = {0x000, 0x027, 0x028, 0x100, 0x101, 0x019, 0x01a, 0x01c},   /* LEVEL PAN MUTE, TEMPO SWING, ROOT SCALE TRANS */
};
#define AND_PID_MODE 0x200u
#define AND_PID_ENERGY 0x26cu

/* GEN's first page: MODE and the generator's own settings (their names begin with its name), or STEP's pattern */
static void and_gen_cells(uint16_t *c)
{
    static const uint16_t STEP[7] = {0x01d, 0x01e, 0x01f, 0x020, 0x29b, 0x019, 0x01a};
    static const uint16_t FILL[3] = {0x216, 0x215, 0x26a};   /* TWEAK HUMAN, DROP, GRAVITY */
    const param_desc_t *d = pid_desc(AND_PID_MODE, song.sel);
    int32_t mode = pid_get(AND_PID_MODE, song.sel);
    uint32_t n = 1, i, k;
    c[0] = AND_PID_MODE;
    if (mode <= 0 || !d || !d->names) {
        for (i = 0; i < 7u; i++)
            c[1 + i] = STEP[i];
        return;
    }
    {
        const char *g = d->names[mode - d->min];
        uint32_t len = (uint32_t)strlen(g) < 5u ? (uint32_t)strlen(g) : 5u;
        for (i = 0; i < pid_count() && n < AND_NENC; i++) {
            const pid_entry_t *e = pid_at(i);
            if (e->pid > AND_PID_MODE && e->pid < 0x300u && !(e->flags & PID_F_ACTION) && !memcmp(e->name, g, len))
                c[n++] = e->pid;
        }
    }
    for (k = 0; n < AND_NENC; k++)
        c[n++] = k < 3u ? FILL[k] : PID_NONE;
}

static void and_cells(void)
{
    uint32_t i, p = and_ui.page, s = and_ui.sub[p];
    if (p == 0 && s == 0)
        and_gen_cells(and_ui.cell);
    else
        for (i = 0; i < AND_NENC; i++)
            and_ui.cell[i] = AND_FIXED[p][s][i] ? AND_FIXED[p][s][i] : PID_NONE;
    if (p == 0 && s == 0)
        return;
    for (i = 0; i < AND_NENC; i++)               /* (an engine's unused knob: no cell) */
        if (and_ui.cell[i] != PID_NONE) {
            const param_desc_t *d = pid_desc(and_ui.cell[i], song.sel);
            if (!d || d->max <= d->min)
                and_ui.cell[i] = PID_NONE;
        }
}

/* ------------------------------------------------------------- input --- */
static void and_turn(uint32_t pid, int32_t n)
{
    const param_desc_t *d = pid_desc(pid, song.sel);
    if (!d || !n)
        return;
    pid_set(pid, song.sel, pid_get(pid, song.sel) + n);
    if (pid == P_SCALE || pid == P_ROOT)         /* (as the FM-1's knobs: one scale for the song) */
        scale_share(&trk[song.sel]);
    and_ui.hot_pid = (uint16_t)pid;
    and_ui.hot_ms = fm1_ms | 1u;                 /* (0: nothing turned yet) */
}

/* encoder i (0..7 the grid, 8 ENERGY) turned by n detents */
static void and_enc(uint32_t i, int32_t n)
{
    and_cells();
    if (i == AND_NENC) {
        and_turn(AND_PID_ENERGY, n * 2);
        and_ui.hot = -1;
    } else if (i < AND_NENC && and_ui.cell[i] != PID_NONE) {
        and_turn(and_ui.cell[i], n);
        and_ui.hot = (int8_t)i;
    }
}
/* encoder i pushed: its setting back to the default */
static void and_enc_push(uint32_t i)
{
    uint32_t pid;
    const param_desc_t *d;
    and_cells();
    pid = i == AND_NENC ? AND_PID_ENERGY : i < AND_NENC ? and_ui.cell[i] : PID_NONE;
    if (pid == PID_NONE || !(d = pid_desc(pid, song.sel)))
        return;
    pid_set(pid, song.sel, d->def);
    and_ui.hot = i < AND_NENC ? (int8_t)i : -1;
    and_ui.hot_pid = (uint16_t)pid;
    and_ui.hot_ms = fm1_ms | 1u;
}

static void and_track(int32_t d)
{
    song.sel = (uint8_t)((song.sel + NTRK + d) % NTRK);
}

static void and_button(uint32_t b, uint32_t down)
{
    if (b >= AB_COUNT)
        return;
    and_ui.held[b] = (uint8_t)down;
    if (!down)
        return;
    if (b <= AB_MIX) {
        if (and_ui.page == b)
            and_ui.sub[b] = (uint8_t)((and_ui.sub[b] + 1u) % AND_SUBS[b]);
        and_ui.page = (uint8_t)b;
        and_ui.hot = -1;
    } else if (b == AB_UP || b == AB_DOWN) {
        uint32_t p = and_ui.page;
        and_ui.sub[p] = (uint8_t)((and_ui.sub[p] + (b == AB_UP ? AND_SUBS[p] - 1u : 1u)) % AND_SUBS[p]);
    } else if (b == AB_LEFT || b == AB_RIGHT) {
        and_track(b == AB_LEFT ? -1 : 1);
    } else if (b == AB_PLAY) {
        transport_req = song.playing ? 3u : 1u;
    } else if (b == AB_STOP) {
        transport_req = 2u;
    }
}

/* key k (0..7 the bottom row, 8..15 the top) down or up */
static void and_key(uint32_t k, uint32_t down)
{
    uint32_t bit;
    if (k >= 16u)
        return;
    if (down && and_ui.held[AB_FUNC]) {          /* FUNC + a key: that track */
        if (k < NTRK)
            song.sel = (uint8_t)k;
        return;
    }
    bit = 1u << AND_WHITE[k];
    if (down)
        and_ui.keys |= 1u << k, fm1_in.notes |= bit;
    else
        and_ui.keys &= ~(1u << k), fm1_in.notes &= ~bit;
}

/* --------------------------------------------------------------- draw --- */
static void and_value(uint32_t pid, char *val)
{
    const param_desc_t *d = pid_desc(pid, song.sel);
    const char *unit;
    val[0] = 0;
    if (d)
        param_format(d, pid_get(pid, song.sel), val, &unit);
}

static void and_draw(void)
{
    char s[24], val[16];
    uint32_t i, tr, hot = and_ui.hot_ms && fm1_ms - and_ui.hot_ms < 1200u;
    const pid_entry_t *e;
    and_cells();
    memset(and_fb, 0, sizeof and_fb);
    /* the header: the page (or the setting just turned, by its full name), the track, transport and tempo */
    if (hot && (e = pid_find(and_ui.hot_pid)))
        and_text(1, 0, e->name, 15, 15);
    else {
        int32_t x = and_text(1, 0, AND_PAGE[and_ui.page], 15, 5);
        if (AND_SUBS[and_ui.page] > 1u) {
            s[0] = (char)('1' + and_ui.sub[and_ui.page]);
            s[1] = 0;
            and_text(x + 3, 0, s, 6, 1);
        }
        s[0] = 'T';
        s[1] = (char)('1' + song.sel);
        s[2] = 0;
        and_text(52, 0, s, 15, 2);
    }
    and_text(92, 0, song.playing ? ">" : "-", song.playing ? 15 : 6, 1);
    {
        uint32_t bpm = (uint32_t)pid_get(0x100, 0);
        s[0] = (char)('0' + bpm / 100 % 10);
        s[1] = (char)('0' + bpm / 10 % 10);
        s[2] = (char)('0' + bpm % 10);
        s[3] = 0;
        and_text(108, 0, bpm < 100 ? s + 1 : s, 10, 3);
    }
    and_fill(0, 9, AND_W, 1, 3);
    /* the 2 x 4 grid: each cell its label (dim) over its value, the cell turned last lit */
    for (i = 0; i < AND_NENC; i++) {
        int32_t x = (int32_t)(i % 4u) * 32, y = 12 + (int32_t)(i / 4u) * 20;
        uint32_t pid = and_ui.cell[i];
        const param_desc_t *d;
        if (pid == PID_NONE || !(d = pid_desc(pid, song.sel))) {
            and_text(x + 1, y + 8, "-", 3, 1);
            continue;
        }
        and_text(x + 1, y, d->label ? d->label : "", 7, 5);
        and_value(pid, val);
        if (hot && and_ui.hot == (int8_t)i) {
            and_fill(x, y + 8, 31, 9, 15);
            and_text(x + 1, y + 9, val, 0, 5);
        } else {
            and_text(x + 1, y + 9, val, 15, 5);
        }
    }
    /* the tracks along the foot: number and generator, the selected one inverted, a bar while it sounds; ENERGY */
    and_fill(0, 52, AND_W, 1, 3);
    for (tr = 0; tr < NTRK; tr++) {
        int32_t x = (int32_t)tr * 24;
        uint32_t k, on = 0, sel = tr == song.sel;
        const param_desc_t *d = pid_desc(AND_PID_MODE, tr);
        int32_t mode = pid_get(AND_PID_MODE, tr);
        s[0] = (char)('1' + tr);
        s[1] = 0;
        if (sel)
            and_fill(x, 54, 23, 9, 15);
        and_text(x + 1, 55, s, sel ? 0 : 15, 1);
        and_text(x + 8, 55, d && d->names ? d->names[mode - d->min] : "", sel ? 0 : 8, 2);
        for (k = 0; k < NVOICE; k++)
            on += trk[tr].v[k].active;
        if (on)
            and_fill(x, 63, 23, 1, sel ? 15 : 10);
        if (pid_get(0x028, tr))
            and_fill(x + 1, 58, 21, 1, sel ? 0 : 15);   /* muted: struck through */
    }
    {
        int32_t en = pid_get(AND_PID_ENERGY, 0), w = (en + 100) * 20 / 200;
        and_text(100, 55, "E", 10, 1);
        and_fill(107, 57, 20, 3, 3);
        and_fill(107, 57, w > 20 ? 20 : w, 3, 15);
    }
}

/* the LEDs: bit b = button b lit; keys: bit k = key k lit */
static uint32_t and_led_buttons(void)
{
    uint32_t m = 1u << and_ui.page, b;
    for (b = AB_FUNC; b < AB_COUNT; b++)
        if (and_ui.held[b])
            m |= 1u << b;
    if (song.playing)
        m |= 1u << AB_PLAY;
    return m;
}
static uint32_t and_led_keys(void)
{
    return and_ui.keys;
}
