/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 3: change the sound: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* Season 1, episode 3: change the sound */
static const gstep_t G_SOUND[] = {
    {0, 0, 0, 0, 0, "HOME: the four tracks, each with its sound"},
    {3500, 6500, 600, "ALGORITHM", "PICK TRACK 2", "Turn ALGORITHM to pick a track: here track 2", 1},
    {11000, 14000, 2400, "PRESETS", "NEXT SOUNDS", "Turn PRESETS: track 2 steps through the next sounds (here to WARM PAD)", 3},
    {20500, 23500, 2400, "PRESETS", "BACK", "Turn PRESETS back: the sound you had (KOTO WARM) returns", -3},
    {30000, 0, 0, 0, 0, "Hold SAVE to keep the project with its sounds"},
};

/* lo-fi hip-hop: 84 BPM with swing, a tine piano on CHORD (ii V I, a chord a bar) as track 1, a lazy 808, a round
 * bass on EUCLID, a flute now and then. C major */
static void g_lofi_scene(void)
{
    song.g[G_BPM] = 84;
    song.g[G_SWING] = 28;
    scale(0, 1, 0);                                     /* C, MAJ */
    sends(70, 60, 75, 40);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_CHRD] = 2;
    gen(0, TESS_ENG_CHORD, 6);
    tx(0, TX_CH_PROG, GEN_CH_251); tx(0, TX_CH_BARS, 1); tx(0, TX_CH_HIT, GEN_CH_BEAT);
    mixer(0, 96, -10, 60, 20);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, 0, L_BD | L_CH, 0, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {104, 0, 56, 0, 92, 0, 52, 70, 0, 0, 96, 0, 94, 0, 58, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 92, 0, 15, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 19);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 98, 0, 0, 0);
    sound(3, ENGI_FM6, FX_FLUTE);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 40);
    tx(3, TX_TUR_PROB, 10); tx(3, TX_TW_DROP, 70);
    mixer(3, 74, 25, 70, 50);
}
static const gstep_t G_GLO_PRESETS[] = {
    {0, 0, 0, 0, 0, "Change a track's sound without leaving the page you are on"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: we are on GEN now, away from HOME"},
    {11000, 14000, 8000, "GLO", "HOLD GLO", "Hold GLO and turn PRESETS: track 1 gets the next sound (BELL), right here"},
    {0, 15500, 1500, "PRESETS", "NEXT SOUND", 0, 1},
    {24500, 0, 0, 0, 0, "Let go of GLO: the same page, the new sound"},
    {30000, 0, 0, 0, 0, "Without GLO, a stray turn of PRESETS never changes the sound"},
};
/* synthwave: 104 BPM, a gated 808 (big snare on two and four), a saw bass in eighths on EUCLID, a sync lead on
 * TURING as track 1, a string pad. A minor */
static void g_synthwave_scene(void)
{
    song.g[G_BPM] = 104;
    scale(9, SCL_MIN, 0);
    sends(100, 75, 80, 55);
    sound(0, ENGI_FM6, FX_SYNC_LEAD);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 61);
    tx(0, TX_TUR_PROB, 0); tx(0, TX_TW_DROP, 30);
    mixer(0, 96, 15, 50, 60);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, 0, L_BD, 0, L_CH, L_BD, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {110, 0, 60, 0, 108, 0, 60, 0, 104, 0, 60, 80, 110, 0, 64, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 92, 0, 40, 0);
    sound(2, ENGI_FM6, FX_SAW_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 3);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 8); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 90, 0, 0, 0);
    sound(3, ENGI_TRIO, TR_STRING_MACH);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 9);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 66, -25, 80, 0);
}
static const gstep_t G_EDIT[] = {
    {0, 0, 0, 0, 0, "EDIT: the pages of the selected track's sound"},
    {3500, 6500, 200, "EDIT", "OPEN EDIT", "Tap EDIT: the sound's first page (OPS, on FM6)"},
    {11000, 14000, 200, "EDIT", "THE MAP", "Tap EDIT again: the map of all its pages"},
    {19000, 22000, 600, "KNOB 1", "PICK", "Turn KNOB 1 to pick a page (here PATCH)", 1},
    {27000, 30000, 200, "OCT+", "OPEN IT", "Tap OCT+: PATCH opens"},
    {35000, 38000, 200, "OCT-", "BACK", "Tap OCT-: back to the map"},
    {43000, 46000, 200, "HOME", "GO HOME", "Tap HOME: done"},
};
static void guide_e03(const char *out, FILE *tsv)
{
    G_RUN("e03-glo-presets", "A SOUND FROM ANYWHERE", "ORANGE", 39, 4, g_lofi_scene, G_GLO_PRESETS);   /* lo-fi */
    G_RUN("e03-edit", "THE SOUND'S PAGES", "ORANGE", 53, 4, g_synthwave_scene, G_EDIT);   /* synthwave */
    G_RUN("e03-sound", "CHANGE THE SOUND", "MOSAIC", 43, 30, demo_cedar, G_SOUND);   /* koto (demo: to replace) */
}
