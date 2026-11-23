/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The MIDI clip (midifile.h), alone:
 *   - a recorder keeps notes only, oldest first, and drops the oldest once full;
 *   - the file: a header, the tempo track, one track per channel, every note-on, the length in quarters; it starts on
 *     the bar of the first note; an off of a note from before the clip is left out, a note still on is ended;
 *   - bars N: from the top for N bars exactly, the notes past the end left out, a note still on ended there;
 *   - no notes: no file; the clip's file name leaves out what a file name cannot have.
 * usage: tessera-midifile-test */
#include <cmath>
#include <cstdio>

#include "midifile.h"

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

int main()
{
    midifile::Recorder r(8);
    r.add(0.0, 0xF8, 0, 0);   // (a clock: left out)
    r.add(0.0, 0xB0, 7, 100);  // (a CC: left out)
    CHECK(r.size() == 0, "only notes are kept (%zu)", r.size());
    for (int i = 0; i < 10; i++) r.add(i, 0x90, (uint8_t)(60 + i), 100);
    auto ev = r.take();
    CHECK(ev.size() == 8 && ev[0].d1 == 62 && ev[7].d1 == 69, "the ring keeps the newest 8, oldest first");

    // two channels; the clip starts on bar 2 (beat 4) where the first note is at 5.0
    r = midifile::Recorder(64);
    r.add(4.9, 0x80, 50, 0);    // (the off of a note from before: left out)
    r.add(5.0, 0x90, 60, 100);  // T1
    r.add(5.5, 0x80, 60, 0);
    r.add(6.0, 0x92, 36, 90);   // T3
    r.add(6.25, 0x92, 36, 0);   // (a note-on of 0: an off)
    r.add(7.0, 0x90, 64, 80);   // T1, never off: ended with the last message
    r.add(9.0, 0x92, 38, 70);
    r.add(9.5, 0x82, 38, 0);
    std::vector<uint8_t> f = midifile::smf(r.take(), 96, "UNDERTOW");
    int tracks = 0, notes = 0;
    double q = 0;
    CHECK(midifile::read_back(f, tracks, notes, q), "the file reads back");
    std::printf("clip: %zu bytes, %d tracks, %d notes, %.2f quarters\n", f.size(), tracks, notes, q);
    CHECK(tracks == 3, "the tempo track and one per channel (%d)", tracks);
    CHECK(notes == 4, "four notes (%d)", notes);
    CHECK(std::fabs(q - 5.5) < 1e-9, "the clip runs from the bar (beat 4) to the last message (9.5): 5.5 (%.3f)", q);
    CHECK(f.size() > 22 && f[8] == 0 && f[9] == 1 && f[12] == 0x01 && f[13] == 0xE0, "format 1, 480 a quarter");
    // the tempo: 96 BPM is 625000 us a quarter
    bool tempo = false;
    for (size_t i = 0; i + 6 < f.size(); i++)
        if (f[i] == 0xFF && f[i + 1] == 0x51 && f[i + 2] == 3)
            tempo = (f[i + 3] << 16 | f[i + 4] << 8 | f[i + 5]) == 625000;
    CHECK(tempo, "the tempo is the project's");

    // bars 2: from the top, 8 quarters exactly; the note still on (7.0) ends at 8; the notes from 8 on are left out
    f = midifile::smf(r.take(), 96, "UNDERTOW", 2);
    CHECK(midifile::read_back(f, tracks, notes, q), "the bars clip reads back");
    CHECK(tracks == 3 && notes == 3, "bars 2: three notes before beat 8 (%d tracks, %d notes)", tracks, notes);
    CHECK(std::fabs(q - 8.0) < 1e-9, "bars 2: the clip is 8 quarters long (%.3f)", q);
    CHECK(midifile::smf(r.take(), 96, "", 1).empty(), "bars 1: no note in the first bar, no file");
    CHECK(midifile::smf({}, 120, "").empty(), "no notes: no file");
    midifile::Recorder offs(4);
    offs.add(1, 0x80, 60, 0);
    CHECK(midifile::smf(offs.take(), 120, "").empty(), "offs only: no file");

    CHECK(midifile::file_name("A/B:C?", "2026-11-22 163501") == "ABC 2026-11-22 163501.mid", "a file name's own characters");
    CHECK(midifile::file_name("", "") == "Tessera.mid", "no name: Tessera");
    CHECK(midifile::file_name(" .", "x") == "Tessera x.mid", "a name of spaces and dots: Tessera");

    std::printf(fails ? "midifile test: %d FAILED\n" : "midifile test: ok\n", fails);
    return fails ? 1 : 0;
}
