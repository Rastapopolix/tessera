/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera as a CLAP instrument (phase 0 spike). clap-wrapper turns the same code into VST3 and a standalone app
 * (CMakeLists.txt). What it does:
 *   - 8 tracks (core_defs.txt CORE_TRACKS=8: the FM-1's 4, then 5..8 on the panel's second bank);
 *   - outputs: the main mix, then TRACK 1..8 alone (the FM-1's USB stems); a MIDI out with every track's notes on its
 *     own channel (Settings > NOTES OUT ALL by default here; GLO > MIDI OUT per track);
 *   - the core runs at 44.1 kHz (tess_core.h) and is resampled (linear) to the host's rate;
 *   - notes and MIDI in (CLAP or MIDI dialect) go to the core as USB MIDI does on the FM-1, at their sample; ROUT
 *     starts at SEL here (a DAW's channel 1 plays the selected track), a wildcard note-off or a choke is All Notes Off
 *     / All Sound Off; CLK stays INT and CLOCK OUT off (the DAW keeps time), and MIDI OUT sends only the notes; the
 *     device's MIDI setup (ROUT, DRUM's channel, NOTES OUT, TUNE) is kept in the DAW project beside the .tsp;
 *   - the host's transport drives the song: PLAY starts every track where it would be from the DAW's zero, the DAW's
 *     beat position drives Tessera's clock (CLK INT), a loop or a locate seeks, STOP stops; with no transport (the
 *     standalone app) it plays at once on its own clock;
 *   - a new instance starts from a new project, as an FM-1 with nothing saved does (the power-on sounds, every
 *     pattern empty, no generator, every slot empty): it plays nothing until it gets notes. The demos are in the DAW's
 *     preset browser and under OPEN; the standalone app starts empty too;
 *   - parameters (automatable): MASTER and PERF's MACRO 1..4 (ids 0..4, as since phase 2), then every project setting
 *     by its central ID (pid.h), each track's and the song's, with the FM-1's names and values; changed on the panel
 *     or by the core, they are sent back to the host;
 *   - the plugin's state is the project as a .tsp record, the same bytes SAVE > PROJECT writes on the FM-1; OPEN and
 *     SAVE under the keys open a .tsp or an FM-1 backup (its slots A..D and its music) and save a .tsp; MIDI
 *     writes the notes played since PLAY as a MIDI clip (midifile.h) to drag into the DAW;
 *   - the window (gui.h, Windows, Linux and macOS) shows the FM-1: its screen, buttons, keys and knobs. The audio thread owns
 *     the core: the window's input goes to it through a small queue, and it hands back a view ~30 times a second.
 * Several instances: each takes one of the plugin's private copies of the core (core_copies.h; TESS_PLUGIN_COPIES of
 * them, 8), so each Tessera is wholly its own; past that a new one is refused. A copy given back is used again by the
 * next instance, which starts from a new project unless the host loads its state. */
#include <clap/clap.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include <clap/ext/params.h>
#include <clap/ext/state.h>
#include <clap/ext/draft/undo.h>

#include "core_copies.h"
#include "gui.h"
#include "midifile.h"
#include "listen.h"
#include "presets.h"
#include "projfile.h"
#include "resample.h"

namespace
{
// the copies of the core: which an instance holds, and which have booted (their power-on done once)
std::mutex copies_lock;
bool copy_used[16], copy_booted[16];
uint8_t copy_setup[16][64];   // each copy's MIDI setup at its power-on (tess_core_setup_save), for the next instance
uint32_t copy_setup_n[16];
constexpr uint32_t MIDI_BARS = 16;   // (MIDI while stopped: the bars rendered from the top)

// the parameters: MASTER (0..1023) and MACRO 1..4 (0..100) first, with the ids they had (0..4); then the core's by
// central ID: id PARAM_PID + slot * 0x1000 + pid, slot 0..3 tracks 1..4, 4 the song's, 5..8 tracks 5..8 (added with
// the 8 tracks: the song kept its slot). Ids only ever grow in that order, so a DAW's automation keeps its parameter
// while the list grows
enum { PARAM_MASTER, PARAM_MACRO1, PARAM_LEGACY = PARAM_MACRO1 + 4 };
constexpr clap_id PARAM_PID = 0x10000;
constexpr uint32_t PARAM_SONG = 4, PARAM_SCAN = 48;   // (the song's slot; values compared per block, round the list)
constexpr uint32_t NTRACKS = 8;   // (the core's tracks: core_defs.txt CORE_TRACKS, checked at p_init)
constexpr uint32_t PARAM_SLOTS = NTRACKS + 1;   // (the tracks' slots and the song's)
constexpr uint32_t slot_track(uint32_t slot) { return slot < PARAM_SONG ? slot : slot - 1u; }   // (a track's slot)
constexpr uint32_t PID_MOD_SLOT = 0x31;   // (MOD 1 SOURCE, core.h P_M1SRC: then DESTINATION, AMOUNT and MOD 2..4)
struct Param
{
    clap_id id;
    uint32_t pid, track;   // (the first five: no pid)
    int32_t min, max, def;
    char name[64], module[64];
};

constexpr uint32_t NSTEMS = NTRACKS;
constexpr uint32_t NCH = 2 + NSTEMS;   // main L, R, then tracks 1..8 (the core's stems)

struct Tessera : GuiLink
{
    clap_plugin_t plugin{};
    const clap_host_t *host = nullptr;
    const tess_core_vt *core = nullptr;   // this instance's copy of the core
    uint32_t copy = 0;
    std::mutex core_lock;                 // the copy is not thread-safe: audio, state and the params' flush take it
    std::vector<Param> params;            // in id order (p_init)
    std::unique_ptr<std::atomic<int32_t>[]> param_now;   // each as the core has it (the audio thread writes)
    uint32_t param_scan = PARAM_LEGACY;   // the next of the core's to compare (params_out)
    int32_t master = 700;   // MASTER as set (0..1023: the core keeps it squared, so it reads back a step off)
    void master_set(int32_t v) { master = v < 0 ? 0 : v > 1023 ? 1023 : v; core->master((uint32_t)master); }
    std::unique_ptr<uint32_t[]> param_look;   // how each reads (param_look_of): the engine, an FX type or the scale
                                          // changes it ("ENGINE 1" as CUTOFF, FX 1 RATE as TIME); the audio thread
    std::atomic<bool> text_stale{false};  // a look changed: the host rescans its texts (on its main thread)
    // the window <-> the audio thread
    std::mutex input_lock, view_lock;
    PanelInput pending;                   // input_lock: buttons, keys and MASTER as now, the detents summed
    tess_view_t shared_view{};            // view_lock: the latest view
    std::unique_ptr<tess_tracks_t> shared_tracks{new tess_tracks_t()};   // view_lock: TRACKS, drawn while EXPANDED
    std::atomic<bool> tracks_on{false};   // the window is EXPANDED: the audio thread draws TRACKS with the view
    double rate = 48000.0;
    Resampler<NCH> rs;                    // the core's 44.1 kHz to the host's rate (resample.h)
    float buf[NCH][64];
    uint32_t buf_n = 0, buf_i = 0;
    uint32_t cur_frame = 0;               // the newest core frame the resampler has (tess_core_frames' count)
    bool stems = false;                   // a track output is active: the core makes the stems
    bool transport_known = false, was_playing = false, free_started = false;
    std::atomic<bool> notes_due{false};   // stopped processing: the notes MIDI OUT left on go off in the next block
    uint32_t view_due = 0;               // host frames until the next view
    std::vector<std::pair<uint32_t, uint32_t>> midi_wait;   // MIDI OUT packets (packet, core frame) not yet due
    midifile::Recorder clip;              // core_lock: the notes MIDI OUT sent since PLAY (or a jump back), for MIDI
    bool clip_playing = false;
    double clip_beat = 0;                 // the song's position at the last block
    std::unique_ptr<Gui> gui;

    void next_core_frame(float *f)
    {
        if (buf_i >= buf_n)
        {
            float *st[NSTEMS];
            for (uint32_t k = 0; k < NSTEMS; k++) st[k] = buf[2 + k];
            core->render(buf[0], buf[1], stems ? st : nullptr, 64);
            if (!stems)
                for (uint32_t c = 2; c < NCH; c++) std::memset(buf[c], 0, sizeof buf[c]);
            buf_n = 64;
            buf_i = 0;
        }
        for (uint32_t c = 0; c < NCH; c++)
            f[c] = buf[c][buf_i];
        cur_frame = core->frames() - buf_n + buf_i;
        buf_i++;
    }

    // n host frames into out[c] + at (null: a channel nobody listens to)
    void render(float *const *out, uint32_t at, uint32_t n)
    {
        for (uint32_t k = at; k < at + n; k++)
            rs.frame(out, k, [this](float *f) { next_core_frame(f); });
    }

    // MIDI OUT for a block of n host frames that ended with cur_frame: each packet at the host frame its core frame plays
    void midi_out(const clap_output_events_t *out, uint32_t n)
    {
        uint32_t pkt, frame;
        while (core->midi_out(&pkt, &frame))
            midi_wait.emplace_back(pkt, frame);
        // the clip: begun again when PLAY starts or the song jumps back (a loop, a locate); each note at its beat
        bool playing = core->playing() != 0;
        double pos = core->position(), bpm = core->bpm(), now = core->frames(), crate = core->rate();
        if (playing && (!clip_playing || pos < clip_beat - 0.5))
            clip.clear();
        clip_playing = playing;
        clip_beat = pos;
        size_t keep = 0;
        for (auto &m : midi_wait)
        {
            // host frames after this block's last one that core frame m.second plays at (the resampler plays
            // delay_core frames behind cur_frame, the newest it has)
            double ahead = (double)(int32_t)(m.second - (cur_frame - rs.delay_core())) / rs.step();
            if (ahead > 0.5)
            {
                midi_wait[keep++] = m;   // (not yet: the next block)
                continue;
            }
            if ((m.first >> 8 & 0xF0u) == 0xF0u)
                continue;   // (system messages, a clock: the DAW keeps time; only the tracks' notes go out)
            if (playing)
                clip.add(pos - (now - m.second) / crate * bpm / 60.0, (uint8_t)(m.first >> 8),
                         (uint8_t)(m.first >> 16 & 127), (uint8_t)(m.first >> 24 & 127));
            int64_t t = (int64_t)n - 1 + (int64_t)std::lround(ahead);
            if (t > (int64_t)n - 1) t = (int64_t)n - 1;
            if (out)
            {
                clap_event_midi_t e = {};
                e.header = {sizeof e, (uint32_t)(t < 0 ? 0 : t), CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0};
                e.port_index = 0;
                e.data[0] = (uint8_t)(m.first >> 8);
                e.data[1] = (uint8_t)(m.first >> 16 & 127);
                e.data[2] = (uint8_t)(m.first >> 24 & 127);
                out->try_push(out, &e.header);
            }
        }
        midi_wait.resize(keep);
    }

