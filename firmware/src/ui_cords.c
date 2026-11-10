/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MODS (LFO family; MOD's place: its page hidden, ui.c page_visible; tessera-notes/docs/design/phase2/cords.png): the cords (cord.c)
 * four to a screen, as MOD's rows:
 * "2  [2] ENV -> [4] DECAY  -25 %" over the amount's bar from the middle; the cord KNOB 2 is on: a card. The knobs as
 * MOD's: KNOB 1 the source (--, then T1 LFO .. T4 NOTE), 2 the cord, 3 the destination (--, then T1 PITCH ..), 4 the
 * amount. A cord without a source is a fixed move: KNOB 4 its OFFSET. OCT+: a bipolar source's direction, +- (both
 * ways from the knob) or + (folded to 0..1: one way, "+" after its name), then GATE (">" after its name, Mono "^": all while the
 * source is over a threshold, none under it; OCT- held + KNOB 4 sets the threshold); the mod's own (cord.h uni), not
 * the LFO page's POL BI/UNI (cord.c cord_lfo_raw); then HOLD ("=" after its name: the source as +-, sampled once a
 * bar of the destination's track and held). A mod of mods (onto M1 AMT ..) adds to its mod's move, or with OCT- held +
 * KNOB 3 right SCALEs it ("M3*": times 0..1, a VCA; cord.c cord_vca_mask). Included by ui_pages.c. */

static int32_t accel(uint32_t role, int32_t s, int32_t range);                       /* ui_input.c */
static int32_t list_accel(uint32_t role, int32_t s, uint32_t total, uint32_t *fast);
static uint8_t cord_ui_sel;                             /* the cord KNOB 2 is on: 0 .. CORD_LIVE - 1 */

/* every frame (ui_input): a route of MOD's that came in (a project, a sound) into a cord (cord.c cord_adopt); no
 * room: MODS FULL, once for each route more left */
static void cu_adopt(void)
{
    static uint32_t was;
    uint32_t left = cord_adopt();
    if (left > was) ui_message("MODS FULL");
    if (left != was) ui.force = 1;
    was = left;
}

#define CU_SPT CS_ENERGY                                /* a track's sources (LFO .. OCHD8, then EOC, added later) */
#define CU_NSRC (1u + NTRK * CU_SPT + (CS_EOC - CS_ENERGY))   /* KNOB 1's list: --, track by track, then the project's
                                                         * (ENERGY, the MACROs) once */
static int cu_src_trk(uint32_t s) { return s && !CS_PROJECT(s); }   /* a track's source, not the project's */
/* KNOB 3's list: --, a track's destinations track by track (none, the retired one: not listed), then the send
 * effects' once (CK_GLOBAL: every track's) */
static int cu_global(uint32_t code) { return cord_dest(code) && cord_dest(code)->kind >= CK_GLOBAL; }   /* (CK_CORD too) */
static uint32_t cu_count(uint32_t below, int global)    /* the listed codes 1 .. below - 1 of that kind */
{
    uint32_t code, n = 0;
    for (code = CD_OFF + 1u; code < below; code++)
        n += cord_dest(code) && cu_global(code) == global;
    return n;
}
#define CU_DPT cu_count(CD_N, 0)
#define CU_NDST (1u + NTRK * CU_DPT + cu_count(CD_N, 1))
static uint32_t cu_src_of(const cord_t *c)
{
    if (!c->src) return 0u;
    return cu_src_trk(c->src) ? 1u + c->src_trk * CU_SPT + (c->src == CS_EOC ? CS_ENERGY - 1u : c->src - 1u) : 1u + NTRK * CU_SPT + c->src - CS_ENERGY;
}
static uint32_t cu_src_at(uint32_t v, uint8_t *tr)     /* entry v of KNOB 1's list: its source, *tr its track */
{
    *tr = 0;
    if (!v) return CS_OFF;
    v--;
    if (v >= NTRK * CU_SPT) return CS_ENERGY + v - NTRK * CU_SPT;
    *tr = (uint8_t)(v / CU_SPT);
    return v % CU_SPT == CS_ENERGY - 1u ? (uint32_t)CS_EOC : 1u + v % CU_SPT;
}
static uint32_t cu_dst_k(uint32_t code) { return cu_count(code, cu_global(code)); }   /* code -> its place, in its kind */
static uint32_t cu_dst_code(uint32_t k, int global)
{
    uint32_t code;
    for (code = CD_OFF + 1u; code < CD_N; code++)
        if (cord_dest(code) && cu_global(code) == global && !k--) return code;
    return CD_OFF;
}
static uint32_t cu_dst_of(const cord_t *c)
{
    if (!c->dst) return 0u;
    return cu_global(c->dst) ? 1u + NTRK * CU_DPT + cu_dst_k(c->dst) : 1u + c->dst_trk * CU_DPT + cu_dst_k(c->dst);
}
/* KNOB 3's entry v (1 ..) into c: its destination and track */
static void cu_dst_set(cord_t *c, uint32_t v)
{
    uint32_t n = NTRK * CU_DPT;
    c->dst = (uint8_t)(v - 1u < n ? cu_dst_code((v - 1u) % CU_DPT, 0) : cu_dst_code(v - 1u - n, 1));
    c->dst_trk = (uint8_t)(v - 1u < n ? (v - 1u) / CU_DPT : 0u);
}

