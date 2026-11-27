/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CLOCK OUT, end to end over the built Cosmati.clap as a minimal CLAP host (POSIX), with CLOCK OUT MIDI and PULSE
 * on (the setting, in a settings folder of the test's own):
 *   - in a DAW (a transport at 120 BPM from its zero): the CLOCK port pulses every sixteenth (6000 frames at 48 kHz)
 *     from the plugin's latency on (the frame its audio of beat 0 comes out), the reset with the first; no MIDI clock
 *     goes out (the DAW keeps time);
 *   - in the standalone app (no transport; a fresh core): START, then MIDI clock 24 a beat, each clock at its frame (spaced to a
 *     millisecond, the core's step), and every sixth on a CLOCK pulse to a millisecond.
 * usage: tessera-clock-test path/to/Cosmati.clap */
#include <clap/clap.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static int fails = 0;
#define CHECK(c, ...)                          \
    do                                         \
    {                                          \
        if (!(c))                              \
        {                                      \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                 \
            fails++;                           \
        }                                      \
    } while (0)

static const void *h_get_extension(const clap_host_t *, const char *) { return nullptr; }
static void h_nop(const clap_host_t *) {}
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-clock-test", "Tessera", "", "0",
                                 h_get_extension, h_nop, h_nop, h_nop};

static uint32_t in_size(const clap_input_events_t *) { return 0; }
static const clap_event_header_t *in_get(const clap_input_events_t *, uint32_t) { return nullptr; }

// MIDI out: (status, the frame from the first block's start)
struct Midi
{
    int64_t base = 0;
    std::vector<std::pair<uint8_t, int64_t>> ev;
};
static bool out_push(const clap_output_events_t *l, const clap_event_header_t *h)
{
    auto *m = static_cast<Midi *>(l->ctx);
    if (h->space_id == CLAP_CORE_EVENT_SPACE_ID && h->type == CLAP_EVENT_MIDI)
        m->ev.push_back({reinterpret_cast<const clap_event_midi_t *>(h)->data[0], m->base + h->time});
    return true;
}

constexpr uint32_t RATE = 48000, BLOCK = 480, NPORTS = 10, CLOCK = 9;

// blocks through one instance (tr: the DAW's transport, playing from 0 at 120; null: the standalone app); the CLOCK
// port's two channels, MIDI out; false if the instance would not start
static bool play(const clap_plugin_factory_t *fac, const char *id, bool daw, uint32_t blocks, std::vector<float> &clk,
                 std::vector<float> &rst, Midi &midi, uint32_t *latency)
{
    const clap_plugin_t *p = fac->create_plugin(fac, &host, id);
    if (!p || !p->init(p) || !p->activate(p, RATE, BLOCK, BLOCK) || !p->start_processing(p)) return false;
    auto *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
    *latency = lat ? lat->get(p) : 0;
    static float bufs[NPORTS][2][BLOCK];
    float *chs[NPORTS][2];
    clap_audio_buffer_t outs[NPORTS] = {};
    for (uint32_t k = 0; k < NPORTS; k++)
    {
        chs[k][0] = bufs[k][0];
        chs[k][1] = bufs[k][1];
        outs[k].data32 = chs[k];
        outs[k].channel_count = 2;
    }
    clap_event_transport_t tr = {};
    tr.header = {sizeof tr, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
    tr.tempo = 120.0;
    clap_input_events_t in = {nullptr, in_size, in_get};
    clap_output_events_t oe = {&midi, out_push};
    clap_process_t proc = {};
    proc.frames_count = BLOCK;
    proc.transport = daw ? &tr : nullptr;
    proc.audio_outputs = outs;
    proc.audio_outputs_count = NPORTS;
    proc.in_events = &in;
    proc.out_events = &oe;
    double beats = 0;
    for (uint32_t b = 0; b < blocks; b++)
    {
        tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_IS_PLAYING;
        tr.song_pos_beats = (clap_beattime)std::llround(beats * CLAP_BEATTIME_FACTOR);
        midi.base = (int64_t)b * BLOCK;
        p->process(p, &proc);
        proc.steady_time += BLOCK;
        beats += BLOCK * tr.tempo / 60.0 / RATE;
        clk.insert(clk.end(), bufs[CLOCK][0], bufs[CLOCK][0] + BLOCK);
        rst.insert(rst.end(), bufs[CLOCK][1], bufs[CLOCK][1] + BLOCK);
    }
    p->stop_processing(p);
    p->deactivate(p);
    p->destroy(p);
    return true;
}

static std::vector<int64_t> rises(const std::vector<float> &v)
{
    std::vector<int64_t> out;
    for (size_t k = 0; k < v.size(); k++)
        if (v[k] > 0.5f && (k == 0 || v[k - 1] < 0.5f)) out.push_back((int64_t)k);
    return out;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s Cosmati.clap\n", argv[0]);
        return 2;
    }
    // a settings folder of our own, CLOCK OUT MIDI + PULSE (projfile.cpp's: Linux $XDG_CONFIG_HOME/tessera, macOS
    // $HOME/Library/Application Support/Tessera)
    char tmpl[] = "/tmp/tessera-clock-XXXXXX";
    std::string home = mkdtemp(tmpl) ? tmpl : "/tmp";
    setenv("HOME", home.c_str(), 1);
    setenv("XDG_CONFIG_HOME", home.c_str(), 1);
#ifdef __APPLE__
    std::string dir = home + "/Library/Application Support/Tessera";
    mkdir((home + "/Library").c_str(), 0700);
    mkdir((home + "/Library/Application Support").c_str(), 0700);
#else
    std::string dir = home + "/tessera";
#endif
    mkdir(dir.c_str(), 0700);
    if (FILE *f = std::fopen((dir + "/clock_out").c_str(), "w")) { std::fputs("3\n", f); std::fclose(f); }

    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
#ifdef __APPLE__
    if (!lib) lib = dlopen((std::string(argv[1]) + "/Contents/MacOS/Cosmati").c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
    if (!lib)
    {
        std::printf("FAIL: dlopen %s: %s\n", argv[1], dlerror());
        return 1;
    }
    auto *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
    CHECK(entry && entry->init(argv[1]), "clap_entry");
    if (fails) return 1;
    auto *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    const char *id = fac->get_plugin_descriptor(fac, 0)->id;

    {   // the standalone app (first: a fresh copy of the core, which the app always has; a copy given back keeps
        // its song playing, and the clock starts with a START)
        std::vector<float> clk, rst;
        Midi midi;
        uint32_t lat = 0;
        CHECK(play(fac, id, false, 400, clk, rst, midi, &lat), "an instance with no transport");
        std::vector<int64_t> f8;
        int starts = 0;
        for (auto &m : midi.ev)
        {
            if (m.first == 0xFA) starts += f8.empty() ? 1 : 100;   // (START before any clock)
            if (m.first == 0xF8) f8.push_back(m.second);
        }
        auto c = rises(clk);
        double worst_gap = 0, worst_on = 0, gap = c.size() > 1 ? (double)(c.back() - c.front()) / (c.size() - 1) / 6
                                                                : 0;
        for (size_t k = 1; k < f8.size(); k++) worst_gap = std::fmax(worst_gap, std::fabs((f8[k] - f8[k - 1]) - gap));
        size_t on = 0;
        for (auto t : c)   // each pulse: the nearest clock
        {
            double best = 1e9;
            for (auto f : f8) best = std::fmin(best, std::fabs((double)(f - t)));
            if (best < 1e9) { worst_on = std::fmax(worst_on, best); on++; }
        }
        std::printf("standalone: %d START, %zu clocks (%.1f frames apart, worst %.0f off), %zu pulses, worst clock "
                    "%.0f frames from its pulse\n", starts, f8.size(), gap, worst_gap, c.size(), worst_on);
        CHECK(starts == 1, "one START, before the first clock");
        CHECK(f8.size() > 100 && c.size() > 15, "MIDI clock and pulses go out");
        CHECK(worst_gap <= RATE / 1000.0 + 1, "clocks spaced to a millisecond (%.0f frames)", worst_gap);
        CHECK(worst_on <= RATE / 1000.0 + 1, "the clock on the pulses to a millisecond (%.0f frames)", worst_on);
    }
    {   // in a DAW
        std::vector<float> clk, rst;
        Midi midi;
        uint32_t lat = 0;
        CHECK(play(fac, id, true, 200, clk, rst, midi, &lat), "an instance in a DAW");
        auto c = rises(clk), z = rises(rst);
        bool even = c.size() >= 15;
        for (size_t k = 0; even && k < c.size(); k++) even = std::llabs(c[k] - (int64_t)(lat + k * 6000)) <= 1;
        int clocks = 0;
        for (auto &m : midi.ev) clocks += m.first >= 0xF8;
        std::printf("DAW: latency %u, %zu pulses (first at %lld), %zu resets, %d MIDI clock messages\n", lat, c.size(),
                    c.empty() ? -1LL : (long long)c[0], z.size(), clocks);
        CHECK(even, "a pulse every 6000 frames from the latency on");
        CHECK(z.size() == 1 && !c.empty() && z[0] == c[0], "one reset, with the first pulse");
        CHECK(clocks == 0, "no MIDI clock into a DAW");
    }
    entry->deinit();
    std::printf("clock test: %s\n", fails ? "FAILED" : "ok");
    return fails ? 1 : 0;
}
