/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The screen reader (MELODEE_ACCESS, firmware/src/ui_speak.c, fm1-reader's protocol in announce.c) on
 * tests/ui_test.c's stubs, in the Mono look as shipped: silent until a reader's HELLO; HELLO answered with the
 * firmware's name; the first HELLO says the whole screen; a page change says the page and its knobs once it
 * settles; a knob turning says its name and value once it is still; the menu's rows with their place; the
 * header's messages; PLAY and STOP; WHERE says the screen again; 5 s without a HELLO: silent again.
 * Built with -DMELODEE_ACCESS=1 -DMELODEE_LOOK=1; run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void reader(uint32_t cmd)                  /* a SysEx from the reader, as editor.c's ed_service hands it on */
{
    const uint8_t f[5] = {ANN_H0, ANN_H1, ANN_H2, (uint8_t)cmd, 1};
    ann_frame(f, cmd == ANN_HELLO ? 5u : 4u, fm1_ms);
}
static void said_clear(void) { host_said_n = 0; host_said[0] = 0; host_said_frames = 0; }
static int said(const char *s) { return strstr(host_said, s) != 0; }
static void settle(void) { uint32_t i; for (i = 0; i < 20u; i++) frame(); }   /* 320 ms: what waits is said */

/* speak_test dump FILE: what the reader would hear on every screen (the audit's input, fm1-reader tools/audit.mjs):
 * SCREEN each page as a page change says it, for every engine and MODE (as ui_test's help check walks them) and
 * HOME; KNOB each knob; ROW each menu row; VALUE every name a list value can take (each parameter's, each engine's).
 * The messages (MSG) come from the sources (tools/speak_dump.py). Sorted and made unique by the caller. */
static FILE *dump_f;
static void dump_pages(void)
{
    char b[48];
    uint32_t i, k;
    for (i = 0; i <= NPAGES; i++) {
        ui.home = i == NPAGES;
        if (!ui.home) {
            if (!page_visible(i)) continue;
            ui.page = (uint8_t)i;
        }
        ui.force = 1;
        ui_draw();
        sp_screen();
        fprintf(dump_f, "SCREEN\t%s\n", sp_buf);
        for (k = 0; k < 4u; k++)
            if (sp_knob(k, b, sizeof b)) fprintf(dump_f, "KNOB\t%s\n", b);
        if (list_on()) {                                /* a list page: each row */
            for (lst.row = 0; lst.row < list_n(list_on()); lst.row++)
                if (sp_list_row(b, sizeof b)) fprintf(dump_f, "ROW\t%s\n", b);
            lst.row = 0;
        }
    }
    ui.home = 0;
}
static void dump_screen(void) { sp_screen(); fprintf(dump_f, "SCREEN\t%s\n", sp_buf); }
/* screens a state makes: a song, a pattern queued, each question, the layers, a page's sheet, a knob's list */
static void dump_extra(void)
{
    char b[48];
    uint32_t i, k;
    trk[0].pattern_next = 2;                            /* PATTERNS: a pattern waiting */
    go_title("PATTERNS");
    dump_screen();
    page_sheet_open();
    dump_screen();
    for (pop.sel = 0; pop.sel < pop.n; pop.sel++)
        if (sp_pop_row(b, sizeof b)) fprintf(dump_f, "ROW\t%s\n", b);
    pop_close();
    trk[0].pattern_next = NPAT;
    chain_config.count = 2;                             /* SONG: two sections */
    chain_config.row[0].repeat = 2;
    chain_config.row[1].repeat = 1;
    chain_patterns[1][0] = 1;
    go_title("SONG");
    for (ui.song_row = 0; ui.song_row <= 2u; ui.song_row++) {
        dump_screen();
        for (k = 0; k < 4u; k++)
            if (sp_knob(k, b, sizeof b)) fprintf(dump_f, "KNOB\t%s\n", b);
    }
    ui.song_row = 0;
    chain_config.count = 0;
    go_title("PROJECT");                                /* a slot page's sheet */
    page_sheet_open();
    dump_screen();
    for (pop.sel = 0; pop.sel < pop.n; pop.sel++)
        if (sp_pop_row(b, sizeof b)) fprintf(dump_f, "ROW\t%s\n", b);
    pop_close();
    go_title("SCALES");                                 /* the scale picker: the scale under the cursor */
    dump_screen();
    scale_picker_edit(1, 3);
    dump_screen();
    go_title("ARP");                                    /* a knob's list */
    vlist_open(ui.page, 0);
    dump_screen();
    pop_close();
    for (i = CF_NONE + 1u; i <= CF_PRINT; i++) {        /* each question */
        ui.confirm = (uint8_t)i;
        dump_screen();
    }
    ui.confirm = 0;
    for (i = LAYER_GLO; i < LAYER_N; i++) {             /* the layers */
        ui.layer = (uint8_t)i;
        dump_screen();
    }
    ui.layer = 0;
}
static int dump(const char *path)
{
    char b[48];
    uint32_t e, m, i, tr;
    int32_t v;
    if (!(dump_f = fopen(path, "w"))) return 1;
    ui_power_on();
    for (e = 0; e < NENGINES; e++) {                    /* every engine on STEP */
        if (!ENGINES[e] || !ENGINES[e]->name || str_eq(ENGINES[e]->name, "-")) continue;   /* (retired) */
        set_engine_of(TSEL, e);
        TSEL->engine = TSEL->eng_req;
        seqm_set(0, 0);
        fprintf(dump_f, "# engine %s\n", ENGINES[e]->name);
        dump_pages();
        for (i = 0; i < pid_count(); i++) {             /* the engine's list values */
            const param_desc_t *d = pid_desc(pid_at(i)->pid, 0);
            if (!d || d->fmt != F_ENUM || !d->names) continue;
            for (v = d->min; v <= d->max; v++)
                if (d->names[v] && d->names[v][0])
                    fprintf(dump_f, "VALUE\t%s %s%s%s\n", pid_at(i)->pid >= PID_TRACK(P_E0) && pid_at(i)->pid <
                            PID_TRACK(P_E0 + 8) && d->label ? d->label : pid_at(i)->name, d->names[v][0] == '/' ? "EVERY " : "",
                            d->names[v] + (d->names[v][0] == '/'), d->names[v][0] == '/' ? " STEPS" : "");   /* (as sp_knob says it) */
        }
    }
    set_engine_of(TSEL, ENGI_FM6);
    TSEL->engine = TSEL->eng_req;
    for (m = 0; m <= TESS_ENG_COUNT; m++) {            /* STEP, ARP, then every generator */
        seqm_set(0, (int32_t)m);
        dump_pages();
    }
    seqm_set(0, 0);
    song.sel = 3;                                       /* a DRUM track: STEP, ATLAS */
    set_engine_of(TSEL, ENGI_DRUM);
    TSEL->engine = TSEL->eng_req;
    seqm_set(3, 0);
    dump_pages();
    seqm_set(3, 1 + TESS_ENG_GRIDS);
    dump_pages();
    seqm_set(3, 0);
    song.sel = 0;
    dump_extra();
    ui.menu = 1;                                        /* the menu's rows */
    for (i = 0; i < MI_COUNT; i++) {
        ui.menu_sel = (uint8_t)i;
        sp_menu_row(b, sizeof b);
        fprintf(dump_f, "ROW\t%s\n", b);
    }
    ui.menu = 0;
    (void)tr;
    fclose(dump_f);
    return 0;
}

int main(int argc, char **argv)
{
    int bad = 0;
    if (argc > 2 && str_eq(argv[1], "dump"))
        return dump(argv[2]);
    ui_power_on();
    usb.config = 1;
    go_title("EVOLVE");
    settle();
    turn(EN_K1 + 2u, 2);
    settle();
    bad += check("speak: silent until a reader says HELLO (a DAW on the port sees nothing)", host_said_frames == 0u);

    reader(ANN_HELLO);
    frame();
    bad += check("speak: HELLO answered with the firmware's name and its words file (firmware/words.tsv)",
                 said("H Tessera TEST|" ANN_WORDS_URL "\n") && !strncmp(ANN_WORDS_URL, "https://", 8u));
    bad += check("speak: the first HELLO says the screen at once: page, track, knobs by their whole names",
                 said("N EVOLVE. T1. ") && said("AMOUNT"));
    printf("  said: %s", host_said);

    said_clear();
    turn(EN_K1 + 2u, 3);
    bad += check("speak: a knob turning is not said at once", !said("AMOUNT"));
    settle();
    bad += check("speak: ..once it is still: its name and its value", said("N STEP EVOLVE AMOUNT ") && said(" %"));
    printf("  said: %s", host_said);

    said_clear();
    go_title("TIMING");
    settle();
    bad += check("speak: another page: its name and knobs, once it settles", said("N TIMING. T1. "));
    printf("  said: %s", host_said);

    said_clear();
    go_title("EDIT 1");
    settle();
    bad += check("speak: an engine's knobs by the engine's own names, not ENGINE 1",
                 said("N EDIT 1. T1. ") && !said("ENGINE 1"));
    said_clear();
    go_title("MIXER");
    settle();
    bad += check("speak: MIXER: each knob says its track", said("N MIXER. T1 LEVEL ") && said(", T4 LEVEL "));
    said_clear();
    go_title("VOICE");
    settle();
    bad += check("speak: a list page: its row, with its place", said("N VOICE. T1. ") && said(", 1 OF "));
    said_clear();
    turn(EN_K1, 1);
    settle();
    bad += check("speak: ..KNOB 1 to the next row", said(", 2 OF "));
    said_clear();
    ui.home = 1;
    ui.force = 1;
    settle();
    bad += check("speak: HOME: its knobs and the four tracks' sounds", said("N HOME. T1. ") && said(". SOUNDS T1 ") &&
                 said(", T4 "));
    printf("  said: %s", host_said);
    ui.home = 0;
    ui.force = 1;

    reader(ANN_HELLO);                                  /* (a reader says HELLO every few seconds) */
    said_clear();                                       /* the pages that drew their own grid (slice 3) */
    go_title("PATTERNS");
    settle();
    bad += check("speak: PATTERNS: each track's pattern", said("N PATTERNS. T1 PATTERN 1 ") && said(", T4 PATTERN "));
    said_clear();
    chain_config.count = 1;
    chain_config.row[0].repeat = 2;
    go_title("SONG");
    settle();
    bad += check("speak: SONG: its length, then the section", said("N SONG. 1 SECTION, ") &&
                 said("SECTION A x2, 1 OF 1"));
    printf("  said: %s", host_said);
    chain_config.count = 0;
    said_clear();
    go_title("MODS");
    settle();
    bad += check("speak: MODS: its knobs as the page shows them", said("N MODS. T1. MOD SOURCE ") && said("MOD 1 OF "));
    said_clear();
    turn(EN_K1, 1);
    settle();
    bad += check("speak: ..a source: its name and its bar in words, not +-", said("MOD SOURCE T1 ") &&
                 (said("POLAR") && !said("+")));
    printf("  said: %s", host_said);
    turn(EN_K1, -1);
    said_clear();
    go_title("SCALES");
    settle();
    bad += check("speak: SCALES: the scale under the cursor", said("N SCALES. T1. FAMILY ") && said(", SCALE "));
    said_clear();
    turn(EN_K1 + 1u, 2);
    settle();
    bad += check("speak: ..KNOB 2: the next scale", said("N SCALE ") && !said("CHROMATIC"));
    printf("  said: %s", host_said);
    turn(EN_K1 + 1u, -2);
    said_clear();
    go_title("PRESETS");
    settle();
    bad += check("speak: PRESETS: the list, the sound with its place", said("N PRESETS. T1. LIST ") && said(" OF "));
    printf("  said: %s", host_said);
    said_clear();
    go_title("PROJECT");
    settle();
    bad += check("speak: a slot page: the slot with its place", said("N PROJECT. T1. A, ") && said(", 1 OF 6"));
    reader(ANN_HELLO);
    said_clear();
    ui.confirm = CF_CLEAR_SEQ;
    settle();
    bad += check("speak: a question: its words, then the keys", said("N Clear t1 sequence?") &&
                 said(" OCT+ YES, OCT- NO"));
    printf("  said: %s", host_said);
    ui.confirm = 0;
    said_clear();
    btn_down(B_GLO);                                    /* GLO held: its map */
    settle();
    settle();
    bad += check("speak: a layer: its name, its knobs", said("N GLO SET. T1 ") && said(", T4 "));
    printf("  said: %s", host_said);
    btn_up(B_GLO);
    settle();
    go_title("NOTES");
    settle();
    said_clear();
    ui.cursor = 3;
    settle();
    bad += check("speak: a step key, stopped: the step, as the knobs show it", said("STEP 4 OF "));
    printf("  said: %s", host_said);
    ui.cursor = 0;

    reader(ANN_HELLO);
    said_clear();
    ui_message("SAVED");
    bad += check("speak: a message in the header is said at once", said("N SAVED"));

    said_clear();
    song.playing = 1;
    frame();
    bad += check("speak: PLAY", said("N PLAY"));
    song.playing = 0;
    frame();
    bad += check("speak: STOP", said("N STOP"));

    said_clear();
    hold(B_HOME);
    settle();
    printf("  said: %s", host_said);
    bad += check("speak: the menu: its first row, with its place", said("N MENU. COLOR, 1 OF "));
    said_clear();
    turn(EN_K1, 1);
    settle();
    bad += check("speak: ..KNOB 1 to the next row: its name, value and place", said(", 2 OF "));
    printf("  said: %s", host_said);
    press(B_OCTDN);
    settle();

    reader(ANN_HELLO);                                  /* (a reader says HELLO every few seconds) */
    frame();
    said_clear();
    reader(ANN_WHERE);
    frame();
    bad += check("speak: WHERE says the whole screen again", host_said_n > 0u && host_said[0] == 'N');

    said_clear();
    fm1_ms += 6000u;
    frame();
    turn(EN_K1 + 2u, 1);
    settle();
    bad += check("speak: 5 s without a HELLO: silent again", host_said_frames == 0u);
    printf(bad ? "speak: FAILED\n" : "speak: all passed\n");
    return bad != 0;
}
