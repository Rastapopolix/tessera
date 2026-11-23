/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* N bars of a project as notes, offline (midifile.h render): MIDI while the plugin is stopped */
#include "midifile.h"

#include "core_copies.h"

namespace midifile
{
std::string render(const tess_core_vt *c, const std::vector<uint8_t> &tsp, uint32_t bars, std::vector<Ev> &ev, double &bpm)
{
    std::vector<float> l(1024), r(1024);
    uint32_t pkt, frame;
    c->host(0, 0, 0, 0);   // (its own clock, stopped: not the DAW's of an instance that held the copy before)
    c->transport(0);
    c->render(l.data(), r.data(), nullptr, (uint32_t)l.size());
    while (c->midi_out(&pkt, &frame)) {}   // (what was still going out: not this clip's)
    if (tsp.empty() || c->load(tsp.data(), (uint32_t)tsp.size()))
        return "COULD NOT READ THE PROJECT";
    c->transport(1);
    const double end = bars * 4.0, rate = c->rate();
    // at most as long as the bars take at 20 BPM (a project that waits for a MIDI clock never gets there)
    const uint64_t limit = (uint64_t)(end * 3.0 * rate) + (uint64_t)rate;
    double pos0 = 0, base = 0;
    uint32_t f0 = c->frames();
    bool started = false;
    for (uint64_t done = 0; done < limit; done += l.size())
    {
        c->render(l.data(), r.data(), nullptr, (uint32_t)l.size());
        double pos1 = c->position();
        uint32_t f1 = c->frames();
        if (!started)
        {
            if (!c->playing())   // (PLAY not yet taken: nothing of the clip)
            {
                while (c->midi_out(&pkt, &frame)) {}
                f0 = f1;
                continue;
            }
            started = true;   // (play starts at the top)
        }
        bool back = pos1 < pos0 - 0.5;   // (the song went back to its top: the clip goes on after it)
        double p1 = back ? pos0 : pos1;
        while (c->midi_out(&pkt, &frame))
        {
            if ((pkt >> 8 & 0xF0u) == 0xF0u)
                continue;   // (a clock, system messages)
            double a = f1 != f0 ? (double)(int32_t)(frame - f0) / (double)(uint32_t)(f1 - f0) : 1.0;
            a = a < 0 ? 0 : a > 1 ? 1 : a;
            ev.push_back({base + pos0 + a * (p1 - pos0), (uint8_t)(pkt >> 8), (uint8_t)(pkt >> 16 & 127),
                          (uint8_t)(pkt >> 24 & 127)});
        }
        if (back)
            base += pos0 - pos1;
        pos0 = pos1;
        f0 = f1;
        if (base + pos1 >= end || !c->playing())
            break;
    }
    bpm = c->bpm();
    c->transport(0);
    c->notes_off();
    c->render(l.data(), r.data(), nullptr, (uint32_t)l.size());
    while (c->midi_out(&pkt, &frame)) {}
    return "";
}
}   // namespace midifile
