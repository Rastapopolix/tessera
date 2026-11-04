/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MIDI OUT and CLOCK OUT (firmware/src/midi_out.c) through the real sequencer, keys, MIDI IN and the USB queue
 * (usb.c midi_out_q): KEYS sends the keys only (as before), ALL also the steps, the generators and the ARP, OFF nothing;
 * MIDI IN is never echoed; a track's channel follows ROUT and a drum track the DRUM channel, unless the track has its
 * own (GLO > MIDI OUT: 1..16, or OFF); its TRANSPOSE moves the notes sent (not past 0..127); a mute sends nothing;
 * every note-on gets its note-off (STOP, a panic, a channel change); 24 clock pulses a beat with START and STOP while
 * CLK is INT, none from an external clock; the settings survive a save.
 * Run by tests/run_tests.sh (needs build/gen from one firmware build). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#define KEY_C4 7u                                       /* keys from F3 (0): C4 7 (chord_test.c) */

#define LOG_MAX 8192u
static uint32_t log_pkt[LOG_MAX], log_n;

static void drain(void)
{
    while (mo_r != mo_w) {
        if (log_n < LOG_MAX) log_pkt[log_n++] = midi_out_q[mo_r % MQ];
        mo_r++;
    }
}
static void blocks(uint32_t n) { while (n--) { events_block(CTL); drain(); } }
static uint32_t beat_blocks(void) { return beat_samples() / CTL; }   /* (real time: blocks of samples) */

static void reset(uint32_t out)
{
    ui_power_on();
    for (uint32_t i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 4;
    }
    memset(&midi_clock, 0, sizeof midi_clock); midi_sync_active = 0;
    mi_r = mi_w = 0; mo_r = mo_w = 0; usb.config = 1;
    fm1_in.notes = kb_prev = 0; song.sel = 0; song.g[G_ROUTE] = 0; settings_drumch = 10;
    settings_midi_out = (uint8_t)out; settings_clock_out = 0;
    memset(tessera.midi_ch, 0, sizeof tessera.midi_ch);
    memset(tessera.midi_tr, 0, sizeof tessera.midi_tr);
    memset(tessera.sound_off, 0, sizeof tessera.sound_off);
    events_block(CTL); drain(); log_n = 0;
}

/* the packets of kind st (0x90 note-on, 0x80 note-off: any channel; F8 FA FC) */
static uint32_t count(uint32_t st, int ch, int note)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < log_n; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        if (st >= 0xF0u ? s == st : (s & 0xF0u) == st && (ch < 0 || (s & 15u) == (uint32_t)ch) && (note < 0 || d1 == (uint32_t)note))
            n++;
    }
    return n;
}
/* every note-on was ended by the time the log ends (the same channel and note) */
static int balanced(void)
{
    static uint8_t on[16][128];
    memset(on, 0, sizeof on);
    for (uint32_t i = 0; i < log_n; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        if ((s & 0xF0u) == 0x90u) on[s & 15u][d1] = 1;
        if ((s & 0xF0u) == 0x80u) on[s & 15u][d1] = 0;
    }
    for (uint32_t c = 0; c < 16u; c++)
        for (uint32_t k = 0; k < 128u; k++)
            if (on[c][k]) return 0;
    return 1;
}
static void play(uint32_t beats) { transport_req = 1; blocks(beats * beat_blocks() + 1u); }
static void stop(void) { transport_req = 2; blocks(2); }

static int notes_test(void)
{
    int bad = 0;
    reset(MIDI_OUT_ALL);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    trk[1].step[2] = (step_t){{64, 67}, 2, ST_NOTE, 0, 0, 90};
    play(4);
    bad += check("ALL: steps go out, each track on its own channel",
                 count(0x90, 0, 60) >= 1u && count(0x90, 1, 64) >= 1u && count(0x90, 1, 67) >= 1u);
    bad += check("  the step's velocity", log_n && count(0x90, 0, 60) && (log_pkt[0] >> 24) == 100u);
    stop();
    bad += check("  STOP: every note sent has its note-off", balanced() && count(0x80, -1, -1) >= 3u);

    reset(MIDI_OUT_KEYS);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    play(4);
    bad += check("KEYS: the steps stay inside (as before)", !count(0x90, -1, -1));
    key_down(KEY_C4); drain();
    bad += check("  a key goes out on its track's channel", count(0x90, 0, 60) == 1u);
    key_up(KEY_C4); drain();
    bad += check("  and its release", count(0x80, 0, 60) == 1u && balanced());
    stop();

    reset(MIDI_OUT_OFF);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    play(2); key_down(KEY_C4); key_up(KEY_C4); drain(); stop();
    bad += check("OFF: nothing goes out, keys neither", !log_n);

    reset(MIDI_OUT_ALL);
    trk[0].p[P_MUTE] = 1;
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    play(4); stop();
    bad += check("a muted track sends nothing", !count(0x90, -1, -1));
    return bad;
}

