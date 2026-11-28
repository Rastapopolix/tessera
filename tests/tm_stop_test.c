/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI: STOP leaves no note hanging (Theo on tm2: PLAY|STOP left notes on a Digitone). Eight tracks of
 * four-note chords, every step a new chord at a full gate, so a step's 32 offs and 32 ons go into the USB queue
 * (usb.c midi_out_q, MQ 64) in one block. The device side drains it slowly, as a host port does (one USB packet of
 * 16 events in flight, the device NAKing for a while), and still every note-on gets its note-off, after it, on its
 * channel; STOP ends with All Notes Off (CC 123) on every channel that played. Built as Tessera MIDI by
 * tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>

#define LOG_MAX 32768u
static uint32_t log_pkt[LOG_MAX], log_n, drain_max = ~0u, stall;

static void drain(void)                           /* the device side: at most drain_max events a block, none while stalled */
{
    uint32_t n = 0;
    if (stall) { stall--; return; }
    while (mo_r != mo_w && n < drain_max) {
        if (log_n < LOG_MAX) log_pkt[log_n++] = midi_out_q[mo_r % MQ];
        mo_r++; n++;
    }
}
static void blocks(uint32_t n) { while (n--) { events_block(CTL); mfx_tick(CTL); drain(); } }   /* (mix.c's order) */
static void drain_all(void) { drain_max = ~0u; stall = 0; blocks(64); }

/* every note-on was ended, after it, by an off on its channel (a note-on of velocity 0 counts as an off) */
static int balanced(uint32_t *left)
{
    static uint16_t on[16][128];
    uint32_t c, k, n = 0;
    memset(on, 0, sizeof on);
    for (uint32_t i = 0; i < log_n; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu, d2 = log_pkt[i] >> 24;
        if ((s & 0xF0u) == 0x90u && d2) on[s & 15u][d1]++;
        else if (((s & 0xF0u) == 0x80u || (s & 0xF0u) == 0x90u) && on[s & 15u][d1]) on[s & 15u][d1]--;
    }
    for (c = 0; c < 16u; c++)
        for (k = 0; k < 128u; k++) n += on[c][k];
    if (left) *left = n;
    return !n;
}
static uint32_t count_cc(uint32_t cc)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < log_n; i++)
        if (((log_pkt[i] >> 8) & 0xF0u) == 0xB0u && ((log_pkt[i] >> 16) & 0x7Fu) == cc) n++;
    return n;
}
static uint32_t count_on(void)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < log_n; i++)
        n += ((log_pkt[i] >> 8) & 0xF0u) == 0x90u && (log_pkt[i] >> 24);
    return n;
}

static void reset(void)
{
    ui_power_on();
    memset(&midi_clock, 0, sizeof midi_clock); midi_sync_active = 0;
    mi_r = mi_w = 0; mo_r = mo_w = 0; usb.config = 1;
    song.g[G_ROUTE] = 0; settings_midi_out = MIDI_OUT_ALL; settings_clock_out = 1;
    memset(tessera.midi_ch, 0, sizeof tessera.midi_ch);
    memset(tessera.midi_tr, 0, sizeof tessera.midi_tr);
    for (uint32_t i = 0; i < NTRK; i++) {               /* each track: a new four-note chord on every step, held to the next */
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 8;
        trk[i].p[P_SGATE] = 128;
        for (uint32_t s = 0; s < 8u; s++) {
            uint8_t r = (uint8_t)(36u + i * 7u + s * 2u);
            trk[i].step[s] = (step_t){{r, (uint8_t)(r + 4u), (uint8_t)(r + 7u), (uint8_t)(r + 11u)}, 4, ST_NOTE, 0, 0, 100};
        }
    }
    drain_max = ~0u; stall = 0;
    events_block(CTL); drain(); log_n = 0;
}

static int mfails;
static void mcheck(const char *what, int ok) { printf("%-72s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }

int main(void)
{
    uint32_t bb, left = 0, ons;
    char line[96];
    reset();
    bb = beat_samples() / CTL;
    /* 1: the device takes everything at once */
    transport_req = 1; blocks(3u * bb + bb / 3u);
    transport_req = 2; blocks(2); drain_all();
    ons = count_on();
    mcheck("8 tracks of chords, a fast device: notes went out", ons >= 8u * 4u * 3u);
    snprintf(line, sizeof line, "  STOP: every note-on has its off (%u left on)", (unsigned)(balanced(&left), left));
    mcheck(line, balanced(0));
    mcheck("  STOP: All Notes Off (CC 123) on the 8 channels that played", count_cc(123) == 8u);

    /* 2: a host port's pace, one packet of 16 events a block */
    reset();
    drain_max = 16;
    transport_req = 1; blocks(3u * bb + bb / 3u);
    transport_req = 2; blocks(2); drain_all();
    snprintf(line, sizeof line, "a slow device (16 events a block): every on has its off (%u left on)", (unsigned)(balanced(&left), left));
    mcheck(line, balanced(0));

    /* 3: the device NAKs for 20 ms just before STOP */
    reset();
    drain_max = 16;
    transport_req = 1; blocks(3u * bb + bb / 3u);
    stall = 28;
    transport_req = 2; blocks(2); drain_all();
    snprintf(line, sizeof line, "a stalled device at STOP: every on has its off (%u left on)", (unsigned)(balanced(&left), left));
    mcheck(line, balanced(0));

    /* 4: a stall mid-play, a new chord every step while the queue is full, then STOP */
    reset();
    drain_max = 16;
    transport_req = 1; blocks(bb);
    stall = 3u * bb;
    blocks(3u * bb);
    transport_req = 2; blocks(2); drain_all();
    snprintf(line, sizeof line, "a device stalled while playing: every on has its off (%u left on)", (unsigned)(balanced(&left), left));
    mcheck(line, balanced(0));

    /* 5: the off before a note's next on, never after it (a retriggered note is not cut) */
    {
        static uint8_t on[16][128];
        int order = 1;
        reset();
        drain_max = 16;
        transport_req = 1; blocks(bb);
        stall = bb;
        blocks(3u * bb);
        transport_req = 2; blocks(2); drain_all();
        memset(on, 0, sizeof on);
        for (uint32_t i = 0; i < log_n; i++) {
            uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu, d2 = log_pkt[i] >> 24;
            if ((s & 0xF0u) == 0x90u && d2) { if (on[s & 15u][d1]) order = 0; on[s & 15u][d1] = 1; }
            else if ((s & 0xF0u) == 0x80u) on[s & 15u][d1] = 0;
        }
        mcheck("  a note never starts again before its off went out", order);
    }
    printf("%s\n", mfails ? "FAILED" : "all ok");
    return mfails != 0;
}
