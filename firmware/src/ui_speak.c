/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The screen reader (MELODEE_ACCESS, issue #430): once a UI frame, what changed on the screen, sent as words to
 * fm1-reader on the computer (announce.c, its protocol: nothing is built or sent until a reader says HELLO).
 * The first slice: the page (its name, then its knobs: name and value), a knob turning, the menu's rows, the
 * messages in the header (ui.c ui_say) and PLAY / STOP. The screen's own words are sent; a words file (later)
 * tells the reader how to say them.
 * Knob names come from help_find (the parameter's whole name, "ENV ATTACK"), values as the knob's column shows
 * them (the page's draw_columns). Included by melodee.c after ui_layer.c (it needs ui_menu.c's rows); the includer
 * puts announce.c before ui.c and supplies ann_wire_send. */

#define SP_PAGE_MS 200u                                 /* a page settles before it is said (flipping through) */
#define SP_KNOB_MS 120u                                 /* a knob's value once it is still (announce.c: at least
                                                         * every 4 x this while it keeps turning) */
static struct {
    uint32_t ctx;                                       /* what screen: page, track, menu, layer (sp_context) */
    uint32_t step;                                      /* the step under the cursor (sp_step_sig) */
    uint8_t started, playing, turned;                   /* turned: 1 + the knob turned (ui_input.c page_knob) */
    char knob[48];                                      /* the knob last said ("CUTOFF 64") */
} sp;
static char sp_buf[ANN_MAX + 1];

static void sp_turn(uint32_t k) { sp.turned = (uint8_t)(1u + (k & 3u)); }

static void sp_ascii(char *s)                          /* the screen's own marks: a middle dot a comma, others out */
{
    char *d = s;
    for (; *s; s++)
        if ((uint8_t)*s < 0x80u) *d++ = *s;
        else if ((uint8_t)*s == 0xB7u) *d++ = ',';
    *d = 0;
}

static void sp_add(const char *s)                       /* s after what sp_buf holds, cut at ANN_MAX */
{
    uint32_t n = str_len(sp_buf);
    if (s && n < ANN_MAX)
        str_cpy(sp_buf + n, s, ANN_MAX + 1u - n);
    sp_ascii(sp_buf + n);
}

static int sp_digit(const char *s) { for (; *s; s++) if (*s >= '0' && *s <= '9') return 1; return 0; }

static void sp_cat(char *b, uint32_t size, const char *s)   /* " s" after b (nothing for an empty s or "--") */
{
    uint32_t n = str_len(b);
    if (!s || !s[0] || str_eq(s, "--") || n + 2u >= size)
        return;
    if (n) b[n++] = ' ';
    str_cpy(b + n, s, size - n);
}

static void sp_num(char *b, uint32_t size, int32_t n)   /* " n" after b */
{
    char d[12];
    fmt_int(d, n);
    sp_cat(b, size, d);
}
static void sp_place(char *b, uint32_t size, uint32_t i, uint32_t n)   /* ", i OF n" after b */
{
    str_cpy(b + str_len(b), ",", size - str_len(b));
    sp_num(b, size, (int32_t)i + 1);
    sp_cat(b, size, "OF");
    sp_num(b, size, (int32_t)n);
}

/* a page of its own (PATTERNS, SONG, the sound browser): knob k as it reads there; 0 nothing */
static int sp_own(uint32_t k, char *b, uint32_t size)
{
    uint32_t g = cur_page()->graph, n, r;
    b[0] = 0;
    if (g == GR_PATGRID) {                              /* KNOB k: track k's next pattern (Copy to: the place) */
        const track_t *t = &trk[k % NTRK];
        if (ptc_on()) {
            str_cpy(b, "COPY TO PATTERN", size);
            sp_num(b, size, (int32_t)ui.ptc_dst + 1);
            sp_cat(b, size, pattern_used(ui.ptc_trk, ui.ptc_dst) ? "IN USE" : "EMPTY");
            return 1;
        }
        b[0] = 'T';
        b[1] = (char)('1' + k % NTRK);
        b[2] = 0;
        sp_cat(b, size, "PATTERN");
        sp_num(b, size, (int32_t)t->pattern + 1);
        if (tessera.pat_gen[k % NTRK])                  /* (PAT GEN: the pattern's own generator) */
            sp_cat(b, size, pid_desc(PID_TESS(TX_MODE), k % NTRK)->names[tessera_pg_mode(k % NTRK, t->pattern)]);
        if (!pattern_used(k % NTRK, t->pattern)) sp_cat(b, size, "EMPTY");
        if (t->pattern_next < NPAT && t->pattern_next != t->pattern) {
            sp_cat(b, size, "NEXT");
            sp_num(b, size, (int32_t)t->pattern_next + 1);
            if (tessera.pat_gen[k % NTRK])
                sp_cat(b, size, pid_desc(PID_TESS(TX_MODE), k % NTRK)->names[tessera_pg_mode(k % NTRK, t->pattern_next)]);
        }
        return 1;
    }
    if (g == GR_SONG) {                                 /* KNOB 1 the section, 2 the track, 3 its pattern, 4 repeats */
        n = chain_config.count;
        r = ui.song_row;
        if (k == 0u) {
            if (r >= n) {
                str_cpy(b, "NEW SECTION", size);
                return 1;
            }
            char l[8] = {row_letter((const uint8_t (*)[NTRK])chain_patterns, r), 0};
            str_cpy(b, "SECTION", size);
            sp_cat(b, size, l);
            l[0] = 'x';                                 /* ("x2": the reader's "2 times") */
            fmt_int(l + 1, chain_config.row[r].repeat);
            sp_cat(b, size, l);
            sp_place(b, size, r, n);
            return 1;
        }
        if (k == 3u) {
            if (r >= n) return 0;
            str_cpy(b, "REPEATS", size);
            sp_num(b, size, chain_config.row[r].repeat);
            return 1;
        }
        if (k == 2u && r >= n) return 0;                /* (the section to add: its patterns once KNOB 3 adds it) */
        if (k == 1u) {                                  /* (KNOB 2: the track) */
            b[0] = 'T';
            b[1] = (char)('1' + song.sel % NTRK);
            b[2] = 0;
            return 1;
        }
        if (r < n) {
            uint32_t p = chain_patterns[r][song.sel % NTRK];
            str_cpy(b, "PATTERN", size);
            sp_num(b, size, (int32_t)p + 1);
            if (!pattern_used(song.sel % NTRK, p)) sp_cat(b, size, "EMPTY");
        }
        return 1;
    }
    if (g == GR_BROWSE) {                               /* KNOB 1 the list, 2 the sound, 3 its favourite */
        uint32_t total, cur = preset_pos(&total), src, e;
        char tag[6], nm[16];
        if (k == 0u) {
            str_cpy(b, "LIST", size);
            sp_cat(b, size, browser_list_name(list_mode()));
            sp_num(b, size, (int32_t)total);
            sp_cat(b, size, total == 1u ? "SOUND" : "SOUNDS");
            return 1;
        }
        if (k == 2u) {
            str_cpy(b, "FAVOURITE", size);
            sp_cat(b, size, browser_shown_fav() ? "ON" : "OFF");
            return 1;
        }
        if (k != 1u || !total) return 0;
        if (cur >= total) cur = 0;
        src = preset_at(cur, &e);
        entry_label(src, e, tag, nm);
        str_cpy(b, nm[0] ? nm : tag, size);             /* (its engine: on WHERE's PRESETS, not while turning) */
        sp_place(b, size, cur, total);
        return 1;
    }
    return 0;
}

/* knob k on the page shown, "NAME VALUE" into b; 0: it does nothing here (or nothing to say). A parameter's whole
 * name (pid.c; an engine's own knobs by the engine's label, not "ENGINE 4") and its value as help_value words it
 * (enum names whole); a page's own knob by its column (draw_columns, not on HOME) */
/* the drawn value's numbers all in the said one (each run of digits, as a word of its own) */
static int sp_nums_in(const char *drawn, const char *said)
{
    while (*drawn) {
        char n[8];
        uint32_t i = 0;
        const char *p;
        if (*drawn < '0' || *drawn > '9') { drawn++; continue; }
        while (*drawn >= '0' && *drawn <= '9' && i < 7u) n[i++] = *drawn++;
        n[i] = 0;
        for (p = said; *p; p++) {                       /* (the firmware's libc has no strstr) */
            uint32_t j = 0;
            while (j < i && p[j] == n[j]) j++;
            if (j == i && (p == said || p[-1] < '0' || p[-1] > '9') && (p[i] < '0' || p[i] > '9')) break;
        }
        if (!*p) return 0;
    }
    return 1;
}
static int sp_knob(uint32_t k, char *b, uint32_t size)
{
    __typeof__(help) keep = help;                       /* (MIDI LEARN and the mod shortcut use help_find too) */
    uint32_t keep_tr = help_tr;
    const pid_entry_t *e;
    const char *name;
    char v[24];
    int unused, col;
    b[0] = v[0] = 0;
    if (ui.layer) {                                     /* a layer (GLO, SCL held): its knobs as its map shows them */
        uint8_t cs = col_style;
        memset(lcap, 0, sizeof lcap);
        col_style = CS_CAPTURE;
        layer_cards(ui.layer);
        col_style = cs;
        if (!lcap[k & 3u].on) return 0;
        if (ui.layer == LAYER_GLO) {                    /* (GLO: KNOB k track k's) */
            b[0] = 'T';
            b[1] = (char)('1' + k);
            b[2] = ' ';
            b += 3;
            size -= 3u;
        }
        list_words(b, lcap[k & 3u].label, size);
        sp_cat(b, size, lcap[k & 3u].val);
        if (sp_digit(lcap[k & 3u].val)) sp_cat(b, size, lcap[k & 3u].unit);
        return 1;
    }
    if (!ui.home && cur_page()->graph == GR_SCALE_PICKER) {   /* SCALES: its columns as drawn (the scale under the
                                                         * cursor, not the one in use) */
        const char *cv;
        uint8_t cs = col_style;
        memset(lcap, 0, sizeof lcap);
        col_style = CS_CAPTURE;
        draw_columns();
        col_style = cs;
        if (!lcap[k & 3u].on) return 0;
        for (cv = lcap[k & 3u].val; *cv == ' '; cv++) ;
        str_cpy(b, lcap[k & 3u].label, size);
        sp_cat(b, size, cv);
        if (sp_digit(cv)) sp_cat(b, size, lcap[k & 3u].unit);
        return 1;
    }
    if (!ui.home && own_screen()) return sp_own(k, b, size);
    help_find(k);
    e = help.pid != PID_NONE ? pid_find(help.pid) : 0;
    name = e ? e->name : help.name;                     /* (HP_NONE: the column's own label, or none) */
    if (e && e->pid >= PID_TRACK(P_E0) && e->pid < PID_TRACK(P_E0 + 8)) {   /* (the engine's eight: its own names) */
        const param_desc_t *d = pid_desc(e->pid, help_tr);
        if (d && d->label && d->label[0]) name = d->label;
    }
    if (e) help_value(e, v, sizeof v);
    if (e && e->pid == PID_TESS(TX_MODE) && help_tr < NTRK && seqm_get(help_tr) == 1u)
        str_cpy(v, "ARP", sizeof v);                    /* (GEN's MODE: ARP lives in ARP's MODE, not tessera.mode, so the
                                                         * value's name says STEP; the screen says ARP, ui_draw.c) */
    if (v[0] == '/') {                                  /* "/4": every 4 steps (POLY RHYTHM, QUAD EVERY) */
        char n[8];
        str_cpy(n, v + 1, sizeof n);
        str_cpy(v, "EVERY", sizeof v);
        sp_cat(v, sizeof v, n);
        sp_cat(v, sizeof v, str_eq(n, "1") ? "STEP" : "STEPS");
    } else if (!sp_digit(v)) {                          /* no unit after a word: "OFF", not "OFF bar" */
        char *u = v + str_len(v);
        while (u > v && u[-1] != ' ') u--;
        if (u > v && *u >= 'a' && *u <= 'z') u[-1] = 0;
    }
    unused = !e && help.hp == HP_UNUSED;
    help = keep;
    help_tr = keep_tr;
    col = 0;
    if (!ui.home && !own_screen()) {                    /* the page's own column (draw_columns, as a list row takes
                                                         * it: ui_list.c CS_CAPTURE) */
        uint8_t cs = col_style;
        memset(lcap, 0, sizeof lcap);
        col_style = CS_CAPTURE;
        draw_columns();
        col_style = cs;
        col = lcap[k & 3u].on;
    }
    if (unused && (!col || list_on())) return 0;                       /* (a knob the help calls unused but the page draws, as FM6
                                                         * STORE's SEND and INIT: said as drawn) */
    if ((!name || unused) && col) name = lcap[k & 3u].label;
    if (v[0] && col && !list_on()) {                    /* what is said is what the screen draws: a word drawn where the
                                                         * value would be a number (LOCK, OFF), or other numbers than the
                                                         * value's (a step 1 OF 8, a position 64 OF 127): the column's */
        const char *cv = lcap[k & 3u].val;
        while (*cv == ' ') cv++;
        if (cv[0] && !str_eq(cv, "--") && (sp_digit(cv) != sp_digit(v) || (sp_digit(cv) && !sp_nums_in(cv, v))))
            v[0] = 0;
    }
    if (!v[0] && col) {
        const char *u = lcap[k & 3u].unit;
        const char *cv = lcap[k & 3u].val;
        while (*cv == ' ') cv++;
        str_cpy(v, str_eq(cv, "--") ? "NONE" : cv, sizeof v);   /* ("--": nothing set) */
        if (u[0] == '/') {                              /* "1" "/16": 1 OF 16 (a step, a cord) */
            sp_cat(v, sizeof v, "OF");
            sp_cat(v, sizeof v, u + 1);
        }
        else if (str_eq(u, "+-") || str_eq(u, "+"))   /* (a mod source's bar: both ways, or one) */
            sp_cat(v, sizeof v, u[1] ? "BIPOLAR" : "UNIPOLAR");
        else if (sp_digit(v)) sp_cat(v, sizeof v, u);   /* (no unit after a word: "OFF", not "OFF bar") */
    }
    if (!name || !name[0] || str_eq(name, "-") || (!v[0] && !col && !e)) return 0;   /* ("-": an engine's unused knob) */
    if (!ui.home && (cur_page()->scope == SC_TRK || cur_page()->graph == GR_MIDICH)) {   /* MIXER, MIDI OUT: knob k
                                                         * is track k's */
        b[0] = 'T';
        b[1] = (char)('1' + k);
        b[2] = ' ';
        b += 3;
        size -= 3u;
    }
    str_cpy(b, name, size);
    sp_cat(b, size, v);
    return 1;
}

/* a list page's row (ui_list.c): "GLIDE MODE TIME, 2 OF 9"; 0: not a list page */
static int sp_list_row(char *b, uint32_t size)
{
    const list_t *l = list_on();
    char lb[16], v[16];
    uint16_t vc;
    uint32_t sk = slot_kind();
    if (sk) {                                           /* PROJECT, USER, STORE: the slot picked (ui_slots.c) */
        uint32_t r = slot_cur(sk);
        char tag[8], nm[16], d[24];
        int used = slot_row(sk, r, tag, nm);
        slot_detail(sk, r, used, d);
        b[0] = 0;
        if (sk == SK_PROJECT && !str_eq(tag, "+")) str_cpy(b, tag, size);   /* (A, T; a slot's "U07": its place) */
        if (!str_eq(nm, "--")) sp_cat(b, size, nm);
        if (!str_eq(nm, d) && !(used == 1 && str_eq(d, "Saved"))) {
            if (b[0]) str_cpy(b + str_len(b), ",", size - str_len(b));
            sp_cat(b, size, d);
        }
        if (sk == SK_PROJECT && settings_boot == r + 1u && used == 1) sp_cat(b, size, "BOOT");
        sp_place(b, size, r, slot_count(sk));
        return 1;
    }
    if (!l) return 0;
    list_cap(list_at(l, lst.row), lb, v, &vc);
    str_cpy(b, lb, size);
    sp_cat(b, size, v);
    sp_place(b, size, lst.row, list_n(l));
    return 1;
}

/* ui.c ui_say: a message, said as shown, keycaps' brackets left out ("[SEQ] TO RECORD") */
static void sp_msg(const char *a, const char *b)
{
    char m[48];
    uint32_t i, n = 0;
    str_cpy(m, a, sizeof m);
    str_cpy(m + str_len(m), b ? b : "", sizeof m - str_len(m));
    for (i = 0; m[i]; i++)
        if (m[i] != '[' && m[i] != ']') m[n++] = m[i];
    m[n] = 0;
    sp_ascii(m);
    ann_say(m, 0, ANN_NOW);
}

/* an open sheet's row (OCT+ held: "LOAD, 1 OF 3") or a list of a knob's values; 0: none open */
static int sp_pop_row(char *b, uint32_t size)
{
    uint32_t i, n = 0;
    b[0] = 0;
    if (pop.on == POP_SHEET && pop.sel < pop.n) {
        const sheet_row_t *r = &pop.rows[pop.sel];
        char v[20];
        for (i = 0; r->label[i] && n + 1u < size; i++)
            if ((uint8_t)r->label[i] < 0x80u) b[n++] = r->label[i];   /* (not its "..." mark) */
        b[n] = 0;
        if (r->value) {
            r->value(v);
            sp_cat(b, size, v);
        }
        sp_place(b, size, pop.sel, pop.n);
        return 1;
    }
    if (pop.on == POP_LIST) {
        int16_t *vp;
        const param_desc_t *d = vlist_desc(&vp);
        if (!d || !d->names || vl.sel < d->min || vl.sel > d->max) return 0;
        str_cpy(b, d->names[vl.sel - d->min], size);
        sp_place(b, size, (uint32_t)(vl.sel - d->min), (uint32_t)(d->max - d->min + 1));
        return 1;
    }
    return 0;
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
           (uint32_t)song.sel << 15 | (uint32_t)ui.confirm << 17 | (uint32_t)pop.on << 25 |
           (uint32_t)(pop.on == POP_SHEET) * str_hash(0, pop.title) << 27;
}

/* the screen whole into sp_buf: its name, the track, then each knob that does something */
static void sp_screen(void)
{
    char b[48];
    uint32_t k, n;
    sp_buf[0] = 0;
    if (ui.menu) {
        sp_add(ui.menu == 2 ? "ABOUT" : "MENU. ");
        if (ui.menu == 1) {
            sp_menu_row(b, sizeof b);
            sp_add(b);
        }
        return;
    }
    if (ui.confirm) {                                   /* a question: its words, then the keys */
        char a[24], q[32];
        confirm_text(a, q);
        confirm_words(a);
        sp_add(a);
        if (q[0] && !str_eq(q, "Old settings/template replaced")) {   /* (the question says it) */
            sp_add(" ");
            sp_add(q);
        }
        k = str_len(sp_buf);
        sp_add(k && (sp_buf[k - 1u] == '?' || sp_buf[k - 1u] == '.') ? " OCT+ YES, OCT- NO" : ". OCT+ YES, OCT- NO");
        return;
    }
    if (pop.on == POP_SHEET || pop.on == POP_LIST) {    /* a sheet or a knob's list: its title, its row */
        if (pop.on == POP_SHEET) {
            sp_add(pop.title);
            if (pop.sub[0]) {
                sp_add(" ");
                sp_add(pop.sub);
            }
        } else {
            int16_t *vp;
            const param_desc_t *d = vlist_desc(&vp);
            sp_add(d && d->label ? d->label : "LIST");
        }
        if (sp_pop_row(b, sizeof b)) {
            sp_add(". ");
            sp_add(b);
        }
        return;
    }
    if (ui.layer) {                                     /* a layer: its header, its knobs */
        const char *h = layer_head();
        for (k = n = 0; h[k] && n + 1u < sizeof b; k++)
            if (h[k] != '[' && h[k] != ']') b[n++] = h[k];
        b[n] = 0;
        sp_add(b);
        for (k = n = 0; k < 4u; k++)
            if (sp_knob(k, b, sizeof b)) {
                sp_add(n++ ? ", " : ". ");
                sp_add(b);
            }
        return;
    }
    sp_add(ui.home ? "HOME" : cur_page()->title);
    if (ui.home || (cur_page()->scope != SC_TRK && cur_page()->graph != GR_MIDICH &&
                    cur_page()->graph != GR_PATGRID && cur_page()->graph != GR_SONG)) {   /* (MIXER, MIDI OUT,
                                                         * PATTERNS: each knob says its track; SONG: KNOB 2 does) */
        sp_add(". T");
        b[0] = (char)('1' + song.sel % NTRK);
        b[1] = 0;
        sp_add(b);
    }
    if (!ui.home && cur_page()->graph == GR_SONG) {    /* SONG: how long it is */
        sg_frame();
        b[0] = 0;
        if (!chain_config.count) str_cpy(b, "NO SECTIONS YET", sizeof b);
        else {
            sp_num(b, sizeof b, (int32_t)chain_config.count);
            sp_cat(b, sizeof b, chain_config.count == 1u ? "SECTION," : "SECTIONS,");
            sp_num(b, sizeof b, (int32_t)sg.bars);
            sp_cat(b, sizeof b, sg.bars == 1u ? "BAR" : "BARS");
        }
        sp_add(". ");
        sp_add(b);
    }
    if (sp_list_row(b, sizeof b)) {                    /* a list: its row */
        sp_add(". ");
        sp_add(b);
    } else for (k = n = 0; k < 4u; k++)
        if (sp_knob(k, b, sizeof b)) {
            sp_add(n++ ? ", " : ". ");
            sp_add(b);
        }
    if (ui.home)                                        /* (HOME: the four tracks' sounds too, as it shows them) */
        for (k = 0; k < NTRK; k++) {
            char nm[16];
            trk_short_name(k, nm);
            sp_add(k ? ", T" : ". SOUNDS T");
            b[0] = (char)('1' + k);
            b[1] = ' ';
            str_cpy(b + 2, nm, sizeof b - 2u);
            sp_add(b);
        }
}

/* a step page (NOTES, DRUM HIT): the step under the cursor, its lane and what it holds; 0 not a step page */
static uint32_t sp_step_sig(void)
{
    const uint8_t *p;
    uint32_t i, h = 2166136261u;
    if (ui.home || ui.menu || ui.layer || (cur_page()->scope != SC_STEP && cur_page()->scope != SC_DRUMHIT))
        return 0;
    p = (const uint8_t *)&TSEL->step[ui.cursor % NSTEP];
    for (i = 0; i < sizeof(step_t); i++)
        h = (h ^ p[i]) * 16777619u;
    return (h ^ ui.cursor ^ (uint32_t)ui.lane << 8) | 1u;
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
            else if (!sp_pop_row(sp.knob, sizeof sp.knob)) sp_list_row(sp.knob, sizeof sp.knob);
        } else if (ui.menu == 1 || sp.turned || pop.on == POP_SHEET || pop.on == POP_LIST) {   /* a menu or sheet
                                                         * row, a knob turned: its name and value, when they change (a
                                                         * value moving by itself is not said) */
            char b[48];
            if (ui.menu == 1) sp_menu_row(b, sizeof b);
            else if (sp_pop_row(b, sizeof b)) ;
            else if (!sp_list_row(b, sizeof b) && !sp_knob(sp.turned - 1u, b, sizeof b)) b[0] = 0;
            sp_ascii(b);
            if (b[0] && !str_eq(b, sp.knob)) {
                str_cpy(sp.knob, b, sizeof sp.knob);
                ann_later(b, ANN_NOW, SP_KNOB_MS, now);
            }
        } else if (!playing && sp_step_sig() != sp.step) {   /* a step key (stopped): the step, as the knobs show it */
            uint32_t k, n;
            sp_buf[0] = 0;
            for (k = n = 0; k < 4u; k++) {
                char b[48];
                if (sp_knob(k, b, sizeof b)) {
                    if (n++) sp_add(", ");
                    sp_add(b);
                }
            }
            if (sp_buf[0]) ann_later(sp_buf, ANN_NOW, SP_KNOB_MS, now);
        }
        if (playing != sp.playing)
            ann_say(playing ? "PLAY" : "STOP", 0, ANN_NOW);
        sp.started = 1;
    } else {
        sp.started = 0;
    }
    sp.ctx = ctx;
    sp.step = sp_step_sig();
    sp.turned = 0;
    sp.playing = (uint8_t)playing;
    ann_tick(now);
}
