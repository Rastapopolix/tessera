/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* STEP DIR (firmware/src/seq_dir.c), heard: the notes a STEP pattern plays, through the real sequencer and MIDI OUT
 * (ALL), each step with its own note. PEND and PONG play the steps' notes back the way they came (the note heard is the
 * step under the playhead), BACK too; a TIE going backwards: the tied run plays once, from the run's last step, as long
 * as the run (not a note per step).
 * Run by tests/run_tests.sh (needs build/gen from one firmware build). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdlib.h>

#define LOG_MAX 4096u
static uint32_t log_pkt[LOG_MAX], log_blk[LOG_MAX], log_idx[LOG_MAX], log_n, blk_n, step_at[64];

static void drain(void)
{
    while (mo_r != mo_w) {
        if (log_n < LOG_MAX) {
            log_pkt[log_n] = midi_out_q[mo_r % MQ];
            log_blk[log_n] = blk_n;
            log_idx[log_n] = trk[0].seq_idx;
            log_n++;
        }
        mo_r++;
    }
}
static void blocks(uint32_t n) { while (n--) { events_block(CTL); blk_n++; drain(); } }
/* PLAY, on until step `steps` (since PLAY) begins: step_at[k] the block step k began in */
static void play_steps(uint32_t steps)
{
    uint32_t was = ~0u;
    transport_req = 1;
    for (uint32_t guard = 0; guard < 100000u; guard++) {
        blocks(1);
        if (song.playing && seq_run_step[0] != was) {
            was = seq_run_step[0];
            if (was < 64u) step_at[was] = blk_n;
            if (was >= steps) return;
        }
    }
}

static void reset(uint32_t d, uint32_t len)
{
    ui_power_on();
    for (uint32_t i = 0; i < NTRK; i++) memset(trk[i].step, 0, sizeof trk[i].step);
    track_defaults_steps(&trk[0]);
    trk[0].p[P_SLEN] = (int32_t)len;
    for (uint32_t i = 0; i < len; i++) trk[0].step[i] = (step_t){{(uint8_t)(60u + i)}, 1, ST_NOTE, 0, 0, 96};
    tessera_param_set(0, TX_SEQ_DIR, (int32_t)d);
    mi_r = mi_w = 0; mo_r = mo_w = 0; usb.config = 1;
    fm1_in.notes = kb_prev = 0; song.sel = 0; song.g[G_ROUTE] = 0;
    settings_midi_out = MIDI_OUT_ALL; settings_clock_out = 0;
    memset(tessera.midi_ch, 0, sizeof tessera.midi_ch);
    memset(tessera.midi_tr, 0, sizeof tessera.midi_tr);
    memset(tessera.sound_off, 0, sizeof tessera.sound_off);
    events_block(CTL); drain(); log_n = 0; blk_n = 0;
}
static void stop(void) { transport_req = 2; blocks(2); }

/* the track 1 note-ons, in order (their notes; at most max); idx: the playhead (seq_idx) when each went out */
static uint32_t ons(uint8_t *note, uint8_t *idx, uint32_t *blk, uint32_t max)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < log_n && n < max; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu;
        if (s != 0x90u || !(log_pkt[i] >> 24)) continue;
        note[n] = (uint8_t)((log_pkt[i] >> 16) & 0x7Fu);
        if (idx) idx[n] = (uint8_t)log_idx[i];
        if (blk) blk[n] = log_blk[i];
        n++;
    }
    return n;
}
/* the note-off of the note-on k (its block), or ~0 */
static uint32_t off_blk(uint32_t k)
{
    uint32_t n = 0, i, note = 0;
    for (i = 0; i < log_n; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu;
        if (s == 0x90u && (log_pkt[i] >> 24)) { if (n++ == k) { note = (log_pkt[i] >> 16) & 0x7Fu; break; } }
    }
    for (i++; i < log_n; i++) {
        uint32_t s = (log_pkt[i] >> 8) & 0xFFu, d1 = (log_pkt[i] >> 16) & 0x7Fu;
        if (((s & 0xF0u) == 0x80u || ((s & 0xF0u) == 0x90u && !(log_pkt[i] >> 24))) && d1 == note) return log_blk[i];
    }
    return ~0u;
}
static void show(const char *what, const uint8_t *got, uint32_t n)
{
    printf("  %s:", what);
    for (uint32_t i = 0; i < n; i++) printf(" %u", got[i] - 60u + 1u);
    printf("\n");
}

