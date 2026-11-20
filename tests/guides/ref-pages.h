/* SPDX-License-Identifier: GPL-3.0-only */
/* Guided reference clips for the pages Theo asked for after the second review (2026-11-20): TWEAK 2, the FX and
 * sends, the LFOs, the sound engines' pages, ARP, TEMPO and the MENU. Each scene a style no other clip uses (the
 * HANDOVER registry). Rendered by tests/demo_projects.c guide (GUIDE_ONLY=r-... for one clip) */

/* afrobeat: 108 BPM, D dorian; a marimba on EUCLID as track 1 (it BRANCHes onto track 2's sound), a bright kalimba
 * far right on EUCLID as track 2, a bass on TURING as track 3 (its LOGIC), an organ stab on CHORD as track 4 (its COND) */
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
    mixer(0, 112, -20, 30, 20);
    sound(1, ENGI_FM6, FX_KALIMBA);                     /* (Theo: the BRANCH target must stand out: a bright kalimba */
    trk[1].p[P_SDIV] = DIV_16;                          /* up high, far right, sparse on its own) */
    gen(1, TESS_ENG_EUCLID, 44);
    tx(1, TX_EUC_STEPS, 16); tx(1, TX_EUC_PULSES, 3); tx(1, TX_EUC_ROT, 3); tx(1, TX_TW_SHIFT, 12);
    mixer(1, 114, 60, 30, 35);
    sound(2, ENGI_TRIO, TR_FAT_BASS);                   /* (Theo: the bass must be heard when LOGIC thins it: a fuller bass) */
    trk[2].p[P_SDIV] = DIV_16;
    gen(2, TESS_ENG_TURING, 8);
    tx(2, TX_TUR_PROB, 10); tx(2, TX_TW_SHIFT, -12);
    mixer(2, 110, 0, 0, 0);
    sound(3, ENGI_FM6, FX_ORGAN);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 5);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 1); tx(3, TX_CH_HIT, GEN_CH_BEAT);
    mixer(3, 70, -10, 30, 0);
    g_solo_check();
}
static const gstep_t G_TWEAK2[] = {
    {0, 0, 0, 0, 0, "TWEAK 2 decides which hits play, and on whose sound"},
    {3000, 5000, 200, "ARP", "EVOLVE", "Tap ARP: EVOLVE, then turn SELECT three times: TWEAK 2. Track 1, the marimba"},
    {0, 7000, 1200, "SELECT", "TWEAK 2", 0, 3},
    {12000, 14000, 600, "KNOB 2", "TO T2", "Turn KNOB 2, TO: track 2 and a mode, COIN: each hit tossed", 1},
    {19000, 21000, 2000, "KNOB 1", "BRANCH", "Turn KNOB 1, BRANCH: 50 %. Half the hits play on track 2's kalimba, far right", 25},
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
/* organ house (Theo: an organ and keys for the effects, not the bass): 122 BPM, A minor; an organ's offbeat stabs on
 * CHORD as track 1 (its FX 1 DRIVE, FX 2 FILTER), a 909 in STEP, ORACLE e-piano chords as track 3 (its FX 3 PHASER and
 * its sends), a bass on TURING. Send effects: 1 a reverb, 2 an echo (MODE goes to SPLIT in the clip) */
static void g_organhouse_scene(void)
{
    song.g[G_BPM] = 122;
    scale(9, SCL_MIN, 0);
    sends(70, 55, 60, 45);
    sound(0, ENGI_FM6, FX_DRAWBARS);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 9);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_OFF);
    mixer(0, 110, -10, 20, 0);
    trk[0].p[P_FXRATE] = 12; trk[0].p[P_FXCHAR] = 0;   /* (DRIVE's DRV low and FOLD 0 when TYPE reaches it: the picture starts a plain sine) */
    sound(1, ENGI_DRUM, DK_909_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, L_OH, L_BD, 0, L_CH, 0, L_BD | L_CP, 0, L_CH, L_OH};
        static const uint8_t V[16] = {108, 0, 60, 0, 104, 0, 60, 56, 106, 0, 62, 0, 104, 0, 60, 58};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 72, 0, 10, 0);
    sound(2, ENGI_ORACLE, OR_KEYS);
    trk[2].p[P_CHRD] = 1;
    gen(2, TESS_ENG_CHORD, 9);
    tx(2, TX_CH_PROG, GEN_CH_TWO); tx(2, TX_CH_BARS, 2); tx(2, TX_CH_HIT, GEN_CH_BEAT);
    mixer(2, 90, 25, 30, 0);
    sound(3, ENGI_TRIO, TR_SUB_BASS);
    trk[3].p[P_SDIV] = DIV_8;
    gen(3, TESS_ENG_TURING, 47);
    tx(3, TX_TUR_PROB, 5); tx(3, TX_TUR_LEN, 8); tx(3, TX_TW_SHIFT, -12);
    mixer(3, 80, 0, 0, 0);
    g_solo_check();
}
static const gstep_t G_FX[] = {
    {0, 0, 0, 0, 0, "FX: three insert effects on each track, and three send effects shared by all"},
    {3500, 5500, 200, "FX", "OPEN FX", "Tap FX: FX 1, the first insert on track 1, the organ. TYPE, MIX, and two of the effect's own"},
    {10000, 12000, 1500, "KNOB 1", "DRIVE", "Turn KNOB 1, TYPE: DRIVE, a soft clip", 11},
    {16500, 18500, 1500, "KNOB 2", "MIX", "KNOB 2, MIX: how much of it is heard", 30},
    {22000, 24000, 4500, "KNOB 3", "DRIVE UP", "KNOB 3, DRIVE: the picture's wave rounds off flat. Warmth, then grit", 40},
    {30000, 32000, 6000, "KNOB 4", "FOLD", "KNOB 4, FOLD: the peaks turn back, more times the further up", 55},
    {0, 38500, 4000, "KNOB 4", "FOLD 0", 0, -60},
    {41000, 43000, 600, "SELECT", "FX 2", "Turn SELECT: FX 2, the next insert. TYPE FILTER", 1},
    {0, 45000, 1500, "KNOB 1", "FILTER", 0, 10},
    {0, 48000, 1200, "KNOB 2", "MIX", 0, 30},
    {48500, 51000, 3000, "KNOB 3", "CUT", "KNOB 3, CUT: the filter closes. KNOB 4, RES, its resonance", -25},
    {0, 55000, 2000, "KNOB 4", "RES", 0, 25},
    {60000, 62000, 600, "ALGORITHM", "TRACK 3", "A PHASER suits the e-piano: turn ALGORITHM to track 3, then SELECT to FX 3", 2},
    {0, 64000, 600, "SELECT", "FX 3", 0, 1},
    {0, 66000, 600, "KNOB 1", "PHASER", 0, 1},
    {0, 67500, 1200, "KNOB 2", "MIX", 0, 30},
    {72000, 74000, 600, "SELECT", "FX SEND", "FX SEND: this track's send, the RETURN level, and FX MODE", 1},
    {79000, 81000, 600, "KNOB 3", "SPLIT", "KNOB 3, MODE: CHAIN, one send through all three; SPLIT, a send to each", 1},
    {85000, 87000, 600, "SELECT", "FX SENDS", "FX SENDS: with SPLIT, this track's send to each of the three", 1},
    {0, 90000, 1500, "KNOB 2", "SEND 2", 0, 30},
    {90000, 0, 0, 0, 0, "Turn KNOB 2: the e-piano into SEND FX 2, the echo"},
    {97000, 99000, 600, "SELECT", "SEND FX 1", "SEND FX 1, 2 and 3: the shared effects, here a reverb", 1},
    {104000, 106000, 600, "SELECT", "SEND FX 2", "SEND FX 2: the echo. KNOB 3, its TIME", 1},
    {0, 108000, 1500, "KNOB 3", "TIME", 0, -10},
    {115000, 0, 0, 0, 0, "FX 1 to 3 on each track; FX SEND, FX SENDS; SEND FX 1 to 3"},
};
/* soft drone (Theo: no beat, just a soft drone, so the LFO is all that moves): 70 BPM, G minor; string-machine chords
 * held eight bars at a time as track 1 (its LFO opens and closes the filter), tracks 2 and 3 silent, a breathy flute
 * on four long notes as track 4 (its LFO a vibrato that FADEs in) */