/* the source's name, "+" after it when UNI folds it (cord.c cord_fold_on) */
static const char *cu_src_name(const cord_t *c)
{
    static char b[12];
    if (!c->src) return "--";
    str_cpy(b, cord_src_name(c->src), sizeof b - 1u);
    if (cord_fold_on(c)) str_cpy(b + str_len(b), "+", 2);
    else if (cord_gate(c)) str_cpy(b + str_len(b), LOOK(">", "^"), 2);   /* (Mono: its arrow is a ">") */
    else if (c->uni == CU_HOLD) str_cpy(b + str_len(b), "=", 2);
    return b;
}

/* a destination's name: E1..E8 by the destination track's engine (as MOD names them) */
static const char *cu_dst_name(const cord_t *c)
{
    if (c->dst == CD_CC && c >= cord_get(0) && c < cord_get(0) + CORD_LIVE) {   /* CC OUT: "CC 74", its number */
        static char b[8];
        str_cpy(b, "CC ", sizeof b);
        fmt_int(b + 3, (int32_t)cord_cc_num((uint32_t)(c - cord_get(0))));
        return b;
    }
    if (c->dst >= CD_E1 && c->dst <= CD_E8)
        return mod_dst_name(&trk[c->dst_trk % NTRK], MD_E1 + (c->dst - CD_E1));
    return cord_dest(c->dst) ? cord_dest(c->dst)->name : "--";
}

/* Mono's four characters for a destination: a mod's move "M3%" (M3 AMT: Mono's abbreviation would read "M3MT"), else
 * the name, abbreviated where it is drawn */
static const char *cu_dst_mono(const cord_t *c)
{
    static char b[8];
    if (c->dst == CD_CC) {                              /* (CC OUT: "CC74", three digits "C119") */
        const char *n = cu_dst_name(c) + 3;
        b[0] = 'C'; b[1] = 'C';
        str_cpy(b + (str_len(n) > 2u ? 1 : 2), n, 4);
        return b;
    }
    if (c->dst < CD_M1_AMT || c->dst > CD_M16_AMT) return cu_dst_name(c);
    b[0] = 'M';
    fmt_int(b + 1, (int32_t)(c->dst - CD_M1_AMT + 1u));
    str_cpy(b + str_len(b), cord_vca_mask() >> (uint32_t)(c - cord_get(0)) & 1u ? "*" : "%", 2);   /* (SCALE: "M3*",
                                                         * times; c: one of cords[], as every row's) */
    return b;
}

/* the move's percentage as "+40 %" (an amount, or a fixed cord's offset) */
static int32_t cu_pct(const cord_t *c) { return c->src ? c->amount : c->offset; }
/* the move now (the bar): as a mod of mods turns it (cord.c cord_move), else as set */
static int32_t cu_now(const cord_t *c) { return cord_move((uint32_t)(c - cord_get(0))); }
static void cu_pct_text(char *b, int32_t v)
{
    char n[8];
    fmt_int(n, v < 0 ? -v : v);
    b[0] = v < 0 ? '-' : v > 0 ? '+' : ' ';
    str_cpy(b + 1, n, 8);
    str_cpy(b + str_len(b), " %", 3);
}