static int echo_and_route_test(void)
{
    int bad = 0;
    reset(MIDI_OUT_ALL);
    midi_enqueue(0x09u | 0x90u << 8 | 62u << 16 | 100u << 24, 1); blocks(1);
    midi_enqueue(0x08u | 0x80u << 8 | 62u << 16, 1); blocks(1);
    bad += check("MIDI IN is not echoed back", !count(0x90, -1, -1));

    reset(MIDI_OUT_ALL);
    song.g[G_ROUTE] = 2;                                 /* CH5-8 */
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    play(1);
    bad += check("ROUT CH5-8: track 1 on channel 5", count(0x90, 4, 60) >= 1u && !count(0x90, 0, 60));
    song.g[G_ROUTE] = 3;                                 /* changed while its note may sound */
    blocks(4 * beat_blocks());
    bad += check("  ROUT changed: on to channel 9, the old channel's notes ended", count(0x90, 8, 60) >= 1u &&
                 count(0x80, 4, 60) == count(0x90, 4, 60));
    stop();
    bad += check("  nothing left on", balanced());

    reset(MIDI_OUT_ALL);
    uint32_t d = NTRK;
    for (uint32_t i = 0; i < NTRK; i++) if (drum_track(&trk[i])) { d = i; break; }
    if (d < NTRK) {
        trk[d].step[0] = (step_t){{36}, 1, ST_NOTE, 0, 0, 100};
        play(1); stop();
        bad += check("a drum track on the DRUM channel (10)", count(0x90, 9, 36) >= 1u && balanced());
        reset(MIDI_OUT_ALL); settings_drumch = 0;
        trk[d].step[0] = (step_t){{36}, 1, ST_NOTE, 0, 0, 100};
        play(1); stop();
        bad += check("  DRUM OFF: its track's channel", count(0x90, (int)d, 36) >= 1u);
    } else
        bad += check("a drum track in the power-on project", 0);

    reset(MIDI_OUT_ALL);                                 /* each track's own channel (GLO > MIDI OUT) */
    tessera_param_set(0, TX_MIDI_CH, 12); tessera_param_set(1, TX_MIDI_CH, 3); song.g[G_ROUTE] = 2;
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    trk[1].step[0] = (step_t){{64}, 1, ST_NOTE, 0, 0, 100};
    trk[2].step[0] = (step_t){{67}, 1, ST_NOTE, 0, 0, 100};
    play(1);
    bad += check("MIDI CH: track 1 on its 12, track 2 on its 3 (ROUT CH5-8 left behind), track 3 AUTO on ROUT's 7",
                 count(0x90, 11, 60) >= 1u && count(0x90, 2, 64) >= 1u && count(0x90, 6, 67) >= 1u &&
                 !count(0x90, 4, 60) && !count(0x90, 5, 64));
    tessera_param_set(0, TX_MIDI_CH, 5);                 /* changed while its note may sound */
    blocks(4 * beat_blocks());
    bad += check("  changed: on to channel 5, the old channel's notes ended", count(0x90, 4, 60) >= 1u &&
                 count(0x80, 11, 60) == count(0x90, 11, 60));
    tessera_param_set(1, TX_MIDI_CH, (int32_t)TESS_MIDI_OFF);
    log_n = 0;
    blocks(4 * beat_blocks());
    bad += check("  OFF: track 2 sends nothing more, its last note ended", !count(0x90, -1, 64) && count(0x80, 2, 64) <= 1u);
    stop();
    bad += check("  nothing left on", balanced());
    if (d < NTRK) {
        reset(MIDI_OUT_ALL);
        tessera_param_set(d, TX_MIDI_CH, 16);
        trk[d].step[0] = (step_t){{36}, 1, ST_NOTE, 0, 0, 100};
        play(1); stop();
        bad += check("  a drum track's own channel wins over DRUM", count(0x90, 15, 36) >= 1u && !count(0x90, 9, 36));
    }

    reset(MIDI_OUT_ALL);                                 /* TRANSPOSE (OCT+ held on GLO > MIDI OUT) */
    tessera_param_set(0, TX_MIDI_TR, -24); tessera_param_set(1, TX_MIDI_TR, 99);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    trk[1].step[0] = (step_t){{100}, 1, ST_NOTE, 0, 0, 100};
    play(1);
    bad += check("TRANSPOSE: track 1 two octaves down, track 2 clamped to +48 and past 127 not sent",
                 tessera_param_get(1, TX_MIDI_TR) == TESS_MIDI_TR && count(0x90, 0, 36) >= 1u && !count(0x90, 0, 60) &&
                 !count(0x90, 1, -1) && !count(0x80, 1, -1));
    tessera_param_set(0, TX_MIDI_TR, 12);                /* changed while its note may sound */
    blocks(4 * beat_blocks());
    bad += check("  changed: the notes go out an octave up, those sent before ended as they were sent",
                 count(0x90, 0, 72) >= 1u && count(0x80, 0, 36) == count(0x90, 0, 36));
    stop();
    bad += check("  nothing left on", balanced());

    reset(MIDI_OUT_ALL);                                 /* SOUND OFF (OCT- held on GLO > MIDI OUT) */
    tessera_param_set(0, TX_SOUND, 1); tessera_param_set(1, TX_SOUND, 5);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    trk[1].step[0] = (step_t){{64}, 1, ST_NOTE, 0, 0, 100};
    play(1);
    {
        uint32_t v, voiced0 = 0, voiced1 = 0;
        for (v = 0; v < NVOICE; v++) {
            voiced0 |= trk[0].v[v].active;
            voiced1 |= trk[1].v[v].active;
        }
        bad += check("SOUND OFF: the notes still go out over MIDI, no voice of their own; a value past OFF clamped",
                     count(0x90, 0, 60) >= 1u && count(0x90, 1, 64) >= 1u && !voiced0 && !voiced1 &&
                     tessera_param_get(1, TX_SOUND) == 1);
    }
    stop();
    bad += check("  nothing left on", balanced());

    reset(MIDI_OUT_ALL);
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 0, 100};
    trk[0].step[1] = (step_t){{62}, 1, ST_TIE, 0, 0, 100};
    play(1); blocks(beat_blocks() / 8u);              /* (mid-step: not the block a new loop starts its note in) */
    panic_req = 1u; blocks(1);
    bad += check("a panic ends the notes sent", balanced());
    stop();
    return bad;
}

