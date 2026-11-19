/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* More reference clips (MANUAL by chapter): tests/demo_projects.c guide; tools/guide_video.py.
 * (g_solo_check, the balance and note checks, is in e05.h)
 * r-stems and r-usb-host show menu rows the host build leaves out: render them from a build with
 * -DMELODEE_USB_AUDIO=1 -DMELODEE_USB_HOST=1 added */

/* fourth world (after Jon Hassell): 92 BPM, E phrygian; a busy electric piano on MARKOV in sixteenths as track 1 (it
 * holds most of the voices, so the clip watches them on SYSTEM), a warm pad, a sub, hand drums in STEP */
static void g_fourthworld_scene(void)
{
    song.g[G_BPM] = 92;
    scale(4, SCL_PHRY, 0);
    sends(110, 90, 80, 50);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_MARKOV, 27);
    tx(0, TX_MKV_LEAP, 20); tx(0, TX_MKV_DENS, 100); tx(0, TX_MKV_SPAN, 1);
    mixer(0, 90, -15, 90, 40);
    sound(1, ENGI_TRIO, TR_WARM_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 2);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 64, 20, 90, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 3);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 80, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, 0, L_RS, 0, 0, L_TM, 0, L_TM, 0, L_RS, 0, 0, L_TM, 0, L_RS};
        static const uint8_t V[16] = {84, 0, 0, 56, 0, 0, 70, 0, 80, 0, 58, 0, 0, 66, 0, 52};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 96, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_VOICES[] = {
    {0, 0, 0, 0, 0, "All four tracks share one pool of voices: the BUDGET"},
    {3500, 6500, 200, "GLO", "OPEN GLO", "Tap GLO, then turn SELECT to SYSTEM"},
    {0, 8000, 1200, "SELECT", "SYSTEM", 0, 6},
    {12000, 0, 0, 0, 0, "VOICES: the notes ringing now, out of the BUDGET of 16"},
    {20000, 23000, 800, "KNOB 1", "BUDGET 12", "Turn KNOB 1, BUDGET, down to 12: fewer ring at once", -4},
    {30000, 33000, 1000, "SELECT", "MIXER", "Back to MIXER: F#3 mutes track 1, the busy one", -6},
    {0, 35000, 200, "F#3", "MUTE T1", 0},
    {0, 37000, 1000, "SELECT", "SYSTEM", 0, 6},
    {41000, 0, 0, 0, 0, "Track 1 muted: its voices free up, 6 to 8 now ring"},
    {49000, 0, 0, 0, 0, "Crackles? Lower BUDGET, or thin the busiest track"},
};
/* italo disco, minor: 118 BPM, A minor; four clearly different parts (an octave bass on EUCLID, a 909 in STEP, a
 * plucked arp on TURING, a string pad holding the chord), so four stems are easy to tell apart */
static void g_italo_scene(void)
{
    song.g[G_BPM] = 118;
    scale(9, SCL_MIN, 0);
    sends(70, 60, 90, 50);
    sound(0, ENGI_FM6, FX_SOLID_BASS);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_EUCLID, 8);
    tx(0, TX_EUC_STEPS, 16); tx(0, TX_EUC_PULSES, 8); tx(0, TX_EUC_ROT, 1); tx(0, TX_TW_SHIFT, -12);
    mixer(0, 108, 0, 5, 20);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_OH, 0, L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_OH, L_CH};
        static const uint8_t V[16] = {104, 0, 54, 0, 100, 0, 60, 0, 102, 0, 54, 0, 100, 0, 62, 44};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 88, 0, 10, 0);
    sound(2, ENGI_FM6, V_PLUCK);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_TURING, 52);
    tx(2, TX_TUR_LEN, 8); tx(2, TX_TUR_PROB, 0); tx(2, TX_TUR_SPAN, 1);
    mixer(2, 104, 25, 40, 70);
    sound(3, ENGI_ORACLE, OR_SOFT_STRINGS);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 7);
    tx(3, TX_CH_PROG, GEN_CH_POP); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 74, -25, 70, 0);
    g_solo_check();
}
static const gstep_t G_STEMS[] = {
    {0, 0, 0, 0, 0, "Record the four tracks apart, into a computer, over one USB cable"},
    {3500, 6500, 1200, "HOME", "MENU", "Hold HOME: the menu"},
    {10500, 13500, 2500, "KNOB 1", "USB OUTPUT", "Turn KNOB 1 down the list to USB OUTPUT", 26},
    {19000, 22000, 400, "KNOB 2", "4 TRACKS", "Turn KNOB 2: STEREO becomes 4 TRACKS", 1},
    {27000, 0, 0, 0, 0, "The FM-1 reconnects: Tessera Out now has 4 inputs, one a track"},
    {34000, 0, 0, 0, 0, "In the computer: record inputs 1 to 4 of Tessera Out"},
    {41000, 0, 0, 0, 0, "The stems are dry: add the reverb and delay in the computer"},
    {48000, 0, 0, 0, 0, "Back to STEREO for the mix as you hear it"},
};
/* nu-jazz: 100 BPM, D dorian; an electric piano on AWAKE as track 1, brushes-like 909 in STEP, a sub on TURING, a
 * flute holding long notes on EUCLID: the band a USB keyboard would play along with */