/* a chip: the track's number on its colour, then the name */
static void cu_chip(int32_t x, int32_t y, int32_t w, uint32_t tr, const char *s, int on, int has_trk, uint16_t bg)
{
    uint16_t fill = on ? ux_mix(bg, T_THEME, 30) : ux_mix(bg, T_LINE, 50);
    char num[2] = {(char)('1' + (tr & 3u)), 0};
    cv_rrect(x, y, w, 18, 5, fill, bg);
    if (has_trk) {
        cv_rrect(x + 3, y + 3, 12, 12, 3, on ? T_TRK(tr) : T_DIM, fill);
        cv_text_on(x + 6, y + 4, &AF_X, num, T_BG, on ? T_TRK(tr) : T_DIM);
        x += 14;
        w -= 14;
    }
    cv_free_text(x + 4 + (w - 8 - (text_w(&AF_X, s) < w - 8 ? text_w(&AF_X, s) : w - 8)) / 2, y + 4, &AF_X, s,
                 on ? T_TEXT : T_DIM, fill, w - 8);
}

/* MOD SCOPE (gap G14; mscope.c samples, ui_mono.c mo_scope draws): OCT- held on MODS shows the picked mod's move over
 * time under its row, in the other three rows' place; letting go, the rows again. 1 while shown (cu_scope_draw) */
static uint8_t cu_scope;
static uint8_t cu_thr_t;                                /* frames left of a GATE's threshold drawn across (KNOB 4) */
/* the cord shown in row k of four: the screenful, or with the scope under it the picked one alone; CORD_LIVE none */
static uint32_t cu_slot(uint32_t k)
{
    if (cu_scope) return k ? CORD_LIVE : cord_ui_sel;
    return cord_ui_sel / 4u * 4u + k;
}

#define CU_LED_X 9                                      /* 8 x 8 at (9, a row's y + 28) */
#define CU_LED_Y 28
#define CU_BAR_X 24                                     /* the move: 210 x 9 at (24, a row's y + 27), the bar 206 long */
#define CU_BAR_W 210
#define CU_BAR_Y 27
/* cord i's light and move now, as cu_leds keeps them: its key (the light's level 0..8 in the low four bits), *s its
 * share, *bk where its mark is */
