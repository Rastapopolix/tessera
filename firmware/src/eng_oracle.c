/* SPDX-License-Identifier: GPL-3.0-only */
/* ORACLE: two oscillators, Poly-Mod and a four-pole ladder, after the Prophet-5 (Rev 1 to Rev 4). The engine is the
 * Prophet engine of Melodee 1.4 (eng_prophet_test.c, retired here 2026-10-22), back under its own name with its own
 * sounds: no factory bank, no SysEx, no user bank. The sound is the track's patch (or_patch: the panel's values,
 * 0..127, the Prophet's order), edited on ORACLE's pages (params.c, oracle_pages.c) and saved with the project
 * (project_tsp.c TSP_ORACLE).
 *
 * Signal path: oscillator A (saw, pulse; sync to B) and B (saw, triangle, pulse; keyed or fixed, low range) and
 * noise into a mixer, a 4-pole ladder (LATE: the Rev 3 model, linear stages; EARLY: Rev 1/2, each stage saturating,
 * its filter envelope's decay linear), the amp. The filter envelope and oscillator B modulate A's pitch, A's pulse
 * width and the cutoff (POLY MOD); the LFO, with noise mixed in, A's and B's pitch and width and the cutoff (LFO
 * ROUTE). VINTAGE spreads each voice's tuning, cutoff, level and envelope times as an old unit drifts.
 *
 * MACRO (EDIT 1 and 2: the track's P_E0..P_E7) offsets the patch: CUT, RES, TUNE A, TUNE B, PW A, PW B, LVL A,
 * LVL B. They are what cords, motion and the HOME knobs move. The engine's envelopes are the voice's amplitude
 * (ownenv): the track's ENV is not used. */
#include "x0x/fastmath.h"
enum {                           /* the patch: the Prophet's panel in its own order */
    OR_FREQ_A, OR_FREQ_B, OR_FINE_B, OR_SAW_A, OR_PULSE_A,
    OR_SAW_B, OR_TRI_B, OR_PULSE_B, OR_PW_A, OR_PW_B,
    OR_SYNC, OR_LOW_B, OR_KEY_B, OR_GLIDE, OR_LEVEL_A, OR_LEVEL_B,
    OR_NOISE, OR_CUTOFF, OR_RESONANCE, OR_KEY_FILTER, OR_FILTER_REV,
    OR_LFO_RATE, OR_LFO_INITIAL, OR_LFO_SAW, OR_LFO_TRI, OR_LFO_PULSE,
    OR_WHEEL_MIX, OR_WHEEL_FREQ_A, OR_WHEEL_FREQ_B, OR_WHEEL_PW_A,
    OR_WHEEL_PW_B, OR_WHEEL_FILTER, OR_POLY_ENV, OR_POLY_B,
    OR_POLY_FREQ, OR_POLY_PW, OR_POLY_FILTER, OR_VINTAGE,
    OR_PRESS_FILTER, OR_PRESS_LFO, OR_ENV_FILTER, OR_VEL_FILTER,
    OR_VEL_AMP, OR_ATTACK_FILTER, OR_ATTACK_AMP, OR_DECAY_FILTER,
    OR_DECAY_AMP, OR_SUSTAIN_FILTER, OR_SUSTAIN_AMP, OR_RELEASE_FILTER,
    OR_RELEASE_AMP, OR_RELEASE_ON,
    OR_N                         /* 52 bytes a track */
};
#if MELODEE_DUAL_CORE
#define OR_POLY 8u
#else
#define OR_POLY 5u
#endif
static uint32_t or_poly(void)
{
#if MELODEE_DUAL_CORE
    if (audio_worker_online) return OR_POLY;
#endif
    return 5u;
}
typedef struct { float value; uint8_t stage; } or_env_t;
typedef struct {
    uint32_t phase[2];
    float z[4], triangle, sync_tail;
    int32_t noise;
    int32_t drift, filter_drift, amp_drift, env_drift;
    float filter_prev, amp_prev;
    int32_t glide_pitch, last_pcm;
    float coeff_cut, coeff_k, coeff_g[4], coeff_inv;
    uint32_t coeff_valid;
    or_env_t filter, amp;
} or_voice_t;
typedef struct {                 /* the part's: from the patch and MACRO, once a block */
    float cut, k, filter_gain, env_amount;
    float pa_env, pa_b, pf_env, pf_b, pw_env, pw_b, pwm_a, pwm_b, la, lb, ln;
} or_base_t;
typedef struct {                 /* a voice's block: its envelopes' ends, cutoff, velocities, pitches */
    float f0, a0, f1, a1, cut, filter_velocity, amp_velocity, width;
    uint32_t ia, ib;
    int32_t amp0, amp1;
} or_job_t;
typedef struct {
    or_voice_t voice[OR_POLY];
    uint32_t lfo;
    int32_t last_pitch, mod_noise, dc_x1, dc_y1;
    float wheel[CTL], wheel_pitch[CTL];
    uint8_t value[OR_N], input_key[OR_N], base_valid;
    int16_t base_e[8];
    int32_t coarse_a, coarse_b, fixed_b, fine_b;
    or_base_t base;
} or_part_t;
static or_part_t *or_part(uint32_t part);
static uint8_t or_patch[NTRK][OR_N];             /* the tracks' patches (main loop writes; the ISR reads) */
static uint8_t or_ready[NTRK];
static void or_patch_init(uint8_t *p)            /* INIT: A's saw open, the amp and filter envelopes plain */
{
    memset(p, 0, OR_N);
    p[OR_FREQ_A] = p[OR_FREQ_B] = 24;            /* (24: the keyed oscillators' 0 semitones) */
    p[OR_SAW_A] = p[OR_KEY_B] = 1;
    p[OR_PW_A] = p[OR_PW_B] = 64;
    p[OR_LEVEL_A] = p[OR_CUTOFF] = 127;
    p[OR_FILTER_REV] = 1;                        /* 1: LATE (Rev 3's ladder); 0: EARLY (Rev 1/2) */
    p[OR_DECAY_FILTER] = p[OR_DECAY_AMP] = 60;
    p[OR_SUSTAIN_FILTER] = p[OR_SUSTAIN_AMP] = 127;
    p[OR_RELEASE_FILTER] = p[OR_RELEASE_AMP] = 40;
    p[OR_RELEASE_ON] = 1;
    p[OR_LFO_RATE] = 60;
    p[OR_LFO_TRI] = 1;
}
static uint8_t *or_patch_of(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk) % NTRK;
    if (!or_ready[k]) { or_patch_init(or_patch[k]); or_ready[k] = 1; }
    return or_patch[k];
}
static inline __attribute__((always_inline)) or_voice_t *or_voice(track_t *t, voice_t *v)
{
    or_part_t *part = or_part((uint32_t)(t - trk));
    return part ? &part->voice[(uint32_t)(v - t->v) % OR_POLY] : 0;
}
/* Integer phase counters retain low-frequency tuning precision. Everything
 * after phase conversion is normalized fp32, including BLEP and saturation. */
