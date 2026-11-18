/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 4: keep it in key: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* Season 1, episode 4: keep it in key */
static const gstep_t G_KEY[] = {
    {0, 0, 0, 0, 0, "Every track plays in the project's key. SEL sets it"},
    {3500, 6500, 4500, "SEL", "SET THE KEY", "Hold SEL and press a key: that note is the new root (A becomes D)"},
    {0, 8000, 300, "D4", "ROOT D", 0},
    {13500, 16500, 4500, "SEL", "SET THE KEY", "Still holding SEL, turn KNOB 2: another scale (DORIAN becomes MIXOLYDIAN)"},
    {0, 18000, 1500, "KNOB 2", "NEXT SCALE", 0, 1},
    {24000, 0, 0, 0, 0, "Let go of SEL: every melodic track follows (drums keep their own)"},
};

/* a jazz chord study: 92 BPM, ORACLE keys on CHORD round the circle of fifths (a chord a bar) as track 1, a vibes
 * line on TURING, a walking bass on EUCLID, brushes of 909. F major */
static void g_jazz_scene(void)
{
    song.g[G_BPM] = 92;
    song.g[G_SWING] = 34;
    scale(5, 1, 1);                                     /* F MAJ, QNT SNAP */
    sends(85, 70, 60, 40);
    sound(0, ENGI_ORACLE, V_OR_KEYS);
    trk[0].p[P_CHRD] = 2;                               /* DIA7 */
    gen(0, TESS_ENG_CHORD, 11);
    tx(0, TX_CH_PROG, GEN_CH_CIRCLE); tx(0, TX_CH_BARS, 1); tx(0, TX_CH_HIT, GEN_CH_BEAT);
    mixer(0, 88, -15, 50, 0);
    sound(1, ENGI_FM6, V_DX_MARIMBA);
    trk[1].p[P_SDIV] = DIV_8;
    gen(1, TESS_ENG_TURING, 29);
    tx(1, TX_TUR_PROB, 25); tx(1, TX_TW_DROP, 35);
    mixer(1, 100, 20, 55, 30);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 8);
    tx(2, TX_TUR_PROB, 40); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 96, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_RS, 0, L_CH, 0, L_BD, 0, L_CH, 0, L_RS, 0, L_CH, L_CH};
        static const uint8_t V[16] = {80, 0, 50, 0, 70, 0, 54, 0, 72, 0, 50, 0, 70, 0, 56, 40};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 78, 0, 20, 0);
}
static const gstep_t G_CHORD[] = {
    {0, 0, 0, 0, 0, "Track 1 plays chords. The other tracks only know the scale"},
    {3500, 6500, 200, "SEL", "OPEN SEL", "Tap SEL: the scales"},
    {10000, 13000, 600, "SELECT", "SCL", "Turn SELECT: SCL, the key's settings", 1},
    {17500, 20500, 900, "KNOB 1", "PICK QNT", "Turn KNOB 1 to the QUANTIZE row (now SNAP)", 3},
    {25000, 28000, 1600, "KNOB 2", "QNT CHRD", "Turn KNOB 2 to CHRD: the tracks follow the chords", 4},
    {33000, 0, 0, 0, 0, "Now the marimba and the bass snap to the chord track 1 is playing"},
};
static void guide_e04(const char *out, FILE *tsv)
{
    G_RUN("e04-chord", "FOLLOW THE CHORDS", "BLUE", 50, 4, g_jazz_scene, G_CHORD);   /* jazz */
    G_RUN("e04-key", "KEEP IT IN KEY", "BLUE", 47, 60, demo_dub, G_KEY);             /* dub (demo: to replace) */
}
