/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The Mono look (MELODEE_LOOK 1, a build of its own; tessera-notes/docs/design/MONO_UI.md): Monomachine and Squares &
 * Circles' face. The screen as 120 x 120 cells of 2 x 2 pixels, one ink on the palette's background, a 5 x 7 font. Header 0..9 (the page,
 * the engine, the four tracks as dots), the four knobs as dials 11..49, the picture 51..108, the footer's line 109 and
 * page tabs 111..119 (a clear row between them). Step 1, the prototype: every knob page (draw_page's own branch) gets the header, the dials and the
 * footer; GEN, PATTERN and LFO their pictures, the other pictures a stand-in. Stage, lists, popups, the browser and
 * the other screens of their own keep the detailed look for now. The knobs' labels and values come from
 * draw_columns as ever: draw_column hands them here (mo_capture) while a Mono page is drawn. The page is drawn whole
 * in two 240 x 120 canvases when anything on it changes (mo_sig); cv_blit sends only the rows that differ.
 * Included by ui_draw.c */
#define MO_BG T_BG                                      /* one ink: the palette's (NIGHT: white on black, DAY the reverse) */
#define MO_INK T_TEXT
#define MO_DIM T_MID
#define MO_PY0 52                                       /* the picture's rows: MO_PY0 .. MO_PY1 - 1 */
#define MO_PY1 109

static const int8_t MO_SIN[64] = {0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12};

static struct {
    char label[8], val[12], unit[8];
    int32_t ratio;                                      /* 0..1000, -1: a list value (boxed, no dial) */
    uint8_t used, dim, hot;
} mo_col[4];
static uint32_t mo_sig;

static void mo_capture(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc, int32_t ratio,
                       int hot)
{
    if (c >= 4u || (!label[0] && !val[0]))
        return;
    str_cpy(mo_col[c].label, label, sizeof mo_col[c].label);
    str_cpy(mo_col[c].val, val, sizeof mo_col[c].val);
    str_cpy(mo_col[c].unit, str_eq(unit, label) ? "" : unit, sizeof mo_col[c].unit);
    mo_col[c].ratio = ratio > 1000 ? 1000 : ratio;
    mo_col[c].used = 1;
    mo_col[c].dim = vc == T_DIM;
    mo_col[c].hot = (uint8_t)hot;
}

/* ---- drawing in cells (2 x 2 pixels; cv_oy picks the canvas's half) ---- */
static void mo_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    if (w > 0 && h > 0)
        cv_rect(x * 2, y * 2, w * 2, h * 2, c);
}
static void mo_px(int32_t x, int32_t y, uint16_t c) { mo_fill(x, y, 1, 1, c); }
static void mo_hl(int32_t x, int32_t y, int32_t w, uint16_t c) { mo_fill(x, y, w, 1, c); }
static void mo_vl(int32_t x, int32_t y, int32_t h, uint16_t c) { mo_fill(x, y, 1, h, c); }
static void mo_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* an outline */
{
    mo_hl(x, y, w, c);
    mo_hl(x, y + h - 1, w, c);
    mo_vl(x, y, h, c);
    mo_vl(x + w - 1, y, h, c);
}
static void mo_dash(int32_t x, int32_t y, int32_t w, uint16_t c)
{
    for (int32_t i = 0; i < w; i += 2)
        mo_px(x + i, y, c);
}
static void mo_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0, sx = x0 < x1 ? 1 : -1,
            sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        int32_t e2 = 2 * e;
        mo_px(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) e += dy, x0 += sx;
        if (e2 <= dx) e += dx, y0 += sy;
    }
}
static void mo_circ(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        mo_px(cx + x, cy + y, c); mo_px(cx + y, cy + x, c); mo_px(cx - y, cy + x, c); mo_px(cx - x, cy + y, c);
        mo_px(cx - x, cy - y, c); mo_px(cx - y, cy - x, c); mo_px(cx + y, cy - x, c); mo_px(cx + x, cy - y, c);
        y++;
        if (e < 0) e += 2 * y + 1;
        else x--, e += 2 * (y - x) + 1;
    }
}
static void mo_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    for (int32_t y = -r; y <= r; y++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + y * y <= r * r + r)
            w++;
        mo_hl(cx - w, cy + y, 2 * w + 1, c);
    }
}
/* sine of a (256 a turn), x 127 */
static int32_t mo_sin(int32_t a)
{
    uint32_t i = ((uint32_t)a & 255u) >> 2, f = (uint32_t)a & 3u;
    return (MO_SIN[i] * (int32_t)(4u - f) + MO_SIN[(i + 1u) & 63u] * (int32_t)f) / 4;
}
static int32_t mo_cos(int32_t a) { return mo_sin(a + 64); }

static int32_t mo_tw(const char *s) { int32_t n = (int32_t)str_len(s); return n ? n * 6 - 1 : 0; }
static int32_t mo_text_s(const char *s, int32_t x, int32_t y, int32_t sc, uint16_t c)   /* sc: cells a font pixel */
{
    for (; *s; s++, x += 6 * sc) {
        uint32_t ch = mono_ch((uint8_t)*s), r, b;
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if (MO_FONT[ch - 32u][r] & (16u >> b))
                    mo_fill(x + (int32_t)b * sc, y + (int32_t)r * sc, sc, sc, c);
    }
    return x;
}
static int32_t mo_text(const char *s, int32_t x, int32_t y, uint16_t c) { return mo_text_s(s, x, y, 1, c); }
static void mo_textc(const char *s, int32_t cx, int32_t y, uint16_t c) { mo_text(s, cx - mo_tw(s) / 2, y, c); }
static void mo_textr(const char *s, int32_t rx, int32_t y, uint16_t c) { mo_text(s, rx - mo_tw(s), y, c); }
static void mo_inv(const char *s, int32_t x, int32_t y)   /* ink on a filled box */
{
    mo_fill(x - 1, y - 1, mo_tw(s) + 2, 9, MO_INK);
    mo_text(s, x, y, MO_BG);
}
static int mo_vowel(char c)
{
    c = c >= 'a' && c <= 'z' ? (char)(c - 32) : c;
    return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}
/* four letters at most: the spaces out (SNP A: SNPA); longer, the first letter and the consonants after it (EVERY:
 * EVRY, PULSE: PLS); fewer than three of those, the first four letters */
static void mo_abbr(char *d, const char *s)
{
    char w[16];
    uint32_t i, k = 0, n = 0;
    for (i = 0; s[i] && n < 15u; i++)
        if (s[i] != ' ' && s[i] != '.')
            w[n++] = s[i];
    w[n] = 0;
    if (n <= 4u) {
        str_cpy(d, w, 5);
        return;
    }
    d[k++] = w[0];
    for (i = 1; i < n && k < 4u; i++)
        if (!mo_vowel(w[i]))
            d[k++] = w[i];
    if (k < 3u)
        k = 4u, str_cpy(d, w, 5);
    d[k] = 0;
}
/* the page's name: an engine's EDIT pages by the engine's names (CHIP, FILTER), as the detailed header */
static const char *mo_title(const page_t *pg)
{
    const char *t = pg->scope == SC_ENGINE ? ENGINES[TSEL->eng_req % NENGINES]->page_title[pg->id[0] != P_E0] : 0;
    return t && t[0] ? t : pg->title;
}
static void mo_cut(char *d, const char *s, uint32_t n)   /* s's first n characters */
{
    uint32_t i;
    for (i = 0; i < n && s[i]; i++)
        d[i] = s[i];
    d[i] = 0;
}
/* a dial: the ring and a pointer, v 0..1000 over 270 degrees from the lower left */
static void mo_dial(int32_t cx, int32_t cy, int32_t r, int32_t v, uint16_t c)
{
    int32_t a = -160 + v * 192 / 1000;                  /* -225 .. +45 degrees, 256 a turn */
    mo_circ(cx, cy, r, c);
    mo_line(cx, cy, cx + mo_cos(a) * (r - 1) / 127, cy + mo_sin(a) * (r - 1) / 127, c);
}

/* ---- header, dials, footer ---- */
static void mo_head(void)
{
    const page_t *pg = cur_page();
    char t[24], sub[8];
    uint32_t i;
    str_cpy(sub, eng_abbr(ENGINES[TSEL->eng_req % NENGINES]->name), sizeof sub);
    if (ui.msg_t && ui.msg[0]) {                        /* a message: in the title's place, inverted */
        mo_cut(t, ui.msg, 15);
        mo_inv(t, 2, 1);
    } else {
        str_cpy(t, mo_title(pg), sizeof t);
        if (pg->fam == FAM_GEN && pg->graph == GR_STEPS && song.sel < TESSERA_TRACKS) {
            str_cpy(t + str_len(t), "/", 2);
            str_cpy(t + str_len(t), TX_N_MODE[tessera.mode[song.sel] % TESS_ENG_COUNT], 8);   /* (pid.c) */
        }
        t[(94 - mo_tw(sub) - 4) / 6] = 0;               /* (room before the engine) */
        mo_text(t, 1, 1, MO_INK);
    }
    mo_textr(sub, 94, 1, MO_DIM);
    for (i = 0; i < 4u; i++) {                          /* the tracks: the selected one filled */
        if (i == song.sel) mo_disc(99 + (int32_t)i * 6, 4, 2, MO_INK);
        else mo_circ(99 + (int32_t)i * 6, 4, 2, MO_INK);
    }
    if (song.rec & (1u << song.sel))                    /* REC: a bar under the track's dot */
        mo_hl(97 + (int32_t)song.sel * 6, 8, 5, MO_INK);
    mo_hl(0, 9, 120, MO_INK);
}
/* a value as its column shows it: five characters at most (MORPH, 0.25); longer, a number loses its last decimals
 * (128.00: 128.0) and a word its vowels from the end (MARKOV: MARKV, REVERB: REVRB), then an H; still longer, cut (1: the Mono
 * lint reports it) */
static int mo_fit(char *d, const char *s)
{
    uint32_t n, i, j;
    str_cpy(d, s, 16);
    n = str_len(d);
    if (n > 5u && (d[0] == '-' || d[0] == '+' || (d[0] >= '0' && d[0] <= '9'))) {
        for (i = 0; d[i] && d[i] != '.'; i++) ;
        while (d[i] && n > 5u && n > i + 1u)
            d[--n] = 0;
        if (d[i] && n == i + 1u)
            d[--n] = 0;                                 /* (no decimal left: no point) */
    }
    for (i = n; n > 5u && i-- > 1u;)
        if (mo_vowel(d[i])) {
            for (j = i; j < n; j++) d[j] = d[j + 1u];
            n--;
        }
    for (i = n; n > 5u && i-- > 1u;)                    /* .. then an H after a consonant (RHYTHM: RHYTM) */
        if (d[i] == 'H' && d[i - 1u] >= 'A' && d[i - 1u] <= 'Z' && !mo_vowel(d[i - 1u])) {
            for (j = i; j < n; j++) d[j] = d[j + 1u];
            n--;
        }
    if (n <= 5u)
        return 0;
    d[5] = 0;
    return 1;
}
/* the four knobs' values as drawn, for the Mono lint (tests/ui_render.c): the frame (a box, a filled box or the
 * inverted block; -1 none), the text, the kind */
enum { MV_DIAL, MV_BOX, MV_KEY, MV_INV };
static struct { int16_t fx0, fx1, tx0, tx1; uint8_t kind, cut, used; char s[8]; } mo_vals[4];
static uint8_t mo_flash_col, mo_flash_t;               /* a one-shot knob's value inverted a moment (CLEAR) */
static void mo_flash(uint32_t c)
{
    mo_flash_col = (uint8_t)c;
    mo_flash_t = 13;                                    /* frames: about 200 ms */
}
static uint8_t mo_vals_new;                             /* mo_cols drew them since the lint last looked */
#define MO_BOX_W 27                                     /* a boxed value's outline: 4 characters and a clear cell each side */
static void mo_cols(void)
{
    char v[4][16];
    uint8_t wide[4], kind[4], cut[4];
    uint32_t i;
    for (i = 0; i < 4u; i++) {                          /* what each column shows, first: a wide one narrows its neighbours' margins */
        const char *s = mo_col[i].val;
        uint32_t n;
        wide[i] = kind[i] = cut[i] = 0;
        v[i][0] = 0;
        if (!mo_col[i].used)
            continue;
        if (s[0] == '[') {                              /* "[OCT+]": the key that acts, a filled box */
            str_cpy(v[i], s + 1, sizeof v[i]);
            n = str_len(v[i]);
            if (n && v[i][n - 1u] == ']') v[i][n - 1u] = 0;
            cut[i] = (uint8_t)mo_fit(v[i], v[i]);
            kind[i] = MV_KEY;
        } else {
            char b[24];
            str_cpy(b, s, sizeof b);
            if (mo_col[i].ratio >= 0 && str_len(b) + str_len(mo_col[i].unit) <= 5u)
                str_cpy(b + str_len(b), mo_col[i].unit, sizeof b - str_len(b));
            cut[i] = (uint8_t)mo_fit(v[i], b);
            kind[i] = mo_col[i].ratio < 0 ? MV_BOX : MV_DIAL;
        }
        if (str_len(v[i]) > 4u && kind[i] != MV_KEY)   /* five: no box, inverted at full length */
            wide[i] = 1, kind[i] = MV_INV;
    }
    for (i = 0; i < 4u; i++) {
        int32_t cx = 15 + (int32_t)i * 30, tw = mo_tw(v[i]), tx = cx - tw / 2, ty, fx0 = -1, fx1 = -1;
        uint16_t ink = mo_col[i].dim ? MO_DIM : MO_INK;
        char l[8];
        mo_vals[i].used = mo_col[i].used;
        if (!mo_col[i].used)
            continue;
        mo_abbr(l, mo_col[i].label);
        mo_textc(l, cx, 12, mo_col[i].hot ? MO_INK : MO_DIM);
        ty = mo_col[i].ratio >= 0 && mo_col[i].val[0] != '[' ? 39 : 25;
        if (mo_col[i].ratio >= 0 && mo_col[i].val[0] != '[')
            mo_dial(cx, 28, 7, mo_col[i].ratio, ink);
        if (mo_flash_t && i == mo_flash_col && (kind[i] == MV_BOX || kind[i] == MV_KEY)) {   /* it acted: inverted */
            fx0 = cx - MO_BOX_W / 2, fx1 = cx + MO_BOX_W / 2;
            mo_fill(fx0, 21, MO_BOX_W, 15, MO_INK);
            mo_text(v[i], tx, ty, MO_BG);
        } else if (kind[i] == MV_KEY) {
            fx0 = cx - MO_BOX_W / 2, fx1 = cx + MO_BOX_W / 2;
            mo_fill(fx0, 21, MO_BOX_W, 15, ink);
            mo_text(v[i], tx, ty, MO_BG);
        } else if (kind[i] == MV_BOX) {
            fx0 = cx - MO_BOX_W / 2, fx1 = cx + MO_BOX_W / 2;
            mo_box(fx0, 21, MO_BOX_W, 15, ink);
            mo_text(v[i], tx, ty, ink);
        } else if (kind[i] == MV_INV) {                 /* a cell of margin each side, but none toward another wide one
                                                         * (they keep a cell between them) or past the screen's edge */
            fx0 = tx - (i > 0u && wide[i - 1u] ? 0 : 1);
            fx1 = tx + tw - 1 + (i < 3u && wide[i + 1u] ? 0 : 1);
            fx0 = fx0 < 0 ? 0 : fx0;
            fx1 = fx1 > 119 ? 119 : fx1;
            mo_fill(fx0, ty - 1, fx1 - fx0 + 1, 9, ink);
            mo_text(v[i], tx, ty, MO_BG);
        } else {
            mo_text(v[i], tx, ty, ink);
        }
        mo_vals[i].fx0 = (int16_t)fx0, mo_vals[i].fx1 = (int16_t)fx1;
        mo_vals[i].tx0 = (int16_t)tx, mo_vals[i].tx1 = (int16_t)(tx + tw - 1);
        mo_vals[i].kind = kind[i], mo_vals[i].cut = cut[i];
        str_cpy(mo_vals[i].s, v[i], sizeof mo_vals[i].s);
        if (mo_col[i].hot)
            mo_hl(cx - 12, 48, 25, MO_INK);
    }
    mo_vals_new = 1;
    mo_dash(0, 50, 120, MO_DIM);
}
static void mo_foot(void)
{
    const page_t *pg = cur_page();
    uint32_t i, n = 0, cur = 0, first = 0;
    uint16_t idx[16];
    int32_t x = 1, w = 0;
    mo_hl(0, 109, 120, MO_INK);                         /* (row 110 clear: the selected tab's block below it) */
    for (i = 0; i < NPAGES && n < 16u; i++)             /* the family's pages, as the button turns through them */
        if (PAGES[i].fam == pg->fam && page_visible(i)) {
            if (&PAGES[i] == pg) cur = n;
            idx[n++] = (uint16_t)i;
        }
    for (i = 0; i < n; i++)                             /* how many fit from the first: start late enough to show cur */
        w += 6 * 4 + 5;
    while (first < cur && (int32_t)(cur - first + 1u) * 29 > 118)
        first++;
    for (i = first; i < n; i++) {
        char t[8], p[8] = "";
        const char *s = mo_title(&PAGES[idx[i]]);
        uint32_t k = 0, j;
        for (; *s && k < 4u; s++)                       /* four letters, no spaces: LFO 2 -> LFO2 */
            if (*s != ' ') t[k++] = *s;
        t[k] = 0;
        for (j = 0; j < i && !p[0]; j++) {              /* the same four as a page before: its last letter instead */
            const char *q = mo_title(&PAGES[idx[j]]);
            uint32_t m = 0;
            for (; *q && m < 4u; q++)
                if (*q != ' ') p[m++] = *q;
            p[m] = 0;
            if (!str_eq(p, t)) p[0] = 0;
        }
        if (p[0] && k == 4u) {
            const char *e = mo_title(&PAGES[idx[i]]);
            t[3] = e[str_len(e) - 1u];
        }
        if (x + mo_tw(t) > 119)
            break;
        if (i == cur) mo_inv(t, x + 1, 112);
        else mo_text(t, x + 1, 112, MO_DIM);
        x += mo_tw(t) + 5;
    }
    (void)w;
}

