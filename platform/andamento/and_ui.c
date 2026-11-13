/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's panel UI over Tessera's core: a 128 x 64 screen with 16 levels (the yellow NHD-2.7-12864WDY3-M, so
 * Tessera's default Mono ink), 8 data encoders in 2 rows of 4, ENERGY, the 6 page buttons (GEN SOUND MOD FX PIECE
 * MIX), FUNC NO YES and the arrows, 16 keys in 2 rows of 8, REC PLAY STOP. First stage, for the virtual prototype
 * (web/andamento) and later the Daisy build: each page is a 2 x 4 grid of the selected track's settings, taken from
 * Tessera's central parameter list (pid.c), one per encoder. Included after the firmware and the screen's
 * framebuffer, and_fb.c (a unity build: it uses pid_*, param_format, song, trk, transport_req, fm1_in, and and_fb.c's
 * and_fb, and_clear, and_fill and and_text in the Mono look's 5 x 7 face).
 *
 * Controls (first stage):
 *   HOME             the piece at a glance: ENERGY's arc over time, its rests and entries, the 8 tracks
 *                    (and_home.c); Andamento starts here, NO comes back here from any page
 *   page button      that page; pressed again, its next sub-page (also UP / DOWN)
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

/* a setting of track tr (0..7): Tessera's (pid.c) or, for 5..8, an external track's (AE_PID + AE_*); a song-wide one
 * of Tessera's on an external track is track 1's */
static int and_is_ext(uint32_t pid, uint32_t tr) { return tr >= NTRK && pid >= AE_PID && pid < AE_PID + AE_NPRM; }
static const param_desc_t *and_desc(uint32_t pid, uint32_t tr)
{
    return and_is_ext(pid, tr) ? ae_desc(pid - AE_PID, tr - NTRK) : pid >= AE_PID ? 0 : pid_desc(pid, tr < NTRK ? tr : 0);
}
static int32_t and_get(uint32_t pid, uint32_t tr)
{
    return and_is_ext(pid, tr) ? ae_get(pid - AE_PID, tr - NTRK) : pid >= AE_PID ? 0 : pid_get(pid, tr < NTRK ? tr : 0);
}
static void and_set(uint32_t pid, uint32_t tr, int32_t v)
{
    if (and_is_ext(pid, tr))
        ae_set(pid - AE_PID, tr - NTRK, v);
    else if (pid < AE_PID)
        pid_set(pid, tr < NTRK ? tr : 0, v);
}
static const char *and_name(uint32_t pid, uint32_t tr)   /* the setting's full name (the header, when turned) */
{
    const pid_entry_t *e;
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

/* an external track's pages: GEN its generator and the clock, SOUND its outputs (OUT), PIECE as track 1's (the song's),
 * MIX its outputs and the song's tempo and scale; MOD and FX nothing (no voice of its own) */
#define AE_C(x) (AE_PID + (x))
static const uint16_t AND_EXT_CELLS[6][AND_NENC] = {
    {AE_C(AE_GEN), AE_C(AE_P1), AE_C(AE_P2), AE_C(AE_P3), AE_C(AE_SEED), AE_C(AE_DIV), AE_C(AE_OCT), AE_C(AE_GATE)},
    {AE_C(AE_CHAN), AE_C(AE_VEL), AE_C(AE_GATE), AE_C(AE_OCT), AE_C(AE_DEPTH), AE_C(AE_MUTE), PID_NONE, PID_NONE},
    {PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE},
    {PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE, PID_NONE},
    {0x280, 0x281, 0x282, 0x284, 0x259, 0x25a, 0x27b, 0x27d},
    {AE_C(AE_CHAN), AE_C(AE_VEL), AE_C(AE_MUTE), 0x100, 0x101, 0x019, 0x01a, AE_C(AE_DEPTH)},
};
static const char *const AND_EXT_PAGE[6] = {"GEN", "OUT", "MOD", "FX", "PIECE", "MIX"};

static void and_cells(void)
{
    uint32_t i, p = and_ui.page, s;
    if (p >= 6u) {                               /* (HOME: no grid) */
        for (i = 0; i < AND_NENC; i++)
            and_ui.cell[i] = PID_NONE;
        return;
    }
    if (and_ui.ext >= 0) {                       /* (an external track: one sub-page each) */
        for (i = 0; i < AND_NENC; i++)
            and_ui.cell[i] = AND_EXT_CELLS[p][i];
        if (and_ext.t[and_ui.ext % AE_N].gen == AE_OFF)
            and_ui.cell[1] = and_ui.cell[2] = and_ui.cell[3] = PID_NONE;
        return;
    }
    s = and_ui.sub[p];
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
            and_ui.sub[b] = (uint8_t)((and_ui.sub[b] + 1u) % AND_SUBS[b]);
        and_ui.page = (uint8_t)b;
        and_ui.hot = -1;
    } else if (b == AB_NO) {
        and_ui.page = AND_HOME;
        and_ui.hot = -1;
    } else if ((b == AB_UP || b == AB_DOWN) && and_ui.held[AB_FUNC]) {
        and_keys_shift(b == AB_UP ? 1 : -1);
    } else if ((b == AB_UP || b == AB_DOWN) && and_ui.page < AND_HOME) {
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

static void and_draw(void)
{
    char s[24], val[16];
    uint32_t i, tr, hot = and_ui.hot_ms && fm1_ms - and_ui.hot_ms < 1200u;
    uint32_t say = and_ui.say_ms && fm1_ms - and_ui.say_ms < 1200u;
    uint32_t sel = AND_SEL;
    and_cells();
    and_clear();
    /* the header, a lit bar: the page (or the setting just turned, by its full name), the track, transport, tempo */
    and_fill(0, 0, AND_W, 9, 15);
    if (say)
        and_text(2, 1, and_ui.say, 0, 15);
    else if (hot && and_ui.page == AND_HOME && and_ui.hot >= 0) {   /* (HOME: the track's DEPTH and its value) */
        int32_t x;
        s[0] = 'T';
        s[1] = (char)('1' + and_ui.hot);
        s[2] = 0;
        x = and_text(2, 1, s, 0, 2);
        x += and_text(x + 8, 1, "DEPTH", 0, 5);
        and_value(and_ui.hot_pid, (uint32_t)and_ui.hot, val);
        and_text(x + 16, 1, val, 0, 6);
    } else if (hot)
        and_text(2, 1, and_name(and_ui.hot_pid, sel), 0, 15);
    else if (and_ui.page == AND_HOME)
        and_home_header();
    else {
        int32_t x = and_text(2, 1, (and_ui.ext >= 0 ? AND_EXT_PAGE : AND_PAGE)[and_ui.page], 0, 5);
        if (AND_SUBS[and_ui.page] > 1u && and_ui.ext < 0) {
            s[0] = (char)('1' + and_ui.sub[and_ui.page]);
            s[1] = 0;
            and_text(x + 3, 1, s, 5, 1);
        }
        s[0] = 'T';
        s[1] = (char)('1' + sel);
        s[2] = 0;
        and_text(52, 1, s, 0, 2);
        and_text(66, 1, AK_SHORT[and_keys.mode], 0, 3);   /* the keys' mode */
    }
    and_text(92, 1, song.playing ? ">" : "-", song.playing ? 0 : 6, 1);
    {
        uint32_t bpm = (uint32_t)pid_get(0x100, 0);
        s[0] = (char)('0' + bpm / 100 % 10);
        s[1] = (char)('0' + bpm / 10 % 10);
        s[2] = (char)('0' + bpm % 10);
        s[3] = 0;
        and_text(107, 1, bpm < 100 ? s + 1 : s, 0, 3);
    }
    if (and_ui.page == AND_HOME) {
        and_home_draw();
        return;
    }
    /* the 2 x 4 grid: each cell its label (dim) over its value, the cell turned last lit */
    for (i = 0; i < AND_NENC; i++) {
        int32_t x = (int32_t)(i % 4u) * 32, y = 12 + (int32_t)(i / 4u) * 20;
        uint32_t pid = and_ui.cell[i];
        const param_desc_t *d;
        if (pid == PID_NONE || !(d = and_desc(pid, sel))) {
            if (and_ui.ext >= 0 && and_ui.page == AB_SOUND && i >= 6u) {   /* (OUT: its CV and gate, as the jacks) */
                ae_track_t *a = &and_ext.t[and_ui.ext % AE_N];
                s[0] = i == 6u ? 'C' : 'G', s[1] = i == 6u ? 'V' : 'T', s[2] = ' ';
                s[3] = (char)('1' + and_ui.ext), s[4] = 0;
                and_text(x + 1, y, s, 7, 5);
                if (i == 6u)
                    and_volts(val, a->cv_mv);
                and_text(x + 1, y + 9, i == 6u ? val : a->sounding ? "HIGH" : "LOW", a->sounding ? 15 : 8, 5);
                continue;
            }
            and_text(x + 1, y + 8, "-", 3, 1);
            continue;
        }
        and_text(x + 1, y, d->label ? d->label : "", 7, 5);
        and_value(pid, sel, val);
        if (hot && and_ui.hot == (int8_t)i) {
            and_fill(x, y + 8, 31, 9, 15);
            and_text(x + 1, y + 9, val, 0, 5);
        } else {
            and_text(x + 1, y + 9, val, 15, 5);
        }
    }
    /* the 8 tracks along the foot, 12 px each: the number, the selected one inverted, a bar while it sounds (5..8:
     * the gate), struck through when muted; 5..8 marked under as external; ENERGY */
    and_fill(0, 52, AND_W, 1, 3);
    for (tr = 0; tr < AND_TRACKS; tr++) {
        int32_t x = (int32_t)tr * 12;
        uint32_t k, on = 0, is = tr == sel;
        s[0] = (char)('1' + tr);
        s[1] = 0;
        if (is)
            and_fill(x, 54, 11, 9, 15);
        and_text(x + 3, 55, s, is ? 0 : 15, 1);
        if (tr < NTRK)
            for (k = 0; k < NVOICE; k++)
                on += trk[tr].v[k].active;
        else
            on = (uint32_t)and_ext_gate(tr - NTRK);
        if (on)
            and_fill(x, 63, 11, 1, is ? 15 : 10);
        else if (tr >= NTRK)
            for (k = 0; k < 11u; k += 2)
                and_px(x + (int32_t)k, 63, 4);
        if (and_muted(tr))
            and_fill(x + 1, 58, 9, 1, is ? 0 : 15);   /* muted: struck through */
    }
    {
        int32_t en = pid_get(AND_PID_ENERGY, 0), w = (en + 100) * 20 / 200;
        and_text(100, 55, "E", 10, 1);
        and_fill(107, 57, 20, 3, 3);
        and_fill(107, 57, w > 20 ? 20 : w, 3, 15);
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
