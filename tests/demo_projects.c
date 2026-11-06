/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Tessera 2.0's demo projects (demos/), made by the firmware's own code: each one set up through the functions the
 * pages call (a new project, the sounds, tessera_param_set, cords), saved into a slot of simulated NOR flash by
 * project_save_as, read back as the backup page reads a slot (editor_backup.c, object 2..5: the TSP1 record), loaded
 * into a fresh project and played. Built as the device builds it (tests/run_tests.sh: -DMELODEE_SID=0
 * -DMELODEE_PHYS=0, the power-on of TRK_START_TESS).
 *   build/host/demo_projects make DIR [SECS]   the records (DIR/<name>.tsp) and SECS s of each (DIR/<name>.wav)
 *   build/host/demo_projects check DIR         every demo record in DIR (demos/) restores through the backup page's PUT, loads
 *                                              and plays: a format change that breaks the shipped demos fails here
 *   build/host/demo_projects archive OUT.json  a new device's whole backup, as the editor's Complete backup saves it
 *   build/host/demo_projects restore IN.json   such a backup restored as the editor restores it, then each slot loaded
 *                                              and played (tools/demo_backup.py's test: the demos put into a backup)
 *   build/host/demo_projects levels [SECS]     each track alone: its level, to balance a demo by
 *   build/host/demo_projects video OUTDIR      the demo video's audio, screens and captions (tools/demo_video.py)
 *   build/host/demo_projects log N SECS        demo N's notes as CSV (time, track, note, velocity)
 *   build/host/demo_projects solo N T SECS [WAV] demo N with track T alone heard (0: all): the voices' cost, as a
 *                                              device CPU guide, and the render
 *   build/host/demo_projects presets           each engine's presets by number (the demos' sound())
 *   build/host/demo_projects form N SECS       demo N's long form: the CHAIN's sections, the tracks ENTRIES takes out, the
 *                                              piece's progress, the notes in each (the 3.3 demos)
 * Checks per demo: the record saves, reads back the same, restores and loads with every generator, cord and sound;
 * each track plays notes (a demo with a planned PIECE: over DM_PIECE_SECS, as it layers in); no sample at full scale;
 * the voices stay inside the budget. */
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#define TESSERA_NOTE_HOOK dm_note
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t v_enc[7];                               /* (the video's knob turns: detents waiting for ui_input) */
static int32_t fm1_enc_take(uint32_t e) { int32_t s = v_enc[e % 7u]; v_enc[e % 7u] = 0; return s; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_gamma_switch(uint32_t opt) { (void)opt; }   /* (lcd.c: the device's panel) */
static uint16_t dm_screen[240 * 240];                  /* the display as drawn (video: its frames) */
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    for (uint32_t j = 0; j < h && y + j < 240u; j++)
        for (uint32_t i = 0; i < w && x + i < 240u; i++) dm_screen[(y + j) * 240u + x + i] = (uint16_t)((c >> 8) | (c << 8));
}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    for (uint32_t j = 0; j < h && y + j < 240u; j++)
        for (uint32_t i = 0; i < w && x + i < 240u; i++) dm_screen[(y + j) * 240u + x + i] = p[j * w + i];
}
/* (what the drawing needs beside: tests/ui_test.c's HAL stubs) */
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL];
static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[3][FM1_NCOL], fm1_led_dim_mask[3];
#define FM1_TICKS_PER_US 1u
static uint32_t host_ticks;
static uint32_t fm1_ticks(void) { return host_ticks; }
static uint32_t v_pressed, v_note_edges;               /* (the video's button and key presses, as edges) */
static uint32_t fm1_input_edges(int x) { uint32_t p = v_pressed; (void)x; v_pressed = 0; return p; }
static uint32_t fm1_input_note_edges(void) { uint32_t n = v_note_edges; v_note_edges = 0; return n; }
static void fm1_wdt_feed(void) {}
#define SCOPE_N 512u                                     /* audio.c: the HOME oscilloscope */
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;
static struct { uint32_t stage, crash_magic, crash_audio, crash_stage, crash_page; } melodee_dbg;
#define DBG_CRASH_MAGIC 0x43524831u
#define FM1_CRASH_MAGIC 0x43525348u
static struct { uint32_t magic, count, vec, pc, rets, emu, dbg, sp, psr, icfg, uptime_ms, etm[4], early; } fm1_crash;
#ifndef MELODEE_VERSION
#define MELODEE_VERSION "v2.0"
#endif
#define MELODEE_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/icons.c"                       /* (the drawing: the video mode's screens) */
#include "../firmware/src/ui_graph.c"
#include "../firmware/src/ui_draw.c"
#include "../firmware/src/ui_menu.c"
#include "../firmware/src/ui_input.c"
#include "../firmware/src/ui_layer.c"

static uint8_t nor[0x100000], flash_ok = 1;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { memcpy(nor + off, src, n); return 0; }
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
#include "../firmware/src/fm6_store.c"                 /* (ui_input's STORE and SEND: the video drives ui_input) */

#define ED_BK_FLASH_PTR(off) (nor + (off))
enum { ED_BACKUP_LIST = 65, ED_BACKUP_GET, ED_BACKUP_PUT };
static uint8_t rep[4096];
static uint32_t rep_n;
static void ed_b(uint32_t v) { if (rep_n < sizeof rep) rep[rep_n++] = (uint8_t)(v & 127u); }
static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        uint32_t k = na > 7u ? 7u : na;
        if (!k || n + k > max || (m >> k))
            return 0;
        for (j = 0; j < k; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return na ? 0u : n;
}
static uint8_t ed_smp_buf[512] __attribute__((aligned(4)));
static int ed_flash_stop(void) { return transport_busy(); }
#include "../firmware/src/editor_backup.c"

#define ENGI_TRIO 6u

static int check(const char *what, int ok)
{
    printf("demos: %-80s %s\n", what, ok ? "ok" : "FAIL");
    return !ok;
}

/* ------------------------------------------------------------- the device --- */
static uint32_t dm_notes[NTRK];
static FILE *dm_log;                                  /* (DM_LOG: every note, for a look at what they play) */
static uint32_t dm_f;
static void dm_note(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t tr = trk_index(t);
    if (tr < NTRK) dm_notes[tr]++;
    if (dm_log && tr < NTRK) fprintf(dm_log, "%.2f,%u,%u,%u\n", (double)dm_f / FS, tr + 1u, note, vel);
}

/* power-on with empty flash, then NEW: the device's new project */
static void power_on(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    pattern_init();
    memset(proj_bank_len, 0, sizeof proj_bank_len);
    memset(proj_meta, 0, sizeof proj_meta);
    up_cache_reset(); native_cache_reset();
    memset(&persist_saved, 0, sizeof persist_saved);
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    panel = PANEL_DEFAULT;
    settings_init();
    host_tracks_init();
    fm6_init();
    fm1_ms = 0;
    transport_req = 0;
    ed_bk_valid = ed_bk_put = 0;
    tessera_reset();
    project_new_blank();
    song.master_q12 = 2048;                           /* (melodee_init's) */
}

/* the backup page's PUT of one object: begin, 256-byte chunks, commit; 0 done */
static void put32(uint8_t *a, uint32_t v) { for (uint32_t i = 0; i < 5u; i++) a[i] = (uint8_t)((v >> (7u * i)) & 127u); }
static uint32_t call(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    rep_n = 0;
    ed_backup_handle(cmd, a, n);
    return rep_n;
}
static uint32_t put_all(uint32_t id, const uint8_t *p, uint32_t len)
{
    uint8_t a[16 + 300];
    uint32_t off, crc = st_crc32(p, len);
    a[0] = 0; a[1] = (uint8_t)id; put32(a + 2, len); put32(a + 7, crc);
    call(ED_BACKUP_PUT, a, 12);
    if (rep[2]) return rep[2];
    for (off = 0; off < len; off += 256u) {
        uint32_t n = len - off > 256u ? 256u : len - off, k = 7, i;
        const uint8_t *q = p + off;
        a[0] = 1; a[1] = (uint8_t)id; put32(a + 2, off);
        while (n) {
            uint32_t g = n > 7u ? 7u : n, m = 0;
            for (i = 0; i < g; i++) m |= (uint32_t)(q[i] >> 7) << i;
            a[k++] = (uint8_t)m;
            for (i = 0; i < g; i++) a[k++] = q[i] & 127u;
            q += g; n -= g;
        }
        call(ED_BACKUP_PUT, a, k);
        if (rep[2]) return rep[2];
    }
    a[0] = 2; a[1] = (uint8_t)id;
    call(ED_BACKUP_PUT, a, 2);
    return rep[2];
}

/* ------------------------------------------------------------- the pages --- */
static void sound(uint32_t tr, uint32_t engine, uint32_t preset)   /* PRESETS: an engine's preset, with its sends */
{
    track_t *t = &trk[tr];
    engine_load_of(t, engine, preset);
    t->engine = t->eng_req;
}
static void tx(uint32_t tr, uint32_t x, int32_t v) { tessera_param_set(tr, x, v); }
static void gen(uint32_t tr, uint32_t mode, uint32_t seed)          /* GEN's MODE, EVOLVE's ROLL */
{
    tx(tr, TX_MODE, (int32_t)mode);
    tx(tr, TX_EV_SEED, (int32_t)seed);
}
static void scale(uint32_t root, uint32_t scl, uint32_t qnt)        /* SCL: the song's scale; ROOT on every track */
{
    for (uint32_t k = 0; k < NTRK; k++) {
        trk[k].p[P_ROOT] = (int16_t)root;
        trk[k].p[P_SCALE] = (int16_t)scl;
        trk[k].p[P_QUANT] = (int16_t)qnt;
    }
}
static void cord(uint32_t i, uint32_t src, uint32_t str, uint32_t dst, uint32_t dtr, int32_t amount, int32_t offset, uint32_t uni)
{
    cord_t c = {(uint8_t)src, (uint8_t)str, (uint8_t)dst, (uint8_t)dtr, (int8_t)amount, (int8_t)offset, (uint8_t)uni};
    cord_set(i, &c);
}
/* the send effects: 1 a reverb, 2 an echo (FX: TYPE MIX RATE CHAR) */
static void sends(int32_t rv_rate, int32_t rv_char, int32_t ec_rate, int32_t ec_char)
{
    song.g[G_MFXTYPE] = 8; song.g[G_MFXMIX] = 127; song.g[G_MFXRATE] = (int16_t)rv_rate; song.g[G_MFXCHAR] = (int16_t)rv_char;
    song.g[G_MFX2TYPE] = 6; song.g[G_MFX2MIX] = 127; song.g[G_MFX2RATE] = (int16_t)ec_rate; song.g[G_MFX2CHAR] = (int16_t)ec_char;
}
static void mixer(uint32_t tr, int32_t level, int32_t pan, int32_t reverb, int32_t echo)
{
    trk[tr].p[P_LEVEL] = (int16_t)level; trk[tr].p[P_PAN] = (int16_t)pan;
    trk[tr].p[P_FXSEND] = (int16_t)reverb; trk[tr].p[P_FXSEND2] = (int16_t)echo;
}
enum { DIV_4, DIV_8, DIV_16, DIV_32, DIV_8T, DIV_16T, DIV_2, DIV_1, DIV_2BAR, DIV_4BAR };   /* (params.c N_DIV) */
enum { SCL_MIN = 2, SCL_DOR = 3, SCL_IN = 70, SCL_YO, SCL_HIRA, SCL_IWATO, SCL_KUMOI, SCL_RYUKYU };   /* (scales.h N_SCALE) */
enum { FX_TINE = 8, FX_KALIMBA = 19, FX_SOFT_PAD = 25, FX_DEEP_PAD = 27, FX_RIN = 31, FX_FURIN = 33, FX_MOKUGYO = 35,
       FX_KOTO_PLUCK = 36, FX_KOTO_RING = 37, FX_KUGO_HARP = 39 };   /* FM6 presets (eng_fm6.c FM6_PRESETS) */
enum { TR_FAT_BASS = 0, TR_WARM_PAD = 5, TR_HOLLOW_PAD = 7, TR_SUB_BASS = 9, TR_BREATH_FLUTE = 11 };   /* TRIO's */
enum { DK_808_KIT, DK_909_KIT };                                   /* DRUM's */
enum { OR_WARM_PAD = 1, OR_FLUTE = 21, OR_DUB_CHORD = 23 };              /* ORACLE's (oracle_sounds.h) */
enum { FX_TINE_EP = 0, FX_FLUTE = 20 };
/* a STEP track's step i: one note (0: a rest) at velocity vel */
static void step_note(uint32_t tr, uint32_t i, uint8_t note, uint32_t vel)
{
    step_t *st = &trk[tr].step[i];
    memset(st, 0, sizeof *st);
    st->time = note ? ST_NOTE : ST_REST;
    st->n = note ? 1u : 0u;
    st->note[0] = note;
    st->vel = (uint8_t)(note ? vel : 0u);
}

/* ------------------------------------------------------------- the demos --- */
/* STILL TIDE: the ambient one. An ORACLE drone that breathes, a warm pad wandering and coming home (GRAVITY), an
 * electric piano falling as leaves through the FILTER effect, a sub bass varied from a line written in STEP (STEP as
 * seed); ENERGY's slow arc thins the generators over minutes and RESTS leaves the drone alone now and then. YO on D,
 * 72 BPM: nothing to clash, so it can run for an hour. (3.0: no bell on its own; the chimes are gone.) */
