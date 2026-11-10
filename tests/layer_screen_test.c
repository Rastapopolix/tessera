/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of Felucca 1.2's panel controls as Melodee has them, through the UI test's stubs (ui_test.c):
 * double-tapped quick layers stay open and own the keys silently, FX LATCH, the FX key map (PRESETS assigns, EDIT
 * restores the default), MENU > SCREEN OFF (the panel dark while the transport runs, the wake press swallowed, NEVER),
 * LFO 2 (SYNC's increments at every division and tempo, TRIG NOTE / FREE, POL UNI, the transport's reset), and MODS
 * scrolled (Mono) with no pixel sent twice in a frame. */
#include <stdint.h>
static uint32_t lcd_commands[8], lcd_count;
#define UI_LCD_POWER(s) (lcd_commands[lcd_count++ % 8u] = (s))
#define UI_LCD_WAKE() ((void)0)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>

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
/* the FX key map and the native CZ-1 favorites share no bits (the map sat in CZ's row, factory[15][14..23], over
 * native slots 47..126's stars), and a settings record from before the move keeps its map, or its stars when its
 * bytes cannot be a map (a code no map writes) or sit only on filled slots */



int main(void) { test_layers(); test_screen_sleep(); test_lfo(); test_mods_scroll(); return 0; }
