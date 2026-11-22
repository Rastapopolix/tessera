/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The Standard MIDI File midifile.h writes, and reads back for its tests */
#include "midifile.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>

namespace midifile
{
namespace
{
const int PPQ = 480;

void u32(std::vector<uint8_t> &b, uint32_t v)
{
    for (int s = 24; s >= 0; s -= 8) b.push_back((uint8_t)(v >> s));
}
void u16(std::vector<uint8_t> &b, uint32_t v)
{
    b.push_back((uint8_t)(v >> 8));
    b.push_back((uint8_t)v);
}
void vlq(std::vector<uint8_t> &b, uint32_t v)
{
    uint8_t t[5];
    int n = 0;
    do t[n++] = (uint8_t)(v & 0x7F), v >>= 7; while (v);
    while (n--) b.push_back((uint8_t)(t[n] | (n ? 0x80 : 0)));
}
void meta_text(std::vector<uint8_t> &b, uint8_t type, const std::string &t)
{
    b.push_back(0x00);
    b.push_back(0xFF);
    b.push_back(type);
    vlq(b, (uint32_t)t.size());
    b.insert(b.end(), t.begin(), t.end());
}
// a track chunk around body (its end-of-track added)
void chunk(std::vector<uint8_t> &file, std::vector<uint8_t> body)
{
    body.insert(body.end(), {0x00, 0xFF, 0x2F, 0x00});
    file.insert(file.end(), {'M', 'T', 'r', 'k'});
    u32(file, (uint32_t)body.size());
    file.insert(file.end(), body.begin(), body.end());
}
struct Tick { uint32_t t; uint8_t s, d1, d2; };
}   // namespace

std::vector<uint8_t> smf(const std::vector<Ev> &events, double bpm, const std::string &name)
{
    double first = -1, last = 0;
    for (auto &e : events)
        if ((e.status & 0xF0) == 0x90 && e.d2)
        {
            if (first < 0 || e.beat < first) first = e.beat;
        }
    if (first < 0)
        return {};
    double start = std::floor(first / 4.0) * 4.0;   // (the bar the first note is in)
    std::map<int, std::vector<Tick>> per;            // channel -> its messages, in ticks from start
    std::map<int, int> on;                           // (channel << 8 | key) -> notes on
    for (auto &e : events)
    {
        if (e.beat < start) continue;                // (an off of a note from before the clip)
        int ch = e.status & 15, key = e.d1, k = ch << 8 | key;
        bool is_on = (e.status & 0xF0) == 0x90 && e.d2;
        if (!is_on && !on[k]) continue;              // (its note began before the clip)
        on[k] += is_on ? 1 : -1;
        uint32_t t = (uint32_t)std::lround((e.beat - start) * PPQ);
        per[ch].push_back({t, (uint8_t)(is_on ? 0x90 | ch : 0x80 | ch), (uint8_t)key, (uint8_t)(is_on ? e.d2 : 64)});
        last = std::max(last, e.beat);
    }
    uint32_t end = (uint32_t)std::lround((last - start) * PPQ);
    for (auto &kv : on)                              // (notes still on: off with the last message)
        for (int n = 0; n < kv.second; n++)
            per[kv.first >> 8].push_back({end, (uint8_t)(0x80 | (kv.first >> 8)), (uint8_t)(kv.first & 127), 64});

    std::vector<uint8_t> file = {'M', 'T', 'h', 'd'};
    u32(file, 6);
    u16(file, 1);
    u16(file, (uint32_t)per.size() + 1);
    u16(file, PPQ);
    std::vector<uint8_t> t0;
    if (!name.empty()) meta_text(t0, 0x03, name);
    uint32_t us = (uint32_t)std::lround(60e6 / (bpm > 1 ? bpm : 120));
    t0.insert(t0.end(), {0x00, 0xFF, 0x51, 0x03, (uint8_t)(us >> 16), (uint8_t)(us >> 8), (uint8_t)us});
    t0.insert(t0.end(), {0x00, 0xFF, 0x58, 0x04, 4, 2, 24, 8});   // (4/4)
    chunk(file, t0);
    for (auto &kv : per)
    {
        auto &v = kv.second;
        std::stable_sort(v.begin(), v.end(), [](const Tick &a, const Tick &b) { return a.t < b.t; });
        std::vector<uint8_t> body;
        meta_text(body, 0x03, "T" + std::to_string(kv.first + 1));
        uint32_t now = 0;
        for (auto &m : v)
        {
            vlq(body, m.t - now);
            now = m.t;
            body.insert(body.end(), {m.s, m.d1, m.d2});
        }
        chunk(file, body);
    }
    return file;
}

std::string folder()
{
    namespace fs = std::filesystem;
#ifdef _WIN32
    const wchar_t *home = _wgetenv(L"USERPROFILE");
#else
    const char *home = std::getenv("HOME");
#endif
    fs::path d = home && *home ? fs::path(home) / "Documents" / "Tessera" / "MIDI" : fs::path();
    std::error_code ec;
    if (d.empty() || (fs::create_directories(d, ec), ec)) return "";
    return d.u8string();   // (C++17: UTF-8 in a std::string)
}

std::string file_name(const std::string &project, const std::string &stamp)
{
    std::string n;
    for (char c : project)
        if ((unsigned char)c >= 32 && !std::strchr("<>:\"/\\|?*", c)) n += c;
    while (!n.empty() && (n.back() == ' ' || n.back() == '.')) n.pop_back();
    if (n.empty()) n = "Tessera";
    return n + (stamp.empty() ? "" : " " + stamp) + ".mid";
}

bool read_back(const std::vector<uint8_t> &f, int &tracks, int &notes, double &quarters)
{
    tracks = notes = 0;
    quarters = 0;
    auto be = [&](size_t at, int n) {
        uint32_t v = 0;
        for (int i = 0; i < n; i++) v = v << 8 | f[at + i];
        return v;
    };
    if (f.size() < 14 || std::string(f.begin(), f.begin() + 4) != "MThd")
        return false;
    int ntr = (int)be(10, 2), ppq = (int)be(12, 2);
    size_t at = 14;
    for (int t = 0; t < ntr; t++)
    {
        if (at + 8 > f.size() || std::string(f.begin() + at, f.begin() + at + 4) != "MTrk") return false;
        size_t len = be(at + 4, 4), p = at + 8, endp = p + len;
        if (endp > f.size()) return false;
        uint32_t now = 0;
        uint8_t run = 0;
        while (p < endp)
        {
            uint32_t d = 0;
            uint8_t c;
            do { c = f[p++]; d = d << 7 | (c & 0x7F); } while ((c & 0x80) && p < endp);
            now += d;
            uint8_t s = f[p];
            if (s == 0xFF)
            {
                uint32_t l = 0;
                p += 2;
                do { c = f[p++]; l = l << 7 | (c & 0x7F); } while (c & 0x80);
                p += l;
                continue;
            }
            if (s & 0x80) run = s, p++;
            uint8_t d1 = f[p++], d2 = f[p++];
            if ((run & 0xF0) == 0x90 && d2) notes++;
            (void)d1;
            quarters = std::max(quarters, (double)now / ppq);
        }
        tracks++;
        at = endp;
    }
    return true;
}
}   // namespace midifile