    void midi_in(const clap_event_header_t *h);
    // LISTEN (listen.h): the MIDI channel whose notes set the key and GENERATE (0 off) and SPLIT (the first key
    // note), the window's to pick, saved in the DAW's state ("TLSN"); the audio thread's copy and what it held, and
    // a piece for the main thread (the seed, the style + 1 above it: 0 the strip's)
    std::atomic<int> listen_ch{0}, listen_split{Listen::SPLIT};
    // OUTPUTS (MENU): the eight track ports mono (one channel each: the track before its pan) or stereo pairs (the same,
    // twice); fixed while the instance lives, from the setting when it was made, or the host's pick (audio-ports-config)
    bool mono_outs = projfile::setting_get("outputs") == "mono";
    bool outs_mono() override { return projfile::setting_get("outputs") == "mono"; }
    bool outs_live_mono() override { return mono_outs; }
    void outs_set(bool mono) override { projfile::setting_put("outputs", mono ? "mono" : "stereo"); }
    Listen listen;
    bool listen_params = false;           // a key set: every parameter to the host in this block
    std::atomic<uint64_t> listen_gen{0};
    bool listen_note(uint32_t ch, int key, int vel);
    int listen_channel() override { return listen_ch.load(); }
    void listen_pick(int ch) override;
    int listen_split_note() override { return listen_split.load(); }
    void listen_split_pick(int note) override;
    void listen_generate(uint32_t seed);
    void params_make();
    int param_index(clap_id id) const;
    int32_t param_core(int i);
    void param_set(int i, int32_t v);
    void params_refresh();
    void param_in(const clap_event_param_value_t *e);
    void params_out(const clap_output_events_t *out, uint32_t at, bool all = false);
    // the host's undo (DAWs record a parameter's change as one undo step between a gesture's begin and end): a knob
    // turned in the window or on an FM-1 opens a gesture for each parameter it moves (param_event), ended once the
    // knobs have been still GESTURE_MS (the list compared round in ~20 blocks: the changes are all seen by then)
    static constexpr double GESTURE_MS = 500;
    uint32_t gesture_left = 0;           // host frames until the open gestures end (0: none open, none to open)
    std::vector<uint8_t> gesture_open;   // per parameter: a gesture open
    std::vector<int> gesture_list;       // the ones open, in order
    void gesture(const clap_output_events_t *out, uint32_t at, int i, bool begin);
    void gestures_tick(const clap_output_events_t *out, uint32_t n);
    // a whole project changed by the window (OPEN, GENERATE, a preset): the host's undo told, if it has the CLAP undo
    // extension (it then keeps the state from before as the step)
    void host_undo(const char *what);
    void param_event(const clap_output_events_t *out, uint32_t at, int i, int32_t v);
    uint32_t param_look_of(int i);

    // gui.h: the window's side of the queue
    void view(tess_view_t &v) override
    {
        std::lock_guard<std::mutex> g(view_lock);
        v = shared_view;
    }
    void input(PanelInput &in) override
    {
        std::lock_guard<std::mutex> g(input_lock);
        pending.buttons = in.buttons;
        pending.keys = in.keys;
        pending.key_words = in.key_words;
        pending.away = pending.away || in.away;
        in.away = false;
        if (in.hover_changed)
        {
            pending.hover = in.hover;
            pending.hover_changed = true;
            in.hover_changed = false;
        }
        if (in.master >= 0)
            pending.master = in.master;
        in.master = -1;
        if (in.track >= 0)
            pending.track = in.track;
        in.track = -1;
        for (int r = 0; r < TESS_NE; r++)
        {
            pending.enc[r] += in.enc[r];
            in.enc[r] = 0;
        }
        pending.zones.insert(pending.zones.end(), in.zones.begin(), in.zones.end());
        in.zones.clear();
        pending.tzones.insert(pending.tzones.end(), in.tzones.begin(), in.tzones.end());
        in.tzones.clear();
        pending.text.insert(pending.text.end(), in.text.begin(), in.text.end());
        in.text.clear();
        pending.typed.insert(pending.typed.end(), in.typed.begin(), in.typed.end());
        in.typed.clear();
        pending.changed = true;
        in.changed = false;
    }