static inline __attribute__((always_inline)) int32_t or_fast_clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline __attribute__((always_inline)) float or_fast_blep(uint32_t ph, uint32_t inc)
{
    if (!inc) return 0.0f;
    if (ph < inc) {
        float x = (float)ph / (float)inc;
        return x + x - x * x - 1.0f;
    }
    if (ph > 0xFFFFFFFFu - inc) {
        float x = -(float)(0xFFFFFFFFu - ph) / (float)inc;
        return x * x + x + x + 1.0f;
    }
    return 0.0f;
}
static inline __attribute__((always_inline)) float or_fast_osc_saw(uint32_t ph, uint32_t inc)
{
    return (float)ph * (1.0f / 2147483648.0f) - 1.0f - or_fast_blep(ph, inc);
}
/* A float table keeps the seven saturation evaluations inexpensive (DSP_TANH, gen_tables.py).
 * Linear interpolation has <6e-6 error and no integer amplitude steps. */
static inline __attribute__((always_inline)) float or_fast_softclip(float x)
{
    float a=fm_fabsf(x)*128.0f;
    if(a>=1024.0f)return x<0.0f?-1.0f:1.0f;
    uint32_t i=(uint32_t)a;
    float y=DSP_TANH[i]+(DSP_TANH[i+1u]-DSP_TANH[i])*(a-(float)i);
    return x<0.0f?-y:y;
}

static inline __attribute__((always_inline)) uint32_t or_fast_noise32(int32_t *st)
{
    uint32_t s = (uint32_t)*st;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    *st = (int32_t)s;
    return s;
}
static int32_t or_value(const uint8_t *p,uint32_t k) { return p[k]>127u?127:p[k]; }
#define OR_ENV_END (1.0f / 1024.0f)   /* about -60 dB */
/* Linear attack, exponential decay/release; EARLY's filter envelope uses
 * its measured linear decay. Float avoids an integer floor in quiet tails. */
