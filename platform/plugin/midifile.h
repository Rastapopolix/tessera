/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* What Tessera played as a MIDI clip: the notes MIDI OUT sends (each track on its channel), kept as they go by
 * (Recorder: a fixed ring, no allocation on the audio thread), written as a Standard MIDI File (format 1: the tempo and
 * the project's name, then a track per channel, named T1..T8) for a DAW to take as a clip. */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace midifile
{
struct Ev
{
    double beat;              // quarter notes from the song's top (tess_core_position's)
    uint8_t status, d1, d2;   // a channel message (note on / off; the rest are kept out)
};

class Recorder
{
  public:
    explicit Recorder(size_t cap = 1u << 16) : ring_(cap) {}
    void clear() { head_ = count_ = 0; }
    /* a message as it plays: notes only (on, off), the oldest dropped once the ring is full */
    void add(double beat, uint8_t status, uint8_t d1, uint8_t d2)
    {
        uint8_t kind = status & 0xF0;
        if (kind != 0x80 && kind != 0x90)
            return;
        ring_[head_] = {beat, status, d1, d2};
        head_ = (head_ + 1) % ring_.size();
        if (count_ < ring_.size()) count_++;
    }
    /* what is kept, oldest first */
    std::vector<Ev> take() const
    {
        std::vector<Ev> out;
        out.reserve(count_);
        size_t at = (head_ + ring_.size() - count_) % ring_.size();
        for (size_t i = 0; i < count_; i++) out.push_back(ring_[(at + i) % ring_.size()]);
        return out;
    }
    size_t size() const { return count_; }

  private:
    std::vector<Ev> ring_;
    size_t head_ = 0, count_ = 0;
};

/* a Standard MIDI File (format 1, 480 ticks a quarter; firmware/src/smf.c, the writer Andamento shares). bars 0: the
 * clip starts on the bar (4/4) the first note is in, a note still on at the end ends with the last one; bars N: from
 * the song's top for N bars exactly (a note still on ends there). name: the first track's name ("" none). Empty if
 * there are no notes */
std::vector<uint8_t> smf(const std::vector<Ev> &events, double bpm, const std::string &name, uint32_t bars = 0);
/* the folder exported clips go in, made if need be (UTF-8; "" if there is none): Documents/Tessera/MIDI in the user's
 * home (%USERPROFILE% on Windows, $HOME elsewhere) */
std::string folder();
/* a clip's file name: the project's name (Tessera for none, the characters a file name cannot have left out) and
 * stamp ("2026-11-22 1635"), "UNDERTOW 2026-11-22 1635.mid" */
std::string file_name(const std::string &project, const std::string &stamp);
/* (tests) a file smf wrote, read back: its tracks, notes (note-ons with a velocity) and length in quarters (to the
 * last end-of-track) */
bool read_back(const std::vector<uint8_t> &file, int &tracks, int &notes, double &quarters);
}   // namespace midifile