static void demo_still(void)
{
    song.g[G_BPM] = 72;
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    sound(1, ENGI_TRIO, TR_WARM_PAD);
    sound(2, ENGI_FM6, FX_TINE_EP);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    scale(2, SCL_YO, 0);
    sends(110, 90, 70, 60);

    /* T1: CHORD DRONE on ORACLE, held, every voice on its own breath */
    trk[0].p[P_CHRD] = 1;                             /* DIA3 (ORACLE: three units a voice of the budget's 16) */
    trk[0].p[P_VOIC] = 1;                             /* OPEN */
    gen(0, TESS_ENG_CHORD, 11);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 3); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    tx(0, TX_CH_OCT, -1); tx(0, TX_CH_BREATH, 80);
    mixer(0, 94, -10, 90, 0);

    /* T2: AWAKE at half notes, ringing to the next (LENGTH HIT); EVOLVE every 8 bars, GRAVITY pulls it home */
    trk[1].p[P_SDIV] = DIV_2;
    gen(1, TESS_ENG_AWAKE, 37);
    tx(1, TX_AWK_LENA, 7); tx(1, TX_AWK_LENB, 5); tx(1, TX_AWK_PROB, 55);
    tx(1, TX_NL_MODE, 2);
    tx(1, TX_EV_EVERY, 4); tx(1, TX_EV_AMOUNT, 35);   /* EVERY 8 bars */
    tx(1, TX_SD_GRAVITY, 40);
    mixer(1, 84, 20, 100, 30);

    /* T3: FALL (BOUNCE past 4 balls): leaves landing on an electric piano, through a FILTER insert whose cutoff a
     * TIDE opens and closes over minutes */
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_BOUNCE, 5);
    tx(2, TX_BNC_BALLS, 5); tx(2, TX_BNC_SPEED, 10); tx(2, TX_BNC_ANGLE, 45);
    tx(2, TX_TW_DROP, 45); tx(2, TX_TW_HUMAN, 40); tx(2, TX_TW_SHIFT, 12);
    trk[2].p[P_FXTYPE] = 10; trk[2].p[P_FXMIX] = 127; trk[2].p[P_FXRATE] = 70; trk[2].p[P_FXCHAR] = 40;   /* FILTER */
    mixer(2, 100, -30, 110, 70);

    /* T4: a sub bass line written in STEP (D, rests, A, G), played by TURING from it: the rhythm kept, the pitches
     * varied now and then */
    {
        static const uint8_t LINE[8] = {38, 0, 0, 45, 0, 43, 0, 0};   /* D2 . . A2 . G2 . . */
        for (uint32_t i = 0; i < 8u; i++) step_note(3, i, LINE[i], 96);
        trk[3].p[P_SLEN] = 8;
    }
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_TURING, 64);
    tx(3, TX_SD_SOURCE, 4); tx(3, TX_SD_KEEP, TESS_SD_RHYTHM);
    tx(3, TX_TUR_PROB, 15);
    tx(3, TX_TW_SHIFT, -12); tx(3, TX_NL_MODE, 2);
    mixer(3, 85, 0, 30, 0);

    /* ENERGY: the project's tide and rests */
    tx(0, TX_EN_ARC, 60); tx(0, TX_EN_RESTS, 70);
    /* MODS */
    cord(0, CS_TORPOR, 0, CD_E1, 0, 30, 0, 0);        /* T1 TIDE 1 -> T1 CUT (ORACLE's MACRO) */
    cord(1, CS_OCHD2, 0, CD_PAN, 0, 25, 0, 0);        /* T1 SWARM 2 -> T1 PAN */
    cord(2, CS_INERTIA, 2, CD_FX1_RATE, 2, 35, 0, 0); /* T3 TIDE 2 -> T3 FX1 RTE: the filter's cutoff */
    cord(3, CS_ENERGY, 0, CD_FX_SEND, 1, 30, 0, 0);   /* ENERGY -> T2 FX SEND */
    cord(4, CS_DRIFT, 2, CD_SF2_RATE, 0, 8, 0, 0);    /* T3 DRIFT -> SF2 RTE: the echo's time bends a little */
}

/* KOTO GARDEN: Zen-1's and FuMi-1's FM voices in HIRA on C. Two koto on QUAD's shared grid, one in eighths and one in
 * triplets, a column turning down so notes travel between them; an ORACLE flute on the infinity series in an ABAC
 * FORM whose returns develop; a mokugyo written in STEP, its conditions and accumulator making it a little different
 * each bar. 92 BPM */
static void demo_garden(void)
{
    song.g[G_BPM] = 92;
    sound(0, ENGI_FM6, FX_KOTO_PLUCK);
    sound(1, ENGI_FM6, FX_KOTO_RING);
    sound(2, ENGI_ORACLE, OR_FLUTE);
    sound(3, ENGI_FM6, FX_MOKUGYO);
    scale(0, SCL_HIRA, 0);
    sends(90, 80, 80, 55);

    /* T1, T2: QUAD rows, eighths against triplets; column 1 moves down every 8 steps, column 3 at random */
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_QUAD, 19);
    tx(0, TX_QD_DIR, 0); tx(0, TX_QD_PROB, 80); tx(0, TX_QD_RANGE, 2);
    tx(0, TX_QD_MOVE1, 1); tx(0, TX_QD_EVERY1, 5);
    tx(0, TX_QD_MOVE3, 3); tx(0, TX_QD_EVERY3, 6);
    tx(0, TX_TW_HUMAN, 30);
    mixer(0, 106, -25, 60, 20);
    trk[1].p[P_SDIV] = DIV_8T;
    gen(1, TESS_ENG_QUAD, 19);
    tx(1, TX_QD_DIR, 2); tx(1, TX_QD_PROB, 60);
    tx(1, TX_TW_SHIFT, 12); tx(1, TX_TW_HUMAN, 30);
    mixer(1, 108, 25, 70, 40);

    /* T3: SERIES at quarter notes, two degrees a step, ringing on; FORM ABAC, 4 bars a section, each return of A
     * developed a little (DEVELOP 40) */
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_SERIES, 50);
    tx(2, TX_SR_START, 17); tx(2, TX_SR_INTV, 2); tx(2, TX_SR_LOOP, 0);
    tx(2, TX_NL_MODE, 2); tx(2, TX_TW_DROP, 35); tx(2, TX_TW_HUMAN, 35);
    tx(2, TX_EV_EVERY, 3); tx(2, TX_FM_FORM, 2); tx(2, TX_FM_CONTRAST, 40); tx(2, TX_FM_DEVELOP, 40);
    mixer(2, 92, 0, 90, 30);

    /* T4: STEP, a mokugyo figure in sixteenths: the pickups only every other bar (1:2) or on the fourth (4:4), one
     * note climbing a degree a pass up to 4 and back (Accumulate, PING) */
    {
        static const uint8_t FIG[16] = {48, 0, 0, 0, 0, 0, 55, 0, 48, 0, 0, 0, 0, 52, 0, 55};
        for (uint32_t i = 0; i < 16u; i++) step_note(3, i, FIG[i], i % 8u ? 84 : 104);
        step_set_cond(&trk[3].step[13], SC_1_2);
        step_set_cond(&trk[3].step[15], SC_4_4);
        step_set_climb(&trk[3].step[6], 1);
        trk[3].p[P_SLEN] = 16;
        tx(3, TX_ACC_LIMIT, 4); tx(3, TX_ACC_MODE, TESS_ACC_PING);
    }
    tx(3, TX_MODE, TESS_ENG_OFF);                    /* (GEN: MODE STEP) */
    trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 117, 10, 40, 0);

    cord(0, CS_APATHY, 0, CD_QD_PROB, 0, -40, 0, 0);  /* T1 TIDE 3 -> T1 Q PROB: the koto thins and fills */
    cord(1, CS_OCHD4, 2, CD_E1, 2, 25, 0, 0);         /* T3 SWARM 4 -> T3 CUT (ORACLE's MACRO): the flute's breath */
    cord(2, CS_DRIFT, 1, CD_PAN, 1, 30, 0, 0);        /* T2 DRIFT -> T2 PAN */
}

/* LOW HARBOUR: the ambient dub. Slow and roomy, not a loop: ORACLE chord stabs on the offbeats, most of them dropped,
 * each voice breathing, through a FILTER that a TIDE opens over minutes and into a long dotted echo whose time drifts
 * like a tape's; a sub bass that walks and rests and is pulled home (GRAVITY); a 909 one-drop of rim and hats, the
 * pickups conditional; a flute answering now and then. ENERGY's arc and RESTS clear the floor for the echoes.
 * A dorian, 70 BPM */
static void step_hits(uint32_t tr, uint32_t i, uint32_t lanes, uint32_t vel)   /* a DRUM step: lanes' hits */
{
    step_t *st = &trk[tr].step[i];
    memset(st, 0, sizeof *st);
    st->time = lanes ? ST_NOTE : ST_REST;
    st->hit = (uint8_t)lanes;
    st->vel = (uint8_t)(lanes ? vel : 0u);
}
enum { L_BD = 1, L_SD = 2, L_CP = 4, L_CH = 8, L_OH = 16, L_TM = 32, L_RS = 64, L_CB = 128 };   /* DRUM's lanes */
static void demo_dub(void)
{
    song.g[G_BPM] = 70;
    sound(0, ENGI_ORACLE, OR_DUB_CHORD);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    sound(2, ENGI_DRUM, DK_909_KIT);
    sound(3, ENGI_FM6, FX_FLUTE);
    scale(9, SCL_DOR, 0);
    sends(118, 100, 111, 104);                        /* a big room; ECHO 321 ms: a dotted sixteenth at 70 BPM */

    /* T1: CHORD stabs on the offbeats, TWO's two chords over 2 bars, DROP leaving most out; BREATH swells each voice */
    trk[0].p[P_CHRD] = 1;                             /* DIA3 (ORACLE: three units a voice of the budget's 16) */
    gen(0, TESS_ENG_CHORD, 23);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_OFF);
    tx(0, TX_CH_BREATH, 60); tx(0, TX_TW_DROP, 70); tx(0, TX_TW_HUMAN, 20);
    or_patch[0][OR_DECAY_AMP] = 78; or_patch[0][OR_RELEASE_AMP] = 70;   /* (AMP ENV: short stabs; the echo is the tail) */
    trk[0].p[P_FXTYPE] = 10; trk[0].p[P_FXMIX] = 127; trk[0].p[P_FXRATE] = 64; trk[0].p[P_FXCHAR] = 50;   /* FILTER */
    mixer(0, 108, -15, 60, 120);

    /* T2: MARKOV sub bass in quarters, neighbours and rests; EVOLVE every 4 bars, GRAVITY pulls it back */
    trk[1].p[P_SDIV] = DIV_4;
    gen(1, TESS_ENG_MARKOV, 41);
    tx(1, TX_MKV_LEAP, 15); tx(1, TX_MKV_DENS, 55); tx(1, TX_MKV_SPAN, 1);
    tx(1, TX_TW_SHIFT, -12);
    tx(1, TX_EV_EVERY, 3); tx(1, TX_EV_AMOUNT, 30); tx(1, TX_SD_GRAVITY, 50);
    mixer(1, 92, 0, 10, 0);

    /* T3: 909 STEP, one bar of sixteenths: kick and rim on the third beat (one drop), hats on some offbeats by
     * condition, an open hat every other bar (1:2), a rim pickup on the fourth bar (4:4) */
    {
        static const uint8_t L[16] = {0, 0, L_CH, 0, 0, 0, L_CH, 0, L_BD | L_RS, 0, L_CH, 0, 0, 0, L_OH, L_RS};
        for (uint32_t i = 0; i < 16u; i++) step_hits(2, i, L[i], i == 8u ? 110 : 80);
        step_set_cond(&trk[2].step[2], SC_2_4);      /* (conditions, not chance: the same each time it plays) */
        step_set_cond(&trk[2].step[6], SC_1_2);
        step_set_cond(&trk[2].step[10], SC_3_4);
        step_set_cond(&trk[2].step[14], SC_1_2);
        step_set_cond(&trk[2].step[15], SC_4_4);
        trk[2].p[P_SLEN] = 16;
    }
    tx(2, TX_MODE, TESS_ENG_OFF);                    /* (GEN: MODE STEP) */
    trk[2].p[P_SDIV] = DIV_16;
    tx(2, TX_SUB_HUMAN, 12);
    mixer(2, 96, 10, 40, 70);

    /* T4: AWAKE, a flute (a melodica's part) answering in half notes now and then, into the echo */
    trk[3].p[P_SDIV] = DIV_2;
    gen(3, TESS_ENG_AWAKE, 58);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 7); tx(3, TX_AWK_PROB, 35);
    tx(3, TX_TW_SHIFT, 12);
    mixer(3, 98, 25, 90, 90);

    /* ENERGY: a slow arc, and rests that leave the echoes */
    tx(0, TX_EN_ARC, 45); tx(0, TX_EN_RESTS, 60);
    /* MODS */
    cord(0, CS_INERTIA, 0, CD_FX1_RATE, 0, 40, 0, 0); /* T1 TIDE 2 -> T1 FX1 RTE: the stabs' filter opens and closes */
    cord(1, CS_DRIFT, 0, CD_SF2_RATE, 0, 10, 0, 0);   /* T1 DRIFT -> SF2 RTE: the echo's time bends as a tape's */
    cord(2, CS_APATHY, 0, CD_FX_SEND2, 0, 30, 0, 1);  /* T1 TIDE 3 -> T1 SEND 2: more of the stabs into the echo, then less */
    cord(3, CS_OCHD3, 3, CD_PAN, 3, 35, 0, 0);        /* T4 SWARM 3 -> T4 PAN */
}

/* ---------------------------------------------- 3.3's long-form demos --- */
/* Three more, each built round the long form: the CHAIN (one FORM for the project, KEY B / KEY C), ENTRIES (tracks
 * dropping out and coming back on its phrase lines, by ROLE) and the PIECE (a LENGTH and the SHAPE it takes). */
enum { FC_OFF, FC_AB, FC_ABAC, FC_ABCB, FC_AABA, FC_ABCA, FC_AABC };   /* (pid.c TX_N_FORM) */
enum { PH_4, PH_8, PH_16 };                                            /* (TX_N_PHRASE) */
enum { RO_AUTO, RO_BASS, RO_RHYTHM, RO_HARMONY, RO_MELODY, RO_ANCHOR }; /* (entries.c ET_*) */
enum { SH_ARCH, SH_RISE, SH_FALL, SH_PLATEAU, SH_VALLEY, SH_WAVES, SH_BUILDS, SH_STEPS };   /* (planner.c PL_*) */
enum { END_STOP, END_LOOP, END_NEW, END_FREE };
enum { LEN_8 = 7, LEN_10 = 8, LEN_12 = 9, LEN_15 = 10, LEN_20 = 11, LEN_30 = 13 };   /* (planner.c PL_MIN: minutes) */
enum { SCL_PEN = 5, SCL_PHRY = 8 };
enum { FX_ORGAN = 6, FX_SOLID_BASS = 10, FX_CLAVINET = 13, FX_DRAWBARS = 14, FX_SYNC_LEAD = 17, FX_HARP = 18,
       FX_SAW_BASS = 22, FX_KOTO_WARM = 38 };