static float or_env_tick(or_env_t *e, int gate, const uint8_t *p,
                         uint32_t a, uint32_t d, uint32_t s, uint32_t r, int32_t variation, const uint8_t *value, int ssm)
{
    if (!gate && e->stage != 0u) e->stage = 3;
    float target = (float)value[s] * (1.0f / 127.0f);
    float scale = 1.0f + (float)variation * (1.0f / 32768.0f);
    uint32_t rel = p[OR_RELEASE_ON] ? value[r] : 16u;
    if (e->stage == 1u) {
        e->value += OR_ENV_ATK[value[a]] * scale;
        if (e->value >= 1.0f) { e->value = 1.0f; e->stage = 2; }
    } else if (ssm && e->stage >= 2u) {
        float step = OR_ENV_SSM[e->stage == 2u ? value[d] : rel] * scale;
        if (e->stage == 3u) target = 0.0f;
        e->value += fm_clampf(target - e->value, -step, step);
    } else if (e->stage == 2u) {
        e->value += (target - e->value) * fm_minf(OR_ENV_EXP[value[d]] * scale, 1.0f);
    } else if (e->stage == 3u) {
        e->value -= e->value * fm_minf(OR_ENV_EXP[rel] * scale, 1.0f);
    }
    if (e->stage == 3u && e->value < OR_ENV_END) { e->value = 0.0f; e->stage = 0; }
    return e->value;
}

static void or_note_on(track_t *t, voice_t *v)
{
    or_voice_t *s = or_voice(t, v);
    const uint8_t *p = or_patch_of(t);
    or_part_t *part = or_part((uint32_t)(t - trk));
    if (!s || !part) return;
    if (!voice_was) {
        memset(s, 0, sizeof *s);
        s->phase[0]=v->ph[0];s->phase[1]=v->ph[1];
        s->triangle = 1.0f;
        /* Thermal excitation lets resonance start with the oscillator mixer
         * shut, as self-oscillating sounds require. */
        if(p[OR_RESONANCE]>=110u && !p[OR_LEVEL_A] && !p[OR_LEVEL_B] && !p[OR_NOISE]){
            s->z[0]=0.5f;s->z[2]=-0.5f;
        }
        s->noise = (int32_t)(0x5A1793u + v->age * 17u + (uint32_t)(v-t->v) * 101u);
        s->glide_pitch=part->last_pitch ? part->last_pitch : v->pitch16;
    }
    part->last_pitch=v->pitch16;
    s->filter.stage = s->amp.stage = 1;
    /* VINTAGE 0 is a stable Rev 4, 127 a loose Rev 1 */
    int32_t vintage = or_value(p, OR_VINTAGE);
    /* Fixed mismatch for the life of the voice, deterministic per note/slot. */
    s->drift = ((int32_t)(or_fast_noise32(&s->noise) & 255u) - 128) * vintage / 1024;
    s->filter_drift = ((int32_t)(or_fast_noise32(&s->noise) & 255u) - 128) * vintage / 32;
    s->amp_drift = ((int32_t)(or_fast_noise32(&s->noise) & 255u) - 128) * vintage / 16;
    s->env_drift = ((int32_t)(or_fast_noise32(&s->noise) & 255u) - 128) * vintage / 128;
    /* The oscillators free-run, so a key finds a low B anywhere in its cycle (a fresh voice at the bottom of the
     * ramp held a Poly-Mod B filter shut on every new note) */
    if (!voice_was && p[OR_LOW_B]) s->phase[1] = or_fast_noise32(&s->noise);
}
static int or_done(track_t *t, voice_t *v)
{
    or_voice_t *s = or_voice(t, v);
    return !s || (!v->gate && s->amp.stage == 0u);
}
/* The native modulation sources: triangle +-0.5, saw/pulse 0..1. */
static inline __attribute__((always_inline)) float or_lfo_wave(uint32_t phase,const uint8_t *p)
{
    float pos=(float)phase*(1.0f/4294967296.0f), y=0.0f;uint32_t n=0;
    if(p[OR_LFO_SAW]){y+=pos;n++;}
    if(p[OR_LFO_TRI]){y+=phase<0x80000000u?pos*2.0f-0.5f:1.5f-pos*2.0f;n++;}
    if(p[OR_LFO_PULSE]){y+=phase<0x80000000u?1.0f:0.0f;n++;}
    return n==1u?y:n==2u?y*0.5f:n==3u?y/3.0f:0.0f;
}
/* Float interpolation of one octave, with an exact power-of-two scale.
 * Callers constrain x to [-8,6]; interpolation error is below 1 ppm. */
