/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* ENTRIES (firmware/src/entries.c): tracks out and back in on the CHAIN's phrase lines, through the real sequencer
 * and generators (hostsim.c), a block at a time.
 *   ENTRIES 0 with every other setting turned: the voices the same as without, block for block;  the rules over 96
 *   phrases (ABAC, WANDER 40): changes only on phrase lines, exits only where a section begins, one entry a phrase, a
 *   track in or out at least DWELL phrases, one track always playing, an ANCHOR never out, a melody only over a bass or
 *   harmony;  above 50 the phrase before A down to the anchors, A with everything;  FORM OFF: a section every 4
 *   phrases;  the same piece from PLAY;  CUT and FADE;  an out track starts no voice;  hand mutes and GLO's key;
 *   SOLO heard;  AUTO's roles;  ORDER: in by number, out the other way round, AUTO as by ROLE. */
#define TESSERA_NOTE_HOOK et_note
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
static void check(const char *what, int ok)
{
    printf("entries: %-98s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) fails++;
}

static uint32_t et_blk, et_nhash, et_nn, et_off_bad;
static void et_note(track_t *t, uint32_t note, uint32_t vel)   /* the sequencer's notes, before Entries */
{
    if (!vel) return;
    et_nhash = (et_nhash ^ (et_blk * 131u + note * 7u + trk_index(t))) * 16777619u;
    et_nn++;
}
static uint32_t et_vhash, et_starts[NTRK], et_starts_out[NTRK];
static uint8_t et_gate[NTRK][NVOICE], et_vnote[NTRK][NVOICE];

static void sel(uint32_t tr) { song.sel = (uint8_t)tr; }
static void gen(uint32_t tr, uint32_t mode) { sel(tr); tessera_knob(0, (int32_t)mode); }
static void px(uint32_t tr, uint32_t x, int32_t v) { tessera_param_set(tr, x, v); }
enum { FM_OFF, FM_AB, FM_ABAC, FM_ABCB, FM_AABA, FM_ABCA, FM_AABC };

static void power_on(uint32_t seed)
{
    host_tracks_init();
    for (uint32_t k = 0; k < NTRK; k++) {             /* (no voice left from the run before: the same start each time) */
        memset(trk[k].v, 0, sizeof trk[k].v);
        trk[k].nmono = 0;
    }
    tessera_reset();
    song.g[G_BPM] = 120;
    rng_state = seed;
    et_blk = 0;
    et_vhash = et_nhash = 2166136261u;
    et_nn = et_off_bad = 0;
    memset(et_starts, 0, sizeof et_starts);
    memset(et_starts_out, 0, sizeof et_starts_out);
    for (uint32_t tr = 0; tr < NTRK; tr++)
        for (uint32_t i = 0; i < NVOICE; i++) et_gate[tr][i] = trk[tr].v[i].gate, et_vnote[tr][i] = trk[tr].v[i].note;
}
static void play(void) { transport_req = 1; }
/* a block; the voices begun in it (a gate rising, or a new note on a held gate), into the hash */
static void block(void)
{
    uint32_t tr, i, out = tessera_entries_get(0, 0);
    int32_t g0[NTRK];
    for (tr = 0; tr < NTRK; tr++) g0[tr] = entry_gain[tr];
    events_block(CTL);
    cord_tick();
    if (!tessera.et_amount && (tessera_entries_get(0, 0) || entry_gain[0] != 32768 || entry_gain[1] != 32768 ||
                               entry_gain[2] != 32768 || entry_gain[3] != 32768))
        et_off_bad++;
    for (tr = 0; tr < NTRK; tr++)
        for (i = 0; i < NVOICE; i++) {
            const voice_t *v = &trk[tr].v[i];
            if (v->gate && (!et_gate[tr][i] || v->note != et_vnote[tr][i])) {
                et_vhash = (et_vhash ^ (et_blk * 131u + v->note * 7u + tr)) * 16777619u;
                et_starts[tr]++;
                if ((out >> tr & 1u) && !g0[tr] && !entry_gain[tr] && (tessera_entries_get(0, 0) >> tr & 1u))
                    et_starts_out[tr]++;
            }
            et_gate[tr][i] = v->gate;
            et_vnote[tr][i] = v->note;
        }
    et_blk++;
}
static uint32_t bar_blocks(void) { return 4u * beat_samples() / CTL; }
static uint32_t bits(uint32_t m) { uint32_t n = 0; for (; m; m &= m - 1u) n++; return n; }

/* four tracks, a role each: BASS, RHYTHM, HARMONY (CHORD), MELODY */
static void p_four(void)
{
    gen(0, TESS_ENG_TURING);
    px(0, TX_ET_ROLE, ET_BASS);
    gen(1, TESS_ENG_EUCLID);
    px(1, TX_ET_ROLE, ET_RHYTHM);
    gen(2, TESS_ENG_CHORD);
    gen(3, TESS_ENG_MARKOV);
    px(3, TX_ET_ROLE, ET_MELODY);
}

static uint32_t notes_of(void (*setup)(void), uint32_t bars, uint32_t *n)
{
    uint32_t b;
    power_on(0xE7u);
    setup();
    play();
    for (b = 0; b < bars * bar_blocks(); b++) block();
    *n = et_nn;
    return et_nhash;
}
static void p_chain(void)
{
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
}
static void p_off_turned(void)                        /* ENTRIES 0, everything else of it turned */
{
    p_chain();
    px(0, TX_ET_DWELL, 1);
    px(0, TX_ET_STYLE, 0);
    px(0, TX_ET_FADE, 4);
    px(1, TX_ET_ROLE, ET_ANCHOR);
    px(2, TX_ET_ROLE, ET_MELODY);
}
static void test_off(void)
{
    uint32_t n0, n1, h0 = notes_of(p_chain, 64, &n0), h1 = notes_of(p_off_turned, 64, &n1);
    printf("entries:   %u notes, 0x%08x / %u notes, 0x%08x\n", n0, h0, n1, h1);
    check("ENTRIES 0, every other setting turned: the same notes, block for block, none out, every level full",
          n0 > 100u && h0 == h1 && n0 == n1 && et_off_bad == 0u);
}

/* the rules, phrase by phrase: out[p] the tracks out in phrase p (read in the block its first step plays in) */
#define ET_MAXP 128u
static uint8_t ph_out[ET_MAXP];
static uint32_t stray_change, too_few, anchor_out;
static void run(uint32_t phrases, uint32_t bars)
{
    uint32_t was = 0, pbar, ppos, first, out, tr;
    uint8_t role[NTRK];
    stray_change = too_few = anchor_out = 0;
    while (1) {
        pbar = bar_n, ppos = beat_pos;
        block();
        if (bar_n / bars >= phrases) break;
        first = bar_n % bars == 0u && beat_n == 0u && (pbar != bar_n || ppos == 0u);
        out = tessera_entries_get(0, role);
        if (out != was && !first) stray_change++;
        was = out;
        if (first && bar_n / bars < ET_MAXP) ph_out[bar_n / bars] = (uint8_t)out;
        if (bits(out) >= NTRK) too_few++;
        for (tr = 0; tr < NTRK; tr++)
            if ((out >> tr & 1u) && role[tr] == ET_ANCHOR) anchor_out++;
    }
}

/* project letters at phrase p (fc_at: the tracks' too) */
static uint32_t letter(uint32_t p) { return fc_letter_at(fc_at(p)->pos); }
static uint32_t moved(uint32_t p) { return fc_at(p)->moved & 1u; }

static void test_rules(void)
{
    enum { N = 96 };
    static uint8_t mv[N];
    uint32_t p, tr, exits_off = 0, two_in = 0, dwell_bad = 0, changes = 0, outs = 0, most = 0;
    uint32_t last[NTRK] = {0, 0, 0, 0}, mel_bad = 0;   /* (track 4 the melody; 1 the bass, 3 the chord) */
    power_on(11);
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_WANDER, 40);
    px(0, TX_ET_AMOUNT, 50);
    px(0, TX_ET_DWELL, 2);
    play();
    run(N, 4);
    for (p = 0; p < N; p++) mv[p] = (uint8_t)moved(p);
    for (p = 1; p < N; p++) {
        uint32_t gone = ph_out[p] & ~ph_out[p - 1u], back = ph_out[p - 1u] & ~ph_out[p];
        if (gone && !mv[p]) exits_off++;
        if (bits(back) > 1u) two_in++;
        for (tr = 0; tr < NTRK; tr++)
            if ((gone | back) >> tr & 1u) {
                if (last[tr] && p - last[tr] < 2u) dwell_bad++;
                if (!last[tr] && p < 2u) dwell_bad++;
                last[tr] = p;
                changes++;
            }
        outs += bits(ph_out[p]);
        if (bits(ph_out[p]) > most) most = bits(ph_out[p]);
        if (!(ph_out[p] & 8u) && (ph_out[p] & 5u) == 5u) mel_bad += !!(back & 8u);   /* (melody back, no bass or chord) */
    }
    printf("entries:   ENTRIES 50, 96 phrases: %u changes, %u track-phrases out, at most %u out at once\n", changes, outs, most);
    check("ENTRIES 50: tracks go out and come back", changes >= 8u && outs > 0u);
    check("changes only on the phrase lines (the downbeat's block)", stray_change == 0u);
    check("exits only where a section begins (the CHAIN moves on)", exits_off == 0u);
    check("entries one a phrase", two_in == 0u);
    check("a track in or out DWELL (2) phrases at least", dwell_bad == 0u);
    check("at most ENTRIES of the tracks out (50 %: 2 of 4), one always playing", most <= 2u && too_few == 0u);
    check("a melody comes back only over a bass or harmony", mel_bad == 0u);
}