/* ---- the pictures ---- */
static void mo_pic_pattern(const track_t *t)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN], rows, rh, sh;
    char b[16];
    if (len > NSTEP) len = NSTEP;
    rows = (len + 15u) / 16u;
    rh = rows <= 2u ? 22u : 12u;                        /* a row's height, its steps' */
    sh = rows <= 2u ? 9u : 7u;
    for (i = 0; i < len; i++) {
        const step_t *st = &seq_steps(t)[i];
        int32_t x = 14 + (int32_t)(i % 16u) * 33 / 5, y = 54 + (int32_t)(i / 16u * rh);
        if (i % 16u == 0) {
            fmt_int(b, (int32_t)i + 1);
            mo_text(b, 1, y + (int32_t)sh - 7, MO_DIM);
        }
        if (step_on(st)) {
            mo_fill(x, y, 5, (int32_t)sh, MO_INK);
            if (st->flags & SF_ACCENT) mo_fill(x + 1, y + 1, 3, 2, MO_BG);   /* an accent: a notch */
        } else if (st->time == ST_TIE) {
            mo_box(x, y + (int32_t)sh - 4, 5, 4, MO_INK);
        } else {
            mo_fill(x, y + (int32_t)sh - 2, 5, 2, MO_DIM);
        }
        if (i == ui.cursor)
            mo_box(x - 1, y - 1, 7, (int32_t)sh + 2, MO_INK);
        if (song.playing && i == t->seq_idx)
            mo_hl(x, y + (int32_t)sh + 1, 5, MO_INK);
    }
    str_cpy(b, "LEN ", sizeof b);
    fmt_int(b + 4, (int32_t)len);
    mo_text(b, 1, 101, MO_DIM);
    if (song.playing) {
        str_cpy(b, "STEP ", sizeof b);
        fmt_int(b + 5, (int32_t)t->seq_idx % (int32_t)(len ? len : 1u) + 1);
        mo_textr(b, 118, 101, MO_INK);
    }
}
static void mo_kv(const char *k, uint32_t v, int32_t y, int inv)   /* the GEN pictures' side column */
{
    char b[8];
    fmt_int(b, (int32_t)v);
    if (v < 10u) { b[1] = b[0]; b[0] = '0'; b[2] = 0; }
    mo_text(k, 78, y, MO_DIM);
    if (inv) mo_inv(b, 100, y);
    else mo_text(b, 100, y, MO_INK);
}
static void mo_pic_euclid(const gen_euclid_t *g)
{
    int32_t cx = 42, cy = 80, r = 25;
    uint32_t n = g->steps ? g->steps : 1u, i, head = (g->step_idx + n - 1u) % n;
    char b[8];
    for (i = 0; i < n; i++) {
        int32_t a = -64 + (int32_t)(i * 256u / n), x = cx + mo_cos(a) * r / 127, y = cy + mo_sin(a) * r / 127;
        if (gen_euclid_hit((uint8_t)i, g->steps, g->pulses, g->rot)) mo_fill(x - 2, y - 2, 5, 5, MO_INK);
        else mo_box(x - 2, y - 2, 5, 5, MO_DIM);
        if (song.playing && i == head) mo_circ(x, y, 4, MO_INK);
    }
    mo_box(cx - 7, cy - 7, 15, 15, MO_INK);
    fmt_int(b, g->pulses);
    mo_textc(b, cx + 1, cy - 3, MO_INK);
    mo_kv("LEN", g->steps, 58, 1);
    mo_kv("HIT", g->pulses, 70, 1);
    mo_kv("ROT", g->rot, 82, 1);
    mo_kv("STP", head + 1u, 94, 0);
}
static void mo_pic_turing(const gen_turing_t *g)
{
    uint32_t len = g->len ? g->len : 16u, span = g->oct_span ? g->oct_span : 1u, i;
    for (i = 0; i < 16u; i++) {                         /* the register: its loop's bits, the rest dim */
        int32_t x = 2 + (int32_t)i * 73 / 10;
        uint32_t bit = i < len ? (g->reg >> (len - 1u - i)) & 1u : 0u;
        if (i >= len) mo_box(x, 55, 6, 6, MO_DIM);
        else if (bit) mo_fill(x, 55, 6, 6, MO_INK);
        else mo_box(x, 55, 6, 6, MO_INK);
    }
    mo_hl(2, 63, (int32_t)len * 73 / 10 - 1, MO_INK);    /* the loop's length */
    for (i = 0; i < len; i++) {                         /* each step's note, as a bar (as the detailed picture) */
        uint32_t raw = ((g->reg & (len >= 16u ? 0xFFFFu : ((1u << len) - 1u))) >> (len - 1u - i)) & 0xFFu;
        uint32_t v = ((raw / 7u) % span) * 7u + raw % 7u, mx = span * 7u - 1u;
        int32_t h = 4 + (int32_t)(mx ? v * 32u / mx : 0u), x = 2 + (int32_t)i * 73 / 10;
        if (song.playing && i == len - 1u) mo_fill(x, 104 - h, 6, h, MO_INK);
        else mo_box(x, 104 - h, 6, h, MO_INK);
    }
}
static void mo_pic_grids(const gen_grids_t *g, const track_t *t)
{
    static const char *const LN[3] = {"BD", "SD", "HH"};
    uint32_t steps = gen_grids_steps(tess_steps_per_bar(t)), cur = (g->step_idx + steps - 1u) % steps, p, i;
    int32_t px = 3 + g->x * 37 / 255, py = 55 + g->y * 37 / 255;
    mo_box(2, 54, 40, 40, MO_INK);                      /* the map, its 5 x 5 patterns, the point on it */
    for (i = 1; i < 5u; i++) {
        mo_dash(3, 54 + (int32_t)i * 8, 38, MO_DIM);
        for (int32_t y = 55; y < 93; y += 2) mo_px(2 + (int32_t)i * 8, y, MO_DIM);
    }
    mo_vl(px, 55, 38, MO_INK);
    mo_hl(3, py, 38, MO_INK);
    mo_disc(px, py, 2, MO_INK);
    for (p = 0; p < 3u; p++) {                          /* the three parts: 16 steps as two rows of eight */
        int32_t y = 56 + (int32_t)p * 17;
        mo_text(LN[p], 47, y + 2, MO_DIM);
        for (i = 0; i < 16u; i++) {
            int acc = 0;
            uint32_t s = steps > 16u ? i * 2u : i;
            int32_t x = 61 + (int32_t)(i % 8u) * 7, yy = y + (i < 8u ? 0 : 7);
            if (gen_grids_hit(g, s, steps, p, 0u, &acc)) mo_fill(x, yy, 5, 5, MO_INK);
            else mo_px(x + 2, yy + 2, MO_DIM);
            if (song.playing && (steps > 16u ? cur / 2u : cur) == i) mo_box(x - 1, yy - 1, 7, 7, MO_INK);
        }
    }
}
/* Markov (as the detailed picture, ui_graph.c): the eight degrees, each bar the chance it comes next from the one
 * playing (gen_markov_row, so TENSION reshapes them); the one playing solid on a solid underline, a rest hollow on a
 * dashed one; the others outlined */
static void mo_pic_markov(const gen_markov_t *g)
{
    uint8_t row[8];
    uint32_t d, cur = g->cur_state & 7u, prev = 0;
    char b[16];
    gen_markov_row(g->tension, cur, row);
    for (d = 0; d < 8u; d++) {
        uint32_t pr = row[d] > prev ? row[d] - prev : 0u;   /* (the row is cumulative) */
        int32_t h, x = 5 + (int32_t)d * 14;
        prev = row[d];
        pr = pr > 40u ? 40u : pr;
        h = 3 + (int32_t)(pr * 37u / 40u);
        if (d == cur && g->last_hit) {
            mo_fill(x, 95 - h, 10, h, MO_INK);
            mo_fill(x, 97, 10, 2, MO_INK);
        } else if (d == cur) {
            mo_box(x, 95 - h, 10, h, MO_INK);
            mo_dash(x, 98, 10, MO_INK);
        } else {
            mo_box(x, 95 - h, 10, h, MO_INK);
            mo_hl(x, 97, 10, MO_DIM);
        }
    }
    str_cpy(b, "DEGREE ", sizeof b);
    fmt_int(b + 7, cur + 1u);
    mo_text(b, 1, 101, MO_DIM);
    str_cpy(b, "LEAP ", sizeof b);                      /* (KNOB 2: the tension) */
    fmt_int(b + 5, g->tension);
    mo_textr(b, 118, 101, MO_DIM);
}
static void mo_pic_morph(const gen_morph_t *g)
{
    uint32_t i, cur = g->step_idx & 15u;
    for (i = 0; i < 16u; i++) {
        int32_t x = 14 + (int32_t)i * 33 / 5;
        if ((g->snap_a >> i) & 1u) mo_fill(x, 55, 5, 9, MO_INK); else mo_fill(x, 62, 5, 2, MO_DIM);
        if ((g->snap_b >> i) & 1u) mo_fill(x, 73, 5, 9, MO_INK); else mo_fill(x, 80, 5, 2, MO_DIM);
        if (song.playing && i == cur) mo_hl(x, 84, 5, MO_INK);
    }
    mo_text("A", 1, 56, MO_DIM);
    mo_text("B", 1, 74, MO_DIM);
    mo_box(14, 92, 104, 6, MO_INK);                     /* where it sits between A and B */
    mo_fill(15, 93, (int32_t)g->morph_pos * 102 / 127, 4, MO_INK);
}
static void mo_pic_lfo(track_t *t)
{
    int32_t x, py = 0, uni = t->p[P_LPOL] != 0, z = uni ? 98 : 80, amp = uni ? 40 : 18, now;
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25, step = 0xFFFFFFFFu / 58u;
    char b[16];
    const char *unit;
    mo_dash(2, z, 116, MO_DIM);
    for (x = 0; x < 116; x++) {                         /* two cycles from PHASE */
        int32_t w = lfo_wave(t, ph + (uint32_t)x * step), y;
        if (t->p[P_LWAVE] == 4)
            w = (int32_t)((x / 10 * 2654435761u) >> 16) - 32768;
        y = uni ? z - (w + 32768) * amp / 65536 : z - w * amp / 32768;
        if (x) mo_line(1 + x, py, 2 + x, y, MO_INK);
        py = y;
    }
    now = (int32_t)((t->lfo_ph - ph) / step);           /* where it is: on the first cycle */
    if (now >= 0 && now < 116) {
        int32_t w = t->lfo_val;
        mo_vl(2 + now, MO_PY0 + 2, 50, MO_DIM);
        mo_disc(2 + now, uni ? z - (w + 32768) * amp / 65536 : z - w * amp / 32768, 2, MO_INK);
    }
    param_format(&TP[P_LSYNC], t->p[P_LSYNC], b + 5, &unit);
    if (t->p[P_LSYNC]) { b[0] = 'S'; b[1] = 'Y'; b[2] = 'N'; b[3] = 'C'; b[4] = ' '; mo_text(b, 2, 102, MO_DIM); }
    else mo_text("FREE", 2, 102, MO_DIM);
    {
        int32_t v = t->lfo_val * 100 / 32767, a = v < 0 ? -v : v;
        b[0] = v < 0 ? '-' : '+';
        b[1] = (char)('0' + a / 100);
        b[2] = '.';
        b[3] = (char)('0' + a / 10 % 10);
        b[4] = (char)('0' + a % 10);
        b[5] = 0;
        mo_textr(b, 118, 102, MO_INK);
    }
}
/* NOTES (a melodic track's steps): the cursor's page of 16, tabs 1..4 for the pattern's pages (the one shown inverted)
 * with LEN and DIV; a step a column 5 cells wide in groups of four: a note's height its pitch among the page's notes
 * (a chord: a tick above for each note more), a tie an outline at the note before's height, a rest a dim dash; an
 * accent a mark under, a slide a tail into the next; the playhead a dim frame, the cursor a thick underline */
static void mo_pic_notes(const track_t *t)
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), pages = (len + 15u) / 16u, pg = ui.cursor / 16u, i;
    int32_t lo = 127, hi = 0, prev = 3, base = 92;
    char b[20];
    const char *unit;
    for (i = 0; i < 4u; i++) {                          /* the pattern's pages */
        char d[2] = {(char)('1' + i), 0};
        if (i == pg) mo_inv(d, 3 + (int32_t)i * 9, 54);
        else mo_text(d, 3 + (int32_t)i * 9, 54, i < pages ? MO_DIM : MO_BG);
        if (i >= pages) mo_px(5 + (int32_t)i * 9, 60, MO_DIM);   /* (past LEN: a dot) */
    }
    str_cpy(b, "LEN ", sizeof b);
    fmt_int(b + 4, (int32_t)len);
    mo_text(b, 46, 54, MO_DIM);
    param_format(&TP[P_SDIV], t->p[P_SDIV], b, &unit);
    mo_textr(b, 118, 54, MO_DIM);
    for (i = pg * 16u; i < pg * 16u + 16u && i < len; i++) {   /* the page's pitch range */
        const step_t *st = &seq_steps(t)[i];
        if (step_on(st) && st->n) {
            lo = st->note[0] < lo ? st->note[0] : lo;
            hi = st->note[0] > hi ? st->note[0] : hi;
        }
    }
    if (hi - lo < 12) lo = hi - 12 < 0 ? 0 : (lo + hi) / 2 - 6, hi = lo + 12;   /* (an octave at least) */
    for (i = 0; i < 16u; i++) {
        uint32_t si = pg * 16u + i;
        const step_t *st = &seq_steps(t)[si < NSTEP ? si : 0u];
        int32_t x = 3 + (int32_t)i * 7 + (int32_t)(i / 4u) * 2, h;
        if (si >= len) {                                /* past LEN: nothing but a dot */
            mo_px(x + 2, base - 1, MO_DIM);
            continue;
        }
        if (step_on(st) && st->n) {
            uint32_t k;
            h = 3 + (st->note[0] - lo) * 19 / (hi - lo > 0 ? hi - lo : 1);
            h = h > 22 ? 22 : h;
            prev = h;
            mo_fill(x, base - h, 5, h, MO_INK);
            for (k = 1; k < st->n && k < 4u; k++)       /* a chord's other notes */
                mo_hl(x + 1, base - h - 2 * (int32_t)k, 3, MO_INK);
            if (st->flags & SF_ACCENT) mo_fill(x + 1, base + 2, 3, 2, MO_INK);
            if (st->flags & SF_SLIDE) mo_hl(x + 5, base - h, 2, MO_INK);
        } else if (st->time == ST_TIE) {
            mo_box(x - 1, base - prev, 6, prev, MO_INK);
        } else {
            mo_hl(x, base - 1, 5, MO_DIM);
        }
        if (song.playing && si == t->seq_idx % len)
            mo_box(x - 1, base - 26, 7, 28, MO_DIM);
        if (si == ui.cursor)
            mo_fill(x, base + 5, 5, 2, MO_INK);
    }
    {                                                   /* the cursor's step in words */
        const step_t *st = &seq_steps(t)[ui.cursor % NSTEP];
        static const char *const TM[3] = {"NOTE", "TIE", "REST"};
        str_cpy(b, "STEP ", sizeof b);
        fmt_int(b + 5, (int32_t)ui.cursor + 1);
        str_cpy(b + str_len(b), ": ", 3);
        if (step_on(st) && st->n) note_name(b + str_len(b), st->note[0]);
        else str_cpy(b + str_len(b), TM[st->time % 3u], 6);
        if (st->flags & SF_ACCENT) str_cpy(b + str_len(b), " ACC", 5);
        if (st->flags & SF_SLIDE) str_cpy(b + str_len(b), " SLD", 5);
        mo_text(b, 1, 101, MO_DIM);
    }
}
/* a picture with no Mono drawing of its own yet: the detailed look's (ui_graph.c graph_body, ui_pages.c pv_pic_body)
 * in the picture area, in the ink's palette (gen_ui_palettes.py: every colour the ink or a blend of it on black); what
 * runs past the area cleared again for the footer */
/* MATRIX: the tracks along the top (TO) and down the left (FROM), a cell between each pair: no mod a dim outline,
 * mods filled from the left as far as their amounts reach (MOD: how many); the picked cell framed twice */
static void mo_pic_matrix(void)
{
    uint32_t r, k;
    mo_text("FR>TO", 1, 53, MO_DIM);
    for (k = 0; k < NTRK; k++) {
        char n[2] = {(char)('1' + k), 0};
        mo_textc(n, 43 + (int32_t)k * 22, 53, MO_INK);
        mo_textc(n, 25, 64 + (int32_t)k * 11, MO_INK);
    }
    for (r = 0; r < NTRK; r++)
        for (k = 0; k < NTRK; k++) {
            int32_t x = 33 + (int32_t)k * 22, y = 62 + (int32_t)r * 11;
            uint32_t lv = mx_level(r, k);
            if (r == mx_from && k == mx_to)
                mo_box(x - 1, y - 1, 22, 11, MO_INK);
            mo_box(x, y, 20, 9, lv ? MO_INK : MO_DIM);
            if (lv)
                mo_fill(x + 1, y + 1, 1 + (int32_t)lv * 17 / 8, 7, MO_INK);
        }
}
/* the drum grid's lanes from row y0, dy cells apart (lanes first .. first + n - 1): the lane's name at the left (the
 * lane picked inverted), the bank's 16 steps: a hit a 5 x 5 square (accented) or 3 x 3, none a dot (on the beats ink);
 * the cursor framed; the playhead a bar over the rows at y0 - 2 */