static uint8_t cu_state(uint32_t i, const cord_t *c, int32_t *s, uint8_t *bk)
{
    int32_t v = *s = cord_live(c) ? cord_share(c) : 0;
    uint8_t lv = (uint8_t)(cord_live(c) ? 1 + clamp((v < 0 ? -v : v) * 7 / 32768, 0, 7) : 0);
    *bk = (uint8_t)(clamp(v, -32768, 32768) * 103 / 32768 + 104);   /* (the mark's x, 1 .. 207) */
    return (uint8_t)(lv | (i == cord_ui_sel) << 4 | (ux.pal & 7u) << 5);
}
static uint8_t cu_led_was[4];                           /* (cu_leds: the step each light shows; a row redrawn: none) */
static uint8_t cu_bar_was[4];                           /* (and where each row's live mark is) */
static int8_t cu_mv_was[4];                             /* (and its move now: a mod of mods turns it) */
static uint8_t cu_rows_sent;                            /* (Mono: cu_rows sent the rows, their moves and lights in them) */
#define CU_Y 22                                         /* the rows: 48 px each from y 22 (as MOD's), the chips at 220 */
#define CU_H 48
static void cu_rows(void)
{
    static uint32_t sig;
    uint32_t k, sg = cord_ui_sel * 7u + ux.gen * 977u + ux.pal * 31u + cu_scope * 7919u;
    for (k = 0; k < CORD_LIVE; k++) {
        const uint8_t *b = (const uint8_t *)cord_get(k);
        uint32_t i;
        for (i = 0; i < sizeof(cord_t); i++)
            sg = (sg ^ b[i]) * 16777619u;
        sg = (sg ^ cord_cc_num(k)) * 16777619u;         /* (a CC mod's number: "CC 74") */
    }
    for (k = 0; k < NTRK; k++)
        sg = (sg ^ trk[k].engine) * 16777619u;          /* (E1..E8's names) */
    if (!ui.force && sg == sig)
        return;
    sig = sg;
    memset(cu_led_was, 0xFF, sizeof cu_led_was);        /* (the rows over their lights and moves: cu_leds again) */
    memset(cu_bar_was, 0xFF, sizeof cu_bar_was);
    for (k = 0; k < 4u; k++) {
        uint32_t i = cu_slot(k);
        const cord_t *c = cord_get(i);
        int sel = i == cord_ui_sel, on;
        if (i >= CORD_LIVE) continue;
        on = cord_live(c);
        uint16_t bg = sel ? T_LIFT : T_BG;
        int32_t a = cu_pct(c);
        char b[16];
#if MELODEE_LOOK
        mo_cu_row(i + 1u, c, sel, on);
        {                                               /* its move and light in the row, sent with it once: not the
                                                         * row blank under them, then they (all four flickered as
                                                         * KNOB 2 scrolled) */
            int32_t s;
            uint8_t bk;
            cu_led_was[k] = cu_state(i, c, &s, &bk);
            cu_bar_was[k] = bk;
            cu_mv_was[k] = (int8_t)cu_now(c);
            cv_oy = CU_BAR_Y;
            mo_cu_bar(c, s, CU_BAR_X);
            cv_oy = CU_LED_Y;
            mo_cu_led(cu_led_was[k] & 15u, CU_LED_X);
            cv_oy = 0;
        }
        cv_blit(0, (uint32_t)(CU_Y + (int32_t)k * CU_H));
        cu_rows_sent = 1;
        continue;
#endif
        cv_begin(240, CU_H, T_BG);
        if (sel) {
            cv_rrect(4, 1, 232, CU_H - 4, 7, T_THEME, T_BG);
            cv_rrect(5, 2, 230, CU_H - 6, 6, bg, T_THEME);
        }
        fmt_int(b, (int32_t)(i + 1u));
        cv_text_on(i + 1u >= 10u ? 8 : 12, 7, &AF_S, b, sel ? T_THEME : T_MID, bg);
        cu_chip(26, 6, 74, c->src_trk, cu_src_name(c), c->src != CS_OFF, cu_src_trk(c->src), bg);
        cv_rect(104, 14, 10, 2, on ? T_MID : T_DIM);    /* an arrow */
        cv_line(110, 11, 113, 14, on ? T_MID : T_DIM);
        cv_line(110, 17, 113, 14, on ? T_MID : T_DIM);
        cu_chip(118, 6, 74, c->dst_trk, cu_dst_name(c), c->dst != CD_OFF, c->dst != CD_OFF && !cu_global(c->dst), bg);
        cu_pct_text(b, a);
        cv_text_r(232, 9, &AF_X, b, on ? (sel ? T_THEME : T_SEC) : T_DIM, bg);
        cv_blit(0, (uint32_t)(CU_Y + (int32_t)k * CU_H));   /* (its move: cu_leds) */
    }
}

/* a row's activity light under its number (tessera-notes/docs/design/phase2/cords.png): how far the cord moves its destination
 * now (cord.c cord_share: the source by the amount, and the offset), in 8 steps of the theme's colour; and its move,
 * the bar from the middle (its AMOUNT, or OFFSET; a +- mod's both ways), with a mark where it moves the destination
 * now. Each its own small
 * canvas, sent when it changes */
