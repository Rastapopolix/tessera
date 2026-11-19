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
/* acid house: 124 BPM, A minor; a TRIO saw bass on TURING as track 1 (its FX: DRIVE, FILTER, PHASER), a 909 in STEP,
 * ORACLE chord stabs on the offbeat, a soft pad. Send effects: 1 a reverb, 2 an echo (MODE goes to SPLIT in the clip) */
static void g_acid_scene(void)
{
    song.g[G_BPM] = 124;
    scale(9, SCL_MIN, 0);
    sends(70, 55, 60, 45);
    sound(0, ENGI_TRIO, TR_FAT_BASS);
    trk[0].p[P_SDIV] = DIV_16;
    gen(0, TESS_ENG_TURING, 47);
    tx(0, TX_TUR_PROB, 5); tx(0, TX_TUR_LEN, 8); tx(0, TX_TW_SHIFT, -12);
    mixer(0, 110, 0, 0, 0);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, L_OH, L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, L_OH};
        static const uint8_t V[16] = {108, 0, 60, 0, 104, 0, 60, 56, 106, 0, 62, 0, 104, 0, 60, 58};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 76, 0, 10, 0);
    sound(2, ENGI_ORACLE, OR_KEYS);
    trk[2].p[P_E0] = -15;
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 9);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_OFF);
    mixer(2, 70, 20, 40, 40);
    sound(3, ENGI_TRIO, TR_WARM_PAD);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 9);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 52, -20, 70, 0);
    g_solo_check();
}
static const gstep_t G_FX[] = {
    {0, 0, 0, 0, 0, "FX: three insert effects on each track, and three send effects shared by all"},
    {3500, 5500, 200, "FX", "OPEN FX", "Tap FX: FX 1, track 1's first insert. TYPE, MIX, and two of the effect's own"},
    {10000, 12000, 1500, "KNOB 1", "DRIVE", "Turn KNOB 1, TYPE: DRIVE, a soft clip", 11},
    {16500, 18500, 1500, "KNOB 2", "MIX", "KNOB 2, MIX: how much of it is heard", 30},
    {23000, 25000, 2500, "KNOB 3", "DRIVE UP", "KNOB 3, DRIVE: warmth first, then grit. The level stays about the same", 30},
    {31000, 33000, 2000, "KNOB 4", "FOLD", "KNOB 4, FOLD: the peaks turned back, as a wavefolder does", 25},
    {0, 39000, 1500, "KNOB 4", "FOLD 0", 0, -25},
    {41000, 43000, 600, "SELECT", "FX 2", "Turn SELECT: FX 2, the next insert. TYPE FILTER", 1},
    {0, 45000, 1500, "KNOB 1", "FILTER", 0, 10},
    {0, 48000, 1200, "KNOB 2", "MIX", 0, 30},
    {48500, 51000, 3000, "KNOB 3", "CUT", "KNOB 3, CUT: the filter closes. KNOB 4, RES, its resonance", -25},
    {0, 55000, 2000, "KNOB 4", "RES", 0, 25},
    {60000, 62000, 600, "SELECT", "FX 3", "FX 3: a PHASER, last in the track's chain", 1},
    {0, 64000, 600, "KNOB 1", "PHASER", 0, 1},
    {0, 66000, 1200, "KNOB 2", "MIX", 0, 25},
    {72000, 74000, 600, "SELECT", "FX SEND", "FX SEND: this track's send, the RETURN level, and FX MODE", 1},
    {79000, 81000, 600, "KNOB 3", "SPLIT", "KNOB 3, MODE: CHAIN, one send through all three; SPLIT, a send to each", 1},
    {85000, 87000, 600, "SELECT", "FX SENDS", "FX SENDS: with SPLIT, this track's send to each of the three", 1},
    {0, 90000, 1500, "KNOB 2", "SEND 2", 0, 30},
    {90000, 0, 0, 0, 0, "Turn KNOB 2: the bass into SEND FX 2, the echo"},
    {97000, 99000, 600, "SELECT", "SEND FX 1", "SEND FX 1, 2 and 3: the shared effects, here a reverb", 1},
    {104000, 106000, 600, "SELECT", "SEND FX 2", "SEND FX 2: the echo. KNOB 3, its TIME", 1},
    {0, 108000, 1500, "KNOB 3", "TIME", 0, -10},
    {115000, 0, 0, 0, 0, "FX 1 to 3 on each track; FX SEND, FX SENDS; SEND FX 1 to 3"},
};
/* filter house: 122 BPM, G minor; string-machine chords on the offbeat as track 1 (its LFO opens and closes the
 * filter), a 909 in STEP, a bass on EUCLID, a soft lead on AWAKE as track 4 (its LFO a vibrato that FADEs in) */
