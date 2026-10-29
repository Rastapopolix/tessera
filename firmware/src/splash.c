/* SPDX-License-Identifier: GPL-3.0-only */
/* The boot screen: Tessera's mark, a 3 x 3 mosaic of lapis tiles with one gold tessera, over TESSERA and a line.
 * While the firmware starts, the mosaic's bed shows (the nine places in the grout's colour); when it is ready,
 * splash_lay() sets the tiles from the middle outwards, one every SPL_STEP_MS, the gold one last with a press,
 * in the wait the boot already makes before the first frame, then holds the finished mark a moment (SPL_HOLD_MS;
 * a button or key pressed ends it sooner). The mark's colours are fixed (it is the logo),
 * the rest the palette's; the Mono look: outlined tiles in the ink, the gold one solid. Drawn with lcd_fill and draw_text_box only. */

#define SPL_U 22                                /* a tile, px */
#define SPL_G 4                                 /* the grout between two */
#define SPL_CX 120
#define SPL_CY 78                               /* the mosaic's centre */
#define SPL_STEP_MS 60u
#define SPL_HOLD_MS 800u                        /* the finished mark held (a press ends it sooner) */
#define SPL_GOLD 2u                             /* the gold tessera: top right */
#ifndef SPL_FRAME_HOOK
#define SPL_FRAME_HOOK(f) ((void)0)             /* (tests: each frame of the outro) */
#endif
#if !MELODEE_LOOK
static const uint16_t SPL_LAPIS[9] = {RGB(44, 64, 120), RGB(58, 84, 150), 0, RGB(66, 96, 170), RGB(50, 74, 136),
                                      RGB(40, 60, 112), RGB(62, 90, 160), RGB(46, 68, 126), RGB(54, 80, 146)};
#define SPL_GOLD_C RGB(236, 184, 72)
#else
#define SPL_GOLD_C T_TEXT                       /* (Mono: the ink) */
#endif
/* a hand-set look without rotation: each tile a pixel off the grid */
static const int8_t SPL_JIT[9][2] = {{0, 1}, {1, -1}, {-1, 0}, {1, 0}, {0, 0}, {-1, 1}, {0, -1}, {1, 1}, {-1, -1}};
static const uint8_t SPL_ORDER[9] = {4, 1, 3, 5, 7, 0, 6, 8, 2};   /* from the middle outwards, the gold one last */

static void spl_tile(uint32_t i, int32_t grow, uint16_t c)
{
    int32_t x = SPL_CX - SPL_U / 2 + ((int32_t)(i % 3u) - 1) * (SPL_U + SPL_G) + SPL_JIT[i][0] - grow;
    int32_t y = SPL_CY - SPL_U / 2 + ((int32_t)(i / 3u) - 1) * (SPL_U + SPL_G) + SPL_JIT[i][1] - grow;
    lcd_fill((uint32_t)x, (uint32_t)y, (uint32_t)(SPL_U + 2 * grow), (uint32_t)(SPL_U + 2 * grow), c);
#if MELODEE_LOOK                                /* Mono (the site's logo): the tiles outlined in the ink, 2 px, the
                                                 * gold one solid */
    if (i != SPL_GOLD && c != T_BG)
        lcd_fill((uint32_t)(x + 2), (uint32_t)(y + 2), (uint32_t)(SPL_U + 2 * grow - 4), (uint32_t)(SPL_U + 2 * grow - 4),
                 T_BG);
#endif
}

#if MELODEE_LOOK
static const char *spl_line;                    /* splash()'s line (the outro sweeps it too) */
#endif
static void splash(const char *line)          /* the boot screen: the mosaic's bed, TESSERA, then a line */
{
    uint32_t i;
#if MELODEE_LOOK
    spl_line = line;
#endif
    lcd_fill(0, 0, 240, 240, T_BG);
    for (i = 0; i < 9u; i++)
        spl_tile(i, 0, T_LINE);
    draw_text_box(0, 120, 240, &AF_L, "TESSERA", T_TEXT, 1);
    draw_text_box(0, 160, 240, &AF_S, line, T_MID, 1);
}

static void spl_wait(uint32_t ms)              /* (the boot's wait, the watchdog fed) */
{
    while (ms--) {
        fm1_delay_ms(1);
        if (!(ms & 31u)) fm1_wdt_feed();
    }
}

/* the finished mark held ms, the watchdog fed; a button or key pressed since (not one held from power-on) ends it
 * (returns 1) */
static int spl_hold(uint32_t ms)
{
    uint32_t held = fm1_in.buttons | fm1_in.notes;
    while (ms--) {
        fm1_delay_ms(1);
        if (!(ms & 31u)) fm1_wdt_feed();
        if ((fm1_in.buttons | fm1_in.notes) & ~held) return 1;
    }
    return 0;
}

#if MELODEE_LOOK
/* The Mono outro: the held mark breaks into grains of 2 px (a cell of the mark's grid is one when half of it is
 * ink), they shiver loose, then a wind from the left sweeps them off, column by column, each grain speeding up,
 * drifting and dimming to a pixel. Nothing kept per grain: frame f is drawn from the mark itself (spl_ink), in the
 * band the mark covers, through the canvas (the diff sends what changed: the SPI sets the pace, ~40 ms a frame). */