static void test_breakdown(void)
{
    enum { N = 64 };
    uint32_t p, before_a = 0, bd_ok = 0, a_full = 0, a_n = 0, lone = 0;
    power_on(12);
    p_four();
    px(1, TX_ET_ROLE, ET_ANCHOR);
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_REP2, 2);                             /* (B two phrases, C one: a breakdown before A after B only) */
    px(0, TX_ET_AMOUNT, 80);
    px(0, TX_ET_DWELL, 4);
    play();
    run(N, 4);
    for (p = 1; p + 1u < N; p++) {
        if (moved(p + 1u) && !letter(p + 1u) && letter(p) == 1u) {   /* (the phrase before a return to A, after B) */
            before_a++;
            bd_ok += ph_out[p] == 0xDu;                  /* (all but the anchor, track 2) */
        }
        if (moved(p + 1u) && !letter(p + 1u) && letter(p) == 2u && ph_out[p] == 0xDu && ph_out[p - 1u] != 0xDu) lone++;
        if (moved(p) && !letter(p) && letter(p - 1u)) {
            a_n++;
            a_full += ph_out[p] == 0u;
        }
    }
    printf("entries:   ENTRIES 80: %u returns to A, %u broke down before, %u came back full\n", before_a, bd_ok, a_full);
    check("ENTRIES 80: the phrase before A down to the ANCHOR (DWELL 4 or not), after two phrases away", before_a >= 4u && bd_ok == before_a);
    check("ENTRIES 80: a single phrase away (C) is no breakdown", lone == 0u);
    check("ENTRIES 80: A comes back with everything", a_n >= 4u && a_full == a_n);
    check("the ANCHOR never out", anchor_out == 0u);
    {   /* no anchor: the one lowest in the order (the BASS) plays on */
        uint32_t ok = 1, seen = 0;
        power_on(12);
        p_four();
        px(0, TX_FC_FORM, FM_ABAC);
        px(0, TX_FC_PHRASE, 0);
        px(0, TX_FC_REP2, 2);
        px(0, TX_ET_AMOUNT, 100);
        play();
        run(N, 4);
        for (p = 1; p + 1u < N; p++)
            if (moved(p + 1u) && !letter(p + 1u) && letter(p) == 1u) seen++, ok &= ph_out[p] == 0xEu;
        check("ENTRIES 100, no ANCHOR: before A only the BASS plays; never all out", seen && ok && too_few == 0u);
    }
}

