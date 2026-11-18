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
static void guide_ref(const char *out, FILE *tsv)
{
    G_RUN("r-mod-first", "YOUR FIRST MOD", "LILAC", 45, 4, g_triphop_scene, G_MOD_FIRST);   /* trip-hop */
}
