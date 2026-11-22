/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1, drawn in software (no GUI library): the web simulator's wide layout (web/sim/index.html
 * LAYOUTS.wide, 1000 x 624) in one of the FM-1's shells, or in TESSERA's own (shell 0, the default: the project page's
 * look, flat and 1-bit, its settings in a menu under the mark), the screen at 1:1, the labels in the Mono face
 * (firmware/src/mo_font.h). TESSERA's shell has a second mode, EXPANDED (MODE in its menu): 1500 x 624, the TRACKS
 * screen (tess_core_tracks_view) beside the FM-1's, the knobs and buttons to its right, the keys as wide as the window.
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
    int32_t track = -1;               // a track to select (the 1-4 / 5-8 switch: T1 or T5), else -1
    // what the mouse did on the screen's zones (tess_view_t.zones), in order, for tess_core_zone_do; and on the TRACKS
    // screen's (tess_tracks_t.zones), for tess_core_tracks_do
    struct ZoneAct { tess_zone_t z; uint32_t action; int32_t n; };
    std::vector<ZoneAct> zones, tzones;
    bool changed = false;
};

class Panel
{
  public:
    /* the panel's size at 1:1: W x H (FM-1 mode), EXPANDED's W_EXPANDED x H; width() the one it is now (a buffer of
     * W_MAX x H holds either, row by row at width()) */
    static constexpr int W = 1000, H = 624, W_EXPANDED = 1500, W_MAX = 1500;
    static int width_of(bool expanded) { return expanded ? W_EXPANDED : W; }
    int width() const { return width_of(expanded()); }
    /* the window's size: SIZE in % (TESSERA's menu): 100, 150 or 200 in FM-1 mode, 100 or 125 in EXPANDED (2250 px would
     * not fit a 1080p screen); size_fit: the nearest of those to pct. The panel draws at 1:1; a window shows it pct /
     * 100 times larger and hands the mouse in at 1:1 (x * 100 / pct) */
    static int size_fit(int pct, bool expanded)
    {
        if (expanded) return pct >= 113 ? 125 : 100;
        return pct >= 175 ? 200 : pct >= 125 ? 150 : 100;
    }
    /* px (w x H) into out (w * pct / 100 x H * pct / 100): 100 % a copy, 200 % each pixel doubled, 125 and 150 % each
     * window pixel the mean of the 1:1 pixels under its quarter points (the 1-bit lines stay even, a little soft) */
    static void scale(const uint32_t *px, uint32_t *out, int w, int pct);
    /* MODE (TESSERA's menu): FM-1 or EXPANDED, as the instance has it (set_expanded), and a pick asks
     * (take_mode_request: 1 EXPANDED, 0 FM-1, -1 none). Only TESSERA's shell is EXPANDED: an FM-1 shell is FM-1 mode */
    void set_expanded(bool on) { if (on != expanded_) { expanded_ = on; layout(); } }
    bool expanded() const { return expanded_ && native(); }
    int take_mode_request() { int r = mode_req; mode_req = -1; return r; }
    /* SIZE (TESSERA's menu): what the window is (set_size_pct), and a pick asks (take_size_request: 100, 125, 150 or
     * 200, -1 none) */
    void set_size_pct(int pct) { size_now = pct; }
    int take_size_request() { int r = size_req; size_req = -1; return r; }
    Panel();
    /* the whole window into px (width() x H, 0xAARRGGBB, top row first); EXPANDED: the TRACKS screen from t2 (none:
     * black) */
    void draw(uint32_t *px, const tess_view_t &v, const tess_tracks_t *t2 = nullptr);
    /* the mouse, in window pixels; wheel: notches, + = up / clockwise. Each returns true if the input changed. On the
     * screen, the zones the view has (tess_view_t.zones) come first: a value's column turns with a drag or the wheel
     * (a double-click: its default), a list's row takes a click (a double-click: and OK; the wheel: its value), OK,
     * BACK, YES, NO and HOME's tracks a click; the one under the mouse outlined, its name and value large beside it */
    bool down(int x, int y);
    bool move(int x, int y);
    bool up(int x, int y);
    bool wheel(int x, int y, int notches);
    /* a right-click (or Ctrl-click on a Mac): a button or key latched, held until it is right-clicked or clicked again
     * (or Esc), so the mouse can play the FM-1's two-handed combos (GLO + a button, SEL + a key). true if changed */
    bool latch(int x, int y);
    uint32_t latched_buttons() const { return latch_buttons; }
    /* the computer keyboard, as the web simulator maps it (by where a key is, not what it prints): code a PC scan code
     * (set 1: Z 0x2C, Q 0x10, 1 0x02 ..; what Windows reports, and X11's keycode - 8) or KEY_UP .. KEY_RIGHT. Z..' and
     * Q..O the keys (F3..G5), 1..= the twelve buttons in the panel's order, [ ] OCT- OCT+ (each held while its key is),
     * up / down SELECT, left / right ALGORITHM. true if the input changed. release_keys: every key up (focus lost) */
    enum { KEY_UP = 0x148, KEY_LEFT = 0x14B, KEY_RIGHT = 0x14D, KEY_DOWN = 0x150 };
    bool key(int code, bool down);
    bool release_keys();
    /* KEYS (TESSERA's menu): the computer keyboard plays Tessera (on), or every key is left to the DAW (off). A click
     * asks (take_keys_request: 1 on, 0 off, -1 none); the window remembers it */
    void set_keys(bool on) { keys_on_ = on; if (!on) release_keys(); }
    bool keys_on() const { return keys_on_; }
    int take_keys_request() { int r = keys_req; keys_req = -1; return r; }
    /* LISTEN (TESSERA's menu): the MIDI channel whose notes set the key and GENERATE (listen.h), 0 OFF, 1..16. < and >
     * step it, its value turns it OFF or back on (16 if never on); a click asks (take_listen_request: the channel, -1
     * none); the window shows the instance's */
    void set_listen(int ch) { listen_ch = ch; if (ch) listen_last = ch; }
    int listen() const { return listen_ch; }
    int take_listen_request() { int r = listen_req; listen_req = -1; return r; }
    /* LISTEN's SPLIT (the row under it): the first note that sets the key, a C 12..108 shown as its note number (the
     * notes under it GENERATE); < and > step it an octave; a click asks (take_split_request: the note, -1 none) */
    void set_split(int note) { split_note = note; }
    int split() const { return split_note; }
    int take_split_request() { int r = split_req; split_req = -1; return r; }
    PanelInput input;
    /* the build's tracks (tess_core_tracks): with 8, a 1-4 / 5-8 switch beside OPEN / SAVE selects T1 or T5 (Tab too),
     * the bank of the track selected lit */
    void set_tracks(int n) { tracks_ = n; layout(); }
    /* the project-file buttons under the keys: OPEN (a .tsp or an FM-1 backup) and SAVE (a .tsp). A click asks; the
     * window takes the ask (1 OPEN, 2 SAVE, 0 none) and shows the outcome with note() for a few seconds */
    enum { FILE_NONE, FILE_OPEN, FILE_SAVE, FILE_MIDI };
    int take_file_request() { int r = file_req; file_req = FILE_NONE; return r; }
    void note(const std::string &text);
    /* the shells: TESSERA's own (0), then the FM-1's (web/sim/index.html SHELLS). The FM-1's show a row of swatches
     * under the keys; TESSERA's a menu under the mark (SHELL, INK, MODE, SIZE, ABOUT). A pick shows that shell at once
     * and asks for its ink (take_shell_request: the shell's index, -1 none); each shell's id (stored) and Mono ink
     * (MENU > COLOR's name) */
    static int shell_count();
    static const char *shell_id(int i);
    static const char *shell_ink(int i);
    static int shell_by_id(const std::string &id);   // -1 if none
    void set_shell(int i);
    int shell() const { return shell_; }
    int take_shell_request() { int r = shell_req; shell_req = -1; return r; }
    /* the screen's inks (MENU > COLOR's choices, tess_core_ink_at): a row of swatches under the buttons, the one the
     * view has (tess_view_t.ink) ringed, its name while the mouse is on one. A click asks for that ink
     * (take_ink_request: its index, -1 none). rgb: the four track colours, alike for one ink */
    struct Ink { std::string name; uint32_t rgb[4]; };
    void set_inks(const std::vector<Ink> &list) { inks = list; }
    int ink_count() const { return (int)inks.size(); }
    int take_ink_request() { int r = ink_req; ink_req = -1; return r; }
    /* MIDI in, as the instance counts it (any change: notes came in): TESSERA's shell lights its MIDI lamp a moment */
    void set_midi(uint32_t count);
    /* the plugin's version (ABOUT in TESSERA's menu) */
    void set_version(const std::string &v) { version = v; }
    bool menu_shown() const { return menu_open; }
    /* an FM-1 on USB playing this instance (fm1_link.h): TESSERA's menu has FM-1 USB OFF / ON with its state under it,
     * the FM-1's shells an FM-1 USB button under the keys, lit while on. on: this instance wants it; mine: and no other
     * Tessera has it; live: it plays this one; text: the state in words, shown as a note when it changes. A click asks
     * (take_fm1_request: 1 on, taken from any other Tessera; 0 off; -1 none) */
    void set_fm1(bool on, bool mine, bool live, const std::string &text);
    int take_fm1_request() { int r = fm1_req; fm1_req = -1; return r; }
    /* GENERATE (TESSERA's shell, in the strip at the top; a build without it has no styles and shows none of it):
     * GENERATE makes a new piece in the style shown, from a new seed; held half a second, BEFORE / AFTER: the project
     * before the last piece and the piece change places. The style: a click on its name opens the list, the arrows or
     * the wheel the next one or the one before. VAR 0..100: how wide each pick is (a drag or the wheel, 5 a detent; a
     * double-click: 100). The seed: the piece's; the wheel over it makes the pieces made before again (and back). A
     * press asks (take_gen_request); set_gen shows what the instance has */
    enum { GEN_NONE, GEN_MAKE, GEN_SWAP, GEN_RECALL, GEN_SET };
    struct GenRequest { int what = GEN_NONE, style = 0, variation = 100, dir = 0; };
    struct GenState { int style = 0, variation = 100, at = -1, made = 0; uint32_t seed = 0; bool before = false; };
    void set_gen_styles(const std::vector<std::string> &names) { gen_names = names; }
    bool gen_known() const { return gen_names_asked; }
    void gen_asked() { gen_names_asked = true; }
    void set_gen(const GenState &g);
    GenRequest take_gen_request() { GenRequest r = gen_req; gen_req = GenRequest{}; return r; }