enum { TR_ARP_LEAD = 1, TR_STRING_MACH = 6, TR_SOFT_PAD = 8, TR_SOFT_LEAD = 10, TR_FIFTH_DRONE = 14 };
enum { OR_GLASS_PAD = 2, OR_KEYS = 11, OR_DRONE = 14, OR_SEQ_BLIP = 20 };
/* GLO > CHAIN: FORM, PHRASE, WANDER, each place's repeats; KEY B / KEY C in semitones */
static void chain_set(uint32_t form, uint32_t phrase, int32_t wander, const uint8_t rep[4], int32_t key_b, int32_t key_c)
{
    tx(0, TX_FC_FORM, (int32_t)form); tx(0, TX_FC_PHRASE, (int32_t)phrase); tx(0, TX_FC_WANDER, wander);
    for (uint32_t i = 0; i < 4u; i++) tx(0, TX_FC_REP1 + i, rep[i]);
    tx(0, TX_FC_KEYB, key_b); tx(0, TX_FC_KEYC, key_c);
}
/* GLO > ENTRIES: ENTRIES, DWELL, STYLE (0 CUT, 1 FADE), FADE bars; each track's ROLE */
static void entries_set(int32_t amount, int32_t dwell, int32_t style, int32_t fade, const uint8_t role[4])
{
    tx(0, TX_ET_AMOUNT, amount); tx(0, TX_ET_DWELL, dwell); tx(0, TX_ET_STYLE, style); tx(0, TX_ET_FADE, fade);
    for (uint32_t k = 0; k < 4u; k++) tx(k, TX_ET_ROLE, role[k]);
}
/* GLO > PIECE: LENGTH, SHAPE, PEAK (ARCH .. VALLEY) or COUNT (WAVES .. STEPS), END */
static void piece_set(int32_t length, int32_t shape, int32_t peak, int32_t count, int32_t end)
{
    tx(0, TX_PL_LENGTH, length); tx(0, TX_PL_SHAPE, shape); tx(0, TX_PL_PEAK, peak); tx(0, TX_PL_COUNT, count);
    tx(0, TX_PL_END, end);
}
/* a generator track's part in the CHAIN: how new its B and C are (CONTRAST), how its returns vary (DEVELOP), SKIP */
static void sections(uint32_t tr, int32_t contrast, int32_t develop, int32_t skip)
{
    tx(tr, TX_FM_CONTRAST, contrast); tx(tr, TX_FM_DEVELOP, develop); tx(tr, TX_FC_SKIP, skip);
}

/* (STEP, a chord on step i: up to four notes; 0 ends the list) */
static void step_chord(uint32_t tr, uint32_t i, const uint8_t *notes, uint32_t vel)
{
    step_t *st = &trk[tr].step[i];
    uint32_t n = 0;
    memset(st, 0, sizeof *st);
    while (n < 4u && notes[n]) st->note[n] = notes[n], n++;
    st->time = n ? ST_NOTE : ST_REST;
    st->n = (uint8_t)n;
    st->vel = (uint8_t)(n ? vel : 0u);
}
static void step_tie(uint32_t tr, uint32_t i)       /* (STEP: step i holds the note before it on) */
{
    step_t *st = &trk[tr].step[i];
    memset(st, 0, sizeof *st);
    st->time = ST_TIE;
}
enum { FX_SOFT_SWELL = 26, FX_BAMBOO = 32, FX_HYOSHIGI = 34 };
enum { TR_WIND = 12 };
enum { OR_SOFT_STRINGS = 3, OR_VOX = 18 };

/* SLOW LIGHT: the drone, made so nothing in it clicks or crowds the CPU. A low fifth on TRIO held for the whole piece
 * (CHORD DRONE, no BREATH: it is struck once and never again), a TRIO wind above it whose cutoff a TIDE moves like
 * weather, an ORACLE choir singing one long note every two bars (MORPH, MONO: one voice, gliding on) and FM6's SOFT
 * SWELL rising out of the drone now and then, high in the reverb. The CHAIN carries the upper voices to A in B (KEY B
 * +7: a C sharp over the D, the light changing) while the drone stays OWN on D; AABA in 16-bar phrases; ENTRIES lets
 * the choir and the swell come and go over 4 bars; a 15-minute piece in three WAVES, played again. D dorian, 54 BPM. */
static void demo_light(void)
{
    song.g[G_BPM] = 54;
    sound(0, ENGI_TRIO, TR_FIFTH_DRONE);
    sound(1, ENGI_TRIO, TR_WIND);
    sound(2, ENGI_ORACLE, OR_VOX);
    sound(3, ENGI_FM6, FX_SOFT_SWELL);
    scale(2, SCL_DOR, 0);
    sends(124, 112, 70, 40);                          /* the longest hall; a little slow echo */

    /* T1: the fifth, low D and A with the D above, written in STEP as one chord held by ties for 64 steps of 4 bars:
     * struck once at PLAY and never again (a held chord struck anew while it sounds is what clicked) */
    {
        static const uint8_t FIFTH[4] = {38, 45, 50, 0};
        step_chord(0, 0, FIFTH, 100);
        for (uint32_t i = 1; i < 64u; i++) step_tie(0, i);
        trk[0].p[P_SLEN] = 64;
    }
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_4BAR;
    mixer(0, 77, 0, 70, 0);

    /* T2: the wind, one D held as the fifth is */
    step_note(1, 0, 62, 100);
    for (uint32_t i = 1; i < 64u; i++) step_tie(1, i);
    trk[1].p[P_SLEN] = 64;
    tx(1, TX_MODE, TESS_ENG_OFF);
    trk[1].p[P_SDIV] = DIV_4BAR;
    mixer(1, 81, 0, 110, 20);

    /* T3: the choir, MORPH a note every 2 bars, ringing into the next, MONO; new lines in B, a return a little changed */
    trk[2].p[P_SDIV] = DIV_2BAR;
    gen(2, TESS_ENG_MORPH, 61);
    tx(2, TX_MRP_POS, 35); tx(2, TX_NL_MODE, 2); tx(2, TX_TW_DROP, 15);
    trk[2].p[P_VOICE] = 2;                            /* LEGATO: one ORACLE voice, the next note gliding on */
    sections(2, 50, 30, 0);
    mixer(2, 97, -20, 110, 15);

    /* T4: the swell, AWAKE in whole notes an octave up, most of them left out: a chord tone rising now and then */
    trk[3].p[P_SDIV] = DIV_1;
    gen(3, TESS_ENG_AWAKE, 22);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 3); tx(3, TX_AWK_PROB, 45);
    tx(3, TX_NL_MODE, 2); tx(3, TX_TW_SHIFT, 12); tx(3, TX_TW_DROP, 45);
    sections(3, 60, 30, 25);
    mixer(3, 112, 25, 127, 50);

    {
        static const uint8_t REP[4] = {1, 1, 1, 1}, ROLE[4] = {RO_ANCHOR, RO_ANCHOR, RO_HARMONY, RO_MELODY};
        chain_set(FC_AABA, PH_16, 10, REP, 0, 0);
        entries_set(40, 2, 1, 4, ROLE);
        piece_set(LEN_15, SH_WAVES, 50, 3, END_LOOP);
    }
    tx(0, TX_EN_ARC, 30);
    cord(0, CS_TORPOR, 0, CD_CUTOFF, 0, 25, 0, 0);    /* T1 TIDE 1 -> T1 CUT: the drone darkens and opens over minutes */
    cord(1, CS_INERTIA, 1, CD_CUTOFF, 1, 45, 0, 0);   /* T2 TIDE 2 -> T2 CUT: the wind rising and falling */
    cord(2, CS_OCHD2, 1, CD_PAN, 1, 40, 0, 0);        /* T2 SWARM 2 -> T2 PAN: the wind moving across */
    cord(3, CS_DRIFT, 2, CD_E1, 2, 20, 0, 0);         /* T3 DRIFT -> T3 CUT (ORACLE's MACRO): the choir's vowel */
    cord(4, CS_APATHY, 3, CD_FX_SEND2, 3, 25, 0, CU_UNI);   /* T4 TIDE 3 -> T4 SEND 2: the swell into the echo, then not */
}

/* CEDAR RAIN: plucked strings and a bamboo flute again, as KOTO GARDEN, but in KUMOI on E at 80 BPM and built another
 * way: a KUGO harp on TURING turning a 12-step figure in eighths (locked, a note now and then new), a koto under it on
 * EUCLID 16 / 5 in sixteenths, an octave down, a breath flute on MARKOV in half notes that is the song, and a pair of
 * 808 toms and a rim written in STEP like hand drums, ADORN filling the rests a little differently each round. The
 * CHAIN goes to A (KEY B +5) and to D (KEY C -2) and home, ABAC in 8-bar phrases (A twice), each B and C a new figure;
 * ENTRIES at 45 with FADE; a 12-minute ARCH, played again. */
static void demo_cedar(void)
{
    song.g[G_BPM] = 80;
    sound(0, ENGI_FM6, FX_KUGO_HARP);
    sound(1, ENGI_FM6, FX_KOTO_WARM);
    sound(2, ENGI_TRIO, TR_BREATH_FLUTE);
    sound(3, ENGI_DRUM, DK_808_KIT);
    scale(4, SCL_KUMOI, 0);
    sends(96, 84, 82, 60);

    /* T1: TURING, 12 eighths, nearly locked, two octaves of span; some notes dropped, played by hand */
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 47);
    tx(0, TX_TUR_LEN, 12); tx(0, TX_TUR_PROB, 6); tx(0, TX_TUR_SPAN, 2);
    tx(0, TX_TW_DROP, 30); tx(0, TX_TW_HUMAN, 35);
    tx(0, TX_NL_MODE, 1); tx(0, TX_NL_LEN, 3);        /* FIXED: a quarter's ring, so few voices sound at once */
    sections(0, 55, 30, 0);
    mixer(0, 107, -30, 80, 35);

    /* T2: the koto, EUCLID 16 / 5 in sixteenths, an octave down: the pulse under the harp */
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 29);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 5); tx(1, TX_EUC_ROT, 3);
    tx(1, TX_TW_SHIFT, -12); tx(1, TX_TW_HUMAN, 25);
    tx(1, TX_NL_MODE, 1); tx(1, TX_NL_LEN, 2);
    sections(1, 35, 20, 0);
    mixer(1, 102, 25, 55, 20);

    /* T3: the flute, MARKOV in half notes an octave up, one voice, close steps, rests between; B and C wander further */
    trk[2].p[P_SDIV] = DIV_2;
    gen(2, TESS_ENG_MARKOV, 12);
    tx(2, TX_MKV_LEAP, 10); tx(2, TX_MKV_DENS, 45); tx(2, TX_MKV_SPAN, 1);
    tx(2, TX_NL_MODE, 2); tx(2, TX_TW_HUMAN, 40); tx(2, TX_TW_SHIFT, 12);
    trk[2].p[P_VOICE] = 2;                            /* LEGATO */
    sections(2, 70, 35, 0);
    mixer(2, 83, 5, 105, 55);

    /* T4: 808 STEP, two bars of sixteenths: a low tom on one and the "and" of three, a high answer, rims; conditions
     * and ADORN (STEP EVOLVE) make each pass a little different */
    {
        static const uint8_t L[32] = {L_TM, 0, 0, 0, 0, 0, L_RS, 0, 0, 0, L_TM, 0, 0, 0, L_RS, 0,
                                      L_TM, 0, 0, L_RS, 0, 0, L_RS, 0, 0, 0, L_TM, 0, L_RS, 0, L_TM, L_RS};
        for (uint32_t i = 0; i < 32u; i++) step_hits(3, i, L[i], i % 16u == 0u ? 100 : 70);
        step_set_cond(&trk[3].step[19], SC_1_2);
        step_set_cond(&trk[3].step[28], SC_2_4);
        step_set_cond(&trk[3].step[30], SC_4_4);
        step_set_cond(&trk[3].step[31], SC_4_4);
        trk[3].p[P_SLEN] = 32;
    }
    tx(3, TX_MODE, TESS_ENG_OFF);
    trk[3].p[P_SDIV] = DIV_16;
    tx(3, TX_SV_EVERY, 3); tx(3, TX_SV_AMOUNT, 20); tx(3, TX_SV_LOCK, 60); tx(3, TX_SV_HOW, TESS_SV_ORNAMENT);
    tx(3, TX_SUB_HUMAN, 14);
    mixer(3, 96, -5, 45, 25);

    {
        static const uint8_t REP[4] = {2, 1, 1, 1}, ROLE[4] = {RO_ANCHOR, RO_BASS, RO_MELODY, RO_RHYTHM};
        chain_set(FC_ABAC, PH_8, 15, REP, 5, -2);
        entries_set(45, 2, 1, 2, ROLE);
        piece_set(LEN_12, SH_ARCH, 55, 3, END_LOOP);
    }
    cord(0, CS_APATHY, 0, CD_TW_DROP, 0, 30, 0, 0);   /* T1 TIDE 3 -> T1 DROP: the harp thins and fills */
    cord(1, CS_OCHD4, 2, CD_E1, 2, 20, 0, 0);         /* T3 SWARM 4 -> T3 CUT: the flute's breath */
    cord(2, CS_DRIFT, 1, CD_PAN, 1, 30, 0, 0);        /* T2 DRIFT -> T2 PAN */
}

/* STONE PIER: the ambient dub, after LOW HARBOUR but slower and emptier. Chords written in STEP on ORACLE's DUB
 * CHORD, one a bar on the second beat, held for a beat, through a FILTER a TIDE opens over minutes and into a dotted
 * echo whose time bends and whose feedback swells; Fm7 and Bb7 shells (no root: the bass has it); a sub bass written
 * in STEP that leaves most of each bar empty; an 808 one drop (kick and rim on three, a hat now and then); a soft lead
 * calling now and then into the echo. ENTRIES at 55, CUT: the mixer moves of a dub, parts dropped and back on the
 * phrase lines. The CHAIN takes it all to the fourth in B (KEY B +5), ABAB.. in 8-bar phrases (A twice); a 10-minute
 * piece in three WAVES, played again. F dorian, 66 BPM. */
