/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's panel UI over Tessera's core: two 128 x 64 screens with 16 levels (the yellow NHD-2.7-12864WDY3-M, so
 * Tessera's default Mono ink), 8 data encoders in 2 rows of 4, ENERGY, the 6 page buttons (GEN SOUND MOD FX PIECE
 * MIX), FUNC NO YES and the arrows, 16 keys in 2 rows of 8, REC PLAY STOP. For the virtual prototype (web/andamento)
 * and later the Daisy build. The PAGE screen is the FM-1's Mono page at one pixel a cell: Tessera's header (the
 * page's title, the engine, the 8 tracks as marks), a picture, the page's sub-pages as tabs. The DIAL screen sits
 * over the encoders: each page's 8 settings, from Tessera's central parameter list (pid.c), as Tessera's dials (a list
 * of names in a box), one over each encoder. Left- or right-handed only changes which side each screen is on (the
 * platform's: web/andamento/index.html). Included after the firmware and the screens' framebuffers, and_fb.c (a
 * unity build: it uses pid_*, param_format, param_ratio, song, trk, transport_req, fm1_in, and and_fb.c's drawing
 * calls in the Mono look's 5 x 7 face); never the FM-1's own UI (ui_*.c).
 *
 * Controls:
 *   HOME             the piece at a glance: ENERGY's arc over time, its rests and entries, the 8 tracks
 *                    (and_home.c); Andamento starts here, NO comes back here from any page
 *   page button      that page; pressed again, its next sub-page (also UP / DOWN); FX's 2nd sub-page is the
 *                    master bus, tilt EQ then compressor (and_master.c), song-wide
 *   LEFT / RIGHT     the selected track, 1..8; FUNC + one of the first keys picks a track directly. Tracks 5..8 are
 *                    the external ones (and_ext.c): GEN their generator and clock, SOUND their outputs (OUT)
 *   encoder 1..8     the cell's setting; pushed, back to its default (HOME: tracks 1..8's ENERGY DEPTH)
 *   ENERGY           ENERGY LEVEL on every page
 *   keys             SCALE, CHROM, STEP or MUTE (FUNC + key 9..12), and their RGB lights: and_keys.c
 *   FUNC + UP / DOWN the keys' octave, or STEP's 16 steps shown
 *   PLAY / STOP      transport (PLAY while playing restarts) */

#define AND_NENC 8
#define AND_TRACKS 8                             /* Andamento's tracks: Tessera's NTRK (1..4) and the external 5..8 */

enum { AB_GEN, AB_SOUND, AB_MOD, AB_FX, AB_PIECE, AB_MIX, AB_FUNC, AB_NO, AB_YES, AB_UP, AB_DOWN, AB_LEFT, AB_RIGHT,
       AB_REC, AB_PLAY, AB_STOP, AB_COUNT };

#define AND_HOME 6                               /* the page after the 6 buttons' (and_home.c) */

static struct {
    uint8_t page, sub[6], held[AB_COUNT];
    uint16_t cell[AND_NENC];                     /* this page's settings (PID_NONE: an empty cell) */
    int8_t hot;                                  /* the cell turned last (-1: none), shown until hot_ms + 1200 */
    uint32_t hot_ms, say_ms;
    uint16_t hot_pid;
    const char *say;                             /* a word for the header (a mode, the octave), until say_ms + 1200 */
    int8_t ext;                                  /* an external track selected (0..3: Andamento's 5..8), else -1:
                                                  * song.sel's, one of Tessera's (the core never sees past NTRK) */
} and_ui = {.page = AND_HOME, .hot = -1, .ext = -1};

/* the track selected, 0..7 (5..8 external: and_ext.c), and selecting one */
#define AND_SEL ((uint32_t)(and_ui.ext >= 0 ? NTRK + (uint32_t)and_ui.ext : song.sel))
static void and_select(uint32_t tr)
{
    if (tr < NTRK) {
        song.sel = (uint8_t)tr;
        and_ui.ext = -1;
    } else if (tr < AND_TRACKS) {
        and_ui.ext = (int8_t)(tr - NTRK);
    }
}

static void and_ui_say(const char *s)
{
    static char buf[16];
    uint32_t i;
    for (i = 0; s[i] && i + 1u < sizeof buf; i++)
        buf[i] = s[i];
    buf[i] = 0;
    and_ui.say = buf;
    and_ui.say_ms = fm1_ms | 1u;
    and_ui.hot_ms = 0;
}

#include "and_ext.c"                             /* tracks 5..8: generators out to MIDI and CV / gate */
#include "and_master.c"                           /* the master bus: tilt EQ then compressor, FX page's 2nd sub-page */
#include "and_texture.c"                          /* the texture engine: a granular cloud, FX page's 3rd sub-page */
#define AM_C(x) (AM_PID + (x))
#define AT_C(x) (AT_PID + (x))
#include "and_pages.c"                            /* SOUND, MOD, FX from Tessera's page table, Monomachine-style */

/* a setting of track tr (0..7): Tessera's (pid.c), an external track's (AE_PID + AE_*), or one of the song-wide
 * modules (AM_PID + AM_*, the master bus; AT_PID + AT_*, the texture engine: tr does not matter, each is the
 * same for every track) */
static int and_is_ext(uint32_t pid, uint32_t tr) { return tr >= NTRK && pid >= AE_PID && pid < AE_PID + AE_NPRM; }
static int and_is_master(uint32_t pid) { return pid >= AM_PID && pid < AM_PID + AM_NPRM; }
static int and_is_texture(uint32_t pid) { return pid >= AT_PID && pid < AT_PID + AT_NPRM; }
static const param_desc_t *and_desc(uint32_t pid, uint32_t tr)
{
    int16_t *vp;
    if (and_is_ap(pid))
        return tr < NTRK ? ap_desc(pid - AP_PID, &vp) : 0;
    if (and_is_an(pid))
        return tr < NTRK ? an_desc(pid - AN_PID, tr) : 0;
    if (and_is_master(pid))
        return am_desc(pid - AM_PID);
    if (and_is_texture(pid))
        return at_desc(pid - AT_PID);
    return and_is_ext(pid, tr) ? ae_desc(pid - AE_PID, tr - NTRK) : pid >= AE_PID ? 0 : pid_desc(pid, tr < NTRK ? tr : 0);
}
static int32_t and_get(uint32_t pid, uint32_t tr)
{
    if (and_is_ap(pid))
        return tr < NTRK ? ap_get(pid - AP_PID) : 0;
    if (and_is_an(pid))
        return tr < NTRK ? an_get(pid - AN_PID, tr) : 0;
    if (and_is_master(pid))
        return am_get(pid - AM_PID);
    if (and_is_texture(pid))
        return at_get(pid - AT_PID);
    return and_is_ext(pid, tr) ? ae_get(pid - AE_PID, tr - NTRK) : pid >= AE_PID ? 0 : pid_get(pid, tr < NTRK ? tr : 0);
}
static void and_set(uint32_t pid, uint32_t tr, int32_t v)
{
    if (and_is_ap(pid)) {
        if (tr < NTRK)
            ap_set(pid - AP_PID, v);
    } else if (and_is_an(pid)) {
        if (tr < NTRK)
            an_set(pid - AN_PID, tr, v);
    } else if (and_is_master(pid))
        am_set(pid - AM_PID, v);
    else if (and_is_texture(pid))
        at_set(pid - AT_PID, v);
    else if (and_is_ext(pid, tr))
        ae_set(pid - AE_PID, tr - NTRK, v);
    else if (pid < AE_PID)
        pid_set(pid, tr < NTRK ? tr : 0, v);
}
static const char *and_name(uint32_t pid, uint32_t tr)   /* the setting's full name (the header, when turned) */
{
    const pid_entry_t *e;
    if (and_is_ap(pid))
        return ap_name(pid - AP_PID);
    if (and_is_an(pid))
        return pid == AN_PID + AN_ENGINE ? "ENGINE" : pid == AN_PID + AN_PRESET ? "PRESET" : "FM6 OPERATOR";
    if (and_is_master(pid))
        return am_desc(pid - AM_PID)->label;
    if (and_is_texture(pid))
        return at_desc(pid - AT_PID)->label;
    if (and_is_ext(pid, tr))
        return ae_desc(pid - AE_PID, tr - NTRK)->label;
    return pid < AE_PID && (e = pid_find(pid)) ? e->name : "";
}
/* a track's generator, by name */
static const char *and_gen_name(uint32_t tr)
{
    const param_desc_t *m;
    int32_t mode;
    if (tr >= NTRK)
        return AE_GEN_NAME[and_ext.t[(tr - NTRK) % AE_N].gen];
    m = pid_desc(0x200u, tr);
    mode = pid_get(0x200u, tr);
    return m && m->names ? m->names[mode - m->min] : "";
}
/* a track muted (MIX's MUTE, or an external track's) */
static int and_muted(uint32_t tr) { return tr >= NTRK ? and_ext.t[(tr - NTRK) % AE_N].mute : pid_get(0x028, tr) != 0; }
static void and_mute(uint32_t tr, int on)
{
    if (tr >= NTRK)
        ae_set(AE_MUTE, tr - NTRK, on);
    else
        pid_set(0x028, tr, on);
}

#include "and_keys.c"                            /* the 16 keys and their lights */
#include "and_home.c"                            /* HOME: the piece, its ENERGY arc, the 8 tracks */

/* ------------------------------------------------------------- pages --- */
static const char *const AND_PAGE[6] = {"GEN", "SOUND", "MOD", "FX", "PIECE", "MIX"};
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

/* an external track's pages: GEN its generator and the clock, SOUND its outputs (OUT), PIECE as track 1's (the song's),
 * MIX its outputs and the song's tempo and scale; MOD and FX nothing (no voice of its own) */
#define AE_C(x) (AE_PID + (x))
static const uint16_t AND_EXT_CELLS[6][AND_NENC] = {
    {AE_C(AE_GEN), AE_C(AE_P1), AE_C(AE_P2), AE_C(AE_P3), AE_C(AE_SEED), AE_C(AE_DIV), AE_C(AE_OCT), AE_C(AE_GATE)},
    {AE_C(AE_CHAN), AE_C(AE_VEL), AE_C(AE_GATE), AE_C(AE_OCT), AE_C(AE_DEPTH), AE_C(AE_MUTE), PID_NONE, PID_NONE},
    {PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE},
    {AM_C(AM_TILT), AM_C(AM_THRESH), AM_C(AM_RATIO), AM_C(AM_ATTACK), AM_C(AM_RELEASE), AM_C(AM_MAKEUP),
     AM_C(AM_MIX), PID_NONE},                       /* song-wide: the same master bus as an internal track's FX2 */
    {0x280, 0x281, 0x282, 0x284, 0x259, 0x25a, 0x27b, 0x27d},
    {AE_C(AE_CHAN), AE_C(AE_VEL), AE_C(AE_MUTE), 0x100, 0x101, 0x019, 0x01a, AE_C(AE_DEPTH)},
};
static const char *const AND_EXT_PAGE[6] = {"GEN", "OUT", "MOD", "FX", "PIECE", "MIX"};

/* page p's sub-pages now (an internal track: and_pages.c's map, which follows the engine; an external track: one) */
static uint32_t and_nsub(uint32_t p)
{
    if (p >= 6u)
        return 1;
    if (and_ui.ext >= 0)
        return 1;
    and_map(p);
    return and_nsubs ? and_nsubs : 1u;
}

static void and_cells(void)
{
    uint32_t i, p = and_ui.page, n;
    if (p >= 6u) {                               /* (HOME: no grid) */
        for (i = 0; i < AND_NENC; i++)
            and_ui.cell[i] = PID_NONE;
        return;
    }
    if (and_ui.ext >= 0) {                       /* (an external track: one sub-page each) */
        for (i = 0; i < AND_NENC; i++)
            and_ui.cell[i] = AND_EXT_CELLS[p][i];
        /* GEN's and OUT's cells 1..3 are the generator's own: nothing to show with it OFF. FX is the master bus
         * (and_master.c), the same whatever the generator is doing, so it is left alone. */
        if (p != AB_FX && and_ext.t[and_ui.ext % AE_N].gen == AE_OFF)
            and_ui.cell[1] = and_ui.cell[2] = and_ui.cell[3] = PID_NONE;
        return;
    }
    n = and_nsub(p);                             /* (and_map: and_subs; an engine changed may leave fewer) */
    if (and_ui.sub[p] >= n)
        and_ui.sub[p] = (uint8_t)(n - 1u);
    for (i = 0; i < AND_NENC; i++)
        and_ui.cell[i] = and_nsubs ? and_subs[and_ui.sub[p]].c[i] : PID_NONE;
    if (p == AB_GEN && and_ui.sub[p] == 0)       /* (GEN's: and_gen_cells, whole) */
        return;
    for (i = 0; i < AND_NENC; i++)               /* (an engine's unused knob: no cell; and_desc, not pid_desc, so
                                                   * AM_* (and_master.c) is not mistaken for an unknown pid) */
        if (and_ui.cell[i] != PID_NONE) {
            const param_desc_t *d = and_desc(and_ui.cell[i], song.sel);
            if (!d || d->max <= d->min)
                and_ui.cell[i] = PID_NONE;
        }
}

/* ------------------------------------------------------------- input --- */
static void and_turn(uint32_t pid, int32_t n)
{
    const param_desc_t *d = and_desc(pid, AND_SEL);
    if (!d || !n)
        return;
    and_set(pid, AND_SEL, and_get(pid, AND_SEL) + n);
    if (pid == P_SCALE || pid == P_ROOT)         /* (as the FM-1's knobs: one scale for the song) */
        scale_share(&trk[AND_SEL < NTRK ? AND_SEL : 0u]);
    and_ui.hot_pid = (uint16_t)pid;
    and_ui.hot_ms = fm1_ms | 1u;                 /* (0: nothing turned yet) */
    and_ui.say_ms = 0;
}

/* encoder i (0..7 the grid, 8 ENERGY) turned by n detents */
static void and_enc(uint32_t i, int32_t n)
{
    and_cells();
    if (i == AND_NENC) {
        and_turn(AND_PID_ENERGY, n * 2);
        and_ui.hot = -1;
        and_keys.energy_ms = fm1_ms | 1u;        /* (the keys show it) */
    } else if (and_ui.page == AND_HOME) {
        and_home_enc(i, n, 0);
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
    if (and_ui.page == AND_HOME && i < AND_NENC) {
        and_home_enc(i, 0, 1);
        return;
    }
    pid = i == AND_NENC ? AND_PID_ENERGY : i < AND_NENC ? and_ui.cell[i] : PID_NONE;
    if (pid == PID_NONE || !(d = and_desc(pid, AND_SEL)))
        return;
    and_set(pid, AND_SEL, d->def);
    and_ui.hot = i < AND_NENC ? (int8_t)i : -1;
    and_ui.hot_pid = (uint16_t)pid;
    and_ui.hot_ms = fm1_ms | 1u;
    and_ui.say_ms = 0;
    if (pid == AND_PID_ENERGY)
        and_keys.energy_ms = fm1_ms | 1u;
}

static void and_track(int32_t d)
{
    and_select((AND_SEL + AND_TRACKS + (uint32_t)d) % AND_TRACKS);
    if (and_ui.page == AND_HOME) {               /* (HOME has no T in its header: the track and its generator) */
        char b[16] = {'T', (char)('1' + AND_SEL), ' ', 0};
        str_cpy(b + 3, and_gen_name(AND_SEL), sizeof b - 3);
        and_ui_say(b);
    }
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
            and_ui.sub[b] = (uint8_t)((and_ui.sub[b] + 1u) % and_nsub(b));
        and_ui.page = (uint8_t)b;
        and_ui.hot = -1;
    } else if (b == AB_NO) {
        and_ui.page = AND_HOME;
        and_ui.hot = -1;
    } else if ((b == AB_UP || b == AB_DOWN) && and_ui.held[AB_FUNC]) {
        and_keys_shift(b == AB_UP ? 1 : -1);
    } else if ((b == AB_UP || b == AB_DOWN) && and_ui.page < AND_HOME) {
        uint32_t p = and_ui.page, n = and_nsub(p);
        and_ui.sub[p] = (uint8_t)((and_ui.sub[p] % n + (b == AB_UP ? n - 1u : 1u)) % n);
    } else if (b == AB_LEFT || b == AB_RIGHT) {
        and_track(b == AB_LEFT ? -1 : 1);
    } else if (b == AB_PLAY) {
        transport_req = song.playing ? 3u : 1u;
    } else if (b == AB_STOP) {
        transport_req = 2u;
    }
}

/* key k (0..7 the bottom row, 8..15 the top) down or up: and_keys.c */
static void and_key(uint32_t k, uint32_t down)
{
    if (down)
        and_keys_down(k);
    else
        and_keys_up(k);
}

/* --------------------------------------------------------------- draw --- */
static void and_value(uint32_t pid, uint32_t tr, char *val)
{
    const param_desc_t *d = and_desc(pid, tr);
    const char *unit;
    val[0] = 0;
    if (d)
        param_format(d, and_get(pid, tr), val, &unit);
}

/* CV in volts, as 0.00V .. 10.0V */
static void and_volts(char *b, int32_t mv)
{
    uint32_t v = (uint32_t)(mv < 0 ? 0 : mv), n = 0;
    if (v >= 10000u) {
        str_cpy(b, "10.0V", 8);
        return;
    }
    b[n++] = (char)('0' + v / 1000u);
    b[n++] = '.';
    b[n++] = (char)('0' + v / 100u % 10u);
    b[n++] = (char)('0' + v / 10u % 10u);
    b[n++] = 'V';
    b[n] = 0;
}

/* ------------------------------------------------- the Mono frame --- */
/* Two panels (andamento README, Screen, 2026-11-14): the PAGE screen is the FM-1's Mono page at one pixel a cell
 * (its header, a picture, the sub-pages as tabs) and the DIAL screen the 8 encoders' settings as Tessera's dials, one
 * over each encoder (2 rows of 4, 32 x 32 px a cell). Which side each panel is on (left- or right-handed) is the
 * platform's: the frames are the same. Layout rules (and_lint in the tests): no value cut, a clear pixel between
 * neighbours. */
#define AND_HEAD_Y 1                             /* the header's text; its line at row 9 */
#define AND_PIC_Y 11                             /* the picture: rows 11..51 */
#define AND_PIC_H 41
#define AND_TAB_Y 55                             /* the tabs: rows 55..63 (the line over them at 53) */

/* a label as the FM-1 shortens it (ui_mono.c mo_abbr): 4 letters, no spaces or points; longer, the first letter
 * and the consonants after it */
static void and_abbr(char *d, const char *s)
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
        if (!strchr("AEIOUaeiou", w[i]))
            d[k++] = w[i];
    if (k < 3u)
        k = 4u, str_cpy(d, w, 5);
    d[k] = 0;
}
/* a value as the FM-1 fits it (ui_mono.c mo_fit): 5 characters at most; a number loses its last decimals, a word its
 * vowels from the end; still longer, cut (returns 1: the layout check reports it) */
