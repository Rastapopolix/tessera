/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI's MIDI FX (midi_out.c mfx_*): ECHO repeats fading and moving, STRUM spreads a chord UP / DOWN, VEL fixes
 * the velocity, HUMAN keeps within its limits; every on gets its off, after it; a purge (STOP) leaves nothing hanging.
 * Built as Tessera MIDI by tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>
typedef struct { uint32_t at; uint8_t st, note, vel; } ev_t;
static ev_t got[512];
static uint32_t ngot, clock_s;
static void drain(void)
{
    for (; mo_r != mo_w; mo_r++) {
        uint32_t e = midi_out_q[mo_r % MQ];
        if (ngot < 512u) { got[ngot].at = clock_s; got[ngot].st = (uint8_t)(e >> 8); got[ngot].note = (uint8_t)(e >> 16);
                           got[ngot].vel = (uint8_t)(e >> 24); ngot++; }
    }
}
static void run(uint32_t samples) { while (samples) { uint32_t n = samples < 64u ? samples : 64u; mfx_tick(n); clock_s += n; drain(); samples -= n; } }
static int balanced(void)                         /* every on has a later off, nothing left on */
{
    int on[128] = {0};
    for (uint32_t i = 0; i < ngot; i++) {
        if ((got[i].st & 0xF0u) == 0x90u && got[i].vel) on[got[i].note]++;
        else if (on[got[i].note]) on[got[i].note]--;
    }
    for (int n = 0; n < 128; n++) if (on[n]) return 0;
    return 1;
}
static int mfails;
static void mcheck(const char *what, int ok) { printf("%-60s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }
int main(void)
{
    track_t *t = &trk[0];
    uint32_t i, ons;
    ui_power_on();
    usb.config = 1;
    settings_midi_out = MIDI_OUT_ALL;
    drain(); ngot = 0;
    /* ECHO 3, FADE 50, PITCH +12, TIME 1/8 */
    t->p[P_MECHO] = 3; t->p[P_MEFADE] = 50; t->p[P_MEPIT] = 12; t->p[P_METIME] = 5;
    midi_out_note(t, 60, 100); drain();
    run(seq_beat_samples() / 4u);
    midi_out_note(t, 60, 0); drain();
    run(seq_beat_samples() * 3u);
    for (i = ons = 0; i < ngot; i++) ons += (got[i].st & 0xF0u) == 0x90u && got[i].vel;
    mcheck("ECHO 3: four notes", ons == 4u);
    mcheck("each repeat an octave up, half as loud", got[0].note == 60 && got[0].vel == 100 && ngot >= 4u);
    {
        int okp = 1; uint32_t seen = 0;
        for (i = 0; i < ngot; i++) if ((got[i].st & 0xF0u) == 0x90u && got[i].vel) {
            if (got[i].note != 60u + 12u * seen || got[i].vel != (uint8_t)(100 >> seen)) okp = 0;
            if (seen && got[i].at < seq_beat_samples() / 2u * seen - 64u) okp = 0;
            seen++;
        }
        mcheck("repeats: +12 a time, 50 % each, an eighth apart", okp);
    }
    mcheck("ECHO: every note ends", balanced());
    /* TIME in ms (8.. : 10 .. 500 ms), the same at any tempo */
    {
        static const uint16_t MS[12] = {10, 20, 30, 40, 50, 75, 100, 150, 200, 300, 400, 500};
        int okt = 1;
        for (uint32_t ti = 8; ti < 20u; ti++)
            for (uint32_t bpm = 60; bpm <= 180u; bpm += 120u) {
                uint32_t o[3], c = 0, want = MS[ti - 8u] * 441u / 10u;
                song.g[G_BPM] = (int16_t)bpm;
                ngot = 0; clock_s = 0;
                t->p[P_MECHO] = 2; t->p[P_MEFADE] = 0; t->p[P_MEPIT] = 0; t->p[P_METIME] = (int32_t)ti;
                midi_out_note(t, 60, 100); drain(); run(64);
                midi_out_note(t, 60, 0); drain(); run(2u * 44100u);
                for (i = 0; i < ngot && c < 3u; i++) if ((got[i].st & 0xF0u) == 0x90u && got[i].vel) o[c++] = i;
                if (c != 3u || got[o[1]].at - got[o[0]].at + 64u < want || got[o[1]].at - got[o[0]].at > want + 64u
                    || got[o[2]].at - got[o[1]].at + 64u < want || got[o[2]].at - got[o[1]].at > want + 64u || !balanced())
                    okt = 0;
            }
        mcheck("ECHO TIME 10 .. 500 ms: repeats that far apart at 60 and 180 BPM", okt);
        song.g[G_BPM] = 120;
    }
    t->p[P_MECHO] = 0; t->p[P_MEPIT] = 0;
    /* STRUM 40 (GLIDE's curve: ~18 ms) DOWN, VEL 90 */
    ngot = 0; clock_s = 0;
    t->p[P_MSTRUM] = 40; t->p[P_MSDIR] = 1; t->p[P_MVEL] = 90;
    midi_out_note(t, 60, 50); midi_out_note(t, 64, 50); midi_out_note(t, 67, 50); drain();
    run(44100u / 5u);
    {
        uint32_t o[3], c = 0;
        for (i = 0; i < ngot && c < 3u; i++) if ((got[i].st & 0xF0u) == 0x90u && got[i].vel) o[c++] = i;
        mcheck("STRUM DOWN: 67, 64, 60", c == 3u && got[o[0]].note == 67 && got[o[1]].note == 64 && got[o[2]].note == 60);
        uint32_t gap = time_ms_x10(40) * 441u / 100u;
        mcheck("STRUM 40: ~18 ms apart (+- a block)", c == 3u && got[o[1]].at - got[o[0]].at + 64u >= gap && got[o[1]].at - got[o[0]].at <= gap + 64u);
        mcheck("VEL 90 on every note", c == 3u && got[o[0]].vel == 90 && got[o[1]].vel == 90 && got[o[2]].vel == 90);
    }
    midi_out_note(t, 60, 0); midi_out_note(t, 64, 0); midi_out_note(t, 67, 0); drain();
    run(44100u / 5u);
    mcheck("STRUM: every note ends", balanced());
    /* the widest STRUM (~440 ms) over eight notes, released at once: each off after its own on, all over 3 s */
    ngot = 0; clock_s = 0;
    t->p[P_MSTRUM] = 84; t->p[P_MSDIR] = 0; t->p[P_MVEL] = 0;
    for (i = 0; i < 8u; i++) midi_out_note(t, 48u + 3u * i, 80);
    drain(); run(64);
    for (i = 0; i < 8u; i++) midi_out_note(t, 48u + 3u * i, 0);
    drain(); run(44100u * 4u);
    {
        int ok = 1; uint32_t j, last_on = 0;
        for (i = 0; i < ngot; i++) {
            if (!((got[i].st & 0xF0u) == 0x90u && got[i].vel)) continue;
            last_on = got[i].at;
            for (j = i + 1u; j < ngot && !(got[j].note == got[i].note && !((got[j].st & 0xF0u) == 0x90u && got[j].vel)); j++)
                ;
            if (j >= ngot) ok = 0;
        }
        mcheck("STRUM 440 ms, 8 notes: the last starts ~3 s on", last_on >= 44100u * 3u - 44100u / 5u);
        mcheck("  each note's off after its on", ok && balanced());
    }
    {
        char v[8]; const char *u;
        param_format(&TP[P_MSTRUM], 0, v, &u);
        mcheck("STRUM 0 reads OFF", str_eq(v, "OFF"));
    }
    t->p[P_MSTRUM] = 0;
    /* a chord started and stopped in the same block: on, then off */
    ngot = 0;
    midi_out_note(t, 72, 80); midi_out_note(t, 72, 0); drain();
    run(44100u / 10u);
    mcheck("a note ended before its strum: on then off", ngot == 2u && got[0].vel && !got[1].vel);
    /* HUMAN 100: at most 20 ms late, velocity within 20 */
    t->p[P_MSTRUM] = 0; t->p[P_MVEL] = 0; t->p[P_MHUMAN] = 100;
    {
        int ok = 1;
        for (i = 0; i < 40u; i++) {
            uint32_t at0 = clock_s; ngot = 0;
            midi_out_note(t, 48, 64); drain(); run(44100u / 25u);
            if (ngot != 1u || got[0].at - at0 > 882u + 64u || got[0].vel < 44 || got[0].vel > 84) ok = 0;
            midi_out_note(t, 48, 0); drain(); run(44100u / 25u);
        }
        mcheck("HUMAN 100: <= 20 ms late, velocity 64 +- 20", ok);
    }
    /* STOP with echoes waiting: their ons dropped, the waiting offs sent */
    t->p[P_MHUMAN] = 0; t->p[P_MECHO] = 4; t->p[P_MEFADE] = 0; t->p[P_METIME] = 7;
    ngot = 0;
    midi_out_note(t, 50, 100); drain(); run(seq_beat_samples() + seq_beat_samples() / 4u);
    midi_out_stop(); drain();
    run(seq_beat_samples() * 6u);
    mcheck("STOP: nothing left on", balanced() && mfx_n == 0u);
    printf("midi_fx_test %s\n", mfails ? "FAILED" : "passed");
    return !!mfails;
}