static void test_form_off(void)
{
    uint32_t p, off_sec = 0, changes = 0;
    power_on(13);
    p_four();
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 60);
    px(0, TX_ET_DWELL, 1);
    play();
    run(64, 4);
    for (p = 1; p < 64u; p++) {
        uint32_t gone = ph_out[p] & ~ph_out[p - 1u];
        changes += ph_out[p] != ph_out[p - 1u];
        if (gone && p % 4u) off_sec++;
    }
    check("FORM OFF: changes, exits only every 4 phrases (its sections)", changes >= 4u && off_sec == 0u && stray_change == 0u);
}

static void test_replay(void)
{
    uint8_t a[48];
    uint32_t same;
    power_on(14);
    p_four();
    px(0, TX_FC_FORM, FM_ABCA);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_WANDER, 60);
    px(0, TX_ET_AMOUNT, 70);
    play();
    run(48, 4);
    memcpy(a, ph_out, sizeof a);
    seq_stop();
    block();
    check("STOP: every track back in, full", tessera_entries_get(0, 0) == 0u && entry_gain[0] == 32768 && entry_gain[3] == 32768);
    play();
    run(48, 4);
    same = !memcmp(a, ph_out, sizeof a);
    check("PLAY again: the same tracks out, phrase for phrase", same);
}