static int generator_test(void)
{
    int bad = 0;
    reset(MIDI_OUT_ALL);
    song.sel = 1; tessera_knob(0, TESS_ENG_TURING);
    play(16);
    uint32_t n = count(0x90, 1, -1);
    stop();
    bad += check("ALL: a generator's notes go out on its track's channel", n >= 8u && balanced());

    reset(MIDI_OUT_KEYS);
    song.sel = 1; tessera_knob(0, TESS_ENG_TURING);
    play(16); stop();
    bad += check("KEYS: a generator stays inside", !count(0x90, -1, -1));

    reset(MIDI_OUT_ALL);
    trk[0].p[P_AMODE] = 1;
    key_down(KEY_C4); drain();
    bad += check("ALL with the ARP on: the key itself is not sent", !count(0x90, -1, -1));
    blocks(4 * beat_blocks());
    key_up(KEY_C4); blocks(4 * beat_blocks());
    bad += check("  the ARP's notes are, each ended", count(0x90, 0, -1) >= 2u && balanced());
    return bad;
}

static int clock_out_test(void)
{
    int bad = 0;
    reset(MIDI_OUT_KEYS);
    play(4); stop();
    bad += check("CLOCK OUT off: no clock", !count(0xF8, 0, 0) && !count(0xFA, 0, 0) && !count(0xFC, 0, 0));

    reset(MIDI_OUT_KEYS);
    settings_clock_out = 1;
    play(8);
    uint32_t pulses = count(0xF8, 0, 0);
    bad += check("CLOCK OUT: START first, then 24 pulses a beat", log_n && ((log_pkt[0] >> 8) & 0xFFu) == 0xFAu &&
                 pulses >= 8u * 24u && pulses <= 8u * 24u + 2u);
    stop();
    bad += check("  STOP at the stop, no pulse after", count(0xFC, 0, 0) == 1u && ((log_pkt[log_n - 1u] >> 8) & 0xFFu) == 0xFCu);
    log_n = 0; blocks(beat_blocks());
    bad += check("  stopped: no pulses", !count(0xF8, 0, 0));
    play(2);
    settings_clock_out = 0; blocks(2);
    bad += check("  turned off while playing: STOP goes out, the pulses end", count(0xFC, 0, 0) == 1u &&
                 ((log_pkt[log_n - 1u] >> 8) & 0xFFu) == 0xFCu);
    stop();

    reset(MIDI_OUT_KEYS);
    settings_clock_out = 1;
    song.g[G_CLOCK] = 1;                                 /* following USB clock: Tessera is not the master */
    blocks(1);
    log_n = 0;
    midi_enqueue(0x0Fu | 0xFAu << 8, 1); blocks(1);
    for (uint32_t i = 0; i < 48u; i++) { midi_enqueue(0x0Fu | 0xF8u << 8, 1); fm1_ms += 20u; blocks(1); }
    bad += check("an external clock is not sent on", song.playing && !count(0xF8, 0, 0) && !count(0xFA, 0, 0));
    return bad;
}

