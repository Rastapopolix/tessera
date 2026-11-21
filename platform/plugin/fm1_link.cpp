/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* An FM-1 on USB plays the plugin (fm1_link.h): the decoder, the port names, and the link's thread. */
#include "fm1_link.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

const uint8_t FM1_PANEL_ON[7] = {0xF0, 0x7D, 0x46, 0x4C, 75, 1, 0xF7};
const uint8_t FM1_PANEL_OFF[7] = {0xF0, 0x7D, 0x46, 0x4C, 75, 0, 0xF7};

// ---- the decoder (web/sim/index.html fm1Message)

void Fm1Decoder::bytes(const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        uint8_t c = b[i];
        if (c >= 0xF8)   // real-time (clock, start, stop): between any bytes, changes nothing
            continue;
        if (c == 0xF0)
        {
            sx = 0;
            status = 0;
            continue;
        }
        if (c == 0xF7)
        {
            // PANEL's answer: 7D 46 4C 4B 01 between F0 and F7 (an answer 0, PANEL off, is not one)
            if (sx == 5 && sysex[0] == 0x7D && sysex[1] == 0x46 && sysex[2] == 0x4C && sysex[3] == 75 && sysex[4] == 1)
                answered = true;
            sx = -1;
            continue;
        }
        if (c & 0x80)   // a status byte: ends a SysEx left open; system common ones carry nothing of ours
        {
            sx = -1;
            status = c < 0xF0 ? c : 0;
            have = 0;
            continue;
        }
        if (sx >= 0)
        {
            if (sx < (int)sizeof sysex)
                sysex[sx] = c;
            sx++;
            continue;
        }
        if (!status)
            continue;
        int need = (status & 0xE0) == 0xC0 ? 1 : 2;   // (program change and channel pressure: one data byte)
        data[have++] = c;
        if (have == need)
        {
            message(status, data[0], need == 2 ? data[1] : 0);
            have = 0;   // (running status: the next data bytes are another of the same)
        }
    }
}

void Fm1Decoder::message(uint8_t st, uint8_t a, uint8_t v)
{
    if ((st & 0x0F) != 15)   // channel 16 only: the FM-1's other messages (its tracks' notes) are left out
        return;
    uint8_t type = st & 0xF0;
    if (type == 0xB0 && a >= 80 && a < 80 + TESS_NB)
    {
        uint32_t bit = 1u << (a - 80);
        in.buttons = v ? in.buttons | bit : in.buttons & ~bit;
    }
    else if (type == 0xB0 && a == 27)
        in.master = (v * 1023 + 63) / 127;
    else if (type == 0xB0 && a >= 20 && a < 20 + TESS_NE)
        in.enc[a - 20] += v < 64 ? v : 64 - v;   // relative, the sign bit: 1..63 clockwise, 65..127 back
    else if ((type == 0x90 || type == 0x80) && a < TESS_KEYS)
    {
        uint32_t bit = 1u << a;
        in.keys = type == 0x90 && v ? in.keys | bit : in.keys & ~bit;
    }
    else
        return;
    changed = true;
}

void Fm1Decoder::release()
{
    if (in.buttons || in.keys)
        changed = true;
    in.buttons = in.keys = 0;
}

void Fm1Decoder::take_turns()
{
    for (int r = 0; r < TESS_NE; r++)
        in.enc[r] = 0;
    in.master = -1;
    changed = false;
}

// ---- the port names (web/sim/index.html RE_FM1_PORT)

