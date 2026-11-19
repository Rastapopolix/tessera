/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* More reference clips (MANUAL by chapter): tests/demo_projects.c guide; tools/guide_video.py.
 * (g_solo_check, the balance and note checks, is in e05.h) */

/* fourth world (after Jon Hassell): 92 BPM, E phrygian; a busy electric piano on MARKOV in sixteenths as track 1 (it
 * holds most of the voices, so the clip watches them on SYSTEM), a warm pad, a sub, hand drums in STEP */
static void g_fourthworld_scene(void)
{
    song.g[G_BPM] = 92;
    scale(4, SCL_PHRY, 0);
    sends(110, 90, 80, 50);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_MARKOV, 27);
    tx(0, TX_MKV_LEAP, 20); tx(0, TX_MKV_DENS, 100); tx(0, TX_MKV_SPAN, 1);
    mixer(0, 90, -15, 90, 40);
    sound(1, ENGI_TRIO, TR_WARM_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 2);
    tx(1, TX_CH_PROG, GEN_CH_DRONE); tx(1, TX_CH_BARS, 3); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 64, 20, 90, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 3);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 3); tx(2, TX_EUC_ROT, 0); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 80, 0, 5, 0);
    sound(3, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_TM, 0, 0, L_RS, 0, 0, L_TM, 0, L_TM, 0, L_RS, 0, 0, L_TM, 0, L_RS};
        static const uint8_t V[16] = {84, 0, 0, 56, 0, 0, 70, 0, 80, 0, 58, 0, 0, 66, 0, 52};
        for (uint32_t i = 0; i < 16u; i++) step_hits(3, i, L[i], V[i]);
        trk[3].p[P_SLEN] = 16;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_16;
    mixer(3, 96, 0, 20, 0);
    g_solo_check();
}
static const gstep_t G_VOICES[] = {
    {0, 0, 0, 0, 0, "All four tracks share one pool of voices: the BUDGET"},
    {3500, 6500, 200, "GLO", "OPEN GLO", "Tap GLO, then turn SELECT to SYSTEM"},
    {0, 8000, 1200, "SELECT", "SYSTEM", 0, 6},
    {12000, 0, 0, 0, 0, "VOICES: the notes ringing now, out of the BUDGET of 16"},
    {20000, 23000, 800, "KNOB 1", "BUDGET 12", "Turn KNOB 1, BUDGET, down to 12: fewer ring at once", -4},
    {30000, 33000, 1000, "SELECT", "MIXER", "Back to MIXER: F#3 mutes track 1, the busy one", -6},
    {0, 35000, 200, "F#3", "MUTE T1", 0},
    {0, 37000, 1000, "SELECT", "SYSTEM", 0, 6},
    {41000, 0, 0, 0, 0, "Track 1 muted: its voices free up, 6 to 8 now ring"},
    {49000, 0, 0, 0, 0, "Crackles? Lower BUDGET, or thin the busiest track"},
};
static void guide_ref_more(const char *out, FILE *tsv)
{
    G_RUN("r-voices", "HOW MANY VOICES", "YELLOW", 56, 4, g_fourthworld_scene, G_VOICES);   /* fourth world */
}
