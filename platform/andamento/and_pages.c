/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's page map: SOUND, MOD and FX built from Tessera's own page table (params.c PAGES[]), grouped the
 * Monomachine way. Each sub-page is 8 encoders, two of the FM-1's 4-knob pages one over the other (row A over row
 * B), so a page and its companion come across whole, the engine's own pages included (FM6's operators, ORACLE's
 * patch) as the selected track's engine has them. Included by and_ui.c (a unity build) after the firmware: it uses
 * PAGES, page_desc, the put-backs of FM6's and ORACLE's pages (params.c), set_engine_of and apply_preset_to.
 *
 *   SOUND  S1..Sn   the engine's pages (EDIT 1 + EDIT 2 as the engine names them, then FM6's / ORACLE's / DRUM's
 *                   own), two at a time
 *          FILTER   the engine's filter (CUT, RES; ORACLE: its FILTER and FILT ENV), then ENV and LFO to the filter
 *                   and ENV TO SHAPE; none on an engine with envelopes of its own and no filter (FM6)
 *          AMP      the envelope (ORACLE: its AMP ENV; FM6: none, it has its own per operator), ENV TO PITCH,
 *                   SPREAD, PAN, LEVEL
 *          VOICE    ENGINE and PRESET, VOICE MODE and ALLOCATION, GLIDE, GLIDE MODE, PRIORITY, DETUNE
 *   MOD    LFO      LFO and LFO DEST
 *          LFO 2    LFO 2, and MACRO 1..4 (the project's cord sources)
 *          (engine) FM6's FM LFO pages, ORACLE's LFO MOD and LFO ROUTE
 *   FX     INS 1    FX 1 and FX 2 (the track's inserts)
 *          INS 2    FX 3 and the track's sends (one, or three in FX MODE's SPLIT)
 *          SEND 1   SEND FX 1 and 2 (the song's)
 *          SEND 2   SEND FX 3, FX RETURN and FX MODE
 *          MASTER, TEXTURE  Andamento's own (and_master.c, and_texture.c)
 *
 * A cell is a pid. Most of PAGES[]' columns are one already (a track's P_*, the song's G_*); a column of FM6's or
 * ORACLE's pages is a view of the engine's patch, so it is AP_PID + page * 4 + column and goes through page_desc and
 * the page's put-back as on the FM-1. ENGINE, PRESET and FM6's operator are AN_PID + AN_*. Not yet here: ORGANIC
 * (its knobs have no pids), MODS and MATRIX (full-screen pages, a later thread), the shared per-voice filter and amp
 * (new code in voice.c: andamento README). */

#define AP_PID 0xD000u                           /* a column of an engine's own page: AP_PID + page * 4 + column */
#define AN_PID 0xD800u                           /* Andamento's own: AN_PID + AN_* */
enum { AN_ENGINE, AN_PRESET, AN_FMOP, AN_NPRM };
#define AND_NSUB 12                              /* sub-pages a page can have (FM6's SOUND: 8 + FILTER AMP VOICE) */

typedef struct {
    char title[16];                              /* the header's: the two FM-1 pages' titles ("OSC TONE") */
    char tab[6];                                 /* the tab's: 4 letters */
    uint16_t c[8];                               /* the 8 encoders' cells, row A then row B (PID_NONE: empty) */
} and_sub_t;
static and_sub_t and_subs[AND_NSUB];
static uint32_t and_nsubs;

static int and_is_ap(uint32_t pid) { return pid >= AP_PID && pid < AP_PID + 4u * NPAGES; }
static int and_is_an(uint32_t pid) { return pid >= AN_PID && pid < AN_PID + AN_NPRM; }

/* ENGINE: the engines this build has, in ID order (a value is a place in this list) */
static const char *an_eng_name[NENGINES + 1u];
static uint8_t an_eng_id[NENGINES];
static uint32_t an_neng;
static void an_engines(void)
{
    uint32_t i;
    if (an_neng)
        return;
    for (i = 0; i < NENGINES; i++)
        if (strcmp(N_ENGNAME[i], "-"))
            an_eng_id[an_neng] = (uint8_t)i, an_eng_name[an_neng++] = N_ENGNAME[i];
    an_eng_name[an_neng] = 0;
}
static uint32_t an_eng_pos(uint32_t e)
{
    uint32_t k;
    an_engines();
    for (k = 0; k < an_neng; k++)
        if (an_eng_id[k] == e)
            return k;
    return 0;
}
static const char *const AN_OPS[] = {"OP1", "OP2", "OP3", "OP4", "OP5", "OP6", 0};

static int32_t an_get(uint32_t x, uint32_t tr);
static const param_desc_t *an_desc(uint32_t x, uint32_t tr)
{
    static param_desc_t d;
    const engine_t *e = ENGINES[trk[tr % NTRK].eng_req % NENGINES];
    an_engines();
    if (x == AN_ENGINE)
        d = (param_desc_t){"ENGINE", F_ENUM, 0, (int16_t)(an_neng - 1u), (int16_t)an_get(x, tr), an_eng_name, 0};
    else if (x == AN_PRESET)                     /* (its name: an_preset_name; pushed, the one loaded stays) */
        d = (param_desc_t){"PRESET", F_INT, 0, (int16_t)(e->npresets ? e->npresets - 1u : 0u), (int16_t)an_get(x, tr),
                           0, 0};
    else if (x == AN_FMOP && trk[tr % NTRK].eng_req == ENGI_FM6)
        d = (param_desc_t){"OP", F_ENUM, 0, 5, 0, AN_OPS, 0};
    else
        return 0;
    return &d;
}
static int32_t an_get(uint32_t x, uint32_t tr)
{
    track_t *t = &trk[tr % NTRK];
    return x == AN_ENGINE ? (int32_t)an_eng_pos(t->eng_req) : x == AN_PRESET ? t->preset : x == AN_FMOP ? fm6_opsel % 6u : 0;
}
static void an_set(uint32_t x, uint32_t tr, int32_t v)
{
    track_t *t = &trk[tr % NTRK];
    const param_desc_t *d = an_desc(x, tr);
    if (!d)
        return;
    v = clamp(v, d->min, d->max);
    if (x == AN_ENGINE && an_eng_id[v] != t->eng_req)
        set_engine_of(t, an_eng_id[v]);          /* (the engine's defaults and its first preset, as the FM-1's knob) */
    else if (x == AN_PRESET && v != t->preset)
        apply_preset_to(t, (uint32_t)v);
    else if (x == AN_FMOP)
        fm6_opsel = (uint8_t)v;
}
/* the preset's name (0: not PRESET); short: its first word, at most 5 letters (a cell's; the whole name is the
 * picture's when it is turned) */
static const char *an_preset_name(uint32_t pid, uint32_t tr, int shrt)
{
    static char b[8];
    const engine_t *e = ENGINES[trk[tr % NTRK].eng_req % NENGINES];
    const char *n;
    uint32_t k;
    if (pid != AN_PID + AN_PRESET || !e->npresets)
        return 0;
    n = eng_preset(e, trk[tr % NTRK].preset % e->npresets)->name;
    if (!shrt)
        return n;
    for (k = 0; k < 5u && n[k] && n[k] != ' '; k++)
        b[k] = n[k];
    b[k] = 0;
    return b;
}

/* a column of an engine's page (AP_PID): as the FM-1's knob has it (params.c page_desc, the selected track's) */
static const param_desc_t *ap_desc(uint32_t x, int16_t **vp)
{
    *vp = 0;
    return x / 4u < NPAGES ? page_desc(&PAGES[x / 4u], x % 4u, vp) : 0;
}
static int32_t ap_get(uint32_t x)
{
    int16_t *vp;
    const param_desc_t *d = ap_desc(x, &vp);
    return d && vp ? *vp : 0;
}
static void ap_set(uint32_t x, int32_t v)
{
    int16_t *vp;
    const page_t *pg = &PAGES[x / 4u % NPAGES];
    const param_desc_t *d = ap_desc(x, &vp);
    if (!d || !vp || d->max <= d->min)
        return;
    v = enum_orig(d, clamp(v, d->min, d->max));
    *vp = (int16_t)v;
    if (pg->scope == SC_FM6 || pg->scope == SC_FMOP)
        fm6_page_put(pg, x % 4u, v);             /* (a copy of the patch's value: written back there) */
    else if (pg->scope == SC_ORACLE)
        or_page_put(pg, x % 4u, v);
}
static const char *ap_name(uint32_t x)          /* "OSC A FREQ" */
{
    static char b[24];
    int16_t *vp;
    const param_desc_t *d = ap_desc(x, &vp);
    uint32_t n;
    str_cpy(b, PAGES[x / 4u % NPAGES].title, sizeof b);
    n = (uint32_t)strlen(b);
    if (d && d->label && n + 2u < sizeof b)
        b[n] = ' ', str_cpy(b + n + 1u, d->label, (unsigned)(sizeof b - n - 1u));
    return b;
}

/* ------------------------------------------------------------ the map --- */
/* page i as Andamento shows it for the selected track: Tessera's own rules (ui.c page_visible) for an engine's pages,
 * without the FM-1's (its family's lanes, a page left open) */
static int ap_page_ok(uint32_t i)
{
    const page_t *p = &PAGES[i];
    uint32_t e = TSEL->eng_req;
    if (p->graph == GR_MOD || p->graph == GR_SLICES || p->graph == GR_FMSTORE)
        return 0;
    if (p->scope == SC_FM6 || p->scope == SC_FMOP)
        return MELODEE_FM6 && e == ENGI_FM6;
    if (p->scope == SC_ORACLE)
        return MELODEE_ORACLE && e == ENGI_ORACLE;
    if (p->scope == SC_DRUM || p->scope == SC_DRUMHIT)  /* (DRUM SOUND, DRUM MIX: not values, the drum patch's) */
        return 0;
    if (p->fam == FAM_EDIT && p->scope == SC_TRACK && p->id[0] >= P_LN0 && p->id[0] <= P_LN7)
        return e == ENGI_DRUM;                  /* LANES */
    if (p->fam == FAM_EDIT && p->id[0] >= P_FM1_ATK && p->id[0] <= P_FM4_LEVEL)
        return 0;                                /* (the old 4-operator pages: hidden on the FM-1 too) */
    return 1;
}
static uint32_t ap_find(const char *title)       /* a page by its title (NPAGES: none) */
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (!strcmp(PAGES[i].title, title))
            return i;
    return NPAGES;
}
/* column s of page i as a cell: its pid, or AP_PID's; PID_NONE when it has nothing to set (an engine's unused knob,
 * an action) */
static const param_desc_t *and_desc(uint32_t pid, uint32_t tr);
static uint16_t ap_cell(uint32_t i, uint32_t s)
{
    const page_t *p = &PAGES[i];
    uint32_t id = p->id[s & 3u], c;
    const pid_entry_t *e;
    const param_desc_t *d;
    if (i >= NPAGES || id == 0xFFu)
        return PID_NONE;
    if (p->scope == SC_TRACK || p->scope == SC_ENGINE)
        c = PID_TRACK(id);
    else if (p->scope == SC_GLOBAL)
        c = id == G_TRK_SEND ? PID_TRACK(P_FXSEND) : id == G_TRK_RECQ ? PID_TRACK(P_RECQ) : PID_GLOBAL(id);
    else if (p->scope == SC_FM6 || p->scope == SC_FMOP || p->scope == SC_ORACLE)
        c = AP_PID + i * 4u + (s & 3u);
    else
        return PID_NONE;
    if (c < AP_PID && (!(e = pid_find(c)) || (e->flags & PID_F_ACTION)))
        return PID_NONE;
    d = and_desc(c, song.sel);
    return d && d->max > d->min ? (uint16_t)c : PID_NONE;
}
static void ap_row(uint16_t *row, uint32_t i)    /* page i's 4 columns (NPAGES: none) */
{
    uint32_t s;
    for (s = 0; s < 4u; s++)
        row[s] = i < NPAGES ? ap_cell(i, s) : PID_NONE;
}
static int ap_row_any(const uint16_t *row) { return row[0] != PID_NONE || row[1] != PID_NONE || row[2] != PID_NONE ||
                                                    row[3] != PID_NONE; }
/* a cell as a pid that is a value here (PID_NONE when not) */
static uint16_t an_cell(uint32_t pid)
{
    const param_desc_t *d = and_desc(pid, song.sel);
    return d && d->max > d->min ? (uint16_t)pid : PID_NONE;
}

/* a sub-page from two rows: empty rows close up (row B moves up when A has nothing), unless keep (AMP: LEVEL stays
 * on encoder 8 whatever the engine) */
static void and_sub_rows(const char *title, const char *tab, const uint16_t *a, const uint16_t *b, int keep)
{
    and_sub_t *s;
    uint32_t k;
    if (and_nsubs >= AND_NSUB || (!ap_row_any(a) && !ap_row_any(b)))
        return;
    s = &and_subs[and_nsubs++];
    str_cpy(s->title, title, sizeof s->title);
    str_cpy(s->tab, tab, sizeof s->tab);
    if (!keep && !ap_row_any(a))
        a = b, b = 0;
    for (k = 0; k < 4u; k++)
        s->c[k] = a[k], s->c[4u + k] = b ? b[k] : PID_NONE;
}
static void and_sub_add(const char *title, const char *tab, const uint16_t *a, const uint16_t *b)
{
    and_sub_rows(title, tab, a, b, 0);
}
/* a title of two FM-1 pages, "OSC TONE"; an engine's EDIT 1 / 2 by the names it gives them */
static const char *ap_title(uint32_t i)
{
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    if (i >= NPAGES)
        return "";
    if (PAGES[i].scope == SC_ENGINE)
        return e->page_title[PAGES[i].id[0] == P_E0 ? 0 : 1] ? e->page_title[PAGES[i].id[0] == P_E0 ? 0 : 1]
                                                              : PAGES[i].title;
    return PAGES[i].title;
}
static void ap_pair_title(char *t, uint32_t a, uint32_t b)
{
    uint32_t n = 0;
    t[0] = 0;
    if (a < NPAGES && PAGES[a].scope == SC_FMOP)   /* (an operator's: which one) */
        str_cpy(t, AN_OPS[fm6_opsel % 6u], 16), str_cpy(t + 3, " ", 13), n = 4;
    str_cpy(t + n, ap_title(a), 16 - n);
    n = (uint32_t)strlen(t);
    if (b < NPAGES && n + 2u < 16u)
        t[n] = ' ', str_cpy(t + n + 1u, ap_title(b), 15 - n);
}

/* SOUND's synth pages: the engine's FAM_EDIT pages (the others are FILTER's, AMP's, VOICE's, MOD's), two at a time */
static int ap_synth_page(uint32_t i)
{
    static const char *const NOT[] = {"VOICE", "VOICE 2", "VOICE 3", "FILTER", "FILT ENV", "AMP ENV", "LFO MOD",
                                      "LFO ROUTE", "FM LFO", "FM LFO 2", 0};
    uint32_t k;
    uint16_t row[4];
    if (PAGES[i].fam != FAM_EDIT || !ap_page_ok(i))
        return 0;
    for (k = 0; NOT[k]; k++)
        if (!strcmp(PAGES[i].title, NOT[k]))
            return 0;
    ap_row(row, i);
    return ap_row_any(row);
}
static void and_map_sound(void)
{
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    uint32_t i, k, n = 0, pg[2], np = 0, ownenv = e->ownenv, oracle = TSEL->eng_req == ENGI_ORACLE;
    uint16_t a[4], b[4];
    char t[16], tab[6];
    for (i = 0; i <= NPAGES; i++) {             /* (a pair is two pages of one kind: the engine's EDIT 1 and 2, FM6's
                                                  * patch, its operator's, ORACLE's; one left over has a row alone) */
        uint32_t brk;
        if (i < NPAGES && !ap_synth_page(i))
            continue;
        brk = np == 1u && (i == NPAGES || PAGES[i].scope != PAGES[pg[0]].scope);
        if (i < NPAGES && !brk)
            pg[np++] = i;
        if (np == 2u || brk || (i == NPAGES && np)) {
            uint32_t s0 = and_nsubs;
            ap_row(a, pg[0]);
            ap_row(b, np == 2u ? pg[1] : NPAGES);
            ap_pair_title(t, pg[0], np == 2u ? pg[1] : NPAGES);
            tab[0] = 'S', tab[1] = (char)('1' + n), tab[2] = 0;
            and_sub_add(t, tab, a, b);
            if (and_nsubs > s0) {                /* (FM6's operator: on the first empty cell of its last pair) */
                and_sub_t *s = &and_subs[and_nsubs - 1u];
                if (PAGES[pg[np - 1u]].scope == SC_FMOP)
                    for (k = 0; k < 8u; k++)
                        if (s->c[k] == PID_NONE && (k == 7u || s->c[k + 1u] == PID_NONE)) {
                            s->c[k] = an_cell(AN_PID + AN_FMOP);
                            break;
                        }
                n++;
            }
            np = 0;
            if (brk && i < NPAGES)               /* (page i starts the next pair) */
                pg[np++] = i;
        }
    }
    /* FILTER: the engine's own (CUT, RES ..), then what moves it */
    for (k = 0; k < 4u; k++)
        a[k] = b[k] = PID_NONE;
    if (oracle) {
        ap_row(a, ap_find("FILTER"));
        ap_row(b, ap_find("FILT ENV"));
    } else {
        uint32_t m = 0;
        for (k = 0; k < 8u && m < 4u; k++) {
            const char *l = e->edit[k].label;
            if (l && (str_prefix(l, "CUT") || str_prefix(l, "RES") || str_prefix(l, "FLT") || str_prefix(l, "FILT")))
                a[m++] = an_cell(PID_TRACK(P_E0 + k));
        }
        if (!ownenv)
            b[0] = an_cell(PID_TRACK(P_ED_FLT)), b[1] = an_cell(PID_TRACK(P_LD_FLT)),
            b[2] = an_cell(PID_TRACK(P_ED_SHP));
    }
    and_sub_add("FILTER", "FILT", a, b);
    /* AMP: the envelope, then where the voice sits */
    for (k = 0; k < 4u; k++)
        a[k] = PID_NONE;
    if (oracle)
        ap_row(a, ap_find("AMP ENV"));
    else if (!ownenv)
        ap_row(a, ap_find("ENV"));
    b[0] = ownenv ? PID_NONE : an_cell(PID_TRACK(P_ED_PIT));
    b[1] = an_cell(PID_TRACK(P_SPRD)), b[2] = an_cell(PID_TRACK(P_PAN)), b[3] = an_cell(PID_TRACK(P_LEVEL));
    and_sub_rows("AMP", "AMP", a, b, 1);
    /* VOICE: the engine and its preset, then how its voices play */
    a[0] = an_cell(AN_PID + AN_ENGINE), a[1] = an_cell(AN_PID + AN_PRESET);
    a[2] = an_cell(PID_TRACK(P_VOICE)), a[3] = an_cell(PID_TRACK(P_ALLOC));
    b[0] = an_cell(PID_TRACK(P_GLIDE)), b[1] = an_cell(PID_TRACK(P_GLMODE));
    b[2] = an_cell(PID_TRACK(P_PRIO)), b[3] = an_cell(PID_TRACK(P_DETUNE));
    and_sub_add("VOICE", "VOIC", a, b);
}

static void and_map_mod(void)
{
    uint32_t e = TSEL->eng_req, k;
    uint16_t a[4], b[4];
    ap_row(a, ap_find("LFO"));
    ap_row(b, ap_find("LFO DEST"));
    and_sub_add("LFO DEST", "LFO", a, b);
    ap_row(a, ap_find("LFO 2"));
    for (k = 0; k < 4u; k++)
        b[k] = an_cell(PID_TESS(0x8du + k));     /* MACRO 1..4 (0x28d..: pid.c), the project's */
    and_sub_add("LFO 2 MACRO", "LFO2", a, b);
    if (MELODEE_FM6 && e == ENGI_FM6) {
        ap_row(a, ap_find("FM LFO"));
        ap_row(b, ap_find("FM LFO 2"));
        and_sub_add("FM LFO", "FM", a, b);
    } else if (MELODEE_ORACLE && e == ENGI_ORACLE) {
        ap_row(a, ap_find("LFO MOD"));
        ap_row(b, ap_find("LFO ROUTE"));
        and_sub_add("LFO MOD ROUTE", "POLY", a, b);
    }
}

static void and_map_fx(void)
{
    static const uint16_t MASTER[8] = {AM_C(AM_TILT), AM_C(AM_THRESH), AM_C(AM_RATIO), AM_C(AM_ATTACK),
                                       AM_C(AM_RELEASE), AM_C(AM_MAKEUP), AM_C(AM_MIX), PID_NONE};
    static const uint16_t TEXTURE[8] = {AT_C(AT_SIZE), AT_C(AT_DENS), AT_C(AT_POS), AT_C(AT_SPREAD), AT_C(AT_PITCH),
                                        AT_C(AT_FREEZE), AT_C(AT_MIX), PID_NONE};
    uint16_t a[4], b[4];
    ap_row(a, ap_find("FX 1"));
    ap_row(b, ap_find("FX 2"));
    and_sub_add("FX 1 FX 2", "INS1", a, b);
    ap_row(a, ap_find("FX 3"));
    ap_row(b, song.g[G_FXMODE] ? ap_find("FX SENDS") : NPAGES);   /* (one send, or three in SPLIT) */
    if (!song.g[G_FXMODE])
        b[0] = an_cell(PID_TRACK(P_FXSEND));
    and_sub_add("FX 3 SENDS", "INS2", a, b);
    ap_row(a, ap_find("SEND FX 1"));
    ap_row(b, ap_find("SEND FX 2"));
    and_sub_add("SEND FX 1 2", "SND1", a, b);
    ap_row(a, ap_find("SEND FX 3"));
    b[0] = an_cell(PID_GLOBAL(G_FXRETURN)), b[1] = an_cell(PID_GLOBAL(G_FXMODE)), b[2] = b[3] = PID_NONE;
    and_sub_add("SEND FX 3", "SND2", a, b);
    and_sub_add("MASTER", "MSTR", MASTER, MASTER + 4);   /* (song-wide: and_master.c) */
    and_sub_add("TEXTURE", "TEXT", TEXTURE, TEXTURE + 4);   /* (song-wide: and_texture.c) */
}

/* the fixed pages, GEN's second, PIECE's and MIX's (GEN's first: and_gen_cells) */
static const uint16_t AND_FIXED_GEN2[8] = {0x20f, 0x20e, 0x20d, 0x210, 0x211, 0x212, 0x26e, 0x26a};
static const uint16_t AND_FIXED_PIECE[2][8] = {
    {0x280, 0x281, 0x282, 0x284, 0x259, 0x25a, 0x27b, 0x27d},   /* PIECE, ENERGY ARC RESTS, ENTRIES */
    {0x272, 0x273, 0x274, 0x27c, 0x27e, 0x26d, 0x283, 0x279},   /* CHAIN, ENTRIES, DEPTH, COUNT, FOLLOW */
};
static const uint16_t AND_FIXED_MIX[8] = {0x000, 0x027, 0x028, 0x100, 0x101, 0x019, 0x01a, 0x01c};

/* page p's sub-pages for the selected (internal) track, into and_subs */
static void and_gen_cells(uint16_t *c);
static void and_map(uint32_t p)
{
    and_nsubs = 0;
    if (p == AB_GEN) {
        uint16_t c[8];
        and_gen_cells(c);
        and_sub_add("", "GEN", c, c + 4);         /* (the title: the generator's name, and_draw) */
        and_sub_add("EVOLVE", "EVLV", AND_FIXED_GEN2, AND_FIXED_GEN2 + 4);
    } else if (p == AB_SOUND) {
        and_map_sound();
    } else if (p == AB_MOD) {
        and_map_mod();
    } else if (p == AB_FX) {
        and_map_fx();
    } else if (p == AB_PIECE) {
        and_sub_add("PIECE", "PIEC", AND_FIXED_PIECE[0], AND_FIXED_PIECE[0] + 4);
        and_sub_add("CHAIN", "CHAN", AND_FIXED_PIECE[1], AND_FIXED_PIECE[1] + 4);
    } else if (p == AB_MIX) {
        and_sub_add("MIX", "MIX", AND_FIXED_MIX, AND_FIXED_MIX + 4);
    }
}
