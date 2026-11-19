/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Reference clips, by manual chapter (ids r-): the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* trip-hop: 82 BPM, a dusty 909 (a lazy kick, a rim), a sub on EUCLID, an ORACLE pad holding the chord as track 1
 * (its filter for the first cord), a tine piano on TURING. G minor */
static void g_triphop_scene(void)
{
    song.g[G_BPM] = 82;
    song.g[G_SWING] = 18;
    scale(7, SCL_MIN, 0);
    sends(90, 70, 70, 50);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 24);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    trk[0].p[P_LRATE] = 40;
    mixer(0, 100, 0, 60, 0);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_RS, 0, L_CH, L_BD, 0, L_BD, L_CH, 0, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {100, 0, 54, 0, 80, 0, 50, 64, 0, 90, 56, 0, 96, 0, 52, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 86, 0, 20, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 31);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 4); tx(2, TX_EUC_ROT, 1); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 96, 0, 0, 0);
    sound(3, ENGI_FM6, FX_TINE_EP);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 58);
    tx(3, TX_TUR_PROB, 10); tx(3, TX_TW_DROP, 60);
    mixer(3, 70, 25, 60, 40);
    g_solo_check();
}
static const gstep_t G_MOD_FIRST[] = {
    {0, 0, 0, 0, 0, "A mod moves one thing with another. Make one: an LFO on a filter"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS, the cords"},
    {10500, 13500, 600, "KNOB 1", "SOURCE", "Turn KNOB 1: the source (here T1 LFO)", 1},
    {19000, 22000, 900, "KNOB 3", "DESTINATION", "Turn KNOB 3: what it moves (here T1 CUTOFF). It starts at +50 %, heard at once", 2},
    {27500, 30500, 6000, "OCT-", "SCOPE", "Hold OCT-: the scope. The line is the mod, the faint dots its source"},
    {33000, 36000, 200, "OCT+", "PIN", "Still holding OCT-, tap OCT+: the scope stays (SCOPE PINNED)"},
    {40000, 43000, 1500, "KNOB 4", "AMOUNT", "Turn KNOB 4: the amount, +50 to +80 %. The line swings wider", 30},
    {49500, 52500, 200, "OCT-", "FREE", "Tap OCT-: SCOPE FREE, the rows come back"},
    {57000, 0, 0, 0, 0, "Sixteen cords; any source, on any track, onto any destination"},
};
/* downtempo: 90 BPM, a marimba figure written in STEP as track 1 (eight notes, one to get a condition), a soft
 * 808, a sub on EUCLID, a pad. A minor */
static void g_downtempo_scene(void)
{
    static const uint8_t N[8] = {57, 64, 60, 69, 57, 64, 72, 67};
    song.g[G_BPM] = 90;
    scale(9, SCL_MIN, 0);
    sends(85, 65, 70, 45);
    sound(0, ENGI_FM6, V_DX_MARIMBA);
    for (uint32_t i = 0; i < 8u; i++) step_note(0, i, N[i], 100);
    trk[0].p[P_SLEN] = 8;
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_8;
    mixer(0, 100, -10, 50, 30);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, L_BD, L_CH, 0, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {96, 0, 50, 0, 84, 0, 48, 0, 90, 60, 52, 0, 86, 0, 54, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 80, 0, 15, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 9);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 92, 0, 0, 0);
    sound(3, ENGI_TRIO, TR_SOFT_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 5);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 60, 20, 80, 0);
    g_solo_check();
}
static const gstep_t G_COND[] = {
    {0, 0, 0, 0, 0, "A step can play only on some passes, so a short loop sounds longer"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: track 1 is a STEP pattern, written by hand"},
    {10000, 13000, 600, "SELECT", "NOTES", "Turn SELECT: NOTES", 1},
    {17000, 20000, 900, "KNOB 1", "PICK STEP", "Turn KNOB 1 to a note (step 7, C5)", 6},
    {24000, 27000, 200, "OCT+", "SHEET", "Tap OCT+: the note's sheet"},
    {31000, 34000, 900, "KNOB 1", "CONDITION", "Turn KNOB 1 to CONDITION", 3},
    {38000, 41000, 600, "KNOB 2", "1:2", "Turn KNOB 2 to 1:2: the first of every two passes", 1},
    {45000, 48000, 200, "OCT-", "CLOSE", "Tap OCT-, and listen: C5 now comes every other time"},
    {56000, 0, 0, 0, 0, "Others: 1:3 to 8:8, FIRST, !FIRST, and FILL for fills on demand"},
};
/* ambient techno: 118 BPM, A minor; a TRIO pad on CHORD, a tine line on TURING, a sub on EUCLID, a 909 on ATLAS. The
 * CHAIN ABAC in 4-bar phrases and ENTRIES at 75, so a PIECE has something to steer: layers out and in */
static void g_ambtechno_scene(void)
{
    static const uint8_t REP[4] = {1, 1, 1, 1}, ROLE[4] = {RO_HARMONY, RO_MELODY, RO_BASS, RO_RHYTHM};
    song.g[G_BPM] = 118;
    scale(9, SCL_MIN, 0);
    sends(95, 75, 75, 55);
    sound(0, ENGI_TRIO, TR_WARM_PAD);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 3);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    mixer(0, 80, -20, 80, 0);
    sound(1, ENGI_FM6, FX_TINE_EP);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 17);
    tx(1, TX_TUR_PROB, 20);
    mixer(1, 90, 25, 60, 40);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 5);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 92, 0, 10, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    gen(3, TESS_ENG_GRIDS, 9);
    mixer(3, 85, 0, 20, 0);
    chain_set(FC_ABAC, PH_4, 0, REP, 0, 0);
    entries_set(75, 1, 1, 1, ROLE);
    g_solo_check();
}
static const gstep_t G_PIECE[] = {
    {0, 0, 0, 0, 0, "PIECE plans a whole piece: how long it lasts, and its shape"},
    {3500, 6500, 200, "GLO", "OPEN MIXER", "Tap GLO: MIXER"},
    {10000, 13000, 200, "GLO", "PERF", "Tap GLO again: PERF"},
    {16500, 19500, 200, "GLO", "PIECE", "Once more: PIECE"},
    {23000, 26000, 600, "KNOB 1", "1 MINUTE", "Turn KNOB 1: LENGTH, from FREE to 1 minute. SHAPE is ARCH", 1},
    {30000, 33000, 200, "PLAY", "STOP", "Press PLAY twice: it starts from the top, and the piece with it"},
    {0, 34500, 200, "PLAY", "PLAY", 0},
    {37500, 0, 0, 0, 0, "It begins sparse: the bass alone"},
    {46000, 0, 0, 0, 0, "As it builds, tracks come in on phrase lines"},
    {58500, 0, 0, 0, 0, "The top of the arch: all four play"},
    {74500, 0, 0, 0, 0, "Then it thins out the same way"},
    {99000, 0, 0, 0, 0, "At the end the tracks fade, the tails ring, and it stops"},
};
/* chillout: 96 BPM, E dorian (as E minor's brighter sister); a soft 808, a kalimba on QUAD-less EUCLID, a sub, an
 * ORACLE pad: four parts, so MOSAIC has four colours to show */
static void g_chillout_scene(void)
{
    song.g[G_BPM] = 96;
    scale(4, SCL_DOR, 0);
    sends(95, 70, 70, 50);
    sound(0, ENGI_FM6, FX_KALIMBA);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_EUCLID, 13);
    tx(0, TX_EUC_STEPS, 16); tx(0, TX_EUC_PULSES, 7); tx(0, TX_EUC_ROT, 2);
    mixer(0, 92, 20, 60, 50);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_CP, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_CP, 0, L_OH, 0};
        static const uint8_t V[16] = {96, 0, 52, 0, 80, 0, 50, 0, 90, 0, 54, 62, 82, 0, 56, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 84, 0, 20, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 41);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 4); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 94, 0, 0, 0);
    sound(3, ENGI_ORACLE, OR_WARM_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 12);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 66, -20, 80, 0);
    g_solo_check();
}
static const gstep_t G_COLOR[] = {
    {0, 0, 0, 0, 0, "Pick the screen's colour: one ink, or a colour for each track"},
    {3500, 6500, 900, "HOME", "MENU", "Hold HOME: the menu. COLOR is its first row"},
    {11000, 14000, 2400, "KNOB 2", "INKS", "Turn KNOB 2: the inks (here to BLUE)", 3},
    {20000, 23000, 3000, "KNOB 2", "MOSAIC", "Further: MOSAIC, a colour for each track", 6},
    {29500, 32500, 200, "OCT-", "CLOSE", "Tap OCT- twice: the colour picker closes, then the menu"},
    {0, 34200, 200, "OCT-", "CLOSE", 0},
    {38000, 0, 0, 0, 0, "Back on HOME: the four tracks, each in its own colour"},
    {45500, 0, 0, 0, 0, "GROUND, just below COLOR, draws the ink on paper (LIGHT)"},
};
/* neo-classical: 72 BPM, A minor; a harp wandering on AWAKE (track 1: far out on the water), strings holding the
 * chord, a flute on TURING, a kalimba on EUCLID (track 4: close by) */
static void g_neoclassical_scene(void)
{
    song.g[G_BPM] = 72;
    scale(9, SCL_MIN, 0);
    sends(110, 85, 60, 50);
    sound(0, ENGI_FM6, FX_HARP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_AWAKE, 19);
    tx(0, TX_AWK_LENA, 7); tx(0, TX_AWK_LENB, 5); tx(0, TX_AWK_PROB, 60);
    mixer(0, 92, -20, 80, 30);
    sound(1, ENGI_TRIO, TR_STRING_MACH);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 4);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 70, 0, 90, 0);
    sound(2, ENGI_TRIO, TR_BREATH_FLUTE);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 26);
    tx(2, TX_TUR_PROB, 15); tx(2, TX_TW_DROP, 50);
    mixer(2, 76, 20, 80, 40);
    sound(3, ENGI_FM6, FX_KALIMBA);
    trk[3].p[P_SDIV] = DIV_16;
    gen(3, TESS_ENG_EUCLID, 37);
    tx(3, TX_EUC_STEPS, 16); tx(3, TX_EUC_PULSES, 5); tx(3, TX_EUC_ROT, 3); tx(3, TX_TW_SHIFT, 12);
    mixer(3, 74, 30, 60, 50);
    g_solo_check();
}
static const gstep_t G_LANDSCAPE[] = {
    {0, 0, 0, 0, 0, "LANDSCAPE: a night sea, drawn by the music"},
    {3500, 6500, 200, "GLO", "MIXER", "Tap GLO twice: PERF"},
    {0, 8000, 200, "GLO", "PERF", 0},
    {12000, 15000, 2000, "SELECT", "LANDSCAPE", "Turn SELECT: on to LANDSCAPE", 4},
    {21000, 0, 0, 0, 0, "Each note is a drop: low notes land left, high right"},
    {29000, 0, 0, 0, 0, "Track 1's drops land far out, track 4's close by, each in its colour"},
    {37000, 40000, 200, "F#3", "MUTE T1", "The keys are MIXER's: mute all four tracks"},
    {0, 41500, 200, "G#3", "MUTE T2", 0},
    {0, 43000, 200, "A#3", "MUTE T3", 0},
    {0, 44500, 200, "C#4", "MUTE T4", 0},
    {48000, 0, 0, 0, 0, "Muted, they make no drops: the water goes still"},
    {56000, 59000, 200, "C4", "UNMUTE ALL", "C4 brings them all back"},
};
/* bossa nova: 128 BPM with a little swing, D dorian; a flute line on TURING as track 1 (its FORM), a plucked
 * guitar on CHORD on the beats (HIT STEP at 1/4), a bass on EUCLID, rim and shaker on a 909 */