static void g_nujazz_scene(void)
{
    song.g[G_BPM] = 100;
    song.g[G_SWING] = 25;
    scale(2, SCL_DOR, 0);
    sends(90, 80, 70, 40);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_AWAKE, 41);
    tx(0, TX_AWK_LENA, 7); tx(0, TX_AWK_LENB, 5); tx(0, TX_AWK_PROB, 70);
    mixer(0, 96, -15, 60, 30);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_CH, L_RS, 0, L_CH, L_CH, 0, L_BD, L_CH, L_CH, L_RS, 0, L_CH, L_OH};
        static const uint8_t V[16] = {90, 0, 52, 40, 70, 0, 52, 40, 0, 80, 52, 40, 72, 0, 54, 48};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 84, 10, 20, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 33);
    tx(2, TX_TUR_LEN, 8); tx(2, TX_TUR_PROB, 5); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 84, 0, 5, 0);
    sound(3, ENGI_ORACLE, OR_FLUTE);
    trk[3].p[P_SDIV] = DIV_2;
    gen(3, TESS_ENG_EUCLID, 6);
    tx(3, TX_EUC_STEPS, 8); tx(3, TX_EUC_PULSES, 3); tx(3, TX_EUC_ROT, 1);
    mixer(3, 90, 30, 90, 50);
    g_solo_check();
}
static const gstep_t G_USB_HOST[] = {
    {0, 0, 0, 0, 0, "Plug a USB-MIDI keyboard or synth straight into the FM-1"},
    {3500, 6500, 1200, "HOME", "MENU", "Hold HOME: the menu"},
    {10500, 13500, 2500, "KNOB 1", "USB HOST", "Turn KNOB 1 down the list to USB HOST", 28},
    {19000, 22000, 400, "KNOB 2", "ON", "Turn KNOB 2: USB HOST ON. The FM-1 is now the computer", 1},
    {27000, 0, 0, 0, 0, "Plug the device in: after a moment, USB MIDI READY (not shown here)"},
    {35000, 0, 0, 0, 0, "Its keys play the tracks; NOTES OUT and CLOCK OUT go to it"},
    {42000, 0, 0, 0, 0, "The FM-1 doesn't power it: use a powered hub"},
    {48000, 0, 0, 0, 0, "While it's on: no USB audio, editor or updates. It starts OFF"},
};
static void guide_ref_more(const char *out, FILE *tsv)
{
    G_RUN("r-voices", "HOW MANY VOICES", "YELLOW", 56, 4, g_fourthworld_scene, G_VOICES);   /* fourth world */
    G_RUN("r-stems", "FOUR STEMS OVER USB", "YELLOW", 54, 4, g_italo_scene, G_STEMS);   /* italo disco */
    G_RUN("r-usb-host", "USB HOST: NO COMPUTER", "WHITE", 54, 4, g_nujazz_scene, G_USB_HOST);   /* nu-jazz */
}