static inline __attribute__((always_inline)) float or_pitch_ratio(float x)
{
    int32_t octave=(int32_t)x;if((float)octave>x)octave--;
    float pos=(x-(float)octave)*512.0f;uint32_t i=(uint32_t)pos;
    float ratio=OR_EXP2[i]+(OR_EXP2[i+1u]-OR_EXP2[i])*(pos-(float)i);
    fm_bits_t scale;scale.u=(uint32_t)(octave+127)<<23;
    return ratio*scale.f;
}
/* Rev-4 full-amount triangle swings +-235 cents. Shared by every voice. */
static __attribute__((section(".dsp_text"))) void or_mod_samples(or_part_t *s,const uint8_t *p,uint32_t inc,float amount,float mix)
{
    uint32_t phase=s->lfo;if(!s->mod_noise)s->mod_noise=0x517953;
    for(uint32_t i=0;i<CTL;i++){
        float noise=(float)(int32_t)(or_fast_noise32(&s->mod_noise)>>8)*(1.0f/16777216.0f)-0.5f;
        float w=(or_lfo_wave(phase,p)*(1.0f-mix)+noise*mix)*amount;
        s->wheel[i]=w;
        s->wheel_pitch[i]=or_pitch_ratio(w*(470.0f/1200.0f));
        phase+=inc;
    }
    s->lfo=phase;
}
static __attribute__((noinline)) void or_cache_update(track_t *t,or_part_t *s,const uint8_t *p)
{
    if(s->base_valid && !memcmp(s->input_key,p,OR_N) && !memcmp(s->base_e,t->p+P_E0,sizeof s->base_e))return;
    memcpy(s->input_key,p,OR_N);
    for(uint32_t k=0;k<OR_N;k++)s->value[k]=(uint8_t)or_value(p,k);
    memcpy(s->base_e,t->p+P_E0,sizeof s->base_e);s->base_valid=1;
    const uint8_t *q=s->value;int curtis=!!p[OR_FILTER_REV];or_base_t *c=&s->base;
    s->coarse_a=(clamp(q[OR_FREQ_A],0,96)/2-12)*16+t->p[P_E2]*16;
    s->coarse_b=(clamp(q[OR_FREQ_B],0,96)/2-12)*16+t->p[P_E3]*16;
    s->fixed_b=12*16+q[OR_FREQ_B]*108*16/127+t->p[P_E3]*16;
    s->fine_b=q[OR_FINE_B]*100/128;
    int32_t cut=q[OR_CUTOFF],res=clamp(q[OR_RESONANCE]+t->p[P_E1],0,127);
    c->cut=(float)(1618+cut*236+(curtis?0:(127-cut>66?66:127-cut)*38)+t->p[P_E0]*256)/256.0f;
    c->k=(float)(res*(curtis?19200:18800)/127)/4096.0f;
    c->filter_gain=(float)(32767-res*(curtis?100:45))/32768.0f;
    c->env_amount=(float)(q[OR_ENV_FILTER]*165)/256.0f;
    c->pa_env=(float)(q[OR_POLY_ENV]*85)/4096.0f;c->pa_b=(float)(q[OR_POLY_B]*35)/4096.0f;
    c->pf_env=(float)(q[OR_POLY_ENV]*226)/256.0f;c->pf_b=(float)(q[OR_POLY_B]*226)/256.0f;
    c->pw_env=(float)(q[OR_POLY_ENV]*64)/32768.0f;c->pw_b=(float)(q[OR_POLY_B]*258)/32768.0f;
    c->pwm_a=(float)(q[OR_PW_A]*258+t->p[P_E4]*256)/32768.0f;
    c->pwm_b=(float)(q[OR_PW_B]*258+t->p[P_E5]*256)/32768.0f;
    c->la=(float)(clamp(q[OR_LEVEL_A]+t->p[P_E6],0,127)*258)/32768.0f;
    c->lb=(float)(clamp(q[OR_LEVEL_B]+t->p[P_E7],0,127)*258)/32768.0f;c->ln=(float)(q[OR_NOISE]*258)/32768.0f;
}
static void or_block(track_t *t)
{
    or_part_t *s=or_part((uint32_t)(t-trk));const uint8_t *p=or_patch_of(t);
    if(!s)return;
    /* Raw-to-DSP clamping is part-wide; envelopes and all voices share it. */
    or_cache_update(t,s,p);
    uint32_t inc=OR_LFO_INC[or_value(p,OR_LFO_RATE)];
    uint32_t live=0;for(uint32_t i=0;i<NVOICE;i++)live|=t->v[i].active;
    if(!live){
        /* Keep the LFO clock and noise sequence exact while skipping
         * stateless modulation buffers that no voice can read. */
        if(!s->mod_noise)s->mod_noise=0x517953;
        for(uint32_t i=0;i<CTL;i++)or_fast_noise32(&s->mod_noise);
        s->lfo+=inc*CTL;
        return;
    }
    float amount=OR_AMOUNT[clamp(or_value(p,OR_LFO_INITIAL)+t->mw+(p[OR_PRESS_LFO]?t->at:0),0,127)];
    void (*volatile run)(or_part_t *,const uint8_t *,uint32_t,float,float)=or_mod_samples;
    run(s,p,inc,amount,(float)or_value(p,OR_WHEEL_MIX)*(1.0f/127.0f));
}
static uint32_t or_cap(const track_t *t) { (void)t; return or_poly(); }

