/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 7: shape a performance: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* MIXER: four plain parts, nothing automatic (no ENTRIES, no PIECE, no mod on FILL), so every key is heard as pressed.
 * T1 a 909 in STEP, its last three steps a snare roll on FILL; T2 a bass on EUCLID; T3 an ORACLE pad on CHORD; T4 a
 * koto on TURING. A minor pentatonic, 100 BPM */
static void g_mixer_scene(void)
{
    song.g[G_BPM] = 100;
    scale(9, 6, 0);
    sends(80, 60, 70, 45);
    sound(0, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_SD, L_SD, L_SD, L_SD};
        static const uint8_t V[16] = {104, 0, 70, 0, 96, 0, 70, 0, 104, 0, 70, 80, 96, 76, 88, 100};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        step_set_cond(&trk[0].step[13], SC_FILL); step_set_cond(&trk[0].step[14], SC_FILL);
        step_set_cond(&trk[0].step[15], SC_FILL);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 100, 0, 15, 0);
    sound(1, ENGI_TRIO, TR_FAT_BASS);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 44);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 5); tx(1, TX_EUC_ROT, 0);
    tx(1, TX_TW_SHIFT, -12); tx(1, TX_NL_MODE, 1); tx(1, TX_NL_LEN, 1);
    mixer(1, 96, 0, 5, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 1;                               /* DIA3 */
    gen(2, TESS_ENG_CHORD, 7);
    tx(2, TX_CH_PROG, GEN_CH_DRONE); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 78, -20, 80, 0);
    sound(3, ENGI_FM6, FX_KOTO_PLUCK);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 23);
    mixer(3, 92, 25, 60, 40);
}
static const gstep_t G_MIXER[] = {
    {0, 0, 0, 0, 0, "MIXER: mute, solo and FILL. Four tracks are playing"},
    {3500, 6500, 200, "GLO", "OPEN MIXER", "Tap GLO: the MIXER page opens"},
    {9000, 12000, 200, "F#3", "MUTE TRACK 1", "Black key F#3 mutes track 1, the drums"},
    {15000, 18000, 200, "F#3", "UNMUTE TRACK 1", "Press F#3 again: the drums come back"},
    {21000, 24000, 5500, "G3", "SOLO TRACK 2", "Hold white key G3: only track 2, the bass, plays. Let go: all four again"},
    {31000, 34000, 5600, "E4", "FILL", "Hold E4 for FILL: the steps set to FILL play, here a snare roll"},
    {40000, 43000, 200, "A#3", "MUTE TRACK 3", "Black keys A#3 and C#4 mute tracks 3 and 4"},
    {0, 44500, 200, "C#4", "MUTE TRACK 4", 0},
    {45500, 48500, 200, "C4", "UNMUTE ALL", "C4 unmutes every track at once"},
    {50500, 0, 0, 0, 0, "On any other page, hold GLO for the same keys"},
};

/* old-school dubstep: 140 BPM at half time (a kick, the snare on the third beat), a bass on EUCLID whose filter its LFO
 * wobbles in eighth triplets, a low pad, a bell now and then. G minor */
static void g_dubstep_scene(void)
{
    song.g[G_BPM] = 140;
    scale(7, SCL_MIN, 0);
    sends(80, 55, 80, 55);
    sound(0, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, 0, 0, L_CH, 0, L_SD, 0, L_CH | L_BD, 0, 0, L_RS, L_CH, 0};
        static const uint8_t V[16] = {110, 0, 60, 0, 0, 0, 56, 0, 106, 0, 70, 0, 0, 62, 58, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 100, 0, 10, 0);
    sound(1, ENGI_TRIO, TR_FAT_BASS);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 21);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 3); tx(1, TX_EUC_ROT, 0); tx(1, TX_TW_SHIFT, -12);
    tx(1, TX_NL_MODE, 1); tx(1, TX_NL_LEN, 4);
    lfo(1, 0, 7);                                       /* (8T: the wobble) */
    cord(0, CS_LFO, 1, CD_CUTOFF, 1, 70, 0, 0);         /* T2 LFO -> T2 CUT */
    mixer(1, 100, 0, 0, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 8);
    tx(2, TX_CH_PROG, GEN_CH_DRONE); tx(2, TX_CH_BARS, 4); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 64, -20, 80, 0);
    sound(3, ENGI_FM6, FX_RIN);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 6);
    tx(3, TX_TUR_PROB, 10);
    mixer(3, 76, 25, 70, 60);
}
/* electro: 112 BPM, an 808 (kick on the one and the offbeats, claps on two and four, hats), a saw bass on TURING in
 * sixteenths, a lead on EUCLID, a pad under it. D minor */
