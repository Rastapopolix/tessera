/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 8: save and come back to it: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* (g_solo_check, the balance and note checks, is in e01.h) */

/* synth-pop: 104 BPM, A dorian; an electric piano line of sixteenths on TURING as track 1, changing every bar (the clip
 * prints it), a light 808 pop beat in STEP, a warm pad on CHORD every two bars, a round bass on EUCLID
 * (Theo: the earlier techno blips did not suit the clip) */
static void g_synthpop_scene(void)
{
    song.g[G_BPM] = 104;
    scale(9, SCL_DOR, 0);
    sends(85, 70, 90, 55);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_16;                          /* (sixteenths: a print counts 16 steps a bar) */
    gen(0, TESS_ENG_TURING, 23);
    tx(0, TX_TUR_LEN, 16); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 2);
    tx(0, TX_EV_EVERY, 1); tx(0, TX_EV_AMOUNT, 50);
    mixer(0, 100, 10, 45, 50);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_CP, 0, L_CH, L_BD, 0, L_BD, L_CH, 0, L_CP, 0, L_CH, 0};
        static const uint8_t V[16] = {104, 0, 50, 0, 92, 0, 46, 70, 0, 84, 50, 0, 94, 0, 54, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 86, 0, 15, 0);
    sound(2, ENGI_ORACLE, OR_WARM_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 5);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_OFF);
    mixer(2, 88, -20, 60, 20);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    trk[3].p[P_SDIV] = DIV_16;
    gen(3, TESS_ENG_EUCLID, 11);
    tx(3, TX_EUC_STEPS, 16); tx(3, TX_EUC_PULSES, 5); tx(3, TX_EUC_ROT, 0); tx(3, TX_TW_SHIFT, -12);
    tx(3, TX_NL_MODE, 1); tx(3, TX_NL_LEN, 2);
    mixer(3, 74, 0, 0, 0);
    g_solo_check();
}
/* Season 1, episode 8: print a generator's bars into steps, and undo it */
static const gstep_t G_PRINT[] = {
    {0, 0, 0, 0, 0, "Track 1's generator changes its line every bar"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: MODE is TURING"},
    {10500, 13500, 1200, "REC", "PRINT", "Hold REC: its last bars become steps (PRINTED 4 BARS)"},
    {19000, 0, 0, 0, 0, "MODE is STEP: those 4 bars now repeat as they are"},
    {28000, 31000, 1200, "EDIT", "UNDO", "Changed your mind? Hold EDIT: UNDO T1"},
    {36000, 0, 0, 0, 0, "TURING is back, and changes again"},
    {43000, 0, 0, 0, 0, "Hold REC whenever a generator plays something to keep"},
};
/* minimal: 122 BPM, D minor; a marimba-like tick figure on EUCLID as track 1, a dry kick and rim in STEP, a soft
 * chord on the offbeat every other bar, a round bass on TURING */
static void g_minimal_scene(void)
{
    song.g[G_BPM] = 122;
    scale(2, SCL_MIN, 0);
    sends(75, 60, 100, 60);
    sound(0, ENGI_FM6, FX_MOKUGYO);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_EUCLID, 19);
    tx(0, TX_EUC_STEPS, 13); tx(0, TX_EUC_PULSES, 5); tx(0, TX_EUC_ROT, 1);
    mixer(0, 116, -20, 30, 70);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, L_RS, L_BD, 0, L_CH, 0, L_BD, 0, 0, L_RS, L_BD, 0, L_CH, L_RS};
        static const uint8_t V[16] = {104, 0, 0, 60, 100, 0, 50, 0, 102, 0, 0, 64, 100, 0, 52, 44};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 80, 0, 5, 0);
    sound(2, ENGI_TRIO, TR_HOLLOW_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 12);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_OFF); tx(2, TX_CH_OCT, -1);
    mixer(2, 92, 25, 60, 80);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 58);
    tx(3, TX_TUR_LEN, 6); tx(3, TX_TUR_PROB, 0); tx(3, TX_TW_SHIFT, -12); tx(3, TX_TW_DROP, 35);
    mixer(3, 80, 0, 0, 0);
    g_solo_check();
}
/* Season 1, episode 8: save the project */
static const gstep_t G_SAVE_IT[] = {
    {0, 0, 0, 0, 0, "A new song has no slot yet. Save it before you switch off"},
    {3500, 6500, 1200, "SAVE", "HOLD SAVE", "Hold SAVE: playing stops, and it asks for a slot"},
    {11000, 14000, 200, "OCT+", "OPEN SLOT", "Slot A is free. Tap OCT+: what to do with it"},
    {17000, 20000, 400, "KNOB 1", "SAVE HERE", "Turn KNOB 1 to SAVE HERE", 1},
    {23000, 26000, 200, "OCT+", "NAME", "Tap OCT+: a name (PROJECT A, or your own)"},
    {29000, 32000, 200, "OCT+", "SAVE", "Tap OCT+ on SAVE: SAVED"},
    {35000, 38000, 200, "PLAY", "PLAY", "Tap PLAY to carry on"},
    {42000, 45000, 1200, "SAVE", "HOLD SAVE", "From now on, hold SAVE: straight to slot A"},
    {52000, 0, 0, 0, 0, "Saving stops the music: save between takes"},
};
static void guide_e08(const char *out, FILE *tsv)
{
    G_RUN("e08-print", "PRINT AND KEEP", "GREEN", 50, 4, g_synthpop_scene, G_PRINT);   /* synth-pop */
    G_RUN("e08-save", "SAVE IT", "YELLOW", 59, 4, g_minimal_scene, G_SAVE_IT);   /* minimal */
}
