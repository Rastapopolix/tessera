/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's tracks 5..8 (platform/andamento/and_ext.c) on the PC, through the virtual prototype's own build
 * (web/andamento/sim.c, at 48 kHz) driven as the page drives it, the MIDI read as the page reads it (web_midi_take).
 *   and_ext_test [dir]   (run by tests/run_tests.sh; exit 1 on a failed check; with a dir, frames as dir/ppm/ext_*.ppm)
 * Checks:
 *   defaults  TURING, EUCLID, MARKOV, AWAKE on MIDI channels 5..8; stopped, nothing is sent
 *   play      a demo playing: every track sends notes on its own channel, on its DIV's grid, each note-on ended by its
 *             note-off before the next (a mono line), every note in track 1's scale; the CV is the note (1 V an octave,
 *             0 V at C2) and the gate high while it sounds
 *   STOP      every note off, every gate low, nothing more sent; PLAY again: the same notes from the top (the SEED)
 *   MUTE, CHAN OFF  a muted track sends nothing; CHAN OFF sends no MIDI but still drives its CV / gate
 *   ENERGY    sparse, a track with DEPTH 100 plays fewer notes than with DEPTH 0 (an anchor)
 *   keys      track 5 selected (FUNC + key 5): a key sends its note at once on channel 5, its release the note-off
 *   pages     track 5's GEN page: GEN and its settings, encoder 1 changes the generator; OUT: CHAN and the CV / gate;
 *             LEFT / RIGHT move through all 8 tracks
 *   core      tracks 1..4 play exactly the same with tracks 5..8 running (their notes and the audio, bit for bit) */
#include "../web/andamento/sim.c"

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const char *dir;
static void btn(uint32_t b) { web_button(b, 1); web_button(b, 0); }
static void func_key(uint32_t k) { web_button(AB_FUNC, 1); web_key(k, 1); web_key(k, 0); web_button(AB_FUNC, 0); }

