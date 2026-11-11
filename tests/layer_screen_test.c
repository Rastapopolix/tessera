/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of Felucca 1.2's panel controls as Melodee has them, through the UI test's stubs (ui_test.c):
 * double-tapped quick layers stay open and own the keys silently, FX LATCH, the FX key map (PRESETS assigns, EDIT
 * restores the default), MENU > SCREEN OFF (the panel dark while the transport runs, the wake press swallowed, NEVER),
 * LFO 2 (SYNC's increments at every division and tempo, TRIG NOTE / FREE, POL UNI, the transport's reset), and MODS
 * scrolled, the mod shortcut's knobs and MODS' AMOUNT turned (Mono) with no pixel sent twice in a frame. */
#include <stdint.h>
static uint32_t lcd_commands[8], lcd_count;
#define UI_LCD_POWER(s) (lcd_commands[lcd_count++ % 8u] = (s))
#define UI_LCD_WAKE() ((void)0)
#define UI_TEST_NO_MAIN 1
/* the mods' sources moving between a frame's canvases, as the device's audio interrupt moves them while the UI draws
 * (host_mid_audio: each track's LFO a step on, fast) */
static int host_mid_audio;
static void host_mid_draw(void);
#define GFX_HOOK_TEXT(x0, y0, x1, y1, s, flags) ((void)0)
#define GFX_HOOK_BLIT(x, y, r0) ((void)0)
#define GFX_HOOK_BEGIN() host_mid_draw()
#define GFX_HOOK_PIXELS(n) ((void)0)
#include "ui_test.c"
#include <assert.h>
static void host_mid_draw(void)
{
    if (!host_mid_audio)
        return;
    for (uint32_t i = 0; i < NTRK; i++) {
        trk[i].lfo_fade = 32767;
        trk[i].lfo_val = (int32_t)((trk[i].lfo_val + 32768 + 9001 + 1000 * (int32_t)i) % 65536) - 32768;
    }
    cord_tick();                                        /* (the block's mods, from them) */
}

static void reset_controls(void)
{
    seq_stop(); ui_power_on();
    memset(&scrn, 0, sizeof scrn); lcd_count = 0;
    settings_screen = kb_asleep = kb_lock = 0;
}
static void tap_fast(uint32_t b) { btn_down(b); frame(); btn_up(b); frame(); }
static void test_layers(void)
{
    for (uint32_t l=LAYER_GLO;l<LAYER_N;l++) {
        reset_controls(); uint32_t b=LAYERS[l].btn;
        tap_fast(b); tap_fast(b);
        assert(ui.lock==l && ui.layer==l && ui.home);
        key_down(white(0)); frame();
        assert(kb_layer & (1u<<white(0))); assert(!gates());
        key_up(white(0)); frame(); tap_fast(b);
        assert(!ui.lock && !ui.layer);
    }
    puts("layers/screen: double-tap layers and silent layer keys passed");
}
static void test_screen_sleep(void)
{
    reset_controls(); scr_put(1); song.playing=1;
    fm1_ms=scrn.idle+300000u; ui_draw();
    assert(scrn.st==SCR_OFF && kb_asleep && song.playing && lcd_commands[0]==0);
    key_down(white(0)); frame(); assert(!gates() && scrn.st==SCR_WAKE);
    fm1_ms+=120; ui_draw(); assert(scrn.st==SCR_SLPOUT && lcd_commands[1]==1);
    fm1_ms+=120; ui_draw(); assert(scrn.st==SCR_ON && lcd_commands[2]==2);
    key_up(white(0)); fm1_ms+=320; frame(); assert(!kb_asleep);
    key_down(white(0)); frame(); assert(gates()); key_up(white(0)); frame();
    song.playing=0;
    for(uint32_t v=0;v<5;v++){scr_put(v);assert(scr_get()==v);}
    scr_put(0);fm1_ms+=3600001u;ui_draw();assert(scrn.st==SCR_ON);
    scr_put(1);ui.confirm=1;fm1_ms+=300001;ui_draw();assert(scrn.st==SCR_ON);
    ui.confirm=0;scr_put(1);fm1_ms+=300001;ui_draw();assert(scrn.st==SCR_OFF);
    int16_t value=TSEL->p[P_LEVEL];host_enc[panel.enc[EN_K1]]=2;frame();assert(TSEL->p[P_LEVEL]==value);
    scr_wake_now();fm1_ms+=320;frame();assert(!kb_asleep);
    puts("layers/screen: screen timeout, continuing transport, silent wake key/knob and NEVER passed");
}
static void test_lfo(void)
{
    reset_controls(); track_t *t=TSEL;
    for(uint32_t s=1;s<NELEM(N_LSYNC);s++){
        t->p[P_LSYNC]=s;lfo_sync_rem[t-trk]=0;
        /* One cycle of units is exactly one turn, however the blocks fall (and at any tempo: units, mix.c). */
        uint32_t turn=0;
        for(uint32_t left=seq_div_samples(LSYNC_DIV[s]);left;left-=lfo_block_units){
            lfo_block_units=left<37u?left:37u;turn+=lfo_sync_inc(t);
        }
        assert(!turn);
    }
    t->p[P_LSYNC]=0;t->p[P_LTRIG]=1;t->lfo_ph=0x34567890;
    trk_note_on(t,60,100);assert(t->lfo_ph==0x34567890);trk_note_off(t,60);
    t->p[P_LTRIG]=0;trk_note_on(t,64,100);assert(!t->lfo_ph);
    t->p[P_LPOL]=1;t->p[P_LWAVE]=0;
    int32_t out[CTL];for(uint32_t i=0;i<2000;i++){track_render(t,out,CTL);assert(t->lfo_val>=0 && t->lfo_val<=32767);}
    t->p[P_LTRIG]=1;t->p[P_LSYNC]=5;t->lfo_ph=1;seq_start();assert(!t->lfo_ph);seq_stop();
    puts("layers/screen: all synced LFO divisions, NOTE/FREE, UNI and transport reset passed");
}
/* MODS (Mono): KNOB 2 scrolling sixteen mods sends each row once, its move and light drawn in it; no pixel goes out
 * twice in a frame, different (3.5-beta5: each row went out blank under them, then they: the bars and lights flickered
 * on the device as the mods scrolled). Also with the MOD SCOPE opened (OCT- held) and let go between */
