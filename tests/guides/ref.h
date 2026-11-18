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
    {27500, 30500, 1500, "KNOB 4", "AMOUNT", "Turn KNOB 4: how far (+50 becomes +80 %: the sweep widens)", 30},
    {37000, 0, 0, 0, 0, "Sixteen cords; any source, on any track, onto any destination"},
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
static void guide_ref(const char *out, FILE *tsv)
{
    G_RUN("r-condition", "EVERY OTHER TIME", "MOSAIC", 64, 4, g_downtempo_scene, G_COND);   /* downtempo */
    G_RUN("r-mod-first", "YOUR FIRST MOD", "LILAC", 45, 4, g_triphop_scene, G_MOD_FIRST);   /* trip-hop */
}
