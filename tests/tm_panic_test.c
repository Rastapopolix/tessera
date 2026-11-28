/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI's MIDI PANIC (midi_out.c midi_out_panic): every note sent ends, then CC 123, 120 and 121 on all 16
 * channels, all of it reaching a slow device. Its keys: STOP (PLAY) held on 0.7 s, GLO + PLAY while stopped; a STOP
 * tapped and GLO + PLAY while playing (RESTART) send none of it. Built as Tessera MIDI by tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>

#define LOG_MAX 32768u
static uint32_t log_pkt[LOG_MAX], log_n, drain_max = ~0u;

static void drain(void)
{
    uint32_t n = 0;
    while (mo_r != mo_w && n < drain_max) {
        if (log_n < LOG_MAX) log_pkt[log_n++] = midi_out_q[mo_r % MQ];
        mo_r++; n++;
    }
}
static void blocks(uint32_t n) { while (n--) { events_block(CTL); mfx_tick(CTL); drain(); } }
static void drain_all(void) { drain_max = ~0u; blocks(64); }
static uint32_t left_on(void)
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
    return n;
}
static uint32_t count_cc(uint32_t cc)              /* channels that got it */
{
    uint32_t m = 0;
    for (uint32_t i = 0; i < log_n; i++)
        if (((log_pkt[i] >> 8) & 0xF0u) == 0xB0u && ((log_pkt[i] >> 16) & 0x7Fu) == cc) m |= 1u << ((log_pkt[i] >> 8) & 15u);
    return (uint32_t)__builtin_popcount(m);
}
static void play(void)                            /* 8 tracks of four-note chords, a new one every step */
{
    ui_power_on();
    memset(&midi_clock, 0, sizeof midi_clock); midi_sync_active = 0;
    mi_r = mi_w = 0; mo_r = mo_w = 0; usb.config = 1;
    song.g[G_ROUTE] = 0; settings_midi_out = MIDI_OUT_ALL;
    memset(tessera.midi_ch, 0, sizeof tessera.midi_ch);
    for (uint32_t i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 8; trk[i].p[P_SGATE] = 128;
        for (uint32_t s = 0; s < 8u; s++) {
            uint8_t r = (uint8_t)(36u + i * 7u + s * 2u);
            trk[i].step[s] = (step_t){{r, (uint8_t)(r + 4u), (uint8_t)(r + 7u), (uint8_t)(r + 11u)}, 4, ST_NOTE, 0, 0, 100};
        }
    }
    drain_max = ~0u;
    events_block(CTL); drain(); log_n = 0;
    transport_req = 1; blocks(beat_samples() / CTL * 2u);
}

static int mfails;
static void mcheck(const char *what, int ok) { printf("%-72s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }

int main(void)
{
    char line[96];
    /* the panic itself, while playing, to a device taking 16 events a block */
    play();
    drain_max = 16;
    midi_out_panic();
    transport_req = 2; blocks(2); drain_all();
    snprintf(line, sizeof line, "PANIC to a slow device: every note ends (%u left on)", (unsigned)left_on());
    mcheck(line, !left_on());
    mcheck("  CC 123, 120 and 121 on all 16 channels", count_cc(123) == 16u && count_cc(120) == 16u && count_cc(121) == 16u);

    /* STOP tapped: no panic (CC 123 only on the channels played, no 120 / 121) */
    play();
    press(B_PLAY); blocks(4); drain_all();
    mcheck("STOP tapped: notes end, CC 123 on the 8 played, no panic", !left_on() && count_cc(123) == 8u && !count_cc(120));

    /* STOP held on 0.7 s: the panic */
    play();
    hold(B_PLAY); blocks(4); drain_all();
    mcheck("STOP held on: the panic (CC 120 on all 16)", !song.playing && !left_on() && count_cc(120) == 16u && count_cc(121) == 16u);

    /* GLO + PLAY while stopped: the panic; while playing: RESTART, no panic */
    play();
    btn_down(B_GLO); frames(800);
    press(B_PLAY);
    btn_up(B_GLO); frames(100); blocks(4);
    mcheck("GLO + PLAY while playing: RESTART, no panic", song.playing && !count_cc(120));
    press(B_PLAY); blocks(4); drain_all();
    log_n = 0;
    btn_down(B_GLO); frames(800);
    press(B_PLAY);
    btn_up(B_GLO); frames(100); blocks(4); drain_all();
    mcheck("GLO + PLAY while stopped: the panic, nothing starts", !song.playing && count_cc(120) == 16u && count_cc(123) == 16u);

    printf("%s\n", mfails ? "FAILED" : "all ok");
    return mfails != 0;
}