static void mods_scroll(int32_t d)
{
    for (uint32_t i = 0; i + 1u < CORD_LIVE; i++) turn(EN_K1 + 1u, d);
}
static void test_mods_scroll(void)
{
    reset_controls(); cord_clear();
    for (uint32_t i = 0; i < CORD_LIVE; i++) {
        cord_t c = {0};
        c.src = i & 1u ? CS_LFO : CS_ENV; c.src_trk = (uint8_t)(i & 3u);
        c.dst = CD_CUTOFF; c.dst_trk = (uint8_t)(i & 3u); c.amount = (int8_t)(20 + 5 * (int32_t)i);
        assert(cord_set(i, &c));
    }
    go_title("MODS"); assert(cur_page()->graph == GR_CORDS);
    cord_ui_sel = 0; ui.force = 1; frame(); frame();
    uint64_t sent = host_sent;
    host_redrawn = 0;
    mods_scroll(1);
    assert(cord_ui_sel == CORD_LIVE - 1u && host_sent > sent);    /* (scrolled: the rows went out) */
    mods_scroll(-1);
    assert(!cord_ui_sel);
    if (host_redrawn) printf("MODS scrolled: %llu pixels sent twice in a frame\n", (unsigned long long)host_redrawn);
    assert(!host_redrawn);
    for (uint32_t i = 0; i < 4u; i++) {                 /* the scope: OCT- held a while, let go, scroll again */
        pr_hold(1); for (uint32_t f = 0; f < 60u; f++) frame();   /* (past a tap: no Back) */
        assert(cu_scope);
        pr_hold(0); frame();
        assert(!cu_scope && cur_page()->graph == GR_CORDS && !ui.home);
        host_redrawn = 0;
        mods_scroll(i & 1u ? -1 : 1);
        if (host_redrawn) printf("MODS after the scope: %llu pixels sent twice in a frame\n", (unsigned long long)host_redrawn);
        assert(!host_redrawn);
    }
    cord_clear();
    puts("MODS: sixteen mods scrolled with KNOB 2, before and after the scope: every row sent once, its move and light in it");
}
/* FX held + a knob (the mod shortcut, ui_modsc.c): its card over the dimmed page; each turn after the first redraws the
 * card alone, no pixel sent twice in a frame (3.5-beta5: every turn sent the whole page dimmed, then the card over it:
 * the card flickered on the device while a knob turned). On a sound page and an FX page */
static void test_msc_turns(const char *title)
{
    reset_controls(); cord_clear();
    go_title(title); assert(!ui.home && str_eq(cur_page()->title, title));
    btn_down(B_FX); frame();
    turn(EN_K1 + 1u, 1);                                /* (opened: the page under it, then the card, once) */
    assert(msc.on);
    host_redrawn = 0;
    for (uint32_t k = 0; k < 4u; k++)
        for (uint32_t i = 0; i < 6u; i++) turn(EN_K1 + k, i & 1u ? -1 : 2);
    if (host_redrawn) printf("FX held on %s, knobs turned: %llu pixels sent twice in a frame\n", title,
                             (unsigned long long)host_redrawn);
    assert(!host_redrawn);
    btn_up(B_FX); frame(); frame();
    assert(!msc.on);
    cord_clear();
    printf("FX held on %s: knobs turned, the mod card sent once a frame\n", title);
}
/* MODS (Mono): KNOB 4 (AMOUNT) turned on the selected mod, its source still and moving: no pixel sent twice in a frame.
 * Moving, it moves between the frame's canvases (host_mid_audio, as on the device while the UI draws): 3.5-beta5 sent each
 * row with its move and light, then the moves and lights again (cu_leds, the turn's ui.force), the marks moved on
 * between: the selected mod's row flickered as its amount changed */