static void g_bossa_scene(void)
{
    song.g[G_BPM] = 128;
    song.g[G_SWING] = 12;
    scale(2, SCL_DOR, 0);
    sends(80, 60, 60, 40);
    sound(0, ENGI_FM6, V_FLUTE);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 44);
    tx(0, TX_TUR_PROB, 0);
    mixer(0, 116, 15, 50, 30);
    sound(1, ENGI_FM6, V_PLUCK);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 7);
    tx(1, TX_CH_PROG, GEN_CH_251); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_STEP);
    trk[1].p[P_SDIV] = DIV_4;                           /* (Theo: no offbeat chords here; HIT STEP on the beats) */
    mixer(1, 86, -20, 40, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 16);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 80, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_RS, L_CH, 0, L_RS | L_CH, 0, L_BD, 0, L_CH, L_RS, L_CH, 0, L_RS | L_CH, 0};
        static const uint8_t V[16] = {80, 0, 50, 70, 46, 0, 72, 0, 76, 0, 50, 70, 46, 0, 72, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 15, 0);
    g_solo_check();
}
static const gstep_t G_FORM[] = {
    {0, 0, 0, 0, 0, "FORM makes one track's line a little song: A, then B, and back"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE, how the line changes"},
    {10000, 13000, 200, "ARP", "FORM", "Tap ARP again: FORM"},
    {17000, 20000, 600, "KNOB 1", "FORM AB", "Turn KNOB 1: FORM AB, each section 4 bars", 1},
    {26000, 0, 0, 0, 0, "Section B: a new line, made from A (CONTRAST 60 %)"},
    {33500, 0, 0, 0, 0, "Back to A"},
    {41000, 0, 0, 0, 0, "B again, note for note: DEVELOP at 0 keeps each return exact"},
    {48500, 0, 0, 0, 0, "Hold OCT- for DEVELOP: returns that change a little each time"},
};
/* liquid drum and bass: 172 BPM, F minor; a broken 909 as track 1 (RHYTHM), a sub on EUCLID (BASS), an ORACLE pad
 * (HARMONY), a bell line on TURING (MELODY); the CHAIN ABAC in 4-bar phrases, so ENTRIES has lines to move on */
static void g_dnb_scene(void)
{
    static const uint8_t REP[4] = {1, 1, 1, 1}, ROLE[4] = {RO_RHYTHM, RO_BASS, RO_HARMONY, RO_MELODY};
    song.g[G_BPM] = 172;
    scale(5, SCL_MIN, 0);
    sends(95, 70, 70, 50);
    sound(0, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, 0, L_BD | L_CH, 0, L_SD, 0, L_CH, L_SD};
        static const uint8_t V[16] = {108, 0, 62, 0, 104, 0, 60, 80, 0, 0, 96, 0, 104, 0, 64, 50};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 92, 0, 10, 0);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_EUCLID, 22);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 3); tx(1, TX_EUC_ROT, 0); tx(1, TX_TW_SHIFT, -12);
    tx(1, TX_NL_MODE, 1); tx(1, TX_NL_LEN, 4);
    mixer(1, 100, 0, 0, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 2;
    gen(2, TESS_ENG_CHORD, 10);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 70, -20, 80, 0);
    sound(3, ENGI_FM6, FX_RIN);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 39);
    tx(3, TX_TUR_PROB, 10);
    mixer(3, 80, 25, 70, 50);
    chain_set(FC_ABCB, PH_4, 0, REP, 0, 0);
    entries_set(0, 1, 1, 1, ROLE);
    g_solo_check();
}
static const gstep_t G_ENTRIES[] = {
    {0, 0, 0, 0, 0, "ENTRIES arranges for you: tracks drop out and come back on phrase lines"},
    {3500, 6500, 200, "GLO", "MIXER", "Tap GLO twice: PERF"},
    {0, 8000, 200, "GLO", "PERF", 0},
    {12000, 15000, 1500, "SELECT", "ENTRIES", "Turn SELECT: on to ENTRIES", 3},
    {19000, 22000, 4000, "KNOB 1", "76 %", "Turn KNOB 1: ENTRIES 76 %, how many tracks may be out at once", 38},
    {28500, 0, 0, 0, 0, "Each track's part decides its turn: bass in first, melody out first"},
    {35100, 0, 0, 0, 0, "Before A comes round again, only the bass plays on"},
    {40700, 0, 0, 0, 0, "A returns with everything"},
    {46200, 0, 0, 0, 0, "Away from A it thins again: the melody leaves first"},
    {51800, 0, 0, 0, 0, "and is back on the next line. Your own mutes still win"},
};
/* disco: 116 BPM, A minor; an 808 four on the floor (open hats on the offbeat, claps), an octave bass on EUCLID, a
 * clavinet on POLY, strings holding the chord. CHAIN FORM OFF until the clip turns it on; KEY B +5 (a fourth up) */
static void g_disco_scene(void)
{
    static const uint8_t REP[4] = {1, 1, 1, 1};
    song.g[G_BPM] = 116;
    scale(9, SCL_MIN, 0);
    sends(75, 60, 70, 45);
    sound(0, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_OH, 0, L_BD | L_CP, 0, L_OH, 0, L_BD, 0, L_OH, 0, L_BD | L_CP, 0, L_OH, L_CH};
        static const uint8_t V[16] = {110, 0, 70, 0, 104, 0, 70, 0, 106, 0, 72, 0, 104, 0, 70, 50};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 92, 0, 10, 0);
    sound(1, ENGI_FM6, FX_SOLID_BASS);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 18);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 8); tx(1, TX_EUC_ROT, 1); tx(1, TX_TW_SHIFT, -12);
    sections(1, 40, 0, 0);
    mixer(1, 96, 0, 0, 0);
    sound(2, ENGI_FM6, FX_CLAVINET);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_POLY, 52);
    tx(2, TX_POL_DIV1, 3); tx(2, TX_POL_DIV2, 4); tx(2, TX_POL_DIV3, 6); tx(2, TX_POL_DIV4, 0); tx(2, TX_POL_SPAN, 1);
    sections(2, 60, 0, 0);
    mixer(2, 78, 20, 30, 40);
    sound(3, ENGI_TRIO, TR_STRING_MACH);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 6);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 66, -20, 80, 0);
    chain_set(FC_OFF, PH_4, 0, REP, 5, 0);
    g_solo_check();
}
static const gstep_t G_CHAIN[] = {
    {0, 0, 0, 0, 0, "CHAIN gives the whole project one FORM: the tracks change section together"},
    {3500, 6500, 200, "GLO", "MIXER", "Tap GLO twice: PERF"},
    {0, 8000, 200, "GLO", "PERF", 0},
    {12000, 15000, 800, "SELECT", "CHAIN", "Turn SELECT: on to CHAIN", 2},
    {19000, 22000, 600, "KNOB 1", "FORM AB", "Turn KNOB 1: FORM AB, a section every 4-bar phrase", 1},
    {28000, 0, 0, 0, 0, "A: the project as it was. Tracks 2 to 4 follow the CHAIN"},
    {37400, 0, 0, 0, 0, "B: each following track moves to its own B, all at once"},
    {45700, 0, 0, 0, 0, "Back to A, together"},
    {53900, 0, 0, 0, 0, "This project's B is a fourth up: Key in B, on SCL's last rows"},
};
/* afro house: 120 BPM, A minor; a 909 on ATLAS as track 1 (a generator, so ENERGY moves it), a marimba on EUCLID, a
 * bass on TURING, a pad holding the chord */