static void g_softdrone_scene(void)
{
    song.g[G_BPM] = 70;
    scale(7, SCL_MIN, 0);
    sends(100, 70, 60, 45);
    sound(0, ENGI_TRIO, TR_STRING_MACH);
    trk[0].p[P_E5] = 44;
    trk[0].p[P_LTRIG] = 1;                              /* (FREE: SQR and S&H move while the chord holds) */
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 21);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 8); tx(0, TX_CH_HIT, GEN_CH_HOLD); tx(0, TX_CH_OCT, -1);
    mixer(0, 84, 0, 60, 20);
    tx(1, TX_MODE, TESS_ENG_OFF);
    tx(2, TX_MODE, TESS_ENG_OFF);
    sound(3, ENGI_TRIO, TR_BREATH_FLUTE);
    {
        static const uint8_t N[4] = {74, 70, 72, 67};
        for (uint32_t i = 0; i < 4u; i++) step_note(3, i, N[i], 80);
        trk[3].p[P_SLEN] = 4; trk[3].p[P_SGATE] = 110;
    }
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_1;
    trk[3].p[P_LRATE] = 88;                             /* (a vibrato's rate, about 5 Hz) */
    mixer(3, 80, 20, 70, 40);
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
    {81000, 83000, 600, "KNOB 2", "NOTE", "KNOB 2, TRIG: FREE lets the wave run. NOTE starts it again on each note", -1},
    {0, 87000, 600, "KNOB 2", "FREE", 0, 1},
    {89000, 91000, 600, "KNOB 3", "UNI", "KNOB 3, POL: BI swings both ways, UNI only up from where the setting is", 1},
    {97000, 99000, 600, "ALGORITHM", "TRACK 4", "Turn ALGORITHM to track 4, the flute: its own LFO", 3},
    {103000, 105000, 600, "SELECT", "LFO DEST", "Its LFO DEST: KNOB 1, PIT, a little: a vibrato", 2},
    {0, 107000, 1200, "KNOB 1", "PIT", 0, 5},
    {113000, 115000, 600, "SELECT", "LFO", "On LFO, KNOB 4, FADE: the vibrato comes in after each note starts", -3},
    {0, 117000, 8500, "KNOB 4", "FADE", 0, 85},
    {127000, 0, 0, 0, 0, "Now each long note starts plain, and the vibrato grows in over half a second"},
    {134000, 0, 0, 0, 0, "Each LFO is also a source on MODS: T1 LFO can move anything, on any track"},
};
/* folktronica: 88 BPM, E minor; a plucked lead ARP as track 1 (Theo: GATE must be heard, so a sound that holds while the note does, not the decaying harp; HOLD on: the keys held stay; the clip moves through Em, C,
 * Am, D, G), a brushed 808 in STEP, a sub's pedal E and a fifth drone (E, B) in STEP under every chord (Theo: the
 * trance one repeated for its own sake) */