#define SPL_Y0 36                               /* the band: rows SPL_Y0 .. SPL_Y0 + 124 + SPL_H1 - 1 */
#define SPL_H1 20u                              /* (its foot, on the small canvas) */
#define SPL_LOOSE 2                             /* frames the grains shiver before the wind */
#define SPL_FRONT 20                            /* the wind's front, px a frame */
#define SPL_LIFE 8                              /* frames a grain flies */
#define SPL_FRAME_MS 30u                        /* (at least) */
static uint16_t spl_ink(int32_t x, int32_t y)   /* the mark's colour at x, y (0: none) */
{
    uint32_t i, n;
    int32_t t;
    for (i = 0; i < 9u; i++) {
        int32_t tx = x - (SPL_CX - SPL_U / 2 + ((int32_t)(i % 3u) - 1) * (SPL_U + SPL_G) + SPL_JIT[i][0]);
        int32_t ty = y - (SPL_CY - SPL_U / 2 + ((int32_t)(i / 3u) - 1) * (SPL_U + SPL_G) + SPL_JIT[i][1]);
        if (tx >= 0 && ty >= 0 && tx < SPL_U && ty < SPL_U)
            return i == SPL_GOLD || tx < 2 || ty < 2 || tx >= SPL_U - 2 || ty >= SPL_U - 2 ? T_TEXT : 0;
    }
    t = x - 36;                                 /* TESSERA (AF_L: font pixels of 4, centred at row 120) */
    if (y >= 124 && y < 152 && t >= 0 && t < 168 && t % 24 < 20)
        return MO_FONT["TESSERA"[t / 24] - 32][(y - 124) / 4] & (16u >> (t % 24 / 4)) ? T_TEXT : 0;
    n = str_len(spl_line);                      /* the line (AF_S: of 2, centred at row 160) */
    t = x - (240 - 12 * (int32_t)n) / 2;
    if (y >= 160 && y < 174 && t >= 0 && t < 12 * (int32_t)n && t % 12 < 10)
        return MO_FONT[mono_ch((uint8_t)spl_line[t / 12]) - 32u][(y - 160) / 2] & (16u >> (t % 12 / 2)) ? T_MID : 0;
    return 0;
}
static uint32_t spl_frame(int32_t f)            /* frame f of the outro, band by band; the grains still flying */
{
    uint32_t part, left = 0;
    for (part = 0; part < 2u; part++) {
        int32_t oy = SPL_Y0 + (part ? 124 : 0), cx, cy;
        cv_begin(240, part ? SPL_H1 : 124u, T_BG);
        for (cy = SPL_Y0; cy < SPL_Y0 + 124 + (int32_t)SPL_H1; cy += 2)
            for (cx = 32; cx < 208; cx += 2) {
                uint32_t h = ((uint32_t)cx * 73856093u ^ (uint32_t)cy * 19349663u) * 2654435761u;
                int32_t x = cx, y = cy, s = 2, a = f - SPL_LOOSE - (cx - 32) / SPL_FRONT - (int32_t)(h >> 30);
                uint16_t c = spl_ink(cx, cy), c1 = spl_ink(cx + 1, cy + 1);
                if (!c && !c1)
                    continue;
                c = c ? c : c1;
                x += ((int32_t)(h >> 4 & 3u) - 1) * (f > 0) * (1 + (f > 1));   /* loose: a pixel or two off */
                y += ((int32_t)(h >> 6 & 3u) - 1) * (f > 1);
                if (a >= 0) {                   /* swept: faster each frame, drifting, dimming, smaller */
                    x += a * (a + 3) * (int32_t)(4u + (h >> 8 & 7u)) / 4;
                    y += ((int32_t)(h >> 12 & 7u) - 4) * a * (a + 1) / 6;
                    if (a >= 3)
                        c = T_MID;
                    if (a >= 5)
                        s = 1;
                    if (a >= SPL_LIFE - (int32_t)(h >> 16 & 3u))
                        continue;
                }
                if (x < 240 && y + s > oy && y < oy + (int32_t)cv_h) {
                    cv_rect(x, y - oy, s, s, c);
                    left++;
                }
            }
        cv_blit(0, (uint32_t)oy);
    }
    return left;
}
static void spl_outro(void)
{
    uint32_t f, n, t0 = fm1_ms, held = fm1_in.buttons | fm1_in.notes;
    for (f = 1; f < 40u; f++) {                 /* (until the last grain is gone: ~20 frames) */
        if (!spl_frame((int32_t)f))
            break;
        SPL_FRAME_HOOK(f);
        for (n = 0; fm1_ms - t0 < (f + 1u) * SPL_FRAME_MS && n < SPL_FRAME_MS; n++)
            spl_wait(1);
        if ((fm1_in.buttons | fm1_in.notes) & ~held)
            break;
    }
    df_damage(0, 0, 240, 240, DF_N);            /* (main clears the screen past the diff's places) */
}
#endif

/* the tiles set, then at least `ms` gone in all (the boot's own wait); anim 0: the wait alone */
static void splash_lay(uint32_t ms, int anim)
{
    uint32_t k, used = 0;
    if (anim) {
        for (k = 0; k < 8u; k++) {
#if MELODEE_LOOK
            spl_tile(SPL_ORDER[k], 0, T_TEXT);
#else
            spl_tile(SPL_ORDER[k], 0, SPL_LAPIS[SPL_ORDER[k]]);
#endif
            spl_wait(SPL_STEP_MS);
            used += SPL_STEP_MS;
        }
        spl_tile(SPL_GOLD, 2, SPL_GOLD_C);      /* the gold tessera, pressed in */
        spl_wait(2u * SPL_STEP_MS);
        spl_tile(SPL_GOLD, 2, T_BG);
        spl_tile(SPL_GOLD, 0, SPL_GOLD_C);
        spl_wait(2u * SPL_STEP_MS);
        used += 4u * SPL_STEP_MS;
        if (!spl_hold(SPL_HOLD_MS))
            LOOK((void)0, spl_outro());
    }
    if (used < ms)
        spl_wait(ms - used);
}