static void test_fade(void)
{
    uint32_t b, line = 0, zero_at = 0, cut_at = 0, tr = 0xFFu;
    power_on(15);
    p_four();
    px(0, TX_FC_FORM, FM_AB);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    px(0, TX_ET_FADE, 2);
    play();
    for (b = 0; b < 32u * bar_blocks() && !zero_at; b++) {
        uint32_t out0 = tessera_entries_get(0, 0);
        block();
        if (!line && tessera_entries_get(0, 0) & ~out0) {
            line = et_blk;
            for (tr = 0; !((tessera_entries_get(0, 0) & ~out0) >> tr & 1u); tr++)
                ;
        }
        if (line && tr < NTRK && entry_gain[tr] == 0) zero_at = et_blk;
    }
    printf("entries:   FADE 2 bars: out at block %u, silent at %u (a bar %u blocks)\n", line, zero_at, bar_blocks());
    check("FADE 2: the level falls over 2 bars from the line", line && zero_at > line + bar_blocks() && zero_at <= line + 2u * bar_blocks() + 2u);
    power_on(15);
    p_four();
    px(0, TX_FC_FORM, FM_AB);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    px(0, TX_ET_STYLE, 0);
    play();
    line = 0;
    for (b = 0; b < 32u * bar_blocks() && !cut_at; b++) {
        uint32_t out0 = tessera_entries_get(0, 0);
        block();
        if (tessera_entries_get(0, 0) & ~out0) {
            line = et_blk;
            for (tr = 0; !((tessera_entries_get(0, 0) & ~out0) >> tr & 1u); tr++)
                ;
            if (entry_gain[tr] == 0) cut_at = et_blk;
        }
    }
    check("CUT: silent on the line's block", line && cut_at == line);
}

static void test_gate(void)
{
    uint32_t tr, out_starts = 0, starts = 0;
    power_on(16);
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 75);
    px(0, TX_ET_DWELL, 1);
    play();
    run(64, 4);
    for (tr = 0; tr < NTRK; tr++) out_starts += et_starts_out[tr], starts += et_starts[tr];
    printf("entries:   %u voices begun, %u by a track out and silent\n", starts, out_starts);
    check("a track out (faded to nothing) begins no voice", starts > 100u && out_starts == 0u);
}

static void test_hand(void)
{
    uint32_t p, b, tr, back = 0xFFu, held = 1, kept = 1;
    power_on(17);
    p_four();
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    trk[3].p[P_MUTE] = 1;                             /* (the melody muted by hand) */
    play();
    for (b = 0; b < 40u * 4u * bar_blocks(); b++) {
        block();
        if (!trk[3].p[P_MUTE]) kept = 0;
        if (back == 0xFFu)
            for (tr = 0; tr < NTRK; tr++)
                if (tessera_entries_get(0, 0) >> tr & 1u) {
                    check("GLO's key on an out track: back in, level full", entries_hand_in(tr) && !(tessera_entries_get(0, 0) >> tr & 1u) && entry_gain[tr] == 32768);
                    back = tr;
                    p = bar_n / 4u;
                    break;
                }
        if (back < NTRK && (tessera_entries_get(0, 0) >> back & 1u)) {   /* (out again: not before the next section) */
            uint32_t q;
            for (q = p + 1u; q <= bar_n / 4u; q++)
                if (moved(q)) break;
            if (q > bar_n / 4u) held = 0;
            break;
        }
    }
    check("a track muted by hand stays muted", kept);
    check("brought back by hand: left alone until the next section", back < NTRK && held);
    check("a muted track is not one of the rules' (never out)", !(tessera_entries_get(0, 0) >> 3 & 1u) || !trk[3].p[P_MUTE]);
}

static void test_solo(void)
{
    uint32_t b, tr = 0xFFu;
    int32_t buf[CTL];
    power_on(18);
    p_four();
    px(0, TX_FC_FORM, FM_AB);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    px(0, TX_ET_STYLE, 0);
    play();
    for (b = 0; b < 32u * bar_blocks() && tr == 0xFFu; b++) {
        block();
        for (uint32_t k = 0; k < NTRK; k++)
            if (tessera_entries_get(0, 0) >> k & 1u) { tr = k; break; }
    }
    for (b = 0; b < CTL; b++) buf[b] = 100000;
    solo_gain[tr] = 32768;
    for (b = 0; b < 8u; b++) mix_solo(&trk[tr], buf, CTL);   /* (the mix's own 128-sample ramp) */
    check("an out track's mix level goes to nothing", tr < NTRK && buf[CTL - 1u] == 0);
    track_solo = (uint8_t)(1u << tr);
    for (uint32_t c = 0; c < 8u; c++) {
        for (b = 0; b < CTL; b++) buf[b] = 100000;
        mix_solo(&trk[tr], buf, CTL);
    }
    check("SOLO on it: heard whatever Entries says", buf[CTL - 1u] > 90000);
    track_solo = 0;
}

