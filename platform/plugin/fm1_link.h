/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* An FM-1 on USB plays the plugin: the web simulator's "An FM-1 on USB" (web/sim/index.html), for the plugin. The
 * plugin opens the FM-1's own USB MIDI ports (the DAW is not involved: the FM-1 must be off in the DAW's MIDI settings
 * on Windows, where one program holds a port), asks with PANEL (editor SysEx 75, web/EDITOR_PROTOCOL.md) about once a
 * second, and while the FM-1 answers, its buttons (CC 80..93), keys (notes 0..26), knobs (CC 20..26, sign bit) and
 * MASTER (CC 27) on channel 16 press, turn and set the plugin's panel. With the FM-1's SIM CONTROL on (its menu), the
 * FM-1 itself goes quiet and plays only this. One FM-1 for the whole process: one instance has it at a time (the one
 * that turned it on last, else the oldest that wants it), the others show that another Tessera does.
 * The ports: fm1_midi_win.cpp (WinMM), fm1_midi_linux.cpp (ALSA's raw MIDI devices, no library), fm1_midi_mac.cpp
 * (CoreMIDI). No firmware change: the FM-1 side is ed_panel (firmware/src/editor.c), as for the web simulator. */
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "tess_core.h"

/* what the FM-1 played: buttons and keys held now, knob detents summed since the last take, MASTER (0..1023, -1: not
 * moved) */
struct Fm1Input
{
    uint32_t buttons = 0, keys = 0;
    int32_t enc[TESS_NE] = {};
    int32_t master = -1;
};

/* the FM-1's MIDI read as a byte stream (running status, SysEx in pieces, real-time bytes in between): PANEL's answer,
 * and channel 16's panel messages into an Fm1Input. Pure (fm1_link_test.cpp) */
struct Fm1Decoder
{
    Fm1Input in;
    bool answered = false;   // PANEL's answer (F0 7D 46 4C 4B 01 F7) seen (the link clears it at each ask)
    bool changed = false;    // something in `in` since the last take
    void bytes(const uint8_t *b, size_t n);
    void release();          // every button and key let go (the FM-1 gone or quiet)
    void take_turns();       // the detents and MASTER handed on: cleared (buttons and keys stay as held)

  private:
    uint8_t status = 0, data[2] = {}, sysex[8] = {};
    int have = 0, sx = -1;   // sx: SysEx bytes so far (-1: not in one)
    void message(uint8_t st, uint8_t a, uint8_t b);
};

/* PANEL on (1) or off (0), as the editor and the simulator send it */
extern const uint8_t FM1_PANEL_ON[7], FM1_PANEL_OFF[7];

/* the FM-1's port pair (a backend): opened by name, the bytes it sends handed to `in` (on any thread, never two at
 * once) */
struct Fm1Ports
{
    virtual ~Fm1Ports() = default;
    virtual bool send(const uint8_t *b, size_t n) = 0;
    virtual bool alive() = 0;   // still there (unplugged: false)
    virtual void poll() {}      // the link's thread, ~20 times a second (WinMM: SysEx buffers handed back)
    /* open the first input and output whose names match (fm1_port_name); busy: a match, but another program has it */
    static std::unique_ptr<Fm1Ports> open(std::function<void(const uint8_t *, size_t)> in, bool &busy);
};
/* "Tessera", "Melodee", "Felucca", or "FM-1" / "FM1" as a word (any case): the web simulator's RE_FM1_PORT */
bool fm1_port_name(const std::string &name);

class Fm1Link
{
  public:
    /* as an instance sees it: OFF (it doesn't want it), LOOKING (no FM-1 port), BUSY (another program has the port),
     * OLD (the FM-1 doesn't answer PANEL: firmware before 3.5.1, or not Tessera), LIVE (it plays this instance),
     * ELSEWHERE (it plays another Tessera) */
    enum State { OFF, LOOKING, BUSY, OLD, LIVE, ELSEWHERE };
    static Fm1Link &get();
    /* an instance wants it or not. first: it takes the FM-1 over from any other (a click in its window); else it
     * waits its turn behind them (an instance opening with the setting on). closed: an instance gone (as off, and
     * forgotten) */
    void want(const void *owner, bool on, bool first = true);
    void closed(const void *owner);
    /* off for every instance (a click on OFF: the setting is the plugin's, not one instance's) */
    void off_all();
    State state(const void *owner);
    /* the audio thread: what the FM-1 played since the last take, for this instance (never waits: false if busy or
     * nothing new). An instance that lost it (unplugged, taken over, turned off) gets one take of everything let go */
    bool take(const void *owner, Fm1Input &out);
    /* tests: a stand-in for the ports, a shorter ask (ms) */
    using Opener = std::function<std::unique_ptr<Fm1Ports>(std::function<void(const uint8_t *, size_t)>, bool &)>;
    void test_hooks(Opener open, int ask_ms);
    ~Fm1Link();

  private:
    Fm1Link();
    struct Impl;
    std::unique_ptr<Impl> impl;
};

/* how a state reads in the window (capitals, the Mono face) */
const char *fm1_state_text(Fm1Link::State s);