static void g_folkarp_scene(void)
{
    song.g[G_BPM] = 88;
    scale(4, SCL_MIN, 0);
    sends(85, 65, 66, 50);
    sound(0, ENGI_TRIO, TR_ARP_LEAD);
    tx(0, TX_MODE, TESS_ENG_OFF);
    trk[0].p[P_AMODE] = 3; trk[0].p[P_ARATE] = DIV_8; trk[0].p[P_AHOLD] = 1;   /* UPDN */
    mixer(0, 92, -10, 60, 40);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, L_CH, L_RS, 0, L_CH, L_CH, L_BD, L_BD, L_CH, L_CH, L_RS, 0, L_CH, L_CH};
        static const uint8_t V[16] = {92, 0, 40, 28, 74, 0, 42, 28, 84, 56, 40, 28, 76, 0, 42, 28};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 78, 0, 15, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);                   /* a pedal E under every chord the clip plays */
    {
        static const uint8_t N[16] = {40, 0, 0, 0, 0, 0, 40, 0, 0, 0, 40, 0, 0, 0, 0, 0};
        for (uint32_t i = 0; i < 16u; i++) if (N[i]) step_note(2, i, N[i], i ? 84 : 100);
        trk[2].p[P_SLEN] = 16;
    }
    tx(2, TX_MODE, TESS_ENG_OFF); trk[2].p[P_SDIV] = DIV_8;
    mixer(2, 80, 0, 0, 0);
    sound(3, ENGI_TRIO, TR_FIFTH_DRONE);                /* E and B held: they sit under all of them */
    step_note(3, 0, 52, 80);
    trk[3].p[P_SLEN] = 1; trk[3].p[P_SGATE] = 127;
    tx(3, TX_MODE, TESS_ENG_OFF); trk[3].p[P_SDIV] = DIV_1;
    mixer(3, 68, 20, 70, 0);
    g_solo_check();
}
#define G_CHORD3(ms, a, b, c, la, lb, lc, cap) \
    {ms - 2000, ms, 1500, a, la, cap}, {0, ms, 1500, b, lb, 0}, {0, ms, 1500, c, lc, 0}