static void g_afrohouse_scene(void)
{
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 8;
    scale(9, SCL_MIN, 0);
    sends(75, 60, 65, 45);
    sound(0, ENGI_DRUM, DK_909_KIT);
    gen(0, TESS_ENG_GRIDS, 21);
    mixer(0, 90, 0, 10, 0);
    sound(1, ENGI_FM6, V_DX_MARIMBA);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 34);
    tx(1, TX_EUC_STEPS, 12); tx(1, TX_EUC_PULSES, 7); tx(1, TX_EUC_ROT, 1);
    mixer(1, 92, 20, 40, 30);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 29);
    tx(2, TX_TUR_PROB, 15); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 30);
    mixer(2, 94, 0, 0, 0);
    sound(3, ENGI_ORACLE, OR_WARM_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 8);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 58, -20, 80, 0);
    g_solo_check();
}
static const gstep_t G_ENERGY[] = {
    {0, 0, 0, 0, 0, "ENERGY: the project's mood, from sparse and soft to busy and loud"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE"},
    {10000, 13000, 600, "SELECT", "ENERGY", "Turn SELECT back once: ENERGY, the last page", -1},
    {17000, 20000, 3000, "KNOB 3", "LEVEL DOWN", "Turn KNOB 3: LEVEL to -100. The generators leave out hits and play softer", -100},
    {31000, 34000, 4000, "KNOB 3", "LEVEL UP", "Turn it up to +100: busier and louder", 200},
    {45000, 0, 0, 0, 0, "ARC swings LEVEL as a slow tide over minutes; RESTS adds silences"},
};
/* folktronica: 98 BPM, G major; a plucked line on TURING as track 1 (the one the clip tweaks), a soft 808 with a
 * shaker, a kalimba on EUCLID, a bass on two pulses */
static void g_folktronica_scene(void)
{
    song.g[G_BPM] = 98;
    scale(7, 1, 0);                                     /* G, MAJ */
    sends(80, 65, 65, 45);
    sound(0, ENGI_FM6, V_PLUCK);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_TURING, 62);
    tx(0, TX_TUR_PROB, 10);
    mixer(0, 100, -15, 40, 40);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_CH, L_RS, 0, L_CH, L_CH, L_BD, L_BD, L_CH, L_CH, L_RS, 0, L_CH, L_CH};
        static const uint8_t V[16] = {96, 0, 44, 30, 80, 0, 46, 30, 90, 60, 44, 30, 82, 0, 46, 30};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 80, 0, 15, 0);
    sound(2, ENGI_FM6, FX_KALIMBA);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 23);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 2); tx(2, TX_TW_SHIFT, 12);
    mixer(2, 66, 25, 60, 40);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_EUCLID, 7);
    tx(3, TX_EUC_STEPS, 8); tx(3, TX_EUC_PULSES, 2); tx(3, TX_TW_SHIFT, -12);
    mixer(3, 90, 0, 0, 0);
    g_solo_check();
}
static const gstep_t G_TWEAK[] = {
    {0, 0, 0, 0, 0, "TWEAK reshapes what a generator plays, without changing the generator"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE"},
    {10000, 13000, 900, "SELECT", "TWEAK", "Turn SELECT twice: TWEAK", 2},
    {17000, 20000, 1500, "KNOB 1", "SHIFT", "Turn KNOB 1: SHIFT, here +12, the line an octave up", 12},
    {27000, 30000, 2000, "KNOB 2", "DROP", "Turn KNOB 2: DROP, here 50 %, about half the notes left out", 25},
    {38000, 0, 0, 0, 0, "KNOB 3, HUMAN, varies each note's loudness and length. KNOB 4, FOLLOW, has a clip of its own"},
};
/* cinematic: 84 BPM, D minor; a harp line on TURING as track 1 (the leader), a sub on EUCLID in quarters as track 2
 * (the follower: its own rhythm, until FOLLOW its own notes), strings holding the chord, soft toms in STEP */
static void g_cinematic_scene(void)
{
    song.g[G_BPM] = 84;
    scale(2, SCL_MIN, 0);
    sends(110, 80, 60, 50);
    sound(0, ENGI_FM6, FX_HARP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 71);
    tx(0, TX_TUR_PROB, 15);
    mixer(0, 96, -20, 80, 30);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_4;
    gen(1, TESS_ENG_EUCLID, 15);
    tx(1, TX_EUC_STEPS, 8); tx(1, TX_EUC_PULSES, 5); tx(1, TX_EUC_ROT, 0); tx(1, TX_TW_SHIFT, -12);
    mixer(1, 100, 0, 0, 0);
    sound(2, ENGI_TRIO, TR_STRING_MACH);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 18);
    tx(2, TX_CH_PROG, GEN_CH_DRONE); tx(2, TX_CH_BARS, 3); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 62, 20, 90, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, 0, 0, 0, 0, L_TM, 0, L_BD, 0, 0, 0, 0, 0, L_TM, L_TM};
        static const uint8_t V[16] = {90, 0, 0, 0, 0, 0, 70, 0, 80, 0, 0, 0, 0, 0, 60, 74};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 50, 0);
    g_solo_check();
}
static const gstep_t G_FOLLOW[] = {
    {0, 0, 0, 0, 0, "FOLLOW: one track plays another's notes, in its own rhythm"},
    {3500, 6500, 600, "ALGORITHM", "TRACK 2", "Turn ALGORITHM to track 2, the bass", 1},
    {10000, 13000, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE"},
    {16500, 19500, 900, "SELECT", "TWEAK", "Turn SELECT twice: TWEAK", 2},
    {23500, 26500, 600, "KNOB 4", "FOLLOW T1", "Turn KNOB 4: FOLLOW T1. The bass takes the harp's notes", 1},
    {33500, 36500, 1200, "KNOB 1", "SHIFT -7", "While following, SHIFT counts scale steps: KNOB 1 to -7, an octave down", 5},
    {44000, 0, 0, 0, 0, "The bass now doubles the harp an octave down, in its own rhythm"},
};
/* boom bap: 90 BPM with swing, C minor; track 1 is ATLAS on the 909 playing only the hats (the track that ratchets:
 * Theo heard the kick roll and click when the whole kit rolled), a sparse sub on TURING, organ chords on the beat,
 * and the kick and snare written in STEP on track 4, so they stay put while the hats roll */
static void g_boombap_scene(void)
{
    song.g[G_BPM] = 90;
    song.g[G_SWING] = 30;
    scale(0, SCL_MIN, 0);
    sends(70, 55, 60, 40);
    sound(0, ENGI_DRUM, DK_909_KIT);
    gen(0, TESS_ENG_GRIDS, 44);
    tx(0, TX_GRD_KICK, 0); tx(0, TX_GRD_SNR, 0); tx(0, TX_GRD_HAT, 100);
    mixer(0, 127, 10, 12, 0);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 12);
    tx(1, TX_TUR_PROB, 10); tx(1, TX_TW_SHIFT, -12); tx(1, TX_TW_DROP, 55);
    mixer(1, 74, 0, 0, 0);
    sound(2, ENGI_FM6, FX_ORGAN);
    trk[2].p[P_CHRD] = 2;
    gen(2, TESS_ENG_CHORD, 20);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 1); tx(2, TX_CH_HIT, GEN_CH_BEAT);
    mixer(2, 52, -20, 40, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, 0, L_SD, 0, 0, L_BD, 0, 0, L_BD, 0, L_SD, 0, 0, 0};
        static const uint8_t V[16] = {110, 0, 0, 0, 100, 0, 0, 70, 0, 0, 96, 0, 104, 0, 0, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 82, 0, 10, 0);
    g_solo_check();
}
static const gstep_t G_TIMING[] = {
    {0, 0, 0, 0, 0, "TIMING: rolls and feel for a generator track. Here, the hats on ATLAS"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE"},
    {10000, 13000, 1200, "SELECT", "TIMING", "Turn SELECT back twice: TIMING", -2},
    {17000, 20000, 900, "KNOB 1", "RATCH x4", "Turn KNOB 1: RATCH x4, each step played four times", 3},
    {26000, 29000, 6000, "KNOB 2", "CHANCE", "Turn KNOB 2: CHANCE down to 30 %, so only some steps roll", -70},
    {38500, 41500, 600, "KNOB 3", "ACCEL", "Turn KNOB 3: SHAPE ACCEL, rolls that speed up", 1},
    {49500, 0, 0, 0, 0, "KNOB 4, HUMAN, plays each step a little late, like a player behind the beat"},
};
/* chill trap: 70 BPM (half time), F minor; an 808 written in STEP as track 1 (the step that rolls is a hat), a soft
 * sub on EUCLID, a pad, a harp line on TURING */
static void g_chilltrap_scene(void)
{
    song.g[G_BPM] = 70;
    scale(5, SCL_MIN, 0);
    sends(90, 70, 70, 50);
    sound(0, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_CH, 0, L_CH, 0, L_SD | L_CH, 0, L_CH, L_BD, L_CH, 0, L_CH, 0};
        static const uint8_t V[16] = {104, 0, 60, 0, 64, 0, 58, 0, 100, 0, 62, 80, 60, 0, 66, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 92, 0, 12, 0);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_EUCLID, 30);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 3); tx(1, TX_EUC_ROT, 0); tx(1, TX_TW_SHIFT, -12);
    mixer(1, 94, 0, 0, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 17);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 62, -20, 80, 0);
    sound(3, ENGI_FM6, FX_HARP);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 59);
    tx(3, TX_TUR_PROB, 12); tx(3, TX_TW_DROP, 50);
    mixer(3, 74, 25, 70, 40);
    g_solo_check();
}
static const gstep_t G_RATCHET[] = {
    {0, 0, 0, 0, 0, "One step can roll: a ratchet plays it again, inside its own step"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: track 1 is a drum pattern, written in STEP"},
    {10000, 13000, 600, "SELECT", "NOTE", "Turn SELECT: NOTE, the drum grid", 1},
    {16500, 19500, 600, "KNOB 2", "CH ROW", "Turn KNOB 2 to the CH row, the hats", 3},
    {23000, 26000, 1200, "KNOB 1", "STEP 15", "Turn KNOB 1 to step 15", 14},
    {29500, 32500, 200, "OCT+", "SHEET", "Tap OCT+: that hit's sheet"},
    {36000, 39000, 900, "KNOB 1", "RATCHET", "Turn KNOB 1 to RATCHET", 3},
    {42500, 45500, 900, "KNOB 2", "X4", "Turn KNOB 2 to X4: the hat plays four times in its step", 3},
    {49000, 52000, 200, "OCT-", "CLOSE", "Tap OCT-, and listen: a hat roll near the end of each bar"},
    {59000, 0, 0, 0, 0, "TIMING's SHAPE makes a STEP track's rolls speed up or slow down"},
};
/* dream pop: 88 BPM, E major; a warm TRIO voice on TURING as track 1 (sparse; its LENGTH the clip changes), a soft
 * 808, a bass on EUCLID, a chime line */
static void g_dreampop_scene(void)
{
    song.g[G_BPM] = 88;
    scale(4, 1, 0);                                     /* E, MAJ */
    sends(110, 85, 65, 50);
    sound(0, ENGI_TRIO, TR_WARM_PAD);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 46);
    tx(0, TX_TUR_PROB, 15); tx(0, TX_TW_DROP, 55);
    mixer(0, 100, -10, 90, 30);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {92, 0, 46, 0, 84, 0, 46, 0, 88, 0, 46, 60, 86, 0, 50, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 76, 0, 40, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 25);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 4); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 90, 0, 0, 0);
    sound(3, ENGI_FM6, FX_FURIN);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_AWAKE, 38);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 3); tx(3, TX_AWK_PROB, 35);
    mixer(3, 62, 25, 100, 60);
    g_solo_check();
}
static const gstep_t G_LENGTH[] = {
    {0, 0, 0, 0, 0, "LENGTH: how long a generator's notes last"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE"},
    {10000, 13000, 1500, "SELECT", "LENGTH", "Turn SELECT on to LENGTH", 4},
    {18000, 21000, 900, "KNOB 1", "HIT", "Turn KNOB 1 to HIT: each note rings on until the next, so the gaps fill", 2},
    {32000, 0, 0, 0, 0, "FIXED holds each note for LEN instead, with up to KEYS notes at once"},
};
/* city pop: 108 BPM, D major; an 808 and a slap-ish bass written in STEP, two patterns each (pattern 2 busier: the
 * clip switches to it), tine chords on CHORD, strings */
static void g_citypop_scene(void)
{
    static const uint8_t D1[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_SD, 0, L_CH, 0};
    static const uint8_t D2[16] = {L_BD, 0, L_CH, L_CH, L_SD, 0, L_CH, L_BD, L_BD, 0, L_CH, L_BD, L_SD, L_SD, L_OH, L_SD};
    static const uint8_t B1[16] = {38, 0, 0, 50, 0, 0, 38, 0, 43, 0, 0, 55, 0, 0, 43, 0};
    static const uint8_t B2[16] = {38, 50, 38, 0, 45, 0, 38, 50, 43, 55, 43, 0, 47, 45, 43, 40};
    song.g[G_BPM] = 108;
    scale(2, 1, 0);                                     /* D, MAJ */
    sends(75, 60, 65, 45);
    sound(0, ENGI_DRUM, DK_808_KIT);
    sound(1, ENGI_FM6, FX_SOLID_BASS);
    for (uint32_t pat = 0; pat < 2u; pat++) {
        for (uint32_t i = 0; i < 32u; i++) {
            const uint8_t *D = pat ? D2 : D1, *B = pat ? B2 : B1;
            step_hits(0, i, D[i % 16u], D[i % 16u] ? (uint8_t)(i % 4u ? 70 : 100) : 0);
            if (B[i % 16u]) step_note(1, i, B[i % 16u], 96);
        }
        trk[0].p[P_SLEN] = 32; trk[1].p[P_SLEN] = 32;
        if (!pat) { pattern_switch(&trk[0], 1); pattern_switch(&trk[1], 1); }
    }
    pattern_switch(&trk[0], 0); pattern_switch(&trk[1], 0);
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(0, 88, 0, 12, 0);
    mixer(1, 90, 0, 0, 10);
    sound(2, ENGI_FM6, FX_TINE_EP);
    trk[2].p[P_CHRD] = 2;
    gen(2, TESS_ENG_CHORD, 19);
    tx(2, TX_CH_PROG, GEN_CH_251); tx(2, TX_CH_BARS, 1); tx(2, TX_CH_HIT, GEN_CH_OFF);
    mixer(2, 74, 20, 50, 30);
    sound(3, ENGI_TRIO, TR_STRING_MACH);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 6);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 56, -25, 80, 0);
    g_solo_check();
}
static const gstep_t G_PATTERNS[] = {
    {0, 0, 0, 0, 0, "Each track has 8 patterns, 8 pages of its own steps"},
    {3500, 6500, 900, "SEQ", "PATTERNS", "Hold SEQ: PATTERNS, a row per track, a cell per pattern"},
    {11000, 14000, 600, "KNOB 1", "TRACK 1: 2", "Turn KNOB 1: track 1's next pattern is 2. Dashed: it waits", 1},
    {18200, 0, 0, 0, 0, "The pattern playing ends, and it switches: busier drums"},
    {21500, 24500, 600, "KNOB 2", "TRACK 2: 2", "Turn KNOB 2: the bass queues its pattern 2 too", 1},
    {27100, 0, 0, 0, 0, "On its line it switches: a new bass part"},
    {32000, 0, 0, 0, 0, "JAM logs what you played. On SONG, the next page, take it as a song"},
};
/* exotica, darker: 96 BPM, D harmonic minor; track 1 a koto pluck on MARKOV in eighths (LEAP 0 to start), a
 * soft vibraphone-like tine on CHORD, a sub on EUCLID, bongos from the 909's toms with a rim in STEP */
