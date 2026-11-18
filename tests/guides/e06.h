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
/* gamelan: 96 BPM in PELOG on D; a tine line in eighths on TURING as track 1 (EVERY bar a round: the clip pulls it
 * home with GRAVITY), a marimba answering on AWAKE in quarters, a gong of a sub on every other bar, an ORACLE drone */
static void g_gamelan_scene(void)
{
    song.g[G_BPM] = 96;
    scale(2, 61, 0);                                    /* D, PELOG */
    sends(110, 85, 80, 55);
    sound(0, ENGI_FM6, FX_TINE);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 63);
    tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 2);
    tx(0, TX_EV_EVERY, 1); tx(0, TX_EV_AMOUNT, 60);
    mixer(0, 108, -20, 70, 30);
    sound(1, ENGI_FM6, V_DX_MARIMBA);
    trk[1].p[P_SDIV] = DIV_4;
    gen(1, TESS_ENG_AWAKE, 25);
    tx(1, TX_AWK_LENA, 5); tx(1, TX_AWK_LENB, 3); tx(1, TX_AWK_PROB, 60);
    mixer(1, 110, 30, 60, 20);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_2;
    gen(2, TESS_ENG_EUCLID, 4);
    tx(2, TX_EUC_STEPS, 4); tx(2, TX_EUC_PULSES, 1); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    tx(2, TX_NL_MODE, 1); tx(2, TX_NL_LEN, 3);
    mixer(2, 72, 0, 40, 0);
    sound(3, ENGI_ORACLE, OR_DRONE);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 2);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 78, -10, 90, 0);
    g_solo_check();
}
/* Season 1, episode 6: GRAVITY: drift, then come home */
static const gstep_t G_GRAVITY[] = {
    {0, 0, 0, 0, 0, "EVERY bar, track 1's line drifts further from where it began"},
    {4500, 7500, 200, "ARP", "OPEN ARP", "Tap ARP: EVOLVE, EVERY 1 bar"},
    {12000, 15000, 4000, "OCT-", "HOLD OCT-", "Hold OCT- and turn KNOB 4: GRAVITY up to 100%"},
    {0, 16000, 2000, "KNOB 4", "GRAVITY 100", 0, 100},
    {21000, 0, 0, 0, 0, "Each round now steps it back: home is the line it began with"},
    {30000, 33000, 4000, "OCT-", "HOLD OCT-", "Hold OCT- and turn GRAVITY down to 60%"},
    {0, 34000, 1500, "KNOB 4", "GRAVITY 60", 0, -60},
    {39000, 0, 0, 0, 0, "Now it wanders, and keeps coming back home"},
};
/* balearic: 104 BPM, G major; a plucked line on TURING as track 1, an electric piano on MARKOV, a bass on EUCLID, all
 * three with EVERY on (the clip holds them), a soft 909 with a shaker-like hat in STEP */
static void g_balearic_scene(void)
{
    song.g[G_BPM] = 104;
    song.g[G_SWING] = 12;
    scale(7, 1, 0);                                     /* G, MAJ */
    sends(90, 70, 85, 55);
    sound(0, ENGI_FM6, V_PLUCK);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 101);
    tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 2);
    tx(0, TX_EV_EVERY, 1); tx(0, TX_EV_AMOUNT, 50);
    mixer(0, 116, -25, 50, 50);
    sound(1, ENGI_FM6, FX_TINE_EP);
    trk[1].p[P_SDIV] = DIV_4;
    gen(1, TESS_ENG_MARKOV, 30);
    tx(1, TX_MKV_LEAP, 30); tx(1, TX_MKV_DENS, 70); tx(1, TX_MKV_SPAN, 1);
    tx(1, TX_EV_LOCK, 100); tx(1, TX_EV_EVERY, 2); tx(1, TX_EV_AMOUNT, 50);
    mixer(1, 84, 25, 60, 30);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 16);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    tx(2, TX_EV_EVERY, 2); tx(2, TX_EV_AMOUNT, 40);
    mixer(2, 84, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_CH, 0, L_CH, L_CH, 0, L_BD | L_RS, 0, L_CH, L_CH, 0, L_CH, L_OH, 0};
        static const uint8_t V[16] = {90, 0, 44, 36, 0, 44, 36, 0, 84, 0, 44, 36, 0, 44, 52, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 20, 0);
    g_solo_check();
}
/* Season 1, episode 6: HOLD: keep every line as it is */
static const gstep_t G_HOLD[] = {
    {0, 0, 0, 0, 0, "Three tracks here change as they play (EVERY)"},
    {4500, 7500, 2500, "GLO", "HOLD GLO", "Hold GLO and tap D4 twice: HOLD LATCHED"},
    {0, 8500, 150, "D4", "HOLD", 0},
    {0, 8850, 150, "D4", "LATCH", 0},
    {14000, 0, 0, 0, 0, "Now every line stays as it is, bar after bar"},
    {25000, 28000, 1600, "GLO", "HOLD GLO", "Hold GLO and tap D4 once more: HOLD OFF"},
    {0, 29000, 150, "D4", "LET GO", 0},
    {34000, 0, 0, 0, 0, "They carry on changing from where they were"},
    {42000, 0, 0, 0, 0, "HOLD is for playing: it is not saved"},
};
static void guide_e06(const char *out, FILE *tsv)
{
    G_RUN("e06-roll", "A NEW LINE, AND BACK", "GREEN", 46, 4, g_pulse_scene, G_ROLL);   /* pulse minimalism */
    G_RUN("e06-every", "HOW OFTEN IT CHANGES", "GREEN", 49, 4, g_motorik_scene, G_EVERY);   /* motorik */
    G_RUN("e06-gravity", "DRIFT, THEN COME HOME", "GREEN", 58, 4, g_gamelan_scene, G_GRAVITY);   /* gamelan */
    G_RUN("e06-hold", "HOLD THE LINES", "GREEN", 49, 4, g_balearic_scene, G_HOLD);   /* balearic */
}