/* Four trapezoidal one-poles with an algebraic feedback solution. LATE
 * saturates the input; EARLY saturates each stage. Coefficients, feedback and stage memory are all fp32. */
static inline __attribute__((always_inline)) float or_filter_core(or_voice_t *s, float input, float cutoff, float k, float gain, int curtis)
{
    cutoff=fm_clampf(cutoff,0.0f,127.0f);
    float g,g2,g3,g4,reciprocal;
    if(s->coeff_valid && cutoff==s->coeff_cut && k==s->coeff_k){
        g=s->coeff_g[0];g2=s->coeff_g[1];g3=s->coeff_g[2];g4=s->coeff_g[3];reciprocal=s->coeff_inv;
    }else{
        uint32_t i=(uint32_t)cutoff;g=OR_TPT_G[i];
        if(i<127u)g+=(OR_TPT_G[i+1u]-g)*(cutoff-(float)i);
        g2=g*g;g3=g2*g;g4=g3*g;
        reciprocal=1.0f/(1.0f+k*g4);
        s->coeff_g[0]=g;s->coeff_g[1]=g2;s->coeff_g[2]=g3;s->coeff_g[3]=g4;
        s->coeff_cut=cutoff;s->coeff_k=k;s->coeff_inv=reciprocal;s->coeff_valid=1;
    }
    float sigma=(1.0f-g)*(g3*s->z[0]+g2*s->z[1]+g*s->z[2]+s->z[3]);
    float u=or_fast_softclip((input*gain-k*sigma)*reciprocal);
#define OR_STAGE(j, clip) do { \
    float delta=g*(u-s->z[j]); \
    float y=s->z[j]+delta; \
    s->z[j]=fm_clampf(y+delta,-1.83105469f,1.83105469f); \
    if(fm_fabsf(s->z[j])<1e-20f)s->z[j]=0.0f; \
    u=clip(y); \
} while(0)
#define OR_LINEAR(y) (y)
    if(curtis){OR_STAGE(0,OR_LINEAR);OR_STAGE(1,OR_LINEAR);OR_STAGE(2,OR_LINEAR);OR_STAGE(3,OR_LINEAR);}
    else {OR_STAGE(0,or_fast_softclip);OR_STAGE(1,or_fast_softclip);OR_STAGE(2,or_fast_softclip);OR_STAGE(3,or_fast_softclip);}