static void g_exotica_scene(void)
{
    song.g[G_BPM] = 96;
    song.g[G_SWING] = 22;
    scale(2, 7, 1);                                     /* D HARM (harmonic minor), QNT SNAP */
    sends(100, 75, 70, 45);
    sound(0, ENGI_FM6, FX_KOTO_PLUCK);                  /* (Theo: pluckier, more exotic, minor) */
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_MARKOV, 57);
    tx(0, TX_MKV_LEAP, 0); tx(0, TX_MKV_DENS, 90); tx(0, TX_MKV_SPAN, 1); tx(0, TX_TW_SHIFT, 12);
    {   /* a melody on its STEP page, unheard until the clip turns SOURCE to it (OCT- layer) */
        static const uint8_t N[16] = {50, 0, 51, 54, 0, 57, 0, 54, 51, 0, 50, 0, 49, 0, 50, 0};   /* (+12: D4) */
        for (uint32_t i = 0; i < 16u; i++) if (N[i]) step_note(0, i, N[i], i % 4u ? 84 : 104);
        trk[0].p[P_SLEN] = 16;
    }
    mixer(0, 108, 10, 45, 40);
    sound(1, ENGI_FM6, FX_TINE);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 32);
    tx(1, TX_CH_PROG, GEN_CH_251); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_BEAT); tx(1, TX_CH_OCT, -1);
    mixer(1, 70, -25, 55, 20);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_EUCLID, 3);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 4); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 72, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, L_TM, L_TM, L_RS, 0, L_TM, 0, L_TM, 0, L_TM, L_TM, L_RS, 0, L_TM, L_TM};
        static const uint8_t V[16] = {70, 0, 46, 54, 50, 0, 60, 0, 66, 0, 46, 54, 50, 0, 58, 44};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 86, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_MARKOV[] = {
    {0, 0, 0, 0, 0, "MARKOV walks between notes of the scale: mostly a step, sometimes a leap"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. Track 1's koto is MARKOV. LEAP 0: mostly small steps"},
    {14000, 17000, 2500, "KNOB 2", "LEAP 80", "Turn KNOB 2: LEAP up to 80 %. Now the koto jumps", 20},
    {22000, 0, 0, 0, 0, "The bars are the odds of each next note: they level out, so wide leaps come often"},
    {29000, 32000, 2500, "KNOB 3", "DENS", "Turn KNOB 3: DENS down to 50 %, the chance a step plays", -8},
    {41000, 44000, 600, "KNOB 4", "SPAN 2", "Turn KNOB 4: SPAN 2OCT. Each note picks one of two octaves (3OCT reaches higher)", 1},
    {53000, 56000, 20000, "OCT-", "HOLD OCT-", "Hold OCT-: SOURCE, KEEP and GRAVITY, as on TURING and EUCLID"},
    {0, 58000, 300, "KNOB 2", "SOURCE T1", 0, 1},
    {58500, 0, 0, 0, 0, "KNOB 2, SOURCE T1: MARKOV learns what follows what in track 1's written melody"},
    {66000, 0, 0, 0, 0, "LEAP is now the chance of a jump to anywhere in that melody"},
    {78000, 0, 0, 0, 0, "LEAP, DENS and SPAN; SOURCE, KEEP and GRAVITY"},
};
/* Afro-Cuban: 104 BPM, A dorian; track 1 a marimba on EUCLID (3 of 8, the tresillo, at sixteenths), electric piano
 * chords on CHORD in the offbeats, a sub on TURING, a 909 in STEP: soft kick, rim clave, cascara hat, tom fill */
static void g_afrocuban_scene(void)
{
    song.g[G_BPM] = 104;
    scale(9, SCL_DOR, 1);                               /* A DOR, QNT SNAP */
    sends(70, 55, 55, 35);
    sound(0, ENGI_FM6, V_MARIMBA);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_EUCLID, 35);
    tx(0, TX_EUC_STEPS, 8); tx(0, TX_EUC_PULSES, 3); tx(0, TX_EUC_ROT, 0);
    {   /* a melody on its STEP page, unheard until the clip turns SOURCE to it (OCT- layer) */
        static const uint8_t N[16] = {69, 0, 72, 0, 74, 76, 0, 74, 72, 0, 69, 0, 67, 0, 69, 0};
        for (uint32_t i = 0; i < 16u; i++) if (N[i]) step_note(0, i, N[i], i % 4u ? 84 : 104);
        trk[0].p[P_SLEN] = 16;
    }
    mixer(0, 120, 15, 30, 30);
    sound(1, ENGI_FM6, FX_TINE_EP);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 4);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_OFF);
    mixer(1, 76, -25, 35, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 16);
    tx(2, TX_TUR_PROB, 15); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 45);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 82, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        /* (Theo: the drums went unheard. Now a soft kick, the son clave on the rim, cascara on the hat, toms at the end) */
        static const uint8_t L[16] = {L_BD | L_RS | L_CH, 0, L_CH, L_RS | L_CH, L_CH, 0, L_RS | L_CH, 0,
                                      L_BD | L_CH, 0, L_RS | L_CH, L_CH, L_RS, L_CH, L_TM, L_TM};
        static const uint8_t V[16] = {96, 0, 50, 72, 56, 0, 74, 0, 88, 0, 72, 54, 70, 50, 62, 70};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 86, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_EUCLID[] = {
    {0, 0, 0, 0, 0, "EUCLID spreads a number of hits over the steps, as evenly as they go"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. Track 1's marimba: 3 hits in 8 steps, the tresillo"},
    {12000, 15000, 600, "KNOB 3", "PULSES 5", "Turn KNOB 3: PULSES 3 to 5, five hits in the 8", 2},
    {22000, 25000, 600, "KNOB 4", "ROT 2", "Turn KNOB 4: ROT 2, the same hits two steps later", 2},
    {32000, 35000, 600, "KNOB 2", "STEPS 12", "Turn KNOB 2: STEPS 8 to 12, five hits in 12 steps now", 4},
    {43000, 0, 0, 0, 0, "Against the 16 steps of the drums the pattern shifts round, bar by bar"},
    {50000, 53000, 24000, "OCT-", "HOLD OCT-", "Hold OCT-: SOURCE, KEEP and GRAVITY, to play a melody you wrote"},
    {0, 55000, 300, "KNOB 2", "SOURCE T1", 0, 1},
    {55500, 0, 0, 0, 0, "KNOB 2, SOURCE T1: the melody on track 1's STEP page, in order, on EUCLID's hits"},
    {0, 63000, 300, "KNOB 3", "RHYTHM", 0, 1},
    {63500, 0, 0, 0, 0, "KNOB 3, KEEP RHYTHM: the melody's own rhythm instead, on EUCLID's pitches"},
    {0, 70000, 300, "KNOB 3", "FREE", 0, -1},
    {0, 71000, 2000, "KNOB 4", "GRAVITY", 0, 40},
    {71000, 0, 0, 0, 0, "KNOB 4, GRAVITY: how often a step plays the melody's own hit or rest"},
    {80000, 0, 0, 0, 0, "STEPS, PULSES and ROT; SOURCE, KEEP and GRAVITY"},
};
/* highlife: 112 BPM, C major; track 1 a guitar-like pluck written in STEP (16 sixteenths, with rests) that STEP EVOLVE
 * varies, organ chords on the offbeats on CHORD, a walking sub on TURING, a 909 with shaker-like hats in STEP */
static void g_highlife_scene(void)
{
    static const uint8_t N[16] = {72, 0, 76, 79, 0, 76, 74, 0, 72, 0, 69, 72, 0, 67, 69, 0};
    song.g[G_BPM] = 112;
    scale(0, 1, 1);                                     /* C MAJ, QNT SNAP */
    sends(70, 55, 65, 40);
    sound(0, ENGI_TRIO, TR_SOFT_LEAD);                  /* (Theo: a softer pluck, dry) */
    trk[0].p[P_E5] = 52; trk[0].p[P_ATK] = 0; trk[0].p[P_DEC] = 52; trk[0].p[P_SUS] = 0; trk[0].p[P_REL] = 40;
    for (uint32_t i = 0; i < 16u; i++) if (N[i]) step_note(0, i, N[i], i % 4u ? 80 : 100);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_16; trk[0].p[P_SLEN] = 16;
    mixer(0, 127, 15, 0, 20);
    sound(1, ENGI_FM6, FX_ORGAN);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 6);
    tx(1, TX_CH_PROG, GEN_CH_POP); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_OFF);
    mixer(1, 88, -25, 40, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 24);
    tx(2, TX_TUR_PROB, 25); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 30);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 76, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, L_CH, L_CH, L_CH, L_RS, L_CH, L_CH, L_BD, L_CH, L_CH, L_BD, L_CH, L_RS, L_CH, L_CH, L_CH};
        static const uint8_t V[16] = {84, 30, 44, 30, 66, 30, 44, 60, 30, 30, 74, 30, 66, 30, 44, 34};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 72, 0, 10, 0);
    g_solo_check();
}
static const gstep_t G_SEVOLVE[] = {
    {0, 0, 0, 0, 0, "Track 1's soft pluck is written in STEP. EVOLVE can vary it as it plays"},
    {3500, 6500, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE, here STEP EVOLVE"},
    {10500, 0, 0, 0, 0, "EVERY 1BAR: a round of changes each bar. AMOUNT is OFF: none yet"},
    {17500, 20500, 2500, "KNOB 3", "AMOUNT", "Turn KNOB 3: AMOUNT, how many steps change (here 40 %)", 20},
    {27000, 0, 0, 0, 0, "Notes move, drop out or swap; the steps as written stay"},
    {35000, 38000, 4500, "OCT-", "HOW", "Hold OCT- and turn KNOB 4: HOW SERIAL, the whole phrase at once"},
    {0, 39500, 1000, "KNOB 4", "SERIAL", 0, 2},
    {45000, 0, 0, 0, 0, "Backwards, upside down, rotated or half at half speed: the screen says which"},
    {58000, 61000, 900, "KNOB 4", "CLEAR", "Turn KNOB 4: CLEAR, back to the steps as written", 1},
    {66000, 0, 0, 0, 0, "Like a variation? Hold REC to keep it as the steps"},
};
/* ambient folk: 88 BPM, A minor pentatonic; track 1 a plucked melody written in STEP that TURING plays from (SOURCE T1), a breathy
 * soft strings on CHORD, a sub on EUCLID, a light 808 in STEP */
static void g_ambfolk_scene(void)
{
    static const uint8_t N[8] = {69, 72, 76, 74, 72, 69, 67, 64};
    song.g[G_BPM] = 88;
    scale(9, 6, 1);                                     /* A MPEN (minor pentatonic), QNT SNAP */
    sends(95, 75, 70, 45);
    sound(0, ENGI_FM6, V_PLUCK);
    for (uint32_t i = 0; i < 8u; i++) step_note(0, i, N[i], i % 4u ? 84 : 104);
    trk[0].p[P_SLEN] = 8;
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 66);
    tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_LEN, 8);
    mixer(0, 116, 10, 0, 25);                           /* (Theo: no reverb on the tune) */
    sound(1, ENGI_ORACLE, OR_SOFT_STRINGS);             /* (Theo: the flute chord sounded like a hum) */
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 8);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 56, -25, 60, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_EUCLID, 11);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 74, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, 0, L_RS, 0, L_CH, 0, 0, 0, L_BD, 0, L_RS, 0, L_CH, 0};
        static const uint8_t V[16] = {80, 0, 0, 0, 56, 0, 36, 0, 0, 0, 66, 0, 56, 0, 40, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 86, 0, 25, 0);
    g_solo_check();
}
static const gstep_t G_SEED[] = {
    {0, 0, 0, 0, 0, "Track 1 is TURING, and has a tune written in STEP too"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. TURING plays a loop of its own for now"},
    {11000, 14000, 4500, "OCT-", "SOURCE", "Hold OCT- and turn KNOB 2: SOURCE T1, the tune"},
    {0, 15500, 300, "KNOB 2", "SOURCE T1", 0, 1},
    {21000, 0, 0, 0, 0, "PROB is LOCK: TURING plays the tune as written"},
    {28000, 31000, 3000, "KNOB 3", "PROB", "Turn KNOB 3: PROB up. Each pass a few steps change, and stay", 15},
    {42000, 45000, 5000, "OCT-", "GRAVITY", "Hold OCT- and turn KNOB 4: GRAVITY. More steps keep to the tune"},
    {0, 46500, 3000, "KNOB 4", "GRAVITY", 0, 40},
    {57000, 0, 0, 0, 0, "The tune itself never changes. KEEP can hold its rhythm or its notes"},
};
/* future garage: 132 BPM, C# minor, a shuffled 2-step; an ORACLE pad on CHORD as track 1 whose filter follows BUS A,
 * which the bass's GATE feeds (cord 1: T3 GATE -> BUS A, cord 2: BUS A -> T1 CUTOFF), a tine line, a sub, an 808 */
static void g_fgarage_scene(void)
{
    song.g[G_BPM] = 132;
    song.g[G_SWING] = 28;
    scale(1, SCL_MIN, 1);                               /* C# MIN, QNT SNAP */
    sends(95, 75, 70, 50);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 2;
    gen(0, TESS_ENG_CHORD, 21);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    cord(0, CS_GATE, 2, CD_BUS_A, 0, 100, 0, 0);
    cord(1, CS_BUSA, 0, CD_CUTOFF, 0, 70, 0, 0);
    mixer(0, 92, 0, 50, 0);
    sound(1, ENGI_FM6, V_TINE);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_TURING, 52);
    tx(1, TX_TUR_PROB, 8); tx(1, TX_TW_DROP, 70);
    mixer(1, 56, 25, 60, 45);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 37);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 2); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 86, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_CH, 0, 0, L_BD | L_CH, 0, L_SD, 0, L_CH, L_RS};
        static const uint8_t V[16] = {92, 0, 40, 0, 82, 0, 44, 30, 0, 0, 80, 0, 82, 0, 46, 40};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 15, 0);
    g_solo_check();
}
static const gstep_t G_BUSES[] = {
    {0, 0, 0, 0, 0, "Two MIX buses: mods go into one, and the bus is a source of its own"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS. Cord 1: the bass's GATE into BUS A"},
    {12500, 0, 0, 0, 0, "Cord 2: BUS A onto the pad's filter. It opens while a bass note sounds"},
    {19500, 22500, 900, "OCT+", "SHEET", "Hold OCT+: MODS' sheet, the buses' Mode and Lag"},
    {26500, 29500, 600, "KNOB 1", "BUS A LAG", "Turn KNOB 1 to BUS A LAG, now OFF", 2},
    {33500, 36500, 2000, "KNOB 2", "LAG", "Turn KNOB 2: 500 MS of lag. The filter now swells and falls", 6},
    {46000, 49000, 200, "OCT-", "CLOSE", "Tap OCT-: the sheet closes"},
    {55000, 0, 0, 0, 0, "One bus can feed as many mods as you like"},
};
/* new age: 76 BPM, E major; an ORACLE poly line on TURING as track 1 whose filter its LFO moves a little (cord 1, +10 %)
 * and SWARM 5 deepens and lightens (cord 2: T1 SWARM 5 -> M1 AMT, +70 %), a glass pad on CHORD, a sub, a soft rim */
static void g_newage_scene(void)
{
    song.g[G_BPM] = 76;
    scale(4, 1, 1);                                     /* E MAJ, QNT SNAP */
    sends(115, 85, 75, 55);
    sound(0, ENGI_ORACLE, V_OR_FAT_POLY);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 44);
    tx(0, TX_TUR_PROB, 10); tx(0, TX_TUR_LEN, 8); tx(0, TX_TW_DROP, 30);
    trk[0].p[P_LRATE] = 45;
    cord(0, CS_LFO, 0, CD_CUTOFF, 0, 10, 0, 0);
    cord(1, CS_OCHD5, 0, CD_M1_AMT, 0, 70, 0, 0);
    mixer(0, 92, 10, 55, 40);
    sound(1, ENGI_FM6, V_GLASS_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 9);
    tx(1, TX_CH_PROG, GEN_CH_POP); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 50, -25, 85, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_EUCLID, 13);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 82, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {0, 0, 0, 0, L_RS, 0, 0, 0, 0, 0, L_RS, 0, L_RS, 0, 0, 0};
        static const uint8_t V[16] = {0, 0, 0, 0, 56, 0, 0, 0, 0, 0, 36, 0, 50, 0, 0, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 62, 0, 50, 0);
    g_solo_check();
}
static const gstep_t G_MODMOD[] = {
    {0, 0, 0, 0, 0, "A mod can move another mod's amount: a mod of a mod"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS. Cord 1, T1 LFO onto T1 CUT, is set to only +10 %"},
    {13000, 0, 0, 0, 0, "Cord 2 moves cord 1's amount: T1 SWARM 5 onto M1 AMT (M1%)"},
    {19500, 22500, 12000, "OCT-", "SCOPE", "Hold OCT- and tap OCT+: cord 1 on the scope"},
    {0, 24500, 200, "OCT+", "PIN", 0},
    {29000, 0, 0, 0, 0, "Its swing grows and shrinks as SWARM 5 wanders: the filter breathes"},
    {40000, 43000, 200, "OCT-", "FREE", "Tap OCT-: the bar under cord 1 shows its amount as it is now"},
    {50000, 0, 0, 0, 0, "M1 AMT to M16 AMT are at the end of KNOB 3's list"},
};
/* minimal house: 122 BPM, G minor; an ORACLE pad on CHORD as track 1 that ducks under the bass (T3 GATE -> T1 GAIN), its
 * filter on its LFO, a filtered soft lead on TURING whose DRIFT pans the hats (T2 DRIFT -> T4 PAN), a sub on EUCLID, a 909 */
static void g_minhouse_scene(void)
{
    song.g[G_BPM] = 122;
    scale(7, SCL_MIN, 1);                               /* G MIN, QNT SNAP */
    sends(80, 65, 70, 45);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 2;
    gen(0, TESS_ENG_CHORD, 15);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    trk[0].p[P_LRATE] = 25;
    cord(0, CS_LFO, 0, CD_CUTOFF, 0, 40, 0, 0);
    cord(1, CS_GATE, 2, CD_GAIN, 0, -45, 0, 0);
    cord(2, CS_DRIFT, 1, CD_PAN, 3, 50, 0, 0);
    mixer(0, 80, 0, 45, 0);
    sound(1, ENGI_TRIO, TR_SOFT_LEAD);
    trk[1].p[P_E5] = 40; trk[1].p[P_E6] = 30;           /* (Theo: a more filtered lead) */
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 47);
    tx(1, TX_TUR_PROB, 10); tx(1, TX_TW_DROP, 55);
    mixer(1, 90, 25, 50, 40);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 29);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 1); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 80, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_CH, L_BD, 0, L_OH, 0, L_BD, 0, L_CH, L_CH, L_BD | L_CP, 0, L_OH, L_CH};
        static const uint8_t V[16] = {90, 0, 40, 30, 88, 0, 50, 0, 90, 0, 40, 30, 88, 0, 50, 34};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 72, 0, 12, 0);
    g_solo_check();
}
static const gstep_t G_MATRIX[] = {
    {0, 0, 0, 0, 0, "MATRIX: which track moves which, at a glance"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS, three cords"},
    {10000, 13000, 600, "SELECT", "MATRIX", "Turn SELECT: MATRIX. FROM down the left, TO along the top", 1},
    {17500, 0, 0, 0, 0, "A lit cell: a cord goes that way, brighter the more it moves now"},
    {24000, 27000, 600, "KNOB 1", "FROM T3", "Turn KNOB 1 to FROM T3, the bass. TO is T1, the pad", 2},
    {31500, 0, 0, 0, 0, "That cell holds one cord: MOD 2"},
    {37000, 40000, 200, "OCT+", "OPEN", "Tap OCT+: it opens in MODS. T3 GATE onto T1 GAIN, -45 %"},
    {47000, 0, 0, 0, 0, "While a bass note sounds, the pad dips"},
};
/* space ambient: 70 BPM, D lydian; an ORACLE glass-like pad holding chords as track 1, its filter and pan on SWARM
 * (cords: T1 SWARM 3 -> T1 CUTOFF, T1 SWARM 6 -> T1 PAN), a tine line on TURING, a deep sub on EUCLID, no drums but a
 * soft rim now and then */
static void g_space_scene(void)
{
    song.g[G_BPM] = 70;
    scale(2, 9, 1);                                     /* D LYD, QNT SNAP */
    sends(120, 90, 80, 60);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 2;
    gen(0, TESS_ENG_CHORD, 27);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    cord(0, CS_OCHD3, 0, CD_CUTOFF, 0, 55, 0, 0);
    cord(1, CS_OCHD6, 0, CD_PAN, 0, 60, 0, 0);
    mixer(0, 100, 0, 70, 0);
    sound(1, ENGI_FM6, V_TINE);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 19);
    tx(1, TX_TUR_PROB, 10); tx(1, TX_TW_DROP, 65);
    mixer(1, 58, 25, 70, 50);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_EUCLID, 5);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 2); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 82, 0, 10, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {0, 0, 0, 0, L_RS, 0, 0, 0, 0, 0, 0, 0, L_RS, 0, 0, L_RS};
        static const uint8_t V[16] = {0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 44, 0, 0, 30};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 60, 0, 60, 0);
    g_solo_check();
}
static const gstep_t G_ORGANIC[] = {
    {0, 0, 0, 0, 0, "TIDE and SWARM: slow movement that never quite repeats"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS. Two cords from SWARM: the pad's filter, and its pan"},
    {11500, 14500, 9000, "OCT-", "SCOPE", "Hold OCT- and tap OCT+: cord 1 on the scope, wandering"},
    {0, 16500, 200, "OCT+", "PIN", 0},
    {26500, 29500, 200, "OCT-", "FREE", "Tap OCT-: the rows again"},
    {33000, 36000, 1500, "SELECT", "ORGANIC", "Turn SELECT to ORGANIC: KNOB 1 TIDE's speed, KNOB 2 SWARM's", 4},
    {41000, 44000, 4000, "KNOB 2", "RATE 75", "Turn KNOB 2: RATE 50 to 75, four times as fast", 25},
    {52000, 0, 0, 0, 0, "The bars are the eight SWARMs. The filter and the pan move faster now"},
    {60000, 0, 0, 0, 0, "PLAY starts them again from the track's seed, so a piece replays the same"},
};
/* detroit techno: 124 BPM, A minor; a TRIO lead on TURING in sixteenths as track 1 (its filter gets the shortcut mod),
 * a string machine on CHORD, a sub on EUCLID, a 909 in STEP */
static void g_detroit_scene(void)
{
    song.g[G_BPM] = 124;
    scale(9, SCL_MIN, 1);                               /* A MIN, QNT SNAP */
    sends(80, 65, 70, 50);
    sound(0, ENGI_TRIO, TR_ARP_LEAD);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_TURING, 62);
    tx(0, TX_TUR_PROB, 8); tx(0, TX_TUR_LEN, 16); tx(0, TX_TUR_SPAN, 1);
    trk[0].p[P_LRATE] = 35;
    mixer(0, 92, 10, 35, 40);
    sound(1, ENGI_TRIO, TR_STRING_MACH);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 12);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 50, -25, 80, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 23);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 1); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 86, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_OH, 0, L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_OH, L_CH};
        static const uint8_t V[16] = {90, 0, 44, 0, 88, 0, 54, 0, 90, 0, 44, 0, 88, 0, 54, 34};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 12, 0);
    g_solo_check();
}
static const gstep_t G_FXMOD[] = {
    {0, 0, 0, 0, 0, "The quickest mod: hold FX and turn the knob you want to move"},
    {3500, 6500, 200, "EDIT", "OPEN EDIT", "Tap EDIT: track 1's sound, a TRIO lead"},
    {10000, 13000, 600, "SELECT", "TONE", "Turn SELECT: TONE, the filter. KNOB 2 is CUT", 1},
    {17000, 20000, 6500, "FX", "HOLD FX", "Hold FX and turn KNOB 2: a cord, LFO to CUT. The turn is how far"},
    {0, 21500, 4000, "KNOB 2", "AMOUNT", 0, 30},
    {30000, 0, 0, 0, 0, "Let go: the filter now opens and closes with the LFO"},
    {37000, 40000, 200, "LFO", "MODS", "Tap LFO: MODS. The shortcut's cord is there, to edit like any other"},
    {47000, 0, 0, 0, 0, "Another source than the LFO: with FX held, turn SELECT first"},
};
/* nu-soul: 86 BPM, F minor; ORACLE warm pad holding a chord as track 1 with a cord (T1 LFO -> T1 CUTOFF, +60 %), a
 * tine line on TURING, a sub on EUCLID, a laid-back 808 in STEP */