static int and_fit(char *d, const char *s)
{
    uint32_t n, i, j;
    str_cpy(d, s, 16);
    n = (uint32_t)strlen(d);
    if (n > 5u && (d[0] == '-' || d[0] == '+' || (d[0] >= '0' && d[0] <= '9'))) {
        for (i = 0; d[i] && d[i] != '.'; i++) ;
        while (d[i] && n > 5u && n > i + 1u)
            d[--n] = 0;
        if (d[i] && n == i + 1u)
            d[--n] = 0;
    }
    for (i = n; n > 5u && i-- > 1u;)
        if (strchr("AEIOUaeiou", d[i])) {
            for (j = i; j < n; j++) d[j] = d[j + 1u];
            n--;
        }
    if (n <= 5u)
        return 0;
    d[5] = 0;
    return 1;
}

/* the layout check's record of the dial screen (tests/and_ui_test.c): each cell's value as drawn */
static struct {
    char s[8];
    int16_t x0, x1;                              /* the value's frame (a box) or its text (a dial), in pixels */
    uint8_t kind, cut, used;
} and_cellv[AND_NENC];
enum { AV_DIAL, AV_BOX };

/* a value's kind, as the FM-1's knob pages decide it: a list of names (an engine, a scale, a wave, OFF / ON) in a box,
 * an amount (a number, a time, a level, a count of octaves) on a dial */
