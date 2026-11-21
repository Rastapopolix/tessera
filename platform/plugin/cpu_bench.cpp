/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* What Tessera costs per instance: a minimal CLAP host (POSIX) over the built Tessera.clap runs 1, 4 and 8 instances,
 * each playing the project it starts with (STILL TIDE) with the host's transport, and prints for each count:
 *   - one thread: the time per instance as a share of real time (a DAW running every instance on one core);
 *   - a thread per instance: the time for all of them at once, as a share of real time;
 *   - the slowest block of one instance on one thread, against the block's real time (what a DAW's buffer holds).
 * It measures; it passes or fails nothing (the machine sets the numbers).
 * usage: tessera-cpu-bench path/to/Tessera.clap [seconds] [rate] [block] */
#include <clap/clap.h>
#include <dlfcn.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

static const void *h_get_extension(const clap_host_t *, const char *) { return nullptr; }
static void h_nop(const clap_host_t *) {}
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-cpu-bench", "Tessera", "", "0",
                                 h_get_extension, h_nop, h_nop, h_nop};
static uint32_t ev_size(const clap_input_events_t *) { return 0; }
static const clap_event_header_t *ev_get(const clap_input_events_t *, uint32_t) { return nullptr; }
static bool out_try_push(const clap_output_events_t *, const clap_event_header_t *) { return true; }
static const clap_input_events_t no_events = {nullptr, ev_size, ev_get};
static const clap_output_events_t out_events = {nullptr, out_try_push};

using Clock = std::chrono::steady_clock;
static uint32_t rate = 48000, block = 256;

struct Inst
{
    const clap_plugin_t *p = nullptr;
    std::vector<float> L, R;
    float *ch[2];
    clap_audio_buffer_t out = {};
    clap_event_transport_t tr = {};
    clap_process_t proc = {};
    double beats = 0, slowest = 0;   // (seconds; slowest: of one block, on one thread)
    bool timed = true;

    bool open(const clap_plugin_factory_t *fac, const char *id)
    {
        L.assign(block, 0.f);
        R.assign(block, 0.f);
        ch[0] = L.data();
        ch[1] = R.data();
        p = fac->create_plugin(fac, &host, id);
        if (!p || !p->init(p) || !p->activate(p, rate, block, block) || !p->start_processing(p)) return false;
        out.data32 = ch;
        out.channel_count = 2;
        tr.header = {sizeof tr, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
        tr.tempo = 120.0;
        tr.flags = CLAP_TRANSPORT_IS_PLAYING | CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
        proc.frames_count = block;
        proc.transport = &tr;
        proc.audio_outputs = &out;
        proc.audio_outputs_count = 1;
        proc.in_events = &no_events;
        proc.out_events = &out_events;
        return true;
    }
    void close()
    {
        p->stop_processing(p);
        p->deactivate(p);
        p->destroy(p);
    }
    void step()
    {
        tr.song_pos_beats = (clap_beattime)(beats * CLAP_BEATTIME_FACTOR);
        auto t0 = Clock::now();
        p->process(p, &proc);
        double s = std::chrono::duration<double>(Clock::now() - t0).count();
        if (timed && s > slowest) slowest = s;
        proc.steady_time += block;
        beats += block * tr.tempo / 60.0 / rate;
    }
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s Tessera.clap [seconds] [rate] [block]\n", argv[0]);
        return 2;
    }
    double seconds = argc > 2 ? std::atof(argv[2]) : 10.0;
    if (argc > 3) rate = (uint32_t)std::atoi(argv[3]);
    if (argc > 4) block = (uint32_t)std::atoi(argv[4]);
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib)
    {
        std::printf("dlopen %s: %s\n", argv[1], dlerror());
        return 1;
    }
    auto *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
    if (!entry || !entry->init(argv[1])) return 1;
    auto *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    const char *id = fac->get_plugin_descriptor(fac, 0)->id;
    const uint32_t blocks = (uint32_t)(seconds * rate / block);
    const double real = (double)blocks * block / rate, block_real = (double)block / rate;

    std::printf("Tessera CPU: %.0f s of audio per instance, %u Hz, blocks of %u (%.2f ms), %u hardware threads\n",
                real, rate, block, block_real * 1e3, std::thread::hardware_concurrency());
    std::printf("instances  one thread: per instance, all   a thread each: all   slowest block (one instance)\n");
    for (int n : {1, 4, 8})
    {
        std::vector<Inst> in(n);
        bool ok = true;
        for (auto &x : in) ok = ok && x.open(fac, id);
        if (!ok)
        {
            std::printf("%9d  (could not open %d instances)\n", n, n);
            for (auto &x : in) if (x.p) x.close();
            continue;
        }
        for (uint32_t b = 0; b < rate / block; b++)   // (a second first: the start's work out of the way)
            for (auto &x : in) x.step();
        for (auto &x : in) x.slowest = 0;

        auto t0 = Clock::now();   // one thread: every instance in turn, block by block
        for (uint32_t b = 0; b < blocks; b++)
            for (auto &x : in) x.step();
        double one = std::chrono::duration<double>(Clock::now() - t0).count();

        for (auto &x : in) x.timed = false;   // (threads: the slowest block would measure the scheduler)
        t0 = Clock::now();        // a thread per instance (a DAW spreads tracks over cores)
        std::vector<std::thread> th;
        for (auto &x : in)
            th.emplace_back([&x, blocks] { for (uint32_t b = 0; b < blocks; b++) x.step(); });
        for (auto &t : th) t.join();
        double par = std::chrono::duration<double>(Clock::now() - t0).count();

        double slow = 0;
        for (auto &x : in) if (x.slowest > slow) slow = x.slowest;
        std::printf("%9d  %15.2f %% %7.2f %%   %16.2f %%   %8.3f ms (%.0f %% of a block)\n", n, 100.0 * one / n / real,
                    100.0 * one / real, 100.0 * par / real, slow * 1e3, 100.0 * slow / block_real);
        for (auto &x : in) x.close();
    }
    entry->deinit();
    return 0;
}