static void g_nusoul_scene(void)
{
    song.g[G_BPM] = 86;
    song.g[G_SWING] = 24;
    scale(5, SCL_MIN, 1);                               /* F MIN, QNT SNAP */
    sends(90, 70, 65, 45);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 2;
    gen(0, TESS_ENG_CHORD, 26);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_HOLD);
    trk[0].p[P_LRATE] = 30;
    cord(0, CS_LFO, 0, CD_CUTOFF, 0, 60, 0, 0);
    mixer(0, 100, 0, 55, 0);
    sound(1, ENGI_FM6, FX_TINE_EP);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 41);
    tx(1, TX_TUR_PROB, 12); tx(1, TX_TW_DROP, 55);
    mixer(1, 66, 25, 55, 35);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 17);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 88, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, L_BD, L_CH, 0, L_SD, 0, L_CH, L_CH};
        static const uint8_t V[16] = {92, 0, 42, 0, 78, 0, 46, 56, 0, 74, 42, 0, 80, 0, 48, 34};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 15, 0);
    g_solo_check();
}
static const gstep_t G_SHAPES[] = {
    {0, 0, 0, 0, 0, "A cord's shape sets how its source moves what it moves"},
    {3500, 6500, 200, "LFO", "OPEN MODS", "Tap LFO: MODS. Cord 1 is T1 LFO onto T1 CUTOFF, the pad's filter"},
    {11000, 14000, 5000, "OCT-", "SCOPE", "Hold OCT- and tap OCT+: the scope, pinned. The line is the mod"},
    {0, 16000, 200, "OCT+", "PIN", 0},
    {22000, 25000, 200, "OCT+", "+", "Tap OCT+: + (LFO+), one way only from the knob"},
    {33000, 36000, 200, "OCT+", "GATE", "Again: GATE (LFO^), all or nothing"},
    {44000, 47000, 200, "OCT+", "HOLD", "Again: HOLD (LFO=), taken once a bar and held"},
    {55000, 58000, 200, "OCT+", "+-", "Again: back to +-, both ways"},
    {63000, 66000, 200, "OCT-", "FREE", "Tap OCT-: SCOPE FREE, the rows come back"},
};
/* post-rock: 90 BPM, E minor; track 1 a STEP line climbing the pentatonic in eighths on a ringing tine, a soft pad on
 * CHORD, a sub on TURING, brushed 909 in STEP */
