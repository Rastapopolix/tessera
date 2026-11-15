/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from MIDI clock contributions by ChanceTheMaker and keremimo (2026).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Audio ISR state. Musical phase is exactly MIDI_BEAT_UNITS / PPQN units per MIDI pulse (1024 at 24); changing
 * the measured tempo cannot move steps, gates, ties or the arpeggiator. */
static struct {
    uint32_t pos, rendered, last_ms, start_ms, last_time, interval_time, tempo_time, tempo_dt;
    uint16_t pulse_count;
    uint16_t spot;                                  /* Song Position: 1/16 steps from the top */
    uint8_t mode, selected, have_pulse, tempo_valid, tempo_n, tempo_count, count_valid, spot_valid, seek_wait;
    uint8_t ppqn;                                   /* the pulses a beat the tempo was measured with */
} midi_clock;

/* G_PPQN: the clock's pulses a beat, in (here) and out (midi_out.c CLOCK OUT). 24 is MIDI's; 48 for a clock that
 * would play twice as fast; 4, 8, 12 for gear that counts fewer. Each divides MIDI_BEAT_UNITS and is a multiple of
 * 4, so a quarter beat is always whole pulses: the tempo is measured over a quarter beat at every rate */
static const uint8_t PPQN_PULSES[] = {24, 48, 4, 8, 12};
_Static_assert(MIDI_BEAT_UNITS % 48u == 0u && MIDI_BEAT_UNITS % 12u == 0u, "whole units a pulse");
static uint32_t midi_ppqn(void)
{
    uint32_t k = (uint32_t)song.g[G_PPQN];
    return PPQN_PULSES[k < sizeof PPQN_PULSES ? k : 0u];
}
static uint32_t midi_pulse_units(void) { return MIDI_BEAT_UNITS / midi_ppqn(); }

/* An external clock's LFO units (Melodee 1.6.1, bc886e4): the measured tempo, so pulse jitter cannot modulate a synced
 * LFO; past a quarter of a pulse from the song's units it is pulled back, so it cannot slide either */
static struct {
    uint32_t rem;
    int32_t err;
} lfo_ext;
static uint32_t lfo_ext_units(uint32_t n, uint32_t song_units)
{
    uint32_t beat = beat_samples(), ticks = n * MIDI_BEAT_UNITS + lfo_ext.rem, units = ticks / beat;
    lfo_ext.rem = ticks % beat;
    lfo_ext.err += (int32_t)song_units - (int32_t)units;
    int32_t pull = lfo_ext.err / 256, out = (int32_t)units + pull;
    if (out < 0) {
        pull -= out;
        out = 0;
    }
    lfo_ext.err -= pull;
    return (uint32_t)out;
}

/* (Sequencer and ARP phase carry over: every clock counts the same units, mix.c) */
static void midi_clock_mode(uint32_t mode)
{
    midi_sync_active = (uint8_t)mode;
    midi_clock.mode = (uint8_t)mode;
}

/* Local PLAY / a REC note: follow the master only while its clock is running. A stopped master
 * (Stop received, pulses gone) hands timing back to the internal clock until its next Start,
 * Continue or Stop, so PLAY never dies on the lost-clock timeout. 1 = sync kept. */
static uint32_t midi_clock_local_start(uint32_t ms)
{
    if (!midi_clock.mode)
        return 0;
    if (midi_clock.tempo_valid && ms - midi_clock.last_ms <= 500u)
        return 1;
    midi_clock_mode(0);
    return 0;
}

/* Song Position (Melodee 1.6.1, 1106352), stopped only: the next Continue starts there, on its first clock (a master
 * holds that clock until its song reaches the spot). Every track is where it would be after playing from the top
 * (seq_seek); a SONG chain is the FM-1's own and starts from its top, as with Start */
static void seq_seek(uint32_t units);
static void midi_clock_position(uint32_t spot)
{
    if (song.playing || chain.armed) return;
    midi_clock.spot = (uint16_t)spot;
    midi_clock.spot_valid = 1;
}
static void midi_clock_seek(void)
{
    midi_clock.seek_wait = 0;
    midi_clock.pos = midi_clock.rendered = 0;
    midi_clock.have_pulse = 0;
    seq_start();
    seq_seek(midi_clock.spot * (MIDI_BEAT_UNITS / 4u));
}

static __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)
{
    if (status == 0xFAu) {
        midi_clock.spot_valid = midi_clock.seek_wait = 0;
        midi_clock.pos = midi_clock.rendered = 0;
        lfo_ext.err = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.count_valid = 0;
        midi_clock.interval_time = 0;
        midi_clock.start_ms = ms;
        seq_start();
    } else if (status == 0xFBu) {
        cin_left = 0;
        midi_clock.pos = midi_clock.rendered;
        lfo_ext.err = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.count_valid = 0;
        midi_clock.interval_time = 0;
        midi_clock.start_ms = ms;
        if (midi_clock.spot_valid && !song.playing) {
            midi_clock.spot_valid = 0;
            midi_clock.seek_wait = 1;                  /* (from the spot, on the first clock) */
            return;
        }
        if (!song.playing) {
            motion_begin();
            song.rec_wait = song.rec;
            song.rec_pass = 0;
        }
        song.playing = 1;
    } else if (status == 0xFCu) {
        midi_clock.seek_wait = 0;
        seq_stop();
    }
}