static void mo_lanes_draw(const track_t *t, int32_t y0, int32_t dy, uint32_t first, uint32_t n)
{
    uint32_t l, i, len = (uint32_t)t->p[P_SLEN], base = ui.bank * 16u;
    for (l = first; l < first + n && l < NLANE; l++) {
        int32_t y = y0 + (int32_t)(l - first) * dy;
        int sel = l == ui.lane;
        char a[8], b[4];
        mo_ascii(a, drum_lane_abbr(t, l), sizeof a);
        mo_cut(b, a, 2);
        if (sel) mo_inv(b, 2, y);
        else mo_text(b, 2, y, MO_DIM);
        for (i = 0; i < 16u && base + i < len; i++) {
            const step_t *st = &seq_steps(t)[base + i];
            uint32_t bit = 1u << l;
            int32_t x = 18 + (int32_t)i * 6 + (int32_t)(i / 4u);
            if (step_lanes(st) & bit) {
                if (step_accents(st) & bit) mo_fill(x, y + 1, 5, 5, MO_INK);
                else mo_fill(x + 1, y + 2, 3, 3, MO_INK);
            } else {
                mo_px(x + 2, y + 3, i % 4u == 0u || sel ? MO_INK : MO_DIM);
            }
            if (sel && base + i == ui.cursor)
                mo_box(x - 1, y, 7, 7, MO_INK);
        }
    }
    if (song.playing && t->seq_idx < len && t->seq_idx / 16u == ui.bank) {
        i = t->seq_idx % 16u;
        mo_hl(18 + (int32_t)i * 6 + (int32_t)(i / 4u), y0 - 2, 5, MO_INK);
    }
}
/* DRUM HIT's picture (under the dials): six lanes round the one picked, 9 cells apart */
static void mo_pic_grid(const track_t *t)
{
    uint32_t first = ui.lane > 2u ? ui.lane - 2u : 0u;
    if (first > NLANE - 6u) first = NLANE - 6u;
    mo_lanes_draw(t, 55, 9, first, 6);
}
/* the drum grid's own page (pv_draw PV_GRID): all eight lanes, 10 cells apart, rows 11 .. 97 */
static void mo_grid_panel(void)
{
    int32_t top;
    for (top = 0; top < PV_GRID_H; top += 120) {
        int32_t sh = PV_GRID_H - top < 120 ? PV_GRID_H - top : 120;
        cv_begin(240, (uint32_t)sh, MO_BG);
        cv_oy = -top;
        mo_lanes_draw(TSEL, 4, 10, 0, NLANE);
        cv_oy = 0;
        cv_blit(0, (uint32_t)(PV_GRID_Y + top));
    }
}
static void mo_pic_detailed(void)
{
    int32_t oy0 = cv_oy, oy = cv_oy + MO_PY0 * 2;
    uint16_t bg = cv_bg;
    cv_bg = MO_BG;
    if (pv_kind() == PV_CURVE || pv_kind() == PV_RINGS) {
        int32_t cut, res;
        uint32_t pic = pv_pic(&cut, &res);
        cv_oy = oy;
        if (pic == 4u || pic == 8u) graph_body(oy);     /* (FM6's chart, CHORD's chord: as pv_picture) */
        else pv_pic_body(pic, cut, res, 8, (MO_PY1 - MO_PY0) * 2 - 16);
    } else {
        graph_body(oy);
    }
    cv_oy = oy0;
    cv_bg = bg;
    mo_fill(0, MO_PY1, 120, 120 - MO_PY1, MO_BG);
}
/* BOUNCE: the box, its walls hit on the last step drawn twice as thick; the balls, discs. FALL (BALLS past 4): the floor
 * (thick on a landing), the leaves tilted as they sway, a landed one flat on the floor (dim as it rests, a bass leaf
 * hollow), a gust's streaks dashed across the box */
static void mo_pic_bounce(const gen_bounce_t *b)
{
    static const uint8_t LEAF[3][2] = {{1, 2}, {0, 4}, {1, 2}};   /* (a row's first cell and length) */
    uint32_t w = b->hit, n = b->balls >= 1u && b->balls <= GEN_BOUNCE_MAX ? b->balls : 1u, k;
    if (b->balls == GEN_BOUNCE_FALL) {
        mo_fill(2, w ? 105 : 106, 116, w ? 2 : 1, w ? MO_INK : MO_DIM);
        if (b->gust)
            for (k = 0; k < 4u; k++) {
                int32_t gx = 4 + (int32_t)b->lx[k * 2u] * 90 / 255 + (b->angle > 45u ? -4 : 4) * (int32_t)b->gust;
                mo_dash(gx < 2 ? 2 : gx, 56 + (int32_t)((b->ly[k * 2u] + 97u * k) & 255u) * 40 / 255, 12, MO_DIM);
            }
        for (k = 0; k < GEN_FALL_LEAVES; k++) {
            int32_t lx = 3 + (int32_t)b->lx[k] * 110 / 255, ly = 101 - (int32_t)b->ly[k] * 46 / 255, r;
            int tilt = (((int32_t)b->ly[k] + (int32_t)k * 40) & 63) < 32;
            if (b->lt[k]) {                              /* landed */
                if (b->lv[k] & 0x80u) mo_box(lx, 103, 4, 2, b->lt[k] > 3u ? MO_INK : MO_DIM);
                else mo_fill(lx, 103, 4, 2, b->lt[k] > 3u ? MO_INK : MO_DIM);
                continue;
            }
            for (r = 0; r < 3; r++)
                mo_hl(tilt ? lx + LEAF[r][0] + r - 1 : lx + 1 - r + LEAF[r][0], ly + r, LEAF[r][1], MO_INK);
        }
        return;
    }
    mo_fill(2, 53, 116, w & 8u ? 2 : 1, w & 8u ? MO_INK : MO_DIM);                      /* top */
    mo_fill(2, w & 1u ? 105 : 106, 116, w & 1u ? 2 : 1, w & 1u ? MO_INK : MO_DIM);      /* floor */
    mo_fill(2, 53, w & 2u ? 2 : 1, 54, w & 2u ? MO_INK : MO_DIM);                       /* left */
    mo_fill(w & 4u ? 116 : 117, 53, w & 4u ? 2 : 1, 54, w & 4u ? MO_INK : MO_DIM);      /* right */
    for (k = n; k-- > 0;)
        mo_disc(7 + (int32_t)b->x[k] * 106 / 255, 101 - (int32_t)b->y[k] * 43 / 255, 2, MO_INK);
}
/* AWAKE: loop A (the notes) as bars 1..8 high over loop B (their lift) as its figure, 16 steps each; a rest a dash,
 * past a loop's length a dim line; the steps playing solid, the others outlined; the step the keys picked framed */
static void mo_pic_awake(const gen_awake_t *a)
{
    uint32_t la = a->len_a ? a->len_a : 1u, lb = a->len_b ? a->len_b : 1u, i;
    uint32_t ca = (a->pos_a + la - 1u) % la, cb = (a->pos_b + lb - 1u) % lb;
    char b[16];
    for (i = 0; i < 16u; i++) {
        int32_t x = 2 + (int32_t)i * 7 + (int32_t)(i / 4u);
        uint32_t va = gen_awake_a(a, i), vb = gen_awake_b(a, i);
        int on_a = song.playing && i == ca, on_b = song.playing && i == cb;
        if (i >= la) mo_hl(x, 80, 6, MO_DIM);
        else if (!va) mo_fill(x, 79, 6, 2, on_a ? MO_INK : MO_DIM);
        else {
            int32_t h = 3 + ((int32_t)va - 1) * 21 / 7;
            if (on_a) mo_fill(x, 81 - h, 6, h, MO_INK);
            else mo_box(x, 81 - h, 6, h, MO_INK);
        }
        if (i >= lb) mo_hl(x, 92, 6, MO_DIM);
        else {
            char d[2] = {(char)('0' + (vb > 7u ? 7u : vb)), 0};
            if (on_b) mo_inv(d, x + 1, 85);
            else mo_text(d, x + 1, 85, vb ? MO_INK : MO_DIM);
        }
        if (a->edit && (a->edit & 15u) == i)
            mo_box(x - 1, 54, 8, 40, MO_INK);
    }
    str_cpy(b, "A ", sizeof b);
    fmt_int(b + 2, la);
    str_cpy(b + str_len(b), "  +B ", 6);
    fmt_int(b + str_len(b), lb);
    mo_text(b, 1, 101, MO_DIM);
}
/* POLY: the four rhythms a row each: its divider (the one RHY picks inverted; OFF dim), the bar's 16 steps, a tick where
 * it lands (solid when it fires at the step playing); the step playing underlined; under them the four notes, a bar
 * each at its pitch, the one playing solid */
static void mo_pic_poly(const gen_poly_t *p)
{
    uint32_t n = 7u * (p->span >= 1u && p->span <= 3u ? p->span : 1u), now = (p->tick + GEN_POLY_WRAP - 1u) % GEN_POLY_WRAP;
    uint32_t base = now - now % 16u, r, i;
    for (r = 0; r < 4u; r++) {
        uint32_t d = p->div[r];
        int32_t y = 54 + (int32_t)r * 10;
        char lb[4] = "OFF";
        if (d) lb[0] = '/', fmt_int(lb + 1, d);
        if (r == (p->sel & 3u)) mo_inv(lb, 2, y);
        else mo_text(lb, 2, y, d ? MO_INK : MO_DIM);
        for (i = 0; i < 16u; i++) {
            int32_t x = 22 + (int32_t)i * 6 + (int32_t)(i / 4u);
            uint32_t st = base + i, cur = song.playing && st == now;
            if (d && st % d == 0u) {
                if (cur) mo_fill(x, y, 5, 7, MO_INK);
                else mo_box(x, y, 5, 7, MO_INK);
            } else {
                mo_px(x + 2, y + 6, MO_DIM);
            }
            if (cur && r == 0u) mo_hl(x, 94, 5, MO_INK);
        }
    }
    for (i = 0; i < 4u; i++) {
        int32_t x = 22 + (int32_t)i * 25, h = 2 + (int32_t)(p->pitch[i] > n - 1u ? n - 1u : p->pitch[i]) * 9 / (int32_t)(n - 1u);
        if (i == p->pos) mo_fill(x, 107 - h, 22, h, MO_INK);
        else mo_box(x, 107 - h, 22, h, MO_INK);
    }
}
/* QUAD: the grid the tracks share, a row a track: T1 and its DIR, its four cells (a bar its pitch, a rest a dash; the
 * cell it plays solid), its RANGE and LINKs (a dot a track, solid when linked); a row not in QUAD dim. Over each column
 * its MOVE (a triangle the way it goes, RAND two dots, STOP a dash), under it its EVERY; the column OCT+ edits marked */
static void mo_pic_quad(uint32_t tr)
{
    static const char *const QD[GEN_QD_COUNT] = {">", "<", "<>", "?", "="};
    static const char *const QR[GEN_QR_COUNT] = {"3", "1", "2", "+-1"};
    int layer = quad_cols_on();
    uint32_t r, i, k;
    for (i = 0; i < 4u; i++) {
        int32_t x = 30 + (int32_t)i * 18, cx = x + 7;
        uint32_t mv = tessera.qcol[i] & 3u;
        int sel = layer && i == (tessera.qcol_sel & 3u);
        uint16_t c = sel || mv ? MO_INK : MO_DIM;
        char e[4] = {'/', 0, 0, 0};
        if (mv == GEN_QC_DOWN || mv == GEN_QC_UP)
            for (k = 0; k < 3u; k++)
                mo_hl(cx - 2 + (int32_t)k, mv == GEN_QC_DOWN ? 53 + (int32_t)k : 55 - (int32_t)k, 5 - 2 * (int32_t)k, c);
        else if (mv == GEN_QC_RAND) {
            mo_px(cx - 2, 53, c);
            mo_px(cx + 2, 55, c);
        } else
            mo_hl(cx - 2, 54, 5, c);
        fmt_int(e + 1, tessera_quad_every(i));
        mo_textc(e, cx + 1, 101, c);
        if (sel) mo_hl(x, 57, 16, MO_INK);
    }
    for (r = 0; r < TESSERA_TRACKS; r++) {
        const gen_quad_t *q = tessera_quad(r);
        int on = tessera.mode[r] == TESS_ENG_QUAD;
        int32_t y = 59 + (int32_t)r * 10;
        uint16_t c = on ? MO_INK : MO_DIM;
        char lb[3] = {'T', (char)('1' + r), 0};
        if (r == tr) mo_inv(lb, 2, y);
        else mo_text(lb, 2, y, c);
        mo_text(QD[q->dir % GEN_QD_COUNT], 16, y, c);
        mo_text(QR[q->range % GEN_QR_COUNT], 102, y, c);
        for (k = 0; k < TESSERA_TRACKS; k++)            /* (its LINKs, under the RANGE) */
            if (k != r) mo_px(102 + (int32_t)k * 4, y + 8, on && ((q->link >> k) & 1u) ? MO_INK : MO_DIM);
        for (i = 0; i < 4u; i++) {
            int32_t x = 30 + (int32_t)i * 18, v = q->cell[i] > 21u ? 21 : q->cell[i], h;
            int cur = on && i == q->pos;
            mo_hl(x, y + 8, 16, MO_DIM);
            if (!v) {
                mo_hl(x + 5, y + 6, 6, cur ? MO_INK : c);
                continue;
            }
            h = 2 + (v - 1) * 5 / 20;
            if (cur) mo_fill(x, y + 7 - h, 16, h, MO_INK);
            else mo_box(x, y + 7 - h, 16, h, c);
        }
    }
}
/* CONSTELLATION: the sky scrolling left, the far two layers dim dots; the front layer's stars a plus their size (bright
 * ones and the ones heard ink, the others ahead ink and behind dim); behind the ship the stars it heard joined; the ship
 * mid-sky, its thruster flickering, the brackets SIZE rows round it (dim when the loop plays), the star sounding ringed.
 * LOCK up: the loop under the sky, its stars joined, the one played ringed, a mark where LEN ends it */
static void mo_pic_constel(uint32_t tr)
{
    const gen_constel_t *g = tessera_constel(tr);
    int lock = tessera.evolve[tr].lock != 0;
    int32_t top = 54, sh = lock ? 32 : 52, cw = 7, sx = 4 + 7 * cw + 3, row = g->ship >> 8;
    int32_t wd = g->size > 7u ? 7 : (int32_t)g->size, i, px = 0, py = 0, pv = 0;
#define MCS_Y(r) (top + sh - 2 - (int32_t)(r) * (sh - 4) / (int32_t)(GEN_CS_ROWS - 1u))
    for (uint32_t l = 2; l > 0; l--)
        for (i = -7; i < 9; i++) {
            uint32_t st = gen_cs_star(g->sky, ((uint32_t)g->col >> l) + (uint32_t)i, l, 30u);
            if (st) mo_px(sx + i * cw + (int32_t)l, MCS_Y((st & 15u) - 1u), MO_DIM);
        }
    for (i = -7; i <= 0; i++) {                         /* (the constellation behind the ship: its lines first) */
        uint32_t st = gen_cs_star(g->sky, (uint32_t)g->col + (uint32_t)i, 0, g->dens);
        int32_t y;
        if (!st || !((g->trail >> (uint32_t)-i) & 1u)) continue;
        y = MCS_Y((st & 15u) - 1u);
        if (pv) mo_line(px, py, sx + i * cw, y, MO_INK);
        px = sx + i * cw, py = y, pv = 1;
    }
    for (i = -7; i < 9; i++) {
        uint32_t st = gen_cs_star(g->sky, (uint32_t)g->col + (uint32_t)i, 0, g->dens);
        int32_t x = sx + i * cw, y, a;
        int heard = i <= 0 && ((g->trail >> (uint32_t)-i) & 1u);
        uint16_t c;
        if (!st) continue;
        y = MCS_Y((st & 15u) - 1u);
        a = 1 + (int32_t)((st >> 6) & 3u) / 2;
        c = heard || (st >> 4) & 2u || i >= 0 ? MO_INK : MO_DIM;
        mo_hl(x - a, y, 2 * a + 1, c);
        mo_vl(x, y - a, 2 * a + 1, c);
        if (!i && g->caught) mo_circ(x, y, 4, MO_INK);
    }
    {
        int32_t y = MCS_Y(row), yt = MCS_Y(row + wd > 14 ? 14 : row + wd) - 2, yb = MCS_Y(row - wd < 0 ? 0 : row - wd) + 2;
        uint16_t bc = g->held ? MO_DIM : MO_INK;
        mo_hl(sx - 2, yt, 5, bc); mo_px(sx - 2, yt + 1, bc); mo_px(sx + 2, yt + 1, bc);
        mo_hl(sx - 2, yb, 5, bc); mo_px(sx - 2, yb - 1, bc); mo_px(sx + 2, yb - 1, bc);
        mo_fill(sx - 11, y, 5, 1, MO_INK);              /* the ship: body, nose, fins, thruster */
        mo_px(sx - 6, y, MO_INK);
        mo_px(sx - 10, y - 1, MO_INK); mo_px(sx - 10, y + 1, MO_INK);
        mo_hl(sx - 13 - (int32_t)(g->col & 1u), y, 1 + (int32_t)(g->col & 1u), MO_DIM);
        if (g->filled && song.playing) {                /* (FILL ARP: a hollow diamond at the note it played) */
            int32_t fy = MCS_Y(g->filled - 1u);
            mo_px(sx, fy - 2, MO_INK); mo_px(sx, fy + 2, MO_INK); mo_px(sx - 2, fy, MO_INK); mo_px(sx + 2, fy, MO_INK);
            mo_px(sx - 1, fy - 1, MO_INK); mo_px(sx + 1, fy - 1, MO_INK); mo_px(sx - 1, fy + 1, MO_INK); mo_px(sx + 1, fy + 1, MO_INK);
        }
    }
#undef MCS_Y
    if (lock) {
        int32_t ly = 92, lh = 12;
        uint32_t now = song.playing && g->held ? (uint32_t)g->held - 1u : 16u, n = g->len >= 1u && g->len <= 16u ? g->len : 16u, k;
        int32_t qx = -1, qy = 0;
        mo_dash(4, 89, 112, MO_DIM);
        if (n < 16u) mo_vl(4 + (int32_t)n * cw, ly - 1, lh + 3, MO_INK);
        for (k = 0; k < n; k++) {
            uint32_t v = g->loop[k];
            int32_t x = 4 + (int32_t)k * cw + cw / 2, y;
            if (!(v & 15u)) {
                if (k == now) mo_fill(x - 1, ly + lh, 3, 2, MO_INK);
                continue;
            }
            y = ly + lh - (int32_t)((v & 15u) - 1u) * lh / (int32_t)(GEN_CS_ROWS - 1u);
            if (qx >= 0) mo_line(qx, qy, x, y, MO_DIM);
            qx = x, qy = y;
            mo_fill(x - 1, y - 1, 3, 3, MO_INK);
            if (k == now) mo_circ(x, y, 3, MO_INK);
        }
    }
}
/* SERIES: the next 16 degrees as bars (the one playing solid), a dim line joining their tops behind them; the loop's
 * length bracketed under them, LOOP OFF dashed (the series running on) */
