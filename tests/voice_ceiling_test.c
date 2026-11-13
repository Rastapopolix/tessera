/* SPDX-License-Identifier: GPL-3.0-only */
/* The load ceiling (#586): an emergency shed caps admission at what was sounding; past it a new voice replaces one
 * (released first, bass kept); it holds while the load is high, recovers a unit per ~250 ms under 60 %, lets go at
 * BUDGET, never goes under 2 units, and BUDGET stays the maximum. Resident profile, as the device. */
#define MELODEE_RAM_AUDIO 1
#define MELODEE_DUAL_CORE 1
static int audio_worker_online=1;
static int audio_worker_submit(void (*fn)(void *),void *ctx){(void)fn;(void)ctx;return 0;}
static int audio_worker_join(void){return 1;}
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static void fresh(void)
{
    memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
    memset(resource,0,sizeof resource);
    host_tracks_init();
    memset(voice_kills_part,0,sizeof voice_kills_part);voice_kills=0;
    audio_worker_online=1;vceil=0xFFu;vceil_wait=0;
    for(unsigned p=0;p<NPART;p++){
        host_preset(&trk[p],HOST_SYNTH,HOST_PLAIN);
        trk[p].p[P_VOICE]=V_POLY;
    }
}

static void ticks(unsigned n,uint32_t load){while(n--)voice_ceiling_tick(load);}

int main(void)
{
    uint32_t full=voice_budget();
    fresh();assert(voice_budget()==full && vceil==0xFFu);  /* no shed yet: BUDGET alone */
    ticks(1000,0);assert(vceil==0xFFu);                     /* nothing to recover */

    /* a shed with 16 units sounding: the ceiling is 16; a new note replaces a voice, the bass stays */
    for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<2u;n++)trk_note_on(&trk[p],48u+4u*n,100);
    assert(voices_busy()==16u && !voice_kills);
    voice_ceiling_drop();
    assert(vceil==16u && voice_budget()==16u);
    trk_note_on(&trk[3],67,100);
    assert(voices_busy()<=16u && voice_kills);
    for(unsigned p=0;p<NPART;p++)assert(trk[p].v[lowest_held(&trk[p])].note==48u);

    /* held while the load is high, and a high half restarts the wait */
    ticks(500,VCEIL_LOW);assert(vceil==16u);
    ticks(VCEIL_HALVES-1u,0);assert(vceil==16u);
    ticks(1,VCEIL_LOW+50u);ticks(VCEIL_HALVES-1u,0);assert(vceil==16u);
    ticks(1,0);assert(vceil==17u && voice_budget()==17u);  /* one unit after ~250 ms under 60 % */
    while(vceil!=0xFFu)ticks(1,0);                          /* back to BUDGET: the ceiling lets go */
    assert(voice_budget()==full);

    /* BUDGET stays the maximum: a lower BUDGET wins over a higher ceiling */
    fresh();
    for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<2u;n++)trk_note_on(&trk[p],48u+4u*n,100);
    voice_ceiling_drop();assert(vceil==16u);
    { uint8_t keep=settings_budget; settings_budget=8; assert(voice_budget()==8u); settings_budget=keep; }

    /* the floor: nothing sounding still leaves 2 units, and each part's protected voice is admitted past it */
    fresh();voice_ceiling_drop();
    assert(vceil==VCEIL_MIN && voice_budget()==VCEIL_MIN);
    for(unsigned p=0;p<NPART;p++)trk_note_on(&trk[p],48u,100);
    for(unsigned p=0;p<NPART;p++)assert(trk[p].v[lowest_held(&trk[p])].note==48u && trk[p].v[lowest_held(&trk[p])].gate);

    /* a later shed lowers it again from what is sounding then */
    fresh();
    for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<2u;n++)trk_note_on(&trk[p],48u+4u*n,100);
    voice_ceiling_drop();assert(vceil==16u);
    trk_note_off(&trk[0],52u);trk_note_off(&trk[1],52u);
    for(unsigned b=0;b<64u;b++){int32_t o[2u*CTL];mix_block(o,CTL);}    /* the released tails ring out */
    voice_ceiling_drop();assert(vceil<16u && vceil==voices_busy());

    printf("Load ceiling: shed caps admission, replaces past it with the bass kept, holds under load, ~250 ms a unit back, BUDGET the maximum, floor 2: OK\n");
    return 0;
}