static void g_filterhouse_scene(void)
{
    song.g[G_BPM] = 122;
    scale(7, SCL_MIN, 0);
    sends(70, 55, 60, 45);
    sound(0, ENGI_TRIO, TR_STRING_MACH);
    trk[0].p[P_E5] = 44;
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 21);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_OFF);
    mixer(0, 120, 0, 30, 20);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, L_OH, L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, 0};
        static const uint8_t V[16] = {108, 0, 58, 0, 102, 0, 58, 54, 106, 0, 60, 0, 102, 0, 58, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 70, 0, 10, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_EUCLID, 12);
    tx(2, TX_EUC_STEPS, 16); tx(2, TX_EUC_PULSES, 5); tx(2, TX_EUC_ROT, 2); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 72, 0, 0, 0);
    sound(3, ENGI_TRIO, TR_SOFT_LEAD);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_AWAKE, 9);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 3); tx(3, TX_AWK_PROB, 45);
    tx(3, TX_NL_MODE, 2); tx(3, TX_TW_SHIFT, 12);
    trk[3].p[P_LRATE] = 88;                             /* (a vibrato's rate, about 5 Hz) */
    mixer(3, 74, 20, 60, 40);
    g_solo_check();
}
static const gstep_t G_LFO[] = {
    {0, 0, 0, 0, 0, "LFO: each track's own slow wave, to move its sound"},
    {3500, 5500, 200, "LFO", "OPEN", "Tap LFO: MODS, the cords, come first. SELECT turns on to LFO, track 1's own"},
    {0, 9000, 600, "SELECT", "LFO", 0, 2},
    {13000, 0, 0, 0, 0, "RATE, WAVE, PHS (where the wave starts), FADE (how slowly it comes in). It moves nothing yet"},
    {20000, 22000, 600, "SELECT", "LFO DEST", "Turn SELECT to LFO DEST: how far it moves PIT, FLT, SHP and AMP", 3},
    {27000, 29000, 2000, "KNOB 2", "FLT", "Turn KNOB 2, FLT: the strings' filter opens and closes", 35},
    {36000, 38000, 600, "SELECT", "LFO", "Back to LFO. KNOB 1, RATE: faster", -3},
    {0, 40000, 1500, "KNOB 1", "FASTER", 0, 20},
    {45000, 47000, 2000, "KNOB 1", "SLOWER", "Slower: a sweep over bars", -30},
    {53000, 55000, 600, "KNOB 2", "SQR", "KNOB 2, WAVE: SIN, TRI, SAW, SQR, S&H. SQR jumps between two places", 3},
    {61000, 63000, 600, "KNOB 2", "S&H", "S&H: a new place, at random, each cycle", 1},
    {0, 69000, 600, "KNOB 2", "SIN", 0, -4},
    {70000, 72000, 600, "SELECT", "LFO 2", "LFO 2: KNOB 1, SYNC ties it to the tempo. 1/4: a cycle a beat", 1},
    {0, 74000, 1500, "KNOB 1", "1/4", 0, 9},
    {81000, 83000, 600, "KNOB 2", "FREE", "KNOB 2, TRIG: NOTE starts the wave again on each note, FREE lets it run", 1},
    {89000, 91000, 600, "KNOB 3", "UNI", "KNOB 3, POL: BI swings both ways, UNI only up from where the setting is", 1},
    {97000, 99000, 600, "ALGORITHM", "TRACK 4", "Turn ALGORITHM to track 4, the lead: its own LFO", 3},
    {103000, 105000, 600, "SELECT", "LFO DEST", "Its LFO DEST: KNOB 1, PIT, a little: a vibrato", 2},
    {0, 107000, 1200, "KNOB 1", "PIT", 0, 5},
    {113000, 115000, 600, "SELECT", "LFO", "On LFO, KNOB 4, FADE: the vibrato comes in after each note starts", -3},
    {0, 117000, 1500, "KNOB 4", "FADE", 0, 22},
    {125000, 0, 0, 0, 0, "Each LFO is also a source on MODS: T1 LFO can move anything, on any track"},
};
/* progressive trance: 128 BPM, A minor; an ARP lead as track 1 (HOLD on: the keys held stay), a 909 kick and offbeat
 * hats in STEP, an offbeat bass on EUCLID, a glass pad holding A on CHORD DRONE */