    // gui.h: the project files (the host's main thread)
    bool standalone() override { return free_started && !transport_known; }
    std::string open_file(const std::string &path) override;
    std::string save_file(const std::string &path) override;
    std::string file_name() override;
    std::string export_midi() override;
    std::string midi_drag_file(std::string &path) override;
    std::string midi_clip(std::vector<uint8_t> &f, std::string &file);
    void changed_by_file(const char *what = "OPEN PROJECT");   // (what: the host's undo step's name)
    // the shell (gui.h): the last one picked in any window, from the plugin's settings
    int shell_i = 0;
    int shell() override { return shell_i; }
    void shell_picked(int i) override;
    // the inks (gui.h): MENU > COLOR's choices; the last one shown in any window, from the plugin's settings
    std::vector<Panel::Ink> inks() override;
    void ink_picked(int i) override;
    void ink_seen(int i) override;
    int tracks() override { return (int)core->tracks(); }
    // SIZE (gui.h): the one picked or dragged to in any window (the setting `size`, in %), else the host's scale (a
    // HiDPI screen: set_scale) no larger than the screen has room for (size_room), each held to the mode's range
    // (Panel::size_fit)
    int size_user = 0, size_host = 100;   // %; size_user 0: none picked
    int size_pct() override
    {
        int pct = size_user ? size_user : size_host;
        if (!size_user && size_room > 0 && pct > size_room * 100 / win_w()) pct = size_room * 100 / win_w();
        return Panel::size_fit(pct, win_expanded());
    }
    int size_room = 0;                  // the screen's width less EXPANDED_ROOM (Gui::screen_width; 0 unknown)
    void size_pick(int pct) override;
    bool size_set(int pct);             // the window at that size (the host's set_size, a pick)
    // MODE (gui.h): EXPANDED picked in any window (the setting `mode`); the window is EXPANDED in TESSERA's shell
    std::atomic<bool> mode_expanded{false};
    bool expanded() override { return mode_expanded.load(); }
    bool win_expanded() const { return mode_expanded.load() && Panel::shell_native(shell_i); }
    int win_w() const { return Panel::width_of(win_expanded()); }
    void mode_pick(bool on) override;
    // no MODE picked yet: EXPANDED if a screen of width w (Gui::screen_width; 0 unknown: FM-1) has room for its window
    // at the SIZE it would have, with EXPANDED_ROOM to spare for the DAW's frame; not remembered (a pick is)
    static constexpr int EXPANDED_ROOM = 100;
    void mode_default(int w)
    {
        int pct = Panel::size_fit(size_user ? size_user : size_host, true);
        mode_expanded.store(w > 0 && w >= Panel::W_EXPANDED * pct / 100 + EXPANDED_ROOM);
        tracks_on.store(win_expanded());
    }
    void resize_window();               // the window and the host to the mode's width at SIZE
    bool tracks_view(tess_tracks_t &t) override
    {
        std::lock_guard<std::mutex> g(view_lock);
        if (!shared_tracks->w) return false;
        t = *shared_tracks;
        return true;
    }
    std::atomic<uint32_t> midi_seen{0};   // MIDI in so far (the window's MIDI lamp)
    uint32_t midi_count() override { return midi_seen.load(std::memory_order_relaxed); }
    // an FM-1 on USB plays this Tessera (fm1_link.h): asked for from the first activate when the setting is on
    Fm1Link::State fm1_state() override { return Fm1Link::get().state(this); }
    void fm1_set(bool on) override;
    bool fm1_started = false;
    // the audio thread: held in the window and on the FM-1, apart (the core gets the two together)
    uint32_t win_buttons = 0, win_keys = 0, fm1_buttons = 0, fm1_keys = 0;
    // GENERATE (gui.h; the host's main thread): the style and VARIATION picked (the plugin's settings, for every
    // window), the pieces made (style, VARIATION, seed: each made again exactly), the one now, before / after
    struct Piece { int style, variation; uint32_t seed; };
    int gen_style = 0, gen_variation = 100;
    std::vector<Piece> gen_made;
    int gen_at = -1;
    bool gen_before = false;
    std::vector<std::string> gen_styles() override;
    std::string generate(int style, int variation) override;
    std::string gen_swap() override;
    std::string track_do(int what, int track) override;
    std::string gen_recall(int dir) override;
    void gen_pick(int style, int variation) override;
    Panel::GenState gen_state() override;
    std::string gen_make(const Piece &pc);
    // A / B (Cosmati audit G7): two whole projects, and each one's settings (every settable parameter, each track's) for
    // MORPH; the one shown (-1 none, or MORPH's blend), MORPH's place (-1 not used since). Saved in the DAW's state (TSNP)
    struct SnapPV { uint16_t pid; uint8_t trk; int16_t v; };
    struct Snap { std::vector<uint8_t> tsp; std::vector<SnapPV> pv; };
    Snap snaps[2];
    int snap_active = -1, snap_morph = -1;
    std::mutex snap_lock;
    Panel::SnapState snap_state() override;
    std::string snap_do(int what, int slot, int value) override;
    void snap_take(Snap &s);   // (core_lock held)
    std::vector<uint8_t> snaps_state();
    bool snaps_load(const uint8_t *b, uint32_t len);
    std::string gen_seed(uint32_t seed, int style = -1);
};

Tessera *self(const clap_plugin_t *p) { return static_cast<Tessera *>(p->plugin_data); }

int strcasecmp_ascii(const char *a, const char *b)
{
    for (;; a++, b++)
    {
        int x = *a >= 'a' && *a <= 'z' ? *a - 32 : *a, y = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
        if (x != y || !x) return x - y;
    }
}

// ---- the project files (OPEN / SAVE under the keys)

// the music as it is now, as a .tsp record
static std::vector<uint8_t> record_of(Tessera *t)
{
    std::vector<uint8_t> b(t->core->tsp_max());
    std::lock_guard<std::mutex> g(t->core_lock);
    b.resize(t->core->save(b.data(), (uint32_t)b.size()));
    return b;
}

std::string Tessera::open_file(const std::string &path)
{
    std::vector<uint8_t> bytes;
    if (!projfile::read_file(path, bytes, 4u << 20))
        return "CANNOT READ THAT FILE";
    std::string name, err;
    if (bytes.size() >= 4 && !std::memcmp(bytes.data(), "TSP1", 4))   // a project
    {
        if (!projfile::tsp_name(bytes, name))
            return "A DAMAGED PROJECT FILE";
        uint32_t r;
        {
            std::lock_guard<std::mutex> g(core_lock);
            r = core->load(bytes.data(), (uint32_t)bytes.size());
        }
        if (r)
            return "A PROJECT THIS VERSION CANNOT READ";
        gen_before = false;
        changed_by_file();
        return "OPENED " + (name.empty() ? std::string("THE PROJECT") : name);
    }
    projfile::Backup bk;   // an FM-1 backup: its slots A..D, then the music it was playing
    if (!projfile::backup_read(std::string(bytes.begin(), bytes.end()), bk, err))
        return "NOT A PROJECT: " + err;
    uint32_t slots = 0, r;
    {
        std::lock_guard<std::mutex> g(core_lock);
        for (uint32_t k = 0; k < 4; k++)
            if (!bk.slot[k].empty() && core->slot_put(k, bk.slot[k].data(), (uint32_t)bk.slot[k].size()) == 0)
                slots++;
        r = core->load(bk.music.data(), (uint32_t)bk.music.size());
    }
    projfile::tsp_name(bk.music, name);
    changed_by_file();
    if (r)
        return "BACKUP: " + std::to_string(slots) + " SLOTS; ITS MUSIC THIS VERSION CANNOT READ";
    return "BACKUP: " + (name.empty() ? std::string("ITS MUSIC") : name) + ", " + std::to_string(slots) + " SLOTS";
}

std::string Tessera::save_file(const std::string &path)
{
    std::vector<uint8_t> b = record_of(this);
    std::string name;
    if (b.empty() || !projfile::tsp_name(b, name))
        return "COULD NOT SAVE THE PROJECT";
    if (!projfile::write_file(path, b))
        return "CANNOT WRITE THAT FILE";
    return "SAVED " + (name.empty() ? std::string("THE PROJECT") : name);
}

// the project (a .tsp record) played from the top for bars bars on a spare copy of the core (midifile::render), as
// fast as it renders. The instance's own copy plays on untouched. "" done, else why not
std::string render_bars(const std::vector<uint8_t> &tsp, uint32_t bars, std::vector<midifile::Ev> &ev, double &bpm)
{
    uint32_t copy = 0;
    bool again;
    {
        std::lock_guard<std::mutex> c(copies_lock);   // a copy no instance holds, held while it plays
        while (copy < tess_copy_count() && copy_used[copy])
            copy++;
        if (copy >= tess_copy_count())
            return "NO SPARE CORE: CLOSE AN INSTANCE";
        copy_used[copy] = true;
        again = copy_booted[copy];
        copy_booted[copy] = true;
    }
    const tess_core_vt *c = tess_copy(copy);
    c->boot();
    if (again)   // (as p_init: a new project and the power-on MIDI setup, NOTES OUT ALL)
    {
        c->project_new();
        c->setup_load(copy_setup[copy], copy_setup_n[copy]);
    }
    else
        copy_setup_n[copy] = c->setup_save(copy_setup[copy], sizeof copy_setup[0]);
    std::string err = midifile::render(c, tsp, bars, ev, bpm);
    std::lock_guard<std::mutex> g(copies_lock);
    copy_used[copy] = false;
    return err;
}

// MIDI under the keys: a clip as a .mid, named after the project and the time. Playing: the notes played since PLAY;
// stopped: the project rendered from the top for MIDI_BARS bars (render_bars). The clip's bytes and file name, or what
// to show instead
std::string Tessera::midi_clip(std::vector<uint8_t> &f, std::string &file)
{
    std::string name;
    std::vector<uint8_t> tsp = record_of(this);
    projfile::tsp_name(tsp, name);
    std::vector<midifile::Ev> ev;
    double bpm;
    bool playing;
    {
        std::lock_guard<std::mutex> g(core_lock);
        playing = core->playing() != 0;
        if (playing)
            ev = clip.take();
        bpm = core->bpm();
    }
    uint32_t bars = 0;
    if (!playing)
    {
        bars = MIDI_BARS;
        std::string err = render_bars(tsp, bars, ev, bpm);
        if (!err.empty())
            return err;
    }
    f = midifile::smf(ev, bpm, name, bars);
    if (f.empty())
        return playing ? "NO NOTES YET: PLAY, THEN MIDI" : "NO NOTES IN THE FIRST BARS";
    char stamp[32];
    std::time_t tt = std::time(nullptr);
    std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H%M%S", std::localtime(&tt));
    file = midifile::file_name(name, stamp);
    return "";
}

// MIDI clicked: the clip in Documents/Tessera/MIDI
std::string Tessera::export_midi()
{
    std::vector<uint8_t> f;
    std::string file, err = midi_clip(f, file);
    if (!err.empty())
        return err;
    std::string dir = midifile::folder();
    if (dir.empty())
        return "NO DOCUMENTS FOLDER FOR THE CLIP";
    if (!projfile::write_file((std::filesystem::u8path(dir) / std::filesystem::u8path(file)).u8string(), f))
        return "CANNOT WRITE THE CLIP";
    return "CLIP SAVED IN DOCUMENTS/TESSERA/MIDI";
}

// MIDI dragged off: the clip in the system's temporary folder (Tessera/), for the window's drag into a DAW (which
// copies it); its path, or what to show instead
std::string Tessera::midi_drag_file(std::string &path)
{
    path.clear();
    std::vector<uint8_t> f;
    std::string file, err = midi_clip(f, file);
    if (!err.empty())
        return err;
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::temp_directory_path(ec) / "Tessera";
    if (ec || (std::filesystem::create_directories(dir, ec), ec))
        return "NO TEMPORARY FOLDER FOR THE CLIP";
    std::string p = (dir / std::filesystem::u8path(file)).u8string();
    if (!projfile::write_file(p, f))
        return "CANNOT WRITE THE CLIP";
    path = p;
    return "";
}

std::string Tessera::file_name()
{
    std::string name;
    projfile::tsp_name(record_of(this), name);
    return projfile::tsp_file_name(name);
}

// ---- GENERATE (TESSERA's strip): a whole piece from a style and a seed (firmware/src/generate.c)

static const size_t GEN_KEEP = 16;   // the pieces the seed's wheel goes back through

std::vector<std::string> Tessera::gen_styles()
{
    std::vector<std::string> names;
    for (uint32_t i = 0, n = core->gen_styles(); i < n; i++)
        names.emplace_back(core->gen_style(i));
    return names;
}

void Tessera::gen_pick(int style, int variation)
{
    int n = (int)core->gen_styles();
    if (style < 0 || style >= n) style = 0;
    variation = variation < 0 ? 0 : variation > 100 ? 100 : variation;
    if (style == gen_style && variation == gen_variation)
        return;
    gen_style = style;
    gen_variation = variation;
    projfile::setting_put("gen_style", core->gen_style((uint32_t)style));
    projfile::setting_put("gen_variation", std::to_string(variation));
}

// a piece made (a new one or one made before): the tempo the DAW's while a DAW drives (its transport), the style's in
// the standalone app; the DAW's state and parameters are out of date after
std::string Tessera::gen_make(const Piece &pc)
{
    uint32_t r;
    {
        std::lock_guard<std::mutex> g(core_lock);
        r = core->generate((uint32_t)pc.style, pc.seed, (uint32_t)pc.variation, transport_known ? TESS_GEN_KEEP_TEMPO : 0u);
    }
    if (r)
        return "COULD NOT GENERATE";
    gen_before = false;
    changed_by_file("GENERATE");
    return "";
}

std::string Tessera::generate(int style, int variation)
{
    gen_pick(style, variation);
    static std::mt19937 rng{std::random_device{}() ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    uint32_t seed = 0;
    while (!seed) seed = rng();
    return gen_seed(seed);
}

// a piece of the style (-1, or one past the last: the strip's) and the strip's VARIATION from seed, kept in the
// seed's wheel (GENERATE, LISTEN); the strip shows its style after (not kept as the window's setting, as the wheel)
std::string Tessera::gen_seed(uint32_t seed, int style)
{
    if (style < 0 || style >= (int)core->gen_styles())
        style = gen_style;
    Piece pc{style, gen_variation, seed};
    std::string said = gen_make(pc);
    if (!said.empty())
        return said;
    gen_style = style;
    gen_made.push_back(pc);   // (made after going back: the newer ones stay, this one after them)
    if (gen_made.size() > GEN_KEEP) gen_made.erase(gen_made.begin());
    gen_at = (int)gen_made.size() - 1;
    return gen_made.size() == 1 ? "HOLD GENERATE FOR BEFORE" : "";   // (the strip shows the style and the seed)
}

// TRACK's copy: one for every Cosmati in this process (a track copied in one pastes into another; the copies are the
// same build's, so each reads the others')
static std::mutex track_clip_lock;
static std::vector<uint8_t> track_clip;
static int track_clip_from = -1;

// TRACK (panel.h): a track copied, a copy pasted onto it (whole or its sound), or the track moved a place (swapped
// with its neighbour); the DAW's state and parameters are out of date after a paste or a move (one undo step)
std::string Tessera::track_do(int what, int track)
{
    char said[64];
    const char *undo = nullptr;
    {
        std::lock_guard<std::mutex> g(core_lock);
        int n = (int)core->tracks();
        if (track < 0 || track >= n)
            return "";
        if (what == Panel::TR_COPY)
        {
            std::vector<uint8_t> b(core->tsp_max());
            uint32_t len = core->track_copy((uint32_t)track, b.data(), (uint32_t)b.size());
            if (!len)
                return "COULD NOT COPY THE TRACK";
            b.resize(len);
            std::lock_guard<std::mutex> c(track_clip_lock);
            track_clip.swap(b);
            track_clip_from = track;
            std::snprintf(said, sizeof said, "T%d COPIED", track + 1);
            return said;
        }
        if (what == Panel::TR_PASTE || what == Panel::TR_PASTE_SOUND)
        {
            bool sound = what == Panel::TR_PASTE_SOUND;
            std::lock_guard<std::mutex> c(track_clip_lock);
            if (track_clip.empty())
                return "NOTHING COPIED YET (CTRL+C)";
            if (core->track_paste((uint32_t)track, sound ? TESS_PASTE_SOUND : TESS_PASTE_TRACK, track_clip.data(),
                                  (uint32_t)track_clip.size()))
                return "COULD NOT PASTE";
            std::snprintf(said, sizeof said, sound ? "T%d'S SOUND ON T%d" : "T%d PASTED ONTO T%d", track_clip_from + 1,
                          track + 1);
            undo = sound ? "PASTE SOUND" : "PASTE TRACK";
        }
        else if (what == Panel::TR_UP || what == Panel::TR_DOWN)
        {
            int to = track + (what == Panel::TR_UP ? -1 : 1);
            if (to < 0 || to >= n)
                return "";
            if (core->track_swap((uint32_t)track, (uint32_t)to))
                return "COULD NOT MOVE THE TRACK";
            std::snprintf(said, sizeof said, "T%d NOW T%d", track + 1, to + 1);
            undo = "MOVE TRACK";
        }
        else
            return "";
    }
    changed_by_file(undo);
    return said;
}

std::string Tessera::gen_swap()
{
    uint32_t r;
    {
        std::lock_guard<std::mutex> g(core_lock);
        r = core->gen_swap();
    }
    if (r)
        return "NOTHING GENERATED YET";
    gen_before = !gen_before;
    changed_by_file(gen_before ? "GENERATE: BEFORE" : "GENERATE: AFTER");
    return gen_before ? "BEFORE THE PIECE" : "THE PIECE AGAIN";
}

std::string Tessera::gen_recall(int dir)
{
    int to = gen_at + dir;
    if (gen_made.empty() || to < 0 || to >= (int)gen_made.size())
        return gen_made.empty() ? "NOTHING GENERATED YET" : dir < 0 ? "THE FIRST PIECE MADE" : "THE LAST PIECE MADE";
    std::string said = gen_make(gen_made[to]);
    if (!said.empty())
        return said;
    gen_at = to;
    gen_style = gen_made[to].style;
    gen_variation = gen_made[to].variation;
    return "PIECE " + std::to_string(to + 1) + " OF " + std::to_string(gen_made.size());
}

Panel::GenState Tessera::gen_state()
{
    Panel::GenState s;
    s.style = gen_style;
    s.variation = gen_variation;
    s.at = gen_at;
    s.made = (int)gen_made.size();
    s.seed = gen_at >= 0 ? gen_made[gen_at].seed : 0u;
    s.before = gen_before;
    return s;
}

// a project came in from a file: the DAW's saved state and the MACROs it shows are out of date
void Tessera::host_undo(const char *what)
{
    auto *hu = static_cast<const clap_host_undo_t *>(host->get_extension(host, CLAP_EXT_UNDO));
    if (hu && hu->change_made)
        hu->change_made(host, what, nullptr, 0, false);   // (no delta: the host keeps the state from before)
}

// ---- A / B
void Tessera::snap_take(Snap &sn)
{
    sn.tsp.assign(core->tsp_max(), 0);
    sn.tsp.resize(core->save(sn.tsp.data(), (uint32_t)sn.tsp.size()));
    sn.pv.clear();
    uint32_t nt = core->tracks();
    for (uint32_t n = 0, cnt = core->param_count(); n < cnt; n++)
    {
        tess_param_t in;
        uint32_t pid = core->param_at(n, 0, &in);
        if (pid == 0xFFFF || !in.settable)
            continue;
        for (uint32_t tr = 0; tr < (in.per_track ? nt : 1u); tr++)
            sn.pv.push_back({(uint16_t)pid, (uint8_t)tr, (int16_t)core->param_get(pid, tr)});
    }
}

Panel::SnapState Tessera::snap_state()
{
    std::lock_guard<std::mutex> g(snap_lock);
    Panel::SnapState st;
    st.has[0] = !snaps[0].tsp.empty(), st.has[1] = !snaps[1].tsp.empty();
    st.active = snap_active, st.morph = snap_morph;
    return st;
}

std::string Tessera::snap_do(int what, int slot, int value)
{
    const char *nm = slot ? "B" : "A";
    if (slot < 0 || slot > 1)
        return "";
    std::lock_guard<std::mutex> s(snap_lock);
    if (what == Panel::SNAP_STORE)
    {
        {
            std::lock_guard<std::mutex> g(core_lock);
            snap_take(snaps[slot]);
        }
        snap_active = slot, snap_morph = -1;
        return std::string("STORED IN ") + nm;
    }
    if (what == Panel::SNAP_CLICK)
    {
        std::string said;
        {
            std::lock_guard<std::mutex> g(core_lock);
            if (snap_active >= 0 && snap_active != slot)   // (the one shown keeps its edits)
                snap_take(snaps[snap_active]);
            if (snaps[slot].tsp.empty())
            {
                snap_take(snaps[slot]);   // (an empty one: the project as it is, A and B the same until edited)
                const char *from = snap_active == 0 ? "A" : snap_active == 1 ? "B" : "THE PROJECT";
                snap_active = slot, snap_morph = -1;
                return std::string(nm) + ": A COPY OF " + from;
            }
            if (core->load(snaps[slot].tsp.data(), (uint32_t)snaps[slot].tsp.size()))
                return std::string("COULD NOT RECALL ") + nm;
        }
        snap_active = slot, snap_morph = -1;
        gen_before = false;
        changed_by_file(slot ? "Recall B" : "Recall A");
        return std::string(nm) + " RECALLED";
    }
    if (what == Panel::SNAP_MORPH)
    {
        const Snap &a = snaps[0], &b = snaps[1];
        if (a.pv.empty() || b.pv.empty())
            return "MORPH: STORE A AND B FIRST";
        int x = value < 0 ? 0 : value > 100 ? 100 : value;
        {
            std::lock_guard<std::mutex> g(core_lock);
            for (size_t i = 0, j = 0; i < a.pv.size(); i++)   // (the same order; one missing from B: passed over)
            {
                while (j < b.pv.size() && (b.pv[j].pid < a.pv[i].pid || (b.pv[j].pid == a.pv[i].pid && b.pv[j].trk < a.pv[i].trk)))
                    j++;
                if (j >= b.pv.size() || b.pv[j].pid != a.pv[i].pid || b.pv[j].trk != a.pv[i].trk)
                    continue;
                int32_t va = a.pv[i].v, vb = b.pv[j].v, v = va;
                if (va == vb)
                    continue;
                char t[32] = {};   // (a choice shown as a word, SAW or an engine: no in-between, B's from halfway)
                core->param_text(a.pv[i].pid, a.pv[i].trk, va, t, sizeof t);
                bool number = (t[0] >= '0' && t[0] <= '9') || ((t[0] == '-' || t[0] == '+' || t[0] == '.') && t[1] >= '0' && t[1] <= '9');
                v = number ? va + (int32_t)std::lround((vb - va) * x / 100.0) : x >= 50 ? vb : va;
                if (v != core->param_get(a.pv[i].pid, a.pv[i].trk))
                    core->param_set(a.pv[i].pid, a.pv[i].trk, v);
            }
            params_refresh();
        }
        snap_active = -1, snap_morph = x;
        auto *hp = static_cast<const clap_host_params_t *>(host->get_extension(host, CLAP_EXT_PARAMS));
        if (hp && hp->rescan)
            hp->rescan(host, CLAP_PARAM_RESCAN_VALUES);
        return "";
    }
    return "";
}

// the state's part: A's and B's projects and settings, the one shown, MORPH's place (empty: none stored)
std::vector<uint8_t> Tessera::snaps_state()
{
    std::lock_guard<std::mutex> s(snap_lock);
    std::vector<uint8_t> o;
    if (snaps[0].tsp.empty() && snaps[1].tsp.empty())
        return o;
    auto u32 = [&](uint32_t v) { for (int k = 0; k < 4; k++) o.push_back((uint8_t)(v >> (8 * k))); };
    for (const Snap &sn : snaps)
    {
        u32((uint32_t)sn.tsp.size());
        o.insert(o.end(), sn.tsp.begin(), sn.tsp.end());
        u32((uint32_t)sn.pv.size());
        for (const SnapPV &e : sn.pv)
            o.insert(o.end(), {(uint8_t)e.pid, (uint8_t)(e.pid >> 8), e.trk, (uint8_t)e.v, (uint8_t)((uint16_t)e.v >> 8)});
    }
    o.push_back((uint8_t)(int8_t)snap_active);
    o.push_back((uint8_t)(int8_t)snap_morph);
    return o;
}
bool Tessera::snaps_load(const uint8_t *b, uint32_t len)
{
    std::lock_guard<std::mutex> s(snap_lock);
    Snap got[2];
    uint32_t at = 0;
    auto u32 = [&](uint32_t &v) {
        if (at + 4 > len) return false;
        v = b[at] | (uint32_t)b[at + 1] << 8 | (uint32_t)b[at + 2] << 16 | (uint32_t)b[at + 3] << 24;
        at += 4;
        return true;
    };
    for (Snap &sn : got)
    {
        uint32_t n;
        if (!u32(n) || n > len - at)
            return false;
        sn.tsp.assign(b + at, b + at + n);
        at += n;
        if (!u32(n) || (uint64_t)n * 5 > len - at)
            return false;
        for (uint32_t i = 0; i < n; i++, at += 5)
            sn.pv.push_back({(uint16_t)(b[at] | b[at + 1] << 8), b[at + 2], (int16_t)(uint16_t)(b[at + 3] | b[at + 4] << 8)});
    }
    if (at + 2 != len)
        return false;
    snaps[0] = std::move(got[0]), snaps[1] = std::move(got[1]);
    snap_active = (int8_t)b[at], snap_morph = (int8_t)b[at + 1];
    return true;
}

void Tessera::changed_by_file(const char *what)
{
    {
        std::lock_guard<std::mutex> g(core_lock);
        params_refresh();
    }
    auto *hs = static_cast<const clap_host_state_t *>(host->get_extension(host, CLAP_EXT_STATE));
    if (hs && hs->mark_dirty)
        hs->mark_dirty(host);
    auto *hp = static_cast<const clap_host_params_t *>(host->get_extension(host, CLAP_EXT_PARAMS));
    if (hp && hp->rescan)
        hp->rescan(host, CLAP_PARAM_RESCAN_VALUES);   // (the texts: params_refresh asked for them)
    host_undo(what);
}

// LISTEN picked in the menu (0 off, 1..16): this instance's, saved in the DAW's state
void Tessera::listen_pick(int ch)
{
    if (ch < 0 || ch > 16 || ch == listen_ch.load())
        return;
    listen_ch.store(ch);
    auto *hs = static_cast<const clap_host_state_t *>(host->get_extension(host, CLAP_EXT_STATE));
    if (hs && hs->mark_dirty)
        hs->mark_dirty(host);
}

// LISTEN's SPLIT picked in the menu (a C, 12..108): this instance's, saved in the DAW's state
void Tessera::listen_split_pick(int note)
{
    if (!Listen::split_ok(note) || note == listen_split.load())
        return;
    listen_split.store(note);
    auto *hs = static_cast<const clap_host_state_t *>(host->get_extension(host, CLAP_EXT_STATE));
    if (hs && hs->mark_dirty)
        hs->mark_dirty(host);
}

// the window at the mode's width and SIZE, and the host asked to make room
void Tessera::resize_window()
{
    int pct = size_pct();
    tracks_on.store(win_expanded());
    if (gui)
        gui->set_scale(pct);
    auto *hg = static_cast<const clap_host_gui_t *>(host->get_extension(host, CLAP_EXT_GUI));
    if (hg && hg->request_resize)
        hg->request_resize(host, (uint32_t)(win_w() * pct / 100), (uint32_t)(Panel::H * pct / 100));
}

// SIZE picked in the menu: remembered for every window, the window that size
void Tessera::size_pick(int pct)
{
    size_user = Panel::size_fit(pct, win_expanded());
    projfile::setting_put("size", std::to_string(size_user));
    resize_window();
}

// MODE picked in the menu: remembered for every window (EXPANDED: a SIZE over 150 shows at 150), the window that wide
void Tessera::mode_pick(bool on)
{
    if (on == mode_expanded.load())
        return;
    mode_expanded.store(on);
    projfile::setting_put("mode", on ? "expanded" : "fm1");
    resize_window();
}

bool Tessera::size_set(int pct)
{
    if (pct != Panel::size_fit(pct, win_expanded()))   // (outside the mode's range)
        return false;
    if (pct != size_pct())   // (the host's own resize, a drag on its frame: kept as the pick)
    {
        size_user = pct;
        projfile::setting_put("size", std::to_string(pct));
    }
    if (gui)
        gui->set_scale(pct);
    return true;
}

// a shell picked under the keys: its ink on this Tessera's screen (as MENU > COLOR; after, that still changes it), and
// the shell the next window opens with, in any instance
void Tessera::shell_picked(int i)
{
    if (i < 0 || i >= Panel::shell_count())
        return;
    bool wide = win_expanded(), skin = Panel::shell_native(shell_i) && Panel::shell_native(i);
    shell_i = i;
    if (!skin)   // (NIGHT <-> DAY: the ink stays as it is)
    {
        std::lock_guard<std::mutex> g(core_lock);
        core->ink(Panel::shell_ink(i));
    }
    projfile::setting_put("shell", Panel::shell_id(i));
    if (wide != win_expanded())   // (EXPANDED is TESSERA's shell's: an FM-1 shell is FM-1 mode, and back)
        resize_window();
}

std::vector<Panel::Ink> Tessera::inks()
{
    std::vector<Panel::Ink> list;
    uint32_t n = core->ink_at(0, nullptr, nullptr);
    for (uint32_t i = 0; i < n; i++)
    {
        const char *name = "";
        Panel::Ink k{};
        core->ink_at(i, &name, k.rgb);
        k.name = name;
        list.push_back(k);
    }
    return list;
}

// an ink picked under the buttons: on this Tessera's screen, as MENU > COLOR would (ink_seen remembers it)
void Tessera::ink_picked(int i)
{
    const char *name = nullptr;
    if (i < 0 || (uint32_t)i >= core->ink_at((uint32_t)i, &name, nullptr) || !name)
        return;
    std::lock_guard<std::mutex> g(core_lock);
    core->ink(name);
}

// the screen's ink changed, however: the ink the next window opens with, in any instance (over its shell's ink until
// a shell is picked again)
void Tessera::ink_seen(int i)
{
    const char *name = nullptr;
    if (i >= 0 && (uint32_t)i < core->ink_at((uint32_t)i, &name, nullptr) && name)
        projfile::setting_put("ink", name);
}

// FM-1 USB clicked: on takes the FM-1 for this Tessera; off is off for all; either is the setting a new one opens with
void Tessera::fm1_set(bool on)
{
    if (on)
        Fm1Link::get().want(this, true, true);
    else
        Fm1Link::get().off_all();
    projfile::setting_put("fm1_usb", on ? "1" : "0");
}

void Tessera::midi_in(const clap_event_header_t *h)
{
    if (h->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;
    if (h->type == CLAP_EVENT_NOTE_ON || h->type == CLAP_EVENT_NOTE_OFF || h->type == CLAP_EVENT_NOTE_CHOKE ||
        h->type == CLAP_EVENT_MIDI)
        midi_seen.fetch_add(1, std::memory_order_relaxed);
    switch (h->type)
    {
    case CLAP_EVENT_NOTE_ON:
    case CLAP_EVENT_NOTE_OFF:
    case CLAP_EVENT_NOTE_CHOKE:
    {
        auto *e = reinterpret_cast<const clap_event_note_t *>(h);
        if (h->type != CLAP_EVENT_NOTE_ON && e->key < 0)   // (a wildcard: every key, on its channel or on all 16)
        {
            uint32_t cc = h->type == CLAP_EVENT_NOTE_CHOKE ? 120u : 123u;   // (All Sound Off / All Notes Off)
            for (uint32_t c = 0; c < 16; c++)
                if (e->channel < 0 || (uint32_t)e->channel == c)
                    core->midi(0xB0u | c, cc, 0);
            return;
        }
        if (e->key < 0 || e->key > 127)
            return;
        uint32_t ch = e->channel < 0 ? 0u : (uint32_t)e->channel & 15u;
        uint32_t vel = (uint32_t)(e->velocity * 127.0 + 0.5);
        if (listen_note(ch, e->key, h->type == CLAP_EVENT_NOTE_ON ? (vel ? (int)vel : 1) : 0))
            break;
        if (h->type == CLAP_EVENT_NOTE_ON)
            core->midi(0x90u | ch, (uint32_t)e->key, vel ? vel : 1u);
        else
            core->midi(0x80u | ch, (uint32_t)e->key, vel);
        break;
    }
    case CLAP_EVENT_MIDI:
    {
        auto *e = reinterpret_cast<const clap_event_midi_t *>(h);
        uint8_t kind = e->data[0] & 0xF0;
        if ((kind == 0x90 || kind == 0x80) &&
            listen_note(e->data[0] & 15u, e->data[1] & 127, kind == 0x90 ? e->data[2] & 127 : 0))
            break;
        core->midi(e->data[0], e->data[1], e->data[2]);
        break;
    }
    case CLAP_EVENT_NOTE_EXPRESSION:   // (per note: PRESSURE, BRIGHTNESS (MPE's slide), TUNING; the rest not yet)
    {
        auto *e = reinterpret_cast<const clap_event_note_expression_t *>(h);
        if (e->expression_id == CLAP_NOTE_EXPRESSION_TUNING)   // (semitones, -120..120)
        {
            if (e->key > 127 || e->channel > 15)
                break;
            double c = e->value * 100.0;
            c = c < -12000.0 ? -12000.0 : c > 12000.0 ? 12000.0 : c;
            core->note_tune(e->channel < 0 ? -1 : e->channel, e->key < 0 ? -1 : e->key, (int32_t)(c < 0 ? c - 0.5 : c + 0.5));
            break;
        }
        uint32_t kind = e->expression_id == CLAP_NOTE_EXPRESSION_PRESSURE     ? TESS_EXPR_PRESSURE
                        : e->expression_id == CLAP_NOTE_EXPRESSION_BRIGHTNESS ? TESS_EXPR_SLIDE
                                                                               : 99u;
        if (kind == 99u || e->key > 127 || e->channel > 15 || (e->channel >= 0 && listen.mine(e->channel)))
            break;   // (LISTEN's channel steers: its notes play nothing to shape)
        double v = e->value < 0 ? 0 : e->value > 1 ? 1 : e->value;
        core->note_expr(kind, e->channel < 0 ? -1 : e->channel, e->key < 0 ? -1 : e->key, (uint32_t)(v * 127.0 + 0.5));
        break;
    }
    default:
        break;
    }
}

// LISTEN (the audio thread, the core locked): a note on the LISTEN channel (vel 0: off) steers instead of playing;
// true if it was one
bool Tessera::listen_note(uint32_t ch, int key, int vel)
{
    int lc = listen_ch.load(std::memory_order_relaxed), ls = listen_split.load(std::memory_order_relaxed);
    if (lc != listen.channel || ls != listen.split)
        listen.channel = lc, listen.split = ls, listen.clear();
    if (!listen.mine((int)ch))
        return false;
    Listen::Ask a = listen.note(key, vel);
    if (a.what == Listen::KEY && core->key((uint32_t)a.root, a.minor))
        listen_params = true;
    else if (a.what == Listen::GENERATE)
    {
        listen_gen.store(a.seed | (uint64_t)(a.style + 1) << 32);   // (GENERATE takes a while: the main thread)
        if (host->request_callback)
            host->request_callback(host);   // (p_on_main_thread)
    }
    return true;
}

// ---- parameters (audio thread: the core is locked)

// the list (after the core boots): MASTER, MACRO 1..4, then every settable project setting, tracks 1..4's, the
// song's, then tracks 5..8's (not the device's own: A4, the MIDI routing ..; MACRO 1..4 once only, as ids 1..4)
void Tessera::params_make()
{
    params.clear();
    params.push_back({PARAM_MASTER, 0, 0, 0, 1023, 700, "MASTER", ""});
    for (uint32_t k = 0; k < 4; k++)
    {
        Param m{PARAM_MACRO1 + k, 0, 0, 0, 100, 0, "", "PERF"};
        std::snprintf(m.name, sizeof m.name, "MACRO %u", k + 1);
        params.push_back(m);
    }
    uint32_t n = core->param_count();
    for (uint32_t slot = 0; slot < PARAM_SLOTS; slot++)
        for (uint32_t i = 0; i < n; i++)
        {
            tess_param_t info;
            bool song = slot == PARAM_SONG;
            uint32_t trk = song ? 0 : slot_track(slot);
            uint32_t pid = core->param_at(i, trk, &info);
            if (pid == 0xFFFFu || !info.settable || (info.per_track != 0) == song || pid >= 0x1000u)
                continue;
            if (!std::strncmp(info.name, "MACRO ", 6) && !info.per_track)
                continue;   // (MACRO 1..4: ids 1..4 already)
            if (pid >= PID_MOD_SLOT && pid < PID_MOD_SLOT + 12u)
                continue;   // (MOD 1..4's routes: the core makes each a cord at once (cord.c cord_adopt) and clears it)
            Param q{PARAM_PID + slot * 0x1000u + pid, pid, trk, info.span_min, info.span_max, info.def, "", ""};
            if (!song)
            {
                std::snprintf(q.name, sizeof q.name, "T%u %s", trk + 1, info.name);
                std::snprintf(q.module, sizeof q.module, "Track %u/%s", trk + 1, pid >= 0x200u ? "Generator" : "Sound and sequence");
            }
            else
            {
                std::snprintf(q.name, sizeof q.name, "%s", info.name);
                std::snprintf(q.module, sizeof q.module, "Song");
            }
            params.push_back(q);
        }
    param_now.reset(new std::atomic<int32_t>[params.size()]);
    param_look.reset(new uint32_t[params.size()]);
    gesture_open.assign(params.size(), 0);
    gesture_list.clear();
    gesture_list.reserve(params.size());   // (the audio thread pushes, never grows it)
    for (size_t i = 0; i < params.size(); i++)
    {
        param_now[i] = param_core((int)i);
        params[i].def = param_now[i];   // (the default: what a new instance starts with, its first project's)
        param_look[i] = param_look_of((int)i);
    }
}
// (the core locked) a hash of how parameter i's values read: its texts at the ends and the middle of its range
uint32_t Tessera::param_look_of(int i)
{
    if (i < PARAM_LEGACY)
        return 0;
    const Param &q = params[i];
    uint32_t h = 2166136261u;   // (FNV-1a)
    const int32_t at[3] = {q.min, q.min + (q.max - q.min) / 2, q.max};
    for (int32_t v : at)
    {
        char s[48];
        if (!core->param_text(q.pid, q.track, v, s, sizeof s))
            s[0] = 0;
        for (const char *c = s;; c++)
        {
            h = (h ^ (uint8_t)*c) * 16777619u;
            if (!*c)
                break;
        }
    }
    return h;
}
int Tessera::param_index(clap_id id) const
{
    size_t lo = 0, hi = params.size();   // (in id order)
    while (lo < hi)
    {
        size_t mid = (lo + hi) / 2;
        if (params[mid].id == id) return (int)mid;
        if (params[mid].id < id) lo = mid + 1;
        else hi = mid;
    }
    return -1;
}
int32_t Tessera::param_core(int i)
{
    if (i == PARAM_MASTER) return master;
    if (i < PARAM_LEGACY) return (int32_t)core->macro((uint32_t)(i - PARAM_MACRO1));
    return core->param_get(params[i].pid, params[i].track);
}
void Tessera::param_set(int i, int32_t v)
{
    if (i == PARAM_MASTER)
        master_set(v);
    else if (i < PARAM_LEGACY)
        core->macro_set((uint32_t)(i - PARAM_MACRO1), (uint32_t)(v < 0 ? 0 : v));
    else
        core->param_set(params[i].pid, params[i].track, v);
    param_now[i] = param_core(i);
}
// (the core locked) every value read again: a project came in
void Tessera::params_refresh()
{
    bool look = false;
    for (size_t i = PARAM_MACRO1; i < params.size(); i++)
    {
        param_now[i] = param_core((int)i);
        uint32_t h = param_look_of((int)i);
        if (h != param_look[i]) param_look[i] = h, look = true;
    }
    if (look && !text_stale.exchange(true) && host->request_callback)
        host->request_callback(host);   // (its engines, FX and scales: p_on_main_thread)
}
void Tessera::param_in(const clap_event_param_value_t *e)
{
    int i = param_index(e->param_id);
    if (i >= 0)
        param_set(i, (int32_t)std::lround(e->value));
}
void Tessera::gesture(const clap_output_events_t *out, uint32_t at, int i, bool begin)
{
    clap_event_param_gesture_t e = {};
    e.header = {sizeof e, at, CLAP_CORE_EVENT_SPACE_ID,
                (uint16_t)(begin ? CLAP_EVENT_PARAM_GESTURE_BEGIN : CLAP_EVENT_PARAM_GESTURE_END), 0};
    e.param_id = params[i].id;
    out->try_push(out, &e.header);
}
// a block's end: the gestures still open end once the knobs have been still long enough
void Tessera::gestures_tick(const clap_output_events_t *out, uint32_t n)
{
    if (!gesture_left)
        return;
    gesture_left = gesture_left > n ? gesture_left - n : 0;
    if (gesture_left)
        return;
    for (int i : gesture_list)
    {
        if (out) gesture(out, n ? n - 1 : 0, i, false);
        gesture_open[i] = 0;
    }
    gesture_list.clear();
}

void Tessera::param_event(const clap_output_events_t *out, uint32_t at, int i, int32_t v)
{
    if (!out)
        return;
    if (gesture_left && !gesture_open[i])   // (a knob turned: this parameter's undo step begins)
    {
        gesture(out, at, i, true);
        gesture_open[i] = 1;
        gesture_list.push_back(i);
    }
    clap_event_param_value_t e = {};
    e.header = {sizeof e, at, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e.param_id = params[i].id;
    e.cookie = nullptr;
    e.note_id = -1;
    e.port_index = -1;
    e.channel = -1;
    e.key = -1;
    e.value = v;
    out->try_push(out, &e.header);
}
// values the panel, a project or the core changed since the host last heard: MASTER and the MACROs each block, the
// rest PARAM_SCAN at a time round the list (a full round in ~20 blocks)
void Tessera::params_out(const clap_output_events_t *out, uint32_t at, bool all)
{
    for (int i = 0; i < PARAM_LEGACY && i < (int)params.size(); i++)
    {
        int32_t v = param_core(i);
        if (v != param_now[i]) param_now[i] = v, param_event(out, at, i, v);
    }
    if (all)   // (the host set some: one can move others, as a QUAD's moves follow its LINK)
        for (size_t i = PARAM_LEGACY; i < params.size(); i++)
        {
            int32_t v = param_core((int)i);
            if (v != param_now[i]) param_now[i] = v, param_event(out, at, (int)i, v);
        }
    uint32_t rest = (uint32_t)params.size() - PARAM_LEGACY;
    for (uint32_t k = 0; k < PARAM_SCAN && k < rest; k++)
    {
        if (param_scan >= params.size()) param_scan = PARAM_LEGACY;
        int i = (int)param_scan++;
        int32_t v = param_core(i);
        if (v != param_now[i]) param_now[i] = v, param_event(out, at, i, v);
        uint32_t look = param_look_of(i);
        if (look != param_look[i])
        {
            param_look[i] = look;
            if (!text_stale.exchange(true) && host->request_callback)
                host->request_callback(host);   // (p_on_main_thread)
        }
    }
}

// ---- plugin

bool p_init(const clap_plugin_t *p)
{
    auto *t = self(p);
    if (t->core->tracks() != NTRACKS)   // (core_defs.txt and NTRACKS disagree: the ports and ids would be wrong)
        return false;
    std::lock_guard<std::mutex> g(t->core_lock);
    bool again;
    {
        std::lock_guard<std::mutex> c(copies_lock);
        again = copy_booted[t->copy];
        copy_booted[t->copy] = true;
    }
    t->core->boot();
    if (again)                 // (a copy an instance gave back: a new project, as a fresh copy starts with, and its
    {                          // power-on MIDI setup, not the last instance's ROUT, NOTES OUT ..)
        t->core->project_new();
        t->core->setup_load(copy_setup[t->copy], copy_setup_n[t->copy]);
    }
    else
        copy_setup_n[t->copy] = t->core->setup_save(copy_setup[t->copy], sizeof copy_setup[0]);
    // a shell and ink saved by a build before TESSERA's shell (or before this mark) are put aside once: the first window
    // of this build opens in TESSERA's shell; picks after that are remembered as before
    if (projfile::setting_get("shell_v") != "2")
    {
        projfile::setting_put("shell", Panel::shell_id(0));
        projfile::setting_put("ink", "");
        projfile::setting_put("shell_v", "2");
    }
    if (projfile::setting_get("shell_v") == "2")   // (TESSERA's shell was YELLOW until 0.1-beta5: that ink, remembered
    {                                             // as the last shown, gives way to its MOSAIC once; another ink stays)
        if (projfile::setting_get("ink") == "YELLOW")
            projfile::setting_put("ink", "");
        projfile::setting_put("shell_v", "3");
    }
    if (projfile::setting_get("shell_v") == "3")   // (the FM-1's shells are no longer offered: one picked opens as
    {                                             // TESSERA's, NIGHT, with its MOSAIC; NIGHT or DAY stays)
        if (!Panel::shell_native(Panel::shell_by_id(projfile::setting_get("shell"))))
        {
            projfile::setting_put("shell", Panel::shell_id(0));
            projfile::setting_put("ink", "");
        }
        projfile::setting_put("shell_v", "4");
    }
    int sh = Panel::shell_by_id(projfile::setting_get("shell"));   // (the shell last picked, and its ink)
    if (sh >= 0)
    {
        t->shell_i = sh;
        t->core->ink(Panel::shell_ink(sh));
    }
    std::string ink = projfile::setting_get("ink");   // (the ink last shown, if it differs from the shell's)
    if (!ink.empty())
        t->core->ink(ink.c_str());
    t->master_set(700);   // (MASTER's default)
    {   // SIZE: the one last picked in any window (none: the host's scale)
        std::string sz = projfile::setting_get("size");
        if (!sz.empty()) t->size_user = Panel::size_fit(std::atoi(sz.c_str()), false);
    }
    t->mode_expanded.store(projfile::setting_get("mode") == "expanded");   // MODE: the one last picked (none: gui_create)
    t->tracks_on.store(t->win_expanded());
    {   // GENERATE: the style (by name) and VARIATION last picked in any window
        std::string st = projfile::setting_get("gen_style"), var = projfile::setting_get("gen_variation");
        if (st == "RND") st = "WILD";   // (RND's name before it was WILD)
        for (uint32_t i = 0, n = t->core->gen_styles(); i < n; i++)
            if (st == t->core->gen_style(i)) t->gen_style = (int)i;
        if (!var.empty()) t->gen_variation = std::atoi(var.c_str()) < 0 ? 0 : std::atoi(var.c_str()) > 100 ? 100 : std::atoi(var.c_str());
    }
    t->params_make();
    return true;
}

void p_destroy(const clap_plugin_t *p)
{
    auto *t = self(p);
    t->gui.reset();
    Fm1Link::get().closed(t);
    uint32_t copy = t->copy;
    delete t;
    std::lock_guard<std::mutex> c(copies_lock);
    copy_used[copy] = false;
}

bool p_activate(const clap_plugin_t *p, double rate, uint32_t, uint32_t)
{
    auto *t = self(p);
    t->rate = rate;
    t->rs.setup((double)t->core->rate(), rate);
    t->buf_n = t->buf_i = 0;
    t->transport_known = t->free_started = false;
    t->view_due = 0;
    if (!t->fm1_started)   // (here, not at init: a DAW scanning its plugins opens and closes each without this)
    {
        t->fm1_started = true;
        if (projfile::setting_get("fm1_usb") == "1")
            Fm1Link::get().want(t, true, false);   // (behind any Tessera that has it already)
    }
    {
        std::lock_guard<std::mutex> g(t->input_lock);   // MASTER where the window's knob starts
        t->pending.changed = true;
    }
    return true;
}

// (no events can go out here: the next block sends the note-offs, so a synth on MIDI OUT is not left holding them)
void p_deactivate(const clap_plugin_t *p) { self(p)->notes_due = true; }
bool p_start_processing(const clap_plugin_t *) { return true; }
void p_stop_processing(const clap_plugin_t *p) { self(p)->notes_due = true; }
void p_reset(const clap_plugin_t *p)
{
    self(p)->rs.reset();
    self(p)->notes_due = true;
}

clap_process_status p_process(const clap_plugin_t *p, const clap_process_t *proc)
{
    auto *t = self(p);
    uint32_t n = proc->frames_count;
    // the outputs: main L R, then each track's port (its left; its right gets a copy)
    float *out[NCH] = {};
    bool any_stem = false;
    for (uint32_t port = 0; port < proc->audio_outputs_count && port < 1 + NSTEMS; port++)
    {
        const clap_audio_buffer_t &b = proc->audio_outputs[port];
        if (!b.data32 || !b.channel_count)
            continue;
        if (port == 0)
        {
            out[0] = b.data32[0];
            out[1] = b.channel_count > 1 ? b.data32[1] : nullptr;
        }
        else
        {
            out[1 + port] = b.data32[0];
            any_stem = true;
        }
    }
    t->stems = t->stems || any_stem;   // (once asked for, the core keeps making them)

    std::unique_lock<std::mutex> g(t->core_lock, std::try_to_lock);
    if (!g.owns_lock())   // the main thread is saving or loading the project: a block of silence
    {
        for (uint32_t port = 0; port < proc->audio_outputs_count; port++)
            for (uint32_t c = 0; c < proc->audio_outputs[port].channel_count; c++)
                if (proc->audio_outputs[port].data32)
                    std::memset(proc->audio_outputs[port].data32[c], 0, n * sizeof(float));
        return CLAP_PROCESS_CONTINUE;
    }
    if (t->notes_due.exchange(false))
        t->core->notes_off();

    if (proc->transport)
    {
        const clap_event_transport_t *tr = proc->transport;
        bool playing = (tr->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
        bool has_pos = (tr->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) && (tr->flags & CLAP_TRANSPORT_HAS_TEMPO);
        if (playing || (t->transport_known && t->was_playing))   // (stopped: once, so the panel's PLAY still works)
            t->core->host(playing, has_pos, tr->tempo, (double)tr->song_pos_beats / CLAP_BEATTIME_FACTOR);
        t->transport_known = true;
        t->was_playing = playing;
    }
    else if (!t->free_started)   // a free-running host (the standalone app): its clock runs at once (a new project:
    {                            // silent till notes, a demo from OPEN or a song)
        t->core->transport(1);
        t->free_started = true;
    }

    {   // the window's input, if the window is not handing some in right now
        std::unique_lock<std::mutex> in(t->input_lock, std::try_to_lock);
        PanelInput &pi = t->pending;
        if (in.owns_lock() && pi.changed)
        {
            t->win_buttons = pi.buttons;
            t->win_keys = pi.keys;
            t->core->key_words(pi.key_words ? 1 : 0);   // (TESSERA's shell: BACK / OK / SHIFT of its own)
            t->core->buttons(t->win_buttons | t->fm1_buttons | TESS_BTN_POINTER);   // (new presses: the mouse's)
            t->core->keys(t->win_keys | t->fm1_keys);
            bool master_turned = pi.master >= 0;
            if (pi.master >= 0)
                t->master_set(pi.master);
            pi.master = -1;
            if (pi.track >= 0)
                t->core->track_select((uint32_t)pi.track);
            pi.track = -1;
            bool turned = master_turned || !pi.zones.empty();   // (a knob: the settings it moves are one undo step)
            for (int r = 0; r < TESS_NE; r++)
                if (pi.enc[r])
                {
                    t->core->enc((uint32_t)r, pi.enc[r]);
                    pi.enc[r] = 0;
                    turned = true;
                }
            if (turned)
                t->gesture_left = (uint32_t)(t->rate * Tessera::GESTURE_MS / 1000);
            for (auto &a : pi.zones)   // the screen's zones: what the mouse did there, in order
                t->core->zone_do(&a.z, a.action, a.n);
            pi.zones.clear();          // (keeps its room: no allocation on this thread)
            for (auto &a : pi.tzones)  // TRACKS' (EXPANDED)
                t->core->tracks_do(&a.z, a.action, a.n);
            pi.tzones.clear();
            for (uint32_t ch : pi.text)   // typed into NAME
                t->core->text(ch);
            pi.text.clear();
            for (auto &a : pi.typed)   // a value typed (Ctrl / Cmd-click)
                t->core->zone_type(&a.z, a.text);
            pi.typed.clear();
            if (pi.away)               // a click away: a picker, a sheet or a pinned help card closed
                t->core->away();
            pi.away = false;
            if (pi.hover_changed)      // HOVER HELP: the zone the mouse rests on
                t->core->hover(pi.hover.kind ? &pi.hover : nullptr);
            pi.hover_changed = false;
            pi.changed = false;
        }
    }
    {   // an FM-1 on USB (fm1_link.h): what it played since the last block, if this Tessera has it
        Fm1Input fi;
        if (Fm1Link::get().take(t, fi))
        {
            if (fi.buttons != t->fm1_buttons || fi.keys != t->fm1_keys)
            {
                t->fm1_buttons = fi.buttons;
                t->fm1_keys = fi.keys;
                t->core->buttons(t->win_buttons | t->fm1_buttons);
                t->core->keys(t->win_keys | t->fm1_keys);
            }
            if (fi.master >= 0)
                t->master_set(fi.master);
            bool turned = fi.master >= 0;
            for (int r = 0; r < TESS_NE; r++)
                if (fi.enc[r])
                {
                    t->core->enc((uint32_t)r, fi.enc[r]);
                    turned = true;
                }
            if (turned)
                t->gesture_left = (uint32_t)(t->rate * Tessera::GESTURE_MS / 1000);
        }
    }

    // events at their sample: render up to each, then hand it in
    uint32_t done = 0;
    bool set = false;
    uint32_t ne = proc->in_events ? proc->in_events->size(proc->in_events) : 0;
    for (uint32_t i = 0; i < ne; i++)
    {
        const clap_event_header_t *h = proc->in_events->get(proc->in_events, i);
        uint32_t at = h->time < n ? h->time : n;
        if (at > done)
        {
            t->render(out, done, at - done);
            done = at;
        }
        if (h->space_id == CLAP_CORE_EVENT_SPACE_ID && h->type == CLAP_EVENT_PARAM_VALUE)
            t->param_in(reinterpret_cast<const clap_event_param_value_t *>(h)), set = true;
        else
            t->midi_in(h);
    }
    if (done < n)
        t->render(out, done, n - done);
    for (uint32_t port = 0; port < proc->audio_outputs_count && port < 1 + NSTEMS; port++)   // (mono into stereo)
    {
        const clap_audio_buffer_t &b = proc->audio_outputs[port];
        if (b.data32 && b.channel_count > 1 && (port || !out[1]))
            std::memcpy(b.data32[1], b.data32[0], n * sizeof(float));
    }

    t->midi_out(proc->out_events, n);   // (the tracks' notes, each on its channel: GLO > MIDI OUT)

    t->params_out(proc->out_events, n ? n - 1 : 0, set || t->listen_params);   // (a setting changed on the panel, by a
    t->listen_params = false;                                               // project, a cord's source or LISTEN)
    t->gestures_tick(proc->out_events, n);

    if (t->view_due <= n)   // the view for the window, ~30 times a second
    {
        std::unique_lock<std::mutex> v(t->view_lock, std::try_to_lock);
        if (v.owns_lock())
        {
            t->core->view(&t->shared_view);
            if (t->tracks_on.load(std::memory_order_relaxed))   // (EXPANDED: TRACKS too)
                t->core->tracks_view(t->shared_tracks.get(), TESS_TRACKS_W, TESS_TRACKS_H);
            t->view_due = (uint32_t)(t->rate / 30);
        }
    }
    else
        t->view_due -= n;
    return CLAP_PROCESS_CONTINUE;
}

// ---- audio ports: main (the mix, as the FM-1's output), then TRACK 1..8 (each alone: the FM-1's USB stems)

uint32_t ap_count(const clap_plugin_t *, bool is_input) { return is_input ? 0 : 1 + NSTEMS; }
bool ap_get(const clap_plugin_t *p, uint32_t index, bool is_input, clap_audio_port_info_t *info)
{
    const bool mono = index > 0 && self(p)->mono_outs;   // (OUTPUTS MONO: a track's port one channel)
    if (is_input || index > NSTEMS)
        return false;
    info->id = index;
    if (index == 0)
        std::strncpy(info->name, "Main", sizeof(info->name));
    else
        std::snprintf(info->name, sizeof(info->name), "Track %u", index);
    info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
    info->channel_count = mono ? 1 : 2;
    info->port_type = mono ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}
const clap_plugin_audio_ports_t audio_ports = {ap_count, ap_get};

// ---- audio ports configs (a host that offers them: the track ports as stereo pairs, 0, or mono, 1; Main stereo)
uint32_t apc_count(const clap_plugin_t *) { return 2; }
bool apc_get(const clap_plugin_t *, uint32_t index, clap_audio_ports_config_t *config)
{
    if (index > 1)
        return false;
    config->id = index;
    std::strncpy(config->name, index ? "Tracks mono" : "Tracks stereo", sizeof(config->name));
    config->input_port_count = 0;
    config->output_port_count = 1 + NSTEMS;
    config->has_main_input = false;
    config->has_main_output = true;
    config->main_output_channel_count = 2;
    config->main_output_port_type = CLAP_PORT_STEREO;
    return true;
}
bool apc_select(const clap_plugin_t *p, clap_id config_id)   // (the host calls it while deactivated)
{
    if (config_id > 1)
        return false;
    self(p)->mono_outs = config_id == 1;
    return true;
}
const clap_plugin_audio_ports_config_t audio_ports_config = {apc_count, apc_get, apc_select};

// ---- note ports: one in

uint32_t np_count(const clap_plugin_t *, bool) { return 1; }
bool np_get(const clap_plugin_t *, uint32_t index, bool is_input, clap_note_port_info_t *info)
{
    if (index != 0)
        return false;
    info->id = is_input ? 0 : 1;
    info->supported_dialects =
        is_input ? CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE : CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    std::strncpy(info->name, is_input ? "MIDI in" : "MIDI out", sizeof(info->name));
    return true;
}
const clap_plugin_note_ports_t note_ports = {np_count, np_get};

// ---- state: the project as a .tsp record (tracks 1..4 in the parts an FM-1 reads, tracks 5..8 in its XTRK part,
// project_tsp.c), then the device's MIDI setup (ROUT, DRUM's channel, NOTES OUT, TUNE: tess_core_setup_save, never
// in the .tsp) as its bytes, their count and "TGLO", then MASTER (the device's, not the project's) as "TMST" and 2
// bytes, then while LISTEN is on or its SPLIT moved "TLSN", its channel and SPLIT (1 byte each; a plugin build before
// LISTEN cannot open such a state, one with LISTEN off at SPLIT 48 is as before). A state of a plugin build before XTRK may also hold "TX58" (tracks 5..8's settings by central ID: the count and the tag), read as before; none
// is written now (XTRK holds them, with the steps, voices and generators).

constexpr uint32_t TX58_ENTRY = 7;   // track, pid (2 bytes), value (4 bytes), little-endian
uint32_t tx58_get(const uint8_t *b, int n)
{
    uint32_t v = 0;
    for (int k = 0; k < n; k++) v |= (uint32_t)b[k] << (8 * k);
    return v;
}
// (the core locked) an older state's TX58, after its project loaded: tracks 5..8's settings back. Twice: a setting can
// move another (an ENGINE its sound's), and the second pass puts back what the first one's later moved
void tx58_load(Tessera *t, const uint8_t *b, uint32_t n)
{
    for (int pass = 0; pass < 2; pass++)
        for (uint32_t i = 0; i < n; i++)
        {
            const uint8_t *e = b + i * TX58_ENTRY;
            uint32_t trk = e[0], pid = tx58_get(e + 1, 2);
            int32_t v = (int32_t)tx58_get(e + 3, 4);
            if (trk >= 4 && trk < NTRACKS && (pass == 0 || t->core->param_get(pid, trk) != v))
                t->core->param_set(pid, trk, v);
        }
}

bool st_save(const clap_plugin_t *p, const clap_ostream_t *s)
{
    auto *t = self(p);
    std::vector<uint8_t> b(t->core->tsp_max());
    uint32_t len;
    {
        std::lock_guard<std::mutex> g(t->core_lock);
        len = t->core->save(b.data(), (uint32_t)b.size());
        if (len)
        {
            uint8_t setup[64];
            uint32_t ns = t->core->setup_save(setup, sizeof setup);
            b.resize(len);
            if (ns)
            {
                b.insert(b.end(), setup, setup + ns);
                const uint8_t tag[5] = {(uint8_t)ns, 'T', 'G', 'L', 'O'};
                b.insert(b.end(), tag, tag + 5);
                len += ns + 5;
            }
            uint32_t m = (uint32_t)t->master;
            const uint8_t tail[6] = {'T', 'M', 'S', 'T', (uint8_t)m, (uint8_t)(m >> 8)};
            b.insert(b.begin() + len, tail, tail + 6);
            len += 6;
            int lc = t->listen_ch.load(), lsp = t->listen_split.load();
            if (lc || lsp != Listen::SPLIT)   // (LISTEN on or SPLIT moved: last; else the state is as before LISTEN)
            {
                const uint8_t ls[6] = {'T', 'L', 'S', 'N', (uint8_t)lc, (uint8_t)lsp};
                b.insert(b.begin() + len, ls, ls + 6);
                len += 6;
            }
            std::vector<uint8_t> ab = t->snaps_state();   // (A / B, Cosmati audit G7: after it all, only once stored)
            if (!ab.empty())
            {
                uint32_t n = (uint32_t)ab.size();
                const uint8_t tag[8] = {(uint8_t)n, (uint8_t)(n >> 8), (uint8_t)(n >> 16), (uint8_t)(n >> 24), 'T', 'S', 'N', 'P'};
                b.insert(b.begin() + len, ab.begin(), ab.end());
                b.insert(b.begin() + len + n, tag, tag + 8);
                len += n + 8;
            }
        }
    }
    if (!len)
        return false;
    for (uint32_t o = 0; o < len;)
    {
        int64_t w = s->write(s, b.data() + o, len - o);
        if (w <= 0)
            return false;
        o += (uint32_t)w;
    }
    return true;
}

bool st_load(const clap_plugin_t *p, const clap_istream_t *s)
{
    auto *t = self(p);
    const uint32_t ab_max = 2 * (t->core->tsp_max() + 8 + 5 * t->core->param_count() * t->core->tracks()) + 2 + 8;
    std::vector<uint8_t> b(t->core->tsp_max() + 6 + 6 + 260 + 8 + TX58_ENTRY * (uint32_t)t->params.size() + ab_max);
    uint32_t len = 0;
    for (;;)
    {
        if (len == b.size())
            return false;   // larger than any record
        int64_t got = s->read(s, b.data() + len, b.size() - len);
        if (got < 0)
            return false;
        if (got == 0)
            break;
        len += (uint32_t)got;
    }
    bool ok;
    {
        std::lock_guard<std::mutex> g(t->core_lock);
        bool ab = false;   // (A / B: the last part, once stored)
        if (len >= 8 && !std::memcmp(&b[len - 4], "TSNP", 4))
        {
            uint32_t n = b[len - 8] | (uint32_t)b[len - 7] << 8 | (uint32_t)b[len - 6] << 16 | (uint32_t)b[len - 5] << 24;
            if ((uint64_t)n + 8 > len || !t->snaps_load(b.data() + len - 8 - n, n))
                return false;   // (not a state this build wrote)
            len -= n + 8;
            ab = true;
        }
        if (!ab)
        {
            std::lock_guard<std::mutex> sl(t->snap_lock);
            t->snaps[0] = {}, t->snaps[1] = {}, t->snap_active = t->snap_morph = -1;
        }
        int lc = 0, lsp = Listen::SPLIT;
        if (len >= 6 && !std::memcmp(&b[len - 6], "TLSN", 4) && b[len - 2] <= 16 && Listen::split_ok(b[len - 1]))
            lc = b[len - 2], lsp = b[len - 1], len -= 6;
        t->listen_ch.store(lc);
        t->listen_split.store(lsp);
        if (len >= 6 && !std::memcmp(&b[len - 6], "TMST", 4))
        {
            len -= 6;
            uint32_t m = b[len + 4] | (uint32_t)b[len + 5] << 8;
            t->master_set((int32_t)m);
            t->param_now[PARAM_MASTER] = t->master;
        }
        const uint8_t *setup = nullptr;
        uint32_t ns = 0;
        if (len >= 5 && !std::memcmp(&b[len - 4], "TGLO", 4))
        {
            ns = b[len - 5];
            if (ns + 5u > len)
                return false;   // (not a state this build wrote)
            len -= ns + 5;
            setup = b.data() + len;
        }
        const uint8_t *x58 = nullptr;
        uint32_t n58 = 0;
        if (len >= 8 && !std::memcmp(&b[len - 4], "TX58", 4))
        {
            n58 = tx58_get(&b[len - 8], 4);
            if ((uint64_t)n58 * TX58_ENTRY + 8 > len)
                return false;   // (not a state this build wrote)
            len -= 8 + n58 * TX58_ENTRY;
            x58 = b.data() + len;
        }
        ok = t->core->load(b.data(), len) == 0;
        if (ok)
            t->gen_before = false;   // (GENERATE: what plays is the DAW's state, not BEFORE)
        if (ok && setup)
            t->core->setup_load(setup, ns);   // (a state from before TGLO keeps the setup this instance has)
        if (ok && x58)
            tx58_load(t, x58, n58);
        t->params_refresh();
    }
    auto *hp = static_cast<const clap_host_params_t *>(t->host->get_extension(t->host, CLAP_EXT_PARAMS));
    if (hp && hp->rescan)
        hp->rescan(t->host, CLAP_PARAM_RESCAN_VALUES);   // (the project's settings: the host reads them again)
    return ok;
}
const clap_plugin_state_t state = {st_save, st_load};

// ---- preset-load: a preset picked in the DAW's browser (presets.cpp: a demo, or a .tsp of the user's), loaded as OPEN
// loads a project; the host told it loaded (its browser shows it), or why not
bool pl_from_location(const clap_plugin_t *p, uint32_t kind, const char *location, const char *load_key)
{
    auto *t = self(p);
    auto *hpl = static_cast<const clap_host_preset_load_t *>(t->host->get_extension(t->host, CLAP_EXT_PRESET_LOAD));
    if (!hpl) hpl = static_cast<const clap_host_preset_load_t *>(t->host->get_extension(t->host, CLAP_EXT_PRESET_LOAD_COMPAT));
    std::vector<uint8_t> rec;
    std::string err;
    uint32_t r = 1;
    if (presets::record(kind, location, load_key, rec, err))
    {
        {
            std::lock_guard<std::mutex> g(t->core_lock);
            r = t->core->load(rec.data(), (uint32_t)rec.size());
        }
        if (r) err = "A PROJECT THIS VERSION CANNOT READ";
    }
    if (r)
    {
        if (hpl && hpl->on_error) hpl->on_error(t->host, kind, location, load_key, 0, err.c_str());
        return false;
    }
    t->gen_before = false;
    t->changed_by_file("PRESET");   // (the DAW's undo: the preset loaded as one step)
    if (hpl && hpl->loaded) hpl->loaded(t->host, kind, location, load_key);
    return true;
}
const clap_plugin_preset_load_t preset_load_ext = {pl_from_location};

// ---- latency: the resampler plays half its kernel behind (resample.h: 32 core frames, ~0.7 ms; none at 44.1 kHz)
uint32_t lat_get(const clap_plugin_t *p) { return self(p)->rs.delay_host(); }
const clap_plugin_latency_t latency_ext = {lat_get};

// ---- the parameters' extension

uint32_t pr_count(const clap_plugin_t *p) { return (uint32_t)self(p)->params.size(); }
bool pr_info(const clap_plugin_t *p, uint32_t i, clap_param_info_t *info)
{
    auto *t = self(p);
    if (i >= t->params.size())
        return false;
    const Param &q = t->params[i];
    std::memset(info, 0, sizeof *info);
    info->id = q.id;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
    std::snprintf(info->name, sizeof info->name, "%s", q.name);
    std::snprintf(info->module, sizeof info->module, "%s", q.module);
    info->min_value = q.min;
    info->max_value = q.max;
    info->default_value = q.def;
    return true;
}
bool pr_value(const clap_plugin_t *p, clap_id id, double *v)
{
    auto *t = self(p);
    int i = t->param_index(id);
    if (i < 0)
        return false;
    *v = t->param_now[i];
    return true;
}
// (on the host's main thread, often: not locked, so the audio never waits for a display. The text reads the core's
// tables and the track's engine, as the FM-1's screen would)
bool pr_to_text(const clap_plugin_t *p, clap_id id, double v, char *out, uint32_t size)
{
    auto *t = self(p);
    int i = t->param_index(id);
    if (i < 0 || !size)
        return false;
    int32_t x = (int32_t)std::lround(v);
    if (i == PARAM_MASTER)
        std::snprintf(out, size, "%d %%", (int)std::lround(v * 100 / 1023));
    else if (i < PARAM_LEGACY || !t->core->param_text(t->params[i].pid, t->params[i].track, x, out, size))
        std::snprintf(out, size, "%d", (int)x);
    return true;
}
bool pr_from_text(const clap_plugin_t *p, clap_id id, const char *text, double *v)
{
    auto *t = self(p);
    int i = t->param_index(id);
    if (i < 0)
        return false;
    const Param &q = t->params[i];
    if (i >= PARAM_LEGACY)   // a name or a value as shown ("SAW", "1/16", "42%"), any case
        for (int32_t x = q.min; x <= q.max; x++)
        {
            char s[48];
            if (t->core->param_text(q.pid, q.track, x, s, sizeof s) && !strcasecmp_ascii(s, text))
            {
                *v = x;
                return true;
            }
        }
    char *end = nullptr;
    double x = std::strtod(text, &end);
    if (end == text)
        return false;
    *v = i == PARAM_MASTER ? x * 1023 / 100 : x;
    return true;
}
void pr_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out)
{
    auto *t = self(p);
    std::lock_guard<std::mutex> g(t->core_lock);   // (not processing: the main thread, or the audio thread between blocks)
    uint32_t n = in ? in->size(in) : 0;
    bool any = false;
    for (uint32_t i = 0; i < n; i++)
    {
        const clap_event_header_t *h = in->get(in, i);
        if (h->space_id == CLAP_CORE_EVENT_SPACE_ID && h->type == CLAP_EVENT_PARAM_VALUE)
            t->param_in(reinterpret_cast<const clap_event_param_value_t *>(h)), any = true;
    }
    t->params_out(out, 0, any);
}
const clap_plugin_params_t params_ext = {pr_count, pr_info, pr_value, pr_to_text, pr_from_text, pr_flush};

// ---- the window

bool gui_is_api_supported(const clap_plugin_t *, const char *api, bool is_floating)
{
    return !is_floating && Gui::api() && !std::strcmp(api, Gui::api());
}
bool gui_get_preferred_api(const clap_plugin_t *, const char **api, bool *is_floating)
{
    if (!Gui::api()) return false;
    *api = Gui::api();
    *is_floating = false;
    return true;
}
bool gui_create(const clap_plugin_t *p, const char *api, bool is_floating)
{
    auto *t = self(p);
    if (!gui_is_api_supported(p, api, is_floating) || t->gui)
        return false;
    int sw = Gui::screen_width();
    t->size_room = sw > 0 ? sw - Tessera::EXPANDED_ROOM : 0;
    if (projfile::setting_get("mode").empty())   // no MODE picked yet: EXPANDED if the screen has room for it
        t->mode_default(sw);
    t->gui = Gui::make(t->host, t);
    if (t->gui)
        t->gui->set_scale(t->size_pct());
    return t->gui != nullptr;
}
void gui_destroy(const clap_plugin_t *p) { self(p)->gui.reset(); }
// the window's size: the panel at SIZE % (gui.h). A host's scale (Windows, X11: the screen's DPI) is the size until
// SIZE is picked; a host that lets its frame be dragged gets any size in the mode's range, at the panel's aspect
bool gui_set_scale(const clap_plugin_t *p, double scale)
{
    auto *t = self(p);
    int pct = scale > 0 ? (int)(scale * 100 + 0.5) : 100;
    bool was = t->size_pct() == Panel::size_fit(pct, t->win_expanded());
    t->size_host = pct;
    if (projfile::setting_get("mode").empty())   // (no MODE picked: EXPANDED only if it still fits at this scale)
    {
        bool wide = t->win_expanded();
        t->mode_default(Gui::screen_width());
        if (wide != t->win_expanded() && t->gui)
            t->resize_window();
    }
    if (!was && t->gui && !t->size_user)
        t->gui->set_scale(t->size_pct());
    return true;
}
bool gui_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h)
{
    auto *t = self(p);
    int pct = t->size_pct();
    *w = (uint32_t)(t->win_w() * pct / 100);
    *h = (uint32_t)(Panel::H * pct / 100);
    return true;
}
bool gui_can_resize(const clap_plugin_t *) { return true; }
bool gui_get_resize_hints(const clap_plugin_t *p, clap_gui_resize_hints_t *h)
{
    h->can_resize_horizontally = h->can_resize_vertically = true;
    h->preserve_aspect_ratio = true;
    h->aspect_ratio_width = (uint32_t)self(p)->win_w();
    h->aspect_ratio_height = Panel::H;
    return true;
}
// the SIZE (whole %, in the mode's range) whose width is nearest w
int size_nearest(const Tessera *t, uint32_t w)
{
    return Panel::size_fit(((int)w * 100 + t->win_w() / 2) / t->win_w(), t->win_expanded());
}
bool gui_adjust_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h)
{
    auto *t = self(p);
    int pct = size_nearest(t, *w);
    *w = (uint32_t)(t->win_w() * pct / 100);
    *h = (uint32_t)(Panel::H * pct / 100);
    return true;
}
bool gui_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h)
{
    auto *t = self(p);
    int pct = size_nearest(t, w);
    if (w == (uint32_t)(t->win_w() * pct / 100) && h == (uint32_t)(Panel::H * pct / 100))
        return t->size_set(pct);
    return false;
}
bool gui_set_parent(const clap_plugin_t *p, const clap_window_t *w)
{
    auto *t = self(p);
    return t->gui && t->gui->set_parent(w);
}
bool gui_set_transient(const clap_plugin_t *, const clap_window_t *) { return false; }
void gui_suggest_title(const clap_plugin_t *, const char *) {}
bool gui_show(const clap_plugin_t *p)
{
    auto *t = self(p);
    return t->gui && t->gui->show();
}
bool gui_hide(const clap_plugin_t *p)
{
    auto *t = self(p);
    return t->gui && t->gui->hide();
}
const clap_plugin_gui_t gui_ext = {gui_is_api_supported, gui_get_preferred_api, gui_create, gui_destroy,
                                   gui_set_scale, gui_get_size, gui_can_resize, gui_get_resize_hints,
                                   gui_adjust_size, gui_set_size, gui_set_parent, gui_set_transient,
                                   gui_suggest_title, gui_show, gui_hide};

void timer_tick(const clap_plugin_t *p, clap_id)   // (X11: the host's timer drives the window)
{
    auto *t = self(p);
    if (t->gui)
        t->gui->tick();
}
const clap_plugin_timer_support_t timer_ext = {timer_tick};

const void *p_get_extension(const clap_plugin_t *, const char *id)
{
    if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &params_ext;
    if (!std::strcmp(id, CLAP_EXT_GUI)) return Gui::api() ? &gui_ext : nullptr;
    if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &timer_ext;
    if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &audio_ports;
    if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS_CONFIG)) return &audio_ports_config;
    if (!std::strcmp(id, CLAP_EXT_NOTE_PORTS)) return &note_ports;
    if (!std::strcmp(id, CLAP_EXT_STATE)) return &state;
    if (!std::strcmp(id, CLAP_EXT_PRESET_LOAD) || !std::strcmp(id, CLAP_EXT_PRESET_LOAD_COMPAT)) return &preset_load_ext;
    if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &latency_ext;
    return nullptr;
}