static void mo_pic_series(const gen_series_t *g)
{
    gen_series_t p = *g;                                /* (from the step just played) */
    int32_t px = -1, py = 0;
    uint32_t k;
    if (p.n) p.n--;
    for (int pass = 0; pass < 2; pass++)
        for (k = 0; k < 16u; k++) {
            uint32_t d = gen_sr_degree(&p, k);
            int32_t x = 4 + (int32_t)k * 7, h = 2 + (int32_t)d * 38 / 14, y = 97 - h;
            if (!pass) {
                if (px >= 0) mo_line(px, py, x + 2, y, MO_DIM);
                px = x + 2, py = y;
            } else if (song.playing && g->n && k == 0u) mo_fill(x, y, 5, h, MO_INK);
            else mo_box(x, y, 5, h, MO_INK);
        }
    if (g->loop) {
        int32_t w = (int32_t)g->loop * 7 - 2;
        mo_hl(4, 103, w, MO_INK);
        mo_vl(4, 100, 4, MO_INK);
        mo_vl(4 + w - 1, 100, 4, MO_INK);
    } else
        mo_dash(4, 103, 110, MO_DIM);
}
/* a chord's name in a card x .. x + w - 1 from row y: on one line when it fits, else the root over its quality, "maj"
 * as "M" and "m7b5" as o slashed when the card is narrow; the small letters kept (Am7) */
static void mo_chord_card(char *b, int32_t x, int32_t w, int32_t y, uint16_t c)
{
    uint32_t rl = b[1] == '#' || b[1] == 'b' ? 2u : 1u, m = (uint32_t)(w - 2) / 6u;
    char r[3] = {b[0], rl > 1u ? b[1] : 0, 0}, *q = b + rl;
    TEXT_CASE(1);
    if (str_len(b) <= m) {
        mo_textc(b, x + w / 2, y + 4, c);
    } else {
        if (str_len(q) > m && q[0] == 'm' && q[1] == 'a' && q[2] == 'j') q[2] = 'M', q += 2;
        if (str_len(q) > m && q[0] == 'm' && q[1] == '7' && q[2] == 'b' && q[3] == '5') q[0] = (char)0xF8, q[2] = 0;
        if (str_len(q) > m) q[m] = 0;
        mo_textc(r, x + w / 2, y, c);
        mo_textc(q, x + w / 2, y + 8, c);
    }
    TEXT_CASE(0);
}
/* CHORD: the progression as cards (the one playing solid, a bar under it for how far through it is), then an octave
 * of keys from C, the playing chord's notes solid, its root with a notch */
static void mo_pic_chord(uint32_t tr)
{
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    const gen_chord_t *g = tessera_chord(tr);
    uint32_t n = gen_chord_len(g), cur = g->n ? g->idx : 0u, pcs = 0, len = gen_chord_steps(g, tess_steps_per_bar(&trk[tr]));
    uint32_t i, k;
    int32_t cw = 116 / (int32_t)(n ? n : 1u);
    char b[12];
    for (i = 0; i < n; i++) {
        int32_t x = 2 + (int32_t)i * cw;
        uint32_t at = tessera_chord_at(tr, i, b), on = i == cur;
        for (char *q = b; *q; q++)                      /* ("dim" as its symbol: B degree, C degree 7) */
            if (q[0] == 'd' && q[1] == 'i' && q[2] == 'm') {
                q[0] = (char)0xB0;
                for (char *s2 = q + 1; (s2[0] = s2[2]) != 0; s2++) ;
                break;
            }
        if (on) pcs = at, mo_fill(x, 53, cw - 1, 18, MO_INK);
        else mo_box(x, 53, cw - 1, 18, MO_DIM);
        mo_chord_card(b, x, cw - 1, 55, on ? MO_BG : MO_INK);
        if (on && song.playing && g->n)
            mo_hl(x, 73, (cw - 1) * (int32_t)((g->n - 1u) % len + 1u) / (int32_t)len, MO_INK);
    }
    for (k = 0; k < 12u; k++) {
        int32_t x = 4 + (int32_t)k * 9, y = BLACK[k] ? 78 : 86;
        uint32_t lit = (pcs >> k) & 1u, root = k == (pcs >> 12);
        if (lit) mo_fill(x, y, 8, 107 - y - (BLACK[k] ? 8 : 0), MO_INK);
        else mo_box(x, y, 8, 107 - y - (BLACK[k] ? 8 : 0), MO_DIM);
        if (lit && root) mo_fill(x + 3, y + 2, 2, 3, MO_BG);
    }
}
/* ANSWER: the phrase on a staff of its steps, a note a square at its height, lines joining them, the one playing
 * ringed; under it what it does now: LISTENING, an answer of FADE's count (a square an answer, the played ones dim),
 * LOOPS or PLAY KEYS. The squares outlined once the answers fade past half */
static void mo_pic_answer(uint32_t tr)
{
    const tessera_answer_t *a = tessera_answer(tr);
    uint32_t n = a->n, span = n ? (uint32_t)a->at[n - 1u] + 1u : 1u, lo = 127, hi = 0, ph = tess_an.phase[tr], i;
    uint32_t cnt = tessera_answer_count(tr), now = tess_an.slot[tr];
    int fade = ph == TESS_AN_PLAY && cnt && tess_an.ans[tr] * 2u >= cnt;
    int32_t px = -1, py = 0;
    char b[16];
    if (span < 8u) span = 8u;
    for (i = 0; i < n; i++) {
        lo = a->note[i] < lo ? a->note[i] : lo;
        hi = a->note[i] > hi ? a->note[i] : hi;
    }
    if (hi < lo + 7u) hi = lo + 7u;
    mo_hl(2, 97, 116, MO_DIM);
    for (int pass = 0; pass < 2; pass++)
        for (i = 0; i < n; i++) {
            int32_t x = 5 + (int32_t)a->at[i] * 110 / (int32_t)(span - 1u);
            int32_t y = 93 - (int32_t)(a->note[i] - lo) * 37 / (int32_t)(hi - lo);
            if (!pass) {
                if (px >= 0) mo_line(px, py, x, y, MO_DIM);
                px = x, py = y;
                continue;
            }
            if (fade && ph != TESS_AN_HEAR) mo_box(x - 1, y - 1, 3, 3, MO_INK);
            else mo_fill(x - 1, y - 1, 3, 3, MO_INK);
            if (song.playing && now == i + 1u && (ph == TESS_AN_PLAY || ph == TESS_AN_LOOP)) mo_circ(x, y, 3, MO_INK);
        }
    if (ph == TESS_AN_HEAR) str_cpy(b, "LISTENING", sizeof b);
    else if (!n || ph == TESS_AN_IDLE) str_cpy(b, "PLAY KEYS", sizeof b);
    else if (!cnt) str_cpy(b, "LOOPS", sizeof b);
    else {
        str_cpy(b, "ANSWER ", sizeof b);
        fmt_int(b + 7, (int32_t)(tess_an.ans[tr] + 1u));
        for (i = 0; i < cnt && i < 8u; i++) {
            int32_t x = 118 - (int32_t)(cnt - i) * 5;
            if (i == tess_an.ans[tr]) mo_fill(x, 102, 4, 4, MO_INK);
            else mo_box(x, 102, 4, 4, i < tess_an.ans[tr] ? MO_DIM : MO_INK);
        }
    }
    mo_text(b, 2, 101, ph == TESS_AN_HEAR ? MO_INK : MO_DIM);
}
/* STEP as seed: the loop as bars, a step the generator changed solid with the step as written outlined over it (cut
 * into the bar where it is lower), an unchanged one outlined, a rest a dash, past the loop a dot; the step playing
 * underlined. Under it MORPH's place between A and B (MORPH), and HOME: how many steps have moved off the written ones */
static void mo_pic_seed(uint32_t tr)
{
    uint8_t now[16], home[16];
    uint32_t at = 0, len = tessera_sd_loop(tr, now, home, &at), i, k, n = 0;
    int32_t lo = 127, hi = 0;
    for (i = 0; i < len; i++)
        for (k = 0; k < 2u; k++) {
            uint32_t e = k ? home[i] : now[i];
            if (e & 0x80u) {
                if ((int32_t)(e & 0x7Fu) < lo) lo = (int32_t)(e & 0x7Fu);
                if ((int32_t)(e & 0x7Fu) > hi) hi = (int32_t)(e & 0x7Fu);
            }
        }
    for (i = 0; i < 16u; i++) {
        int32_t x = 2 + (int32_t)i * 7 + (int32_t)(i / 4u), nh = 0;
        int head = song.playing && i == at, ch = i < len && now[i] != home[i];
        if (i >= len) {
            mo_px(x + 2, 92, MO_DIM);
            continue;
        }
        n += (uint32_t)ch;
        for (k = 0; k < 2u; k++) {
            uint32_t e = k ? home[i] : now[i];
            int32_t bh = 3 + (hi > lo ? ((int32_t)(e & 0x7Fu) - lo) * 33 / (hi - lo) : 0);
            if (k && !ch) break;
            if (!(e & 0x80u)) {
                if (!k) mo_hl(x, 92, 6, head ? MO_INK : MO_DIM);
                continue;
            }
            if (k) mo_box(x, 93 - bh, 6, bh, bh <= nh ? MO_BG : MO_INK);
            else if (ch || head) nh = bh, mo_fill(x, 93 - bh, 6, bh, MO_INK);
            else mo_box(x, 93 - bh, 6, bh, MO_INK);
        }
        mo_fill(x, 95, 6, head ? 2 : 1, head ? MO_INK : MO_DIM);
    }
    if (tessera.mode[tr] == TESS_ENG_MORPH) {
        mo_hl(2, 104, 50, MO_DIM);
        mo_fill(2 + (int32_t)tessera_morph(tr)->morph_pos * 47 / 127, 101, 3, 7, MO_INK);
    }
    mo_text("HOME", 60, 101, MO_DIM);
    mo_box(86, 101, 32, 7, MO_DIM);
    if (n) mo_fill(87, 102, (int32_t)(n * 30u / len) ? (int32_t)(n * 30u / len) : 1, 5, MO_INK);
}
static void mo_picture(void)
{
    const page_t *pg = cur_page();
    track_t *t = TSEL;
    uint32_t tr = song.sel;
    if (pg->graph == GR_STEPS && pg->fam == FAM_GEN && tr < TESSERA_TRACKS && tessera.mode[tr] != TESS_ENG_OFF) {
        uint8_t now[16], home[16];
        uint32_t at;
        if (tessera_sd_loop(tr, now, home, &at)) {      /* STEP as seed: its loop */
            mo_pic_seed(tr);
            return;
        }
        switch (tessera.mode[tr]) {
        case TESS_ENG_TURING: mo_pic_turing(tessera_turing(tr)); return;
        case TESS_ENG_EUCLID: mo_pic_euclid(tessera_euclid(tr)); return;
        case TESS_ENG_MARKOV: mo_pic_markov(tessera_markov(tr)); return;
        case TESS_ENG_MORPH: mo_pic_morph(tessera_morph(tr)); return;
        case TESS_ENG_GRIDS: mo_pic_grids(tessera_grids(tr), t); return;
        case TESS_ENG_BOUNCE: mo_pic_bounce(tessera_bounce(tr)); return;
        case TESS_ENG_AWAKE: mo_pic_awake(tessera_awake(tr)); return;
        case TESS_ENG_POLY: mo_pic_poly(tessera_poly(tr)); return;
        case TESS_ENG_QUAD: mo_pic_quad(tr); return;
        case TESS_ENG_CONSTEL: mo_pic_constel(tr); return;
        case TESS_ENG_SERIES: mo_pic_series(tessera_series(tr)); return;
        case TESS_ENG_CHORD: mo_pic_chord(tr); return;
        case TESS_ENG_ANSWER: mo_pic_answer(tr); return;
        default: mo_pic_detailed(); return;
        }
    }
    if ((pg->graph == GR_ROLL || pg->graph == GR_DRUMHIT) && grid_on()) {
        mo_pic_grid(t);
        return;
    }
    if (pg->graph == GR_ROLL) {
        mo_pic_notes(t);
        return;
    }
    if (pg->graph == GR_STEPS) {
        mo_pic_pattern(t);
        return;
    }
    if (pg->graph == GR_LFO) {
        mo_pic_lfo(t);
        return;
    }
    if (pg->graph == GR_MATRIX) {
        mo_pic_matrix();
        return;
    }
    mo_pic_detailed();                                  /* the rest: the detailed picture until its Mono one is drawn */
}

/* ---- lists over the page: a list knob's picker, OCT+'s whole list, the OCT+ sheets ---- */
/* a frame of w x h cells at (x, y) on the screen, a title row (if any), the rows 10 cells each: the one picked
 * inverted across the frame (a clear cell inside it), the value now a dot at the right, a value right-aligned; more
 * above or below the rows shown: a notch on the frame. Drawn in slices of 120 px (a canvas's height) */
typedef struct { char name[20], val[12]; uint8_t dim, warn; } mo_row_t;
static void mo_ascii(char *d, const char *s, uint32_t n)   /* the 5 x 7 font's: an ellipsis (0x85) three dots, other
                                                         * characters past it left out */
{
    uint32_t k = 0;
    for (; *s && k + 1u < n; s++)
        if ((uint8_t)*s == 0x85u)
            for (uint32_t j = 0; j < 3u && k + 1u < n; j++) d[k++] = '.';
        else if ((uint8_t)*s < 0x80u)
            d[k++] = *s;
    d[k] = 0;
}
static void mo_list(int32_t x, int32_t y, int32_t w, const char *title, const char *sub, const mo_row_t *row,
                    int32_t rows, int32_t sel, int32_t now, int more_up, int more_down)
{
    int32_t y0 = title ? 12 : 2, h = y0 + rows * 10 + 1, top, i;
    for (top = 0; top < h * 2; top += 120) {
        int32_t sh = h * 2 - top < 120 ? h * 2 - top : 120;
        cv_begin((uint32_t)w * 2u, (uint32_t)sh, MO_BG);
        cv_oy = -top;
        mo_box(0, 0, w, h, MO_INK);
        if (title) {
            char t[24];
            mo_cut(t, title, (uint32_t)(w - 6) / 6u);
            mo_text(t, 3, 2, MO_INK);
            if (sub && sub[0] && mo_tw(t) + mo_tw(sub) + 10 < w)
                mo_textr(sub, w - 3, 2, MO_DIM);
            mo_hl(1, 10, w - 2, MO_INK);
        }
        for (i = 0; i < rows; i++) {
            int32_t r = y0 + i * 10, on = i == sel;
            uint16_t ink = on ? MO_BG : row[i].dim ? MO_DIM : MO_INK;
            char t[24], a[24];
            mo_ascii(a, row[i].name, sizeof a);
            mo_cut(t, a, (uint32_t)(w - 8 - (row[i].val[0] ? mo_tw(row[i].val) + 4 : 0)) / 6u);
            if (row[i].warn && str_len(t) + 2u < sizeof t)
                str_cpy(t + str_len(t), " !", 3);   /* (a row that erases: red in the detailed look) */
            if (on)
                mo_fill(2, r, w - 4, 9, MO_INK);
            mo_text(t, 4, r + 1, ink);
            if (row[i].val[0])
                mo_textr(row[i].val, w - 4, r + 1, on ? MO_BG : MO_DIM);
            else if (i == now)
                mo_disc(w - 7, r + 4, 2, ink);
        }
        if (more_up)
            mo_hl(w / 2 - 3, 0, 7, MO_BG), mo_px(w / 2, 0, MO_INK);
        if (more_down)
            mo_hl(w / 2 - 3, h - 1, 7, MO_BG), mo_px(w / 2, h - 1, MO_INK);
        cv_oy = 0;
        cv_blit((uint32_t)x * 2u, (uint32_t)(y * 2 + top));
    }
}
/* the picker (pick_draw): the names around the value (pick_name, called directly: the stack audit resolves no
 * pointer), under its knob's label, over its column and the next */