static int and_is_list(const param_desc_t *d, const char *val)
{
    if (d->fmt != F_ENUM)
        return 0;
    if (d->max - d->min < 2)
        return 1;
    return !(val[0] >= '0' && val[0] <= '9') && val[0] != '-' && val[0] != '+';
}

/* the value of cell i at (x, y) its top left, 32 x 32: the label over a dial over the value, or the label over the
 * value in a box (a list); hot: the value inverted */
static void and_cell(uint32_t i, const char *label, const char *val, int32_t ratio, int list, int hot)
{
    int32_t x = (int32_t)(i % 4u) * 32, y = (int32_t)(i / 4u) * 32, cx = x + 16, tw, tx;
    char l[8], v[8];
    and_abbr(l, label);
    and_cellv[i].cut = (uint8_t)and_fit(v, val);
    tw = (int32_t)strlen(v) * 6 - 1;
    tx = cx - tw / 2;
    and_text(cx - ((int32_t)strlen(l) * 6 - 1) / 2, y + 1, l, hot ? 15 : 7, 4);
    str_cpy(and_cellv[i].s, v, sizeof and_cellv[i].s);
    and_cellv[i].used = 1;
    if (list) {                                  /* a box round the text, a clear pixel inside; five letters: its top
                                                  * and foot only, so it keeps clear of the next cell */
        int32_t bw = tw + 4, bx = cx - bw / 2;
        if (bx < x + 1)                          /* (inside x+1 .. x+30: two clear pixels to the next cell's) */
            bx = x + 1;
        if (bx + bw > x + 31)
            bw = x + 31 - bx;
        and_cellv[i].kind = AV_BOX;
        and_cellv[i].x0 = (int16_t)bx, and_cellv[i].x1 = (int16_t)(bx + bw - 1);
        if (hot) {
            and_fill(bx, y + 11, bw, 11, 15);
            and_text(tx, y + 13, v, 0, 5);
        } else {
            if (tw + 4 <= bw)
                and_rect(bx, y + 11, bw, 11, 8);
            else
                and_fill(bx, y + 11, bw, 1, 8), and_fill(bx, y + 21, bw, 1, 8);
            and_text(tx, y + 13, v, 15, 5);
        }
        return;
    }
    and_cellv[i].kind = AV_DIAL;
    and_cellv[i].x0 = (int16_t)tx, and_cellv[i].x1 = (int16_t)(tx + tw - 1);
    and_dial(cx, y + 15, 6, ratio, 5, 15);
    if (hot) {
        and_fill(tx - 1, y + 23, tw + 2, 9, 15);
        and_text(tx, y + 24, v, 0, 5);
    } else {
        and_text(tx, y + 24, v, 15, 5);
    }
}

