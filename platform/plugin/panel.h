/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1, drawn in software (no GUI library): the web simulator's wide layout (web/sim/index.html
 * LAYOUTS.wide, 1000 x 624) in one of the FM-1's shells, the screen at 1:1, the labels in the Mono face (firmware/src/mo_font.h).
 * Platform-independent: gui_win32.cpp / gui_x11.cpp give it pixels and the mouse. Not thread-safe: the GUI thread only. */
#pragma once
#include <cstdint>
#include <string>
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
    /* the computer keyboard, as the web simulator maps it (by where a key is, not what it prints): code a PC scan code
     * (set 1: Z 0x2C, Q 0x10, 1 0x02 ..; what Windows reports, and X11's keycode - 8) or KEY_UP .. KEY_RIGHT. Z..' and
     * Q..O the keys (F3..G5), 1..= the twelve buttons in the panel's order, [ ] OCT- OCT+ (each held while its key is),
     * up / down SELECT, left / right ALGORITHM. true if the input changed. release_keys: every key up (focus lost) */
    enum { KEY_UP = 0x148, KEY_LEFT = 0x14B, KEY_RIGHT = 0x14D, KEY_DOWN = 0x150 };
    bool key(int code, bool down);
    bool release_keys();
    PanelInput input;
    /* the project-file buttons under the keys: OPEN (a .tsp or an FM-1 backup) and SAVE (a .tsp). A click asks; the
     * window takes the ask (1 OPEN, 2 SAVE, 0 none) and shows the outcome with note() for a few seconds */
    enum { FILE_NONE, FILE_OPEN, FILE_SAVE };
    int take_file_request() { int r = file_req; file_req = FILE_NONE; return r; }
    void note(const std::string &text);
    /* the FM-1's shells (web/sim/index.html SHELLS): a row of swatches under the keys. A click shows that shell at once
     * and asks for its ink (take_shell_request: the shell's index, -1 none); each shell's id (stored) and Mono ink
     * (MENU > COLOR's name) */
    static int shell_count();
    static const char *shell_id(int i);
    static const char *shell_ink(int i);
    static int shell_by_id(const std::string &id);   // -1 if none
    void set_shell(int i);
    int shell() const { return shell_; }
    int take_shell_request() { int r = shell_req; shell_req = -1; return r; }

  private:
    struct Knob { int x, y, r; const char *name; int role; double angle; };   // role -1: MASTER
    struct Button { int x, y, w, h, bit; const char *face, *sub; };
    struct Key { int cx, cy, k; };
    struct FileButton { int x, y, w, h, action; const char *face; };
    std::vector<FileButton> files;
    int file_req = FILE_NONE, file_held = -1;
    std::string note_text;
    static constexpr int SWATCH_R = 9;
    int shell_ = 0, shell_req = -1, shell_hover = -1;
    void swatch_xy(int i, int &x, int &y) const;
    int swatch_at(int x, int y) const;
    int note_frames = 0;
    std::vector<Knob> knobs;
    std::vector<Button> buttons;
    std::vector<Key> keys;
    int pitch = 0, kw = 0, kh = 0, sw = 0;
    // held by the mouse and by the computer keyboard, apart (input.keys / input.buttons: the two together)
    uint32_t mouse_keys = 0, mouse_buttons = 0, kbd_keys = 0, kbd_buttons = 0;
    void merge_held();
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
