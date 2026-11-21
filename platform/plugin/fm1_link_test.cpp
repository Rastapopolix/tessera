/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* An FM-1 on USB (fm1_link.h): the decoder over byte streams as the FM-1 sends them (editor.c ed_panel: buttons CC
 * 80..93, keys notes 0..26, knobs CC 20..26 with the sign bit, MASTER CC 27, all on channel 16, and PANEL's answer),
 * the port names, and the link with a stand-in for the ports: asking, LIVE, OLD, BUSY, unplugging, one instance at a
 * time, taking over and letting go. No MIDI hardware. */
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "fm1_link.h"

static int fails = 0;
#define CHECK(c)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(c))                                                                                                      \
        {                                                                                                              \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                                   \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

static void feed(Fm1Decoder &d, std::initializer_list<int> b)
{
    std::vector<uint8_t> v(b.begin(), b.end());
    d.bytes(v.data(), v.size());
}

static void decoder()
{
    Fm1Decoder d;
    // PANEL's answer, then SEL (bit 1) and HOME (bit 6) down, key 7 (C4) down
    feed(d, {0xF0, 0x7D, 0x46, 0x4C, 0x4B, 0x01, 0xF7, 0xBF, 81, 127, 0xBF, 86, 127, 0x9F, 7, 127});
    CHECK(d.answered);
    CHECK(d.changed);
    CHECK(d.in.buttons == ((1u << 1) | (1u << 6)));
    CHECK(d.in.keys == (1u << 7));
    // knobs: SELECT 2 clockwise, KNOB 1 (role 3) 1 back (65) twice; MASTER 127 and 64
    feed(d, {0xBF, 20, 2, 0xBF, 23, 65, 0xBF, 23, 65, 0xBF, 27, 127});
    CHECK(d.in.enc[0] == 2);
    CHECK(d.in.enc[3] == -2);
    CHECK(d.in.master == 1023);
    feed(d, {0xBF, 27, 64});
    CHECK(d.in.master == 516);   // (64 * 1023 / 127, rounded: the web simulator's Math.round)
    d.take_turns();
    CHECK(d.in.enc[0] == 0 && d.in.enc[3] == 0 && d.in.master == -1);
    CHECK(d.in.buttons && d.in.keys);   // (still held)
    CHECK(!d.changed);
    // running status, a clock byte in the middle of a message, a SysEx in pieces
    feed(d, {0xBF, 81, 0});                            // SEL up
    feed(d, {0x8F, 7});                                // key 7 up, split ..
    feed(d, {0xF8});                                   // .. a clock between ..
    feed(d, {0x00, 9, 127});                           // .. then running status: key 9 off as a note off
    CHECK(d.in.buttons == (1u << 6));
    CHECK(d.in.keys == 0);
    d.answered = false;
    feed(d, {0xF0, 0x7D, 0x46});
    feed(d, {0x4C, 0x4B});
    feed(d, {0x01, 0xF7});
    CHECK(d.answered);
    // a note on with velocity 0 is a note off
    feed(d, {0x9F, 26, 127});
    CHECK(d.in.keys == (1u << 26));
    feed(d, {0x9F, 26, 0});
    CHECK(d.in.keys == 0);
    // not ours: other channels, notes past the keys, CCs elsewhere, PANEL answering 0, another SysEx
    Fm1Decoder e;
    feed(e, {0x90, 7, 127, 0xB0, 81, 127, 0x9F, 60, 127, 0xBF, 7, 100, 0xC5, 3, 0xD5, 9});
    feed(e, {0xF0, 0x7D, 0x46, 0x4C, 0x4B, 0x00, 0xF7, 0xF0, 0x43, 0x10, 0x01, 0xF7});
    CHECK(!e.changed && !e.answered && !e.in.keys && !e.in.buttons);
    // a SysEx cut short by a status byte ends it; the message after it still counts
    feed(e, {0xF0, 0x7D, 0x46, 0xBF, 80, 127});
    CHECK(!e.answered && e.in.buttons == 1u);
    e.release();
    CHECK(e.in.buttons == 0 && e.changed);
    // the last button, OCT+ (CC 93, bit 13), and nothing past it
    Fm1Decoder f;
    feed(f, {0xBF, 93, 127, 0xBF, 94, 127});
    CHECK(f.in.buttons == (1u << 13));
}

static void names()
{
    CHECK(fm1_port_name("FM-1"));
    CHECK(fm1_port_name("FM1 MIDI 1"));
    CHECK(fm1_port_name("M-VAVE fm-1"));
    CHECK(fm1_port_name("2- FM-1"));
    CHECK(fm1_port_name("Tessera"));
    CHECK(fm1_port_name("MELODEE MIDI"));
    CHECK(fm1_port_name("Felucca"));
    CHECK(fm1_port_name(" 1 [FM1            ]: USB-Audio - FM-1\n"));
    CHECK(!fm1_port_name("FM10"));
    CHECK(!fm1_port_name("XFM1"));
    CHECK(!fm1_port_name("Elektron Digitone"));
    CHECK(!fm1_port_name("loopMIDI Port"));
    CHECK(!fm1_port_name("FM-12"));
}

// ---- the link, with a stand-in FM-1

struct FakeFm1
{
    std::mutex m;
    bool plugged = true, busy = false, answers = true;
    int opens = 0, asks = 0, offs = 0;
    std::function<void(const uint8_t *, size_t)> to_link;   // what the FM-1 sends
    void play(std::initializer_list<int> b)
    {
        std::vector<uint8_t> v(b.begin(), b.end());
        std::function<void(const uint8_t *, size_t)> f;
        {
            std::lock_guard<std::mutex> g(m);
            f = to_link;
        }
        if (f)
            f(v.data(), v.size());
    }
};

struct FakePorts : Fm1Ports
{
    FakeFm1 *fm1;
    ~FakePorts() override
    {
        std::lock_guard<std::mutex> g(fm1->m);
        fm1->to_link = nullptr;
    }
    bool send(const uint8_t *b, size_t n) override
    {
        bool on = n == 7 && !std::memcmp(b, FM1_PANEL_ON, 7), off = n == 7 && !std::memcmp(b, FM1_PANEL_OFF, 7);
        std::function<void(const uint8_t *, size_t)> f;
        {
            std::lock_guard<std::mutex> g(fm1->m);
            fm1->asks += on;
            fm1->offs += off;
            if (on && fm1->answers)
                f = fm1->to_link;
        }
        if (f)
        {
            static const uint8_t answer[7] = {0xF0, 0x7D, 0x46, 0x4C, 0x4B, 0x01, 0xF7};
            f(answer, 7);
        }
        return true;
    }
    bool alive() override
    {
        std::lock_guard<std::mutex> g(fm1->m);
        return fm1->plugged;
    }
};

static bool wait_for(const std::function<bool()> &c, int ms = 2000)
{
    for (int i = 0; i < ms / 2; i++)
    {
        if (c())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return c();
}

// what the audio thread would see: takes until one comes (or none in a while)
static bool take(Fm1Link &l, const void *who, Fm1Input &out, int ms = 300)
{
    return wait_for([&] { return l.take(who, out); }, ms);
}

static void link()
{
    FakeFm1 fm1;
    Fm1Link &l = Fm1Link::get();
    l.test_hooks(
        [&](std::function<void(const uint8_t *, size_t)> in, bool &busy) -> std::unique_ptr<Fm1Ports> {
            std::lock_guard<std::mutex> g(fm1.m);
            busy = false;
            if (!fm1.plugged)
                return nullptr;
            if (fm1.busy)
            {
                busy = true;
                return nullptr;
            }
            fm1.opens++;
            fm1.to_link = std::move(in);
            auto p = std::unique_ptr<FakePorts>(new FakePorts);
            p->fm1 = &fm1;
            return p;
        },
        20);
    int a = 0, b = 0;   // two instances
    Fm1Input in;

    CHECK(l.state(&a) == Fm1Link::OFF);
    l.want(&a, true, false);
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LIVE; }));
    CHECK(fm1.asks >= 1);
    // what it plays reaches the instance that has it
    fm1.play({0xBF, 90, 127, 0xBF, 24, 3});   // PLAY down, KNOB 2 three clockwise
    CHECK(take(l, &a, in));
    CHECK(in.buttons == (1u << 10));
    CHECK(in.enc[4] == 3);
    CHECK(!l.take(&a, in));   // (nothing new)
    // a second instance opening with the setting on waits behind the first
    l.want(&b, true, false);
    CHECK(l.state(&b) == Fm1Link::ELSEWHERE);
    CHECK(l.state(&a) == Fm1Link::LIVE);
    fm1.play({0xBF, 24, 1});
    CHECK(take(l, &a, in) && in.enc[4] == 1);
    CHECK(!l.take(&b, in));
    // a click in the second's window takes it over: the first lets go of PLAY, the second gets PLAY held
    l.want(&b, true, true);
    CHECK(l.state(&b) == Fm1Link::LIVE);
    CHECK(l.state(&a) == Fm1Link::ELSEWHERE);
    CHECK(take(l, &a, in) && in.buttons == 0 && in.keys == 0);
    CHECK(take(l, &b, in) && in.buttons == (1u << 10));
    fm1.play({0xBF, 90, 0});
    CHECK(take(l, &b, in) && in.buttons == 0);
    // the one that has it closes: the other has it again
    fm1.play({0x9F, 3, 127});
    CHECK(take(l, &b, in) && in.keys == (1u << 3));
    l.closed(&b);
    CHECK(l.state(&b) == Fm1Link::OFF);
    CHECK(l.state(&a) == Fm1Link::LIVE);
    CHECK(take(l, &a, in) && in.keys == (1u << 3));
    CHECK(!l.take(&b, in));   // (closed: owed nothing)
    // unplugged: everything let go, then LOOKING; plugged back: LIVE again
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.plugged = false;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LOOKING; }));
    CHECK(take(l, &a, in) && in.keys == 0);
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.plugged = true;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LIVE; }));
    CHECK(fm1.opens == 2);
    // an FM-1 that stops answering (older firmware, or not Tessera): OLD after three asks, and its messages unheard
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.answers = false;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::OLD; }));
    while (l.take(&a, in)) {}
    fm1.play({0xBF, 80, 127});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(!l.take(&a, in));
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.answers = true;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LIVE; }));
    // the port held by another program: BUSY
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.plugged = false;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LOOKING; }));
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.plugged = true;
        fm1.busy = true;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::BUSY; }));
    {
        std::lock_guard<std::mutex> g(fm1.m);
        fm1.busy = false;
    }
    CHECK(wait_for([&] { return l.state(&a) == Fm1Link::LIVE; }));
    // turned off: the thread stops and tells the FM-1 (PANEL 0)
    int offs = fm1.offs;
    l.want(&a, false);
    CHECK(l.state(&a) == Fm1Link::OFF);
    CHECK(fm1.offs == offs + 1);
    CHECK(fm1.to_link == nullptr);   // (the ports closed)
    l.closed(&a);
}

int main()
{
    decoder();
    names();
    link();
    if (fails)
    {
        std::printf("fm1 link: %d failed\n", fails);
        return 1;
    }
    std::printf("fm1 link: all passed\n");
    return 0;
}