static const gstep_t G_ARP[] = {
    {0, 0, 0, 0, 0, "ARP: hold notes, and it plays them one at a time"},
    {3500, 5500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. Track 1's MODE is ARP. Turn SELECT to ARP"},
    {0, 9000, 600, "SELECT", "ARP", 0, 2},
    G_CHORD3(15000, "G3", "B3", "E4", "G", "B", "E", "Hold G, B and E. HOLD is on, so they play on after you let go"),
    {22000, 24000, 600, "KNOB 1", "DN", "KNOB 1, MODE: UPDN, up then down. Now DN, high to low", -1},
    {28000, 30000, 900, "KNOB 1", "RND", "RND: a new random note each step", 2},
    {34000, 36000, 1200, "KNOB 1", "RND1", "RND1: one random order, kept until the keys change", 4},
    G_CHORD3(42000, "G3", "C4", "E4", "G", "C", "E", "A new chord: G, C, E. RND1 picks a new order for it"),
    {46000, 48000, 1500, "KNOB 1", "UPDN", "Back to UPDN", -5},
    {51000, 53000, 600, "KNOB 2", "16T", "KNOB 2, RATE: 1/8 to 16T, faster, in triplets", 1},
    {57000, 59000, 600, "KNOB 2", "1/8", "And back to 1/8", -1},
    G_CHORD3(63000, "A3", "C4", "E4", "A", "C", "E", "A, C, E"),
    {65000, 67000, 600, "KNOB 3", "OCT 2", "KNOB 3, OCT: over two octaves", 1},
    {69500, 71500, 4000, "KNOB 4", "SHORT", "KNOB 4, GATE: down, short notes", -40},
    {75000, 75500, 7500, "KNOB 4", "LONG", "Up: long notes that run into the next", 90},
    {83000, 85000, 14000, "OCT-", "HOLD OCT-", "Hold OCT-: SWING, PROB, HOLD and ORDER"},
    {0, 87000, 2500, "KNOB 1", "SWING", 0, 25},
    {87500, 0, 0, 0, 0, "KNOB 1, SWING: every second note later"},
    {92000, 93000, 2000, "KNOB 2", "PROB", "KNOB 2, PROB: some notes rest", -40},
    G_CHORD3(103000, "F#3", "A3", "D4", "F#", "A", "D", "F#, A, D"),
    G_CHORD3(111000, "G3", "B3", "D4", "G", "B", "D", "G, B, D"),
    G_CHORD3(119000, "G3", "B3", "E4", "G", "B", "E", "And G, B, E again"),
    {125000, 0, 0, 0, 0, "ORDER: from the lowest note, or as pressed. TWEAK's SHIFT moves the arpeggio"},
};
/* ambient (Theo: something more ambient than dub techno): 72 BPM, D dorian; ORACLE glass chords on EUCLID as track 1
 * (its PATTERN changes), a soft 808 pulse in STEP, a slow sub on TURING, a far marimba on AWAKE */
static void g_ambtempo_scene(void)
{
    song.g[G_BPM] = 72;
    scale(2, SCL_DOR, 0);
    sends(110, 85, 70, 60);
    sound(0, ENGI_ORACLE, OR_GLASS_PAD);
    trk[0].p[P_CHRD] = 1;
    trk[0].p[P_SDIV] = DIV_8;
    gen(0, TESS_ENG_EUCLID, 15);
    tx(0, TX_EUC_STEPS, 16); tx(0, TX_EUC_PULSES, 5); tx(0, TX_EUC_ROT, 0);
    mixer(0, 124, 0, 80, 50);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, 0, 0, 0, L_RS, 0, 0, 0, L_CH, 0, 0, 0, L_RS, 0};
        static const uint8_t V[16] = {86, 0, 0, 0, 0, 0, 52, 0, 0, 0, 36, 0, 0, 0, 44, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 76, 0, 40, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_4;
    gen(2, TESS_ENG_TURING, 22);
    tx(2, TX_TUR_PROB, 5); tx(2, TX_TUR_LEN, 8); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 40);
    mixer(2, 72, 0, 0, 0);
    sound(3, ENGI_FM6, V_DX_MARIMBA);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_AWAKE, 9);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 3); tx(3, TX_AWK_PROB, 35); tx(3, TX_TW_SHIFT, 12);
    mixer(3, 92, 30, 100, 60);
    g_solo_check();
}
static const gstep_t G_PATTEMPO[] = {
    {0, 0, 0, 0, 0, "PATTERN and TEMPO: how long, how fast, how it swings"},
    {3500, 5500, 200, "SEQ", "OPEN SEQ", "Tap SEQ: GEN. Turn SELECT once: PATTERN, track 1's"},
    {0, 9000, 600, "SELECT", "PATTERN", 0, 1},
    {13000, 0, 0, 0, 0, "LEN, DIV, SWG and GATE: the track's own, here the glass chords"},
    {19000, 21000, 2000, "KNOB 1", "LEN 5", "KNOB 1, LEN: 5 steps. The chords loop across the bar", -11},
    {28000, 30000, 2000, "KNOB 1", "LEN 16", "Back to 16", 11},
    {34000, 36000, 900, "KNOB 2", "1/4", "KNOB 2, DIV: 1/4, half as fast", -2},
    {42000, 44000, 600, "KNOB 2", "8T", "8T: triplets", 1},
    {49000, 51000, 600, "KNOB 2", "1/8", "And 1/8 again", 1},
    {54000, 56000, 4000, "KNOB 3", "SWG", "KNOB 3, SWG: every other step late. This track only", 35},
    {62000, 64000, 3500, "KNOB 4", "LONGER", "KNOB 4, GATE: longer chords", 30},
    {70000, 72000, 5000, "KNOB 4", "SHORTER", "Shorter: stabs", -60},
    {78000, 80000, 600, "SELECT", "TEMPO", "Turn SELECT: TEMPO, the project's BPM, SWG and QNT", 1},
    {85000, 87000, 2000, "KNOB 1", "BPM UP", "KNOB 1, BPM: from 72 to 84", 12},
    {93000, 94000, 2000, "KNOB 1", "BPM BACK", "And back", -12},
    {98000, 100000, 3500, "KNOB 2", "SWG", "KNOB 2, SWG here swings every track at once", 25},
    {106000, 108000, 2500, "KNOB 3", "QNT", "KNOB 3, QNT: where the notes you record land. 1/16, or OFF: as played", 9},
    {115000, 0, 0, 0, 0, "On HOME, or with GLO held, SELECT sets the BPM too"},
};
/* drone (Theo: more drone-like, no melody): 60 BPM, D; a low fifth drone as track 1, a hollow pad holding one chord
 * eight bars at a time, a far string drone an octave up. Nothing plays a tune; AUDIO CLICK is heard plainly */