static void demo_pier(void)
{
    song.g[G_BPM] = 66;
    sound(0, ENGI_ORACLE, OR_DUB_CHORD);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    sound(2, ENGI_DRUM, DK_808_KIT);
    sound(3, ENGI_TRIO, TR_SOFT_LEAD);
    scale(5, SCL_DOR, 0);
    sends(118, 102, 117, 106);                        /* a big room; ECHO 342 ms, a dotted sixteenth at 66 BPM */

    /* T1: four bars of eighths, a chord on each bar's second beat held to the "and" (ORACLE's three units a voice: two
     * chords never overlap in the budget); a Cm7 leading home on the fourth bar's last beat every fourth time */
    {
        static const uint8_t FM7[4] = {56, 60, 63, 0}, BB7[4] = {56, 62, 65, 0}, CM7[4] = {55, 58, 63, 0};
        for (uint32_t i = 0; i < 32u; i++) step_note(0, i, 0, 0);
        step_chord(0, 2, FM7, 96); step_tie(0, 3);
        step_chord(0, 10, FM7, 90); step_tie(0, 11);
        step_chord(0, 18, BB7, 96); step_tie(0, 19);
        step_chord(0, 26, FM7, 90); step_tie(0, 27);
        step_chord(0, 30, CM7, 60); step_set_cond(&trk[0].step[30], SC_2_4);
        trk[0].p[P_SLEN] = 32;
    }
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_8;
    tx(0, TX_SUB_HUMAN, 10);
    or_patch[0][OR_DECAY_AMP] = 80; or_patch[0][OR_RELEASE_AMP] = 50;   /* (AMP ENV: the echo is the tail) */
    trk[0].p[P_FXTYPE] = 10; trk[0].p[P_FXMIX] = 127; trk[0].p[P_FXRATE] = 60; trk[0].p[P_FXCHAR] = 45;   /* FILTER */
    mixer(0, 122, -10, 70, 118);

    /* T2: the sub, four bars of eighths: F held, a step up late in the second bar, Bb under the third, a pickup */
    {
        static const uint8_t LINE[32] = {41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 0, 44, 0,
                                         34, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 36, 0, 39};
        for (uint32_t i = 0; i < 32u; i++) step_note(1, i, LINE[i], 100);
        step_tie(1, 1); step_tie(1, 2); step_tie(1, 17); step_tie(1, 18); step_tie(1, 25);
        step_set_cond(&trk[1].step[31], SC_1_2);
    }
    trk[1].p[P_SLEN] = 32;
    tx(1, TX_MODE, TESS_ENG_OFF);
    trk[1].p[P_SDIV] = DIV_8;
    mixer(1, 99, 0, 5, 0);

    /* T3: 808 STEP, a bar of sixteenths: the one drop, the kick alone on three; a rim on four every other bar, a hat on
     * some offbeats by condition, an open hat every fourth bar, a rim pickup on FILL */
    {
        static const uint8_t L[16] = {0, 0, L_CH, 0, 0, 0, 0, 0, L_BD, 0, L_CH, 0, L_RS, 0, L_OH, L_RS};
        for (uint32_t i = 0; i < 16u; i++) step_hits(2, i, L[i], i == 8u ? 112 : 66);
        step_set_cond(&trk[2].step[2], SC_2_2);
        step_set_cond(&trk[2].step[10], SC_1_2);
        step_set_cond(&trk[2].step[12], SC_2_2);
        step_set_cond(&trk[2].step[14], SC_4_4);
        step_set_cond(&trk[2].step[15], SC_FILL);
        trk[2].p[P_SLEN] = 16;
    }
    tx(2, TX_MODE, TESS_ENG_OFF);
    trk[2].p[P_SDIV] = DIV_16;
    tx(2, TX_SUB_HUMAN, 10);
    mixer(2, 101, 5, 35, 60);

    /* T4: the lead, AWAKE in half notes, mostly silent; into the echo */
    trk[3].p[P_SDIV] = DIV_2;
    gen(3, TESS_ENG_AWAKE, 74);
    tx(3, TX_AWK_LENA, 6); tx(3, TX_AWK_LENB, 4); tx(3, TX_AWK_PROB, 30);
    tx(3, TX_TW_DROP, 50); tx(3, TX_TW_HUMAN, 30);
    trk[3].p[P_VOICE] = 2;                            /* LEGATO */
    sections(3, 60, 30, 25);
    mixer(3, 94, 30, 90, 120);

    {
        static const uint8_t REP[4] = {2, 1, 1, 1}, ROLE[4] = {RO_ANCHOR, RO_BASS, RO_RHYTHM, RO_MELODY};
        chain_set(FC_AB, PH_8, 0, REP, 5, 0);
        entries_set(55, 1, 0, 1, ROLE);
        piece_set(LEN_10, SH_WAVES, 50, 3, END_LOOP);
    }
    tx(0, TX_EN_ARC, 30);
    cord(0, CS_INERTIA, 0, CD_FX1_RATE, 0, 40, 0, 0); /* T1 TIDE 2 -> T1 FX1 RTE: the chords' filter opens and closes */
    cord(1, CS_DRIFT, 0, CD_SF2_RATE, 0, 8, 0, 0);    /* T1 DRIFT -> SF2 RTE: the echo's time bends as a tape's */
    cord(2, CS_TORPOR, 0, CD_SF2_CHAR, 0, 20, 0, 0);  /* T1 TIDE 1 -> SF2 CHR: the echo's feedback swells and settles */
    cord(3, CS_OCHD3, 3, CD_PAN, 3, 35, 0, 0);        /* T4 SWARM 3 -> T4 PAN */
}

typedef struct { const char *name, *file; void (*make)(void); } demo_t;
static const demo_t DEMO_LIST[] = {
    {"STILL TIDE", "still-tide", demo_still},
    {"KOTO GARDEN", "koto-garden", demo_garden},
    {"LOW HARBOUR", "low-harbour", demo_dub},
    {"SLOW LIGHT", "slow-light", demo_light},
    {"CEDAR RAIN", "cedar-rain", demo_cedar},
    {"STONE PIER", "stone-pier", demo_pier},
};

/* ------------------------------------------------------- the test projects --- */
/* demos/checks/: short projects for the device checks, each set so its feature shows within a minute or two (the
 * tracker's rows; demos/checks/README.md). All start from one plain four-track base, A minor at 110 BPM:
 * T1 a TRIO pad on CHORD, T2 an FM6 piano on TURING, T3 a TRIO sub bass on EUCLID, T4 the 909 on ATLAS */
static void chk_base(void)
{
    song.g[G_BPM] = 110;
    sound(0, ENGI_TRIO, TR_WARM_PAD);
    sound(1, ENGI_FM6, FX_TINE_EP);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    sound(3, ENGI_DRUM, DK_909_KIT);
    scale(9, SCL_MIN, 0);
    sends(90, 80, 70, 60);
    gen(0, TESS_ENG_CHORD, 3);
    tx(0, TX_CH_BARS, 2);
    mixer(0, 80, -20, 80, 0);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 17);
    tx(1, TX_TUR_PROB, 20);
    mixer(1, 90, 25, 60, 40);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 5);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3);
    tx(2, TX_TW_SHIFT, -12);
    mixer(2, 90, 0, 10, 0);
    gen(3, TESS_ENG_GRIDS, 9);
    mixer(3, 85, 0, 20, 0);
}
/* CHAIN: ABAC, a phrase of 4 bars (8.7 s), WANDER 0: every track changes section on the same phrase line */
static void chk_chain_set(void)
{
    tx(0, TX_FC_FORM, 2); tx(0, TX_FC_PHRASE, 0); tx(0, TX_FC_WANDER, 0);
}
static void chk_chain(void) { chk_base(); chk_chain_set(); }
/* ENTRIES 50 on that CHAIN: tracks fade out on section lines (FADE 1 bar) and come back one a phrase; T3 the bass */
static void chk_entries_set(void)
{
    chk_chain_set();
    tx(0, TX_ET_AMOUNT, 50); tx(0, TX_ET_DWELL, 1); tx(0, TX_ET_STYLE, 1); tx(0, TX_ET_FADE, 1);
    tx(2, TX_ET_ROLE, 1);                             /* (entries.c ET_BASS) */
}
static void chk_entries(void) { chk_base(); chk_entries_set(); }
/* PIECE: 2 minutes, ARCH, END STOP, on ENTRIES 75: layers in, full at the top, thins out, fades and stops */
static void chk_piece(void)
{
    chk_base(); chk_entries_set();
    tx(0, TX_ET_AMOUNT, 75);
    tx(0, TX_PL_LENGTH, 2); tx(0, TX_PL_SHAPE, 0); tx(0, TX_PL_PEAK, 50); tx(0, TX_PL_END, 0);   /* (planner.c PL_MIN, PL_ARCH) */
}
/* MODS: mod 1 (T1 SWARM 2 -> T1 CUT) at 0 %, its amount moved by mod 2 (T2 SWARM 1 -> M1 AMT): the filter
 * mod breathes in and out; mod 3 a GATE (T3 SWARM 3 over its middle -> T2 CUT +60); mod 4 SWARM 4 -> FILL */
static void chk_mods(void)
{
    chk_base();
    cord(0, CS_OCHD2, 0, CD_CUTOFF, 0, 0, 0, CU_BI);
    cord(1, CS_OCHD1, 1, CD_M1_AMT, 0, 80, 0, CU_BI);
    cord(2, CS_OCHD3, 2, CD_CUTOFF, 1, 60, 0, CU_GATE);
    cord(3, CS_OCHD4, 3, CD_FILL, 0, 100, 0, CU_UNI);
}
/* TEMPO: SWARM 1 (about a minute a turn) onto TEMPO at +30: everything speeds up and slows down together */
static void chk_tempo(void)
{
    chk_base();
    cord(0, CS_OCHD1, 0, CD_TEMPO, 0, 30, 0, CU_BI);
}
/* MIDI: T2 SOUND OFF on channel 1, two octaves down on the way out (MIDI OUT TRANSPOSE -24); T3 SOUND OFF with its
 * channel OFF too, a lane whose CV opens T1's filter. (MENU: MIDI OUT ALL for the notes, CLOCK OUT for the clock) */
static void chk_midi(void)
{
    chk_base();
    tx(1, TX_SOUND, 1); tx(1, TX_MIDI_CH, 1); tx(1, TX_MIDI_TR, -24);
    tx(2, TX_SOUND, 1); tx(2, TX_MIDI_CH, (int32_t)TESS_MIDI_OFF);
    cord(0, CS_CV, 2, CD_CUTOFF, 0, 50, 0, CU_BI);
}
/* CPU: four ORACLE tracks, dense, a FILTER insert on T2, the room and echo high: what a busy project costs (Settings >
 * Health in the editor while it plays) */