#undef OR_LINEAR
#undef OR_STAGE
    return or_fast_softclip(u);
}
static inline __attribute__((always_inline)) float or_wave(uint32_t phase, uint32_t inc, uint32_t pw, int saw, int pulse, int tri, float *triangle)
{
    if(!saw && !pulse && !tri)return 0.0f;
    float y=0.0f,a=or_fast_osc_saw(phase,inc),square=0.0f;
    if(saw)y+=a*0.5f;
    if(pulse){square=(a-or_fast_osc_saw(phase+pw,inc))*0.5f;y+=square;}
    if(tri){
        if(!pulse || pw!=0x80000000u)square=(a-or_fast_osc_saw(phase+0x80000000u,inc))*0.5f;
        *triangle=fm_clampf(*triangle*(32767.0f/32768.0f)+square*(float)inc*(8.0f/4294967296.0f),-1.220703125f,1.220703125f);
        y+=*triangle*0.5f;
    }
    return y;
}
static inline __attribute__((always_inline)) float or_sync_fraction(uint32_t distance,uint32_t inc)
{
    return inc ? fm_minf((float)distance/(float)inc,1.0f) : 0.0f;
}
static inline __attribute__((always_inline)) uint32_t or_pw(float value)
{
    return (uint32_t)(fm_clampf(value,0.03125f,31743.0f/32768.0f)*4294967296.0f);
}
/* Exponential Poly-Mod, x in octaves; cap below Nyquist before conversion. */
static inline __attribute__((always_inline)) uint32_t or_inc(uint32_t base,float x)
{
    if(!x)return base;
    float y=(float)base*or_pitch_ratio(fm_clampf(x,-8.0f,6.0f));
    return (uint32_t)fm_clampf(y,1.0f,1932735232.0f);
}
static inline __attribute__((always_inline)) uint32_t or_wheel_inc(uint32_t base,float ratio)
{
    if(ratio==1.0f)return base;
    return (uint32_t)fm_minf((float)base*ratio,1932735232.0f);
}
static __attribute__((section(".dsp_text"))) void or_samples(or_voice_t *restrict s,const uint8_t *restrict p,int32_t *restrict out,uint32_t n,
                                                             const or_part_t *restrict part,const or_job_t *restrict c)
{
    const or_base_t *b=&part->base;
    int32_t last=0;
    for(uint32_t i=0;i<n;i++){
        float time=(float)i*(1.0f/CTL);
        float fe=c->f0+(c->f1-c->f0)*time,ae=c->a0+(c->a1-c->a0)*time;
        float noise=(float)(or_fast_noise32(&s->noise)>>8)*(1.0f/16777216.0f)-0.5f;
        float lfo=part->wheel[i],wheel_pitch=part->wheel_pitch[i];
        uint32_t db=p[OR_WHEEL_FREQ_B]?or_wheel_inc(c->ib,wheel_pitch):c->ib;
        float bw=or_wave(s->phase[1],db,or_pw(b->pwm_b+c->width+(p[OR_WHEEL_PW_B]?lfo:0.0f)),p[OR_SAW_B],p[OR_PULSE_B],p[OR_TRI_B],&s->triangle);
        float bn=bw*2.0f;
        uint32_t da=p[OR_POLY_FREQ]?or_inc(c->ia,fe*b->pa_env+bn*b->pa_b):c->ia;
        if(p[OR_WHEEL_FREQ_A])da=or_wheel_inc(da,wheel_pitch);
        uint32_t pw=or_pw(b->pwm_a+c->width+(p[OR_POLY_PW]?fe*b->pw_env+bw*b->pw_b:0.0f)+(p[OR_WHEEL_PW_A]?lfo:0.0f));
        float unused=0.0f;
        float a=or_wave(s->phase[0],da,pw,p[OR_SAW_A],p[OR_PULSE_A],0,&unused)+s->sync_tail;
        s->sync_tail=0.0f;
        uint32_t nb=s->phase[1]+db,na=s->phase[0]+da;
        if(p[OR_SYNC] && nb<s->phase[1] && db){
            float frac=or_sync_fraction(0u-s->phase[1],db),remain=1.0f-frac;
            uint32_t at=s->phase[0]+(uint32_t)((float)da*frac);
            float before=or_wave(at,da,pw,p[OR_SAW_A],p[OR_PULSE_A],0,&unused);
            float after=or_wave(0,da,pw,p[OR_SAW_A],p[OR_PULSE_A],0,&unused);
            float jump=(after-before)*0.5f;
            a+=jump*remain*remain;s->sync_tail=-jump*frac*frac;
            na=(uint32_t)((float)da*remain);
        }
        s->phase[0]=na;s->phase[1]=nb;
        float x=a*b->la+bw*b->lb+noise*b->ln;
        float fc=c->cut+fe*c->filter_velocity*b->env_amount;
        if(p[OR_POLY_FILTER])fc+=fe*b->pf_env+bn*b->pf_b;
        if(p[OR_WHEEL_FILTER])fc+=lfo*112.0f;
        float y=or_filter_core(s,or_fast_softclip(x),fc,b->k,b->filter_gain,!!p[OR_FILTER_REV]);
        float gain=ae*c->amp_velocity*(1.0f+(float)s->amp_drift/32768.0f);
        float amp=((float)c->amp0+(float)(c->amp1-c->amp0)*time)*(1.0f/32768.0f);
        last=(int32_t)(y*gain*amp*(2.0f*VOICE_FS));out[i]+=last;
    }
    s->last_pcm=last;
}

typedef struct { or_voice_t *voice; const uint8_t *patch; const or_part_t *part; or_job_t job; uint32_t n; } or_batch_t;
_Static_assert(sizeof(or_batch_t)<=AUDIO_BATCH_BYTES,"ORACLE batch descriptor");
static void or_batch_kernel(const void *data,int32_t *pcm)
{
    const or_batch_t *j=data;
    void (*volatile run)(or_voice_t *,const uint8_t *,int32_t *,uint32_t,const or_part_t *,const or_job_t *)=or_samples;
    run(j->voice,j->patch,pcm,j->n,j->part,&j->job);
}
/* the part after its voices: the output's coupling capacitor, about 5 Hz (y = x - x' + (1 - 2 pi 5 / FS) y'). The
 * oscillators are unipolar, as the CEM3340's, so the mixer's DC rides through the filter and the VCA; without the
 * capacitor it reached the sends and the USB stems. With no voice it starts afresh */
