/* SPDX-License-Identifier: GPL-3.0-only */
/* Guided reference clips for the pages Theo asked for after the second review (2026-11-20): TWEAK 2, the FX and
 * sends, the LFOs, the sound engines' pages, ARP, TEMPO and the MENU. Each scene a style no other clip uses (the
 * HANDOVER registry). Rendered by tests/demo_projects.c guide (GUIDE_ONLY=r-... for one clip) */

/* afrobeat: 108 BPM, D dorian; a marimba on EUCLID as track 1 (it BRANCHes onto track 2's sound), a muted guitar-like
 * pluck on EUCLID as track 2, a bass on TURING as track 3 (its LOGIC), an organ stab on CHORD as track 4 (its COND) */
static void g_afrobeat_scene(void)
{
    song.g[G_BPM] = 108;
    song.g[G_SWING] = 12;
    scale(2, SCL_DOR, 0);
    sends(70, 55, 60, 40);
    sound(0, ENGI_FM6, V_DX_MARIMBA);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_EUCLID, 31);
    tx(0, TX_EUC_STEPS, 16); tx(0, TX_EUC_PULSES, 9); tx(0, TX_EUC_ROT, 1);
    mixer(0, 100, -20, 30, 20);
    sound(1, ENGI_FM6, V_PLUCK);
    trk[1].p[P_SDIV] = DIV_16;
    gen(1, TESS_ENG_EUCLID, 44);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 5); tx(1, TX_EUC_ROT, 3); tx(1, TX_TW_SHIFT, 12);
    mixer(1, 96, 25, 30, 25);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_TURING, 8);
    tx(2, TX_TUR_PROB, 10); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 88, 0, 0, 0);
    sound(3, ENGI_FM6, FX_ORGAN);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 5);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 1); tx(3, TX_CH_HIT, GEN_CH_BEAT);
    mixer(3, 78, -10, 30, 0);
    g_solo_check();
}
static const gstep_t G_TWEAK2[] = {
    {0, 0, 0, 0, 0, "TWEAK 2 decides which hits play, and on whose sound"},
    {3000, 5000, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE, then turn SELECT three times: TWEAK 2. Track 1, the marimba"},
    {0, 7000, 1200, "SELECT", "TWEAK 2", 0, 3},
    {12000, 14000, 600, "KNOB 2", "TO T2", "Turn KNOB 2, TO: track 2 and a mode, COIN: each hit tossed", 1},
    {19000, 21000, 2000, "KNOB 1", "BRANCH", "Turn KNOB 1, BRANCH: 50 %. Half the hits play on track 2's sound", 25},
    {28000, 30000, 600, "KNOB 2", "TOGGLE", "TOGGLE: it switches side on a toss won. Call and response", 1},
    {37000, 39000, 600, "KNOB 2", "LATCH", "LATCH: one toss a bar, a whole bar on one side", 1},
    {46000, 48000, 2500, "KNOB 1", "BRANCH 0", "BRANCH back to 0", -25},
    {52000, 54000, 600, "ALGORITHM", "TRACK 3", "Turn ALGORITHM to track 3, the bass", 2},
    {58000, 60000, 600, "KNOB 3", "AND T1", "Turn KNOB 3, LOGIC: AND T1. The bass plays only where the marimba hits too", 1},
    {67000, 69000, 1500, "KNOB 3", "XOR T1", "Further, past OR: XOR T1. Only where one of them hits, never both", 6},
    {76000, 78000, 1500, "KNOB 3", "OFF", "NOT is last; back to OFF", -7},
    {81000, 83000, 600, "ALGORITHM", "TRACK 4", "Track 4, the organ", 1},
    {86000, 88000, 600, "KNOB 4", "COND 1:2", "Turn KNOB 4, COND: 1:2, it plays in every other bar", 1},
    {95000, 97000, 600, "KNOB 4", "COND 1:4", "1:4: the first bar of four (also 2:4, 4:4, 1:8, FIRST, !FIRST)", 2},
    {106000, 0, 0, 0, 0, "BRANCH and TO, LOGIC, COND: two simple parts become one groove"},
};
static void guide_ref_pages(const char *out, FILE *tsv)
{
    G_RUN("r-tweak2", "TWEAK 2: WHICH HITS PLAY", "MOSAIC", 112, 4, g_afrobeat_scene, G_TWEAK2);   /* afrobeat */
}