static void g_postrock_scene(void)
{
    static const uint8_t N[8] = {64, 67, 69, 71, 74, 76, 79, 81};
    song.g[G_BPM] = 90;
    scale(4, SCL_MIN, 1);                               /* E MIN, QNT SNAP */
    sends(110, 80, 80, 55);
    sound(0, ENGI_FM6, V_TINE);
    for (uint32_t i = 0; i < 8u; i++) step_note(0, i, N[i], i ? 84 : 104);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_8; trk[0].p[P_SLEN] = 8;
    mixer(0, 112, 10, 60, 50);
    sound(1, ENGI_TRIO, TR_SOFT_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 7);
    tx(1, TX_CH_PROG, GEN_CH_POP); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 54, -25, 85, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 33);
    tx(2, TX_TUR_PROB, 20); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 40);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 84, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, 0, L_RS, 0, 0, L_CH, L_BD, 0, L_BD, 0, L_RS, 0, L_CH, 0};
        static const uint8_t V[16] = {80, 0, 0, 0, 62, 0, 0, 40, 70, 0, 60, 0, 62, 0, 44, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 70, 0, 25, 0);
    g_solo_check();
}
static const gstep_t G_SHEET[] = {
    {0, 0, 0, 0, 0, "Track 1 climbs eight notes. Its sheet can play them in another order"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN, track 1's pattern"},
    {10000, 13000, 900, "OCT+", "SHEET", "Hold OCT+: the pattern's sheet"},
    {16500, 19500, 1500, "KNOB 1", "DIRECTION", "Turn KNOB 1 to DIRECTION, now FWD: step 1 to 8", 6},
    {24500, 27500, 900, "KNOB 2", "PEND", "Turn KNOB 2 to PEND: up, then back down", 2},
    {37000, 40000, 300, "KNOB 2", "FOLD", "One more: FOLD, from the ends inwards (1 8 2 7 3 6 4 5)", 1},
    {50000, 0, 0, 0, 0, "Each of the track's 8 patterns keeps its own Direction"},
    {56000, 59000, 200, "OCT-", "BACK", "Tap OCT-: back to GEN"},
};
/* ambient house: 112 BPM, E minor; track 1 an empty STEP pattern (two bars of eighths) on a pluck, recorded live in the
 * clip; a glass pad on CHORD, a sub on EUCLID, a soft 909 four on the floor in STEP */
static void g_ambhouse_scene(void)
{
    song.g[G_BPM] = 112;
    scale(4, SCL_MIN, 1);                               /* E MIN, QNT SNAP */
    sends(105, 80, 70, 45);
    sound(0, ENGI_FM6, V_PLUCK);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_8; trk[0].p[P_SLEN] = 16;
    mixer(0, 118, 10, 50, 40);
    sound(1, ENGI_FM6, V_GLASS_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 19);
    tx(1, TX_CH_PROG, GEN_CH_POP); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 52, -25, 85, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 27);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 1); tx(2, TX_TW_SHIFT, -12);
    trk[2].p[P_QUANT] = Q_CHRD;
    mixer(2, 84, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD, 0, L_OH, 0, L_BD, 0, L_CH, 0, L_BD, 0, L_OH, L_CH};
        static const uint8_t V[16] = {86, 0, 40, 0, 84, 0, 50, 0, 86, 0, 40, 0, 84, 0, 50, 30};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 70, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_REC[] = {
    {0, 0, 0, 0, 0, "Record the keys live, while it all plays. Track 1 is empty"},
    {3500, 6500, 200, "REC", "ARM", "Tap REC: track 1 WAITS FOR LOOP, its pattern's start, then records"},
    {11500, 14500, 250, "E4", "PLAY", "Play: what you play goes into the pattern"},
    {0, 15040, 250, "G4", "PLAY", 0},
    {0, 15580, 500, "B4", "PLAY", 0},
    {0, 16650, 250, "A4", "PLAY", 0},
    {0, 17190, 500, "G4", "PLAY", 0},
    {20600, 23620, 250, "E5", "PLAY", "Round again: add more on top"},
    {0, 25760, 250, "D5", "PLAY", 0},
    {0, 26830, 400, "B4", "PLAY", 0},
    {31000, 34000, 200, "REC", "STOP REC", "Tap REC again: the take ends"},
    {40000, 0, 0, 0, 0, "Both passes play back together, round and round"},
};
/* lounge: 100 BPM, D dorian; track 1 an empty STEP pattern (8 eighths) on a flute, written on the keys in the clip;
 * ORACLE keys on CHORD (two chords, on the beat), a sub on TURING, a light 909 in STEP */