static void chk_cpu(void)
{
    chk_base();
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    sound(1, ENGI_ORACLE, OR_FLUTE);
    sound(2, ENGI_ORACLE, OR_DUB_CHORD);
    sound(3, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 1;                             /* DIA3 */
    trk[1].p[P_SDIV] = DIV_16;
    tx(2, TX_TW_SHIFT, 0);
    gen(3, TESS_ENG_AWAKE, 21);
    trk[3].p[P_SDIV] = DIV_8;
    trk[1].p[P_FXTYPE] = 10; trk[1].p[P_FXMIX] = 127; trk[1].p[P_FXRATE] = 70; trk[1].p[P_FXCHAR] = 40;   /* FILTER */
    sends(120, 110, 100, 100);
    for (uint32_t k = 0; k < NTRK; k++) mixer(k, 80, (int32_t)k * 20 - 30, 100, 80);
}
static const demo_t CHECK_LIST[] = {
    {"CHK CHAIN", "chk-chain", chk_chain},
    {"CHK ENTRIES", "chk-entries", chk_entries},
    {"CHK PIECE", "chk-piece", chk_piece},
    {"CHK MODS", "chk-mods", chk_mods},
    {"CHK TEMPO", "chk-tempo", chk_tempo},
    {"CHK MIDI", "chk-midi", chk_midi},
    {"CHK CPU", "chk-cpu", chk_cpu},
};
/* the set make / check work on: the demos, or ("checks ...") the test projects */
static const demo_t *DEMOS = DEMO_LIST;
static uint32_t NDEMO = sizeof DEMO_LIST / sizeof DEMO_LIST[0];

/* ------------------------------------------------------------- playing --- */
/* PLAY for secs s (wav: written there, 0 none): the notes each track played, the samples near full scale, the most
 * voices sounding; returns the peak */
typedef struct { uint32_t notes[NTRK], clips, voices, taken; int32_t peak; double rms; } dm_play_t;
/* the video mode (video OUTDIR): make's play that writes a demo's WAV also draws its screens on a timeline */
typedef struct { uint32_t ms; int home; uint8_t sel; const char *page, *caption; } vstep_t;
static const char *v_root;                              /* (0: no video) */
static int v_want = -1, v_on;                           /* (the demo filmed in this make; v_on: its final play now) */
static const uint32_t V_DEMO_SECS[] = {35, 30, 35};    /* (each demo's part of the video) */
static FILE *v_cap;
static double v_t0;
static void v_page(int home, uint32_t sel, const char *title);
static void v_ppm(const char *dir, uint32_t n);
static const vstep_t *v_steps_of(const char *wav, uint32_t *n, char *dir, uint32_t cap);
#define V_KEEP 255u                                     /* (vstep_t sel: the track the record selected, unchanged) */
#define V_BLOCKS 46u                                    /* control blocks a frame: 1472 samples, 29.96 frames a second */
static dm_play_t play(uint32_t secs, const char *wav)
{
    dm_play_t r;
    FILE *w = 0;
    uint32_t f, i, frames = secs * FS, kills0 = voice_kills;
    double sq = 0;
    memset(&r, 0, sizeof r);
    memset(dm_notes, 0, sizeof dm_notes);
    uint32_t step_age[NTRK];
    for (i = 0; i < NTRK; i++) {
        step_age[i] = 0;
        for (uint32_t k = 0; k < NVOICE; k++) if (trk[i].v[k].age > step_age[i]) step_age[i] = trk[i].v[k].age;
    }
    if (wav && !(w = fopen(wav, "wb"))) fprintf(stderr, "demos: cannot write %s\n", wav);
    if (w) wav_hdr(w, frames);
    char vdir[600];
    uint32_t vn = 0, vsi = 0, vf = 0, vb = 0;
    const vstep_t *vst = v_on && wav ? v_steps_of(wav, &vn, vdir, sizeof vdir) : 0;
    transport_req = 1;
    for (f = 0; f < frames; f += CTL, vb++) {
        int32_t o[2 * CTL];
        if (vst) {
            uint32_t ms = (uint32_t)((uint64_t)f * 1000u / FS);
            while (vsi < vn && ms >= vst[vsi].ms) {      /* (pages only: the sound is untouched) */
                v_page(vst[vsi].home, vst[vsi].sel, vst[vsi].page);
                fprintf(v_cap, "%s\t%.3f\t%.3f\t%s\n", strrchr(vdir, '/') + 1, v_t0 + vst[vsi].ms / 1000.0,
                        v_t0 + (vsi + 1u < vn ? vst[vsi + 1u].ms / 1000.0 : (double)secs), vst[vsi].caption);
                vsi++;
            }
        }
        dm_f = f;
        mix_block(o, CTL);
        for (i = 0; i < NTRK; i++)                       /* (a STEP track's notes: dm_note sees a generator's only, so
                                                         * the voices it started, by their age) */
            if (i >= TESSERA_TRACKS || tessera.mode[i] == TESS_ENG_OFF)
                for (uint32_t k = 0; k < NVOICE; k++)
                    if (trk[i].v[k].age > step_age[i]) step_age[i] = trk[i].v[k].age, dm_notes[i]++;
        fm1_ms = f / (FS / 1000u);
        if (vst && vb % V_BLOCKS == 0) { ui_draw(); v_ppm(vdir, vf++); }
        if (busy_now() > r.voices) r.voices = busy_now();
        for (i = 0; i < 2u * CTL; i++) {
            int32_t a = o[i] < 0 ? -o[i] : o[i];
            if (a >= 32700) r.clips++;
            if (a > r.peak) r.peak = a;
            sq += (double)o[i] * o[i];
        }
        if (w) for (i = 0; i < CTL; i++) wav_put(w, o[2 * i], o[2 * i + 1]);
    }
    if (w) fclose(w);
    if (vst) { printf("video: %s %u s, %u frames\n", vdir, secs, vf); v_t0 += secs; }
    transport_req = 2;                                /* STOP */
    for (f = 0; f < FS; f += CTL) { int32_t o[2 * CTL]; mix_block(o, CTL); }
    memcpy(r.notes, dm_notes, sizeof r.notes);
    r.taken = voice_kills - kills0;
    r.rms = sqrt(sq / (2.0 * frames)) / 32768.0;
    return r;
}

/* a demo with a planned PIECE starts sparse and layers in (Entries, by its SHAPE): its checks play this long, so every
 * track has come in */
#define DM_PIECE_SECS 600u
static int report(const char *name, const dm_play_t *p)
{
    char what[160];
    int bad = 0;
    snprintf(what, sizeof what, "%s plays: notes T1 %u T2 %u T3 %u T4 %u, peak %.2f, rms %.3f, voices %u", name,
             p->notes[0], p->notes[1], p->notes[2], p->notes[3], p->peak / 32768.0, p->rms, p->voices);
    bad += check(what, p->notes[0] && p->notes[1] && p->notes[2] && p->notes[3]);
    snprintf(what, sizeof what, "%s: no sample at full scale (%u), voices inside the budget of %u (%u taken)", name, p->clips,
             NVOICE, p->taken);
    bad += check(what, !p->clips && p->voices <= NVOICE);
    return bad;
}

/* raw (len) through the backup page's PUT into slot 1, loaded into a new project: 0 done */
static int restore_load(const uint8_t *raw, uint32_t len, char *name)
{
    power_on();
    if (put_all(2, raw, len)) return 1;
    project_new_blank();
    tessera_reset();
    project_load(0);
    if (proj_cur != 0u) return 2;
    project_cur_name(name);
    song.g[G_SLOT] = 1;
    return 0;
}

static uint8_t rec[TSP_MAX], again[TSP_MAX];

static int make(const char *dir, uint32_t secs)
{
    int bad = 0;
    for (uint32_t d = 0; d < NDEMO; d++) {
        char path[512], name[PROJ_NAME_LEN + 1u], what[160];
        uint32_t len, len2;
        int n;
        FILE *f;
        dm_play_t a, b;
        uint8_t tess[TESSERA_PK_BYTES], cords[CORD_PK_BYTES];
        power_on();
        DEMOS[d].make();
        song.g[G_SLOT] = 1;                           /* (SLOT 1 in every record; a load keeps the device's own) */
        tessera_pack(tess); cord_pack(cords);
        snprintf(what, sizeof what, "%s: saved to slot %u", DEMOS[d].name, d % 4u + 1u);
        bad += check(what, project_save_as(d % 4u, DEMOS[d].name) == 0);
        n = st_load(OBJ_SLOT(d % 4u), rec, sizeof rec);      /* (what the backup page's GET of object 2 + d reads) */
        len = n > 0 ? (uint32_t)n : 0u;
        snprintf(what, sizeof what, "%s: the slot is a whole TSP1 record (%u B of %u)", DEMOS[d].name, len, TSP_MAX);
        bad += check(what, len && tsp_valid(rec, len));
        snprintf(path, sizeof path, "%s/%s.tsp", dir, DEMOS[d].file);
        if (!(f = fopen(path, "wb")) || fwrite(rec, 1, len, f) != len) { bad += check(path, 0); if (f) fclose(f); continue; }
        fclose(f);
        /* through a restore and a load: the same record, generators and cords */
        bad += check("restored through the backup page and loaded", restore_load(rec, len, name) == 0);
        bad += check("its name", !strcmp(name, DEMOS[d].name));
        {
            uint8_t t2[TESSERA_PK_BYTES], c2[CORD_PK_BYTES];
            tessera_pack(t2); cord_pack(c2);
            bad += check("the generators and cords as made", !memcmp(t2, tess, sizeof tess) && !memcmp(c2, cords, sizeof cords));
        }
        project_capture(&proj_scratch);
        len2 = tsp_pack(again, &proj_scratch, 1);
        bad += check("saved again, the same record", len2 == len && !memcmp(again, rec, len));
        /* played as a player gets it, loaded (not as made: FALL's leaves, the balls, are not saved, so they start from
         * where a load leaves them); twice, the same notes */
        a = play(20, 0);
        restore_load(rec, len, name);
        b = play(20, 0);
        snprintf(what, sizeof what, "loaded again, the same notes (T1 %u T2 %u T3 %u T4 %u)", b.notes[0], b.notes[1], b.notes[2], b.notes[3]);
        bad += check(what, !memcmp(a.notes, b.notes, sizeof a.notes));
        restore_load(rec, len, name);
        snprintf(path, sizeof path, "%s/%s.wav", dir, DEMOS[d].file);
        v_on = v_root && v_want == (int)d;              /* (video: the demo filmed plays long, the others as the tests do) */
        a = play(v_root ? (v_on ? V_DEMO_SECS[d] : 20u) : secs, path);
        v_on = 0;
        if (tessera.pl_length && !v_root) {           /* (a planned piece layers in: its tracks come in over minutes) */
            restore_load(rec, len, name);
            a = play(DM_PIECE_SECS, 0);
        }
        bad += report(DEMOS[d].name, &a);
    }
    return bad;
}

static int check_dir(const char *dir)
{
    int bad = 0;
    for (uint32_t d = 0; d < NDEMO; d++) {
        char path[512], name[PROJ_NAME_LEN + 1u], what[600];
        FILE *f;
        uint32_t len;
        snprintf(path, sizeof path, "%s/%s.tsp", dir, DEMOS[d].file);
        if (!(f = fopen(path, "rb"))) { bad += check(path, 0); continue; }
        len = (uint32_t)fread(rec, 1, sizeof rec, f);
        fclose(f);
        snprintf(what, sizeof what, "%s: restores through the backup page and loads", path);
        bad += check(what, restore_load(rec, len, name) == 0 && !strcmp(name, DEMOS[d].name));
        bad += report(DEMOS[d].name, (dm_play_t[]){play(tessera.pl_length ? DM_PIECE_SECS : 20u, 0)});
    }
    /* the web editor's Demo projects (web/fm1backup.js putProject): each demo's PUT alone into a slot of its own (B,
     * C, D) on a device that stays on: the slot list names them at once, and each loads from it */
    power_on();
    for (uint32_t d = 0; d < NDEMO && d < 3u; d++) {
        char path[512];
        FILE *f;
        uint32_t len;
        snprintf(path, sizeof path, "%s/%s.tsp", dir, DEMOS[d].file);
        if (!(f = fopen(path, "rb"))) { bad += check(path, 0); continue; }
        len = (uint32_t)fread(rec, 1, sizeof rec, f);
        fclose(f);
        bad += check("the editor's PUT of a demo into one slot", put_all(3u + d, rec, len) == 0);
    }
    for (uint32_t d = 0; d < NDEMO && d < 3u; d++) {
        char name[PROJ_NAME_LEN + 1u], what[160];
        snprintf(what, sizeof what, "slot %c names %s with no power cycle (it shows %s)", 'B' + d, DEMOS[d].name,
                 proj_meta[1u + d].name);
        bad += check(what, project_used(1u + d) && !strcmp(proj_meta[1u + d].name, DEMOS[d].name));
        project_new_blank();
        project_load(1u + d);
        project_cur_name(name);
        snprintf(what, sizeof what, "slot %c: %s loads", 'B' + d, name);
        bad += check(what, proj_cur == 1u + d && !strcmp(name, DEMOS[d].name));
    }
    bad += check("slot A left empty", !project_used(0));
    return bad;
}


/* form N SECS: demo N's long form as it plays: a line each time the CHAIN's place or the tracks out change (time, the
 * letter, the tracks out by ENTRIES, where the piece is), with each track's notes since the line before */
static void form_log(uint32_t d, uint32_t secs)
{
    uint32_t f, i, last_pos = 99, last_out = 99, step_age[NTRK], n0[NTRK] = {0};
    char was = '?';
    power_on();
    DEMOS[d].make();
    memset(dm_notes, 0, sizeof dm_notes);
    for (i = 0; i < NTRK; i++) step_age[i] = 0;
    transport_req = 1;
    printf("%s\n  time  sect  out   piece  | notes T1 T2 T3 T4 since the line before\n", DEMOS[d].name);
    for (f = 0; f < secs * FS; f += CTL) {
        int32_t o[2 * CTL];
        uint8_t seq[4], role[4];
        uint32_t pos, left, warn, out, x = 0, sl = 0;
        dm_f = f;
        mix_block(o, CTL);
        fm1_ms = f / (FS / 1000u);
        for (i = 0; i < NTRK; i++)
            if (i >= TESSERA_TRACKS || tessera.mode[i] == TESS_ENG_OFF)
                for (uint32_t k = 0; k < NVOICE; k++)
                    if (trk[i].v[k].age > step_age[i]) step_age[i] = trk[i].v[k].age, dm_notes[i]++;
        if (f % (FS / 10u) >= CTL) continue;
        uint32_t n = tessera_chain_get(seq, &pos, &left);
        out = tessera_entries_get(&warn, role);
        int planned = tessera_piece_get(&x, &sl);
        if (pos != last_pos || out != last_out) {
            char now = n ? (char)('A' + seq[pos % 4u]) : '-';
            printf("%6.1f  %c%c    %c%c%c%c  %5.1f%%  |       %3u %3u %3u %3u\n", (double)f / FS, now, now != was ? '*' : ' ',
                   out & 1 ? '1' : '.', out & 2 ? '2' : '.', out & 4 ? '3' : '.', out & 8 ? '4' : '.',
                   planned ? x * 100.0 / 65536.0 : -1.0, dm_notes[0] - n0[0], dm_notes[1] - n0[1], dm_notes[2] - n0[2],
                   dm_notes[3] - n0[3]);
            memcpy(n0, dm_notes, sizeof n0);
            last_pos = pos; last_out = out; was = now;
        }
    }
    printf("%6.1f  end: notes T1 %u T2 %u T3 %u T4 %u, %s\n", (double)secs, dm_notes[0], dm_notes[1], dm_notes[2], dm_notes[3],
           song.playing ? "still playing" : "stopped");
}

/* ------------------------------------------------- the backup page's file --- */
/* A whole archive as the backup page writes it (web/fm1backup.js captureBackup: LIST, then GET of every object),
 * from this simulated device; and one read back and restored as restoreBackup does (each object's PUT begun and
 * aborted first, then the objects from 2 on, the settings, the music playing last). For tools/demo_backup.py's test */
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t obj_buf[40000];
static int archive_write(const char *path)
{
    FILE *f = fopen(path, "w");
    uint8_t list[3 + 30 * 11];
    uint32_t n, i, off;
    if (!f) return 1;
    call(ED_BACKUP_LIST, 0, 0);
    if (rep[1] || rep[2] != ED_BK_N) { fclose(f); return 2; }
    memcpy(list, rep, sizeof list);
    n = list[2];
    fprintf(f, "{\"format\":\"felucca-backup\",\"version\":1,\"firmware\":\"host\",\"created\":\"2026-10-22T00:00:00.000Z\",\"objects\":[");
    for (i = 0; i < n; i++) {
        const uint8_t *o = list + 3 + i * 11u;
        uint32_t id = o[0], len = ed_bk_r32(o + 1), crc = ed_bk_r32(o + 6), k;
        for (off = 0; off < len; off += 256u) {
            uint32_t c = len - off > 256u ? 256u : len - off;
            uint8_t a[8] = {(uint8_t)id};
            put32(a + 1, off); a[6] = (uint8_t)(c & 127u); a[7] = (uint8_t)(c >> 7);
            call(ED_BACKUP_GET, a, sizeof a);
            if (rep[1] || ed_unpack7(rep + 9u, rep_n - 9u, obj_buf + off, c) != c) { fclose(f); return 3; }
        }
        fprintf(f, "%s{\"id\":%u,\"size\":%u,\"crc\":%u,\"data\":\"", i ? "," : "", id, len, crc);
        for (k = 0; k < len; k += 3u) {
            uint32_t v = (uint32_t)obj_buf[k] << 16 | (k + 1u < len ? (uint32_t)obj_buf[k + 1u] << 8 : 0u) | (k + 2u < len ? obj_buf[k + 2u] : 0u);
            fputc(B64[v >> 18 & 63u], f); fputc(B64[v >> 12 & 63u], f);
            fputc(k + 1u < len ? B64[v >> 6 & 63u] : '=', f); fputc(k + 2u < len ? B64[v & 63u] : '=', f);
        }
        fprintf(f, "\"}");
    }
    fprintf(f, "]}\n");
    fclose(f);
    return 0;
}
static uint32_t b64_val(int c)
{
    const char *p = c ? strchr(B64, c) : 0;
    return p ? (uint32_t)(p - B64) : 0u;
}
static char *json;
/* object id's bytes from the archive in json -> obj_buf: its length, -1 not there */
static int32_t archive_obj(uint32_t id)
{
    char key[24];
    const char *p, *d;
    uint32_t n = 0;
    snprintf(key, sizeof key, "{\"id\":%u,", id);
    if (!(p = strstr(json, key)) || !(d = strstr(p, "\"data\":\""))) return -1;
    for (d += 8; d[0] && d[0] != '"'; d += 4) {
        uint32_t v = b64_val(d[0]) << 18 | b64_val(d[1]) << 12 | b64_val(d[2]) << 6 | b64_val(d[3]);
        obj_buf[n++] = (uint8_t)(v >> 16);
        if (d[2] != '=') obj_buf[n++] = (uint8_t)(v >> 8);
        if (d[3] != '=') obj_buf[n++] = (uint8_t)v;
    }
    return (int32_t)n;
}
static int archive_restore(const char *path)
{
    static const uint8_t ORDER[30] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                      24, 25, 26, 27, 28, 29, 1, 0};
    FILE *f = fopen(path, "rb");
    long sz;
    int bad = 0;
    if (!f) return check(path, 0);
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    json = malloc((size_t)sz + 1u);
    if (fread(json, 1, (size_t)sz, f) != (size_t)sz) sz = 0;
    json[sz] = 0;
    fclose(f);
    power_on();
    for (uint32_t i = 0; i < 30u; i++) {                /* (sizes accepted before the first write) */
        int32_t n = archive_obj(i);
        uint8_t a[12] = {0, (uint8_t)i};
        if (n <= 0) continue;
        put32(a + 2, (uint32_t)n); put32(a + 7, st_crc32(obj_buf, (uint32_t)n));
        call(ED_BACKUP_PUT, a, sizeof a);
        if (rep[2]) { char w[80]; snprintf(w, sizeof w, "object %u accepted before any write (rc %u)", i, rep[2]); bad += check(w, 0); }
        a[0] = 3; call(ED_BACKUP_PUT, a, 2);
    }
    for (uint32_t i = 0; i < 30u; i++) {
        int32_t n = archive_obj(ORDER[i]);
        char w[80];
        if (n < 0) { snprintf(w, sizeof w, "object %u in the archive", ORDER[i]); bad += check(w, 0); continue; }
        snprintf(w, sizeof w, "object %u restored (%d B)", ORDER[i], n);
        bad += check(w, put_all(ORDER[i], obj_buf, (uint32_t)n) == 0);
    }
    memset(proj_meta, 0, sizeof proj_meta);              /* switched off and on: the slot list read from flash */
    for (uint32_t slot = 0; slot < 4u; slot++) proj_fetch(slot);
    for (uint32_t slot = 0; slot < 4u; slot++) {         /* what each slot holds now, loaded and played */
        char name[PROJ_NAME_LEN + 1u], w[120];
        if (!project_used(slot)) { printf("demos: slot %c empty\n", 'A' + slot); continue; }
        project_new_blank();
        project_load(slot);
        project_cur_name(name);
        snprintf(w, sizeof w, "slot %c: %s loads", 'A' + slot, name);
        bad += check(w, proj_cur == slot);
        bad += report(name, (dm_play_t[]){play(20, 0)});
    }
    free(json);
    return bad;
}