// a setting's values read differently now (a track's engine, an FX type, the scale): the host's lists and lanes
// show the new names ("CUTOFF 2.1kHz", not "ENGINE 1 54")
void p_on_main_thread(const clap_plugin_t *p)
{
    auto *t = self(p);
    if (uint64_t g = t->listen_gen.exchange(0))   // (a note under SPLIT on the LISTEN channel: GENERATE)
        t->gen_seed((uint32_t)g, (int)(g >> 32) - 1);
    if (!t->text_stale.exchange(false))
        return;
    auto *hp = static_cast<const clap_host_params_t *>(t->host->get_extension(t->host, CLAP_EXT_PARAMS));
    if (hp && hp->rescan)
        hp->rescan(t->host, CLAP_PARAM_RESCAN_TEXT);
}

// ---- factory

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER,
                          CLAP_PLUGIN_FEATURE_STEREO, nullptr};
#ifndef TESS_VERSION_STR
#define TESS_VERSION_STR "0.0.1"   // (CMakeLists.txt's TESS_VERSION)
#endif
const clap_plugin_descriptor_t desc = {
    CLAP_VERSION_INIT,
    TESS_PLUGIN_ID,   // (the id stays tessera's: songs saved with the beta still find it)
    "Cosmati",         // (Tessera as a plugin; the firmware keeps its name)
    "Rastapopolix",
    "https://github.com/Rastapopolix/tessera",
    "",
    "https://github.com/Rastapopolix/tessera",
    TESS_VERSION_STR,
    "Tessera's generative sequencer and voices as a plugin",
    features,
};

