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

static void guide_e07(const char *out, FILE *tsv)
{
    G_RUN("e07-mixer", "MUTE, SOLO, FILL", "MOSAIC", 59, 4, g_mixer_scene, G_MIXER); /* a beat */
}