static void g_trance_scene(void)
{
    song.g[G_BPM] = 128;
    scale(9, SCL_MIN, 0);
    sends(80, 60, 66, 50);
    sound(0, ENGI_TRIO, TR_ARP_LEAD);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_AMODE] = 1; trk[0].p[P_ARATE] = DIV_16; trk[0].p[P_AHOLD] = 1;
    mixer(0, 92, 0, 40, 50);
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_OH, 0, L_BD, 0, L_OH, 0, L_BD, 0, L_OH, 0, L_BD, 0, L_OH, L_CH};
        static const uint8_t V[16] = {110, 0, 56, 0, 106, 0, 56, 0, 108, 0, 58, 0, 106, 0, 56, 44};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 72, 0, 10, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_EUCLID, 3);
    tx(2, TX_EUC_STEPS, 8); tx(2, TX_EUC_PULSES, 4); tx(2, TX_EUC_ROT, 1); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 72, 0, 0, 0);
    sound(3, ENGI_ORACLE, OR_GLASS_PAD);
    gen(3, TESS_ENG_CHORD, 4);
    tx(3, TX_CH_PROG, GEN_CH_DRONE); tx(3, TX_CH_BARS, 4); tx(3, TX_CH_HIT, GEN_CH_HOLD);
    mixer(3, 66, 0, 80, 0);
    g_solo_check();
}
static const gstep_t G_ARP[] = {
    {0, 0, 0, 0, 0, "ARP: hold notes, and it plays them one at a time"},
    {3500, 5500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. Track 1's MODE is ARP. Turn SELECT to ARP"},
    {0, 9000, 600, "SELECT", "ARP", 0, 2},
    {13000, 15000, 1500, "A3", "A", "Hold A, C and E. HOLD is on, so the notes play on after you let go"},
    {0, 15000, 1500, "C4", "C", 0},
    {0, 15000, 1500, "E4", "E", 0},
    {22000, 24000, 600, "KNOB 1", "DN", "KNOB 1, MODE: UP, then DN, high to low", 1},
    {28000, 30000, 600, "KNOB 1", "UPDN", "UPDN: up, then down", 1},
    {34000, 36000, 600, "KNOB 1", "RND", "RND: a new random note each step", 1},
    {40000, 42000, 1200, "KNOB 1", "RND1", "RND1: one random order, played again and again", 4},
    {46000, 48000, 1500, "KNOB 1", "UP", "Back to UP", -7},
    {51000, 53000, 600, "KNOB 2", "16T", "KNOB 2, RATE: 16T, triplets", -1},
    {57000, 59000, 600, "KNOB 2", "1/16", "And back to 1/16", 1},
    {61000, 63000, 900, "KNOB 3", "OCT 3", "KNOB 3, OCT: over three octaves", 2},
    {68000, 70000, 1500, "KNOB 4", "GATE", "KNOB 4, GATE: shorter notes", -30},
    {75000, 77000, 15000, "OCT-", "HOLD OCT-", "Hold OCT-: SWING, PROB, HOLD and ORDER"},
    {0, 80000, 1500, "KNOB 1", "SWING", 0, 30},
    {80500, 0, 0, 0, 0, "KNOB 1, SWING: every second note later"},
    {85000, 86000, 1500, "KNOB 2", "PROB", "KNOB 2, PROB: some notes rest", -40},
    {94000, 96000, 1500, "F3", "F", "A new chord: F, A and C. The arpeggio takes the new notes"},
    {0, 96000, 1500, "A3", "A", 0},
    {0, 96000, 1500, "C4", "C", 0},
    {103000, 105000, 1500, "G3", "G", "G, B and D"},
    {0, 105000, 1500, "B3", "B", 0},
    {0, 105000, 1500, "D4", "D", 0},
    {112000, 114000, 1500, "A3", "A", "And A, C, E again"},
    {0, 114000, 1500, "C4", "C", 0},
    {0, 114000, 1500, "E4", "E", 0},
    {120000, 0, 0, 0, 0, "ORDER: from the lowest note, or as pressed. TWEAK's SHIFT moves the arpeggio"},
};
static void guide_ref_pages(const char *out, FILE *tsv)
{
    G_RUN("r-arp", "ARP: ONE NOTE AT A TIME", "MOSAIC", 128, 4, g_trance_scene, G_ARP);   /* progressive trance */
    G_RUN("r-lfo", "LFO: A WAVE THAT MOVES", "LILAC", 132, 4, g_filterhouse_scene, G_LFO);   /* filter house */
    G_RUN("r-fx", "FX: INSERTS AND SENDS", "LILAC", 122, 4, g_acid_scene, G_FX);   /* acid house */
    G_RUN("r-tweak2", "TWEAK 2: WHICH HITS PLAY", "MOSAIC", 112, 4, g_afrobeat_scene, G_TWEAK2);   /* afrobeat */
}