/* the dial screen: the page's 8 settings (HOME: tracks 1..8's ENERGY DEPTH) */
static void and_draw_dials(uint32_t hot)
{
    char val[16], lab[8];
    uint32_t i, sel = AND_SEL;
    and_target(AND_SCR_DIAL);
    and_clear();
    memset(and_cellv, 0, sizeof and_cellv);
    for (i = 0; i < AND_NENC; i++) {
        uint32_t pid = and_ui.cell[i], tr = sel, ishot = hot && and_ui.hot == (int8_t)i;
        const param_desc_t *d;
        const char *unit = "";
        if (and_ui.page == AND_HOME) {           /* (HOME: encoder i is track i's DEPTH) */
            tr = i;
            pid = i >= NTRK ? AE_PID + AE_DEPTH : AND_PID_DEPTH;
            lab[0] = 'T', lab[1] = (char)('1' + i), lab[2] = 0;
        }
        if (pid == PID_NONE || !(d = and_desc(pid, tr))) {
            if (and_ui.ext >= 0 && and_ui.page == AB_SOUND && i >= 6u) {   /* (OUT: its CV and gate, as the jacks) */
                ae_track_t *a = &and_ext.t[and_ui.ext % AE_N];
                lab[0] = i == 6u ? 'C' : 'G', lab[1] = i == 6u ? 'V' : 'T', lab[2] = (char)('5' + and_ui.ext), lab[3] = 0;
                if (i == 6u) {
                    and_volts(val, a->cv_mv);
                    and_cell(i, lab, val, a->cv_mv / 10, 0, 0);
                } else {
                    and_cell(i, lab, a->sounding ? "HIGH" : "LOW", -1, 1, a->sounding);
                }
                continue;
            }
            and_text((int32_t)(i % 4u) * 32 + 14, (int32_t)(i / 4u) * 32 + 13, "-", 3, 1);
            continue;
        }
        {
            int32_t v = and_get(pid, tr);
            char b[24];
            param_format(d, v, val, &unit);
            if (d->fmt == F_ENUM && d->names && v >= d->min && v <= d->max)
                str_cpy(val, d->names[v - d->min], sizeof val);   /* (the whole name: and_fit shortens it as the FM-1) */
            str_cpy(b, val, sizeof b);
            if (!and_is_list(d, val) && unit && unit[0] && strlen(b) + strlen(unit) <= 5u && strcmp(unit, d->label))
                str_cpy(b + strlen(b), unit, sizeof b - strlen(b));   /* (a unit when it fits: 12ms, 60%) */
            {
                const char *pn = an_preset_name(pid, tr, 1);   /* (PRESET: the sound's name, in a box as a list) */
                and_cell(i, and_ui.page == AND_HOME ? lab : d->label ? d->label : "", pn ? pn : b, param_ratio(d, v),
                         pn ? 1 : and_is_list(d, val), (int)ishot);
            }
        }
    }
}