static void test_roles(void)
{
    uint8_t role[NTRK];
    uint32_t b;
    power_on(19);
    gen(0, TESS_ENG_CHORD);
    gen(1, TESS_ENG_GRIDS);
    gen(2, TESS_ENG_TURING);
    px(2, TX_TUR_SPAN, 1);
    trk[2].p[P_ROOT] = 0;
    gen(3, TESS_ENG_MARKOV);
    px(0, TX_ET_AMOUNT, 50);
    play();
    for (b = 0; b < 4u * bar_blocks(); b++) block();
    (void)tessera_entries_get(0, role);
    printf("entries:   AUTO: %u %u %u %u (track 3 averages %u)\n", role[0], role[1], role[2], role[3], et.avg[2] / 16u);
    check("AUTO: CHORD harmony, ATLAS rhythm", role[0] == ET_HARMONY && role[1] == ET_RHYTHM);
    check("AUTO: low notes bass, else melody", role[2] == (et.avg[2] < 48u * 16u ? ET_BASS : ET_MELODY) && role[3] != ET_RHYTHM);
    px(3, TX_ET_ROLE, ET_ANCHOR);
    (void)tessera_entries_get(0, role);
    check("a ROLE set wins over AUTO", role[3] == ET_ANCHOR);
}

/* ORDER: four tracks ENTRIES 100, DWELL 1 (each one ready every phrase) over 96 phrases (ABAC, WANDER 40); each track's
 * ORDER, 0 AUTO */
static void order_run(const uint8_t ord[NTRK], uint32_t anchor)
{
    power_on(11);
    p_four();
    for (uint32_t tr = 0; tr < NTRK; tr++) px(tr, TX_ET_ORDER, ord[tr]);
    if (anchor < NTRK) px(anchor, TX_ET_ROLE, ET_ANCHOR);
    px(0, TX_FC_FORM, FM_ABAC);
    px(0, TX_FC_PHRASE, 0);
    px(0, TX_FC_WANDER, 40);
    px(0, TX_ET_AMOUNT, 100);
    px(0, TX_ET_DWELL, 1);
    play();
    run(96, 4);
}
static void test_order(void)
{
    enum { N = 96 };
    static const uint8_t AUTO[NTRK] = {0, 0, 0, 0}, ASROLE[NTRK] = {1, 2, 3, 0}, CUSTOM[NTRK] = {3, 1, 2, 4};
    static const uint8_t MEL1[NTRK] = {2, 3, 4, 1};
    static uint8_t was[N];
    uint32_t p, tr, ins = 0, outs = 0, in_bad = 0, out_bad = 0, alone = 0, same;
    order_run(AUTO, NTRK);
    memcpy(was, ph_out, N);
    order_run(ASROLE, NTRK);                           /* (BASS, RHYTHM, HARMONY numbered in their ROLE order, MELODY AUTO) */
    same = !memcmp(was, ph_out, N);
    for (p = 1; p < N; p++) same &= was[p] == ph_out[p];
    check("ORDER: the ROLE order numbered (melody AUTO): the same piece as all AUTO, phrase for phrase", same);
    order_run(CUSTOM, NTRK);                           /* (the bass 3RD, the rhythm 1ST, the chord 2ND, the melody 4TH) */
    for (p = 1; p < N; p++) {
        uint32_t gone = ph_out[p] & ~ph_out[p - 1u], back = ph_out[p - 1u] & ~ph_out[p], left = 0xFu & ~ph_out[p] & ~back;
        if (bits(back) == 1u) {                        /* (one in: the lowest number out; A's everything back aside) */
            ins++;
            for (tr = 0; tr < NTRK; tr++)
                if ((ph_out[p - 1u] >> tr & 1u) && CUSTOM[tr] < CUSTOM[__builtin_ctz(back)]) in_bad++;
        }
        for (tr = 0; tr < NTRK; tr++)                  /* (out: higher numbers than every track playing on) */
            if (gone >> tr & 1u) {
                outs++;
                for (uint32_t o = 0; o < NTRK; o++)
                    if ((left >> o & 1u) && CUSTOM[o] > CUSTOM[tr]) out_bad++;
            }
    }
    printf("entries:   ORDER 3 1 2 4, 96 phrases: %u entries, %u exits\n", ins, outs);
    check("ORDER 3RD 1ST 2ND 4TH: one in a phrase, the lowest number out first", ins >= 4u && in_bad == 0u && too_few == 0u);
    check("ORDER: out the other way round, the highest number playing first", outs >= 4u && out_bad == 0u);
    order_run(CUSTOM, 1u);                             /* (the rhythm, 1ST, an ANCHOR too) */
    for (p = 1; p < N; p++) alone += ph_out[p] & 2u;
    check("ORDER set on an ANCHOR: still never out", anchor_out == 0u && alone == 0u);
    order_run(MEL1, NTRK);                             /* (the melody 1ST: no bass or harmony under it needed) */
    for (p = 1; p < N; p++) alone += (ph_out[p] & 0xDu) == 0x5u;   /* (the melody on, the bass and chord out) */
    check("ORDER: a numbered melody plays on without a bass or harmony", alone > 0u);
}

