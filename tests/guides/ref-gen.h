/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The reference clips, one per generator (MANUAL "The generators"): tests/demo_projects.c guide; tools/guide_video.py.
 * r-euclid, r-markov and r-gen-layer are in ref.h. (g_solo_check, the balance and note checks, is in e05.h) */

/* kankyo ongaku (Japanese environmental music): 72 BPM, D major pentatonic; BOUNCE on a soft tine as track 1 (the clip
 * turns its SPEED, BALLS and FALL), a warm pad holding the chord, a low flute on AWAKE, a sub on two pulses */
static void g_kankyo_scene(void)
{
    song.g[G_BPM] = 72;
    scale(2, 5, 0);                                     /* D, PEN */
    sends(120, 90, 85, 70);
    sound(0, ENGI_FM6, FX_DX_TINE);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_BOUNCE, 33);
    tx(0, TX_BNC_SPEED, 12); tx(0, TX_BNC_ANGLE, 37); tx(0, TX_BNC_BALLS, 1); tx(0, TX_TW_SHIFT, 12);
    mixer(0, 104, -15, 90, 50);
    sound(1, ENGI_TRIO, TR_WARM_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 6);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 64, -5, 100, 0);
    sound(2, ENGI_TRIO, TR_BREATH_FLUTE);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_AWAKE, 19);
    tx(2, TX_AWK_LENA, 7); tx(2, TX_AWK_LENB, 4); tx(2, TX_AWK_PROB, 35);
    mixer(2, 76, 30, 100, 60);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_EUCLID, 2);
    tx(3, TX_EUC_STEPS, 8); tx(3, TX_EUC_PULSES, 2); tx(3, TX_EUC_ROT, 0); tx(3, TX_TW_SHIFT, -12);
    mixer(3, 76, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_BOUNCE[] = {
    {0, 0, 0, 0, 0, "BOUNCE: a ball in a box plays a note where it hits a wall"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: the box, and the ball moving"},
    {11500, 14500, 1200, "KNOB 2", "SPEED", "Turn KNOB 2, SPEED, up: more hits, more notes", 12},
    {20500, 23500, 800, "KNOB 4", "BALLS 3", "Turn KNOB 4, BALLS, to 3: each from its own place", 2},
    {30000, 33000, 800, "KNOB 4", "FALL", "Past 4 balls: FALL, eight leaves landing (left low, right high)", 2},
    {40000, 43000, 1200, "KNOB 3", "WIND", "In FALL, KNOB 3 is WIND: it leans the melody low or high", -12},
    {50000, 0, 0, 0, 0, "Sparse, chime-like lines that never quite repeat"},
};

/* chamber jazz in the ECM way: 84 BPM, A dorian; AWAKE on a felt-like electric piano as track 1 (the clip turns its two
 * loops and PROB), a soft upright-ish bass on TURING in quarters, brushes in STEP, a string pad held */
static void g_chamberjazz_scene(void)
{
    song.g[G_BPM] = 84;
    song.g[G_SWING] = 22;
    scale(9, SCL_DOR, 0);
    sends(110, 85, 70, 50);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_AWAKE, 52);
    tx(0, TX_AWK_LENA, 8); tx(0, TX_AWK_LENB, 8); tx(0, TX_AWK_PROB, 100);
    mixer(0, 100, -15, 70, 20);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_4;
    gen(1, TESS_ENG_TURING, 18);
    tx(1, TX_TUR_LEN, 8); tx(1, TX_TUR_PROB, 10); tx(1, TX_TW_SHIFT, -12);
    mixer(1, 84, 0, 10, 0);
    sound(2, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_CH, 0, L_CH, L_CH, L_RS, 0, L_CH, L_CH, L_CH, 0, L_CH, L_CH, L_RS, 0, L_CH, 0};
        static const uint8_t V[16] = {80, 0, 62, 54, 84, 0, 64, 54, 78, 0, 62, 54, 86, 0, 66, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(2, i, L[i], V[i]);
        trk[2].p[P_SLEN] = 16;
    }
    tx(2, TX_MODE, TESS_ENG_OFF); trk[2].p[P_SDIV] = DIV_16;
    mixer(2, 120, 15, 30, 0);
    sound(3, ENGI_ORACLE, OR_SOFT_STRINGS);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 4);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 58, 25, 90, 0);
    g_solo_check();
}
static const gstep_t G_AWAKE[] = {
    {0, 0, 0, 0, 0, "AWAKE: two loops. A is the melody, B lifts it up the scale"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: LEN A and LEN B are both 8"},
    {11500, 14500, 800, "KNOB 3", "LEN B 5", "Turn KNOB 3, LEN B, to 5: B now shifts against A", -3},
    {21000, 0, 0, 0, 0, "The same eight notes come back lifted differently each pass"},
    {29000, 32000, 1500, "KNOB 4", "PROB", "Turn KNOB 4, PROB, down: rests open up", -12},
    {38000, 41000, 600, "KNOB 2", "LEN A 6", "Turn KNOB 2, LEN A, to 6: a shorter melody", -2},
    {47000, 0, 0, 0, 0, "On GEN, the white keys pick AWAKE's steps to set by hand"},
};

/* exotica: 104 BPM, F lydian; POLY on vibes-like marimba as track 1 (the clip adds a third rhythm), a bongo-like
 * figure in STEP, a walking bass on TURING, a lounge organ holding the chord */
static void g_exotica_scene(void)
{
    song.g[G_BPM] = 104;
    scale(5, 9, 0);                                     /* F, LYD */
    sends(100, 80, 80, 50);
    sound(0, ENGI_FM6, V_DX_MARIMBA);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_POLY, 26);
    tx(0, TX_POL_DIV1, 4); tx(0, TX_POL_DIV2, 3); tx(0, TX_POL_DIV3, 0); tx(0, TX_POL_DIV4, 0); tx(0, TX_POL_SPAN, 2);
    mixer(0, 120, -10, 60, 40);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, 0, L_TM, 0, 0, L_RS, 0, L_TM, 0, L_TM, 0, 0, 0, L_RS, 0};
        static const uint8_t V[16] = {80, 0, 0, 60, 0, 0, 56, 0, 76, 0, 58, 0, 0, 0, 54, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 90, 20, 30, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 47);
    tx(2, TX_TUR_LEN, 8); tx(2, TX_TUR_PROB, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 72, 0, 5, 0);
    sound(3, ENGI_FM6, FX_ORGAN);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 10);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 80, -25, 70, 0);
    g_solo_check();
}
static const gstep_t G_POLY[] = {
    {0, 0, 0, 0, 0, "POLY: four notes stepped on by four rhythms"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: RHY 1 is /4, RHY 2 is /3: a polyrhythm"},
    {12000, 15000, 800, "KNOB 2", "RHY 3", "Turn KNOB 2, RHY, to 3: KNOB 3 now edits rhythm 3", 2},
    {21000, 24000, 1000, "KNOB 3", "/5", "Turn KNOB 3, DIVISION, from OFF to /5: a third rhythm", 5},
    {31000, 0, 0, 0, 0, "Where rhythms land together, the note plays once, louder"},
    {39000, 42000, 1000, "KNOB 3", "OFF", "Turn it back to OFF: two rhythms again", -5},
    {48000, 0, 0, 0, 0, "/4 against /3, against /5: the bars line up only now and then"},
};
/* math rock: 132 BPM, E minor; QUAD on a clean pluck as track 1, an electric piano and a sub on the same grid (rows 2
 * and 3, at their own speeds), drums in STEP over 14 sixteenths (7/8); the clip turns DIR and a column's MOVE */
static void g_mathrock_scene(void)
{
    song.g[G_BPM] = 132;
    scale(4, SCL_MIN, 0);
    sends(90, 70, 80, 50);
    sound(0, ENGI_FM6, V_PLUCK);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_QUAD, 21);
    tx(0, TX_QD_DIR, GEN_QD_FWD); tx(0, TX_QD_PROB, 100);
    mixer(0, 110, -20, 40, 30);
    sound(1, ENGI_FM6, FX_TINE_EP);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_QUAD, 21);
    tx(1, TX_QD_DIR, GEN_QD_FWD); tx(1, TX_QD_PROB, 100);
    mixer(1, 80, 25, 50, 20);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_QUAD, 21);
    tx(2, TX_QD_DIR, GEN_QD_FWD); tx(2, TX_QD_PROB, 100); tx(2, TX_QD_RANGE, GEN_QR_ONE); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 80, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[14] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, L_CH, L_BD, 0, L_SD, L_CH};
        static const uint8_t V[14] = {104, 0, 50, 0, 92, 0, 48, 86, 0, 50, 90, 0, 94, 46};
        for (uint32_t i = 0; i < 14u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 14;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 80, 0, 10, 0);
    g_solo_check();
}
static const gstep_t G_QUAD[] = {
    {0, 0, 0, 0, 0, "QUAD: a 4 x 4 grid of notes, a row for each QUAD track"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: tracks 1 to 3 play rows 1 to 3, each at its own speed"},
    {12000, 15000, 800, "KNOB 2", "PEND", "Turn KNOB 2, DIR, to PEND: row 1 goes back and forth", 2},
    {21000, 24000, 3500, "OCT-", "HOLD OCT-", "Hold OCT- and turn KNOB 3, MOVE, to DOWN"},
    {0, 25500, 400, "KNOB 3", "MOVE DOWN", 0, 1},
    {30000, 0, 0, 0, 0, "Every 4 steps column 1 turns: its notes move down a track"},
    {38000, 41000, 1200, "KNOB 3", "PROB", "Turn KNOB 3, PROB, down: some cells rest", -15},
    {48000, 0, 0, 0, 0, "One grid, three tracks: the tune moves between them"},
};
/* library sci-fi (the 1970s library records): 96 BPM, A minor; CONSTL on a soft lead as track 1 (the clip moves the
 * crosshair, adds stars and turns LOCK), a glass pad holding the chord, a round bass on EUCLID, soft toms in STEP */
static void g_scifi_scene(void)
{
    song.g[G_BPM] = 96;
    scale(9, SCL_MIN, 0);
    sends(110, 100, 80, 60);
    sound(0, ENGI_TRIO, TR_SOFT_LEAD);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_CONSTEL, 14);
    tx(0, TX_CS_Y, 5); tx(0, TX_CS_DENS, 40); tx(0, TX_CS_SIZE, 2);
    mixer(0, 98, -15, 90, 60);
    sound(1, ENGI_ORACLE, OR_GLASS_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 9);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 70, 20, 90, 0);
    sound(2, ENGI_FM6, FX_SOLID_BASS);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_EUCLID, 5);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 104, 0, 10, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, 0, L_TM, 0, 0, L_CH, L_BD, 0, L_TM, 0, 0, 0, L_CH, 0};
        static const uint8_t V[16] = {96, 0, 0, 0, 70, 0, 0, 44, 90, 0, 62, 0, 0, 0, 46, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 76, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_CONSTL[] = {
    {0, 0, 0, 0, 0, "CONSTL: a ship flies through stars; the ones it crosses play"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: the sky passes, the crosshair catches stars"},
    {12000, 15000, 1000, "KNOB 2", "Y UP", "Turn KNOB 2, Y, up: the crosshair rises, and the tune", 5},
    {21000, 24000, 1200, "KNOB 3", "DENS", "Turn KNOB 3, DENS, up: more stars, more notes", 15},
    {30000, 33000, 4000, "OCT-", "HOLD OCT-", "Hold OCT- and turn KNOB 2, LOCK, up"},
    {0, 34500, 1500, "KNOB 2", "LOCK", 0, 30},
    {40000, 0, 0, 0, 0, "LOCK plays back the stars already caught: a phrase repeats"},
    {48000, 0, 0, 0, 0, "Low LOCK roams, high LOCK repeats: the knob between them"},
};
/* slow piano after Feldman: 66 BPM, E minor; SERIES on soft electric keys in eighths as track 1 (the clip turns INTV
 * and LOOP), a soft pad holding one chord, a low fifth drone, nothing else */
static void g_feldman_scene(void)
{
    song.g[G_BPM] = 66;
    scale(4, SCL_MIN, 0);
    sends(120, 100, 80, 40);
    sound(0, ENGI_ORACLE, OR_KEYS);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_SERIES, 3);
    tx(0, TX_SR_START, 0); tx(0, TX_SR_INTV, 1); tx(0, TX_SR_LOOP, 0);
    mixer(0, 94, -10, 100, 30);
    sound(1, ENGI_TRIO, TR_SOFT_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 8);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 64, 20, 100, 0);
    sound(2, ENGI_TRIO, TR_FIFTH_DRONE);
    trk[2].p[P_SDIV] = DIV_1;
    gen(2, TESS_ENG_EUCLID, 1);
    tx(2, TX_EUC_STEPS, 2); tx(2, TX_EUC_PULSES, 1); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 84, -20, 80, 0);
    tx(3, TX_MODE, TESS_ENG_OFF);
    mixer(3, 0, 0, 0, 0);
    g_solo_check();
}
static const gstep_t G_SERIES[] = {
    {0, 0, 0, 0, 0, "SERIES: a melody alike at every scale, after Per Nørgård"},
    {4000, 7000, 200, "SEQ", "OPEN SEQ", "Tap SEQ: LOOP is OFF, so it never repeats"},
    {12000, 15000, 800, "KNOB 3", "INTV 3", "Turn KNOB 3, INTV, to 3: each step now 3 scale notes", 2},
    {22000, 0, 0, 0, 0, "The same shape, in wider leaps"},
    {29000, 32000, 1000, "KNOB 4", "LOOP 4", "Turn KNOB 4, LOOP, to 4: the first 4 steps over and over", 4},
    {39000, 42000, 1000, "KNOB 4", "LOOP OFF", "Back to OFF: it unfolds again", -4},
    {48000, 0, 0, 0, 0, "Nothing random: the same START always plays the same tune"},
};
static void guide_ref_gen(const char *out, FILE *tsv)
{
    G_RUN("r-bounce", "BOUNCE AND FALL", "GREEN", 57, 4, g_kankyo_scene, G_BOUNCE);   /* kankyo ongaku */
    G_RUN("r-awake", "AWAKE: TWO LOOPS", "GREEN", 55, 4, g_chamberjazz_scene, G_AWAKE);   /* chamber jazz */
    G_RUN("r-poly", "POLY: FOUR RHYTHMS", "GREEN", 55, 4, g_exotica_scene, G_POLY);   /* exotica */
    G_RUN("r-quad", "QUAD: ONE GRID", "GREEN", 55, 4, g_mathrock_scene, G_QUAD);   /* math rock */
    G_RUN("r-constellation", "CONSTELLATION", "GREEN", 55, 4, g_scifi_scene, G_CONSTL);   /* library sci-fi */
    G_RUN("r-series", "SERIES", "GREEN", 55, 4, g_feldman_scene, G_SERIES);   /* slow piano after Feldman */
}