static void mo_pick(int32_t col, int32_t count, int32_t cur, int32_t first, int32_t rows)
{
    mo_row_t r[8];
    int32_t i, tr;
    if (rows > 8) rows = 8;
    for (i = 0; i < rows; i++) {
        str_cpy(r[i].name, pick_name(first + i, &tr), sizeof r[i].name);
        r[i].val[0] = 0, r[i].warn = 0;
        r[i].dim = first + i - cur > 1 || cur - first - i > 1;
    }
    mo_list(1 + 30 * (col < 3 ? col : 2), 20, 58, 0, 0, r, rows, cur - first, -1, first > 0, first + rows < count);
}
/* OCT+'s whole list (vlist_draw): the list's name, the names around the one picked, the value now a dot; at the
 * bottom of the screen, its width */
static void mo_vlist(const char *title, const char *const *names, int32_t count, int32_t sel, int32_t now,
                     int32_t first, int32_t rows)
{
    mo_row_t r[8];
    int32_t i;
    char pos[12];
    if (rows > 8) rows = 8;
    for (i = 0; i < rows; i++) {
        str_cpy(r[i].name, names[i], sizeof r[i].name);
        r[i].val[0] = 0, r[i].dim = 0, r[i].warn = 0;
    }
    fmt_int(pos, sel + 1);                              /* where it is in the whole list: 3/12 */
    str_cpy(pos + str_len(pos), "/", 2);
    fmt_int(pos + str_len(pos), count);
    mo_list(0, 120 - (12 + rows * 10 + 1), 120, title, pos, r, rows, sel - first, now - first, first > 0,
            first + rows < count);
}

/* an OCT+ sheet (sheet_draw): its title and subtitle, a row an action, its value at the right */
static void mo_sheet(void)
{
    mo_row_t r[10];
    int32_t i, n = pop.n, first = 0, rows;
    rows = n < 10 ? n : 10;
    first = clamp((int32_t)pop.sel - (rows - 1) / 2, 0, n - rows);
    for (i = 0; i < rows; i++) {
        const sheet_row_t *sr = &pop.rows[first + i];
        str_cpy(r[i].name, sr->label, sizeof r[i].name);
        r[i].val[0] = 0;
        if (sr->value) {
            char v[20];
            sr->value(v);
            mo_ascii(r[i].val, v, sizeof r[i].val);
        }
        r[i].dim = 0;
        r[i].warn = sr->cf != 0;
    }
    mo_list(0, 120 - (12 + rows * 10 + 1), 120, pop.title, pop.sub, r, rows, (int32_t)pop.sel - first, -1, first > 0,
            first + rows < n);
}

/* what the page shows: drawn again only when it changes */
static uint32_t mo_signature(void)
{
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    const uint8_t *p = (const uint8_t *)mo_col;
    for (i = 0; i < sizeof mo_col; i++)
        h = (h ^ p[i]) * 16777619u;
    h = (h ^ (uint32_t)(uintptr_t)cur_page()) * 16777619u;
    h = (h ^ song.sel ^ (uint32_t)song.playing << 4 ^ (uint32_t)song.rec << 8 ^ (uint32_t)TSEL->eng_req << 16) * 16777619u;
    h = (h ^ graph_signature()) * 16777619u;
    h = (h ^ t->seq_idx ^ (uint32_t)ui.cursor << 16) * 16777619u;
    if (song.sel < TESSERA_TRACKS) {
        h = (h ^ tessera.mode[song.sel] ^ (uint32_t)tessera.euclid[song.sel].step_idx << 8 ^
             (uint32_t)tessera.grids[song.sel].step_idx << 16 ^ (uint32_t)tessera.turing[song.sel].reg << 24) * 16777619u;
        h = (h ^ tessera.morph[song.sel].step_idx ^ (uint32_t)tessera.markov[song.sel].cur_state << 8) * 16777619u;
        h = (h ^ tessera.markov[song.sel].last_hit ^ (uint32_t)tessera.markov[song.sel].tension << 8) * 16777619u;
    }
    if (cur_page()->graph == GR_ROLL)                   /* NOTES: the steps */
        h = (h ^ steps_hash(t) ^ (uint32_t)t->p[P_SLEN] << 8 ^ (uint32_t)t->p[P_SDIV] << 16) * 16777619u;
    if (cur_page()->graph == GR_LFO)                    /* the LFO's dot: 58 places a cycle */
        h = (h ^ (t->lfo_ph / (0xFFFFFFFFu / 58u))) * 16777619u;
    if (ui.msg_t)
        h = (h ^ str_hash(1u, ui.msg)) * 16777619u;
    return h;
}

static void mono_page(void)
{
    uint32_t s, sig;
    memset(mo_col, 0, sizeof mo_col);
    mo_on = 1;
    draw_columns();                                     /* (its draw_column calls land in mo_capture) */
    mo_on = 0;
    if (mo_flash_t)
        mo_flash_t--;
    sig = mo_signature() ^ (mo_flash_t ? 0x5A5A5A5Au : 0u);
    if (!ui.force && sig == mo_sig)
        return;
    mo_sig = sig;
    for (s = 0; s < 2u; s++) {                          /* the top half, then the bottom */
        cv_begin(240, 120, MO_BG);
        cv_oy = -(int32_t)s * 120;
        mo_head();
        mo_cols();
        mo_picture();
        mo_foot();
        cv_oy = 0;
        cv_blit(0, s * 120u);
    }
}

/* ---- the chrome of the screens still drawn whole by their own code (lists, MIXER, MODS, the browser, ...) ---- */
/* the header (draw_head; 18 px = 9 cells): the page's chip inverted (ERASING, a message or a layer's name in its
 * place), the octave shift, then at the right REC, PLAY or STOP and the tempo; its line on row 8 */
static void mo_dhead(const char *chip, const char *msg, int low)
{
    char b[24], t[24];
    int32_t x = 119;
    cv_begin(240, H_HEAD, MO_BG);
    fmt_int(b, song.g[G_BPM]);
    x = 119 - mo_tw(b);
    mo_text(b, x, 1, ui.bpm_t ? MO_INK : MO_DIM);
    x -= 7;
    if (chain.running || song.playing) {                /* PLAY: a triangle; STOP: a square */
        for (int32_t r = 0; r < 3; r++) mo_vl(x + r, 1 + r, 7 - 2 * r, MO_INK);
    } else {
        mo_fill(x, 2, 5, 5, MO_DIM);
    }
    if (song.rec) {
        x -= 7;
        if (((song.rec & ~song.rec_wait) >> song.sel) & 1u) mo_disc(x + 2, 4, 2, MO_INK);
        else mo_circ(x + 2, 4, 2, MO_INK);
    }
    if (low) {                                          /* the battery, only when it runs low */
        x -= 10;
        mo_box(x, 2, 7, 5, MO_INK), mo_vl(x + 7, 3, 3, MO_INK);
    }
    mo_ascii(t, msg ? msg : chip, sizeof t);
    mo_cut(b, t, (uint32_t)(x - 6) / 6u);
    mo_fill(0, 0, mo_tw(b) + 3, 8, MO_INK);
    mo_text(b, 1, 1, MO_BG);
    if (!msg && song.octave) {                          /* the octave shift, signed */
        t[0] = song.octave > 0 ? '+' : '-';
        fmt_int(t + 1, song.octave < 0 ? -song.octave : song.octave);
        mo_text(t, mo_tw(b) + 6, 1, MO_INK);
    }
    mo_hl(0, 8, 120, MO_INK);
    cv_blit(0, Y_HEAD);
}
/* a list page's rows (list_rows; 22 px = 11 cells apart): the label at the left, the value at the right (dim), the
 * row picked inverted across the screen, a row with more behind it a '>'; a scroll bar when not all fit */
static void mo_list_rows(uint32_t n, uint32_t row, uint32_t first, uint32_t vis,
                         int (*cap)(uint32_t r, char *lb, char *v, uint16_t *vc))
{
    uint32_t r;
    int32_t top, hh = (int32_t)vis * 11;
    char lb[16], v[16], a[20], t[20];
    uint16_t vc;
    for (top = 0; top < hh; top += 55) {                /* five rows a canvas (110 px) */
        int32_t sh = hh - top < 55 ? hh - top : 55;
        cv_begin(240, (uint32_t)sh * 2u, MO_BG);
        cv_oy = -top * 2;
        for (r = first; r < n && r < first + vis; r++) {
            int32_t y = (int32_t)(r - first) * 11, on = r == row, more, vw;
            if (y + 11 <= top || y >= top + sh)
                continue;
            more = cap(r, lb, v, &vc);
            if (on)
                mo_fill(0, y, 118, 10, MO_INK);
            mo_ascii(a, v, sizeof a);
            mo_cut(t, a, 9);
            vw = mo_tw(t);
            mo_textr(t, more ? 109 : 115, y + 2, on ? MO_BG : vc == T_DIM ? MO_DIM : MO_INK);
            if (more)
                mo_text(">", 111, y + 2, on ? MO_BG : MO_DIM);
            mo_ascii(a, lb, sizeof a);
            mo_cut(t, a, (uint32_t)(110 - vw - 8) / 6u);
            mo_text(t, 2, y + 2, on ? MO_BG : MO_DIM);
        }
        if (n > vis) {                                  /* the scroll bar: a cell wide at the right */
            int32_t th = hh * (int32_t)vis / (int32_t)n;
            int32_t ty = (int32_t)((uint32_t)(hh - th) * first / (n - vis));
            mo_vl(119, 0, hh, MO_DIM);
            mo_fill(118, ty, 2, th, MO_INK);
        }
        cv_oy = 0;
        cv_blit(0, (uint32_t)(LIST_Y0 + top * 2));
    }
}
/* the sound's strip at the bottom (pv_sound; 20 px): the page tabs where the page has them, else the track, its
 * engine and its sound's name */
static void mo_sound(const char *en, const char *nm)
{
    char a[24], b[24];
    int32_t x;
    cv_begin(240, 240 - PV_SOUND_Y, MO_BG);
    if (foot_tabs_on()) {
        cv_oy = -PV_SOUND_Y;
        mo_foot();
        cv_oy = 0;
    } else {
        mo_hl(0, 0, 120, MO_INK);
        b[0] = 'T', b[1] = (char)('1' + song.sel % 10u), b[2] = 0;
        mo_inv(b, 1, 2);
        mo_ascii(a, en, sizeof a);
        mo_cut(b, a, 8);
        x = mo_text(b, 16, 2, MO_INK) + 6;
        mo_ascii(a, nm, sizeof a);
        mo_cut(b, a, (uint32_t)(119 - x) / 6u);
        mo_text(b, x, 2, MO_DIM);
    }
    cv_blit(0, PV_SOUND_Y);
}
/* a confirm dialog (draw_confirm): a frame, a warning sign, the question, the name it is about, NO (OCT-) outlined and
 * YES (OCT+) filled */
/* s in up to two lines of n characters at most, broken between words (a word too long cut), centred on cx from row y
 * (one line: half a line lower) */
static void mo_lines(const char *s, int32_t cx, int32_t y, uint32_t n, uint16_t c)
{
    char u[48], l[2][24];
    uint32_t k, m = 0, i = 0;
    mo_ascii(u, s, sizeof u);
    for (k = 0; k < 2u && u[i]; k++) {
        uint32_t e = i, br = 0;
        while (u[e] && e - i < n) {
            if (u[e] == ' ') br = e;
            e++;
        }
        if (u[e] && u[e] != ' ' && br > i && k == 0u) e = br;   /* (between words; the last line cut) */
        mo_cut(l[k], u + i, e - i);
        m++;
        for (i = e; u[i] == ' '; i++) ;
    }
    for (k = 0; k < m; k++)
        mo_textc(l[k], cx, y + (m == 1u ? 4 : 0) + (int32_t)k * 8, c);
}
static void mo_confirm(const char *a, const char *b)
{
    int32_t w = DLG_W / 2, h = DLG_H / 2;
    cv_begin(DLG_W, DLG_H, MO_BG);
    mo_box(0, 0, w, h, MO_INK);
    mo_fill(w / 2 - 5, 2, 11, 11, MO_INK);
    mo_text("!", w / 2 - 2, 4, MO_BG);
    mo_lines(a, w / 2, 15, (uint32_t)(w - 6) / 6u, MO_INK);
    if (b[0])
        mo_lines(b, w / 2, 32, (uint32_t)(w - 6) / 6u, MO_DIM);
    mo_box(8, 49, 40, 11, MO_INK);
    mo_textc("NO", 28, 51, MO_INK);
    mo_fill(56, 49, 40, 11, MO_INK);
    mo_textc("YES", 76, 51, MO_BG);
    cv_blit(DLG_X, DLG_Y);
}

/* ---- Stage (HOME) ---- */
/* The header: PLAY or STOP and the tempo, the selected track's engine. A column a track, 30 cells: its role (the
 * selected track's inverted; an armed track's REC), the sound in two words of four letters, the engine, the pattern
 * playing (inverted) and the next one waiting (dashed), the bar playing as 4 x 4 steps (a hit filled, the playhead
 * hollow or notched), the level (dB or MUTE) over a bar; the selected column outlined. Under them the panel: the
 * output's waveform, the notes the selected track played and their chord over it (a DRUM track: its lanes, lit as
 * they hit). A knob turning: its four dials over the columns' tops, the panel the value. The top half is drawn again
 * when it changes, the bottom (the steps, the panel) every other frame */