/* ------------------------------------------------------------- the video --- */
/* video OUTDIR: the demo video's material. The demos as make makes them (make OUTDIR 52: the same steps, so the
 * same audio as make's WAVs, only longer), each drawing its screens on a timeline; then a short tour of the modes
 * the demos leave out. Audio and screen from one pass, a frame every V_BLOCKS blocks. OUTDIR/<clip>.wav,
 * OUTDIR/<clip>/f00000.ppm .., OUTDIR/captions.txt (tools/demo_video.py: the video) */
typedef struct { uint32_t ms, key, down; } vkey_t;   /* a key (0: F3 .. 7: C4 ..) down or up at ms */
typedef struct { uint32_t ms, btn, down; } vbtn_t;   /* a button (panel.c B_) down or up at ms */
typedef struct { uint32_t ms, role; int32_t steps; uint32_t dur; } vturn_t;   /* a knob (EN_) turned steps detents
                                                     * over dur ms, through ui_input as a hand turns it */
static const vstep_t V_STILL[] = {
    {0, 1, V_KEEP, 0, "STILL TIDE: an ambient demo, four tracks that keep changing on their own"},
    {5000, 0, 0, "GEN", "Track 1, CHORD on ORACLE: a drone in YO on D, each voice breathing on its own"},
    {11000, 0, 1, "EVOLVE", "Track 2, AWAKE: a warm pad that wanders, and GRAVITY pulls it home"},
    {17000, 0, 2, "FX 1", "Track 3, FALL: leaves land on an electric piano, through the FILTER effect"},
    {23000, 0, 3, "GEN", "Track 4: a bass line written in STEP, varied by TURING (STEP as seed)"},
    {29000, 0, 0, "MATRIX", "MATRIX: TIDE, SWARM and DRIFT move the filters, the pan and the echo"},
};
static const vstep_t V_GARDEN[] = {
    {0, 1, V_KEEP, 0, "KOTO GARDEN: FM koto in HIRA on C"},
    {5000, 0, 0, "GEN", "Tracks 1 and 2, QUAD: two koto on one grid, eighths against triplets"},
    {11000, 0, 2, "FORM", "Track 3, an ORACLE flute on SERIES, in an ABAC FORM whose returns develop"},
    {18000, 0, 3, "NOTES", "Track 4, STEP: a mokugyo; conditions and Accumulate change it bar by bar"},
    {24000, 0, 0, "MODS", "MODS: a TIDE thins the koto and fills it again; SWARM breathes in the flute"},
};
static const vstep_t V_HARBOUR[] = {
    {0, 1, V_KEEP, 0, "LOW HARBOUR: an ambient dub in A dorian, 70 BPM"},
    {5000, 0, 0, "GEN", "Track 1, CHORD on ORACLE: offbeat stabs, most left out, into a long echo"},
    {11000, 0, 0, "FX 1", "The stabs' FILTER effect, its cutoff opened and closed by a TIDE"},
    {17000, 0, 2, "NOTES", "Track 3, a 909 in STEP: a one drop, the hats on conditions"},
    {23000, 0, 1, "EVOLVE", "Track 2, a MARKOV sub bass that evolves; GRAVITY keeps it near home"},
    {29000, 0, 0, "MODS", "MODS: DRIFT bends the echo's time like a tape's"},
};
static const vstep_t *v_steps_of(const char *wav, uint32_t *n, char *dir, uint32_t cap)
{
    const char *base = strrchr(wav, '/');
    base = base ? base + 1 : wav;
    snprintf(dir, cap, "%.*s", (int)(strlen(wav) - 4u), wav);   /* (OUTDIR/<file>) */
    mkdir(dir, 0755);
    if (!strncmp(base, "still-tide", 10)) { *n = sizeof V_STILL / sizeof V_STILL[0]; return V_STILL; }
    if (!strncmp(base, "koto-garden", 11)) { *n = sizeof V_GARDEN / sizeof V_GARDEN[0]; return V_GARDEN; }
    if (!strncmp(base, "low-harbour", 11)) { *n = sizeof V_HARBOUR / sizeof V_HARBOUR[0]; return V_HARBOUR; }
    *n = 0;
    return 0;
}
static void v_page(int home, uint32_t sel, const char *title)
{
    if (sel != V_KEEP) song.sel = (uint8_t)sel;
    if (home) go_home();
    else { ui.home = 0; ui.page = (uint8_t)page_titled(title); page_entered(); }
    ui.force = 1;
}
static void v_ppm(const char *dir, uint32_t n)
{
    char path[700];
    snprintf(path, sizeof path, "%s/f%05u.ppm", dir, n);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n240 240\n255\n");
    for (uint32_t i = 0; i < 240u * 240u; i++) {
        uint16_t c = (uint16_t)((dm_screen[i] >> 8) | (dm_screen[i] << 8));
        uint8_t b[3] = {(uint8_t)((c >> 11) * 255u / 31u), (uint8_t)(((c >> 5) & 63u) * 255u / 63u), (uint8_t)((c & 31u) * 255u / 31u)};
        fwrite(b, 1, 3, f);
    }
    fclose(f);
}
/* a clip of the video: the project set up now, played secs s (after pre s unheard) with its pages, keys, buttons and
 * knob turns, frames and audio */
typedef struct {
    const char *name;
    uint32_t secs, pre;
    const vstep_t *st; uint32_t nst;
    const vkey_t *keys; uint32_t nkeys;
    const vbtn_t *btns; uint32_t nbtns;
    const vturn_t *turns; uint32_t nturns;
} vclip_t;
#define VN(a) a, (uint32_t)(sizeof a / sizeof a[0])
static void v_run(const char *out, const vclip_t *c)
{
    char dir[600], wav[600];
    uint32_t frames = c->secs * FS, f, si = 0, ki = 0, bi = 0, b = 0, n = 0, done[16] = {0};
    memset(dm_notes, 0, sizeof dm_notes);
    snprintf(dir, sizeof dir, "%s/%s", out, c->name);
    mkdir(dir, 0755);
    snprintf(wav, sizeof wav, "%s/%s.wav", out, c->name);
    FILE *w = fopen(wav, "wb");
    if (!w) return;
    wav_hdr(w, frames);
    transport_req = 1;
    for (f = 0; f < c->pre * FS; f += CTL) { int32_t o[2 * CTL]; mix_block(o, CTL); }
    for (f = 0; f < frames; f += CTL, b++) {
        int32_t o[2 * CTL];
        uint32_t ms = (uint32_t)((uint64_t)f * 1000u / FS), input = b % V_BLOCKS == 0;
        fm1_ms = (c->pre * FS + f) / (FS / 1000u);
        host_ticks = fm1_ms * 1000u;
        while (si < c->nst && ms >= c->st[si].ms) {
            v_page(c->st[si].home, c->st[si].sel, c->st[si].page);
            fprintf(v_cap, "%s\t%.3f\t%.3f\t%s\n", c->name, v_t0 + c->st[si].ms / 1000.0,
                    v_t0 + (si + 1u < c->nst ? c->st[si + 1u].ms / 1000.0 : (double)c->secs), c->st[si].caption);
            si++;
        }
        while (ki < c->nkeys && ms >= c->keys[ki].ms) {
            if (c->keys[ki].down) fm1_in.notes |= 1u << c->keys[ki].key, v_note_edges |= 1u << c->keys[ki].key;
            else fm1_in.notes &= ~(1u << c->keys[ki].key);
            keyboard_block();
            ki++, input = 1;
        }
        while (bi < c->nbtns && ms >= c->btns[bi].ms) {
            uint32_t m = 1u << panel.btn[c->btns[bi].btn];
            if (c->btns[bi].down) fm1_in.buttons |= m, v_pressed |= m;
            else fm1_in.buttons &= ~m;
            bi++, input = 1;
        }
        for (uint32_t t = 0; t < c->nturns && t < 16u; t++) {   /* (the detents due by now, evenly over dur) */
            const vturn_t *u = &c->turns[t];
            uint32_t all = (uint32_t)(u->steps < 0 ? -u->steps : u->steps), due;
            if (ms < u->ms || done[t] >= all) continue;
            due = u->dur ? (ms - u->ms) * all / u->dur + 1u : all;
            if (due > all) due = all;
            if (due > done[t]) {
                v_enc[panel.enc[u->role] % 7u] += (int32_t)(due - done[t]) * (u->steps < 0 ? -1 : 1) * panel.dir[u->role];
                done[t] = due, input = 1;
            }
        }
        if (input) ui_input();
        dm_f = f;
        mix_block(o, CTL);
        for (uint32_t i = 0; i < CTL; i++) wav_put(w, o[2 * i], o[2 * i + 1]);
        if (b % V_BLOCKS == 0) { ui_draw(); v_ppm(dir, n++); }
    }
    fclose(w);
    if (getenv("DM_DEBUG")) {                           /* (the knobs as the clip left them) */
        printf("knobs %s:", c->name);
#if MELODEE_LOOK                                        /* (the suite builds the detailed look: no mo_col) */
        for (uint32_t k = 0; k < 4u; k++) printf(" [%s=%s]", mo_col[k].label, mo_col[k].val);
#endif
        printf(" GRAVITY %d DEVELOP %d SOURCE %d", tessera_param_get(0, TX_SD_GRAVITY), tessera_param_get(0, TX_FM_DEVELOP),
               tessera_param_get(0, TX_SD_SOURCE));
        printf("\n");
    }
    fm1_in.buttons = fm1_in.notes = 0;
    transport_req = 2;
    for (f = 0; f < FS; f += CTL) { int32_t o[2 * CTL]; mix_block(o, CTL); }
    printf("video: %s %u s, %u frames; notes %u %u %u %u\n", dir, c->secs, n, dm_notes[0], dm_notes[1], dm_notes[2],
           dm_notes[3]);
    v_t0 += c->secs;
}
static void v_scene(uint32_t bpm, uint32_t engine, uint32_t preset, uint32_t mode, uint32_t div)
{
    power_on();
    song.g[G_BPM] = (int16_t)bpm;
    scale(9, 6, 0);                                     /* A minor pentatonic */
    for (uint32_t k = 1; k < NTRK; k++) trk[k].p[P_LEVEL] = 0;
    sound(0, engine, preset);
    trk[0].p[P_SDIV] = (int16_t)div;
    gen(0, mode, 23);
    sends(90, 70, 70, 50);
    mixer(0, 104, 0, 70, 0);
}
/* track 2 under it: a soft ORACLE pad holding the scale's chord, so a pluck rings over a chord */
static void v_pad(void)
{
    sound(1, ENGI_ORACLE, OR_WARM_PAD);
    trk[1].p[P_CHRD] = 1;                               /* DIA3 */
    gen(1, TESS_ENG_CHORD, 7);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 80, 0, 90, 0);
}
enum { V_TINE = 0, V_MARIMBA = 5, V_PLUCK = 7, V_DX_MARIMBA = 12, V_GLASS_PAD = 16, V_HARP = 18, V_FLUTE = 20,
       V_KOTO_RING = 37 };                              /* FM6 presets (eng_fm6.c FM6_PRESETS) */
