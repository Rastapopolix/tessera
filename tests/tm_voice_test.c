/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI's notes out (midi_out.c): VOICE MONO sends one note at a time, PRIO LAST / LOW / HIGH picks it, the old
 * note's off before the new on; a released note hands over to one still held; POLY sends every note. Every on gets
 * its off. Built as Tessera MIDI by tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>
typedef struct { uint32_t at; uint8_t st, d1, d2; } ev_t;
static ev_t got[1024];
static uint32_t ngot, clock_s;
static void drain(void)
{
    for (; mo_r != mo_w; mo_r++) {
        uint32_t e = midi_out_q[mo_r % MQ];
        if (ngot < 1024u) { got[ngot].at = clock_s; got[ngot].st = (uint8_t)(e >> 8); got[ngot].d1 = (uint8_t)(e >> 16);
                            got[ngot].d2 = (uint8_t)(e >> 24); ngot++; }
    }
}
static void run(uint32_t samples) { while (samples) { uint32_t n = samples < 32u ? samples : 32u; mfx_tick(n); clock_s += n; drain(); samples -= n; } }
static int is_on(const ev_t *e) { return (e->st & 0xF0u) == 0x90u && e->d2; }
static int is_off(const ev_t *e) { return (e->st & 0xF0u) == 0x80u || ((e->st & 0xF0u) == 0x90u && !e->d2); }
static int balanced(void)                         /* every on has a later off, nothing left on */
{
    int on[128] = {0};
    for (uint32_t i = 0; i < ngot; i++) {
        if (is_on(&got[i])) on[got[i].d1]++;
        else if (is_off(&got[i]) && on[got[i].d1]) on[got[i].d1]--;
    }
    for (int n = 0; n < 128; n++) if (on[n]) return 0;
    return 1;
}
static uint32_t max_sounding(void)                /* the most notes on at once */
{
    int on[128] = {0}, n = 0, m = 0;
    for (uint32_t i = 0; i < ngot; i++) {
        if (is_on(&got[i])) { on[got[i].d1]++; n++; }
        else if (is_off(&got[i]) && on[got[i].d1]) { on[got[i].d1]--; n--; }
        if (n > m) m = n;
    }
    return (uint32_t)m;
}
static uint32_t ons(uint8_t *list, uint32_t max)  /* the notes started, in order */
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < ngot; i++) if (is_on(&got[i]) && c < max) list[c++] = got[i].d1;
    return c;
}
static int mfails;
static void mcheck(const char *what, int ok) { printf("%-66s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }
static void chord(track_t *t, uint32_t vel)       /* C E G, all in one block (a CHORD step) */
{
    midi_out_note(t, 60, vel); midi_out_note(t, 64, vel); midi_out_note(t, 67, vel); drain(); run(32);
}
static void fresh(track_t *t, uint32_t voice, uint32_t prio)
{
    midi_out_flush(0); drain(); run(64); ngot = 0;
    t->p[P_VOICE] = (int32_t)voice; t->p[P_PRIO] = (int32_t)prio;
}
int main(void)
{
    track_t *t = &trk[0];
    uint8_t l[16];
    uint32_t c;
    ui_power_on();
    usb.config = 1;
    settings_midi_out = MIDI_OUT_ALL;
    drain(); ngot = 0;

    fresh(t, V_POLY, 0);
    chord(t, 100); chord(t, 0);
    c = ons(l, 16);
    mcheck("POLY: a chord's three notes out", c == 3u && max_sounding() == 3u && balanced());

    fresh(t, V_MONO, 0);
    chord(t, 100);
    c = ons(l, 16);
    mcheck("MONO LAST: a chord is one note at a time", max_sounding() == 1u);
    mcheck("  the last one (67) is the one left sounding", c >= 1u && l[c - 1u] == 67);
    chord(t, 0);
    mcheck("  every note ends", balanced());

    fresh(t, V_MONO, 1);
    chord(t, 100);
    c = ons(l, 16);
    mcheck("MONO LOW: only 60 goes out", c == 1u && l[0] == 60);
    chord(t, 0);
    mcheck("  every note ends", balanced());

    fresh(t, V_MONO, 2);
    chord(t, 100);
    c = ons(l, 16);
    mcheck("MONO HIGH: 60, 64, 67, one at a time", c == 3u && l[2] == 67 && max_sounding() == 1u);
    chord(t, 0);
    mcheck("  every note ends", balanced());

    /* the old note's off before the new on */
    fresh(t, V_MONO, 0);
    midi_out_note(t, 60, 100); drain(); run(441);
    midi_out_note(t, 62, 100); drain(); run(441);
    {
        uint32_t i, off60 = ~0u, on62 = ~0u;
        for (i = 0; i < ngot; i++) {
            if (is_off(&got[i]) && got[i].d1 == 60 && off60 == ~0u) off60 = i;
            if (is_on(&got[i]) && got[i].d1 == 62 && on62 == ~0u) on62 = i;
        }
        mcheck("MONO: 60's off before 62's on", off60 < on62 && on62 != ~0u);
    }
    /* releasing the note sounding hands over to the one still held */
    ngot = 0;
    midi_out_note(t, 62, 0); drain(); run(441);
    c = ons(l, 16);
    mcheck("  62 released, 60 still held: 60 again", c == 1u && l[0] == 60);
    midi_out_note(t, 60, 0); drain(); run(441);
    mcheck("  every note ends", balanced());
    /* a note released that was not sounding sends nothing */
    fresh(t, V_MONO, 1);
    midi_out_note(t, 60, 100); midi_out_note(t, 72, 100); drain(); run(441);
    c = ngot;
    midi_out_note(t, 72, 0); drain(); run(441);
    mcheck("MONO LOW: releasing 72 (not sounding) sends nothing", ngot == c);
    midi_out_note(t, 60, 0); drain(); run(441);
    mcheck("  every note ends", balanced());
    /* STOP mid-chord in MONO */
    fresh(t, V_MONO, 0);
    chord(t, 100);
    midi_out_stop(); drain(); run(441);
    chord(t, 0);
    mcheck("MONO: STOP mid-chord, every note ends", balanced());
    /* LEGATO: the new note's on before the old note's off (an overlap of about a millisecond), also with HUMAN */
    for (uint32_t h = 0; h <= 100u; h += 100u) {
        uint32_t i, k, ok = 1, n = 0;
        fresh(t, V_LEGATO, 0);
        t->p[P_MHUMAN] = (int32_t)h;
        for (k = 0; k < 8u; k++) { midi_out_note(t, 60u + k, 100); drain(); run(4410); }   /* each held on, a line up */
        for (k = 0; k < 8u; k++) midi_out_note(t, 60u + k, 0);
        drain(); run(4410);
        for (k = 1; k < 8u; k++) {
            uint32_t on_at = ~0u, off_at = ~0u;
            for (i = 0; i < ngot; i++) {
                if (is_on(&got[i]) && got[i].d1 == 60u + k && on_at == ~0u) on_at = i;
                if (is_off(&got[i]) && got[i].d1 == 60u + k - 1u && off_at == ~0u) off_at = i;
            }
            ok &= on_at < off_at && off_at != ~0u;
            n += on_at < off_at && got[off_at].at - got[on_at].at <= 64u + 32u;
        }
        mcheck(h ? "LEGATO, HUMAN 100: still each on before the old note's off" : "LEGATO: each new on before the old note's off", ok);
        mcheck("  never more than two notes on, every note ends", max_sounding() == 2u && balanced());
        if (!h) mcheck("  the overlap about a millisecond", n == 7u);
    }
    t->p[P_MHUMAN] = 0;
    /* the retrigger gap: a note started again right after its off (or while it sounds) waits ~1.5 ms after the off */
    for (uint32_t v = V_POLY; v <= V_MONO; v++) {
        uint32_t i, off_at = ~0u, on_at = ~0u;
        fresh(t, v, 0);
        midi_out_note(t, 60, 100); drain(); run(4410);
        ngot = 0;
        midi_out_note(t, 60, 0); midi_out_note(t, 60, 100); drain(); run(4410);   /* a repeated step: off, on, one block */
        for (i = 0; i < ngot; i++) {
            if (is_off(&got[i]) && got[i].d1 == 60 && off_at == ~0u) off_at = i;
            if (is_on(&got[i]) && got[i].d1 == 60 && on_at == ~0u) on_at = i;
        }
        mcheck(v ? "MONO: a repeated note: off, then on 1.5 ms later" : "POLY: a repeated note: off, then on 1.5 ms later",
               off_at < on_at && on_at != ~0u && got[on_at].at - got[off_at].at >= 64u && got[on_at].at - got[off_at].at <= 64u + 64u);
        ngot = 0;
        midi_out_note(t, 60, 100); drain(); run(4410);                             /* on again while it sounds (a tie) */
        off_at = on_at = ~0u;
        for (i = 0; i < ngot; i++) {
            if (is_off(&got[i]) && got[i].d1 == 60 && off_at == ~0u) off_at = i;
            if (is_on(&got[i]) && got[i].d1 == 60 && on_at == ~0u) on_at = i;
        }
        mcheck("  on again while sounding: off, then on 1.5 ms later",
               off_at < on_at && on_at != ~0u && got[on_at].at - got[off_at].at >= 64u);
        midi_out_note(t, 60, 0); drain(); run(4410);
        mcheck("  every note ends", balanced());
    }
    /* a different note in the same block is not held back */
    fresh(t, V_POLY, 0);
    midi_out_note(t, 60, 100); drain(); run(4410);
    ngot = 0;
    midi_out_note(t, 60, 0); midi_out_note(t, 62, 100); drain();
    mcheck("POLY: a new note after another's off goes at once", ngot == 2u && is_on(&got[1]) && got[1].d1 == 62);
    midi_out_note(t, 62, 0); drain(); run(4410);
    /* POLY again after MONO: every note */
    fresh(t, V_POLY, 0);
    chord(t, 100); chord(t, 0);
    mcheck("POLY after MONO: three notes again", ons(l, 16) == 3u && balanced());

    printf("%s\n", mfails ? "FAILED" : "all ok");
    return mfails != 0;
}
