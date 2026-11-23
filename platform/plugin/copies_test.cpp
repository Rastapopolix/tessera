/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Several instances: the copies of the core are wholly apart (core_copies.h). Two copies boot; one plays STILL TIDE,
 * the other stays stopped and silent; the playing one renders exactly what it renders with the other running beside
 * it (and doing something else); a project saved from one loads into the other; two threads render two copies at
 * once; MIDI's N bars rendered offline on a spare copy (midirender.cpp) give the same notes beside a copy that plays.
 * Exit 0: all passed. */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

#include "core_copies.h"
#include "midifile.h"
#include "sim_demos.h"   // (build/gen: SIM_DEMO_A, demos/still-tide.tsp; the plugin's slots start empty)

static int fails = 0;
#define CHECK(c, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(c))                                                                                                      \
        {                                                                                                              \
            std::printf("FAIL: " __VA_ARGS__);                                                                         \
            std::printf("\n");                                                                                         \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

// n blocks of 256 frames of copy c, playing (its own transport) or not; sum of squares out, the frames into all
static double run(const tess_core_vt *c, int blocks, std::vector<float> *all = nullptr)
{
    float l[256], r[256];
    double e = 0;
    for (int b = 0; b < blocks; b++)
    {
        c->render(l, r, nullptr, 256);
        for (int k = 0; k < 256; k++)
        {
            e += (double)l[k] * l[k] + (double)r[k] * r[k];
            if (all) all->push_back(l[k]), all->push_back(r[k]);
        }
    }
    return e;
}

int main()
{
    std::printf("copies: %u\n", tess_copy_count());
    CHECK(tess_copy_count() >= 2, "at least two copies built in");
    if (fails) return 1;
    const tess_core_vt *a = tess_copy(0), *b = tess_copy(1);
    CHECK(a && b && a != b && a->render != b->render, "two different copies");
    CHECK(tess_copy(tess_copy_count()) == nullptr, "no copy past the count");
    a->boot();
    b->boot();
    if (tess_copy_count() > 2)   // a new copy starts from a new project: playing, it is silent (copy 2, apart from a and b)
    {
        const tess_core_vt *c = tess_copy(2);
        c->boot();
        c->transport(1);
        double ec = run(c, 400);
        c->transport(0);
        std::printf("a new copy playing: energy %.9f\n", ec);
        CHECK(ec < 1e-6, "a new copy plays nothing until it gets notes");
    }
    a->load(SIM_DEMO_A, sizeof SIM_DEMO_A);   // (STILL TIDE in both)
    b->load(SIM_DEMO_A, sizeof SIM_DEMO_A);

    // a alone, then a again from a fresh state (copy 2 if there is one, else skip) is not possible: so compare a with
    // a reference rendered by copy b first, before b does anything else
    std::vector<float> ref, got;
    b->transport(1);
    run(b, 400, &ref);   // (b: ~2.3 s of STILL TIDE from the top)
    b->transport(0);
    run(b, 2);           // (a request takes effect as the copy renders)

    a->transport(1);
    for (int i = 0; i < 400; i++)   // a plays while b is stopped and has a note held (b must not disturb a)
    {
        if (i == 10) b->midi(0x90, 60, 100);
        float l[256], r[256], bl[256], br[256];
        a->render(l, r, nullptr, 256);
        b->render(bl, br, nullptr, 256);
        for (int k = 0; k < 256; k++) got.push_back(l[k]), got.push_back(r[k]);
    }
    b->midi(0x80, 60, 0);
    size_t same = 0;
    for (size_t k = 0; k < ref.size() && k < got.size(); k++) same += ref[k] == got[k];
    double e = 0;
    for (float x : got) e += (double)x * x;
    std::printf("a playing beside b: %zu of %zu samples as b alone played them; energy %.3f\n", same, ref.size(), e);
    CHECK(e > 1.0, "STILL TIDE should sound in copy a");
    CHECK(same == ref.size() && got.size() == ref.size(), "a copy renders the same with another copy running beside it");
    CHECK(a->playing() && !b->playing(), "a's transport is a's own (a playing, b stopped)");

    // b stopped and quiet stays silent while a plays
    for (int i = 0; i < 1120; i++) run(a, 1), run(b, 1);   // (6.5 s: b's tails die, as the render test waits)
    double eb = run(b, 40), ea = run(a, 40);
    std::printf("energy: a playing %.3f, b stopped %.6f\n", ea, eb);
    CHECK(ea > 0.1 && eb < ea * 1e-3, "b stopped and quiet while a plays");

    // the project: a's MACRO and BPM are a's own; a's project saved and loaded into b
    a->macro_set(0, 77);
    CHECK(a->macro(0) == 77 && b->macro(0) != 77, "MACRO 1 is a's own (%u, %u)", a->macro(0), b->macro(0));
    std::vector<uint8_t> tsp(a->tsp_max());
    uint32_t n = a->save(tsp.data(), (uint32_t)tsp.size());
    CHECK(n > 0, "a's project saves");
    CHECK(b->load(tsp.data(), n) == 0 && b->macro(0) == 77, "a's project loads into b");

    // MIDI while stopped: STILL TIDE's first 4 bars on a spare copy, as fast as it renders, alone and again on a thread
    // while a plays: the same notes both times, each inside the 4 bars, a file of 16 quarters
    if (tess_copy_count() > 3)
    {
        const tess_core_vt *x = tess_copy(3);
        x->boot();
        std::vector<uint8_t> demo(SIM_DEMO_A, SIM_DEMO_A + sizeof SIM_DEMO_A);
        std::vector<midifile::Ev> ev1, ev2;
        double bpm1 = 0, bpm2 = 0;
        auto t0 = std::chrono::steady_clock::now();
        std::string err = midifile::render(x, demo, 4, ev1, bpm1);
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        CHECK(err.empty(), "4 bars render (%s)", err.c_str());
        int ons = 0, kept = 0;
        double lo = 1e9, hi = -1;
        for (auto &e : ev1)
        {
            if ((e.status & 0xF0) == 0x90 && e.d2) ons++, kept += std::lround(e.beat * 480) < 16 * 480;
            lo = std::min(lo, e.beat), hi = std::max(hi, e.beat);
        }
        std::printf("4 bars offline: %d notes, beats %.3f .. %.3f, %.0f BPM, %.2f s (%.0fx real time)\n", ons, lo, hi,
                    bpm1, secs, bpm1 > 0 ? 16.0 * 60.0 / bpm1 / secs : 0.0);
        CHECK(ons > 4, "STILL TIDE plays notes in its first 4 bars (%d)", ons);
        CHECK(lo >= 0 && hi < 16.0 + 1e-6, "every note inside the 4 bars (%.3f .. %.3f)", lo, hi);
        std::vector<float> before, during;
        a->transport(1);
        std::thread tx([&] { midifile::render(x, demo, 4, ev2, bpm2); });
        run(a, 200, &during);
        tx.join();
        bool same = ev1.size() == ev2.size() && bpm1 == bpm2;
        for (size_t i = 0; same && i < ev1.size(); i++)
            same = std::fabs(ev1[i].beat - ev2[i].beat) < 0.005 &&   // (the core's clock steps in ms: a step's grain)
                    ev1[i].status == ev2[i].status && ev1[i].d1 == ev2[i].d1;
        CHECK(same, "the same notes again, rendered while another copy plays (%zu, %zu)", ev1.size(), ev2.size());
        std::vector<uint8_t> f = midifile::smf(ev1, bpm1, "STILL TIDE", 4);
        int tr = 0, notes = 0;
        double q = 0;
        CHECK(midifile::read_back(f, tr, notes, q) && notes == kept && std::fabs(q - 16.0) < 1e-9,
              "a .mid of the 4 bars: %d notes (%d before the end), %.3f quarters", notes, kept, q);
        CHECK(x->playing() == 0, "the spare copy stopped after");
    }

    // two threads, two copies at once
    double e1 = 0, e2 = 0;
    std::thread t1([&] { e1 = run(a, 300); }), t2([&] { b->transport(1); e2 = run(b, 300); });
    t1.join();
    t2.join();
    std::printf("two threads: energy %.3f and %.3f\n", e1, e2);
    CHECK(std::isfinite(e1) && std::isfinite(e2) && e1 > 0.1 && e2 > 0.1, "two copies render on two threads at once");

    std::printf(fails ? "copies test: %d FAILED\n" : "copies test: ok\n", fails);
    return fails ? 1 : 0;
}