static int settings_test(void)
{
    int bad = 0;
    persist_t p;
    memset(&p, 0, sizeof p);
    reset(MIDI_OUT_ALL);
    settings_clock_out = 1;
    PREF_BITS |= PREF_ACCEL_OFF;
    settings_export(&p);
    settings_midi_out = MIDI_OUT_KEYS; settings_clock_out = 0;
    bad += check("settings: MIDI OUT and CLOCK OUT are saved", settings_import(&p, sizeof p) &&
                 settings_midi_out == MIDI_OUT_ALL && settings_clock_out == 1u && (PREF_BITS & PREF_ACCEL_OFF));
    p.favorites.factory[15][30] = 0;
    settings_import(&p, sizeof p);
    bad += check("  an older record (bits 5..7 zero): KEYS and no clock, as before", settings_midi_out == MIDI_OUT_KEYS &&
                 !settings_clock_out);
    p.favorites.factory[15][30] = 3u << 5;
    settings_import(&p, sizeof p);
    bad += check("  an unknown value: KEYS", settings_midi_out == MIDI_OUT_KEYS);
    settings_cc_off = 1;
    settings_export(&p);
    settings_cc_off = 0;
    bad += check("settings: MIDI CC IN OFF is saved", settings_import(&p, sizeof p) && settings_cc_off == 1u);
    p.favorites.factory[15][24] &= (uint8_t)~0x10u;
    settings_import(&p, sizeof p);
    bad += check("  an older record (bit 4 zero): CC IN ON", !settings_cc_off);
    return bad;
}

/* CLK INT runs on exact musical units (mix.c, Melodee 1.6.1): over five minutes at each of seven tempos, through the
 * tempo changes, the song keeps the true tempo to a unit, and the tracks at 1/16, 1/32 and 1/8T, the ARP and the
 * clock out never slide against it (before: a 1/16 track drifted ~27 ms in five minutes, the ARP over a second) */
static int no_drift_test(void)
{
    static const int16_t BPM[] = {97, 120, 137, 173, 40, 240, 101};
    static const uint8_t DIV[3] = {2, 3, 5};
    int bad = 0;
    uint32_t pulses = 0, bad_tracks = 0, bad_arp = 0, bad_pulses = 0, bad_tempo = 0, k;
    int64_t off[3], arp_off0, steps_arp = 0;
    double exact = 0.0;
    reset(MIDI_OUT_KEYS);
    settings_clock_out = 1;
    for (k = 0; k < 3u; k++) { trk[k].p[P_SDIV] = DIV[k]; trk[k].p[P_SLEN] = 16; }
    track_t *a = &trk[3];
    a->p[P_AMODE] = 1; a->p[P_ARATE] = 2; a->p[P_ASWING] = 0; a->nheld = 1; a->held[0] = 60; a->arp_phys = 1;
    song.g[G_BPM] = BPM[0];
    transport_req = 1; events_block(CTL);
    mo_r = mo_w;
    uint64_t u0 = ((uint64_t)bar_n * 4u + beat_n) * MIDI_BEAT_UNITS + beat_pos;
    for (k = 0; k < 3u; k++)
        off[k] = (int64_t)seq_run_step[k] * seq_div_samples(DIV[k]) + trk[k].seq_pos - (int64_t)u0;
    uint32_t last_idx = a->arp_idx;
    arp_off0 = (int64_t)a->arp_pos - (int64_t)u0;
    for (uint32_t b = 0; b < sizeof BPM / sizeof BPM[0]; b++) {
        song.g[G_BPM] = BPM[b];
        uint32_t n = 300u * (uint32_t)FS / CTL;       /* five minutes */
        for (uint32_t i = 0; i < n; i++) {
            events_block(CTL);
            exact += (double)CTL * MIDI_BEAT_UNITS * BPM[b] / (FS * 60.0);
            while (mo_r != mo_w) { if (((midi_out_q[mo_r % MQ] >> 8) & 0xFFu) == 0xF8u) pulses++; mo_r++; }
            if (a->arp_idx != last_idx) { steps_arp += (uint32_t)(a->arp_idx - last_idx); last_idx = a->arp_idx; }
            uint64_t u = ((uint64_t)bar_n * 4u + beat_n) * MIDI_BEAT_UNITS + beat_pos;
            if ((double)(u - u0) > exact + 1.0 || (double)(u - u0) + 1.0 < exact) bad_tempo++;
            for (k = 0; k < 3u; k++)
                if ((int64_t)seq_run_step[k] * seq_div_samples(DIV[k]) + trk[k].seq_pos - (int64_t)u != off[k]) bad_tracks++;
            if (steps_arp * (int64_t)seq_div_samples(2) + a->arp_pos - (int64_t)u != arp_off0) bad_arp++;
            /* the clock out: a pulse every 1,024 units from PLAY's first */
            if (pulses != (uint32_t)((u - u0) / 1024u) + 1u && pulses != (uint32_t)((u - u0) / 1024u)) bad_pulses++;
        }
    }
    transport_req = 2; blocks(2);
    a->p[P_AMODE] = 0; a->nheld = 0; a->arp_phys = 0;
    bad += check("CLK INT, 35 minutes over 7 tempos: the song keeps the true tempo to a unit", !bad_tempo);
    bad += check("  tracks at 1/16, 1/32 and 1/8T never slide against it", !bad_tracks);
    bad += check("  the ARP never slides", !bad_arp && steps_arp > 8000);
    bad += check("  the clock out stays on the same unit", !bad_pulses && pulses > 50000u);
    return bad;
}