uint32_t f_count(const clap_plugin_factory_t *) { return 1; }
const clap_plugin_descriptor_t *f_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &desc : nullptr; }
const clap_plugin_t *f_create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id)
{
    if (!clap_version_is_compatible(host->clap_version) || std::strcmp(id, desc.id))
        return nullptr;
    uint32_t copy = 0;
    {
        std::lock_guard<std::mutex> c(copies_lock);   // a free copy of the core, or none: refused
        while (copy < tess_copy_count() && copy_used[copy])
            copy++;
        if (copy >= tess_copy_count())
            return nullptr;
        copy_used[copy] = true;
    }
    auto *t = new Tessera();
    t->host = host;
    t->copy = copy;
    t->core = tess_copy(copy);
    t->plugin.desc = &desc;
    t->plugin.plugin_data = t;
    t->plugin.init = p_init;
    t->plugin.destroy = p_destroy;
    t->plugin.activate = p_activate;
    t->plugin.deactivate = p_deactivate;
    t->plugin.start_processing = p_start_processing;
    t->plugin.stop_processing = p_stop_processing;
    t->plugin.reset = p_reset;
    t->plugin.process = p_process;
    t->plugin.get_extension = p_get_extension;
    t->plugin.on_main_thread = p_on_main_thread;
    return &t->plugin;
}
const clap_plugin_factory_t factory = {f_count, f_desc, f_create};
}   // namespace

bool tessera_entry_init(const char *) { return true; }
void tessera_entry_deinit(void) {}
const void *tessera_entry_get_factory(const char *id)
{
    if (!std::strcmp(id, CLAP_PLUGIN_FACTORY_ID))
        return &factory;
    if (!std::strcmp(id, CLAP_PRESET_DISCOVERY_FACTORY_ID) || !std::strcmp(id, CLAP_PRESET_DISCOVERY_FACTORY_ID_COMPAT))
        return presets::factory();   // (the DAW's preset browser: presets.cpp)
    return nullptr;
}