static void g_lounge_scene(void)
{
    song.g[G_BPM] = 100;
    song.g[G_SWING] = 20;
    scale(2, SCL_DOR, 1);                               /* D DOR, QNT SNAP */
    sends(80, 65, 60, 40);
    sound(0, ENGI_FM6, V_FLUTE);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_SDIV] = DIV_8; trk[0].p[P_SLEN] = 8;
    mixer(0, 120, 10, 45, 35);
    sound(1, ENGI_ORACLE, V_OR_KEYS);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 13);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_BEAT);
    mixer(1, 56, -25, 55, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 21);
    tx(2, TX_TUR_PROB, 20); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 50);
    mixer(2, 86, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_RS, 0, L_CH, L_CH, L_BD, 0, L_CH, L_BD, L_RS, 0, L_CH, 0};
        static const uint8_t V[16] = {84, 0, 46, 0, 64, 0, 50, 34, 74, 0, 46, 56, 64, 0, 52, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 72, 0, 15, 0);
    g_solo_check();
}
static const gstep_t G_STEPW[] = {
    {0, 0, 0, 0, 0, "Track 1, a flute, is an empty STEP pattern: write it on NOTES"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN, the pattern's page"},
    {10000, 13000, 600, "SELECT", "NOTES", "Turn SELECT: NOTES. The cursor is on step 1", 1},
    {17000, 20000, 300, "A4", "WRITE", "Press a key: A4 goes on step 1, and the cursor moves on"},
    {24000, 27000, 300, "C5", "WRITE", "C5 on step 2"},
    {29500, 32500, 900, "KNOB 1", "SKIP", "Turn KNOB 1 past a step to leave a rest", 1},
    {36000, 39000, 300, "D5", "WRITE", "D5, E5, and C5"},
    {0, 40000, 300, "E5", "WRITE", 0},
    {0, 41000, 300, "C5", "WRITE", 0},
    {45000, 48000, 900, "KNOB 1", "PICK STEP", "Turn KNOB 1 back to step 1, A4", -6},
    {52000, 55000, 900, "KNOB 2", "PITCH", "Turn KNOB 2 down: a scale note a detent, A4 to F4", -2},
    {60000, 0, 0, 0, 0, "The loop plays what you wrote, round and round"},
};
/* ethno-ambient: 84 BPM, C; a harp line on TURING as track 1, a warm ORACLE pad on a CHORD drone, a sparse sub on
 * EUCLID, hand drums from the 808's toms and rim in STEP. The scale becomes USER1, made on the keys */
static void g_ethno_scene(void)
{
    song.g[G_BPM] = 84;
    scale(0, 1, 1);                                     /* C MAJ, QNT SNAP */
    sends(110, 80, 75, 50);
    sound(0, ENGI_FM6, V_HARP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 53);
    tx(0, TX_TUR_PROB, 12); tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_SPAN, 1);
    mixer(0, 118, 15, 55, 35);
    sound(1, ENGI_ORACLE, OR_WARM_PAD);
    trk[1].p[P_CHRD] = 0;
    gen(1, TESS_ENG_CHORD, 31);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 2); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 54, -25, 85, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 14);
    tx(2, TX_TUR_PROB, 15); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 60);
    mixer(2, 86, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, 0, L_RS, 0, 0, L_TM, 0, L_TM, 0, L_RS, 0, 0, L_TM, 0, L_RS};
        static const uint8_t V[16] = {90, 0, 0, 56, 0, 0, 70, 0, 80, 0, 60, 0, 0, 66, 0, 48};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 25, 0);
    g_solo_check();
}
static const gstep_t G_USCALE[] = {
    {0, 0, 0, 0, 0, "Four scales of your own, USER1 to USER4, made on the keys"},
    {3500, 6500, 200, "SEL", "OPEN SEL", "Tap SEL: the scales"},
    {10000, 13000, 3000, "KNOB 1", "USER", "Turn KNOB 1 to USER, the last family", 10},
    {19000, 0, 0, 0, 0, "USER1 plays now. Never set, it is the major scale"},
    {25000, 28000, 300, "D4", "D OUT", "Tap D, a note of the scale: it goes out (USER1 D OUT)"},
    {32000, 35000, 300, "C#4", "C# IN", "Tap C#, not in the scale: it comes in"},
    {39000, 42000, 300, "A4", "A OUT", "A out"},
    {44500, 47500, 300, "G#4", "G# IN", "G# in"},
    {51000, 0, 0, 0, 0, "Every track now plays C C# E F G G# B, the root still C"},
    {58000, 61000, 200, "HOME", "HOME", "Tap HOME. USER1 stays on the FM-1, for every project"},
};
/* chillwave: 96 BPM, A minor; soft electric keys with no generator as track 1 (played on the keys; dry, Theo: no
 * reverb, softer), a warm pad on CHORD round the pop progression (a chord a bar, an octave down: the low end too),
 * track 3 a muted pluck on EUCLID that FOLLOWs track 2 with HOW HARM, an LFO on its SHIFT (the clip unmutes it), an
 * 808 in STEP */
static void g_chillwave_scene(void)
{
    song.g[G_BPM] = 96;
    scale(9, SCL_MIN, 1);                               /* A MIN, QNT SNAP */
    sends(100, 75, 70, 45);
    tx(0, TX_QNT_LINK, 0);                              /* QNT link OWN: only track 1's QNT changes */
    sound(0, ENGI_ORACLE, OR_KEYS);
    trk[0].p[P_E0] = -28;
    tx(0, TX_MODE, TESS_ENG_OFF);
    mixer(0, 100, 15, 0, 20);
    sound(1, ENGI_TRIO, TR_WARM_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 24);
    tx(1, TX_CH_PROG, GEN_CH_POP); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 62, -25, 60, 0);
    sound(2, ENGI_TRIO, TR_SOFT_LEAD);
    trk[2].p[P_E5] = 72;
    trk[2].p[P_ATK] = 0; trk[2].p[P_DEC] = 45; trk[2].p[P_SUS] = 0; trk[2].p[P_REL] = 35;
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 9);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 0);
    tx(2, TX_TW_FOLLOW, 2); tx(2, TX_TW_FHOW, TESS_FH_HARM);   /* (FOLLOW T2, HOW HARM: SHIFT counts chord notes) */
    trk[2].p[P_LRATE] = 30;
    cord(0, CS_LFO, 2, CD_TW_SHIFT, 2, 14, 0, 0);
    trk[2].p[P_MUTE] = 1;
    mixer(2, 112, -20, 10, 45);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, L_BD, L_CH, 0, L_SD, 0, L_CH, L_CH};
        static const uint8_t V[16] = {96, 0, 44, 0, 80, 0, 48, 60, 0, 70, 44, 0, 80, 0, 50, 36};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 76, 0, 15, 0);
    g_solo_check();
}
#define G_RUN6(t, lb) \
    {0, (t), 250, "C4", lb, 0}, {0, (t) + 320, 250, "D4", lb, 0}, {0, (t) + 640, 250, "E4", lb, 0}, \
    {0, (t) + 960, 250, "F4", lb, 0}, {0, (t) + 1280, 250, "G4", lb, 0}, {0, (t) + 1600, 600, "A4", lb, 0}