  private:
    // GENERATE's strip (TESSERA's shell): the button, the style, VAR, the seed; the style's list
    std::vector<std::string> gen_names;
    bool gen_names_asked = false;
    GenState gen{};
    GenRequest gen_req{};
    bool gen_list = false, gen_swapped = false;
    int gen_list_hover = -1;
    uint32_t gen_down_frame = 0;
    double gen_var_click = 0;
    bool gen_on() const { return native() && !gen_names.empty(); }
    enum { GP_BUTTON, GP_PREV, GP_STYLE, GP_NEXT, GP_VAR, GP_SEED };
    int gen_part_at(int x, int y) const;   // GP_*, -1 none
    int gen_list_at(int x, int y) const;
    void gen_ask(int what, int dir = 0);
    void gen_style_step(int n);
    void gen_var_step(int n);
    void draw_gen(uint32_t *px);
    // TESSERA's shell (shell 0): its own drawing, its layout of the buttons and files, and its menu
    bool native() const { return shell_ == 0; }
    void layout();
    void draw_native(uint32_t *px, const tess_view_t &v, double breath, const tess_tracks_t *t2);
    bool menu_open = false;
    bool expanded_ = false;
    int mode_req = -1;
    int mode_seg_at(int x) const;   // MODE's row: 0 FM-1, 1 EXPANDED, -1 none
    int size_now = 100, size_req = -1;
    int size_seg_at(int x) const;   // SIZE's row: 100, 125, 150, 200, -1 none
    int menu_sub = 0, menu_hover = -1, sub_hover = -1;   // menu_sub: 0 none, 1 SHELL's list, 2 INK's swatches
    int menu_row_at(int x, int y) const;
    int sub_at(int x, int y) const;
    bool menu_click(int x, int y);
    uint32_t turned[8] = {};      // the frame each knob last turned (TESSERA's shell shows a turning encoder)
    uint32_t midi_count = 0, midi_frame = 0;
    bool midi_known = false;
    std::string version;
    bool fm1_on = false, fm1_mine = false, fm1_live = false, fm1_known = false;
    std::string fm1_text;
    int fm1_req = -1;
    int fm1_seg_at(int x) const;       // TESSERA's menu: 0 OFF, 1 ON, -1 neither
    struct Knob { int x, y, r; const char *name; int role; double angle; };   // role -1: MASTER
    struct Button { int x, y, w, h, bit; const char *face, *sub; };
    struct Key { int cx, cy, k; };
    struct FileButton { int x, y, w, h, action; const char *face; };
    std::vector<FileButton> files;
    FileButton usb{};                  // the FM-1's shells: FM-1 USB, under the keys
    FileButton banks[2];               // the 1-4 / 5-8 switch (8 tracks); action: the bank's first track
    int tracks_ = 4, track_now = 0, bank_held = -1;
    bool banked() const { return tracks_ > 4 && !expanded(); }   // the 1-4 / 5-8 switch (EXPANDED: TRACKS shows all)
    int bank_at(int x, int y) const;
    int file_req = FILE_NONE, file_held = -1;
    std::string note_text;
    static constexpr int SWATCH_R = 9;
    static constexpr int SHELL_GAP = 14;   // (TESSERA's swatch apart from the FM-1's)
    int shell_ = 0, shell_req = -1, shell_hover = -1;
    void swatch_xy(int i, int &x, int &y) const;
    int swatch_at(int x, int y) const;
    static constexpr int INK_R = 7;
    std::vector<Ink> inks;
    int ink_now = -1, ink_req = -1, ink_hover = -1, ink_pending = -1;
    void ink_xy(int i, int &x, int &y) const;
    int ink_at(int x, int y) const;
    int note_frames = 0;
    std::vector<Knob> knobs;
    std::vector<Button> buttons;
    std::vector<Key> keys;
    int pitch = 0, kw = 0, kh = 0, sw = 0, kb_w = 928;   // (kb_w: the keybed's width, as wide as the window less 72)
    // held by the mouse and by the computer keyboard, apart (input.keys / input.buttons: the two together)
    uint32_t mouse_keys = 0, mouse_buttons = 0, kbd_keys = 0, kbd_buttons = 0, latch_keys = 0, latch_buttons = 0;
    bool keys_on_ = true;
    int keys_req = -1;
    int listen_ch = 0, listen_last = 16, listen_req = -1, split_note = 48, split_req = -1;
    int listen_seg_at(int x) const;
    void merge_held();
    // a drag
    enum { NONE, KNOB, BUTTON, KEY, ZONE, GEN_BUTTON, GEN_VAR } drag = NONE;
    int drag_i = -1, last_x = 0, last_y = 0, acc = 0;
    int master_now = 700;   // MASTER as the core has it (the view: the DAW may automate it)
    uint32_t frame = 0;     // frames drawn (the breathing LEDs: each window its own)
    int knob_at(int x, int y) const;
    int button_at(int x, int y) const;
    int key_at(int x, int y) const;
    void turn(int i, int n);
    // the screen's zones: the view's last, the mouse over them, a click's time for a double-click
    void draw_body(uint32_t *px, const tess_view_t &v, const tess_tracks_t *t2);
    void draw_hover(uint32_t *px);
    std::vector<tess_zone_t> zones, zones2;   // the FM-1 screen's, the TRACKS screen's (EXPANDED)
    tess_zone_t drag_zone{}, click_zone{};
    bool click_known = false, hover_in = false;
    int hover_x = 0, hover_y = 0, click_x = 0, click_y = 0;
    double click_t = 0;
    int zone_at(int x, int y) const;
    int zone2_at(int x, int y) const;   // a TRACKS zone under the mouse (EXPANDED), -1 none
    int zone_like(const tess_zone_t &z) const;   // the view's zone that is z, -1 none
    void zone_act(const tess_zone_t &z, uint32_t action, int n);
};
