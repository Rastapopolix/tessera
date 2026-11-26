/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window: a native child window of the host's (Win32, X11 or Cocoa) showing the Panel. gui_win32.cpp,
 * gui_x11.cpp and gui_mac.mm implement it (gui_none.cpp: none, for a platform without one). All on the host's main
 * thread. */
#pragma once
#include <clap/clap.h>

#include <memory>
#include <string>

#include "fm1_link.h"
#include "panel.h"
#include "projfile.h"

/* the window's instance (tessera_clap.cpp): the latest view its audio thread made, and the panel's input for it */
struct GuiLink
{
    virtual ~GuiLink() = default;
    virtual void view(tess_view_t &v) = 0;
    virtual void input(PanelInput &in) = 0;
    /* the project files (on the host's main thread), each returning what to show under the keys: open a .tsp or an
     * FM-1 backup (its music playing, its slots A..D); save the project as a .tsp; the name to offer for that */
    virtual std::string open_file(const std::string &path_utf8) = 0;
    virtual std::string save_file(const std::string &path_utf8) = 0;
    virtual std::string file_name() = 0;
    /* MIDI: the notes played since PLAY as a .mid clip (Documents/Tessera/MIDI); what to show */
    virtual std::string export_midi() { return ""; }
    /* the shell the window shows (Panel::shell_count), and one picked under the keys: the instance sets its ink and
     * remembers it for the next window */
    virtual int shell() = 0;
    virtual void shell_picked(int i) = 0;
    /* the screen's inks, MENU > COLOR's choices (the swatches under the buttons); one picked there: the instance sets
     * it; the ink the screen shows changed (a pick, a shell's ink, MENU > COLOR): remembered for the next window */
    virtual std::vector<Panel::Ink> inks() = 0;
    virtual void ink_picked(int i) = 0;
    virtual void ink_seen(int i) = 0;
    /* the build's tracks (4 or 8: the window's 1-4 / 5-8 switch) */
    virtual int tracks() = 0;
    /* MIDI in so far (notes and messages from the host, a count that wraps): TESSERA's shell lights its MIDI lamp */
    virtual uint32_t midi_count() { return 0; }
    /* the standalone app (no DAW's transport): its own shortcuts (Panel::set_standalone) */
    virtual bool standalone() { return false; }
    /* an FM-1 on USB (fm1_link.h): this instance's state, and a click: on (taken from any other Tessera) or off (for
     * every Tessera) */
    virtual Fm1Link::State fm1_state() { return Fm1Link::OFF; }
    virtual void fm1_set(bool on) { (void)on; }
    /* SIZE (TESSERA's menu): the window's size now in % (75 to 200; EXPANDED to 150: Panel::size_fit), and a
     * pick: the instance remembers it for every window and asks the host for the new size */
    virtual int size_pct() { return 100; }
    virtual void size_pick(int pct) { (void)pct; }
    /* MODE (TESSERA's menu): EXPANDED picked (remembered for every window; the window is EXPANDED in TESSERA's shell),
     * and a pick (the host asked for the new size); TRACKS as the core last drew it (EXPANDED: false if none yet) */
    virtual bool expanded() { return false; }
    virtual void mode_pick(bool on) { (void)on; }
    virtual bool tracks_view(tess_tracks_t &t) { t.w = t.h = 0; t.zone_count = 0; return false; }
    /* GENERATE (TESSERA's strip; on the host's main thread): the styles' names (none: a build without it); a new piece
     * in a style at a VARIATION, from a new seed; before / after; a piece made before made again (dir -1 an older one,
     * +1 a newer); the style and VARIATION picked (remembered for the next window). Each returns what to show under the
     * keys (empty: nothing). gen_state: what the window shows */
    virtual std::vector<std::string> gen_styles() { return {}; }
    virtual std::string generate(int style, int variation) { (void)style; (void)variation; return ""; }
    virtual std::string gen_swap() { return ""; }
    virtual std::string gen_recall(int dir) { (void)dir; return ""; }
    virtual void gen_pick(int style, int variation) { (void)style; (void)variation; }
    virtual Panel::GenState gen_state() { return {}; }
    /* TRACK (TESSERA's menu, Ctrl+C / Ctrl+V ..; on the host's main thread): what (Panel::TR_*) done to track; what to
     * show under the keys */
    virtual std::string track_do(int what, int track) { (void)what; (void)track; return ""; }
    /* KEYS (TESSERA's menu): whether the computer keyboard plays Tessera, the same for every window (a setting) */
    // LISTEN: the MIDI channel whose notes set the key and GENERATE (listen.h), 0 off; this instance's
    virtual int listen_channel() { return 0; }
    virtual void listen_pick(int ch) { (void)ch; }
    // LISTEN's SPLIT: the first note that sets the key (the notes under it GENERATE), a C 12..108; this instance's
    virtual int listen_split_note() { return 48; }
    virtual void listen_split_pick(int note) { (void)note; }
    virtual bool keys_on() { return projfile::setting_get("keys") != "0"; }
    virtual void keys_set(bool on) { projfile::setting_put("keys", on ? "1" : "0"); }
    /* HOVER HELP (TESSERA's menu), remembered as KEYS is: off unless turned on */
    virtual bool hover_help_on() { return projfile::setting_get("hover_help") == "1"; }
    virtual void hover_help_set(bool on) { projfile::setting_put("hover_help", on ? "1" : "0"); }
    /* OUTPUTS (TESSERA's menu): the track ports mono or stereo, the setting (the next instance's) and this instance's */
    virtual bool outs_mono() { return false; }
    virtual bool outs_live_mono() { return false; }
    virtual void outs_set(bool mono) { (void)mono; }
};