static void cu_bar(uint32_t k, const cord_t *c, int sel, int32_t s)
{
    uint16_t bg = sel ? T_LIFT : T_BG;
    int on = cord_live(c);
    int32_t a = cu_now(c), m = 104, x = 2 + (a + 100) * 206 / 200, lx = m + clamp(s, -32768, 32768) * 103 / 32768;
    int32_t d = x < m ? m - x : x - m;
#if MELODEE_LOOK
    cv_begin(CU_BAR_W, 9, T_BG);                        /* (Mono: MO_BG) */
    mo_cu_bar(c, s, 0);
    cv_blit(CU_BAR_X, (uint32_t)(CU_Y + (int32_t)k * CU_H + CU_BAR_Y));
    return;
#endif
    cv_begin(CU_BAR_W, 9, bg);
    cv_rect(2, 3, 206, 3, T_LINE);
    if (a)                                              /* (+-: both ways, as mo_cu_bar) */
        cv_rect(cord_bip(c) ? m - d : x < m ? x : m, 3, cord_bip(c) ? 2 * d : d, 3,
                on ? (sel ? T_THEME : ux_mix(bg, T_THEME, 60)) : T_DIM);
    cv_rect(m, 1, 1, 7, T_MID);
    if (on && c->src)                                   /* (a fixed OFFSET: no mark, the bar says it) */
        cv_rect(lx - 1, 0, 3, 9, T_TEXT);
    cv_blit(CU_BAR_X, (uint32_t)(CU_Y + (int32_t)k * CU_H + CU_BAR_Y));
}
static void cu_leds(void)
{
    uint8_t *was = cu_led_was;
    uint32_t k;
    if (cu_rows_sent) {                                 /* (Mono: in the rows just sent. Not again: a mark moved on
                                                         * since went out twice, the selected row flickered as KNOB 4
                                                         * turned its AMOUNT, 3.5-beta5) */
        cu_rows_sent = 0;
        return;
    }
    for (k = 0; k < 4u; k++) {
        uint32_t i = cu_slot(k);
        const cord_t *c = cord_get(i);
        if (i >= CORD_LIVE) continue;
        int32_t s;
        uint8_t bk, key = cu_state(i, c, &s, &bk), lv = key & 15u;
        uint16_t bg = i == cord_ui_sel ? T_LIFT : T_BG;
        if (ui.force || bk != cu_bar_was[k] || cu_now(c) != cu_mv_was[k]) {
            cu_bar_was[k] = bk;
            cu_mv_was[k] = (int8_t)cu_now(c);
            cu_bar(k, c, i == cord_ui_sel, s);
        }
        if (!ui.force && key == was[k])
            continue;
        was[k] = key;
#if MELODEE_LOOK
        cv_begin(8, 8, T_BG);
        mo_cu_led(lv, 0);
#else
        cv_begin(8, 8, bg);
        if (lv)
            cv_disc(4, 4, 6, lv > 1 ? ux_mix(T_DIM, T_THEME, (lv - 1) * 100 / 7) : T_DIM, 0);   /* (T_DIM: on either row) */
#endif
        cv_blit(CU_LED_X, (uint32_t)(CU_Y + (int32_t)k * CU_H + CU_LED_Y));
    }
}

/* every frame on MODS, before the rows: the picked mod sampled (mscope.c; off MODS not), and while OCT- is held
 * the scope under its row, drawn when a column came in */
static void cu_scope_draw(int held)                     /* held: OCT- held, nothing over the page (ui_pages.c) */
{
    static uint32_t was;
    const cord_t *c = cord_get(cord_ui_sel);
    uint32_t key;
    mscope_take(cord_ui_sel);
    cu_scope = (uint8_t)(MELODEE_LOOK && held);
    cu_thr_t -= (uint8_t)(cu_thr_t != 0u);
#if MELODEE_LOOK
    key = mscope.col | (uint32_t)mscope.bars << 8 | (uint32_t)cord_ui_sel << 16 | (ux.pal & 7u) << 21 |
          (uint32_t)mscope.full << 24 | (uint32_t)cu_scope << 25 | (ux.gen & 15u) << 26;
    if (cord_gate(c))                                   /* (its threshold, shut or open, across or the wedges) */
        key = key * 31u + (uint8_t)c->offset * 4u + (uint32_t)cord_gate_on(c) * 2u + (cu_thr_t != 0u);
    if (!cu_scope || (!ui.force && key == was)) {
        was = cu_scope ? key : 0u;
        return;
    }
    was = key;
    cv_begin(240, 124, T_BG);                           /* (the canvas's most, gfx.c CV_MAX: 124 of the rows' 144, */
    mo_scope(2, 58, cu_src_trk(c->src) ? (int32_t)c->src_trk : -1, cu_thr_t != 0u);   /* centred) */
    cv_blit(0, (uint32_t)(CU_Y + CU_H + 10));
#else
    (void)was, (void)key, (void)c;
#endif
}

