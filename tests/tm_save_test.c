/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI: what a project keeps of each track's MIDI (MIDI_ONLY.md "What is saved where"). Tracks 1 and 7 get
 * values off their defaults (channel, TRANSPOSE, DRUM MAP; ECHO, FEEL; ENV CC, LFO CC; ENV; VOICE, PRIO; LEVEL, PAN),
 * the project is saved, everything changed, the project loaded: each value is back, on its own track. Then the keys:
 * a key on track 7 goes out on track 7's channel, moved by its TRANSPOSE. Built as Tessera MIDI by tests/run_tests.sh */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <stdio.h>
#define KEY_C4 7u                                       /* keys from F3 (0): C4 7 (chord_test.c) */

static const uint8_t IDS[] = {P_MECHO, P_METIME, P_MEFADE, P_MEPIT, P_MSTRUM, P_MSDIR, P_MVEL, P_MHUMAN,
                              P_ECC, P_EDEPTH, P_EBASE, P_LCC, P_LDEPTH, P_LBASE,
                              P_ATK, P_DEC, P_SUS, P_REL, P_VOICE, P_PRIO, P_LEVEL, P_PAN};
static int16_t want[2][sizeof IDS];
static const uint32_t TRK_OF[2] = {0, 6};

static int mfails;
static void mcheck(const char *what, int ok) { printf("%-72s %s\n", what, ok ? "ok" : "FAIL"); mfails += !ok; }

static int16_t off_default(uint32_t id, uint32_t salt)  /* a value in range, not the default */
{
    const param_desc_t *d = track_desc(&trk[0], id);
    int32_t span = d->max - d->min, v = d->min + (int32_t)((salt * 7u + 3u) % (uint32_t)(span > 0 ? span : 1));
    if (v == d->def) v = v < d->max ? v + 1 : v - 1;
    return (int16_t)v;
}

int main(void)
{
    uint32_t j, i, bad = 0;
    char line[96];
    ui_power_on();
    tessera_ensure_init();                              /* (as at boot: before the values are set) */
    usb.config = 1;
    settings_midi_out = MIDI_OUT_ALL;
    for (j = 0; j < 2u; j++) {
        uint32_t k = TRK_OF[j];
        for (i = 0; i < sizeof IDS; i++) want[j][i] = trk[k].p[IDS[i]] = off_default(IDS[i], i + 11u * j);
        tessera.midi_ch[k] = (uint8_t)(3u + 5u * j);    /* channel 3 / 8 (1..16; 0 AUTO) */
        tessera.midi_tr[k] = (int8_t)(j ? -5 : 7);
        tessera.midi_dmap[k] = (uint8_t)j;
    }
    project_save(1);
    for (j = 0; j < 2u; j++) {                          /* everything changed */
        uint32_t k = TRK_OF[j];
        for (i = 0; i < sizeof IDS; i++) trk[k].p[IDS[i]] = track_desc(&trk[k], IDS[i])->def;
        tessera.midi_ch[k] = 0; tessera.midi_tr[k] = 0; tessera.midi_dmap[k] = 0;
    }
    project_load(1);
    for (i = 0; i < 40u; i++) { events_block(CTL); frame(); }   /* (a load may finish in the audio ISR's blocks) */
    for (j = 0; j < 2u; j++) {
        uint32_t k = TRK_OF[j], n = 0;
        for (i = 0; i < sizeof IDS; i++)
            if (trk[k].p[IDS[i]] != want[j][i]) {
                if (n++ < 4u) printf("  T%u %s: %d, saved %d\n", (unsigned)k + 1u, track_desc(&trk[k], IDS[i])->label,
                                     trk[k].p[IDS[i]], want[j][i]);
                bad++;
            }
        snprintf(line, sizeof line, "T%u: ECHO, FEEL, ENV / LFO CC, ENV, VOICE, LEVEL, PAN back after a load", (unsigned)k + 1u);
        mcheck(line, !n);
        snprintf(line, sizeof line, "T%u: its channel, TRANSPOSE and DRUM MAP back", (unsigned)k + 1u);
        mcheck(line, tessera.midi_ch[k] == 3u + 5u * j && tessera.midi_tr[k] == (j ? -5 : 7) && tessera.midi_dmap[k] == j);
    }
    /* the keys on track 7: its channel (8), its TRANSPOSE (-5), no MIDI FX in the way */
    for (i = 0; i < sizeof IDS; i++) trk[6].p[IDS[i]] = track_desc(&trk[6], IDS[i])->def;
    trk[6].p[P_MUTE] = 0;
    song.sel = 6;
    mo_r = mo_w;
    key_down(KEY_C4);
    {
        uint32_t got = 0, ch = 99, note = 999;
        while (mo_r != mo_w) {
            uint32_t e = midi_out_q[mo_r++ % MQ];
            if (((e >> 8) & 0xF0u) == 0x90u && (e >> 24)) { got++; ch = (e >> 8) & 15u; note = (e >> 16) & 127u; }
        }
        snprintf(line, sizeof line, "a key on T7: out on its channel 8, C4 moved -5 (got ch %u note %u)", (unsigned)ch + 1u, (unsigned)note);
        mcheck(line, got == 1u && ch == 7u && note == 60u - 5u);
    }
    key_up(KEY_C4);
    printf("%s\n", mfails ? "FAILED" : "all ok");
    return mfails != 0;
}