bool fm1_port_name(const std::string &name)
{
    std::string s;
    for (char ch : name)
        s += ch >= 'A' && ch <= 'Z' ? (char)(ch + 32) : ch;
    if (s.find("tessera") != s.npos || s.find("melodee") != s.npos || s.find("felucca") != s.npos)
        return true;
    auto alnum = [](char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'); };
    for (size_t i = s.find("fm"); i != s.npos; i = s.find("fm", i + 1))
    {
        size_t j = i + 2;
        if (j < s.size() && s[j] == '-')
            j++;
        if (j >= s.size() || s[j] != '1')
            continue;
        j++;
        if ((i == 0 || !alnum(s[i - 1])) && (j == s.size() || !alnum(s[j])))
            return true;
    }
    return false;
}

const char *fm1_state_text(Fm1Link::State s)
{
    switch (s)
    {
    case Fm1Link::LOOKING: return "NO FM-1 FOUND";
    case Fm1Link::BUSY: return "PORT IN USE";   // (on Windows: the DAW, or the editor's browser, has it)
    case Fm1Link::OLD: return "UPDATE THE FM-1";
    case Fm1Link::LIVE: return "FM-1 CONNECTED";
    case Fm1Link::ELSEWHERE: return "IN ANOTHER TESSERA";
    default: return "OFF";
    }
}

// ---- the link

struct Fm1Link::Impl
{
    using clock = std::chrono::steady_clock;
    std::mutex m;                       // the owners and the port's state (the audio thread only tries it)
    std::vector<const void *> wants;    // front: the one the FM-1 plays
    std::vector<const void *> lost;     // owed one take of everything let go
    State port = LOOKING;               // LOOKING, BUSY, OLD or LIVE
    std::mutex dm;                      // the decoder (the port's callback; the audio thread only tries it)
    Fm1Decoder dec;
    std::thread th;
    std::condition_variable cv;
    bool run = false;
    Opener opener;
    int ask_ms = 1000;

    void lose_front()   // the front owner no longer gets the FM-1's input: it is owed a release (m held)
    {
        if (!wants.empty() && std::find(lost.begin(), lost.end(), wants.front()) == lost.end())
            lost.push_back(wants.front());
    }
    void set_port(State s)   // the thread: the port's state (leaving LIVE: everything let go)
    {
        std::lock_guard<std::mutex> g(m);
        if (port == LIVE && s != LIVE)
        {
            lose_front();
            std::lock_guard<std::mutex> d(dm);
            dec.release();
        }
        if (s == LIVE && port != LIVE)   // (what was held before it answered, and turns since: from now only)
        {
            std::lock_guard<std::mutex> d(dm);
            dec.take_turns();
            dec.changed = true;
        }
        port = s;
    }
    void loop();
    void start()
    {
        if (th.joinable())
            return;
        run = true;
        port = LOOKING;
        th = std::thread([this] { loop(); });
    }
    void stop()   // (m not held)
    {
        if (!th.joinable())
            return;
        {
            std::lock_guard<std::mutex> g(m);
            run = false;
        }
        cv.notify_all();
        th.join();
    }
};

void Fm1Link::Impl::loop()
{
    std::unique_ptr<Fm1Ports> ports;
    auto next_open = clock::now(), next_ask = clock::now();
    int unanswered = 0;
    auto feed = [this](const uint8_t *b, size_t n) {
        std::lock_guard<std::mutex> d(dm);
        dec.bytes(b, n);
    };
    for (;;)
    {
        {
            std::unique_lock<std::mutex> g(m);
            if (!run)
                break;
        }
        auto now = clock::now();
        if (!ports && now >= next_open)
        {
            bool busy = false;
            ports = opener ? opener(feed, busy) : Fm1Ports::open(feed, busy);
            next_open = now + std::chrono::milliseconds(ask_ms);
            if (!ports)
                set_port(busy ? BUSY : LOOKING);
            else
            {
                {
                    std::lock_guard<std::mutex> d(dm);
                    dec.answered = false;
                }
                unanswered = 0;
                next_ask = now;   // (ask at once)
            }
        }
        if (ports)
        {
            ports->poll();
            if (!ports->alive())
            {
                ports.reset();   // (no callback after this)
                set_port(LOOKING);
                next_open = now + std::chrono::milliseconds(ask_ms);
                continue;
            }
            bool answered;
            {
                std::lock_guard<std::mutex> d(dm);
                answered = dec.answered;
            }
            State p;
            {
                std::lock_guard<std::mutex> g(m);
                p = port;
            }
            if (answered && p != LIVE)
                set_port(LIVE);
            if (now >= next_ask)
            {
                {
                    std::lock_guard<std::mutex> d(dm);
                    answered = dec.answered;
                    dec.answered = false;
                }
                unanswered = answered ? 0 : unanswered + 1;
                if (unanswered > 3)   // (three seconds: the FM-1 turns the mirror off by itself after as long)
                    set_port(OLD);
                ports->send(FM1_PANEL_ON, sizeof FM1_PANEL_ON);
                next_ask = now + std::chrono::milliseconds(ask_ms);
            }
        }
        std::unique_lock<std::mutex> g(m);
        cv.wait_for(g, std::chrono::milliseconds(ask_ms < 200 ? 5 : 50), [this] { return !run; });
    }
    if (ports)
    {
        ports->send(FM1_PANEL_OFF, sizeof FM1_PANEL_OFF);   // (the FM-1 back to itself at once)
        ports.reset();
    }
    set_port(LOOKING);
}

Fm1Link::Fm1Link() : impl(new Impl) {}

Fm1Link::~Fm1Link()
{
    if (impl->th.joinable())   // (instances close first, so this is a process ending with one left open)
    {
        {
            std::lock_guard<std::mutex> g(impl->m);
            impl->run = false;
        }
        impl->cv.notify_all();
#ifdef _WIN32
        impl->th.detach();   // (joining under the loader lock, at DLL unload, would hang)
#else
        impl->th.join();
#endif
    }
}

Fm1Link &Fm1Link::get()
{
    static Fm1Link link;
    return link;
}

void Fm1Link::want(const void *owner, bool on, bool first)
{
    Impl &s = *impl;
    bool stop = false;
    {
        std::lock_guard<std::mutex> g(s.m);
        auto it = std::find(s.wants.begin(), s.wants.end(), owner);
        bool had = !s.wants.empty() && s.wants.front() == owner;
        if (on && !first && it != s.wants.end())
            return;
        if (on && !first && !s.wants.empty())   // (behind the others: it has it once they let go)
        {
            s.wants.push_back(owner);
            return;
        }
        if (on)
        {
            if (had)
                return;
            if (s.port == LIVE)
                s.lose_front();   // (the one that had it lets go; this one gets what is held now)
            if (it != s.wants.end())
                s.wants.erase(it);
            s.wants.insert(s.wants.begin(), owner);
            s.lost.erase(std::remove(s.lost.begin(), s.lost.end(), owner), s.lost.end());
            std::lock_guard<std::mutex> d(s.dm);
            s.dec.changed = true;
            s.start();
        }
        else
        {
            if (it == s.wants.end())
                return;
            if (had && s.port == LIVE)
            {
                s.lose_front();
                std::lock_guard<std::mutex> d(s.dm);
                s.dec.changed = true;   // (the next in line gets what is held now)
            }
            s.wants.erase(it);
            stop = s.wants.empty();
        }
    }
    if (stop)
        s.stop();
}

void Fm1Link::closed(const void *owner)
{
    want(owner, false);
    std::lock_guard<std::mutex> g(impl->m);
    impl->lost.erase(std::remove(impl->lost.begin(), impl->lost.end(), owner), impl->lost.end());
}

void Fm1Link::off_all()
{
    {
        std::lock_guard<std::mutex> g(impl->m);
        if (impl->port == LIVE)
            impl->lose_front();
        impl->wants.clear();
    }
    impl->stop();
}

Fm1Link::State Fm1Link::state(const void *owner)
{
    std::lock_guard<std::mutex> g(impl->m);
    auto &w = impl->wants;
    if (std::find(w.begin(), w.end(), owner) == w.end())
        return OFF;
    return w.front() == owner ? impl->port : ELSEWHERE;
}

bool Fm1Link::take(const void *owner, Fm1Input &out)
{
    Impl &s = *impl;
    std::unique_lock<std::mutex> g(s.m, std::try_to_lock);
    if (!g.owns_lock())
        return false;
    auto l = std::find(s.lost.begin(), s.lost.end(), owner);
    if (l != s.lost.end())
    {
        s.lost.erase(l);
        out = Fm1Input{};
        return true;
    }
    if (s.wants.empty() || s.wants.front() != owner || s.port != LIVE)
        return false;
    std::unique_lock<std::mutex> d(s.dm, std::try_to_lock);
    if (!d.owns_lock() || !s.dec.changed)
        return false;
    out = s.dec.in;
    s.dec.take_turns();
    return true;
}

void Fm1Link::test_hooks(Opener open, int ask_ms)
{
    impl->opener = std::move(open);
    impl->ask_ms = ask_ms;
}