/* each window's tick: a click on FM-1 USB handed on, and its state shown */
inline void gui_fm1_tick(Panel &panel, GuiLink *link)
{
    int f = panel.take_fm1_request();
    if (f >= 0)
        link->fm1_set(f == 1);
    Fm1Link::State s = link->fm1_state();
    panel.set_fm1(s != Fm1Link::OFF, s != Fm1Link::ELSEWHERE, s == Fm1Link::LIVE, fm1_state_text(s));
}

/* each window's tick: MODE and SIZE picked in the menu handed on; the mode and the size shown */
inline void gui_size_tick(Panel &panel, GuiLink *link)
{
    int m = panel.take_mode_request();
    if (m >= 0)
        link->mode_pick(m == 1);
    panel.set_expanded(link->expanded());
    int pct = panel.take_size_request();
    if (pct > 0)
        link->size_pick(pct);
    panel.set_size_pct(link->size_pct());
}

/* each window's tick: LISTEN and SPLIT in the menu (a step asked for handed on, the channel and SPLIT shown) */
inline void gui_listen_tick(Panel &panel, GuiLink *link)
{
    int ch = panel.take_listen_request();
    if (ch >= 0)
        link->listen_pick(ch);
    int sp = panel.take_split_request();
    if (sp >= 0)
        link->listen_split_pick(sp);
    panel.set_listen(link->listen_channel());
    panel.set_split(link->listen_split_note());
    int o = panel.take_outs_request();   // (OUTPUTS: the setting; this instance keeps its ports)
    if (o >= 0)
        link->outs_set(o == 1);
    panel.set_outs(link->outs_mono(), link->outs_live_mono());
}

/* each window's tick: GENERATE's strip (its styles once; a press handed on, its answer shown; the state shown); TRACK's
 * picks handed on */
inline void gui_gen_tick(Panel &panel, GuiLink *link)
{
    if (!panel.gen_known())
    {
        panel.set_gen_styles(link->gen_styles());
        panel.gen_asked();
    }
    Panel::GenRequest r = panel.take_gen_request();
    if (r.what != Panel::GEN_NONE)
    {
        link->gen_pick(r.style, r.variation);
        std::string said = r.what == Panel::GEN_MAKE   ? link->generate(r.style, r.variation)
                           : r.what == Panel::GEN_SWAP   ? link->gen_swap()
                           : r.what == Panel::GEN_RECALL ? link->gen_recall(r.dir)
                                                         : std::string();
        if (!said.empty())
            panel.note(said);
    }
    panel.set_gen(link->gen_state());
    Panel::TrackRequest t = panel.take_track_request();   // (TRACK: copy, paste, move)
    if (t.what != Panel::TR_NONE)
    {
        std::string said = link->track_do(t.what, t.track);
        if (!said.empty())
            panel.note(said);
    }
}

/* the plugin's version, for the window's ABOUT (TESS_VERSION, CMakeLists.txt) */
inline const char *tess_version()
{
#ifdef TESS_VERSION_STR
    return TESS_VERSION_STR;
#else
    return "";
#endif
}

class Gui
{
  public:
    virtual ~Gui() = default;
    virtual bool set_parent(const clap_window_t *w) = 0;
    virtual bool show() = 0;
    virtual bool hide() = 0;
    virtual void tick() = 0;   // ~30 times a second (X11: from the host's timer)
    /* the window's size: the panel at pct % (100, 125, 150, 200), its width the panel's (FM-1 or EXPANDED: the window
     * follows Panel::width() each tick); before set_parent, the size it opens at */
    virtual void set_scale(int pct) { (void)pct; }
    static const char *api();  // CLAP_WINDOW_API_WIN32 / _X11 / _COCOA, or nullptr
    /* the main screen's width in the units a window is sized in (X11 and Windows pixels, macOS points; the room the
     * OS leaves where it says), 0 if unknown: whether EXPANDED fits, for a window with no MODE picked yet */
    static int screen_width();
    static std::unique_ptr<Gui> make(const clap_host_t *host, GuiLink *link);
};