static uint32_t mo_stage_sig;
static void mo_word(char *d, const char *s, uint32_t which)   /* s's first or second word, four letters */
{
    char w[24];
    uint32_t i = 0, k = 0;
    while (s[i] == ' ') i++;
    if (which) {
        while (s[i] && s[i] != ' ') i++;
        while (s[i] == ' ') i++;
    }
    for (; s[i] && s[i] != ' ' && k + 1u < sizeof w; i++) w[k++] = s[i];
    w[k] = 0;
    mo_abbr(d, w);
}
static void mo_stage_col(uint32_t k, int top)
{
    const track_t *t = &trk[k];
    uint32_t sel = k == song.sel, mute = t->p[P_MUTE] != 0u, arm = (song.rec >> k) & 1u, len = (uint32_t)t->p[P_SLEN];
    uint32_t page = song.playing && t->seq_idx < len ? t->seq_idx / 16u : 0u, ph = song.playing ? t->seq_idx : 0xFFFFu;
    uint32_t next = t->pattern_next < NPAT ? t->pattern_next + 1u : 0u, lvl = (uint32_t)t->p[P_LEVEL] & 127u, i, src, kk;
    int32_t x0 = (int32_t)k * 30, cx = x0 + 15;
    uint16_t ink = mute ? MO_DIM : MO_INK;
    char nm[16], a[8], b[8], chip[3] = {'P', (char)('1' + t->pattern % NPAT), 0};
    if (sel && !stage.out)
        mo_box(x0, 10, 30, 78, MO_INK);
    else if (sel && !top)                               /* (the knobs out: under them only) */
        mo_box(x0, 60, 30, 28, MO_INK), mo_hl(x0, 60, 30, MO_BG);
    if (top && !stage.out) {
        uint32_t cat;
        trk_short_name(k, nm);
        stage_entry(k, &src, &kk);
        cat = entry_cat(src, kk) % CAT_N;
        if (arm) str_cpy(a, "REC", sizeof a);
        else if (cat == CAT_DRUM) str_cpy(a, "DRUM", sizeof a);
        else if (cat == CAT_KEYS) str_cpy(a, "CHRD", sizeof a);
        else if (cat && cat != CAT_OTHER) mo_abbr(a, CAT_NAME[cat]);
        else a[0] = 'T', a[1] = (char)('1' + k), a[2] = 0;
        if (sel || arm) {
            mo_fill(x0 + 2, 12, 26, 9, ink);
            mo_textc(a, cx, 13, MO_BG);
        } else {
            mo_textc(a, cx, 13, ink);
        }
        mo_word(a, nm, 0);
        mo_word(b, nm, 1);
        mo_textc(a, cx, 23, ink);
        mo_textc(b, cx, 31, ink);
    }
    if (top) {
        str_cpy(a, eng_abbr(ENGINES[eng_idx(t->eng_req)]->name), 5);
        mo_textc(a, cx, 41, MO_DIM);
        mo_fill(x0 + 3, 50, 13, 9, ink);                /* the pattern playing; the next one dashed */
        mo_text(chip, x0 + 4, 51, MO_BG);
        if (next) {
            char n2[3] = {'P', (char)('0' + next), 0};
            mo_dash(x0 + 17, 50, 11, ink);
            mo_dash(x0 + 17, 58, 11, ink);
            mo_text(n2, x0 + 18, 51, ink);
        }
        return;
    }
    for (i = 0; i < 16u; i++) {                         /* the bar playing */
        uint32_t si = page * 16u + i;
        const step_t *st = &seq_steps(t)[si % NSTEP];
        int32_t x = x0 + 4 + (int32_t)(i % 4u) * 6, y = 62 + (int32_t)(i / 4u) * 4;
        if (si >= len)
            break;
        if (step_on(st)) {
            mo_fill(x, y, 4, 3, ink);
            if (si == ph) mo_px(x + 1, y + 1, MO_BG), mo_px(x + 2, y + 1, MO_BG);
        } else if (si == ph) {
            mo_box(x, y, 4, 3, MO_INK);
        } else {
            mo_px(x + 1, y + 1, st->time == ST_TIE ? ink : MO_DIM);
        }
    }
    if (mute) {
        str_cpy(a, "MUTE", sizeof a);
    } else if (lvl) {
        int32_t d = level_db_x10(lvl);
        fmt_int(a, (d + (d < 0 ? -5 : 5)) / 10);
    } else {
        str_cpy(a, "OFF", sizeof a);
    }
    mo_textc(a, cx, 78, ink);
    mo_dash(x0 + 3, 86, 24, MO_DIM);
    mo_hl(x0 + 3, 86, (int32_t)lvl * 24 / 127, ink);
}
static void mo_stage_head(void)
{
    char b[16];
    if (ui.msg_t && ui.msg[0]) {
        mo_cut(b, ui.msg, 15);
        mo_inv(b, 2, 1);
    } else {
        if (song.playing) {                             /* PLAY: a triangle; STOP: a square */
            int32_t r;
            for (r = 0; r < 7; r++) mo_vl(1 + r / 2, 1 + r / 2, 7 - r / 2 * 2, MO_INK);
        } else {
            mo_fill(1, 2, 5, 5, MO_INK);
        }
        fmt_int(b, song.g[G_BPM]);
        mo_text(b, 9, 1, ui.bpm_t ? MO_INK : MO_DIM);
        if (ui.bpm_t) mo_hl(9, 8, mo_tw(b), MO_INK);
    }
    str_cpy(b, eng_abbr(ENGINES[TSEL->eng_req % NENGINES]->name), 8);
    mo_textr(b, 118, 1, MO_DIM);
    mo_hl(0, 9, 120, MO_INK);
}
static void mo_stage_wave(int32_t top, int32_t h, uint16_t c)   /* the output, triggered on a rising zero crossing */
{
    int16_t *snap = scope_snap;
    uint32_t w = scope_w, i, trig = 1;
    int32_t peak = 1500, x, py = 0, cy = top + h / 2;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak) peak = snap[i];
        else if (-snap[i] > peak) peak = -snap[i];
    }
    for (i = 2; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) { trig = i; break; }
    for (x = 0; x < 116; x++) {
        uint32_t k = trig + (uint32_t)x * 2u;
        int32_t v = ((int32_t)snap[k - 1u] + 2 * snap[k] + snap[k + 1u]) / 4, y = cy - v * (h / 2 - 1) / peak;
        if (x) mo_line(1 + x, py, 2 + x, y, c);
        py = y;
    }
}
static void mo_stage_panel(void)
{
    uint32_t sel = song.sel % NTRK, bits[4], i, n = 0, pcs = 0, root = 0, held = 0, l;
    uint8_t note[8];
    char b[40];
    const char *q;
    if (drum_track(TSEL)) {                             /* a DRUM track: its lanes, lit as they hit */
        mo_stage_wave(90, 12, MO_DIM);
        for (l = 0; l < NLANE; l++) {
            int32_t x = 1 + (int32_t)(l % 4u) * 30, y = 104 + (int32_t)(l / 4u) * 0;
            char ab[6];
            mo_cut(ab, drum_lane_abbr(TSEL, l), 4);
            x = 1 + (int32_t)l * 15;
            if (stage.hit_t[l]) { mo_fill(x, y - 1, 14, 9, MO_INK); mo_textc(ab, x + 7, y, MO_BG); }
            else mo_textc(ab, x + 7, y, MO_DIM);
        }
        return;
    }
    if (stage.out) {                                    /* a knob turning: its value, large */
        int16_t *vp;
        const char *unit;
        const page_t *sp = stage_page();
        const param_desc_t *d = page_desc(sp, ui.hot_col & 3u, &vp);
        if (d && vp) {
            stage_value(sp, d, *vp, b, &unit);
            str_cpy(b + str_len(b), unit, 4);
            mo_text(stage_label(ui.hot_col & 3u, d), 2, 92, MO_DIM);
            mo_text_s(b, 2, 102, 2, MO_INK);
            return;
        }
    }
    fm1_irq_off();
    for (i = 0; i < 4u; i++) {
        bits[i] = live_last[sel][i];
        held |= live_held[sel][i];
    }
    fm1_irq_on();
    mo_stage_wave(90, 28, held ? MO_DIM : MO_INK);
    for (i = 0; i < 128u; i++)
        if ((bits[i >> 5] >> (i & 31u)) & 1u) {
            pcs |= 1u << (i % 12u);
            if (n < sizeof note) note[n] = (uint8_t)i;
            n++;
        }
    if (!n)
        return;
    q = micro_active(TSEL) ? 0 : chord_of(pcs, note[0] % 12u, &root);
    b[0] = 0;
    if (q) {
        str_cpy(b, N_NOTE[root], 4);
        str_cpy(b + str_len(b), q, 8);
        if (root != note[0] % 12u) {
            str_cpy(b + str_len(b), "/", 2);
            str_cpy(b + str_len(b), N_NOTE[note[0] % 12u], 4);
        }
        TEXT_CASE(1);
        mo_inv(b, 2, 91);
        TEXT_CASE(0);
    }
    for (i = 0, b[0] = 0; i < n && i < 5u; i++) {       /* the notes */
        if (i) str_cpy(b + str_len(b), " ", 2);
        note_name(b + str_len(b), note[i]);
    }
    b[19] = 0;
    mo_text(b, 2, 111, held ? MO_INK : MO_DIM);
}
static uint32_t mo_stage_signature(void)
{
    uint32_t h = 2166136261u, k, src, kk;
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        char nm[16];
        trk_short_name(k, nm);
        stage_entry(k, &src, &kk);
        h = str_hash(h, nm);
        h = (h ^ t->pattern ^ (uint32_t)t->pattern_next << 4 ^ (uint32_t)t->p[P_MUTE] << 8 ^ (uint32_t)t->eng_req << 12 ^
             entry_cat(src, kk) << 20) * 16777619u;
    }
    h = (h ^ song.sel ^ (uint32_t)song.rec << 4 ^ (uint32_t)song.playing << 8 ^ (uint32_t)song.g[G_BPM] << 12) * 16777619u;
    h = (h ^ stage.out ^ (uint32_t)(ui.bpm_t != 0) << 1 ^ (uint32_t)ui.hot_col << 2) * 16777619u;
    if (ui.msg_t)
        h = str_hash(h, ui.msg);
    if (stage.out) {
        const uint8_t *p = (const uint8_t *)mo_col;
        for (k = 0; k < sizeof mo_col; k++)
            h = (h ^ p[k]) * 16777619u;
    }
    return h;
}
static void mono_stage(void)
{
    uint32_t k, sig;
    stage_panel_hits();
    stage.out = (uint8_t)stage_knobs_out();
    memset(mo_col, 0, sizeof mo_col);
    if (stage.out) {                                    /* the selected track's four knobs, as its pages */
        mo_on = 1;
        stage_columns();
        mo_on = 0;
    }
    sig = mo_stage_signature();
    if (ui.force || sig != mo_stage_sig) {              /* the top half: header, roles, names, engines, patterns */
        mo_stage_sig = sig;
        cv_begin(240, 120, MO_BG);
        cv_oy = 0;
        mo_stage_head();
        for (k = 0; k < NTRK; k++)
            mo_stage_col(k, 1);
        if (stage.out) {                                /* the knobs over the columns' tops */
            mo_fill(0, 10, 120, 50, MO_BG);
            mo_cols();
        }
        cv_blit(0, 0);
    } else if (ui.frame & 1u) {
        return;
    }
    cv_begin(240, 120, MO_BG);                          /* the bottom half: steps, levels, the panel (every other frame) */
    cv_oy = -120;
    for (k = 0; k < NTRK; k++)
        mo_stage_col(k, 0);
    mo_dash(0, 88, 120, MO_DIM);
    mo_stage_panel();
    cv_oy = 0;
    cv_blit(0, 120);
}

/* ---- MIXER ---- */
/* a track's strip (pv_strip; 27 x 106 cells): its sound in two words of four letters, LEVEL as a dial with its dB,
 * the meter beside it, PAN a small dial and its side, SOLO boxed, MUTE inverted (silenced by another's solo: dim),
 * REC a dot (armed) or a ring; the selected track's strip outlined, the knob turning its value inverted */
static void mo_strip(uint32_t k, const char *nm, uint32_t lvl, int32_t pan, uint32_t flags, int32_t meter)
{
    uint32_t sel = flags & 1u, mute = flags & 2u, solo = flags & 4u, quiet = flags & 8u, arm = flags & 16u;
    uint32_t hot = flags >> 5 & 3u, octp = flags >> 7 & 1u;
    uint16_t ink = quiet ? MO_DIM : MO_INK;
    char a[8], b[8];
    int32_t h;
    cv_begin(CARD_W, PV_STRIP_ROWS, MO_BG);
    if (sel)
        mo_box(0, 0, CARD_W / 2, PV_STRIP_ROWS / 2, MO_INK);
    mo_word(a, nm, 0);
    mo_textc(a, 14, 3, ink);
    mo_word(a, nm, 1);
    mo_textc(a, 14, 12, ink);
    mo_dial(12, 31, 8, (int32_t)lvl * 1000 / 127, ink);
    if (lvl) {
        int32_t d = level_db_x10(lvl);
        fmt_int(b, (d + (d < 0 ? -5 : 5)) / 10);
    } else {
        str_cpy(b, "OFF", sizeof b);
    }
    if (hot == 1u) mo_inv(b, 13 - mo_tw(b) / 2, 43);
    else mo_textc(b, 13, 43, ink);
    mo_vl(24, 23, 18, MO_DIM);                          /* the meter */
    h = meter * 18 / (TS_MH - 2);
    if (h > 0)
        mo_fill(23, 41 - h, 3, h, ink);
    mo_textc("PAN", 13, 55, octp ? MO_INK : MO_DIM);
    mo_dial(13, 69, 5, (pan + 64) * 1000 / 127, ink);
    if (pan) {
        b[0] = pan < 0 ? 'L' : 'R';
        fmt_int(b + 1, pan < 0 ? -pan : pan);
    } else {
        str_cpy(b, "C", sizeof b);
    }
    if (hot == 2u) mo_inv(b, 14 - mo_tw(b) / 2, 77);
    else mo_textc(b, 14, 77, MO_DIM);
    if (solo) {
        mo_box(2, 86, 24, 11, MO_INK);
        mo_textc("SOLO", 14, 88, MO_INK);
    } else if (mute) {
        mo_fill(2, 86, 24, 11, MO_INK);
        mo_textc("MUTE", 14, 88, MO_BG);
    } else {
        mo_textc("MUTE", 14, 88, MO_DIM);
    }
    if (arm) mo_disc(13, 101, 3, MO_INK);
    else mo_circ(13, 101, 3, MO_DIM);
    cv_blit((uint32_t)CARD_X(k), PV_STRIP_TOP);
}

/* ---- MODS (CORDS) ---- */
/* a cord's row (cu_rows; 120 x 24 cells): its number, the source as its track and four letters, an arrow, the
 * destination so, the move at the right; the cord picked outlined; under them its light and move (mo_cu_bar) */
static void mo_cu_side(int32_t x, uint32_t tr, const char *name, int has_trk, int on)
{
    char a[8];
    if (!on) {
        mo_text("--", x + 8, 3, MO_DIM);
        return;
    }
    if (has_trk) {                                      /* the track: its numeral inverted */
        a[0] = (char)('1' + (tr & 3u)), a[1] = 0;
        mo_inv(a, x + 1, 3);
    }
    mo_abbr(a, name);
    mo_text(a, x + 8, 3, MO_INK);
}
static void mo_cu_row(uint32_t n, const cord_t *c, int sel, int on)
{
    char b[16];
    uint16_t ink = on ? MO_INK : MO_DIM;
    cv_begin(240, CU_H, MO_BG);
    if (sel)
        mo_box(1, 0, 118, CU_H / 2 - 1, MO_INK);
    fmt_int(b, (int32_t)n);
    mo_text(b, n >= 10u ? 3 : 5, 3, sel ? MO_INK : MO_DIM);
    mo_cu_side(13, c->src_trk, cu_src_name(c), 1, c->src != CS_OFF);
    mo_text(">", 47, 3, ink);
    mo_cu_side(54, c->dst_trk, cu_dst_name(c), !cu_global(c->dst), c->dst != CD_OFF);
    fmt_int(b + 1, cu_pct(c) < 0 ? -cu_pct(c) : cu_pct(c));
    b[0] = cu_pct(c) < 0 ? '-' : '+';
    str_cpy(b + str_len(b), "%", 2);
    mo_textr(cu_pct(c) ? b : b + 1, 116, 3, sel ? MO_INK : ink);
}
/* the move (cu_bar's canvas, 105 x 4 cells): the line, its middle, the amount from the middle, a mark where it moves
 * the destination now */
static void mo_cu_bar(const cord_t *c, int32_t s)
{
    int on = cord_live(c);
    int32_t a = cu_pct(c), m = 52, x = 1 + (a + 100) * 102 / 200, lx = m + clamp(s, -32768, 32768) * 51 / 32768;
    cv_begin(CU_BAR_W, 9, MO_BG);
    mo_hl(1, 2, 102, MO_DIM);
    if (a)
        mo_fill(x < m ? x : m, 1, x < m ? m - x : x - m, 2, on ? MO_INK : MO_DIM);
    mo_vl(m, 0, 4, MO_DIM);
    if (on && c->src)
        mo_vl(lx, 0, 4, MO_INK);
}
/* the light (cu_leds' canvas, 4 x 4 cells): how far the cord moves its destination now, lit or dim */
static void mo_cu_led(uint32_t lv)
{
    cv_begin(8, 8, MO_BG);
    if (lv)
        mo_disc(1, 1, 1, lv > 3u ? MO_INK : MO_DIM);
}

/* ---- a knob as a chip or a strip (pv_chip, pv_column's CS_STRIP) on the screens drawn by their own code ---- */
static void mo_four(char *d, const char *s)            /* four characters: a number shortened, a word abbreviated */
{
    char a[16], f[16];
    mo_ascii(a, s, sizeof a);
    mo_fit(f, a);
    if (str_len(f) <= 4u) str_cpy(d, f, 8);
    else mo_abbr(d, a);
}
/* the chip (27 x 9 cells): the value in four characters, centred; the knob turning: inverted */
static void mo_chip(uint32_t c, const char *val, const char *unit, uint16_t vc, int hot, int32_t y)
{
    char v[24], d[8];
    str_cpy(v, val, sizeof v);
    if (str_len(v) + str_len(unit) < sizeof v) str_cpy(v + str_len(v), unit, sizeof v - str_len(v));
    mo_four(d, v);
    cv_begin(CARD_W, PV_CHIP_H, MO_BG);
    if (hot) {
        mo_fill(0, 0, CARD_W / 2, PV_CHIP_H / 2, MO_INK);
        mo_textc(d, CARD_W / 4, 1, MO_BG);
    } else if (d[0]) {
        mo_textc(d, CARD_W / 4, 1, vc == T_DIM ? MO_DIM : MO_INK);
    }
    cv_blit((uint32_t)CARD_X(c), (uint32_t)y);
}
/* the strip (27 x 15 cells): the label in four letters (dim) over the value in four characters; the knob turning:
 * outlined */
static void mo_strip_col(uint32_t c, const char *label, const char *val, uint16_t vc, int hot, int32_t y)
{
    char d[8];
    cv_begin(CARD_W, PV_STRIP_H, MO_BG);
    if (hot)
        mo_box(0, 0, CARD_W / 2, PV_STRIP_H / 2, MO_INK);
    mo_abbr(d, label);
    mo_textc(d, CARD_W / 4, 0, MO_DIM);
    mo_four(d, val);
    mo_textc(d, CARD_W / 4, 8, vc == T_DIM ? MO_DIM : MO_INK);
    cv_blit((uint32_t)CARD_X(c), (uint32_t)y);
}

/* ---- the help card (ARP held + a knob; help_draw): 114 x 107 cells under the header ---- */
/* the name inverted at the top, the value right of it; the text wrapped to 17 characters a line (help_wrap measures in
 * cells here), eight lines and a scroll bar; what turning does next under a line */
static void mo_help(const char *name, const char *val, const char *text)
{
    uint16_t at[64];
    uint8_t len[64];
    char a[64], b[24], line[64];
    uint32_t n = help_wrap(HELP_FONT, text, at, len, 64), rows = HELP_ROWS, i;
    int32_t w = HELP_W / 2, h = HELP_H / 2, top, vw;
    const char *hint;
    if (n > rows && help.top + rows < n) hint = "CW: MORE  CCW: UP";
    else if (help.top) hint = "CCW: UP";
    else hint = "ARP + KNOB: HELP";
    for (top = 0; top < h * 2; top += 120) {
        int32_t sh = h * 2 - top < 120 ? h * 2 - top : 120, y = HELP_T0 / 2;
        cv_begin((uint32_t)w * 2u, (uint32_t)sh, MO_BG);
        cv_oy = -top;
        mo_box(0, 0, w, h, MO_INK);
        mo_ascii(a, val, sizeof a);
        mo_cut(b, a, 8);
        vw = b[0] ? mo_tw(b) + 4 : 0;
        mo_textr(b, w - 4, 3, MO_INK);
        mo_ascii(a, name ? name : "", sizeof a);
        mo_cut(b, a, (uint32_t)(w - 10 - vw) / 6u);
        mo_fill(2, 2, mo_tw(b) + 3, 9, MO_INK);
        mo_text(b, 4, 3, MO_BG);
        mo_hl(1, 12, w - 2, MO_INK);
        for (i = help.top; i < n && i < 64u && i < help.top + rows; i++, y += (int32_t)HELP_LH / 2) {
            memcpy(line, text + at[i], len[i]);
            line[len[i]] = 0;
            mo_ascii(a, line, sizeof a);
            mo_text(a, 4, y, MO_INK);
        }
        if (n > rows) {                                 /* (the scroll bar: the lines shown of all of them) */
            int32_t th = (HELP_T1 - HELP_T0) / 2 - 2, bh = th * (int32_t)rows / (int32_t)n;
            mo_vl(w - 3, HELP_T0 / 2, th, MO_DIM);
            mo_fill(w - 4, HELP_T0 / 2 + th * (int32_t)help.top / (int32_t)n, 3, bh > 3 ? bh : 3, MO_INK);
        }
        mo_hl(1, h - 12, w - 2, MO_INK);
        mo_text(hint, 4, h - 9, MO_DIM);
        cv_oy = 0;
        cv_blit(HELP_X, (uint32_t)(HELP_Y + top));
    }
}