/* the chips under the rows (ui_draw.c draw_columns): KNOB 1 the source, 2 the cord, 3 the destination, 4 the move */
static void cu_columns(void)
{
    const cord_t *c = cord_get(cord_ui_sel);
    char val[24], src[16];
    if (c->src && !cu_src_trk(c->src))                  /* (the project's: no track) */
        str_cpy(src, cord_src_name(c->src), sizeof src);
    else if (c->src) {
        src[0] = 'T'; src[1] = (char)('1' + c->src_trk); src[2] = ' ';
        str_cpy(src + 3, cord_src_name(c->src), sizeof src - 3);
    } else
        str_cpy(src, "--", sizeof src);
    draw_column(0, "", src, c->src ? (cord_bip(c) ? "+-" : "+") : "",   /* (the bar: both ways, or one) */
                c->src ? VAL(0u) : T_DIM, -1, ICON_NONE);
    fmt_int(val, (int32_t)cord_ui_sel + 1);
    draw_column(1, "MOD", val, "/16", VAL(1u), (int32_t)cord_ui_sel * 1000 / (CORD_LIVE - 1u), ICON_NONE);
    if (c->dst && cu_global(c->dst))
        str_cpy(val, LOOK(cu_dst_name(c), cu_dst_mono(c)), sizeof val);
    else if (c->dst) {
        val[0] = 'T'; val[1] = (char)('1' + c->dst_trk); val[2] = ' ';
        str_cpy(val + 3, cu_dst_name(c), sizeof val - 3);
    } else
        str_cpy(val, "--", sizeof val);
    draw_column(2, "", val, "", c->dst ? VAL(2u) : T_DIM, -1, ICON_NONE);
    cu_pct_text(val, cu_pct(c));
    draw_column(3, c->src ? "" : "OFS", val, "", cu_pct(c) ? VAL(3u) : T_DIM, (cu_pct(c) + 100) * 1000 / 200, ICON_NONE);
}

/* KNOB 1's and 3's lists for the picker (ui_popup.c): their length and the cord's entry; 0: no list on that knob */
static int cu_pick(uint32_t col, int32_t *count, int32_t *cur)
{
    const cord_t *c = cord_get(cord_ui_sel);
    if (col != 0u && col != 2u)
        return 0;
    *count = (int32_t)(col ? CU_NDST : CU_NSRC);
    *cur = (int32_t)(col ? cu_dst_of(c) : cu_src_of(c));
    return 1;
}
/* entry i of a list: its name, its track (-1: none, the "--" first; the project's ENERGY and send effects) */
static const char *cu_pick_name(uint32_t col, int32_t i, int32_t *tr)
{
    cord_t c = {0};
    *tr = -1;
    if (i <= 0)
        return "--";
    if (col == 0u) {
        uint8_t t;
        uint32_t s = cu_src_at((uint32_t)i, &t);
        if (cu_src_trk(s)) *tr = t;                     /* (ENERGY, the MACROs: the project's) */
        return cord_src_name(s);
    }
    cu_dst_set(&c, (uint32_t)i);
    *tr = cu_global(c.dst) ? -1 : c.dst_trk;
    return cu_dst_name(&c);
}

/* entry i's label in Mono's list (ui_mono.c mo_pick): "T1 SLEW", as KNOB 1's and 3's values ("T1 LFO", the rows'
 * codes T1SL); the project's without a track. b: 20 bytes */
static void cu_pick_label(char *b, uint32_t col, int32_t i, int32_t *tr)
{
    const char *s = cu_pick_name(col, i, tr);
    b[0] = 0;
    if (*tr >= 0)
        b[0] = 'T', b[1] = (char)('1' + (*tr & 3)), b[2] = ' ', b[3] = 0;
    str_cpy(b + str_len(b), s, 20u - str_len(b));
}