static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms, uint32_t time, uint16_t count)
{
    uint32_t ppqn = midi_ppqn(), quarter = ppqn / 4u, unit = MIDI_BEAT_UNITS / ppqn;   /* quarter: pulses a quarter beat */
    if (midi_clock.seek_wait) midi_clock_seek();
    if (midi_clock.ppqn != ppqn) {                  /* PPQN changed: measure the tempo again at the new rate */
        midi_clock.ppqn = (uint8_t)ppqn;
        midi_clock.tempo_valid = 0;
        midi_clock.tempo_dt = 0;
    }
    uint32_t pulses = midi_clock.count_valid ? (uint16_t)(count - midi_clock.pulse_count) : 1u;
    if (!pulses || (midi_clock.have_pulse && ms - midi_clock.last_ms > 500u)) pulses = 1u;
    midi_clock.pulse_count = count;
    midi_clock.count_valid = 1;
    /* Missing queue entries have no usable timing endpoints. Keep the last
     * interpolated rate and acquire tempo again from consecutive pulses. */
    if (pulses > 1u) midi_clock.tempo_valid = 0;
    if (!midi_clock.tempo_valid || ms - midi_clock.last_ms > 500u) {
        midi_clock.tempo_valid = 1;
        midi_clock.tempo_time = time;
        midi_clock.tempo_n = 0;
        midi_clock.tempo_dt = 0;
        midi_clock.tempo_count = 0;
    } else if (++midi_clock.tempo_n >= quarter) {   /* (a quarter beat: 6 pulses at 24) */
        uint32_t dt = time - midi_clock.tempo_time;
        midi_clock.tempo_time = time;
        midi_clock.tempo_n = 0;
        /* Allow endpoint jitter around the supported 40..240 BPM range. */
        if (dt >= 60u * MIDI_TICKS_PER_MS && dt <= 377u * MIDI_TICKS_PER_MS) {
            /* A quarter beat's clocks acquire tempo quickly, but their endpoint jitter must
             * not modulate the LFO or the rate between pulses. Smooth duration
             * over roughly two beats; a change above 6.25% locks immediately.
             * Keep eighth-tick precision even with millisecond host clocks.
             * Musical phase still advances by exactly 1024 units per clock. */
            uint32_t first = !midi_clock.tempo_dt;
            int32_t delta = (int32_t)(dt * 8u) - (int32_t)midi_clock.tempo_dt;
            uint32_t change = (uint32_t)(delta < 0 ? -delta : delta);
            if (first || change > midi_clock.tempo_dt / 16u) {
                midi_clock.tempo_dt = dt * 8u;
                midi_clock.tempo_count = 1;
            } else {
                /* Average acquisition windows before using the steady filter,
                 * so a delayed first pulse cannot bias the initial lock. */
                if (midi_clock.tempo_count < 8u) midi_clock.tempo_count++;
                midi_clock.tempo_dt = (uint32_t)((int32_t)midi_clock.tempo_dt + delta / midi_clock.tempo_count);
            }
            dt = midi_clock.tempo_dt / 8u;
            midi_clock.interval_time = dt / quarter;
            midi_beat_samples = (uint32_t)(((uint64_t)FS * dt) / (250u * MIDI_TICKS_PER_MS));
            /* +/- 0.875 BPM hysteresis keeps fractional master tempos from
             * repeatedly rolling the header digits across a rounding edge. */
            int32_t bpm = (int32_t)(((uint64_t)15000u * MIDI_TICKS_PER_MS * 256u + dt / 2u) / dt);
            int32_t shown = (int32_t)song.g[G_BPM] * 256;
            if (first || bpm > shown + 224 || bpm < shown - 224)
                song.g[G_BPM] = (int16_t)clamp((bpm + 128) / 256, 40, 240);
        }
    }
    if (song.playing) {
        if (midi_clock.have_pulse) {
            uint32_t interval = time - midi_clock.last_time;
            if (pulses == 1u && !midi_clock.tempo_dt && interval * quarter >= 48u * MIDI_TICKS_PER_MS &&
                interval * quarter <= 480u * MIDI_TICKS_PER_MS)   /* (8..80 ms a pulse at 24) */
                midi_clock.interval_time = interval;
            midi_clock.pos += pulses * unit;
        } else if (pulses > 1u) {
            /* Start's first clock is phase zero, even when it was discarded. */
            midi_clock.pos += (pulses - 1u) * unit;
        }
        midi_clock.have_pulse = 1;
    }
    if (!midi_clock.interval_time)
        midi_clock.interval_time = 60000u * MIDI_TICKS_PER_MS / ((uint32_t)song.g[G_BPM] * ppqn);
    midi_clock.last_ms = ms;
    midi_clock.last_time = time;
}

static uint32_t midi_clock_advance(uint32_t now)
{
    uint32_t target, elapsed, offset, n, unit = midi_pulse_units();
    if (!midi_clock.have_pulse)
        return 0;
    elapsed = now - midi_clock.last_time;
    /* The render timeline can precede a freshly received pulse. */
    if ((int32_t)elapsed < 0)
        elapsed = 0;
    if (elapsed > midi_clock.interval_time)
        elapsed = midi_clock.interval_time;
    if (elapsed <= 0xFFFFFFFFu / unit)              /* (always at 24: exact, as before) */
        offset = elapsed * unit / midi_clock.interval_time;
    else {                                          /* (a slow pulse at 4 or 8: 32-bit, no 64-bit divide on the FM-1) */
        uint32_t per = midi_clock.interval_time / unit;
        offset = elapsed / (per ? per : 1u);
    }
    if (offset >= unit)
        offset = unit - 1u;
    target = midi_clock.pos + offset;
    n = (int32_t)(target - midi_clock.rendered) > 0 ? target - midi_clock.rendered : 0u;
    midi_clock.rendered += n;
    return n;
}
