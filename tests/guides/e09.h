/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 9: drive other gear: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* (g_solo_check, the balance and note checks, is in e05.h) */

/* UK garage: 132 BPM, C minor; an organ-like stab line on TURING as track 1 (the clip sends it out and silences it
 * here), a two-step 909 in STEP with the kick skipping, a sub on EUCLID, a soft pad holding the chord */
static void g_garage_scene(void)
{
    song.g[G_BPM] = 132;
    song.g[G_SWING] = 30;
    scale(0, SCL_MIN, 0);
    sends(70, 60, 90, 50);
    sound(0, ENGI_FM6, FX_ORGAN);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_TURING, 9);
    tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_PROB, 0); tx(0, TX_TUR_SPAN, 1); tx(0, TX_TW_DROP, 40);
    mixer(0, 122, -15, 30, 40);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_SD, 0, L_CH, L_BD, 0, 0, L_BD | L_CH, 0, L_SD, 0, L_CH, L_CH};
        static const uint8_t V[16] = {104, 0, 58, 0, 96, 0, 62, 78, 0, 0, 92, 0, 98, 0, 60, 46};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 80, 0, 10, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_EUCLID, 31);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 3); tx(2, TX_TW_SHIFT, -12);
    tx(2, TX_NL_MODE, 1); tx(2, TX_NL_LEN, 2);
    mixer(2, 80, 0, 0, 0);
    sound(3, ENGI_TRIO, TR_WARM_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 5);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 3); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 66, 20, 80, 0);
    settings_midi_out = MIDI_OUT_ALL;                   /* (MENU > NOTES OUT ALL, set before: the clip's last caption) */
    g_solo_check();
}
static const gstep_t G_MIDI_OUT[] = {
    {0, 0, 0, 0, 0, "Each track can play another synth over MIDI"},
    {3500, 6500, 200, "GLO", "OPEN GLO", "Tap GLO, then turn SELECT to the last page, MIDI OUT"},
    {0, 8000, 2400, "SELECT", "MIDI OUT", 0, 8},
    {13500, 16500, 400, "KNOB 1", "T1 CH 1", "Turn KNOB 1: track 1 sends on channel 1 (AUTO becomes 1)", 1},
    {22000, 25000, 800, "OCT+", "SHEET", "Hold OCT+: the sheet. SOUND T1 is the first row"},
    {29500, 32500, 400, "KNOB 2", "SOUND OFF", "Turn KNOB 2 right: SOUND T1 OFF. Track 1 goes quiet here", 1},
    {38000, 41000, 200, "OCT-", "CLOSE", "Tap OCT-: its notes still go out on channel 1"},
    {45500, 0, 0, 0, 0, "First set MENU > NOTES OUT to ALL, or only the keys go out"},
};
static void guide_e09(const char *out, FILE *tsv)
{
    G_RUN("e09-midi-out", "PLAY YOUR OTHER SYNTHS", "WHITE", 54, 4, g_garage_scene, G_MIDI_OUT);   /* UK garage */
}