/* OCT+ on MODS: how the cord reads its source, +- -> + -> GATE -> HOLD -> +- (a source that is 0..1 already: + ->
 * GATE -> HOLD -> +). To
 * GATE: the threshold (OFFSET) starts at the middle of a bipolar source, at 50 % of a 0..1 one */
static void cu_polarity(void)
{
    cord_t c = *cord_get(cord_ui_sel);
    int bip;
    if (!c.src) {
        ui_message("NO SOURCE");
        return;
    }
    bip = cord_bipolar(c.src, c.src_trk);
    c.uni = c.uni == CU_HOLD ? (bip ? CU_BI : CU_UNI) : c.uni == CU_GATE ? CU_HOLD : c.uni == CU_UNI || !bip ? CU_GATE : CU_UNI;
    c.offset = c.uni == CU_GATE ? (bip ? 0 : 50) : 0;
    (void)cord_set(cord_ui_sel, &c);
    if (c.uni == CU_GATE) {
        char b[24];
        str_cpy(b, "GATE OVER ", sizeof b);
        cu_pct_text(b + str_len(b), c.offset);
        ui_message(b);
    } else
        ui_message(c.uni == CU_HOLD ? "HOLD A BAR" : c.uni ? "+ ONE WAY" : "+- BOTH WAYS");   /* (HOLD: OCT- held +
                                                         * KNOB 1 its clock, HOLD ON T1 NOTES ..) */
    ui.force = 1;
}
/* MODS' sheet, Clear mod: the picked mod back to empty (no source, destination or move, +-; not a SCALE, CC 1). Mods
 * that move it (M AMT) stay. Nothing set: NOTHING THERE */
static void cu_clear(void)
{
    static const cord_t none;                           /* (CS_OFF, CD_OFF, 0, 0, CU_BI) */
    const cord_t *c = cord_get(cord_ui_sel);
    if (!c->src && !c->dst && !c->amount && !c->offset) {
        ui_message("NOTHING THERE");
        return;
    }
    (void)cord_set(cord_ui_sel, &none);
    cord_vca_load(cord_vca_mask() & ~(1u << cord_ui_sel));
    cord_cc_set(cord_ui_sel, 1u);
    ui_message("MOD CLEARED");
    ui.force = 1;
}
/* OCT- held + KNOB 4 on a GATE mod: its threshold (ui_input.c: OCT-'s tap eaten) */
static int cu_thresh_knob(int32_t steps)
{
    cord_t c = *cord_get(cord_ui_sel);
    char b[24];
    if (!cord_gate(&c)) return 0;
    c.offset = (int8_t)clamp(c.offset + accel(EN_K1 + 3u, steps, 200), cord_bipolar(c.src, c.src_trk) ? -100 : 0, 100);
    (void)cord_set(cord_ui_sel, &c);
    str_cpy(b, "GATE OVER ", sizeof b);
    cu_pct_text(b + str_len(b), c.offset);
    ui_message(b);
    cu_thr_t = 64;                                      /* (about a second, then the wedges at the edges again) */
    return 1;
}

/* OCT- held + KNOB 3 on a CC mod: its CC number, 0..119 (ui_input.c: OCT-'s tap eaten; KNOB 4 a GATE's threshold) */
static int cu_cc_knob(int32_t steps)
{
    const cord_t *c = cord_get(cord_ui_sel);
    char b[12];
    if (c->dst != CD_CC) return 0;
    cord_cc_set(cord_ui_sel, (uint32_t)clamp((int32_t)cord_cc_num(cord_ui_sel) + accel(EN_K1 + 2u, steps, 120), 0, 119));
    str_cpy(b, "CC ", sizeof b);
    fmt_int(b + 3, (int32_t)cord_cc_num(cord_ui_sel));
    ui_message(b);                                      /* (no ui.force: the row redraws by its signature, which
                                                         * holds the CC numbers; a whole frame flickered under the list) */
    return 1;
}

