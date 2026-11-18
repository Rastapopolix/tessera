/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 1: you can't break it: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* Season 1's own music (PLAN.md: a style a clip, new rather than the demos). Nothing automatic (no ENTRIES, PIECE or
 * EVOLVE EVERY), so a clip shows only what its keys do. */
/* ambient: an ORACLE pad holding D dorian's chord, a bell wandering on AWAKE, a sub on two pulses, a kalimba now and
 * then. 64 BPM */
static void g_ambient_scene(void)
{
    song.g[G_BPM] = 64;
    scale(2, SCL_DOR, 0);
    sends(110, 80, 60, 55);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 1;                               /* DIA3 */
    gen(0, TESS_ENG_CHORD, 5);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 4); tx(0, TX_CH_HIT, GEN_CH_HOLD); tx(0, TX_CH_OCT, -1);
    mixer(0, 84, -15, 90, 0);
    sound(1, ENGI_FM6, FX_RIN);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_AWAKE, 31);
    tx(1, TX_AWK_LENA, 7); tx(1, TX_AWK_LENB, 5); tx(1, TX_AWK_PROB, 40);
    mixer(1, 92, 20, 100, 60);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 12);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 2); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 96, 0, 10, 0);
    sound(3, ENGI_FM6, FX_KALIMBA);                     /* (a kalimba now and then, high) */
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_TURING, 44);
    tx(3, TX_TUR_PROB, 10); tx(3, TX_TW_SHIFT, 7);
    mixer(3, 70, 30, 100, 70);
}
/* half-step: 70 BPM, the beat at half time (a kick, a snare on the third beat, eighth hats), a sub on EUCLID, a dark
 * pad and a bell line on TURING. F minor */
static void g_halfstep_scene(void)
{
    song.g[G_BPM] = 70;
    scale(5, SCL_MIN, 0);
    sends(85, 60, 75, 50);
    sound(0, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_CH, 0, L_CH, L_BD, L_SD, 0, L_CH, 0, L_CH, L_BD, L_OH, 0};
        static const uint8_t V[16] = {108, 0, 66, 0, 72, 0, 60, 70, 104, 0, 66, 0, 72, 64, 70, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(0, i, L[i], V[i]);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    mixer(0, 100, 0, 10, 0);
    sound(1, ENGI_TRIO, TR_SUB_BASS);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 9);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 3); tx(1, TX_EUC_ROT, 0); tx(1, TX_TW_SHIFT, -12);
    tx(1, TX_NL_MODE, 1); tx(1, TX_NL_LEN, 2);
    mixer(1, 104, 0, 0, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 3);
    tx(2, TX_CH_PROG, GEN_CH_DRONE); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 70, -20, 80, 0);
    sound(3, ENGI_FM6, FX_RIN);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 17);
    tx(3, TX_TUR_PROB, 15);
    mixer(3, 84, 25, 70, 50);
}
/* Season 1, episode 1: find your way. STILL TIDE, a minute in */
static const gstep_t G_PAGES[] = {
    {0, 0, 0, 0, 0, "Buttons open pages. You can't break anything by looking"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: the GEN page, what this track plays (here CHORD)"},
    {11000, 14000, 1200, "SELECT", "NEXT PAGE", "Turn SELECT: the next page under the same button (PATTERN)", 1},
    {19000, 22000, 200, "ARP", "OPEN ARP", "Tap ARP: how the notes are shaped (EVOLVE first)"},
    {27000, 30000, 200, "HOME", "GO HOME", "Tap HOME: back to the four tracks, from anywhere"},
    {35000, 0, 0, 0, 0, "Away from HOME, the lit tab at the foot of the screen names the page"},
};
/* Season 1, episode 1: back out of anything */
static const gstep_t G_BACK[] = {
    {0, 0, 0, 0, 0, "OCT- is the way back. Try it whenever you are lost"},
    {3500, 6500, 200, "GLO", "OPEN MIXER", "Tap GLO: the MIXER page"},
    {11000, 14000, 800, "OCT+", "OPEN SHEET", "Hold OCT+: the page's sheet opens (here MIXER's MUTES setting)"},
    {19000, 22000, 200, "OCT-", "CLOSE", "Tap OCT-: the sheet closes, nothing changed"},
    {27000, 30000, 200, "HOME", "GO HOME", "Tap HOME: back to the four tracks"},
    {35000, 0, 0, 0, 0, "HOME and OCT- always get you back"},
};
/* Season 1, episode 1: PLAY plays it the same way each time */
static const gstep_t G_PLAY[] = {
    {0, 0, 0, 0, 0, "Listen to how this opens"},
    {7000, 10000, 200, "PLAY", "STOP", "Press PLAY: it stops"},
    {13000, 16000, 200, "PLAY", "PLAY", "Press PLAY again: the same opening, note for note"},
    {27000, 0, 0, 0, 0, "Every generator starts again from its seed, so you can always get back to it"},
};

/* drone: 56 BPM, E minor; an ORACLE pad holding the chord as track 1, a TRIO fifth held under it, wind chimes on
 * AWAKE, a breath flute now and then */
static void g_drone_scene(void)
{
    song.g[G_BPM] = 56;
    scale(4, SCL_MIN, 0);
    sends(120, 90, 50, 60);
    sound(0, ENGI_ORACLE, OR_WARM_PAD);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 13);
    tx(0, TX_CH_PROG, GEN_CH_DRONE); tx(0, TX_CH_BARS, 3); tx(0, TX_CH_HIT, GEN_CH_HOLD); tx(0, TX_CH_OCT, -1);
    mixer(0, 86, -10, 90, 0);
    sound(1, ENGI_TRIO, TR_FIFTH_DRONE);
    gen(1, TESS_ENG_CHORD, 2);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -2);
    mixer(1, 74, 0, 40, 0);
    sound(2, ENGI_FM6, FX_FURIN);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_AWAKE, 57);
    tx(2, TX_AWK_LENA, 5); tx(2, TX_AWK_LENB, 3); tx(2, TX_AWK_PROB, 30);
    mixer(2, 80, 25, 100, 60);
    sound(3, ENGI_TRIO, TR_BREATH_FLUTE);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_TURING, 77);
    tx(3, TX_TUR_PROB, 10); tx(3, TX_TW_DROP, 75);
    mixer(3, 70, -25, 90, 40);
}
static void guide_e01(const char *out, FILE *tsv)
{
    G_RUN("e01-pages", "FIND YOUR WAY", "YELLOW", 43, 8, g_drone_scene, G_PAGES);    /* drone */
    G_RUN("e01-back", "GET BACK", "YELLOW", 43, 0, g_ambient_scene, G_BACK);          /* ambient */
    G_RUN("e01-play", "THE SAME EACH TIME", "YELLOW", 36, 0, g_halfstep_scene, G_PLAY); /* half-step */
}