/* the page screen's header (the FM-1's mo_head): the page's title and the engine dim at the right of it, the 8 tracks as marks (the selected one filled, a sounding one lit, a muted one a dash); a line
 * under it. A word to say (a mode, the octave) or the setting just turned: in the title's place, inverted */
static void and_head(const char *title, const char *eng)
{
    uint32_t tr;
    char e[8];
    if (title[0] == '!') {                       /* (inverted) */
        int32_t w = and_text(2, AND_HEAD_Y, title + 1, 0, 14);
        and_fill(0, 0, w + 3, 9, 15);
        and_text(2, AND_HEAD_Y, title + 1, 0, 14);
    } else {                                     /* (the title, then the engine dim, right-aligned before the marks;
                                                  * the place among the sub-pages is the tabs', which always fit) */
        int32_t ex = 84;
        (void)and_fit(e, eng);
        if (e[0])
            ex = 84 - (int32_t)strlen(e) * 6 + 1, and_text(ex, AND_HEAD_Y, e, 7, 5);
        and_text(1, AND_HEAD_Y, title, 15, (uint32_t)((ex - 5) / 6));
    }
    for (tr = 0; tr < AND_TRACKS; tr++) {
        int32_t x = 88 + (int32_t)tr * 5;
        uint32_t k, on = 0;
        if (tr < NTRK)
            for (k = 0; k < NVOICE; k++)
                on += trk[tr].v[k].active;
        else
            on = (uint32_t)and_ext_gate(tr - NTRK);
        if (tr == AND_SEL)
            and_fill(x, 2, 4, 5, 15);
        else if (and_muted(tr))
            and_fill(x, 4, 4, 1, 5);
        else
            and_rect(x, 3, 3, 3, on ? 15 : 5);
    }
    and_fill(0, 9, AND_W, 1, 8);
}

