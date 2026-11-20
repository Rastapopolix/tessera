/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1, drawn in software (no GUI library): the web simulator's wide layout (web/sim/index.html
 * LAYOUTS.wide, 1000 x 624) in the black shell, the screen at 1:1, the labels in the Mono face (firmware/src/mo_font.h).
 * Platform-independent: gui_win32.cpp / gui_x11.cpp give it pixels and the mouse. Not thread-safe: the GUI thread only. */
#pragma once
#include <cstdint>
#include <vector>

#include "tess_core.h"

/* what the panel asks the core to do; the plugin hands it to the audio thread (tessera_clap.cpp) */
struct PanelInput
{
    uint32_t buttons = 0, keys = 0;   // held now
    int32_t enc[TESS_NE] = {};        // detents turned since the last take
    int32_t master = -1;              // 0..1023 when the knob was turned, else -1
    bool changed = false;
};

class Panel
{
  public:
    static constexpr int W = 1000, H = 624;
    Panel();
    /* the whole window into px (W x H, 0xAARRGGBB, top row first) */
    void draw(uint32_t *px, const tess_view_t &v);
    /* the mouse, in window pixels; wheel: notches, + = up / clockwise. Each returns true if the input changed */
    bool down(int x, int y);
    bool move(int x, int y);
    bool up(int x, int y);
    bool wheel(int x, int y, int notches);
    PanelInput input;

  private:
    struct Knob { int x, y, r; const char *name; int role; double angle; };   // role -1: MASTER
    struct Button { int x, y, w, h, bit; const char *face, *sub; };
    struct Key { int cx, cy, k; };
    std::vector<Knob> knobs;
    std::vector<Button> buttons;
    std::vector<Key> keys;
    int pitch = 0, kw = 0, kh = 0, sw = 0;
    // a drag
    enum { NONE, KNOB, BUTTON, KEY } drag = NONE;
    int drag_i = -1, last_x = 0, last_y = 0, acc = 0;
    int master_now = 700;   // MASTER as the core has it (the view: the DAW may automate it)
    uint32_t frame = 0;     // frames drawn (the breathing LEDs: each window its own)
    int knob_at(int x, int y) const;
    int button_at(int x, int y) const;
    int key_at(int x, int y) const;
    void turn(int i, int n);
};