/* ---- SAVE > PRESETS, the sound browser (browser_draw): drawn whole when anything on it changes ---- */
/* the header: the track and its list inverted, the list's length (or the message) at the right. Left, LIST: its names
 * 9 cells apart, the one shown inverted. Right, the sounds: the one shown in a frame (its engine, its place), two before
 * and three after (dim further off); the place as a scroll bar; a fast turn's acceleration inverted (x4). The foot:
 * the favourite's star (KNOB 3), BACK (OCT-) outlined, OK (OCT+) filled */
static uint32_t mo_br_sig;
static void mo_browser(uint32_t cur, uint32_t total)
{
    uint32_t m = list_mode(), out = cur >= total, at = out ? 0u : cur, k, src, s, first;
    int pending = browse_pending(), badge = pending && brw.x > 1u, fav = browser_shown_fav(), back = browser_can_back();
    uint32_t sig = at * 7u + total * 131u + m * 1031u + (uint32_t)pending * 3u + (badge ? brw.x : 0u) * 40503u +
                   out * 17u + song.sel * 11u + up_gen * 7919u + ux.pal * 31u + (uint32_t)fav * 65537u +
                   (uint32_t)back * 257u + (ui.msg_t ? str_hash(3u, ui.msg) : 0u);
    char a[24], b[24], tag[6], nm[16];
    if (!ui.force && sig == mo_br_sig)
        return;
    mo_br_sig = sig;
    first = (uint32_t)clamp((int32_t)m - 5, 0, (int32_t)LM_N - 10);
    for (s = 0; s < 2u; s++) {
        int32_t r;
        cv_begin(240, 120, MO_BG);
        cv_oy = -(int32_t)s * 120;
        b[0] = (char)('1' + song.sel % NTRK), b[1] = ' ';   /* the header */
        mo_ascii(a, list_name(m), sizeof a);
        mo_cut(b + 2, a, 8);
        mo_fill(0, 0, mo_tw(b) + 3, 8, MO_INK);
        mo_text(b, 1, 1, MO_BG);
        if (ui.msg_t) {
            mo_ascii(a, ui.msg, sizeof a);
            mo_cut(b, a, (uint32_t)(117 - mo_tw(b) - 6) / 6u);
            mo_textr(b, 119, 1, MO_INK);
        } else {
            fmt_int(b, (int32_t)total);
            str_cpy(b + str_len(b), total == 1u ? " SOUND" : " SOUNDS", 8);
            mo_textr(b, 119, 1, MO_DIM);
        }
        mo_hl(0, 8, 120, MO_INK);
        for (r = 0; r < 10 && first + (uint32_t)r < LM_N; r++) {   /* LIST: ten rows */
            uint32_t i = first + (uint32_t)r;
            int32_t y = 12 + r * 9;
            mo_ascii(a, browser_list_name(i), sizeof a);
            mo_cut(b, a, 7);
            if (i == m)
                mo_fill(0, y - 1, 44, 9, MO_INK);
            mo_text(b, 2, y, i == m ? MO_BG : i < LM_CAT ? MO_INK : MO_DIM);
        }
        mo_vl(46, 11, 96, MO_DIM);
        if (!total) {
            mo_textc(m == LM_FAV ? "NO FAVES" : m == LM_RECENT ? "NONE YET" : "NO SOUNDS", 83, 48, MO_DIM);
        } else {
            for (r = -2; r <= 3; r++) {                 /* the sounds round the one shown */
                int32_t n = (int32_t)at + r;
                if (!r || n < 0 || n >= (int32_t)total)
                    continue;
                src = preset_at((uint32_t)n, &k);
                entry_label(src, k, tag, nm);
                mo_ascii(a, nm[0] ? nm : tag, sizeof a);
                mo_cut(b, a, 11);
                mo_text(b, 50, r < 0 ? 40 + r * 12 : 54 + r * 12, r == -1 || r == 1 ? MO_INK : MO_DIM);
            }
            src = preset_at(at, &k);                    /* the card */
            entry_label(src, k, tag, nm);
            mo_box(48, 39, 68, 23, out ? MO_DIM : MO_INK);
            mo_ascii(a, nm[0] ? nm : tag, sizeof a);
            mo_cut(b, a, 10);
            mo_text(b, 51, 42, out ? MO_DIM : MO_INK);
            mo_ascii(a, ENGINES[eng_idx(src_engine(src, k))]->name, sizeof a);
            mo_cut(b, a, 4);
            mo_text(b, 51, 52, MO_DIM);
            fmt_int(b, (int32_t)at + 1);
            str_cpy(b + str_len(b), "/", 2);
            fmt_int(b + str_len(b), (int32_t)total);
            mo_textr(b, 113, 52, MO_DIM);
            mo_vl(119, 11, 96, MO_DIM);                 /* the scroll bar: the place */
            mo_fill(118, 11 + (total > 1u ? (int32_t)at * 86 / (int32_t)(total - 1u) : 0), 2, 10,
                    out ? MO_DIM : MO_INK);
            if (badge) {                                /* a fast turn: its acceleration */
                b[0] = 'X';
                fmt_int(b + 1, brw.x);
                mo_fill(113 - mo_tw(b) - 1, 11, mo_tw(b) + 2, 9, MO_INK);
                mo_text(b, 113 - mo_tw(b), 12, MO_BG);
            }
        }
        mo_hl(0, 108, 120, MO_INK);                     /* the foot */
        if (fav) mo_inv("* FAV", 2, 111);
        else mo_text("* FAV", 2, 111, MO_DIM);
        mo_box(62, 110, 28, 10, back ? MO_INK : MO_DIM);
        mo_textc("BACK", 76, 111, back ? MO_INK : MO_DIM);
        mo_fill(93, 110, 27, 10, MO_INK);
        mo_textc("OK", 106, 111, MO_BG);
        cv_oy = 0;
        cv_blit(0, s * 120u);
    }
}

/* ---- SEQ > PATTERNS and SEQ > SONG (patterns_draw, song_draw): drawn whole when anything on them changes ---- */
static uint32_t mo_pt_sig;
static void mo_dbox(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a dashed outline */
{
    mo_dash(x, y, w, c), mo_dash(x, y + h - 1, w, c);
    for (int32_t i = 2; i < h - 1; i += 2)
        mo_px(x, y + i, c), mo_px(x + w - 1, y + i, c);
}
/* PATTERNS under the header: the patterns' numbers; a row a track: its number boxed (the selected one filled), a cell
 * per pattern, 11 x 16: the one playing filled (how far through it a notch at its foot), the others with something
 * in them outlined with their thirds' notes as three bars, the one waiting dashed, empty ones dim; Copy to: the
 * place framed twice. At the foot the song's sections as letters (the one playing or picked filled), no song: the
 * jam's */
static void mo_patterns(void)
{
    uint32_t n = chain_config.count, jamrows = !n && jam.n, cur, i, k, b, s, sig = 2166136261u;
    const uint8_t (*pat)[NTRK] = jamrows ? (const uint8_t (*)[NTRK])jam.pat : (const uint8_t (*)[NTRK])chain_patterns;
    uint8_t rep[CHAIN_ROWS];
    if (jamrows)
        n = jam.n;
    for (i = 0; i < n; i++)
        rep[i] = jamrows ? jam.rep[i] : chain_config.row[i].repeat;
    cur = chain.running ? chain.row : jamrows ? n - 1u : ui.song_row;
    if (n && cur >= n)
        cur = n - 1u;
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        sig = fnv1(fnv1(fnv1(sig, steps_hash(t)), t->pattern_gen), t->pattern + 16u * t->pattern_next +
                   65536u * (uint32_t)t->p[P_SLEN] + 16384u * (uint32_t)(t->p[P_MUTE] != 0));
        sig = fnv1(sig, pt_prog(t) * 977u);
    }
    sig = fnv1(sig, song_hash(pat, rep, n) + cur * 7919u + jamrows * 3u + song.sel * 131u + ux.pal * 31u);
    if (ptc_on())
        sig = fnv1(sig, 1u + ui.ptc_src + 16u * ui.ptc_dst);
    if (!ui.force && sig == mo_pt_sig)
        return;
    mo_pt_sig = sig;
    for (s = 0; s < 2u; s++) {                          /* rows 9 .. 59, then 60 .. 119 */
        cv_begin(240, s ? 120u : 102u, MO_BG);
        cv_oy = s ? -120 : -18;
        for (b = 0; b < NPAT; b++) {
            char c[2] = {(char)('1' + b), 0};
            mo_textc(c, 20 + 13 * (int32_t)b, 11, MO_DIM);
        }
        for (k = 0; k < NTRK; k++) {
            const track_t *t = &trk[k];
            uint16_t tc = t->p[P_MUTE] ? MO_DIM : MO_INK;
            uint32_t prog = pt_prog(t);
            int32_t y = 20 + (int32_t)k * 19;
            char c[2] = {(char)('1' + k), 0};
            if (k == song.sel) {
                mo_fill(1, y + 3, 9, 9, tc);
                mo_text(c, 3, y + 4, MO_BG);
            } else {
                mo_box(1, y + 3, 9, 9, tc);
                mo_text(c, 3, y + 4, tc);
            }
            for (b = 0; b < NPAT; b++) {
                int32_t x = 15 + 13 * (int32_t)b;
                int dst = ptc_on() && k == ui.ptc_trk && b == ui.ptc_dst, used = pattern_used(k, b);
                if (dst)                                /* Copy to: where it goes */
                    mo_box(x - 1, y - 1, 13, 18, MO_INK);
                if (b == t->pattern) {                  /* playing: filled, how far through it */
                    mo_fill(x, y, 11, 16, tc);
                    if (prog)
                        mo_hl(x + 1, y + 13, 1 + (int32_t)prog * 8 / 17, MO_BG);
                    continue;
                }
                if (b == t->pattern_next)               /* waiting for the bar: dashed */
                    mo_dbox(x, y, 11, 16, tc);
                else
                    mo_box(x, y, 11, 16, used || dst ? tc : MO_DIM);
                if (used) {                             /* its thirds' notes as three bars */
                    uint32_t len, div, j, q[3] = {0, 0, 0}, m = 1;
                    const step_t *st = pat_steps(k, b, &len, &div);
                    len = len > NSTEP ? NSTEP : len ? len : 1u;
                    for (j = 0; j < len; j++)
                        if (step_on(&st[j])) q[j * 3u / len]++;
                    for (j = 0; j < 3u; j++)
                        if (q[j] > m) m = q[j];
                    for (j = 0; j < 3u; j++) {
                        int32_t h = q[j] ? 2 + (int32_t)(q[j] * 9u / m) : 1;
                        mo_vl(x + 3 + 2 * (int32_t)j, y + 14 - h, h, tc);
                    }
                } else if (dst) {
                    mo_textc("+", x + 6, y + 5, MO_INK);
                }
            }
        }
        mo_hl(0, 98, 120, MO_DIM);
        mo_text(jamrows ? "JAM" : "SONG", 2, 104, MO_DIM);
        if (!n) {
            mo_text("--", 30, 104, MO_DIM);
        } else {
            uint32_t first = cur > 2u ? cur - 2u : 0u;
            if (n > 6u && first > n - 6u) first = n - 6u;
            if (n <= 6u) first = 0;
            for (i = first; i < first + 6u && i < n; i++) {
                int32_t x = 28 + 15 * (int32_t)(i - first);
                char l[2] = {row_letter(pat, i), 0};
                if (i == cur) mo_fill(x, 102, 13, 11, MO_INK);
                else mo_box(x, 102, 13, 11, MO_DIM);
                mo_textc(l, x + 7, 104, i == cur ? MO_BG : MO_INK);
            }
        }
        cv_oy = 0;
        cv_blit(0, s ? 120u : 18u);
    }
}
/* SONG: the header (its length at the tempo, the jam at the right: REC while it logs); the sections as columns of 28
 * cells (their letters, repeats after them; the row picked underlined), a track's patterns as blocks (one pattern
 * over sections in a row as one; the selected track's in the row picked framed twice, an empty one there dashed); a
 * song playing: its playhead. At the foot the sections and bars, how far the song has played */
static void mo_song(void)
{
    uint32_t n = chain_config.count, j, k, s, sig;
    uint32_t secs = sg.bars * 240u / (uint32_t)(song.g[G_BPM] > 0 ? song.g[G_BPM] : 120), jam_on = song.playing && !chain.running;
    char b[32], l[8];
    sig = fnv1(fnv1(sg.first + 16u * sg.ncols + 256u * sg.sel, (uint32_t)(sg.px + 1)), sg.bars * 7u + sg.play * 131u);
    sig = fnv1(sig, secs * 7u + jam.n * 131u + jam_on * 3u + song.sel * 1031u + ux.pal * 31u +
                    (ui.msg_t ? str_hash(3u, ui.msg) : 0u) + song.rec * 40503u);
    for (j = 0; j < n; j++)
        sig = fnv1(sig, chain_config.row[j].repeat + 32u * (uint32_t)row_letter((const uint8_t (*)[NTRK])chain_patterns, j));
    for (k = 0; k < NTRK; k++) {
        sig = fnv1(sig, trk[k].pattern_gen + 16384u * (uint32_t)(trk[k].p[P_MUTE] != 0));
        for (j = 0; j < n; j++)
            sig = fnv1(sig, chain_patterns[j][k] + 16u * (uint32_t)pattern_used(k, chain_patterns[j][k]));
    }
    if (!ui.force && sig == mo_pt_sig)
        return;
    mo_pt_sig = sig;
    for (s = 0; s < 2u; s++) {
        int32_t x;
        cv_begin(240, 120, MO_BG);
        cv_oy = -(int32_t)s * 120;
        mo_fill(0, 0, mo_tw("SONG") + 3, 8, MO_INK);    /* the header */
        x = mo_text("SONG", 1, 1, MO_BG) + 4;
        if (sg.bars) {
            fmt_int(b, (int32_t)(secs / 60u));
            str_cpy(b + str_len(b), secs % 60u < 10u ? ":0" : ":", 3);
            fmt_int(b + str_len(b), (int32_t)(secs % 60u));
            x = mo_text(b, x, 1, MO_DIM);
        }
        if (ui.msg_t) {
            char a[24];
            mo_ascii(a, ui.msg, sizeof a);
            mo_cut(b, a, (uint32_t)(119 - x - 6) / 6u);
            mo_textr(b, 119, 1, MO_INK);
        } else if (jam_on || jam.n) {                   /* the jam: logging (a dot), its rows */
            if (jam_on) {
                str_cpy(b, "JAM", 4);
            } else {
                str_cpy(b, "JAM ", 5);
                fmt_int(b + 4, jam.n);
                str_cpy(b + str_len(b), jam.n > 1u ? " ROWS" : " ROW", 6);
            }
            mo_textr(b, 119, 1, jam_on ? MO_INK : MO_DIM);
            if (jam_on)
                mo_disc(119 - mo_tw(b) - 5, 4, 2, MO_INK);
        }
        mo_hl(0, 8, 120, MO_INK);
        for (j = 0; j < SG_COLS && sg.first + j < sg.ncols; j++) {   /* the sections: rules, letters */
            uint32_t r = sg.first + j;
            int32_t cx = 4 + 28 * (int32_t)j;
            l[0] = '+', l[1] = 0;
            if (r < n) {
                l[0] = row_letter((const uint8_t (*)[NTRK])chain_patterns, r);
                if (chain_config.row[r].repeat > 1u)
                    l[1] = 'X', fmt_int(l + 2, chain_config.row[r].repeat);
            }
            mo_vl(cx, 11, 85, MO_DIM);
            mo_text(l, cx + 3, 11, r == sg.sel ? MO_INK : MO_DIM);
            if (r == sg.sel)
                mo_hl(cx + 2, 19, mo_tw(l) + 2, MO_INK);
        }
        for (k = 0; k < NTRK; k++) {                    /* a track's blocks */
            const track_t *t = &trk[k];
            uint16_t tc = t->p[P_MUTE] ? MO_DIM : MO_INK;
            int32_t y = 23 + (int32_t)k * 18;
            for (j = 0; j < SG_COLS && sg.first + j < n;) {
                uint32_t r = sg.first + j, p = chain_patterns[r][k], e = j + 1u, pick = k == song.sel && r == sg.sel;
                int used = pattern_used(k, p);
                int32_t bx = 6 + 28 * (int32_t)j, w;
                char pl[3] = {'P', (char)('1' + p % NPAT), 0};
                while (!pick && used && e < SG_COLS && sg.first + e < n && chain_patterns[sg.first + e][k] == p &&
                       !(k == song.sel && sg.first + e == sg.sel))
                    e++;
                w = 28 * (int32_t)(e - j) - 4;
                if (used) {
                    if (pick)
                        mo_box(bx - 1, y - 1, w + 2, 16, MO_INK);
                    mo_fill(bx + (pick ? 1 : 0), y + (pick ? 1 : 0), w - (pick ? 2 : 0), pick ? 12 : 14, tc);
                    mo_text(pl, bx + 3, y + 4, MO_BG);
                } else if (pick) {
                    mo_dbox(bx, y, w, 14, tc);
                    mo_text(pl, bx + 3, y + 4, tc);
                }
                j = e;
            }
        }
        if (sg.px >= 0)                                 /* the playhead (sg_frame: px of 8 + 56 a column) */
            mo_vl(4 + (sg.px - 8) / 2, 11, 85, MO_INK);
        mo_hl(0, 98, 120, MO_DIM);                      /* the foot */
        if (!n) {
            mo_text("NO SECTIONS YET", 2, 101, MO_DIM);
        } else {
            fmt_int(b, (int32_t)n);
            str_cpy(b + str_len(b), n > 1u ? " SECTIONS " : " SECTION ", 11);
            fmt_int(b + str_len(b), (int32_t)sg.bars);
            str_cpy(b + str_len(b), sg.bars > 1u ? " BARS" : " BAR", 6);
            b[19] = 0;                                  /* (19 characters fit) */
            mo_text(b, 2, 101, MO_DIM);
        }
        mo_box(2, 112, 116, 5, MO_DIM);
        if (chain.running && sg.bars) {
            int32_t w = (int32_t)(sg.play * 114u / sg.bars);
            mo_fill(3, 113, w < 2 ? 2 : w, 3, MO_INK);
        }
        cv_oy = 0;
        cv_blit(0, s * 120u);
    }
}