static void test_params(void)
{
    power_on(20);
    check("defaults: ENTRIES 0, DWELL 2, FADE, 2 bars, AUTO", tessera_param_get(0, TX_ET_AMOUNT) == 0 && tessera_param_get(0, TX_ET_DWELL) == 2 &&
          tessera_param_get(0, TX_ET_STYLE) == 1 && tessera_param_get(0, TX_ET_FADE) == 2 && tessera_param_get(2, TX_ET_ROLE) == 0);
    px(0, TX_ET_DWELL, 99);
    px(0, TX_ET_FADE, 0);
    px(1, TX_ET_ROLE, 9);
    check("ranges: DWELL 1-8, FADE 1-4, ROLE up to ANCHOR", tessera.et_dwell == 8u && tessera.et_fade == 1u && tessera.et_role[1] == ET_ANCHOR);
    tessera_entries_knob(0, 3, 0);
    tessera_entries_knob(2, -1, 0);
    tessera_entries_knob(1, 1, 1);
    check("the ENTRIES knobs: ENTRIES 2 a detent, STYLE, OCT+ held a track's ROLE", tessera.et_amount == 6u && tessera.et_style == 0u &&
          tessera.et_role[1] == ET_ANCHOR);
    tessera_entries_knob(1, -1, 1);
    check("ROLE a step a detent", tessera.et_role[1] == ET_MELODY);
    check("ORDER: AUTO by default", tessera_param_get(0, TX_ET_ORDER) == 0 && tessera_param_get(3, TX_ET_ORDER) == 0);
    tessera_entries_knob(2, 1, 2);
    tessera_entries_knob(2, 1, 2);
    px(3, TX_ET_ORDER, 9);
    check("OCT- held: a track's ORDER a step a detent, up to 4TH", tessera.et_order[2] == 2u && tessera.et_order[3] == 4u &&
          tessera.et_role[2] == 0u);
    et_swapped = 0;
    px(0, TX_ET_ORDER, 2);
    check("ORDER: a rank another track holds swaps (T1 2ND, T3 takes T1's AUTO), the swapped track noted",
          tessera.et_order[0] == 2u && tessera.et_order[2] == 0u && et_swapped == 3u);
    et_swapped = 0;
    px(1, TX_ET_ORDER, 4);
    px(0, TX_ET_ORDER, 4);
    check("..T2 4TH (T4 AUTO), then T1 4TH: T2 takes T1's 2ND; each rank once",
          tessera.et_order[0] == 4u && tessera.et_order[1] == 2u && tessera.et_order[3] == 0u && et_swapped == 2u);
    et_swapped = 0;
    px(1, TX_ET_ORDER, 0);
    px(2, TX_ET_ORDER, 0);
    check("..AUTO any number of tracks, no swap", !tessera.et_order[1] && !tessera.et_order[2] && !et_swapped);
    memset(tessera.et_order, 0, sizeof tessera.et_order);
}

int main(void)
{
    test_params();
    test_off();
    test_rules();
    test_breakdown();
    test_form_off();
    test_replay();
    test_fade();
    test_gate();
    test_hand();
    test_solo();
    test_roles();
    test_order();
    if (fails) {
        printf("entries: %d FAILED\n", fails);
        return 1;
    }
    printf("entries tests passed\n");
    return 0;
}
