/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 5: your first generator: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */

/* checks, called last in a scene of episodes 5 and 6: GUIDE_SOLO=n (1..4) only track n heard (the others at LEVEL 0),
 * to balance a scene by measuring each track alone; GUIDE_NOTES=1 every generator note on stderr (s, track, note,
 * velocity), to check a caption such as "the line repeats" */
static void g_solo_check(void)
{
    const char *s = getenv("GUIDE_SOLO");
    if (getenv("GUIDE_NOTES")) dm_log = stderr;
    if (s && *s >= '1' && *s <= '4') for (uint32_t j = 0; j < NTRK; j++) if (j != (uint32_t)(*s - '1')) trk[j].p[P_LEVEL] = 0;
}
/* Berlin school: 108 BPM, E minor; a sequencer line of sixteenths written in STEP as track 1 (the clip turns its
 * MODE), a bass pulsing in eighths, string-machine chords held four bars, a lead far away on AWAKE */
static void g_berlin_scene(void)
{
    song.g[G_BPM] = 108;
    scale(4, SCL_MIN, 0);
    sends(100, 80, 104, 65);
    sound(0, ENGI_TRIO, TR_ARP_LEAD);
    {
        static const uint8_t SEQ[16] = {64, 71, 67, 76, 64, 71, 67, 74, 64, 71, 67, 76, 72, 71, 67, 66};
        for (uint32_t i = 0; i < 16u; i++) step_note(0, i, SEQ[i], i % 4u ? 80 : 100);
        trk[0].p[P_SLEN] = 16;
    }
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_16;
    trk[0].p[P_VOICE] = 1;                              /* MONO: a sequencer's one voice */
    tx(0, TX_EV_SEED, 21);                              /* (the seed TURING and EUCLID start from once picked) */
    tx(0, TX_TUR_LEN, 16); tx(0, TX_TUR_PROB, 6); tx(0, TX_TUR_SPAN, 2);
    tx(0, TX_EUC_STEPS, 16); tx(0, TX_EUC_PULSES, 9); tx(0, TX_EUC_ROT, 0);
    mixer(0, 100, -10, 45, 60);
    sound(1, ENGI_TRIO, TR_FAT_BASS);
    {
        static const uint8_t B[8] = {40, 40, 52, 40, 40, 40, 52, 43};
        for (uint32_t i = 0; i < 8u; i++) step_note(1, i, B[i], i % 2u ? 76 : 96);
        trk[1].p[P_SLEN] = 8;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_8;
    trk[1].p[P_VOICE] = 1;
    mixer(1, 98, 0, 5, 0);
    sound(2, ENGI_TRIO, TR_STRING_MACH);
    trk[2].p[P_CHRD] = 1;                               /* DIA3 */
    gen(2, TESS_ENG_CHORD, 9);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 3); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    trk[2].p[P_REL] = 50;
    mixer(2, 66, 20, 100, 20);
    sound(3, ENGI_FM6, FX_FLUTE);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_AWAKE, 48);
    tx(3, TX_AWK_LENA, 7); tx(3, TX_AWK_LENB, 5); tx(3, TX_AWK_PROB, 30); tx(3, TX_TW_DROP, 45);
    mixer(3, 92, 30, 90, 80);
    g_solo_check();
}
/* Season 1, episode 5: MODE: who writes the notes */
static const gstep_t G_MODE[] = {
    {0, 0, 0, 0, 0, "Track 1 plays steps written by hand"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN, where KNOB 1 is the MODE (STEP)"},
    {11000, 14000, 1500, "KNOB 1", "TO TURING", "Turn KNOB 1 to TURING: now a generator writes the notes", 3},
    {22000, 25000, 600, "KNOB 1", "TO EUCLID", "One more: EUCLID, another way to write them", 1},
    {33000, 36000, 2000, "KNOB 1", "BACK TO STEP", "Turn back to STEP: your own steps were kept", -4},
    {44000, 0, 0, 0, 0, "Same sound, same track: MODE picks who writes the notes"},
};
/* loops after Eno: 76 BPM, D major pentatonic feel in D major; an electric piano on TURING as track 1 (the clip locks
 * it), a voice on AWAKE in half notes, a soft pad holding the chord, a breath flute on a five-note loop */
static void g_eno_scene(void)
{
    song.g[G_BPM] = 76;
    scale(2, 1, 0);                                     /* D, MAJ */
    sends(120, 95, 90, 75);
    sound(0, ENGI_FM6, FX_TINE_EP);
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_TURING, 63);
    tx(0, TX_TUR_LEN, 8); tx(0, TX_TUR_PROB, 60); tx(0, TX_TUR_SPAN, 2);
    mixer(0, 100, -15, 80, 45);
    sound(1, ENGI_ORACLE, OR_VOX);
    trk[1].p[P_SDIV] = DIV_2;
    gen(1, TESS_ENG_AWAKE, 12);
    tx(1, TX_AWK_LENA, 7); tx(1, TX_AWK_LENB, 3); tx(1, TX_AWK_PROB, 50);
    mixer(1, 98, 25, 110, 40);
    sound(2, ENGI_TRIO, TR_SOFT_PAD);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 4);
    tx(2, TX_CH_PROG, GEN_CH_DRONE); tx(2, TX_CH_BARS, 3); tx(2, TX_CH_HIT, GEN_CH_HOLD); tx(2, TX_CH_OCT, -1);
    mixer(2, 56, -5, 100, 0);
    sound(3, ENGI_TRIO, TR_BREATH_FLUTE);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_TURING, 70);
    tx(3, TX_TUR_LEN, 5); tx(3, TX_TUR_PROB, 0); tx(3, TX_TW_DROP, 55); tx(3, TX_TW_SHIFT, 7);
    mixer(3, 72, 35, 100, 60);
    g_solo_check();
}
/* Season 1, episode 5: TURING: lock a line you like */
static const gstep_t G_TURING[] = {
    {0, 0, 0, 0, 0, "Track 1 is TURING: its line keeps changing"},
    {3500, 6500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: PROB is the chance a note changes (60%)"},
    {11000, 14000, 2000, "KNOB 3", "PROB TO 0", "Turn KNOB 3 down to 0: the knob reads LOCK", -60},
    {19000, 0, 0, 0, 0, "Now the same line comes round, pass after pass"},
    {28000, 31000, 1200, "KNOB 2", "LENGTH 5", "Turn KNOB 2, LENGTH, to 5: a shorter loop", -3},
    {38000, 41000, 1000, "KNOB 3", "PROB 10%", "Turn PROB up a little (10%): it drifts slowly", 5},
    {50000, 0, 0, 0, 0, "PROB 0 keeps a line. A little PROB lets it wander"},
};
static void guide_e05(const char *out, FILE *tsv)
{
    G_RUN("e05-mode", "WHO WRITES THE NOTES", "GREEN", 51, 4, g_berlin_scene, G_MODE);   /* Berlin school */
    G_RUN("e05-turing", "LOCK A LINE", "GREEN", 57, 4, g_eno_scene, G_TURING);   /* loops after Eno */
}