static void g_menudrone_scene(void)
{
    song.g[G_BPM] = 60;
    scale(2, SCL_DOR, 0);
    sends(115, 85, 75, 50);
    sound(0, ENGI_TRIO, TR_FIFTH_DRONE);
    step_note(0, 0, 38, 80);
    trk[0].p[P_SLEN] = 1; trk[0].p[P_SGATE] = 127;
    tx(0, TX_MODE, TESS_ENG_OFF); trk[0].p[P_SDIV] = DIV_1;
    mixer(0, 90, 0, 40, 0);
    sound(1, ENGI_TRIO, TR_HOLLOW_PAD);
    trk[1].p[P_CHRD] = 1;
    gen(1, TESS_ENG_CHORD, 14);
    tx(1, TX_CH_PROG, GEN_CH_TWO); tx(1, TX_CH_BARS, 8); tx(1, TX_CH_HIT, GEN_CH_HOLD); tx(1, TX_CH_OCT, -1);
    mixer(1, 66, -20, 75, 0);
    sound(2, ENGI_ORACLE, OR_DRONE);
    step_note(2, 0, 62, 70);
    trk[2].p[P_SLEN] = 1; trk[2].p[P_SGATE] = 127;
    tx(2, TX_MODE, TESS_ENG_OFF); trk[2].p[P_SDIV] = DIV_1;
    mixer(2, 90, 30, 80, 0);
    tx(3, TX_MODE, TESS_ENG_OFF);                       /* (Theo: lose the static-sounding track, the wind) */
    g_solo_check();
}
static const gstep_t G_MENU[] = {
    {0, 0, 0, 0, 0, "The menu: settings for the screen, the knobs, playing, MIDI and USB"},
    {3500, 5500, 900, "HOME", "MENU", "Hold HOME: the menu. KNOB 1 picks a row, KNOB 2 changes it"},
    {10000, 12000, 600, "KNOB 1", "GROUND", "GROUND: LIGHT draws the ink on paper", 1},
    {0, 13500, 600, "KNOB 2", "LIGHT", 0, 1},
    {18000, 20000, 600, "KNOB 2", "DARK", "And DARK again", -1},
    {23000, 25000, 600, "KNOB 1", "KNOBS", "KNOBS: BAR, an upright bar in place of each dial", 1},
    {0, 26500, 600, "KNOB 2", "BAR", 0, 1},
    {31000, 33000, 600, "KNOB 1", "LARGE VALUE", "LARGE VALUE: ON shows the knob you turn, large, on every page", 1},
    {0, 34500, 600, "KNOB 2", "ON", 0, 1},
    {39000, 41000, 600, "KNOB 1", "TEXT", "TEXT: BOLD, letters a pixel wider, to read from further away", 1},
    {0, 42500, 600, "KNOB 2", "BOLD", 0, 1},
    {47000, 49000, 600, "KNOB 1", "IDLE", "IDLE: LANDSCAPE after two minutes untouched. NOTE LIGHTS: the keys light with the notes", 1},
    {55000, 57000, 3500, "KNOB 1", "MUTES", "Further down, MUTES: BAR, mutes wait for the bar line", 9},
    {0, 61500, 600, "KNOB 2", "BAR", 0, 1},
    {66000, 68000, 600, "KNOB 1", "AUDIO CLICK", "AUDIO CLICK: ON, a click to play along with. REC: only while recording", 1},
    {0, 69500, 900, "KNOB 2", "ON", 0, 2},
    {76000, 78000, 600, "KNOB 2", "OFF", "OFF again. CLICK LEVEL, COUNT-IN, NOTE PREVIEW come next", -2},
    {83000, 85000, 1200, "KNOB 1", "AUTOSAVE", "AUTOSAVE: the project is saved a few seconds after STOP", 5},
    {90000, 92000, 200, "OCT-", "CLOSE", "Tap OCT-: the menu closes. The settings stay"},
    {94000, 96000, 200, "SEQ", "SEQ", "Tap SEQ: BOLD letters, bars, and the knob turned shown large"},
    {0, 98000, 600, "KNOB 4", "GATE", 0, -2},
    {0, 101000, 600, "KNOB 4", "GATE", 0, 2},
    {104000, 0, 0, 0, 0, "They belong to the FM-1, not to a project: kept through power-off"},
};
/* sophisti-pop: 96 BPM, B dorian; an FM6 electric piano on CHORD as track 1 (its pages turned), a soft 808 in STEP, a
 * bass on TURING, ORACLE strings held low */