enum { V_OR_KEYS = 11, V_OR_FAT_POLY = 22 };            /* ORACLE's */
#define V_CLIP(nm, sec, ...) do { static const vclip_t C_ = {nm, sec, 0, __VA_ARGS__}; v_run(out, &C_); } while (0)
static const vkey_t V_NOKEYS[1];
static const vbtn_t V_NOBTNS[1];
static const vturn_t V_NOTURNS[1];
#define V_NO(a) a, 0u
/* the generators, one clip each, their knobs turned as a hand would */
static void v_generators(const char *out)
{
    {
        v_scene(100, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_TURING, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "TURING: a looping shift register, locked"},
                                    {2500, 0, 0, "GEN", "PROB up from LOCK: the loop starts to change, a step at a time"},
                                    {6000, 0, 0, "GEN", "LEN 16 to 8: a shorter loop"}};
        static const vturn_t T[] = {{2600, EN_K3, 5, 1500}, {6100, EN_K2, -8, 1600}};
        V_CLIP("g01-turing", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(100, ENGI_FM6, V_PLUCK, TESS_ENG_EUCLID, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "EUCLID: hits spread as evenly as they go"},
                                    {1500, 0, 0, "GEN", "PULSE 4 to 7: more hits, still evenly spread"},
                                    {4500, 0, 0, "GEN", "ROT: the same rhythm, turned"},
                                    {7000, 0, 0, "GEN", "STEPS 16 to 12: a shorter cycle against the bar"}};
        static const vturn_t T[] = {{1600, EN_K3, 3, 1200}, {4600, EN_K4, 3, 1200}, {7100, EN_K2, -4, 1200}};
        V_CLIP("g02-euclid", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, FX_KUGO_HARP, TESS_ENG_MARKOV, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "MARKOV: a random walk over the scale"},
                                    {2000, 0, 0, "GEN", "LEAP up: wider jumps"},
                                    {5500, 0, 0, "GEN", "DENS down: more rests between the notes"}};
        static const vturn_t T[] = {{2100, EN_K2, 12, 2000}, {5600, EN_K3, -12, 2000}};
        V_CLIP("g03-markov", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(100, ENGI_FM6, V_TINE, TESS_ENG_MORPH, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "MORPH: a rhythm blended between two patterns"},
                                    {1800, 0, 0, "GEN", "POS: from pattern A over to B..."},
                                    {5200, 0, 0, "GEN", "...and back"}};
        static const vturn_t T[] = {{1900, EN_K2, 16, 3000}, {5300, EN_K2, -24, 3000}};
        V_CLIP("g04-morph", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(100, ENGI_DRUM, DK_909_KIT, TESS_ENG_GRIDS, DIV_16);
        static const vstep_t S[] = {{0, 0, 0, "GEN", "ATLAS: kick, snare and hat from a map of drum patterns"},
                                    {1500, 0, 0, "GEN", "X and Y: a walk across the map"},
                                    {6500, 0, 0, "GEN", "DENS up: busier"}};
        static const vturn_t T[] = {{1600, EN_K2, 14, 2200}, {3900, EN_K3, -12, 2200}, {6600, EN_K4, 10, 1600}};
        V_CLIP("g05-atlas", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(92, ENGI_FM6, V_KOTO_RING, TESS_ENG_BOUNCE, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "BOUNCE: a ball bouncing in a box, a note where it hits"},
                                    {1500, 0, 0, "GEN", "SPEED and ANGLE: a new path"},
                                    {5500, 0, 0, "GEN", "BALLS past 4: FALL, leaves drifting down in the wind"}};
        static const vturn_t T[] = {{1600, EN_K2, 8, 1500}, {3300, EN_K3, 10, 1500}, {5600, EN_K4, 4, 1600}};
        V_CLIP("g06-bounce", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, V_HARP, TESS_ENG_AWAKE, DIV_8); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "AWAKE: two loops of different lengths, one melody"},
                                    {2000, 0, 0, "GEN", "LEN A shorter: the loops shift against each other"},
                                    {5500, 0, 0, "GEN", "PROB down: some steps rest"}};
        static const vturn_t T[] = {{2100, EN_K2, -3, 1200}, {5600, EN_K4, -8, 1600}};
        V_CLIP("g07-awake", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(100, ENGI_FM6, V_DX_MARIMBA, TESS_ENG_POLY, DIV_16); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "POLY: rhythms in different divisions against each other"},
                                    {2000, 0, 0, "GEN", "RHY: another set of rhythms"},
                                    {5500, 0, 0, "GEN", "SPAN: the notes spread wider"}};
        static const vturn_t T[] = {{2100, EN_K2, 2, 1000}, {5600, EN_K4, 1, 200}};
        V_CLIP("g08-poly", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(92, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_QUAD, DIV_8);
        sound(1, ENGI_FM6, V_KOTO_RING); trk[1].p[P_SDIV] = DIV_8T; gen(1, TESS_ENG_QUAD, 23); mixer(1, 100, 30, 70, 0);
        static const vstep_t S[] = {{0, 0, 0, "GEN", "QUAD: a 4 x 4 grid of notes the tracks share, a row each"},
                                    {2000, 0, 0, "GEN", "DIR: the way through the row"},
                                    {5500, 0, 0, "GEN", "PROB down: fewer of its notes"}};
        static const vturn_t T[] = {{2100, EN_K2, 2, 1000}, {5600, EN_K3, -10, 1600}};
        V_CLIP("g09-quad", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(90, ENGI_FM6, FX_KUGO_HARP, TESS_ENG_CONSTEL, DIV_8); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "CONSTELLATION: stars play as they cross the ship's crosshair"},
                                    {1500, 0, 0, "GEN", "Y: the ship moves up..."},
                                    {4200, 0, 0, "GEN", "...and down, and other stars cross it"},
                                    {7500, 0, 0, "GEN", "DENS: more stars"}};
        static const vturn_t T[] = {{1600, EN_K2, 5, 2000}, {4300, EN_K2, -9, 2800}, {7600, EN_K3, 10, 1500}};
        V_CLIP("g10-constellation", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, V_FLUTE, TESS_ENG_SERIES, DIV_8); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "GEN", "SERIES: the infinity series, a melody that never quite repeats"},
                                    {2000, 0, 0, "GEN", "START: another place in the series"},
                                    {5500, 0, 0, "GEN", "INTV: wider steps between its notes"}};
        static const vturn_t T[] = {{2100, EN_K2, 12, 1600}, {5600, EN_K3, 1, 300}};
        V_CLIP("g11-series", 9, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_ORACLE, V_OR_KEYS, TESS_ENG_CHORD, DIV_16);
        trk[0].p[P_CHRD] = 1;
        static const vstep_t S[] = {{0, 0, 0, "GEN", "CHORD: a progression in the track's scale, here on ORACLE"},
                                    {2000, 0, 0, "GEN", "HIT: held, then on the beat..."},
                                    {4500, 0, 0, "GEN", "...then offbeat stabs"},
                                    {7000, 0, 0, "GEN", "PROG: another progression"}};
        static const vturn_t T[] = {{2100, EN_K4, 1, 100}, {4600, EN_K4, 2, 600}, {7100, EN_K2, 3, 900}};
        V_CLIP("g12-chord", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, V_TINE, TESS_ENG_ANSWER, DIV_8);   /* ANSWER: C D E G on the keys, then its answers */
        static const vkey_t K[] = {{400, 7, 1}, {700, 7, 0}, {1025, 9, 1}, {1325, 9, 0}, {1650, 11, 1}, {1950, 11, 0},
                                   {2275, 14, 1}, {2700, 14, 0}};
        static const vstep_t S[] = {{0, 0, 0, "GEN", "ANSWER: play a phrase on the keys..."},
                                    {4500, 0, 0, "GEN", "...and it answers, a little different each time"},
                                    {9000, 0, 0, "GEN", "VARY up: the answers stray further"}};
        static const vturn_t T[] = {{9100, EN_K2, 10, 1500}};
        V_CLIP("g13-answer", 14, VN(S), VN(K), V_NO(V_NOBTNS), VN(T));
    }
}
/* what is new in 3.0, and two of the shaping pages, on chords and ringing plucks */
static void v_features(const char *out)
{
    {
        v_scene(92, ENGI_ORACLE, V_OR_FAT_POLY, TESS_ENG_CHORD, DIV_16);
        trk[0].p[P_CHRD] = 1;
        tx(0, TX_CH_PROG, GEN_CH_CIRCLE); tx(0, TX_CH_HIT, GEN_CH_BEAT);
        static const vstep_t S[] = {{0, 0, 0, "FILTER", "ORACLE: two oscillators, POLY MOD and a four-pole filter"},
                                    {2500, 0, 0, "FILTER", "CUTOFF down: darker..."},
                                    {5300, 0, 0, "FILTER", "...and open again"},
                                    {8000, 0, 0, "FILTER", "RES up: the filter starts to sing"}};
        static const vturn_t T[] = {{2600, EN_K1, -24, 2200}, {5400, EN_K1, 40, 2200}, {8100, EN_K2, 24, 1800}};
        V_CLIP("n01-oracle", 11, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(84, ENGI_FM6, V_GLASS_PAD, TESS_ENG_CHORD, DIV_16);
        trk[0].p[P_CHRD] = 2;
        tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_HIT, GEN_CH_HOLD);
        trk[0].p[P_FXTYPE] = 10; trk[0].p[P_FXMIX] = 127; trk[0].p[P_FXRATE] = 90; trk[0].p[P_FXCHAR] = 40;
        static const vstep_t S[] = {{0, 0, 0, "FX 1", "The FILTER effect: a resonant low-pass for any track, here FM6 chords"},
                                    {2500, 0, 0, "FX 1", "CUT down..."},
                                    {5500, 0, 0, "FX 1", "...and up, with more RES"}};
        static const vturn_t T[] = {{2600, EN_K3, -30, 2600}, {5600, EN_K4, 16, 1200}, {6000, EN_K3, 40, 3000}};
        V_CLIP("n02-filter", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(100, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_TURING, DIV_16); v_pad();
        tx(0, TX_EV_EVERY, 1); tx(0, TX_EV_AMOUNT, 60); tx(0, TX_EV_LOCK, 40);
        static const vstep_t S[] = {{0, 0, 0, "EVOLVE", "EVOLVE every bar: the line keeps wandering away"},
                                    {4500, 0, 0, "EVOLVE", "GRAVITY 50 (OCT+ held): half the rounds pull it back toward home"}};
        static const vbtn_t B[] = {{4300, B_OCTUP, 1}, {7500, B_OCTUP, 0}};
        static const vturn_t T[] = {{4600, EN_K1, 50, 2500}};
        V_CLIP("n03-gravity", 12, VN(S), V_NO(V_NOKEYS), VN(B), VN(T));
    }
    {
        v_scene(100, ENGI_FM6, V_HARP, TESS_ENG_MARKOV, DIV_16); v_pad();
        tx(0, TX_EV_EVERY, 1); tx(0, TX_FM_FORM, 2); tx(0, TX_FM_CONTRAST, 60);
        static const vstep_t S[] = {{0, 0, 0, "FORM", "FORM ABAC: A comes back exactly each time"},
                                    {4500, 0, 0, "FORM", "DEVELOP 40 (OCT+ held): each return of A is varied a little"}};
        static const vbtn_t B[] = {{4300, B_OCTUP, 1}, {7500, B_OCTUP, 0}};
        static const vturn_t T[] = {{4600, EN_K1, 20, 2400}};
        V_CLIP("n04-develop", 12, VN(S), V_NO(V_NOKEYS), VN(B), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_TURING, DIV_8); v_pad();
        {   /* track 3: a short melody written in STEP, quiet */
            static const uint8_t M[8] = {69, 0, 72, 74, 0, 76, 74, 0};
            for (uint32_t i = 0; i < 8u; i++) step_note(2, i, M[i], 90);
            trk[2].p[P_SLEN] = 8; trk[2].p[P_SDIV] = DIV_8;
            sound(2, ENGI_FM6, V_KOTO_RING); tx(2, TX_MODE, TESS_ENG_OFF); mixer(2, 60, 20, 60, 0);
        }
        static const vstep_t S[] = {{0, 0, 0, "GEN", "STEP as seed: TURING on its own register..."},
                                    {3500, 0, 0, "GEN", "...SOURCE T3 (OCT+ held): it plays from the melody written on track 3"},
                                    {7500, 0, 0, "GEN", "KEEP RHYTHM: the rhythm stays as written, the pitches drift"}};
        static const vbtn_t B[] = {{3300, B_OCTUP, 1}, {9000, B_OCTUP, 0}};
        static const vturn_t T[] = {{3600, EN_K1, 3, 900}, {7600, EN_K2, 1, 100}};
        V_CLIP("n05-seed", 12, VN(S), V_NO(V_NOKEYS), VN(B), VN(T));
    }
    {
        power_on();
        song.g[G_BPM] = 96;
        scale(9, 6, 0);
        sound(0, ENGI_DRUM, DK_909_KIT);
        {   /* a groove with hits on conditions and a FILL run */
            static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, L_BD, 0, L_CH, 0, L_SD, L_SD, L_SD, L_SD};
            for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], 96);
            step_set_cond(&trk[0].step[7], SC_1_2);
            step_set_cond(&trk[0].step[13], SC_FILL); step_set_cond(&trk[0].step[14], SC_FILL);
            step_set_cond(&trk[0].step[15], SC_FILL);
            trk[0].p[P_SLEN] = 16;
        }
        tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
        sends(80, 60, 70, 50); mixer(0, 104, 0, 30, 0);
        for (uint32_t k = 1; k < NTRK; k++) trk[k].p[P_LEVEL] = 0;
        static const vstep_t S[] = {{0, 0, 0, "NOTES", "Conditions: a kick on every other bar (1:2), a snare run on FILL"},
                                    {5000, 0, 0, "NOTES", "GLO + E4 held: FILL, the run comes in"}};
        static const vbtn_t B[] = {{4900, B_GLO, 1}, {9000, B_GLO, 0}};
        static const vkey_t K[] = {{5000, 11, 1}, {8900, 11, 0}};
        V_CLIP("n06-fill", 10, VN(S), VN(K), VN(B), V_NO(V_NOTURNS));
    }
    {
        v_scene(100, ENGI_FM6, FX_KALIMBA, TESS_ENG_TURING, DIV_8); v_pad();
        static const uint8_t M[5] = {69, 72, 0, 76, 74};
        for (uint32_t i = 0; i < 5u; i++) step_note(0, i, M[i], 96);
        step_set_climb(&trk[0].step[0], 2);
        step_set_climb(&trk[0].step[4], -1);
        trk[0].p[P_SLEN] = 5;
        tx(0, TX_MODE, TESS_ENG_OFF); tx(0, TX_ACC_LIMIT, 7); tx(0, TX_ACC_MODE, TESS_ACC_PING);
        static const vstep_t S[] = {{0, 0, 0, "NOTES", "Accumulate: a note climbs two degrees each pass, up an octave and back"}};
        V_CLIP("n07-accumulate", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), V_NO(V_NOTURNS));
    }
    {
        v_scene(100, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_MARKOV, DIV_16); v_pad();
        sound(2, ENGI_DRUM, DK_909_KIT); trk[2].p[P_SDIV] = DIV_16; gen(2, TESS_ENG_GRIDS, 5); mixer(2, 96, 0, 20, 0);
        static const vstep_t S[] = {{0, 0, 0, "ENERGY", "ENERGY: one control over how busy every generator is"},
                                    {1500, 0, 0, "ENERGY", "LEVEL up: the tracks fill..."},
                                    {5500, 0, 0, "ENERGY", "...LEVEL down: they thin out to a few notes"}};
        static const vturn_t T[] = {{1600, EN_K3, 30, 2500}, {5600, EN_K3, -60, 3000}};
        V_CLIP("n08-energy", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(96, ENGI_FM6, FX_KOTO_PLUCK, TESS_ENG_TURING, DIV_8); v_pad();
        sound(2, ENGI_FM6, V_DX_MARIMBA); trk[2].p[P_SDIV] = DIV_16; gen(2, TESS_ENG_EUCLID, 41); mixer(2, 96, 0, 60, 0);
        static const vstep_t S[] = {{0, 0, 2, "TWEAK", "TWEAK on track 3: a EUCLID rhythm on its own note, under track 1's melody"},
                                    {3000, 0, 2, "TWEAK", "FOLLOW T1: it plays track 1's notes..."},
                                    {6000, 0, 2, "TWEAK", "...SHIFT down an octave, DROP leaves some out"}};
        static const vturn_t T[] = {{3100, EN_K4, 1, 100}, {6100, EN_K1, -5, 1000}, {7400, EN_K2, 12, 1200}};
        V_CLIP("s1-tweak", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
    {
        v_scene(92, ENGI_FM6, FX_KALIMBA, TESS_ENG_TURING, DIV_8); v_pad();
        static const vstep_t S[] = {{0, 0, 0, "TIMING", "TIMING: eighth notes, one hit a step"},
                                    {2500, 0, 0, "TIMING", "RATCH x4: some steps roll..."},
                                    {6000, 0, 0, "TIMING", "...SHAPE ACCEL: faster and faster"}};
        static const vturn_t T[] = {{2600, EN_K1, 3, 900}, {6100, EN_K3, 1, 100}};
        V_CLIP("s2-timing", 10, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), VN(T));
    }
}
static int make(const char *dir, uint32_t secs);
static int video(const char *out)
{
    char cap[600];
    mkdir(out, 0755);
    snprintf(cap, sizeof cap, "%s/captions.txt", out);
    if (!(v_cap = fopen(cap, "w"))) return 1;
    v_root = out;
    double t = 0;
    for (int d = 0; d < (int)(sizeof V_DEMO_SECS / sizeof V_DEMO_SECS[0]); d++) {   /* the 3.0 demos (the video's three),
                                                         * one make each, each in a fresh process as the
                                                         * tests' run is (a load keeps a little of what played before,
                                                         * FALL's leaves, and power_on does not clear it): the one
                                                         * filmed plays V_DEMO_SECS, the others 20 s as in the tests */
        char sub[600];
        snprintf(sub, sizeof sub, "%s/%c", out, 'a' + d);
        mkdir(sub, 0755);
        fflush(stdout);
        fclose(v_cap);
        pid_t pid = fork();
        if (pid == 0) {
            v_cap = fopen(cap, "a");
            v_t0 = t;
            v_want = d;
            make(sub, 20);                              /* OUTDIR/a/still-tide.wav, b/koto-garden.wav, c/low-harbour.wav */
            fclose(v_cap);
            fflush(stdout);
            _exit(0);
        }
        waitpid(pid, 0, 0);
        v_cap = fopen(cap, "a");
        t += V_DEMO_SECS[d];
    }
    v_t0 = t;
    v_generators(out);
    v_features(out);
    {   /* the end: LOW HARBOUR again, from a minute in, under the end card while it fades (tools/demo_video.py) */
        power_on();
        demo_dub();
        static const vstep_t S[] = {{0, 1, V_KEEP, 0, "LOW HARBOUR"}};
        static const vclip_t C = {"zz-end", 12, 60, VN(S), V_NO(V_NOKEYS), V_NO(V_NOBTNS), V_NO(V_NOTURNS)};
        v_run(out, &C);
    }
    fclose(v_cap);
    printf("video: %.0f s in %s\n", v_t0, out);
    return 0;
}

/* each track alone (the others' LEVEL 0) for secs s: its rms and peak, to balance the mix by */
static void levels(uint32_t secs)
{
    for (uint32_t d = 0; d < NDEMO; d++) {
        printf("%-12s", DEMOS[d].name);
        for (uint32_t k = 0; k < NTRK; k++) {
            power_on(); DEMOS[d].make();
            for (uint32_t j = 0; j < NTRK; j++) if (j != k) trk[j].p[P_LEVEL] = 0;
            dm_play_t p = play(secs, 0);
            printf("  T%u rms %.3f pk %.2f", k + 1u, p.rms, p.peak / 32768.0);
        }
        printf("\n");
        power_on(); DEMOS[d].make();
        memset(voice_kills_part, 0, sizeof voice_kills_part);
        dm_play_t p = play(secs, 0);
        printf("%-12s  taken %u: T1 %u T2 %u T3 %u T4 %u\n", "", p.taken, voice_kills_part[0], voice_kills_part[1], voice_kills_part[2], voice_kills_part[3]);
    }
}


/* solo N TRACK SECS [WAV]: demo N with only TRACK (1..4; 0 all) heard, the others' levels at 0 (they still play, so the
 * cost is the whole demo's); prints the voices' cost by audio_batch.h's per-voice figures (ORACLE 3100, FM6 1250, 808
 * 1150, 909 1900; TRIO counted 150 by its render's size), as mean, 99th percentile and peak of a block: a device CPU
 * guide to keep a demo at or under the 3.0 demos that play clean */
#include <time.h>
static uint32_t dm_cost_of(uint32_t k)
{
    switch (trk[k].engine) {
    case ENGI_ORACLE: return 3100u;
    case ENGI_FM6: return 1250u;
    case ENGI_DRUM: return 1500u;
    case ENGI_TRIO: return 150u;
    default: return 1000u;
    }
}
static void solo(uint32_t d, uint32_t only, uint32_t secs, const char *wav)
{
    static uint32_t hist[64];
    FILE *w = 0;
    uint32_t f, frames = secs * FS, nb = 0, peak = 0, k, i;
    double sum = 0, ns = 0, nsmax = 0, sq = 0;
    memset(hist, 0, sizeof hist);
    power_on(); DEMOS[d].make();
    if (only) for (k = 0; k < NTRK; k++) if (k + 1u != only) trk[k].p[P_LEVEL] = 0;
    if (wav && !(w = fopen(wav, "wb"))) fprintf(stderr, "demos: cannot write %s\n", wav);
    if (w) wav_hdr(w, frames);
    transport_req = 1;
    uint32_t kills0 = voice_kills, steals0 = capacity_steals, kprev = voice_kills;
    for (f = 0; f < frames; f += CTL) {
        int32_t o[2 * CTL];
        struct timespec a, b;
        dm_f = f;
        clock_gettime(CLOCK_MONOTONIC, &a);
        mix_block(o, CTL);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double t = (b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec);
        ns += t; if (t > nsmax && f > FS) nsmax = t;
        fm1_ms = f / (FS / 1000u);
        uint32_t c = 0;
        for (k = 0; k < NPART; k++) for (i = 0; i < NVOICE; i++) if (trk[k].v[i].active) c += dm_cost_of(k);
        sum += c; nb++; if (c > peak) peak = c;
        if (getenv("DM_KILLS") && voice_kills != kprev) {   /* (DM_KILLS: when a voice is taken, and the busy voices) */
            fprintf(stderr, "%.2f s: taken, busy", (double)f / FS);
            for (k = 0; k < NPART; k++) { uint32_t b = 0; for (i = 0; i < NVOICE; i++) b += trk[k].v[i].active; fprintf(stderr, " T%u %u", k + 1u, b); }
            fprintf(stderr, "\n");
        }
        kprev = voice_kills;
        hist[c / 1000u < 63u ? c / 1000u : 63u]++;
        for (i = 0; i < 2u * CTL; i++) sq += (double)o[i] * o[i];
        if (w) for (i = 0; i < CTL; i++) wav_put(w, o[2 * i], o[2 * i + 1]);
    }
    if (w) fclose(w);
    uint32_t acc = 0, p99 = 0;
    for (i = 0; i < 64u; i++) { acc += hist[i]; if (acc * 100u >= nb * 99u) { p99 = i * 1000u + 999u; break; } }
    printf("%-12s T%u: cost mean %.0f p99 <%u peak %u; host %.1f us/block (max %.0f); rms %.3f; taken %u, restarted sounding %u\n", DEMOS[d].name,
           only, sum / nb, p99, peak, ns / nb / 1000.0, nsmax / 1000.0, sqrt(sq / (2.0 * frames)) / 32768.0, voice_kills - kills0,
           capacity_steals - steals0);
}

int main(int argc, char **argv)
{
    int bad;
    if (argc >= 3 && !strcmp(argv[1], "checks")) {     /* checks make DIR [SECS] | checks check DIR: the test projects */
        DEMOS = CHECK_LIST; NDEMO = sizeof CHECK_LIST / sizeof CHECK_LIST[0];
        argv++; argc--;
    }
    if (argc >= 3 && !strcmp(argv[1], "make")) bad = make(argv[2], argc > 3 ? (uint32_t)atoi(argv[3]) : 60u);
    else if (argc >= 3 && !strcmp(argv[1], "check")) bad = check_dir(argv[2]);
    else if (argc >= 4 && !strcmp(argv[1], "log")) {     /* log DEMO SECS: its notes as CSV on stdout */
        dm_log = stdout; power_on(); DEMOS[atoi(argv[2])].make();
        if (getenv("DM_DEBUG")) for (uint32_t k = 0; k < NTRK; k++) fprintf(stderr, "T%u scl %d root %d qnt %d eng %d\n", k + 1u, trk[k].p[P_SCALE], trk[k].p[P_ROOT], trk[k].p[P_QUANT], trk[k].engine);
        play((uint32_t)atoi(argv[3]), 0); return 0;
    }
    else if (argc >= 5 && !strcmp(argv[1], "solo")) {
        solo((uint32_t)atoi(argv[2]) % NDEMO, (uint32_t)atoi(argv[3]), (uint32_t)atoi(argv[4]), argc > 5 ? argv[5] : 0);
        return 0;
    }
    else if (argc >= 2 && !strcmp(argv[1], "presets")) {   /* presets: each engine's presets by number (sound()'s) */
        static const uint32_t E[] = {ENGI_FM6, ENGI_TRIO, ENGI_ORACLE, ENGI_DRUM};
        for (uint32_t k = 0; k < 4u; k++) {
            const engine_t *e = ENGINES[E[k]];
            printf("%s:", e->name);
            for (uint32_t i = 0; i < e->npresets; i++) printf(" %u %s;", i, eng_preset(e, i)->name);
            printf("\n");
        }
        return 0;
    }
    else if (argc >= 4 && !strcmp(argv[1], "form")) { form_log((uint32_t)atoi(argv[2]) % NDEMO, (uint32_t)atoi(argv[3])); return 0; }
    else if (argc >= 3 && !strcmp(argv[1], "archive")) {   /* archive OUT.json: a new device's whole backup */
        power_on();
        bad = check("a whole archive written as the backup page writes it", archive_write(argv[2]) == 0);
    }
    else if (argc >= 3 && !strcmp(argv[1], "restore")) bad = archive_restore(argv[2]);
    else if (argc >= 3 && !strcmp(argv[1], "video")) return video(argv[2]);
    else if (argc >= 2 && !strcmp(argv[1], "levels")) { levels(argc > 2 ? (uint32_t)atoi(argv[2]) : 60u); return 0; }
    else { fprintf(stderr, "demo_projects [checks] make DIR [SECS] | [checks] check DIR\n"); return 2; }
    printf(bad ? "demos: %d FAILED\n" : "demos passed\n", bad);
    return bad != 0;
}