static void mods_amount(int moving)
{
    for (uint32_t sel = 0; sel < 4u; sel += 3u) {
        cord_ui_sel = sel; ui.force = 1; frame(); frame();
        int32_t a = cord_get(sel)->amount;
        host_mid_audio = moving;
        host_redrawn = 0;
        for (uint32_t i = 0; i < 12u; i++) turn(EN_K1 + 3u, i < 6u ? 1 : -1);
        assert(cord_get(sel)->amount == a);
        for (uint32_t i = 0; i < 6u; i++) turn(EN_K1 + 3u, 1);
        host_mid_audio = 0;
        assert(cord_get(sel)->amount != a);
        if (host_redrawn) printf("MODS AMOUNT turned (mod %u%s): %llu pixels sent twice in a frame\n", sel + 1u,
                                 moving ? ", its source moving" : "", (unsigned long long)host_redrawn);
        assert(!host_redrawn);
    }
}
static void test_mods_amount(void)
{
    reset_controls(); cord_clear();
    for (uint32_t i = 0; i < 4u; i++) {
        cord_t c = {0};
        c.src = CS_LFO; c.src_trk = (uint8_t)i; c.dst = CD_CUTOFF; c.dst_trk = (uint8_t)i;
        c.amount = 30;
        int set = cord_set(i, &c);
        assert(set);
    }
    go_title("MODS"); assert(cur_page()->graph == GR_CORDS);
    mods_amount(0);
    mods_amount(1);
    cord_clear();
    puts("MODS: AMOUNT turned on a selected mod, its source still and moving: every pixel sent once a frame");
}
/* the FX key map and the native CZ-1 favorites share no bits (the map sat in CZ's row, factory[15][14..23], over
 * native slots 47..126's stars), and a settings record from before the move keeps its map, or its stars when its
 * bytes cannot be a map (a code no map writes) or sit only on filled slots */



/* the PATTERN sheet's Direction row, a mod moving DIR (3.5-beta7: the row flickered, "FWD>PEND" over its value) */
static void test_dir_sheet(void)
{
    reset_controls(); cord_clear();
    cord_t c = {0};
    c.src = CS_LFO; c.src_trk = 0; c.dst = CD_SEQ_DIR; c.dst_trk = 0; c.amount = 100;
    assert(cord_set(0, &c));
    go_title("PATTERN"); frame();
    page_sheet_open(); frame();
    assert(pop.on == POP_SHEET);
    for (pop.sel = 0; pop.sel < pop.n && !str_eq(pop.rows[pop.sel].label, "Direction"); pop.sel++) ;
    assert(pop.sel < pop.n);
    ui.force = 1; frame(); frame();
    uint64_t sent = host_sent;
    unsigned changes = 0; char was[20] = "", v[20];
    host_mid_audio = 1;
    host_redrawn = 0;
    for (uint32_t f = 0; f < 120u; f++) {
        trk[0].lfo_fade = 32767;                        /* (T1's LFO sweeping DIR, a frame a step) */
        trk[0].lfo_val = (int32_t)((f * 4001u) % 65536u) - 32768;
        cord_tick();
        frame();
        pop.rows[pop.sel].value(v);
        if (!str_eq(v, was)) { changes++; str_cpy(was, v, sizeof was); }
        if (changes > 1u) assert(strchr(v, '>'));      /* (live: its width kept, no "FWD" a step, "FWD>PEND" the next) */
    }
    host_mid_audio = 0;
    printf("DIR sheet: %u value changes, %llu bytes sent, %llu pixels sent twice in a frame (%s)\n", changes,
           (unsigned long long)(host_sent - sent), (unsigned long long)host_redrawn, was);
    assert(changes <= 120u * 16u / 300u + 2u && !host_redrawn);   /* (at most one change in 300 ms; 44 before) */
    pop_close(); cord_clear();
}
int main(void) { test_layers(); test_screen_sleep(); test_lfo(); test_mods_scroll();
    test_msc_turns("ENV"); test_msc_turns("SEND FX 3"); test_mods_amount(); test_dir_sheet(); return 0; }