/* OCT- held + KNOB 1 on a HOLD mod: its clock, BAR (its destination track's) or T1 .. T4's notes (cord.c cord_hold_clk;
 * ui_input.c: OCT-'s tap eaten). Another mod: KNOB 1 its source, as without OCT- */
static __attribute__((noinline)) int cu_hclk_knob(int32_t steps)
{
    char b[20];
    uint32_t v;
    if (cord_get(cord_ui_sel)->uni != CU_HOLD || !steps) return 0;
    v = (uint32_t)clamp((int32_t)cord_hold_clk(cord_ui_sel) + (steps > 0 ? 1 : -1), 0, 4);
    cord_hold_clk_set(cord_ui_sel, v);
    if (!v) str_cpy(b, "HOLD A BAR", sizeof b);
    else {
        str_cpy(b, "HOLD ON T1 NOTES", sizeof b);
        b[9] = (char)('0' + v);
    }
    ui_message(b);
    return 1;
}

/* OCT- held + KNOB 3 on a mod of mods: ADD (left; its share added to the mod's move) or SCALE (right; the move times
 * it, a VCA) (ui_input.c: OCT-'s tap eaten; a CC mod's KNOB 3 its number) */
static __attribute__((noinline)) int cu_vca_knob(int32_t steps)
{
    uint32_t bit = 1u << cord_ui_sel;
    if ((uint32_t)(cord_get(cord_ui_sel)->dst - CD_M1_AMT) >= CORD_LIVE) return 0;
    cord_vca_load(steps > 0 ? cord_vca_mask() | bit : cord_vca_mask() & ~bit);
    ui_message(steps > 0 ? "SCALES IT" : "ADDS TO IT");
    ui.force = 1;
    return 1;
}

/* a knob on CORDS (ui_input.c edit_param) */
static void cu_knob(uint32_t slot, int32_t steps)
{
    cord_t c = *cord_get(cord_ui_sel);
    int32_t v;
    if (slot == 1u) {                                   /* (no ui.force: the rows, their lights and the chips redraw
                                                         * by their own signatures; a whole frame flickered) */
        cord_ui_sel = (uint8_t)clamp((int32_t)cord_ui_sel + (steps > 0 ? 1 : -1), 0, CORD_LIVE - 1);
        return;
    }
    if (slot == 0u) {
        v = clamp((int32_t)cu_src_of(&c) + list_accel(EN_K1, steps, CU_NSRC, 0), 0, CU_NSRC - 1);
        c.src = (uint8_t)cu_src_at((uint32_t)v, &c.src_trk);
        if (c.uni >= CU_GATE && !c.src) { c.uni = CU_BI; c.offset = 0; }   /* (no source: no gate, no threshold; */
        if (c.uni == CU_GATE && c.offset < 0 && !cord_bipolar(c.src, c.src_trk)) c.offset = 0;   /* 0..1: none under 0) */
        if (c.src && !c.amount && c.uni != CU_GATE) { c.amount = c.offset; c.offset = 0; }   /* (one move shown: a fixed move becomes */
        if (!c.src && c.amount) { c.offset = c.amount; c.amount = 0; }   /* the source's, and back) */
    } else if (slot == 2u) {
        int was = c.dst != CD_OFF;
        v = clamp((int32_t)cu_dst_of(&c) + list_accel(EN_K1 + 2u, steps, CU_NDST, 0), 0, CU_NDST - 1);
        if (v) cu_dst_set(&c, (uint32_t)v);
        else c.dst = CD_OFF, c.dst_trk = 0u;
        if (!was && c.dst && !c.amount && !c.offset) {  /* (connected: a move to hear at once) */
            if (c.src) c.amount = 50;
            else c.offset = 25;
        }
    } else {
        v = clamp(cu_pct(&c) + accel(EN_K1 + 3u, steps, 200), -100, 100);
        if (c.src) c.amount = (int8_t)v;
        else c.offset = (int8_t)v;
    }
    (void)cord_set(cord_ui_sel, &c);
    if (slot == 3u)                                     /* (SOURCE, DESTINATION: their list over the page, which */
        ui.force = 1;                                   /* stays as it is under it: the chips alone, cu_under) */
}