#define OR_DC_K 1072976913                       /* (1 - 2 pi 5 / FS) << 30 */
_Static_assert(FS == 44100, "OR_DC_K: the sample rate");
static __attribute__((noinline,section(".dsp_text"))) void or_dc_samples(or_part_t *P,int32_t *out,uint32_t n)
{
    int32_t x1=P->dc_x1,y=P->dc_y1;
    for(uint32_t i=0;i<n;i++){
        int32_t x=out[i];
        y=(int32_t)((int64_t)(x-x1)*256+(((int64_t)y*OR_DC_K)>>30));
        x1=x;
        out[i]=(y+128)>>8;
    }
    P->dc_x1=x1;P->dc_y1=y;
}
static void or_post(track_t *t,int32_t *out,uint32_t n,uint32_t nr)
{
    or_part_t *P=or_part((uint32_t)(t-trk)%NPART);
    if(!P)return;
    if(!nr){P->dc_x1=P->dc_y1=0;return;}
    void (*volatile run)(or_part_t *,int32_t *,uint32_t)=or_dc_samples;
    run(P,out,n);
}

static void or_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    or_voice_t *s=or_voice(t,v);
    or_part_t *part=or_part((uint32_t)(t-trk)%NPART);
    const uint8_t *p=or_patch_of(t);
    if (!s || !part) return;
    /* A budget victim is already inactive when the common renderer delivers its final fade block: its last PCM
     * sample fades to zero instead of a whole extra voice beside the one that replaces it */
    if(!v->active && !m->amp1){
        if(n>1u){
            int32_t step=s->last_pcm/(int32_t)(n-1u);
            for(uint32_t i=0;i<n-1u;i++)out[i]+=s->last_pcm-step*(int32_t)i;
        }
        s->last_pcm=0;return;
    }
    if(!part->base_valid)or_cache_update(t,part,p);
    const uint8_t *q=part->value;
    or_job_t c;
    int curtis=!!p[OR_FILTER_REV];
    c.f0=s->filter_prev; c.a0=s->amp_prev;
    c.f1=or_env_tick(&s->filter,v->gate,p,OR_ATTACK_FILTER,OR_DECAY_FILTER,OR_SUSTAIN_FILTER,OR_RELEASE_FILTER,s->env_drift,q,!curtis);
    c.a1=or_env_tick(&s->amp,v->gate,p,OR_ATTACK_AMP,OR_DECAY_AMP,OR_SUSTAIN_AMP,OR_RELEASE_AMP,s->env_drift,q,0);
    s->filter_prev=c.f1; s->amp_prev=c.a1;
    int32_t glide=0;
    if(p[OR_GLIDE]){
        int32_t target=v->pitch16,d=target-s->glide_pitch;
        int32_t step=(int32_t)(ENV_LIN[q[OR_GLIDE]]>>14);if(step<1)step=1;
        s->glide_pitch+=clamp(d,-step,step);glide=s->glide_pitch-v->pitch_cur;
    }else s->glide_pitch=v->pitch16;
    /* Keyed coarse tuning has semitone detents (two raw values each: 24/25 are both 0). Its four octaves end at raw
     * 96. B's fixed (keyboard-off) range spans nine octaves continuously */
    int32_t pitch_a=m->pitch16+glide+part->coarse_a;
    int32_t pitch_b=p[OR_KEY_B]?m->pitch16+glide+part->coarse_b:part->fixed_b;
    c.ia = cents_inc(pitch_a, s->drift, m->fine);
    c.ib = cents_inc(pitch_b, part->fine_b-s->drift, m->fine);
    if (p[OR_LOW_B]) c.ib >>= 7;                 /* LOW B: seven octaves down, an LFO for Poly-Mod */
    c.filter_velocity=p[OR_VEL_FILTER] ? (float)(v->mvel*258)/32768.0f : 32767.0f/32768.0f;
    c.amp_velocity=p[OR_VEL_AMP] ? (float)(v->mvel*258)/32768.0f : 32767.0f/32768.0f;
    /* Cutoff in Q8 steps of the coefficient table (14.02 per octave, 30 Hz at 0). Fitted to Rev-4 recordings: LATE
     * puts the lowest C (MIDI 36) near 870 Hz * 2^((raw-67)/15.2); EARLY sits ~0.7 octave higher below the middle
     * of the knob, converging at the top. The track's FLT routes (LFO, ENV, cords) move it too */
    int32_t cut=m->cutoff+s->filter_drift;
    cut+=(m->pitch16-36*16)*or_fast_clamp(p[OR_KEY_FILTER],0,2)*OR_KEYTRACK_HALF_Q7/128;
    if(p[OR_PRESS_FILTER])cut+=t->at*96;
    c.cut=part->base.cut+(float)cut/256.0f;
    c.width=(float)(m->shape-(64<<8))/32768.0f;  /* (the track's SHP routes: both pulse widths) */
    c.amp0=m->amp0; c.amp1=m->amp1;
    if(audio_batch_active){
        const or_batch_t job={s,p,part,c,n};audio_batch_push(or_batch_kernel,&job,sizeof job,out,n,AP_ORACLE);return;
    }
    /* An indirect call crosses the XIP/RAM distance beyond direct branch range. */
    void (*volatile run)(or_voice_t *,const uint8_t *,int32_t *,uint32_t,const or_part_t *,const or_job_t *)=or_samples;
    run(s,p,out,n,part,&c);
}

