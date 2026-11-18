/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 6: let it evolve: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* (g_solo_check, the balance and note checks, is in e05.h) */

/* pulse minimalism: 112 BPM, D mixolydian; a harp line of sixteenths on TURING as track 1, locked (the clip rolls its
 * seed), a twelve-step marimba figure in STEP against it, strings pulsing the chord in eighths, a low lead holding */
static void g_pulse_scene(void)
{
    song.g[G_BPM] = 112;
    scale(2, 4, 0);                                     /* D, MIX */
    sends(95, 80, 90, 50);
    sound(0, ENGI_FM6, FX_HARP);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_TURING, 40);
    tx(0, TX_TUR_LEN, 12); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 2); tx(0, TX_TW_SHIFT, 12);
    mixer(0, 110, -20, 50, 40);
    sound(1, ENGI_FM6, V_DX_MARIMBA);
    {
        static const uint8_t F[12] = {62, 0, 69, 66, 0, 69, 62, 0, 67, 66, 0, 64};
        for (uint32_t i = 0; i < 12u; i++) step_note(1, i, F[i], i ? 74 : 90);
        trk[1].p[P_SLEN] = 12;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 106, 25, 40, 0);
    sound(2, ENGI_ORACLE, OR_SOFT_STRINGS);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 3);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_STEP); tx(2, TX_CH_OCT, -1);
    trk[2].p[P_SDIV] = DIV_8;
    mixer(2, 60, -5, 80, 0);
    sound(3, ENGI_TRIO, TR_SOFT_LEAD);
    trk[3].p[P_SDIV] = DIV_1;
    gen(3, TESS_ENG_AWAKE, 9);
    tx(3, TX_AWK_LENA, 3); tx(3, TX_AWK_LENB, 2); tx(3, TX_AWK_PROB, 20); tx(3, TX_TW_SHIFT, -12);
    mixer(3, 70, 10, 70, 30);
    g_solo_check();
}
/* Season 1, episode 6: ROLL: a new line, and back */
static const gstep_t G_ROLL[] = {
    {0, 0, 0, 0, 0, "Track 1's line comes from its seed"},
    {3500, 6500, 200, "ARP", "OPEN ARP", "Tap ARP: EVOLVE, and KNOB 4 is ROLL, the seed (40)"},
    {11000, 14000, 300, "KNOB 4", "ROLL 41", "Turn KNOB 4 one step: seed 41, a new line at the bar", 1},
    {20000, 0, 0, 0, 0, "Same sound, same settings, new notes"},
    {28000, 31000, 300, "KNOB 4", "ROLL 40", "Turn it back to 40: the first line returns at the bar", -1},
    {38000, 0, 0, 0, 0, "Like it? Note the number. That seed always plays this line"},
};
/* motorik: 124 BPM, A dorian; a clavinet line in eighths on TURING as track 1 (the clip turns EVERY on and off), the
 * straight beat (a kick on one and three, the snare on two and four, eighth hats), a bass in eighths on root and
 * fifth, drawbars holding the chord */
static void g_motorik_scene(void)
{
    song.g[G_BPM] = 124;
    scale(9, SCL_DOR, 0);
    sends(70, 60, 85, 45);
    sound(0, ENGI_FM6, FX_CLAVINET);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 71);
    tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 1); tx(0, TX_TW_SHIFT, 12);
    tx(0, TX_EV_EVERY, 0); tx(0, TX_EV_AMOUNT, 50);
    mixer(0, 108, -20, 40, 50);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {100, 0, 60, 0, 90, 0, 60, 0, 96, 0, 60, 64, 90, 0, 62, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 84, 0, 10, 0);
    sound(2, ENGI_TRIO, TR_FAT_BASS);
    {
        static const uint8_t B[8] = {45, 45, 52, 45, 45, 45, 52, 50};
        for (uint32_t i = 0; i < 8u; i++) step_note(2, i, B[i], i % 2u ? 74 : 92);
        trk[2].p[P_SLEN] = 8;
    }
    tx(2, TX_MODE, TESS_ENG_OFF); trk[2].p[P_SDIV] = DIV_8;
    trk[2].p[P_VOICE] = 1;
    mixer(2, 92, 0, 0, 0);
    sound(3, ENGI_FM6, FX_DRAWBARS);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 6);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 72, 25, 60, 0);
    g_solo_check();
}
/* Season 1, episode 6: EVERY: how often the line changes */
static const gstep_t G_EVERY[] = {
    {0, 0, 0, 0, 0, "Track 1 plays the same line, bar after bar"},
    {3500, 6500, 200, "ARP", "OPEN ARP", "Tap ARP: EVOLVE. EVERY is OFF, so nothing changes"},
    {11000, 14000, 800, "KNOB 2", "EVERY 2", "Turn KNOB 2, EVERY, to 2: the line changes every 2 bars", 2},
    {20000, 0, 0, 0, 0, "Each change is small: AMOUNT (50%) says how much"},
    {32000, 35000, 800, "KNOB 2", "EVERY OFF", "Found one you like? Turn EVERY back to OFF", -2},
    {42000, 0, 0, 0, 0, "OFF keeps the line it has now"},
};
static void guide_e06(const char *out, FILE *tsv)
{
    G_RUN("e06-roll", "A NEW LINE, AND BACK", "GREEN", 46, 4, g_pulse_scene, G_ROLL);   /* pulse minimalism */
    G_RUN("e06-every", "HOW OFTEN IT CHANGES", "GREEN", 49, 4, g_motorik_scene, G_EVERY);   /* motorik */
}