static void g_sophisti_scene(void)
{
    song.g[G_BPM] = 96;
    scale(11, SCL_DOR, 0);
    sends(80, 60, 60, 45);
    sound(0, ENGI_FM6, V_TINE);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 14);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_BEAT);
    mixer(0, 87, -10, 40, 15);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, L_CH, 0, L_RS, 0, L_CH, 0, 0, L_BD, L_CH, 0, L_RS, 0, L_CH, L_CH};
        static const uint8_t V[16] = {100, 0, 48, 0, 84, 0, 46, 0, 0, 90, 50, 0, 84, 0, 46, 40};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 78, 0, 10, 0);
    sound(2, ENGI_TRIO, TR_FAT_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 17);
    tx(2, TX_TUR_PROB, 6); tx(2, TX_TUR_LEN, 8); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 35);
    mixer(2, 88, 0, 0, 0);
    sound(3, ENGI_ORACLE, OR_SOFT_STRINGS);
    trk[3].p[P_CHRD] = 1;
    gen(3, TESS_ENG_CHORD, 14);
    tx(3, TX_CH_PROG, GEN_CH_TWO); tx(3, TX_CH_BARS, 2); tx(3, TX_CH_HIT, GEN_CH_HOLD); tx(3, TX_CH_OCT, -1);
    mixer(3, 56, 10, 70, 0);
    g_solo_check();
}
static const gstep_t G_FM6[] = {
    {0, 0, 0, 0, 0, "FM6: six operators, as on a DX7. Its pages are on EDIT"},
    {3500, 5500, 200, "EDIT", "OPEN EDIT", "Tap EDIT: OPS, the four turned most. ALG, FB, MLVL, MRAT"},
    {10000, 12000, 1500, "KNOB 3", "BRIGHTER", "KNOB 3, MLVL: the modulators louder, a brighter piano", 12},
    {17000, 19000, 2500, "KNOB 3", "DARKER", "Down: darker and rounder", -24},
    {0, 24000, 1500, "KNOB 3", "MLVL 0", 0, 12},
    {26500, 28500, 600, "SELECT", "PATCH", "SELECT: PATCH. KNOB 4, AEG, the amp envelope of all the carriers", 1},
    {0, 32000, 3000, "KNOB 4", "SLOWER", 0, 30},
    {32500, 0, 0, 0, 0, "AEG right: a slower attack and a longer release"},
    {40000, 42000, 4000, "KNOB 4", "SHORTER", "Left: shorter, a pluck", -60},
    {0, 49000, 3000, "KNOB 4", "AEG 0", 0, 30},
    {53000, 55000, 1500, "SELECT", "EG RATE", "SELECT on to EG RATE (ENV opens it too): one operator's envelope. OP1, a carrier", 5},
    {60000, 62000, 3000, "KNOB 1", "R1", "KNOB 1, R1: how fast it rises. Lower: the note swells in", -40},
    {69000, 71000, 600, "SELECT", "EG LVL", "SELECT: EG LVL, the levels L1 to L4 it moves between", 1},
    {75000, 77000, 2000, "KNOB 2", "L2", "KNOB 2, L2: where it falls to after the attack. Higher: the note rings on", 20},
    {83000, 85000, 600, "PRESETS", "OP2", "Turn PRESETS: the next operator, OP2, a modulator. Then SELECT to its EG LVL", 1},
    {0, 89000, 900, "SELECT", "EG LVL", 0, 3},
    {93000, 95000, 2500, "KNOB 2", "L2", "KNOB 2, L2 down: the brightness fades after the attack", -30},
    {101000, 103000, 200, "EDIT", "MAP", "Tap EDIT: the map of all its pages, each operator a row"},
    {108000, 0, 0, 0, 0, "FREQ, OUT, SCALE, CURVE, PITCH EG, FM LFO and the DX7's BEND and PORTA are there too"},
    {115000, 0, 0, 0, 0, "A turned knob is saved with the project; STORE keeps a patch in a slot"},
};
/* library electronica: 84 BPM, D minor; an ORACLE poly on CHORD as track 1 (its pages turned), a dusty 808 in STEP,
 * a sub on TURING, an FM flute on AWAKE far back */