/* Serial hardware measurements justify three units per voice; with CPU1 sharing the voices, two: eight voices fill
 * the same sixteen units */
static uint32_t or_units(const track_t *t) { (void)t; return or_poly()>5u?2u:3u; }

/* ---- ORACLE's own sounds: INIT and the factory set, each a few values over INIT (a pair: index, value) ---- */
typedef struct { uint8_t k, v; } or_set_t;
#define OR_SOUND_MAX 24u
typedef struct { or_set_t set[OR_SOUND_MAX]; } or_sound_t;
#include "oracle_sounds.h"       /* ORACLE_PRESETS (the names) and OR_SOUNDS (the values), in step */
_Static_assert(NELEM(ORACLE_PRESETS) == NELEM(OR_SOUNDS), "ORACLE: a name for every sound");
/* preset k's patch -> p */
static void or_sound(uint32_t k, uint8_t *p)
{
    or_patch_init(p);
    if (k >= NELEM(OR_SOUNDS)) return;
    for (uint32_t i = 0; i < OR_SOUND_MAX && OR_SOUNDS[k].set[i].k; i++)
        if (OR_SOUNDS[k].set[i].k - 1u < OR_N)
            p[OR_SOUNDS[k].set[i].k - 1u] = OR_SOUNDS[k].set[i].v;
}
/* a sound load (a preset, an engine change, a project's track of before ORACLE): the preset's patch */
static void or_track_loaded(const track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk);
    uint8_t p[OR_N];
    if (tr >= NTRK || t->eng_req != ENGI_ORACLE) return;
    or_sound(t->preset, p);
    memcpy(or_patch[tr], p, OR_N);
    or_ready[tr] = 1;
}
/* the largest value byte i of the patch takes: 1 a switch, 2 KEY (OFF HALF FULL), else 127 */
static uint32_t or_byte_max(uint32_t i)
{
    switch (i) {
    case OR_SAW_A: case OR_PULSE_A: case OR_SAW_B: case OR_TRI_B: case OR_PULSE_B: case OR_SYNC: case OR_LOW_B:
    case OR_KEY_B: case OR_FILTER_REV: case OR_LFO_SAW: case OR_LFO_TRI: case OR_LFO_PULSE: case OR_WHEEL_FREQ_A:
    case OR_WHEEL_FREQ_B: case OR_WHEEL_PW_A: case OR_WHEEL_PW_B: case OR_WHEEL_FILTER: case OR_POLY_FREQ:
    case OR_POLY_PW: case OR_POLY_FILTER: case OR_PRESS_FILTER: case OR_PRESS_LFO: case OR_VEL_FILTER: case OR_VEL_AMP:
    case OR_RELEASE_ON:
        return 1u;
    case OR_KEY_FILTER: return 2u;
    default: return i < OR_N ? 127u : 0u;
    }
}
/* a patch as it may be stored: every byte inside its range */
static int or_patch_ok(const uint8_t *p)
{
    for (uint32_t i = 0; i < OR_N; i++) if (p[i] > or_byte_max(i)) return 0;
    return 1;
}
/* the patch of track tr: the whole of it (a project, an undo); refused unless every value is in range */
static int or_set_patch(uint32_t tr, const uint8_t *p)
{
    if (tr >= NTRK || !or_patch_ok(p)) return 0;
    memcpy(or_patch[tr], p, OR_N);
    or_ready[tr] = 1;
    return 1;
}

static const engine_t ENG_ORACLE = {
    .name = "ORACLE", .page_title = {"MACRO", "MACRO 2"},
    .edit = {{"CUT", F_INT, -63, 63, 0, 0, 0}, {"RES", F_INT, -63, 63, 0, 0, 0},
             {"TUNE A", F_SEMI, -24, 24, 0, 0, 0}, {"TUNE B", F_SEMI, -24, 24, 0, 0, 0},
             {"PW A", F_INT, -63, 63, 0, 0, 0}, {"PW B", F_INT, -63, 63, 0, 0, 0},
             {"LVL A", F_INT, -63, 63, 0, 0, 0}, {"LVL B", F_INT, -63, 63, 0, 0, 0}},
    .presets = ORACLE_PRESETS, .npresets = (uint8_t)NELEM(ORACLE_PRESETS), .poly = OR_POLY, .ownenv = 1,
    .note_on = or_note_on, .render = or_render, .done = or_done, .block = or_block, .units = or_units, .cap = or_cap,
    .post = or_post,
    .knob = {P_E0, P_E1, P_E3, P_E7}
};