/* the tabs: the page's sub-pages, the shown one inverted (the FM-1's mo_foot), and the keys' mode at the right. More
 * than fit (FM6's SOUND has 11): the run of them that holds the shown one, a dotted edge where more lie past it */
static void and_tabs(const char *const *names, uint32_t n, uint32_t cur)
{
    uint32_t i, first = 0;
    int32_t x, room = AND_W - 6 * (int32_t)strlen(AK_SHORT[and_keys.mode]) - 3, w;
    char t[8];
    for (i = 0; i < AND_W; i += 2)
        and_px((int32_t)i, 53, 4);
    for (;;) {                                   /* (the first tab: the shown one and as many before it as fit) */
        for (w = 0, i = first; i <= cur && i < n; i++)
            w += (int32_t)strlen(names[i] ? names[i] : "") * 6 + 5;
        if (w - 4 <= room - (first ? 3 : 0) || first >= cur)
            break;
        first++;
    }
    x = first ? 3 : 0;
    if (first)
        and_px(0, AND_TAB_Y + 4, 6), and_px(0, AND_TAB_Y + 2, 6), and_px(0, AND_TAB_Y + 6, 6);
    for (i = first; i < n; i++) {
        str_cpy(t, names[i] ? names[i] : "", 5);
        w = (int32_t)strlen(t) * 6 + 1;
        if (x + w > room - (i + 1u < n ? 3 : 0)) {
            and_px(x, AND_TAB_Y + 4, 6), and_px(x, AND_TAB_Y + 2, 6), and_px(x, AND_TAB_Y + 6, 6);
            break;
        }
        if (i == cur) {
            and_fill(x, AND_TAB_Y, w, 9, 15);
            and_text(x + 1, AND_TAB_Y + 1, t, 0, 4);
        } else {
            and_text(x + 1, AND_TAB_Y + 1, t, 6, 4);
        }
        x += w + 4;
    }
    and_text(AND_W - 6 * (int32_t)strlen(AK_SHORT[and_keys.mode]), AND_TAB_Y + 1, AK_SHORT[and_keys.mode], 4, 4);
}