static void g_library_scene(void)
{
    song.g[G_BPM] = 84;
    scale(2, SCL_MIN, 0);
    sends(85, 65, 60, 50);
    sound(0, ENGI_ORACLE, V_OR_FAT_POLY);
    trk[0].p[P_CHRD] = 1;
    gen(0, TESS_ENG_CHORD, 19);
    tx(0, TX_CH_PROG, GEN_CH_TWO); tx(0, TX_CH_BARS, 2); tx(0, TX_CH_HIT, GEN_CH_BEAT);
    mixer(0, 88, 0, 50, 10);
    sound(1, ENGI_DRUM, DK_808_KIT);
    {
        static const uint8_t L[16] = {L_BD, 0, 0, L_CH, L_SD, 0, L_CH, 0, 0, L_BD, L_BD, L_CH, L_SD, 0, L_CH, 0};
        static const uint8_t V[16] = {104, 0, 0, 44, 90, 0, 48, 0, 0, 80, 96, 46, 92, 0, 44, 0};
        for (uint32_t i = 0; i < 16u; i++) step_hits(1, i, L[i], V[i]);
        trk[1].p[P_SLEN] = 16;
    }
    tx(1, TX_MODE, TESS_ENG_OFF); trk[1].p[P_SDIV] = DIV_16;
    mixer(1, 78, 0, 20, 0);
    sound(2, ENGI_TRIO, TR_SUB_BASS);
    trk[2].p[P_SDIV] = DIV_8;
    gen(2, TESS_ENG_TURING, 40);
    tx(2, TX_TUR_PROB, 6); tx(2, TX_TUR_LEN, 8); tx(2, TX_TW_SHIFT, -12); tx(2, TX_TW_DROP, 45);
    mixer(2, 76, 0, 0, 0);
    sound(3, ENGI_FM6, V_FLUTE);
    trk[3].p[P_SDIV] = DIV_4;
    gen(3, TESS_ENG_AWAKE, 23);
    tx(3, TX_AWK_LENA, 5); tx(3, TX_AWK_LENB, 4); tx(3, TX_AWK_PROB, 40);
    tx(3, TX_NL_MODE, 2);
    mixer(3, 76, 25, 80, 40);
    g_solo_check();
}
static const gstep_t G_ORACLE[] = {
    {0, 0, 0, 0, 0, "ORACLE: two oscillators, POLY MOD and a four-pole filter"},
    {3500, 5500, 200, "EDIT", "OPEN EDIT", "Tap EDIT: MACRO, quick offsets on the sound. CUT, RES, TUNE A, TUNE B"},
    {10000, 12000, 600, "SELECT", "OSC A", "SELECT: OSC A. FRQ, WAVE (SAW, PLS, S+P), PWA, SYNC", 2},
    {0, 15000, 600, "KNOB 2", "PLS", 0, 1},
    {20000, 22000, 600, "SELECT", "OSC B", "OSC B: KNOB 2, FNB, B a little sharp: a slow beating", 1},
    {0, 24000, 2000, "KNOB 2", "FNB", 0, 20},
    {29000, 31000, 1200, "SELECT", "FILTER", "FILTER: CTFF, RES, ENVM (how far FILT ENV opens it), KEY", 2},
    {35000, 37000, 2500, "KNOB 1", "CLOSE", "KNOB 1: close it", -25},
    {42000, 44000, 3000, "KNOB 2", "RES", "KNOB 2, RES: the filter sings", 35},
    {0, 50000, 2500, "KNOB 1", "OPEN", 0, 25},
    {55000, 57000, 600, "SELECT", "POLY MOD", "POLY MOD: OSC B moves A's pitch, note by note. KNOB 3, TO: FREQ", 1},
    {0, 60000, 600, "KNOB 3", "FREQ", 0, 1},
    {0, 62500, 2500, "KNOB 2", "OSC B", 0, 30},
    {63000, 0, 0, 0, 0, "KNOB 2, OSCB: bells and metal"},
    {0, 70000, 2500, "KNOB 2", "OSC B 0", 0, -30},
    {74000, 76000, 600, "SELECT", "CHARACTER", "CHARACTER: MDL, an EARLY or LATE filter; VNTG, each voice drifts; GLD", 1},
    {82000, 84000, 1200, "SELECT", "AMP ENV", "AMP ENV: ATK, DEC, SUS, REL. ENV opens FILT ENV and AMP ENV too", 2},
    {0, 87500, 2500, "KNOB 1", "ATK", 0, 30},
    {88000, 0, 0, 0, 0, "KNOB 1, ATK: the chords swell in"},
    {94000, 96000, 2500, "KNOB 4", "REL", "KNOB 4, REL: and ring on after", 25},
    {102000, 104000, 600, "SELECT", "LFO MOD", "LFO MOD: its own LFO. KNOB 2, AMNT, how much", 1},
    {0, 106500, 2000, "KNOB 2", "AMNT", 0, 40},
    {110000, 112000, 600, "SELECT", "LFO ROUTE", "LFO ROUTE: where it goes. KNOB 3, FILT ON: the filter breathes", 1},
    {0, 114000, 600, "KNOB 3", "FILT", 0, 1},
    {121000, 0, 0, 0, 0, "The sound is saved with the project; SAVE > SOUND keeps one in a USER slot"},
};
static void guide_ref_pages(const char *out, FILE *tsv)
{
    G_RUN("r-oracle", "ORACLE: AN ANALOGUE VOICE", "ORANGE", 128, 4, g_library_scene, G_ORACLE);   /* library electronica */
    G_RUN("r-fm6", "FM6: THE OPERATORS", "ORANGE", 122, 4, g_sophisti_scene, G_FM6);   /* sophisti-pop */
    G_RUN("r-menu", "THE MENU", "WHITE", 110, 4, g_menudrone_scene, G_MENU);   /* drone */
    G_RUN("r-tempo", "PATTERN AND TEMPO", "MOSAIC", 121, 4, g_ambtempo_scene, G_PATTEMPO);   /* ambient */
    G_RUN("r-arp", "ARP: ONE NOTE AT A TIME", "MOSAIC", 132, 4, g_folkarp_scene, G_ARP);   /* folktronica */
    G_RUN("r-lfo", "LFO: A WAVE THAT MOVES", "LILAC", 141, 4, g_softdrone_scene, G_LFO);   /* soft drone */
    G_RUN("r-fx", "FX: INSERTS AND SENDS", "LILAC", 122, 4, g_organhouse_scene, G_FX);   /* organ house */
    G_RUN("r-tweak2", "TWEAK 2: WHICH HITS PLAY", "MOSAIC", 112, 4, g_afrobeat_scene, G_TWEAK2);   /* afrobeat */
}
