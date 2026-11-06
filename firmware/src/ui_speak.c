/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The screen reader (MELODEE_ACCESS, issue #430): once a UI frame, what changed on the screen, sent as words to
 * fm1-reader on the computer (announce.c, its protocol: nothing is built or sent until a reader says HELLO).
 * The first slice: the page (its name, then its knobs: name and value), a knob turning, the menu's rows, the
 * messages in the header (ui.c ui_say) and PLAY / STOP. The screen's own words are sent; a words file (later)
 * tells the reader how to say them.
 * Knob names come from help_find (the parameter's whole name, "ENV ATTACK"), values as the knob's column shows
 * them (the Mono look's mo_col). Included by melodee.c after ui_layer.c (it needs ui_menu.c's rows); the includer
 * puts announce.c before ui.c and supplies ann_wire_send. */

#define SP_PAGE_MS 200u                                 /* a page settles before it is said (flipping through) */
#define SP_KNOB_MS 120u                                 /* a knob's value once it is still (announce.c: at least
                                                         * every 4 x this while it keeps turning) */
static struct {
    uint32_t ctx;                                       /* what screen: page, track, menu, layer (sp_context) */
    uint8_t started, playing, turned;                   /* turned: 1 + the knob turned (ui_input.c page_knob) */
    char knob[48];                                      /* the knob last said ("CUTOFF 64") */
} sp;
static char sp_buf[ANN_MAX + 1];

static void sp_turn(uint32_t k) { sp.turned = (uint8_t)(1u + (k & 3u)); }

static void sp_add(const char *s)                       /* s after what sp_buf holds, cut at ANN_MAX */
{
    uint32_t n = str_len(sp_buf);
    if (s && n < ANN_MAX)
        str_cpy(sp_buf + n, s, ANN_MAX + 1u - n);
}

/* knob k on the page shown, "NAME VALUE" into b; 0: it does nothing here (or nothing to say) */
static int sp_knob(uint32_t k, char *b, uint32_t size)
{
    __typeof__(help) keep = help;                       /* (MIDI LEARN and the mod shortcut use help_find too) */
    uint32_t keep_tr = help_tr;
    const pid_entry_t *e;
    const char *name;
    int unused;
    b[0] = 0;
    help_find(k);
    e = help.pid != PID_NONE ? pid_find(help.pid) : 0;
    name = e ? e->name : help.name;                     /* (HP_NONE: the column's own label, or none) */
    unused = !e && help.hp == HP_UNUSED;
    help = keep;
    help_tr = keep_tr;
    if (unused) return 0;
#if MELODEE_LOOK
    if (!name && mo_col[k & 3u].used) name = mo_col[k & 3u].label;
    if (!name || !mo_col[k & 3u].used) return 0;
    str_cpy(b, name, size);
    if (mo_col[k & 3u].val[0] && !str_eq(mo_col[k & 3u].val, "--")) {   /* ("--": nothing set, not said) */
        str_cpy(b + str_len(b), " ", size - str_len(b));
        str_cpy(b + str_len(b), mo_col[k & 3u].val, size - str_len(b));
    }
    if (mo_col[k & 3u].unit[0]) {
        str_cpy(b + str_len(b), " ", size - str_len(b));
        str_cpy(b + str_len(b), mo_col[k & 3u].unit, size - str_len(b));
    }
    return 1;
#else
    if (!name) return 0;
    str_cpy(b, name, size);
    return 1;
#endif
}

/* the menu's row: "COLOR YELLOW, 1 OF 26" */
static void sp_menu_row(char *b, uint32_t size)
{
    char v[24];
    const char *val = menu_value(ui.menu_sel, v, sizeof v);
    str_cpy(b, MI_NAME[ui.menu_sel % MI_COUNT], size);
    if (val && val[0]) {
        str_cpy(b + str_len(b), " ", size - str_len(b));
        str_cpy(b + str_len(b), val, size - str_len(b));
    }
    str_cpy(b + str_len(b), ", ", size - str_len(b));
    fmt_int(v, (int32_t)ui.menu_sel + 1);
    str_cpy(b + str_len(b), v, size - str_len(b));
    str_cpy(b + str_len(b), " OF ", size - str_len(b));
    fmt_int(v, (int32_t)MI_COUNT);
    str_cpy(b + str_len(b), v, size - str_len(b));
}

static uint32_t sp_context(void)
{
    return (uint32_t)ui.home | (uint32_t)ui.page << 1 | (uint32_t)ui.menu << 9 | (uint32_t)ui.layer << 11 |
           (uint32_t)song.sel << 15 | (uint32_t)(ui.confirm != 0) << 17;
}

/* the screen whole into sp_buf: its name, the track, then each knob that does something */
static void sp_screen(void)
{
    char b[48];
    uint32_t k;
    sp_buf[0] = 0;
    if (ui.menu) {
        sp_add(ui.menu == 2 ? "ABOUT" : "MENU. ");
        if (ui.menu == 1) {
            sp_menu_row(b, sizeof b);
            sp_add(b);
        }
        return;
    }
    sp_add(ui.home ? "HOME" : cur_page()->title);
    sp_add(". T");
    b[0] = (char)('1' + song.sel % NTRK);
    b[1] = 0;
    sp_add(b);
    for (k = 0; k < 4u; k++)
        if (sp_knob(k, b, sizeof b)) {
            sp_add(k ? ", " : ". ");
            sp_add(b);
        }
}

/* main.c, after ui_draw (the knobs' columns are this frame's): what changed, said */
static void ui_speak(uint32_t now)
{
    uint32_t ctx = sp_context();
    int playing = song.playing || chain_busy();
    if (ann_on()) {
        int where = ann_where();
        if (!sp.started || where) {                     /* a new reader, or WHERE: everything, at once */
            sp_screen();
            ann_say(sp_buf, 0, ANN_NOW);
        } else if (ctx != sp.ctx) {                     /* another page, track or the menu */
            sp_screen();
            ann_later(sp_buf, ANN_NOW, SP_PAGE_MS, now);
            sp.knob[0] = 0;
            if (ui.menu == 1) sp_menu_row(sp.knob, sizeof sp.knob);   /* (its row is in what was said) */
        } else if (ui.menu == 1 || sp.turned) {         /* a menu row or a knob turned: its name and value, when
                                                         * they change (a value moving by itself is not said) */
            char b[48];
            if (ui.menu == 1) sp_menu_row(b, sizeof b);
            else if (!sp_knob(sp.turned - 1u, b, sizeof b)) b[0] = 0;
            if (b[0] && !str_eq(b, sp.knob)) {
                str_cpy(sp.knob, b, sizeof sp.knob);
                ann_later(b, ANN_NOW, SP_KNOB_MS, now);
            }
        }
        if (playing != sp.playing)
            ann_say(playing ? "PLAY" : "STOP", 0, ANN_NOW);
        sp.started = 1;
    } else {
        sp.started = 0;
    }
    sp.ctx = ctx;
    sp.turned = 0;
    sp.playing = (uint8_t)playing;
    ann_tick(now);
}