static const gstep_t G_HARM[] = {
    {0, 0, 0, 0, 0, "Track 2 plays chords. Track 1, soft keys, is ours to play"},
    {3500, 0, 0, 0, 0, "Play the white keys up from C4: the scale"},
    G_RUN6(6200, "SCALE"),
    {10500, 0, 0, 0, 0, "Next: the same run, counted into the chords"},
    {13500, 16500, 200, "SEL", "OPEN SEL", "Tap SEL: the scales"},
    {20000, 23000, 600, "SELECT", "SCL", "Turn SELECT: SCL, the key's settings", 1},
    {27000, 30000, 900, "KNOB 1", "PICK QNT", "Turn KNOB 1 to the QUANTIZE row (now SNAP)", 3},
    {34500, 37500, 1900, "KNOB 2", "QNT HARM", "Turn KNOB 2 to HARM", 5},
    {42000, 45000, 200, "HOME", "HOME", "Tap HOME"},
    {48500, 0, 0, 0, 0, "The same run: each step up is the chord's next note up"},
    G_RUN6(51200, "ARPEGGIO"),
    {55000, 0, 0, 0, 0, "Over another chord, another arpeggio"},
    G_RUN6(58600, "ARPEGGIO"),
    {62500, 0, 0, 0, 0, "Any line in the scale does the same: a generator, or MIDI IN"},
    {69000, 72000, 200, "GLO", "MIXER", "Track 3 FOLLOWs track 2, the chords, with HOW HARM. Tap GLO: MIXER"},
    {0, 74000, 200, "A#3", "UNMUTE T3", 0},
    {75000, 0, 0, 0, 0, "A#3 brings it in: the chord's note, counted into the chord, in its own rhythm"},
    {82000, 85000, 200, "HOME", "HOME", "Tap HOME"},
    {0, 87000, 600, "ALGORITHM", "TRACK 3", 0, 2},
    {0, 89000, 200, "ARP", "EVOLVE", 0},
    {0, 91000, 900, "SELECT", "TWEAK", 0, 2},
    {92000, 0, 0, 0, 0, "Its TWEAK page: SHIFT at +0, FOLLOW T2"},
    {97000, 100000, 200, "LFO", "OPEN MODS", "Tap LFO: MODS. A cord, T3 LFO onto T3 SHIFT"},
    {104000, 107000, 7000, "OCT-", "SCOPE", "Hold OCT-: the scope. The LFO swings SHIFT up and down"},
    {115000, 0, 0, 0, 0, "Under HARM, SHIFT counts the chord's notes: the line climbs and falls inside the chord"},
    {122000, 0, 0, 0, 0, "QNT HARM on the keys; FOLLOW's HOW HARM, and SHIFT by chord notes"},
};
/* reggae: 76 BPM (a one drop), G major; drums and bass written in STEP, two patterns each (pattern 2 a variation),
 * organ skanks on CHORD in the offbeats, a melodica-like flute on TURING */
static void g_reggae_scene(void)
{
    static const uint8_t D1[16] = {0, 0, L_CH, 0, L_CH, 0, L_CH, 0, L_BD | L_RS, 0, L_CH, 0, L_CH, 0, L_CH, 0};
    static const uint8_t D2[16] = {0, 0, L_CH, 0, L_CH, 0, L_CH, L_RS, L_BD | L_RS, 0, L_CH, 0, L_CH, L_RS, L_OH, L_RS};
    static const uint8_t B1[16] = {43, 0, 0, 0, 0, 0, 47, 50, 0, 0, 0, 0, 45, 0, 0, 0};
    static const uint8_t B2[16] = {40, 0, 0, 0, 0, 0, 43, 45, 0, 0, 47, 0, 48, 0, 47, 45};
    song.g[G_BPM] = 76;
    scale(7, 1, 0);                                     /* G, MAJ */
    sends(70, 65, 90, 60);
    sound(0, ENGI_DRUM, DK_808_KIT);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    for (uint32_t pat = 0; pat < 2u; pat++) {
        for (uint32_t i = 0; i < 32u; i++) {
            const uint8_t *D = pat ? D2 : D1, *B = pat ? B2 : B1;
            step_hits(0, i, D[i % 16u], D[i % 16u] ? (uint8_t)(D[i % 16u] & L_BD ? 104 : 66) : 0);
            if (B[i % 16u]) step_note(1, i, B[i % 16u], 100);
        }
        trk[0].p[P_SLEN] = 32; trk[1].p[P_SLEN] = 32;
        if (!pat) { pattern_switch(&trk[0], 1); pattern_switch(&trk[1], 1); }
    }
    pattern_switch(&trk[0], 0); pattern_switch(&trk[1], 0);
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(0, 90, 0, 20, 20);
    mixer(1, 96, 0, 0, 0);
    sound(2, ENGI_FM6, FX_ORGAN);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 23);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_OFF);
    mixer(2, 66, -20, 40, 60);
    sound(3, ENGI_FM6, V_FLUTE);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 34);
    tx(3, TX_TUR_PROB, 15); tx(3, TX_TW_DROP, 60);
    mixer(3, 74, 25, 50, 70);
    g_solo_check();
}
static const gstep_t G_SONG[] = {
    {0, 0, 0, 0, 0, "SONG lines up sets of patterns, one per track, into a song"},
    {3500, 6500, 900, "SEQ", "PATTERNS", "Hold SEQ: PATTERNS"},
    {10000, 13000, 600, "SELECT", "SONG", "Turn SELECT: SONG, empty for now", 1},
    {16500, 19500, 600, "KNOB 3", "ADD A", "Turn KNOB 3 on +: a first section, A, from the patterns playing", 1},
    {23500, 26500, 600, "KNOB 4", "A x2", "Turn KNOB 4: A plays twice", 1},
    {30000, 33000, 600, "KNOB 1", "TO +", "Turn KNOB 1 to the + after it", 1},
    {36500, 39500, 600, "KNOB 3", "ADD", "Turn KNOB 3: a second section, a copy of A", 1},
    {43000, 46000, 600, "KNOB 2", "TRACK 2", "Turn KNOB 2 to track 2, the bass", 1},
    {49500, 52500, 600, "KNOB 3", "B", "Turn KNOB 3: its pattern 2. The section is now B", 1},
    {56000, 59000, 200, "OCT+", "STOP", "Tap OCT+ twice: the first stops what plays, the second plays the song"},
    {0, 61000, 200, "OCT+", "PLAY SONG", 0},
    {65000, 0, 0, 0, 0, "Section A, twice"},
    {73600, 0, 0, 0, 0, "Section B: the bass plays its pattern 2"},
    {81000, 0, 0, 0, 0, "After its last section the song stops, and the patterns are as they were"},
};
static void guide_ref(const char *out, FILE *tsv)
{
    G_RUN("r-landscape", "A SEA TO WATCH", "MOSAIC", 72, 4, g_neoclassical_scene, G_LANDSCAPE);   /* neo-classical */
    G_RUN("r-color", "THE SCREEN'S COLOUR", "YELLOW", 54, 4, g_chillout_scene, G_COLOR);   /* chillout */
    G_RUN("r-form", "A TRACK AS A SONG", "SUNSET", 60, 4, g_bossa_scene, G_FORM);   /* bossa nova */
    G_RUN("r-entries", "TRACKS OUT AND IN", "SUNSET", 60, 4, g_dnb_scene, G_ENTRIES);   /* liquid drum and bass */
    G_RUN("r-chain", "ONE FORM FOR ALL", "SUNSET", 64, 4, g_disco_scene, G_CHAIN);   /* disco */
    G_RUN("r-energy", "ENERGY BY HAND", "SUNSET", 54, 4, g_afrohouse_scene, G_ENERGY);   /* afro house */
    G_RUN("r-tweak", "RESHAPE A LINE", "GREEN", 46, 4, g_folktronica_scene, G_TWEAK);   /* folktronica */
    G_RUN("r-follow", "ONE TRACK FOLLOWS ANOTHER", "MOSAIC", 56, 4, g_cinematic_scene, G_FOLLOW);   /* cinematic */
    G_RUN("r-timing", "ROLLS AND FEEL", "GREEN", 58, 4, g_boombap_scene, G_TIMING);   /* boom bap */
    G_RUN("r-ratchet", "ROLL ONE STEP", "GREEN", 67, 4, g_chilltrap_scene, G_RATCHET);   /* chill trap */
    G_RUN("r-length", "LET NOTES RING", "GREEN", 42, 4, g_dreampop_scene, G_LENGTH);   /* dream pop */
    G_RUN("r-patterns", "PATTERNS", "MOSAIC", 41, 4, g_citypop_scene, G_PATTERNS);   /* city pop */
    G_RUN("r-markov", "A WANDERING LINE: MARKOV", "MOSAIC", 84, 4, g_exotica_scene, G_MARKOV);   /* exotica */
    G_RUN("r-euclid", "EVEN HITS: EUCLID", "MOSAIC", 86, 4, g_afrocuban_scene, G_EUCLID);   /* Afro-Cuban */
    G_RUN("r-step-evolve", "VARY WHAT YOU WROTE", "MOSAIC", 72, 4, g_highlife_scene, G_SEVOLVE);   /* highlife */
    G_RUN("r-seed", "YOUR TUNE AS A SEED", "MOSAIC", 66, 4, g_ambfolk_scene, G_SEED);   /* ambient folk */
    G_RUN("r-buses", "MIX BUSES", "LILAC", 62, 4, g_fgarage_scene, G_BUSES);   /* future garage */
    G_RUN("r-mod-of-mod", "A MOD OF A MOD", "LILAC", 58, 4, g_newage_scene, G_MODMOD);   /* new age */
    G_RUN("r-matrix", "WHO MOVES WHAT", "LILAC", 55, 4, g_minhouse_scene, G_MATRIX);   /* minimal house */
    G_RUN("r-organic", "SLOW, LIVING MOVEMENT", "LILAC", 68, 4, g_space_scene, G_ORGANIC);   /* space ambient */
    G_RUN("r-fx-mod", "THE MOD SHORTCUT", "LILAC", 52, 4, g_detroit_scene, G_FXMOD);   /* detroit techno */
    G_RUN("r-mod-shapes", "A CORD'S SHAPE", "LILAC", 72, 4, g_nusoul_scene, G_SHAPES);   /* nu-soul */
    G_RUN("r-sheet", "PLAY THE STEPS IN ANOTHER ORDER", "MOSAIC", 66, 4, g_postrock_scene, G_SHEET);   /* post-rock */
    G_RUN("r-rec", "RECORD LIVE", "MOSAIC", 50, 4, g_ambhouse_scene, G_REC);   /* ambient house */
    G_RUN("r-step-write", "WRITE NOTES BY HAND", "MOSAIC", 68, 4, g_lounge_scene, G_STEPW);   /* lounge */
    G_RUN("r-user-scale", "A SCALE OF YOUR OWN", "MOSAIC", 70, 4, g_ethno_scene, G_USCALE);   /* ethno-ambient */
    G_RUN("r-harm", "RUN THE KEYS INTO CHORDS", "MOSAIC", 130, 4, g_chillwave_scene, G_HARM);   /* chillwave */
    G_RUN("r-song", "A SONG FROM PATTERNS", "MOSAIC", 91, 4, g_reggae_scene, G_SONG);   /* reggae */
    G_RUN("r-piece", "A PIECE WITH A SHAPE", "SUNSET", 116, 4, g_ambtechno_scene, G_PIECE);   /* ambient techno */
    G_RUN("r-condition", "EVERY OTHER TIME", "MOSAIC", 64, 4, g_downtempo_scene, G_COND);   /* downtempo */
    G_RUN("r-mod-first", "YOUR FIRST MOD", "LILAC", 65, 4, g_triphop_scene, G_MOD_FIRST);   /* trip-hop */
}
