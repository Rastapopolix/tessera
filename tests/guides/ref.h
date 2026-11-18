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
 * guitar on CHORD in the offbeats, a bass on EUCLID, rim and shaker on a 909 */
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
    mixer(0, 100, 15, 50, 30);
    sound(1, ENGI_FM6, V_PLUCK);
    trk[1].p[P_CHRD] = 2;
    gen(1, TESS_ENG_CHORD, 7);
    tx(1, TX_CH_PROG, GEN_CH_251); tx(1, TX_CH_BARS, 1); tx(1, TX_CH_HIT, GEN_CH_OFF);
    mixer(1, 76, -20, 40, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 16);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 92, 0, 0, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_RS, L_CH, 0, L_RS | L_CH, 0, L_BD, 0, L_CH, L_RS, L_CH, 0, L_RS | L_CH, 0};
        static const uint8_t V[16] = {80, 0, 50, 70, 46, 0, 72, 0, 76, 0, 50, 70, 46, 0, 72, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 74, 0, 15, 0);
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
static void guide_ref(const char *out, FILE *tsv)
{
    G_RUN("r-landscape", "A SEA TO WATCH", "MOSAIC", 72, 4, g_neoclassical_scene, G_LANDSCAPE);   /* neo-classical */
    G_RUN("r-color", "THE SCREEN'S COLOUR", "YELLOW", 54, 4, g_chillout_scene, G_COLOR);   /* chillout */
    G_RUN("r-form", "A TRACK AS A SONG", "SUNSET", 60, 4, g_bossa_scene, G_FORM);   /* bossa nova */
    G_RUN("r-entries", "TRACKS OUT AND IN", "SUNSET", 60, 4, g_dnb_scene, G_ENTRIES);   /* liquid drum and bass */
    G_RUN("r-piece", "A PIECE WITH A SHAPE", "SUNSET", 116, 4, g_ambtechno_scene, G_PIECE);   /* ambient techno */
    G_RUN("r-condition", "EVERY OTHER TIME", "MOSAIC", 64, 4, g_downtempo_scene, G_COND);   /* downtempo */
    G_RUN("r-mod-first", "YOUR FIRST MOD", "LILAC", 65, 4, g_triphop_scene, G_MOD_FIRST);   /* trip-hop */
}