static int heard_test(void)
{
    static const uint8_t WANT[4][12] = {{0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1}, {4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3},
                                        {0, 1, 2, 3, 4, 3, 2, 1, 0, 1, 2, 3}, {0, 1, 2, 3, 4, 4, 3, 2, 1, 0, 0, 1}};
    static const char *const NAME[4] = {"FWD", "BACK", "PEND", "PONG"};
    uint8_t got[64], idx[64];
    char what[96];
    int bad = 0;
    for (uint32_t d = 0; d < 4u; d++) {
        reset(d, 5);
        play_steps(12);
        uint32_t n = ons(got, idx, 0, 64), ok = n >= 12u;
        stop();
        for (uint32_t i = 0; i < 12u && i < n; i++) ok &= got[i] == 60u + WANT[d][i] && idx[i] == WANT[d][i];
        if (!ok) show(NAME[d], got, n < 16u ? n : 16u);
        snprintf(what, sizeof what, "DIR %s heard: each step's own note, the playhead's, over two passes", NAME[d]);
        bad += check(what, ok);
    }
    return bad;
}

/* a run of steps 2-4 (index 1..3) tied: 61 then TIE TIE. FWD: one 61, three steps long. BACK: the run from its last
 * step: one 61 at index 3, three steps long, nothing new at 2 and 1. PEND: once each way, three steps each. */
static int tie_test(void)
{
    uint8_t got[64];
    uint32_t blk[64], sb, n, ok, k, on61 = 0;
    int bad = 0;
    for (uint32_t d = SEQ_FWD; d <= SEQ_PEND; d++) {
        reset(d, 5);
        trk[0].step[2].time = trk[0].step[3].time = ST_TIE;
        play_steps(d == SEQ_PEND ? 9 : 5);
        sb = step_at[1] - step_at[0];
        n = ons(got, 0, blk, 64);
        while (n && blk[n - 1] >= step_at[d == SEQ_PEND ? 9 : 5]) n--;   /* (the steps asked for: not the next one's) */
        stop();
        if (getenv("SDT_V")) for (k = 0; k < n; k++) printf("  d%u on %u blk %u off %u (sb %u)\n", d, got[k], blk[k], off_blk(k), sb);
        for (k = 0, on61 = 0, ok = 1; k < n; k++) {
            if (got[k] == 61u) {
                uint32_t off = off_blk(k), len = off == ~0u ? 0u : off - blk[k];
                on61++;
                ok &= len + 2u >= 3u * sb && len <= 3u * sb + 2u;   /* (three steps: the run's last held to its end) */
            } else ok &= got[k] == 60u || got[k] == 64u;
        }
        if (d == SEQ_FWD) {
            ok &= n == 3u && got[0] == 60u && got[1] == 61u && got[2] == 64u;
            bad += check("TIE FWD: steps 2-4 one note, three steps long", ok && on61 == 1u);
        } else if (d == SEQ_BACK) {
            ok &= n == 3u && got[0] == 64u && got[1] == 61u && got[2] == 60u && blk[1] == step_at[1];
            if (!ok) show("BACK tie", got, n < 16u ? n : 16u);
            bad += check("TIE BACK: the run once, from its last step (step 4), three steps long", ok && on61 == 1u);
        } else {
            ok &= n == 5u && got[0] == 60u && got[1] == 61u && got[2] == 64u && got[3] == 61u && got[4] == 60u;
            if (!ok) show("PEND tie", got, n < 16u ? n : 16u);
            bad += check("TIE PEND: the run once each way, three steps long each time", ok && on61 == 2u);
        }
    }
    return bad;
}

/* a live-recorded take (recording.c: timed notes, the steps SF_RECORDED views of them), each step its own note: the
 * same order as typed steps */
static int recorded_test(void)
{
    static const uint8_t WANT[4][12] = {{0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1}, {4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3},
                                        {0, 1, 2, 3, 4, 3, 2, 1, 0, 1, 2, 3}, {0, 1, 2, 3, 4, 4, 3, 2, 1, 0, 0, 1}};
    static const char *const NAME[4] = {"FWD", "BACK", "PEND", "PONG"};
    uint8_t got[64], idx[64];
    char what[96];
    int bad = 0;
    for (uint32_t d = 0; d < 4u; d++) {
        reset(d, 5);
        memset(recording, 0, sizeof recording);
        for (uint32_t i = 0; i < 5u; i++) {
            recording[i] = (recorded_note_t){0, RECORD_UNIT / 4u, (uint8_t)(60u + i), 96, 0, (uint8_t)i, 0, 0};
            trk[0].step[i].flags |= SF_RECORDED;
        }
        recording_reindex();
        play_steps(12);
        uint32_t n = ons(got, idx, 0, 64), ok = n >= 12u;
        stop();
        for (uint32_t i = 0; i < 12u && i < n; i++) ok &= got[i] == 60u + WANT[d][i] && idx[i] == WANT[d][i];
        if (!ok) show(NAME[d], got, n < 16u ? n : 16u);
        snprintf(what, sizeof what, "DIR %s, a recorded take: each step's own note, over two passes", NAME[d]);
        bad += check(what, ok);
        memset(recording, 0, sizeof recording);
        recording_reindex();
    }
    return bad;
}

int main(void)
{
    int bad = 0;
    bad += heard_test();
    bad += recorded_test();
    bad += tie_test();
    printf(bad ? "seq_dir_test: %d FAILED\n" : "seq_dir_test: all passed\n", bad);
    return bad ? 1 : 0;
}