static void g_electro_scene(void)
{
    song.g[G_BPM] = 112;
    scale(2, SCL_MIN, 0);
    sends(60, 50, 70, 45);
    sound(0, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_CP, 0, L_BD | L_CH, 0, 0, 0, L_BD | L_CH, 0, L_CP, 0, L_CH, L_CH};
        static const uint8_t V[16] = {112, 0, 70, 0, 100, 0, 96, 0, 0, 0, 100, 0, 104, 0, 72, 60};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 100, 0, 8, 0);
    sound(1, ENGI_FM6, FX_SAW_BASS);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_TURING, 33);
    tx(1, TX_TUR_PROB, 0); tx(1, TX_TW_SHIFT, -12); tx(1, TX_TW_DROP, 35);
    mixer(1, 92, 0, 0, 20);
    sound(2, ENGI_FM6, FX_SYNC_LEAD);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_EUCLID, 15);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 2);
    mixer(2, 70, 25, 30, 60);
    sound(3, ENGI_ORACLE, OR_WARM_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 4);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 60, -25, 70, 0);
}
static const gstep_t G_MUTES[] = {
    {0, 0, 0, 0, 0, "A mute can wait for the next bar, so a drop lands in time"},
    {3500, 6500, 200, "GLO", "OPEN MIXER", "Tap GLO: the MIXER page"},
    {11000, 14000, 800, "OCT+", "OPEN SHEET", "Hold OCT+: MIXER's sheet, with Mutes on NOW"},
    {19000, 22000, 600, "KNOB 2", "MUTES BAR", "Turn KNOB 2: Mutes on BAR", 1},
    {27000, 30000, 200, "OCT-", "CLOSE", "Tap OCT-: the sheet closes"},
    {35000, 38000, 200, "F#3", "MUTE ON BAR", "Press F#3: its key blinks, and the drums stop on the bar line"},
    {44000, 47000, 200, "F#3", "BACK ON BAR", "Press F#3 again: the drums come back on the bar"},
    {52500, 0, 0, 0, 0, "PHRASE waits for the phrase line. MENU > MUTES is the same setting"},
};
static const gstep_t G_TEMPO[] = {
    {0, 0, 0, 0, 0, "Set the tempo with a knob, or tap it in"},
    {3500, 6500, 1600, "SELECT", "TEMPO UP", "On HOME, turn SELECT: the tempo (112 becomes 120)", 8},
    {12000, 15000, 200, "GLO", "OPEN MIXER", "Tap GLO: the MIXER page"},
    {20000, 23000, 120, "F4", "TAP", "Tap F4 in time, four times: the tempo follows your taps (here 124)"},
    {0, 23480, 120, "F4", "TAP", 0},
    {0, 23960, 120, "F4", "TAP", 0},
    {0, 24440, 120, "F4", "TAP", 0},
    {31000, 0, 0, 0, 0, "With GLO held, SELECT sets the tempo on any page"},
};
static void guide_e07(const char *out, FILE *tsv)
{
    G_RUN("e07-mixer", "MUTE, SOLO, FILL", "MOSAIC", 59, 4, g_mixer_scene, G_MIXER); /* a beat */
    G_RUN("e07-mutes", "MUTES ON THE BAR", "MOSAIC", 61, 4, g_dubstep_scene, G_MUTES); /* old-school dubstep */
    G_RUN("e07-tempo", "THE TEMPO", "MOSAIC", 39, 4, g_electro_scene, G_TEMPO);        /* electro */
}