/* the picture (rows 11..51). The setting just turned: its full name, its value twice the size and its dial large.
 * Else, until each page has its own picture (the generators first), the 8 tracks: the number, a block lit while it
 * sounds (5..8: the gate), struck through when muted; ENERGY's level along the foot */
static void and_picture(uint32_t hot)
{
    uint32_t tr;
    if (hot && and_ui.hot_pid != PID_NONE) {
        const param_desc_t *d = and_desc(and_ui.hot_pid, and_ui.page == AND_HOME && and_ui.hot >= 0 ? (uint32_t)and_ui.hot
                                                                                                     : AND_SEL);
        uint32_t tr0 = and_ui.page == AND_HOME && and_ui.hot >= 0 ? (uint32_t)and_ui.hot : AND_SEL;
        char val[16], v[8];
        const char *unit = "";
        int32_t x = 4;
        if (d) {
            int32_t raw = and_get(and_ui.hot_pid, tr0);
            param_format(d, raw, val, &unit);
            if (d->fmt == F_ENUM && d->names && raw >= d->min && raw <= d->max)
                str_cpy(val, d->names[raw - d->min], sizeof val);
            if (an_preset_name(and_ui.hot_pid, tr0, 0)) {   /* (PRESET: its whole name, as the FM-1's browser) */
                and_text(x, AND_PIC_Y + 2, "PRESET", 7, 6);
                and_text2(x, AND_PIC_Y + 14, an_preset_name(and_ui.hot_pid, tr0, 0), 15, 10);
                return;
            }
            (void)and_fit(v, val);
            if (!and_is_list(d, val)) {
                and_dial(18, AND_PIC_Y + 21, 15, param_ratio(d, raw), 5, 15);
                x = 40;
            }
            and_text(x, AND_PIC_Y + 2, and_name(and_ui.hot_pid, tr0), 7, (uint32_t)(AND_W - x) / 6u);
            and_text2(x, AND_PIC_Y + 14, v, 15, 5);
            if (unit && unit[0] && strcmp(unit, d->label) && !and_is_list(d, val))
                and_text(x, AND_PIC_Y + 32, unit, 7, 6);
        }
        return;
    }
    for (tr = 0; tr < AND_TRACKS; tr++) {
        int32_t x0 = (int32_t)tr * 16, sel = tr == AND_SEL;
        uint32_t k, on = 0;
        char s[2] = {(char)('1' + tr), 0};
        if (tr < NTRK)
            for (k = 0; k < NVOICE; k++)
                on += trk[tr].v[k].active;
        else
            on = (uint32_t)and_ext_gate(tr - NTRK);
        if (sel)
            and_fill(x0 + 1, AND_PIC_Y + 1, 15, 9, 15);
        and_text(x0 + 6, AND_PIC_Y + 2, s, sel ? 0 : 12, 1);
        and_rect(x0 + 2, AND_PIC_Y + 13, 13, 20, tr < NTRK ? 6 : 4);
        if (on)
            and_fill(x0 + 4, AND_PIC_Y + 15, 9, 16, 15);
        if (and_muted(tr))
            and_line(x0 + 2, AND_PIC_Y + 32, x0 + 14, AND_PIC_Y + 13, 11);
    }
    {
        int32_t en = pid_get(AND_PID_ENERGY, 0), w = (en + 100) * 110 / 200;
        and_text(0, AND_PIC_Y + 35, "E", 7, 1);
        and_fill(9, AND_PIC_Y + 37, 110, 2, 3);
        and_fill(9, AND_PIC_Y + 37, w > 110 ? 110 : w, 2, 15);
    }
}