/* ---- PROJECT, USER, the STOREs (slot_draw): a strip a row ---- */
/* the slot picked (46 px = 23 cells): framed, its tag inverted, its name, a line about it (dim), BOOT inverted at the
 * right on the one power-on loads */
static void mo_slot_card(const char *tag, const char *nm, int used, const char *d, int boot, int32_t y)
{
    char a[24], b[24];
    int32_t x;
    cv_begin(240, SL_CARD, MO_BG);
    mo_box(1, 0, 118, 23, MO_INK);
    mo_ascii(a, tag, sizeof a);
    mo_inv(a, 5, 4);
    x = 5 + mo_tw(a) + 5 > 32 ? 5 + mo_tw(a) + 5 : 32;   /* (the names in a column, as the rows') */
    mo_ascii(a, nm, sizeof a);
    mo_cut(b, a, (uint32_t)((boot ? 88 : 115) - x) / 6u);
    mo_text(b, x, 4, used ? MO_INK : MO_DIM);
    mo_ascii(a, d, sizeof a);
    mo_cut(b, a, (uint32_t)(115 - x) / 6u);
    mo_text(b, x, 14, MO_DIM);
    if (boot)
        mo_inv("BOOT", 92, 4);
    cv_blit(0, (uint32_t)y);
}
/* another slot (22 px = 11 cells): its tag and name (dim while empty), BOOT at the right, a selected one (bulk) marked
 * at the left; the scroll bar (ty, th: its thumb in px from this row's top; th 0: none) a cell wide at the right */
static void mo_slot_row(const char *tag, const char *nm, int used, int boot, int mark, int32_t y, int32_t ty, int32_t th)
{
    char a[24], b[24];
    cv_begin(240, SL_ROW, MO_BG);
    if (mark)
        mo_vl(1, 1, 9, MO_INK);
    mo_ascii(a, tag, sizeof a);
    mo_text(a, 5, 2, MO_DIM);
    mo_ascii(a, nm, sizeof a);
    mo_cut(b, a, (uint32_t)((boot ? 88 : 115) - 32) / 6u);
    mo_text(b, 32, 2, used ? MO_INK : MO_DIM);
    if (boot)
        mo_textr("BOOT", 115, 2, MO_DIM);
    if (th) {
        mo_vl(119, 0, 11, MO_DIM);
        if (ty < SL_ROW && ty + th > 0)
            cv_rect(236, ty, 4, th, MO_INK);
    }
    cv_blit(0, (uint32_t)y);
}

/* ---- SCL > SCALES (scales_draw): as the browser, drawn whole when anything on it changes ---- */
/* the header: the track and SCALES inverted, the family's count; the families down the left (the one picked inverted),
 * its scales down the right (the one picked framed: its notes and period under it, a star when a favourite); the
 * place as a scroll bar; the favourite's star, BACK and OK */
static uint32_t mo_sc_sig;
static void mo_scales(uint32_t scale, uint32_t total, uint32_t rank, uint32_t fav)
{
    static const char *const FAM[SCALE_FAMILIES + 2u] = {"ALL", "WESTERN", "EDO", "JUST", "HARMON", "HISTOR", "REGION",
                                                         "NON-OCT", "MAQAM", "* FAVES"};
    uint32_t f, s, sig = total * 7u + song.sel * 131u + ux.pal * 31u + ui.scale_family * 7919u + rank * 40503u +
                            scale * 1031u + fav;
    char a[32], b[32];
    if (!ui.force && sig == mo_sc_sig)
        return;
    mo_sc_sig = sig;
    for (s = 0; s < 2u; s++) {
        int32_t d;
        cv_begin(240, 120, MO_BG);
        cv_oy = -(int32_t)s * 120;
        b[0] = (char)('1' + song.sel % NTRK);
        str_cpy(b + 1, " SCALES", 8);
        mo_fill(0, 0, mo_tw(b) + 3, 8, MO_INK);
        mo_text(b, 1, 1, MO_BG);
        fmt_int(b, (int32_t)total);
        str_cpy(b + str_len(b), total == 1u ? " SCALE" : " SCALES", 8);
        mo_textr(b, 119, 1, MO_DIM);
        mo_hl(0, 8, 120, MO_INK);
        for (f = 0; f < SCALE_FAMILIES + 2u; f++) {     /* the families */
            int32_t y = 12 + (int32_t)f * 9;
            if (f == ui.scale_family)
                mo_fill(0, y - 1, 44, 9, MO_INK);
            mo_text(FAM[f], 2, y, f == ui.scale_family ? MO_BG : MO_INK);
        }
        mo_vl(46, 11, 96, MO_DIM);
        if (!total || rank >= total)
            mo_textc(!total ? (ui.scale_family == SCALE_FAMILIES + 1u ? "NO FAVES" : "NO SCALES") : "NOT HERE", 83, 48,
                     MO_DIM);
        for (d = -2; d <= 3 && total && rank < total; d++) {
            int32_t at = (int32_t)rank + d;
            uint32_t id;
            if (at < 0 || at >= (int32_t)total)
                continue;
            id = scale_picker_at((uint32_t)at);
            scales_name(a, id, sizeof a);
            mo_ascii(b, a, sizeof b);
            if (!d) {                                   /* the one picked: its name in two lines */
                uint32_t n = str_len(b), cut = n > 10u ? 10u : n, k;
                char l[12];
                for (k = cut; n > 10u && k > 3u; k--)  /* (at a space or after a hyphen that fits) */
                    if (b[k] == ' ' || b[k - 1u] == '-') { cut = k; break; }
                mo_box(48, 33, 68, 33, MO_INK);
                mo_cut(l, b, cut);
                mo_text(l, 51, 36, MO_INK);
                while (b[cut] == ' ') cut++;
                mo_cut(l, b + cut, 10);
                mo_text(l, 51, 45, MO_INK);
                if (scale_favorite(id))
                    mo_text("*", 110, 36, MO_INK);
                fmt_int(b, SCALE_DEGREES[id]);
                str_cpy(b + str_len(b), SCALE_FAMILY[id] == 6u ? " NON-OCT" : " NOTES", 10);
                mo_text(b, 51, 56, MO_DIM);
            } else {
                b[11] = 0;
                mo_text(b, 50, d < 0 ? 36 + d * 11 : 59 + d * 11, d == -1 || d == 1 ? MO_INK : MO_DIM);
            }
        }
        if (total > 1u) {                               /* the scroll bar: the place */
            mo_vl(119, 11, 96, MO_DIM);
            mo_fill(118, 11 + (int32_t)rank * 86 / (int32_t)(total - 1u), 2, 10, MO_INK);
        }
        mo_hl(0, 108, 120, MO_INK);                     /* the foot */
        if (scale_favorite(scale)) mo_inv("* FAV", 2, 111);
        else mo_text("* FAV", 2, 111, MO_DIM);
        mo_box(62, 110, 28, 10, MO_INK);
        mo_textc("BACK", 76, 111, MO_INK);
        mo_fill(93, 110, 27, 10, MO_INK);
        mo_textc("OK", 106, 111, MO_BG);
        cv_oy = 0;
        cv_blit(0, s * 120u);
    }
}

/* the sound designer's START: ENGINE START PAGES as the knob pages' dials, the engine twice the size under them, where
 * the sound starts and which pages follow; BACK (OCT-) outlined, DESIGN (OCT+) filled */
static void mo_dstart(void)
{
    char b[24], w[24];
    uint32_t st = designer_start(), s;
    memset(mo_col, 0, sizeof mo_col);
    mo_capture(0, "ENGINE", ENGINES[ds.eng]->name, "", VAL(0u), (int32_t)(eng_rank(ds.eng) * 1000u / (NENG_SHOWN - 1u)),
               ui.hot_t && ui.hot_col == 0u);
    mo_capture(1, "START", st == 0u ? "CURRENT" : st == 1u ? "INIT" : DS_GOAL[ds.goal], "", VAL(1u),
               (int32_t)(st * 1000u / DG_COUNT), ui.hot_t && ui.hot_col == 1u);
    mo_capture(2, "PAGES", ds.expert ? "ALL" : "MAIN", "", VAL(2u), ds.expert ? 1000 : 0, ui.hot_t && ui.hot_col == 2u);
    for (s = 0; s < 2u; s++) {
        cv_begin(240, s ? 120u : 102u, MO_BG);
        cv_oy = s ? -120 : -18;
        mo_cols();
        mo_ascii(b, ENGINES[ds.eng]->name, sizeof b);
        if (mo_tw(b) * 2 + 2 <= 116) mo_text_s(b, 60 - (mo_tw(b) * 2 + 1) / 2, 62, 2, MO_INK);
        else mo_textc(b, 60, 66, MO_INK);
        if (st > 1u) {
            str_cpy(w, DS_GOAL[ds.goal], sizeof w);
            str_cpy(w + str_len(w), " FROM INIT", sizeof w - str_len(w));
        } else str_cpy(w, st ? "FROM INIT" : "FROM THIS SOUND", sizeof w);
        mo_textc(w, 60, 80, MO_DIM);
        mo_textc(ds.expert ? "ALL PAGES" : "MAIN PAGES", 60, 90, MO_DIM);
        mo_box(2, 107, 32, 11, MO_DIM);
        mo_textc("BACK", 18, 109, MO_INK);
        mo_fill(117 - mo_tw("DESIGN") - 8, 107, mo_tw("DESIGN") + 8, 11, MO_INK);
        mo_text("DESIGN", 117 - mo_tw("DESIGN") - 4, 109, MO_BG);
        cv_oy = 0;
        cv_blit(0, s ? 120u : 18u);
    }
}

/* the mod shortcut's map (ui_modsc.c): a card over the dimmed page, 110 x 92 cells at (5, 16): a knob two lines, its
 * parameter and the amount, then the cord's source and a bar from the middle; the knob last turned inverted; at the
 * foot the source SELECT gives a new cord */
static void mo_msc(void)
{
    uint32_t tr = song.sel % NTRK, k;
    int32_t top;
    char b[24], v[12];
    for (top = 0; top < 184; top += 120) {
        cv_begin(220, top ? 64u : 120u, MO_BG);
        cv_oy = -top;
        mo_box(0, 0, 110, 92, MO_INK);
        mo_text("MOD AMOUNT", 4, 3, MO_INK);
        for (k = 0; k < 4u; k++) {
            int32_t y = 13 + (int32_t)k * 17, i, w;
            uint32_t dst = msc_dest(k);
            const pid_entry_t *e = help.pid != PID_NONE ? pid_find(help.pid) : 0;
            int16_t *vp;
            const param_desc_t *d = page_desc(ui.home ? stage_page() : cur_page(), k, &vp);
            mo_ascii(b, d && d->label && d->label[0] ? d->label : e ? e->name : "--", sizeof b);
            b[dst == CD_OFF ? 10 : 11] = 0;
            if (k == msc.knob && dst != CD_OFF) mo_inv(b, 4, y);
            else mo_text(b, 4, y, dst == CD_OFF ? MO_DIM : MO_INK);
            if (dst == CD_OFF) {
                mo_textr("NO MOD", 106, y, MO_DIM);
                continue;
            }
            mo_hl(44, y + 11, 62, MO_DIM);
            mo_vl(75, y + 9, 5, MO_DIM);
            if ((i = msc_find(dst, tr)) < 0) {
                mo_textr("0", 106, y, MO_DIM);
                continue;
            }
            mo_ascii(b, cord_src_name(cord_get((uint32_t)i)->src), sizeof b);
            b[7] = 0;
            mo_text(b, 4, y + 8, MO_DIM);
            w = cord_get((uint32_t)i)->amount * 30 / 100;
            if (w > 0) mo_fill(76, y + 10, w, 3, MO_INK);
            else if (w < 0) mo_fill(75 + w, y + 10, -w, 3, MO_INK);
            v[0] = '+';
            fmt_int(cord_get((uint32_t)i)->amount < 0 ? v : v + 1, cord_get((uint32_t)i)->amount);
            mo_textr(v, 106, y, MO_INK);
        }
        mo_hl(2, 82, 106, MO_DIM);
        str_cpy(b, "SELECT: ", sizeof b);
        mo_ascii(b + 8, cord_src_name(msc_src(tr)), sizeof b - 8u);
        mo_text(b, 4, 84, MO_DIM);
        cv_oy = 0;
        cv_blit(10, (uint32_t)(32 + top));
    }
}

/* the EDIT map (ui_sections.c smap): the page under the cursor named on the first line, then a row a section: its name
 * (the section open ink), a tile a page, four letters of its name where a tile holds them; the cursor's tile filled,
 * the page open a dot. Eleven rows at a time, round the cursor's */
static void mo_smap(uint32_t cur)
{
    uint8_t pg[8];
    uint32_t r, c, n, first = 0, s;
    char b[24], t[24];
    if (sec.n > 11u) first = smap.row > 5u ? smap.row - 5u : 0u;
    if (sec.n > 11u && first > sec.n - 11u) first = sec.n - 11u;
    n = smap_pages(smap.row, pg);
    b[0] = 0;
    if (sec.op[smap.row]) {
        mo_ascii(b, sec.name[smap.row], sizeof b);
        str_cpy(b + str_len(b), " ", 2);
    }
    if (n) {
        sec_short(pg[smap.col < n ? smap.col : 0u], t, sizeof t);
        mo_ascii(b + str_len(b), t, sizeof b - str_len(b));
    }
    for (s = 0; s < 2u; s++) {
        cv_begin(240, s ? 120u : 102u, MO_BG);
        cv_oy = s ? -120 : -18;
        b[19] = 0;
        mo_text(b, 2, 11, MO_INK);
        mo_hl(0, 19, 120, MO_DIM);
        for (r = first; r < sec.n && r < first + 11u; r++) {
            int32_t y = 22 + (int32_t)(r - first) * 8, w;
            n = smap_pages(r, pg);
            mo_ascii(t, sec.name[r], sizeof t);
            mo_cut(b, t, 5);
            mo_text(b, 2, y, r == cur ? MO_INK : MO_DIM);
            w = n ? (86 - (int32_t)(n - 1u) * 2) / (int32_t)n : 0;
            for (c = 0; c < n; c++) {
                int32_t x = 34 + (int32_t)c * (w + 2);
                int on = r == smap.row && c == smap.col, here = pg[c] == ui.page && r == cur;
                if (on) mo_fill(x, y - 1, w, 9, MO_INK);
                else mo_box(x, y - 1, w, 9, MO_DIM);
                if (!sec.op[r] && w >= 26) {
                    sec_short(pg[c], t, sizeof t);
                    mo_ascii(b, t, sizeof b);
                    if (str_len(b) > 4u) mo_abbr(t, b), str_cpy(b, t, sizeof b);
                    mo_textc(b, x + w / 2 + (here ? 2 : 0), y, on ? MO_BG : MO_DIM);
                }
                if (here) mo_fill(sec.op[r] || w < 26 ? x + w / 2 - 1 : x + 2, y + 2, 2, 3, on ? MO_BG : MO_INK);
            }
        }
        if (first) mo_textr("^", 112, 11, MO_DIM);
        if (first + 11u < sec.n) mo_textr("V", 119, 11, MO_DIM);
        cv_oy = 0;
        cv_blit(0, s ? 120u : 18u);
    }
}

/* under a sound designer stage's page (ui_designer.c designer_foot): BACK (OCT-) outlined, the sound, the next stage
 * (OCT+) filled */
static void mo_dfoot(const char *nm, const char *go)
{
    char b[24], t[24];
    int32_t gw;
    mo_ascii(t, go, sizeof t);
    gw = mo_tw(t) + 8;
    cv_begin(240, 240 - PV_SOUND_Y, MO_BG);
    mo_box(2, 0, 32, 10, MO_DIM);
    mo_textc("BACK", 18, 2, MO_INK);
    mo_ascii(b, nm, sizeof b);
    b[(uint32_t)(117 - gw - 38) / 6u] = 0;
    mo_text(b, 38, 2, MO_DIM);
    mo_fill(117 - gw, 0, gw, 10, MO_INK);
    mo_text(t, 117 - gw + 4, 2, MO_BG);
    cv_blit(0, PV_SOUND_Y);
}