/* TEMPO as a mod destination (issue #359): a fixed mod +100 doubles CLK INT, -100 halves it, +50 is half an octave;
 * the clock out follows; the planner's piece stays in BPM's beats (no jump through its shape) */
static int tempo_mod_test(void)
{
    static const int8_t OFF[4] = {100, -100, 50, 0};
    static const double WANT[4] = {2.0, 0.5, 1.41421356, 1.0};
    int bad = 0, ok = 1, pulses_ok = 1, plan_ok = 1;
    reset(MIDI_OUT_KEYS);
    settings_clock_out = 1;
    song.g[G_BPM] = 120;
    tessera.pl_length = 4;                              /* (a 4-minute piece planned) */
    uint32_t beats0 = pl_beats();
    transport_req = 1; events_block(CTL);
    for (uint32_t c = 0; c < 4u; c++) {
        cord_t cd = {0};
        cd.dst = CD_TEMPO;
        cd.offset = OFF[c];
        cord_set(0, &cd);
        cord_tick();
        mo_r = mo_w;
        uint64_t u0 = ((uint64_t)bar_n * 4u + beat_n) * MIDI_BEAT_UNITS + beat_pos;
        uint32_t pulses = 0, n = 10u * (uint32_t)FS / CTL;   /* ten seconds */
        for (uint32_t i = 0; i < n; i++) {
            cord_tick();
            events_block(CTL);
            while (mo_r != mo_w) { if (((midi_out_q[mo_r % MQ] >> 8) & 0xFFu) == 0xF8u) pulses++; mo_r++; }
        }
        uint64_t u = ((uint64_t)bar_n * 4u + beat_n) * MIDI_BEAT_UNITS + beat_pos;
        double got = (double)(u - u0) / ((double)n * CTL * MIDI_BEAT_UNITS * 120.0 / (FS * 60.0));
        ok &= got > WANT[c] * 0.998 && got < WANT[c] * 1.002;
        pulses_ok &= pulses + 2u >= (uint32_t)((u - u0) / 1024u) && pulses <= (uint32_t)((u - u0) / 1024u) + 2u;
        plan_ok &= pl_beats() == beats0 && beats0 == 480u;
    }
    cord_t off = {0};
    cord_set(0, &off);
    cord_tick();
    tessera.pl_length = 0;
    transport_req = 2; blocks(2);
    bad += check("TEMPO mod: +100 doubles CLK INT, -100 halves it, +50 half an octave", ok && seq_tempo_q12 == 4096u);
    bad += check("  the clock out follows it", pulses_ok);
    bad += check("  the planner's piece stays in BPM's beats (no jump through its shape)", plan_ok);
    return bad;
}

int main(void)
{
    int bad = notes_test() + echo_and_route_test() + generator_test() + clock_out_test() + no_drift_test() + tempo_mod_test() +
              settings_test();
    printf("%s\n", bad ? "MIDI OUT TEST FAILED" : "MIDI out tests passed");
    return bad != 0;
}