/* a moment ago (t from fm1_ms | 1, 0 never): the setting turned and a word said show for 1.2 s */
static int and_recent(uint32_t t) { return t && (uint32_t)(fm1_ms + 1u - t) < 1201u; }

static void and_draw(void)
{
    char t[24], eng[8];
    uint32_t hot = (uint32_t)and_recent(and_ui.hot_ms), say = (uint32_t)and_recent(and_ui.say_ms);
    uint32_t sel = AND_SEL, p = and_ui.page, s = p < 6u ? and_ui.sub[p] : 0u;
    and_cells();
    and_draw_dials(hot);
    and_target(AND_SCR_PAGE);
    and_clear();
    eng[0] = 0;
    if (sel < NTRK)
        str_cpy(eng, eng_abbr(ENGINES[trk[sel].eng_req % NENGINES]->name), sizeof eng);
    else
        str_cpy(eng, "EXT", sizeof eng);
    if (say) {
        t[0] = '!';
        str_cpy(t + 1, and_ui.say, sizeof t - 1);
    } else if (p == AND_HOME) {
        t[0] = 0;
    } else if (and_ui.ext >= 0) {
        str_cpy(t, p == AB_GEN ? and_gen_name(sel) : AND_EXT_PAGE[p], sizeof t);
    } else {
        and_nsub(p);                             /* (and_subs: this page's) */
        str_cpy(t, p == AB_GEN && s == 0u ? and_gen_name(sel) : s < and_nsubs ? and_subs[s].title : "", sizeof t);
        if (!t[0] || !strcmp(t, "OFF"))
            str_cpy(t, "STEP", sizeof t);
    }
    if (p == AND_HOME) {                         /* HOME: its own header (the piece's SHAPE and time) and the arc */
        and_head(say ? t : "", "");
        if (!say)
            and_home_header();
        and_home_draw();
        return;
    }
    and_head(t, eng);
    and_picture(hot);
    if (and_ui.ext >= 0)
        and_tabs(&AND_EXT_PAGE[p], 1, 0);
    else {
        const char *names[AND_NSUB];
        uint32_t i, n = and_nsub(p);
        for (i = 0; i < n && i < AND_NSUB; i++)
            names[i] = i < and_nsubs ? and_subs[i].tab : "";
        and_tabs(names, n, s);
    }
}

/* the LEDs: bit b = button b lit; the keys: 16 x RGB, 0..255 a channel (and_keys.c) */
static uint32_t and_led_buttons(void)
{
    uint32_t m = and_ui.page < AND_HOME ? 1u << and_ui.page : 0u, b;   /* (HOME: no page lit) */
    for (b = AB_FUNC; b < AB_COUNT; b++)
        if (and_ui.held[b])
            m |= 1u << b;
    if (song.playing)
        m |= 1u << AB_PLAY;
    return m;
}
static const uint8_t *and_led_keys(void)
{
    and_keys_lights();
    return &and_keys.rgb[0][0];
}
