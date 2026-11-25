/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The Standard MIDI File midifile.h writes, and reads back for its tests */
#include "midifile.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>

// the writer, shared with the core (MELODEE_SMF): its functions static to this file
#include "../../firmware/src/smf.c"

namespace midifile
{
std::vector<uint8_t> smf(const std::vector<Ev> &events, double bpm, const std::string &name, uint32_t bars)
{
    std::vector<smf_ev> ev;
    ev.reserve(events.size());
    for (auto &e : events)
        ev.push_back({(uint32_t)(e.beat > 0 ? std::lround(e.beat * SMF_PPQ) : 0), e.status, e.d1, e.d2, 0});
    std::stable_sort(ev.begin(), ev.end(), [](const smf_ev &a, const smf_ev &b) { return a.tick < b.tick; });
    uint32_t b = (uint32_t)std::lround(bpm > 1 ? bpm : 120);
    uint32_t len = smf_write(ev.data(), (uint32_t)ev.size(), b, name.c_str(), bars, nullptr, 0);
    std::vector<uint8_t> f(len);
    if (len) smf_write(ev.data(), (uint32_t)ev.size(), b, name.c_str(), bars, f.data(), len);
    return f;
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
            quarters = std::max(quarters, (double)now / ppq);   // (an end-of-track counts: a bars clip's length)
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
        }
        tracks++;
        at = endp;
    }
    return true;
}
}   // namespace midifile