/* the MIDI sent, per channel: note-ons, note-offs, the note held, faults */
static struct {
    uint32_t on[16], off[16], bad_off[16], double_on[16], out_of_scale;
    int held[16];
    uint32_t first[16][32], nfirst[16];          /* the first notes on each channel (PLAY again replays them) */
    uint32_t on_pos[16][64], npos[16];           /* the clock's position at each note-on */
} md;
static void midi_clear(void)
{
    uint32_t c;
    memset(&md, 0, sizeof md);
    for (c = 0; c < 16u; c++)
        md.held[c] = -1;
}
static void midi_read(void)
{
    uint32_t n = web_midi_take(), i;
    const uint8_t *m = web_midi_buf();
    for (i = 0; i < n; i++, m += 3) {
        uint32_t c = m[0] & 15u;
        if ((m[0] & 0xF0u) == 0x90u && m[2]) {
            if (md.held[c] >= 0)
                md.double_on[c]++;
            md.held[c] = m[1];
            if (md.nfirst[c] < 32u)
                md.first[c][md.nfirst[c]++] = m[1];
            if (md.npos[c] < 64u)
                md.on_pos[c][md.npos[c]++] = ae_pos();
            if (!((scale_mask(&trk[0]) >> ((m[1] + 120u - (uint32_t)trk[0].p[P_ROOT] % 12u) % 12u)) & 1u))
                md.out_of_scale++;
            md.on[c]++;
        } else if ((m[0] & 0xF0u) == 0x80u || (m[0] & 0xF0u) == 0x90u) {
            if (md.held[c] != (int)m[1])
                md.bad_off[c]++;
            md.held[c] = -1;
            md.off[c]++;
        }
    }
}
/* ms of device time, the MIDI read every millisecond (the page reads it every 2.7 ms) */
static uint32_t gate_bad;
static void run_ms(uint32_t ms)
{
    while (ms--) {
        uint32_t x;
        web_ms();
        midi_read();
        for (x = 0; x < AE_N; x++) {              /* the CV is the note sounding, the gate high while it sounds */
            const ae_track_t *a = &and_ext.t[x];
            if (a->sounding && a->cv_mv != (((int32_t)a->note - 36) * 1000 / 12 < 0 ? 0 : ((int32_t)a->note - 36) * 1000 / 12))
                gate_bad++;
            if (web_gate(x) != a->sounding)
                gate_bad++;
        }
    }
}
static void dump(const char *name)
{
    char p[256];
    FILE *f;
    const uint8_t *fb = web_screen();
    uint32_t i;
    if (!dir)
        return;
    snprintf(p, sizeof p, "%s/ppm/%s.ppm", dir, name);
    if (!(f = fopen(p, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", AND_W, AND_H);
    for (i = 0; i < AND_W * AND_H; i++) {
        uint32_t l = fb[i] & 15u;
        fputc((int)(255u * l / 15u), f);
        fputc((int)(208u * l / 15u), f);
        fputc((int)(32u * l / 15u), f);
    }
    fclose(f);
}

static int sounding_any(uint32_t tr)
{
    uint32_t v;
    for (v = 0; v < NVOICE; v++)
        if (trk[tr].v[v].gate)
            return 1;
    return 0;
}

/* a hash of tracks 1..4's notes (the note feed) and of their audio over ms of the demo from its start, in a copy of
 * this process (fork) so both runs begin from the very same state; ext 0: tracks 5..8 all OFF first */
static uint32_t core_run(uint32_t ms, int ext)
{
    int fd[2];
    uint32_t h = 2166136261u;
    pid_t child;
    if (pipe(fd) || (child = fork()) < 0)
        return 0;
    if (!child) {
        uint8_t seen = land_ev_at;
        uint32_t x;
        close(fd[0]);
        land_open = 1;
        if (!ext)
            for (x = 0; x < AE_N; x++)
                ae_set(AE_GEN, x, AE_OFF);
        web_demo(0);
        while (ms--) {
            uint32_t k;
            web_render(48);
            for (k = 0; k < 48u; k++) {
                union { float f; uint32_t u; } l = {out_l[k]}, r = {out_r[k]};
                h = (h ^ l.u) * 16777619u;
                h = (h ^ r.u) * 16777619u;
            }
            (void)web_midi_take();
            for (; seen != land_ev_at; seen++)
                h = (h ^ (land_ev_tr[seen & (LAND_EV - 1u)] << 8 | land_ev_note[seen & (LAND_EV - 1u)])) * 16777619u;
        }
        if (write(fd[1], &h, sizeof h) != sizeof h)
            _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &h, sizeof h) != sizeof h)
        h = 0;
    close(fd[0]);
    waitpid(child, 0, 0);
    return h;
}

int main(int argc, char **argv)
{
    uint32_t x, c, i, n_dense, n_sparse;
    dir = argc > 1 ? argv[1] : 0;
    web_boot();
    midi_clear();
    run_ms(200);

    /* defaults; stopped, silent */
    CHECK(and_ext.t[0].gen == AE_TURING && and_ext.t[1].gen == AE_EUCLID && and_ext.t[2].gen == AE_MARKOV &&
          and_ext.t[3].gen == AE_AWAKE, "defaults: TURING EUCLID MARKOV AWAKE");
    for (x = 0; x < AE_N; x++)
        CHECK(and_ext.t[x].chan == 5u + x, "track %u on channel %u (%u)", x + 5u, x + 5u, and_ext.t[x].chan);
    CHECK(!md.on[4] && !md.on[5] && !md.on[6] && !md.on[7], "stopped: nothing sent");

    /* playing: STILL TIDE */
    web_demo(0);
    run_ms(8000);
    CHECK(song.playing, "the demo plays");
    for (x = 0; x < AE_N; x++) {
        c = 4u + x;
        CHECK(md.on[c] >= 4u, "track %u: notes on channel %u (%u)", x + 5u, c + 1u, md.on[c]);
        CHECK(!md.double_on[c] && !md.bad_off[c], "track %u: one note at a time, each ended (%u, %u)", x + 5u,
              md.double_on[c], md.bad_off[c]);
        for (i = 0; i < md.npos[c]; i++)          /* (on the grid: within the block the step fell in) */
            if (md.on_pos[c][i] % (MIDI_BEAT_UNITS / AE_DIV_N[and_ext.t[x].div]) >
                (uint32_t)(MIDI_BEAT_UNITS * pid_get(0x100, 0) / 60u / 1000u) + 1u) {
                CHECK(0, "track %u: note %u off the grid (%u)", x + 5u, i, md.on_pos[c][i]);
                break;
            }
    }
    printf("notes in 8 s on channels 5..8: %u %u %u %u\n", md.on[4], md.on[5], md.on[6], md.on[7]);
    CHECK(!md.out_of_scale, "every note in track 1's scale (%u outside)", md.out_of_scale);
    CHECK(!gate_bad, "the CV the note, the gate high while it sounds (%u wrong)", gate_bad);
    for (c = 0; c < 16u; c++)
        if (c < 4u || c > 7u)
            CHECK(!md.on[c], "nothing on channel %u (%u)", c + 1u, md.on[c]);
    and_ui.page = AND_HOME;
    dump("ext_home");

    /* STOP; PLAY again replays from the SEED */
    {
        uint32_t first[4][8];
        for (x = 0; x < AE_N; x++)
            memcpy(first[x], md.first[4u + x], sizeof first[x]);
        btn(AB_STOP);
        run_ms(100);
        for (x = 0; x < AE_N; x++)
            CHECK(md.held[4u + x] < 0 && !web_gate(x), "STOP: track %u's note off, gate low", x + 5u);
        midi_clear();
        run_ms(1000);
        CHECK(!md.on[4] && !md.on[5] && !md.on[6] && !md.on[7], "stopped: nothing sent");
        btn(AB_PLAY);
        run_ms(3000);
        for (x = 0; x < AE_N; x++)
            CHECK(md.nfirst[4u + x] >= 3u && !memcmp(first[x], md.first[4u + x], 3u * sizeof(uint32_t)),
                  "PLAY again: track %u's first notes the same", x + 5u);
    }

    /* MUTE, CHAN OFF */
    ae_set(AE_MUTE, 0, 1);
    ae_set(AE_CHAN, 1, 0);
    run_ms(50);
    midi_clear();
    gate_bad = 0;
    {
        uint32_t gates = 0;
        for (i = 0; i < 4000u; i++) {
            run_ms(1);
            gates += (uint32_t)web_gate(1);
        }
        CHECK(!md.on[4] && !web_gate(0), "MUTE: track 5 sends nothing (%u)", md.on[4]);
        CHECK(!md.on[5] && gates > 0u, "CHAN OFF: track 6 no MIDI (%u), its gate still played (%u ms)", md.on[5], gates);
        CHECK(md.on[6] > 0u, "track 7 plays on");
    }
    ae_set(AE_MUTE, 0, 0);
    ae_set(AE_CHAN, 1, 6);

    /* ENERGY: sparse, DEPTH 100 against 0 (TURING, every step a draw: SERIES plays every step) */
    pid_set(0x26c, 0, -100);                         /* (ENERGY LEVEL: all sparse) */
    pid_set(0x259, 0, 0);
    ae_set(AE_GEN, 3, AE_SERIES);
    ae_set(AE_DEPTH, 3, 0);
    midi_clear();
    run_ms(6000);
    n_dense = md.on[7];
    ae_set(AE_DEPTH, 3, 100);
    midi_clear();
    run_ms(6000);
    n_sparse = md.on[7];
    CHECK(n_dense > 0u && n_sparse * 10u < n_dense * 6u && n_sparse > 0u,
          "ENERGY sparse: DEPTH 100 leaves notes out (%u of %u)", n_sparse, n_dense);
    pid_set(0x26c, 0, 0);
    ae_set(AE_GEN, 3, AE_AWAKE);

    /* a key on track 5 */
    btn(AB_STOP);
    run_ms(100);
    func_key(4);
    CHECK(AND_SEL == 4u && and_ui.ext == 0, "FUNC + key 5 selects track 5 (%u)", AND_SEL);
    func_key(8);                                     /* (SCALE) */
    midi_clear();
    web_key(0, 1);
    midi_read();
    CHECK(md.on[4] == 1u && md.held[4] == (int)ak_key_note(4, 0, AK_SCALE), "a key: its note on channel 5 at once (%d)",
          md.held[4]);
    CHECK(web_gate(0) && and_ext_cv(0) == (md.held[4] - 36) * 1000 / 12, "a key: gate high, CV its note");
    for (i = 0; i < 3u; i++)
        CHECK(!sounding_any(i), "a key on track 5: no voice of track %u sounds", i + 1u);
    web_key(0, 0);
    midi_read();
    CHECK(md.off[4] == 1u && md.held[4] < 0 && !web_gate(0), "the key let go: its note-off, gate low");

    /* track 5's pages */
    btn(AB_GEN);
    and_cells();
    CHECK(and_ui.cell[0] == AE_PID + AE_GEN && and_ui.cell[1] == AE_PID + AE_P1, "track 5's GEN: GEN and its settings");
    dump("ext_gen");
    web_enc(0, 1);
    CHECK(and_ext.t[0].gen == AE_EUCLID, "encoder 1 on GEN: the generator (%u)", and_ext.t[0].gen);
    web_enc(0, -1);
    btn(AB_SOUND);
    and_cells();
    CHECK(and_ui.cell[0] == AE_PID + AE_CHAN, "track 5's SOUND page is OUT: CHAN");
    btn(AB_PLAY);
    for (i = 0; i < 2000u && !web_gate(0); i++)
        run_ms(1);
    dump("ext_out");
    btn(AB_STOP);
    run_ms(50);
    for (i = 0; i < 8u; i++)
        btn(AB_RIGHT);
    CHECK(AND_SEL == 4u, "RIGHT 8 times: round the 8 tracks back to 5 (%u)", AND_SEL);
    btn(AB_RIGHT);
    btn(AB_RIGHT);
    btn(AB_RIGHT);
    btn(AB_RIGHT);
    CHECK(AND_SEL == 0u && song.sel == 0u && and_ui.ext < 0, "RIGHT from 8: track 1");
    btn(AB_NO);

    /* tracks 1..4 the same with tracks 5..8 playing as with them OFF */
    {
        uint32_t h1 = core_run(8000, 1), h2 = core_run(8000, 0), h3 = core_run(8000, 1);
        CHECK(h1 && h1 == h3, "the demo's run repeats bit for bit (%08x %08x)", h1, h3);
        CHECK(h1 == h2, "tracks 1..4: the same notes and audio with or without 5..8 (%08x %08x)", h1, h2);
    }

    printf(nfail ? "and_ext: %u failed\n" : "and_ext: all checks passed\n", nfail);
    return nfail ? 1 : 0;
}
