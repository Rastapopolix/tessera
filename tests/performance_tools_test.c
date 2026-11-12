/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>

static void reset_tools(void)
{
    seq_stop(); momentary_restore(); ui_power_on(); set_engine_of(&trk[0], 6); trk[0].engine = trk[0].eng_req; recording_reset();   /* (TRIO: knobs and CC macros on p[]) */ memset(&midi_clock,0,sizeof midi_clock); song.master_q12=4096;
    settings_click=settings_countin=settings_preview=settings_chord_add=0;
    settings_click_level=1; memset(cin_n,0,sizeof cin_n); clk.env=click_req=0;
}
static void advance(uint32_t samples)
{
    while(samples){uint32_t n=samples>CTL?CTL:samples;events_block(n);samples-=n;}
}
static void page_named(const char *name)
{
    for(uint32_t i=0;i<NPAGES;i++)if(!strcmp(PAGES[i].title,name)){ui.home=0;ui.page=i;return;}
    assert(0);
}
static void test_recording_tools(void)
{
    reset_tools();settings_countin=1;press(B_REC);assert(song.rec==1 && !transport_req && !song.playing);
    press(B_REC);assert(!song.rec && !transport_req);press(B_REC);press(B_PLAY);events_block(0);
    assert(seq_counting() && !song.playing && transport_busy() && click_req==2);
    uint32_t beat=beat_samples();advance(3*beat+beat/2);assert(cin_left==1 && !song.playing);
    input_on(TSEL,60,100);assert(recording_head[0]>=RECORD_MAX);
    advance(beat-beat/2-1);assert(!song.playing);
    advance(1);assert(song.playing && !seq_counting() && TSEL->seq_idx==0);
    assert(TSEL->rh_n==1);input_off(TSEL,60);seq_stop();
    transport_req=1;events_block(0);assert(seq_counting());transport_req=2;events_block(1);assert(!seq_counting() && !song.playing);
    settings_countin=2;transport_req=1;events_block(0);advance(8*beat-1);assert(seq_counting());advance(1);assert(song.playing);
    seq_stop();song.g[G_CLOCK]=1;transport_req=1;events_block(0);assert(seq_counting() && !song.playing);
    midi_enqueue(0xfau<<8|15u,1);events_block(0);assert(!seq_counting() && song.playing);seq_stop();
    reset_tools();settings_click=CLICK_ON;seq_start();assert(click_req==2);
    int32_t audio[2*CTL]={0};click_render(audio,CTL);int energy=0;
    for(uint32_t i=0;i<2*CTL;i++)energy|=audio[i];assert(energy);
    seq_advance(seq_beat_samples());assert(click_req==1);
    seq_stop();settings_click=CLICK_OFF;seq_start();assert(!click_req);seq_stop();
}
static void test_click_volume(void)
{
    reset_tools();
    int32_t reference[2*CTL], actual[2*CTL];
    for(uint32_t beat=1;beat<=2;beat++) {
        uint32_t previous_peak=0;
        for(uint32_t level=0;level<3;level++) {
            settings_click_level=level;song.master_q12=4096;
            memset(reference,0,sizeof reference);click_req=beat;click_render(reference,CTL);
            uint32_t peak=0;
            for(uint32_t i=0;i<2*CTL;i++) { uint32_t a=(uint32_t)abs(reference[i]);if(a>peak)peak=a; }
            assert(peak>previous_peak);previous_peak=peak;
            for(uint32_t master=0;master<=4096;master+=1024) {
                song.master_q12=master;memset(actual,0,sizeof actual);click_req=beat;click_render(actual,CTL);
                for(uint32_t i=0;i<2*CTL;i++)
                    assert(abs(actual[i]-((reference[i]*(int32_t)master)>>12))<=1);
                if(!master)for(uint32_t i=0;i<2*CTL;i++)assert(!actual[i]);
            }
        }
    }
    /* Master changes apply to a sounding tail without waiting for another beat. */
    song.master_q12=0;
    memset(actual,0,sizeof actual);click_render(actual,CTL);
    for(uint32_t i=0;i<2*CTL;i++)assert(!actual[i]);
    song.master_q12=4096;
    memset(actual,0,sizeof actual);click_render(actual,CTL);
    int energy=0;for(uint32_t i=0;i<2*CTL;i++)energy|=actual[i];assert(energy);
    /* Count-in follows master even when AUDIO CLICK is off. */
    for(uint32_t master=0;master<=4096;master+=1024) {
        seq_stop();settings_click=CLICK_OFF;settings_countin=1;song.rec=1;song.master_q12=master;
        transport_req=1;events_block(0);assert(seq_counting() && click_req==2);
        memset(actual,0,sizeof actual);click_render(actual,CTL);
        for(uint32_t i=0;i<2*CTL;i++)
            assert(abs(actual[i]-((reference[i]*(int32_t)master)>>12))<=1);
        if(!master)for(uint32_t i=0;i<2*CTL;i++)assert(!actual[i]);
    }
    seq_stop();
}
static void test_editing_tools(void)
{
    reset_tools();go_page(GR_ROLL);TSEL->p[P_VOICE]=V_POLY;settings_chord_add=1;
    uint8_t a=60,b=64;seq_entry_notes(&a,1);seq_entry_finish();assert(ui.cursor==0 && TSEL->step[0].n==1);
    seq_entry_notes(&b,1);seq_entry_finish();assert(TSEL->step[0].n==2);
    seq_entry_notes(&a,1);seq_entry_finish();assert(TSEL->step[0].n==1 && TSEL->step[0].note[0]==64);
    settings_chord_add=0;seq_entry_notes(&a,1);seq_entry_finish();assert(ui.cursor==1);
    settings_preview=1;cursor_set(0);assert(audition_request);events_block(1);assert(audition_active && recording_head[0]>=RECORD_MAX);
    advance(FS/6);assert(!audition_active);input_on(TSEL,60,90);cursor_set(0);events_block(1);
    assert(!audition_n);advance(FS/6);assert(live_held[0][60>>5]&(1u<<(60&31u)));input_off(TSEL,60);
    reset_tools();ui.home=1;uint32_t param=ENGINES[TSEL->eng_req]->knob[0];int16_t before=TSEL->p[param];btn_down(B_LFO);turn(EN_K1,5);
    assert(momentary.active && TSEL->p[param]!=before);fm1_in.buttons&=~(1u<<panel.btn[B_LFO]);frame();
    assert(!momentary.active && TSEL->p[param]==before && ui.home);
    btn_down(B_LFO);turn(EN_K1,-4);int16_t kept=TSEL->p[param];press(B_OCTUP);
    fm1_in.buttons&=~(1u<<panel.btn[B_LFO]);frame();assert(TSEL->p[param]==kept && song.octave==0);
    set_engine_of(TSEL,ENGI_FM6);TSEL->engine=TSEL->eng_req;page_named("ALGO");
    uint8_t patch[FP_SIZE+1];memcpy(patch,fm6_patch[0],sizeof patch);
    btn_down(B_LFO);turn(EN_K1,1);assert(momentary.active);fm1_in.buttons=0;frame();assert(!memcmp(patch,fm6_patch[0],sizeof patch));
}
static void test_midi_and_levels(void)
{
    reset_tools();midi_event(0xb0,0,7,127);assert(TSEL->p[P_LEVEL]==127);
    midi_event(0xb0,0,10,64);assert(TSEL->p[P_PAN]==0);
    {   /* CC 74 / 71 on an engine without a map (TRIO): its sound values untouched */
        int16_t e[8];memcpy(e,TSEL->p+P_E0,sizeof e);midi_event(0xb0,0,74,0);midi_event(0xb0,0,71,127);
        assert(!memcmp(e,TSEL->p+P_E0,sizeof e));
    }
    song.g[G_ROUTE]=0;midi_event(0xb0,1,91,88);assert(!trk[1].p[P_RESERVED36] && !trk[0].p[P_RESERVED36]);
    set_engine_of(TSEL,ENGI_FM6);TSEL->engine=TSEL->eng_req;
    midi_event(0xb0,0,74,0);assert(TSEL->p[P_E2]==ENGINES[ENGI_FM6]->edit[2].min);   /* (CC 74: FM6's MLVL) */
    midi_event(0xb0,0,73,70);for(uint32_t op=0;op<6;op++)assert(fm6_patch[0][op*FP_OP]==99u-70u*99u/127u);   /* (FM6's own envelope: R1) */
    reset_tools();song.rec=1;seq_start();seq_advance(0);midi_event(0xb0,0,7,17);
    assert(motion.count==1 && motion.event[0].param==P_LEVEL && motion.event[0].value==17);seq_stop();
    reset_tools();track_select(3);TSEL->p[P_LN0]=0;trk_note_on(TSEL,36,127);
    int32_t out[2*CTL];mix_block(out,CTL);for(uint32_t i=0;i<2*CTL;i++)assert(out[i]==0);
    TSEL->p[P_LN0]=127;mix_block(out,CTL);int energy=0;for(uint32_t i=0;i<2*CTL;i++)energy|=out[i];assert(energy);
}
static void test_compatibility(void)
{
    reset_tools();settings_click=2;settings_click_level=0;settings_countin=2;settings_preview=settings_chord_add=1;
    persist_t p={0};settings_export(&p);settings_click=settings_countin=settings_preview=settings_chord_add=0;settings_click_level=1;
    assert(settings_import(&p,sizeof p));assert(settings_click==2 && !settings_click_level && settings_countin==2 && settings_preview && settings_chord_add);
    p.zoom=1;assert(settings_import(&p,sizeof p));assert(!settings_click && settings_click_level==1 && !settings_countin && settings_preview && !settings_chord_add);
    /* NOTE PREVIEW: ON unless chosen OFF; a record from before 3.3 (no tag, its old bit clear) reads ON */
    settings_preview=0;settings_export(&p);settings_preview=1;assert(settings_import(&p,sizeof p) && !settings_preview);
    p.favorites.factory[8][27]=0;p.zoom=0;assert(settings_import(&p,sizeof p) && settings_preview);
    static project_t q,back;static uint8_t current[TSP_MAX];project_capture(&q);q.t[0].p[P_E4]=53;
    for(uint32_t t=0;t<NTRK;t++)q.t[t].p[P_RESERVED35]=77;   /* delay sends and their automation (a FUN16's are kept) */
    q.g[G_RESERVED25]=3;q.g[G_RESERVED26]=9;q.g[G_RESERVED4]=2;q.motion.count=1;q.motion.event[0]=(motion_event_t){0,P_RESERVED35,100};q.sum=proj_sum(&q);
    uint32_t cn=tsp_pack(current,&q,0);assert(cn);
    assert(tsp_valid(current,cn) && (back=proj_scratch, 1) && back.t[1].p[P_RESERVED35]==77 && back.g[G_RESERVED25]==3 && back.g[G_RESERVED26]==9 &&
           back.motion.count==1 && back.motion.event[0].value==100);
    {   /* Both current and earlier user presets ignore retired effects in their live values. */
        up_rec_t r;int16_t v[P_COUNT];memset(&r,0,sizeof r);r.used=UP_USED;r.ver=UP_VER_GRID;r.engine=3;r.np=P_COUNT;memcpy(r.name,"OLD",3);
        for(uint32_t i=0;i<P_COUNT;i++)up_set_value(&r,i,param_desc_of(0,i)->def);
        up_set_value(&r,P_RESERVED35,77);up_values(&r,v);assert(v[P_RESERVED35]==0);
        r.np=100;up_values(&r,v);assert(v[P_RESERVED35]==0);
    }
    {   /* a factory preset: no delay send (its sends predate the delay coming back) */
        TSEL->eng_req=6;TSEL->p[P_RESERVED35]=77;apply_preset_to(TSEL,8);assert(TSEL->p[P_RESERVED35]==0 && TSEL->p[P_RESERVED36]==0);
    }
}
static int pv_sounds(const track_t *t, uint32_t note)
{
    for(uint32_t i=0;i<NVOICE;i++)if(t->v[i].active&&t->v[i].gate&&t->v[i].note==note)return 1;
    return 0;
}
/* NOTES KNOB 2: with NOTE PREVIEW on, the picked note's new pitch sounds at its velocity, briefly; off, nothing */
static void test_knob_preview(void)
{
    reset_tools();go_page(GR_ROLL);track_t *t=TSEL;t->p[P_QUANT]=Q_OFF;t->p[P_VOICE]=V_POLY;
    memset(t->step,0,sizeof t->step);t->p[P_SLEN]=16;
    step_t *st=&t->step[0];st->time=ST_NOTE;st->n=1;st->note[0]=60;st->vel=77;
    cursor_set(0);assert(!audition_request);
    turn(EN_K1+1,1);assert(st->note[0]>60 && !audition_request);st->note[0]=60;
    settings_preview=1;cursor_set(0);advance(FS/6+CTL);
    turn(EN_K1+1,1);uint32_t a=st->note[0];assert(a>60 && audition_request && audition_vel[0]==77 && audition_notes[0]==a);
    events_block(CTL);assert(pv_sounds(t,a) && audition_active);
    turn(EN_K1+1,1);uint32_t b=st->note[0];assert(b>a);events_block(CTL);
    assert(pv_sounds(t,b) && !pv_sounds(t,a));                 /* a turn more: only the new one */
    advance(FS/6+CTL);assert(!audition_active && !audition_n && !pv_sounds(t,b));   /* it ends by itself */
    for(uint32_t i=0;i<NVOICE;i++)assert(!t->v[i].active||!t->v[i].gate);
    assert(recording_head[recording_owner(t)]>=RECORD_MAX);
    /* an accent as it plays; a pitch at the end of the range: no change, nothing heard */
    st->flags|=SF_ACCENT;turn(EN_K1+1,1);assert(audition_request && audition_vel[0]==127);advance(FS/6+CTL);st->flags=0;
    st->note[0]=127;turn(EN_K1+1,1);assert(st->note[0]==127 && !audition_request);
    /* a note placed on an empty step sounds */
    st->note[0]=60;cursor_set(1);turn(EN_K1+1,1);assert(t->step[1].n==1 && audition_request);
    events_block(CTL);assert(pv_sounds(t,t->step[1].note[0]));advance(FS/6+CTL);
    /* REC armed: KNOB 2 edits nothing (STOP RECORDING), so nothing to hear; armed during a preview: it ends, unrecorded */
    cursor_set(0);advance(FS/6+CTL);song.rec=1;a=st->note[0];turn(EN_K1+1,1);assert(st->note[0]==a && !audition_request);song.rec=0;
    turn(EN_K1+1,1);events_block(CTL);assert(pv_sounds(t,st->note[0]));song.rec=1;
    advance(FS/6+CTL);assert(!pv_sounds(t,st->note[0]) && recording_head[recording_owner(t)]>=RECORD_MAX);song.rec=0;
    /* playing, it sounds too: the cursor's and KNOB 2's */
    step_t *far=&t->step[8];far->time=ST_NOTE;far->n=1;far->note[0]=72;far->vel=90;
    st->note[0]=60;seq_start();events_block(CTL);
    cursor_set(8);assert(audition_request);events_block(CTL);assert(audition_active && pv_sounds(t,72));
    turn(EN_K1+1,1);b=far->note[0];assert(b>72 && audition_request);events_block(CTL);assert(pv_sounds(t,b) && !pv_sounds(t,72));
    advance(FS/6+CTL);assert(!audition_active && !pv_sounds(t,b));
    /* a pattern's change moves the cursor; that is no choice of note */
    t->pattern_gen++;frame();assert(!audition_request);
    seq_stop();
    /* the step playing that pitch keeps it: no second note, and the preview's end leaves it */
    seq_start();events_block(CTL);assert(t->seq_n && t->seq_notes[0]==60 && pv_sounds(t,60));
    cursor_set(0);events_block(CTL);assert(audition_active && !audition_n && pv_sounds(t,60));
    audition_stop();assert(pv_sounds(t,60));
    /* recording: nothing */
    song.rec=1;cursor_set(1);assert(!audition_request);song.rec=0;seq_stop();
    /* the cursor's preview is still NOTE PREVIEW's and stays off MIDI */
    settings_preview=1;cursor_set(1);assert(audition_request && !audition_midi);advance(FS/6+CTL);settings_preview=0;
}
/* An ARP track: its tosses from the SEED (RANDOM's order and PROB replay from PLAY), TWEAK's SHIFT and HUMAN */
static uint32_t arp_run(track_t *t, uint8_t *notes, uint8_t *vels, uint32_t n)   /* n notes the arp plays, from PLAY */
{
    uint32_t got = 0, guard = 0, period = seq_div_samples((uint32_t)t->p[P_ARATE]);
    seq_sub_clear();                                             /* (PLAY: the tosses from the start) */
    arp_off(t); t->nheld = 0; t->arp_pos = 0xFFFFFFF; t->arp_idx = 0xFFFFFFFFu;
    arp_add(t, 60); arp_add(t, 64); arp_add(t, 67);
    while (got < n && guard++ < 400u) {
        arp_tick(t, period);
        if (t->arp_note) {
            notes[got] = t->arp_note; vels[got] = 0;
            for (uint32_t i = 0; i < NVOICE; i++) if (t->v[i].active && t->v[i].note == t->arp_note) vels[got] = t->v[i].vel;
            got++;
            arp_off(t);
        }
    }
    t->nheld = 0;
    return got;
}
static void test_arp_shape(void)
{
    reset_tools();track_t *t=TSEL;uint32_t tr=song.sel;
    uint8_t a[24],b[24],va[24],vb[24];
    t->p[P_AMODE]=4;t->p[P_APROB]=100;t->p[P_AOCT]=2;           /* RANDOM over two octaves, some steps skipped */
    t->p[P_VOICE]=V_POLY;                                         /* (TRIO's first sound is MONO) */
    memset(va,0,sizeof va);memset(vb,0,sizeof vb);
    assert(arp_run(t,a,va,24)==24 && arp_run(t,b,vb,24)==24);
    assert(!memcmp(a,b,24));                                     /* PLAY again: the same arpeggio */
    tessera.evolve[tr].shift=5;
    assert(arp_run(t,b,vb,24)==24);
    for(uint32_t i=0;i<24u;i++) assert(b[i]==a[i]+5);            /* SHIFT: every note a fourth up, the same tosses */
    tessera.evolve[tr].shift=0;tessera.evolve[tr].human=100;
    assert(arp_run(t,b,vb,24)==24 && !memcmp(a,b,24));           /* HUMAN: the notes as before */
    uint32_t differ=0;for(uint32_t i=1;i<24u;i++) differ+=vb[i]!=vb[0];
    assert(differ);                                              /* .. their velocities uneven */
    tessera.evolve[tr].human=0;
    t->p[P_AMODE]=0;
}
/* ARP MODE's orders for C E G (pressed E C G for ORD), RND1 repeating and replaying, the old values' meaning kept */
static uint32_t arp_keys(track_t *t, const uint8_t *keys, uint32_t nk, uint8_t *notes, uint32_t n)
{
    uint32_t got = 0, guard = 0, period = seq_div_samples((uint32_t)t->p[P_ARATE]);
    seq_sub_clear();
    arp_off(t); t->nheld = 0; t->arp_pos = 0xFFFFFFF; t->arp_idx = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < nk; i++) arp_add(t, keys[i]);
    while (got < n && guard++ < 400u) {
        arp_tick(t, period);
        if (t->arp_note) { notes[got++] = t->arp_note; arp_off(t); }
    }
    t->nheld = 0;
    return got;
}
static void test_arp_modes(void)
{
    static const uint8_t CEG[3] = {60, 64, 67}, ECG[3] = {64, 60, 67}, CEGC[4] = {60, 64, 67, 72};
    static const struct { int16_t mode; const uint8_t *keys; uint8_t want[6]; } W[] = {
        {1, ECG, {60, 64, 67, 60, 64, 67}},             /* UP: low to high, whatever the order pressed */
        {2, ECG, {67, 64, 60, 67, 64, 60}},             /* DN */
        {3, CEG, {60, 64, 67, 64, 60, 64}},             /* UPDN: the ends once */
        {7, CEG, {67, 64, 60, 64, 67, 64}},             /* DNUP: the ends once */
        {5, ECG, {64, 60, 67, 64, 60, 67}},             /* ORD: as pressed (ORDER NOTE too) */
        {6, ECG, {67, 67, 67, 67, 67, 67}},             /* REPEAT: the last pressed */
    };
    reset_tools(); track_t *t = TSEL; uint32_t tr = song.sel, i, k;
    uint8_t a[12], b[12];
    t->p[P_APROB] = 127; t->p[P_AOCT] = 1; t->p[P_AORDER] = 0;
    for (k = 0; k < sizeof W / sizeof W[0]; k++) {
        t->p[P_AMODE] = W[k].mode;
        assert(arp_keys(t, W[k].keys, 3, a, 6) == 6 && !memcmp(a, W[k].want, 6));
    }
    t->p[P_AORDER] = 1; t->p[P_AMODE] = 1;              /* ORDER PLAY: UP as pressed, as before */
    assert(arp_keys(t, ECG, 3, a, 3) == 3 && a[0] == 64 && a[1] == 60 && a[2] == 67);
    t->p[P_AORDER] = 0; t->p[P_AMODE] = 8;              /* RND1: one order of C E G, repeated */
    assert(arp_keys(t, CEG, 3, a, 12) == 12);
    assert((1u << (a[0] - 60)) + (1u << (a[1] - 60)) + (1u << (a[2] - 60)) == (1u << 0) + (1u << 4) + (1u << 7));
    for (i = 3; i < 12u; i++) assert(a[i] == a[i - 3]);
    assert(arp_keys(t, CEG, 3, b, 12) == 12 && !memcmp(a, b, 12));   /* PLAY again: the same order */
    assert(arp_keys(t, CEGC, 4, b, 12) == 12);          /* a note added: an order of the four, repeated */
    for (i = 4; i < 12u; i++) assert(b[i] == b[i - 4]);
    uint32_t differs = 0;                               /* other SEEDs: other orders */
    uint16_t seed = tessera.seed[tr];
    for (k = 1; k < 12u; k++) {
        tessera.seed[tr] = (uint16_t)(seed + k);
        assert(arp_keys(t, CEG, 3, b, 3) == 3);
        differs += memcmp(a, b, 3) != 0;
    }
    tessera.seed[tr] = seed;
    assert(differs);
    /* the stored values keep their meaning, the new ones appended: an older project plays as it did */
    const param_desc_t *d = param_desc_of(t->eng_req, P_AMODE);
    static const char *const OLD[] = {"OFF", "UP", "DN", "UPDN", "RND", "ORD", "REPEAT", "DNUP", "RND1"};
    assert(d->max == 8);
    for (k = 0; k <= 8u; k++) assert(!strcmp(d->names[k], OLD[k]));
    t->p[P_AMODE] = 0;
}
int main(void)
{
    test_arp_modes();
    test_recording_tools();test_click_volume();test_editing_tools();test_knob_preview();test_arp_shape();test_midi_and_levels();test_compatibility();
    puts("performance tools: count-in, click, additive entry, audition, momentary restore/keep, MIDI, drum levels and inert legacy effect values passed");return 0;
}
