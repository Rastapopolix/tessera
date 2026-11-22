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
 *   - parameters (automatable): MASTER and PERF's MACRO 1..4 (ids 0..4, as since phase 2), then every project setting
 *     by its central ID (pid.h), each track's and the song's, with the FM-1's names and values; changed on the panel
 *     or by the core, they are sent back to the host;
 *   - the plugin's state is the project as a .tsp record, the same bytes SAVE > PROJECT writes on the FM-1; OPEN and
 *     SAVE under the keys open a .tsp or an FM-1 backup (its slots A..D and its music) and save a .tsp;
 *   - the window (gui.h, Windows, Linux and macOS) shows the FM-1: its screen, buttons, keys and knobs. The audio thread owns
 *     the core: the window's input goes to it through a small queue, and it hands back a view ~30 times a second.
 * Several instances: each takes one of the plugin's private copies of the core (core_copies.h; TESS_PLUGIN_COPIES of
 * them, 8), so each Tessera is wholly its own; past that a new one is refused. A copy given back is used again by the
 * next instance, which starts from the copy's power-on project (STILL TIDE) unless the host loads its state. */
#include <clap/clap.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include <clap/ext/params.h>
#include <clap/ext/state.h>

#include "core_copies.h"
#include "gui.h"
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
    void params_make();
    int param_index(clap_id id) const;
    int32_t param_core(int i);
    void param_set(int i, int32_t v);
    void params_refresh();
    void param_in(const clap_event_param_value_t *e);
    void params_out(const clap_output_events_t *out, uint32_t at, bool all = false);
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
        pending.changed = true;
        in.changed = false;
    }

    // gui.h: the project files (the host's main thread)
    std::string open_file(const std::string &path) override;
    std::string save_file(const std::string &path) override;
    std::string file_name() override;
    void changed_by_file();
    // the shell (gui.h): the last one picked in any window, from the plugin's settings
    int shell_i = 0;
    int shell() override { return shell_i; }
    void shell_picked(int i) override;
    // the inks (gui.h): MENU > COLOR's choices; the last one shown in any window, from the plugin's settings
    std::vector<Panel::Ink> inks() override;
    void ink_picked(int i) override;
    void ink_seen(int i) override;
    int tracks() override { return (int)core->tracks(); }
    // SIZE (gui.h): the one picked in any window (the setting `size`: 100, 150, 200), else the host's scale (a HiDPI
    // screen: set_scale) to the nearest of the three
    int size_user = 0, size_host = 2;   // halves; size_user 0: none picked
    int size_halves() override { return size_user ? size_user : size_host; }
    void size_pick(int pct) override;
    bool size_set(int halves);          // the window at that size (the host's set_size, a pick)
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
    std::string gen_recall(int dir) override;
    void gen_pick(int style, int variation) override;
    Panel::GenState gen_state() override;
    std::string gen_make(const Piece &pc);
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
    changed_by_file();
    return "";
}

std::string Tessera::generate(int style, int variation)
{
    gen_pick(style, variation);
    static std::mt19937 rng{std::random_device{}() ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    Piece pc{gen_style, gen_variation, 0};
    while (!pc.seed) pc.seed = rng();
    std::string said = gen_make(pc);
    if (!said.empty())
        return said;
    gen_made.push_back(pc);   // (made after going back: the newer ones stay, this one after them)
    if (gen_made.size() > GEN_KEEP) gen_made.erase(gen_made.begin());
    gen_at = (int)gen_made.size() - 1;
    return gen_made.size() == 1 ? "HOLD GENERATE FOR BEFORE" : "";   // (the strip shows the style and the seed)
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
    changed_by_file();
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
void Tessera::changed_by_file()
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
}

// SIZE picked in the menu: remembered for every window, the window that size, and the host asked to make room
void Tessera::size_pick(int pct)
{
    int n = Panel::size_halves(pct);
    projfile::setting_put("size", std::to_string(Panel::size_pct(n)));
    size_user = n;
    if (gui)
        gui->set_scale(n);
    auto *hg = static_cast<const clap_host_gui_t *>(host->get_extension(host, CLAP_EXT_GUI));
    if (hg && hg->request_resize)
        hg->request_resize(host, (uint32_t)(Panel::W * n / 2), (uint32_t)(Panel::H * n / 2));
}

bool Tessera::size_set(int n)
{
    if (n < 2 || n > 4)
        return false;
    if (n != size_halves())   // (the host's own resize, a drag on its frame: kept as the pick)
    {
        size_user = n;
        projfile::setting_put("size", std::to_string(Panel::size_pct(n)));
    }
    if (gui)
        gui->set_scale(n);
    return true;
}

// a shell picked under the keys: its ink on this Tessera's screen (as MENU > COLOR; after, that still changes it), and
// the shell the next window opens with, in any instance
void Tessera::shell_picked(int i)
{
    if (i < 0 || i >= Panel::shell_count())
        return;
    shell_i = i;
    {
        std::lock_guard<std::mutex> g(core_lock);
        core->ink(Panel::shell_ink(i));
    }
    projfile::setting_put("shell", Panel::shell_id(i));
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
        if (h->type == CLAP_EVENT_NOTE_ON)
            core->midi(0x90u | ch, (uint32_t)e->key, vel ? vel : 1u);
        else
            core->midi(0x80u | ch, (uint32_t)e->key, vel);
        break;
    }
    case CLAP_EVENT_MIDI:
    {
        auto *e = reinterpret_cast<const clap_event_midi_t *>(h);
        core->midi(e->data[0], e->data[1], e->data[2]);
        break;
    }
    default:
        break;
    }
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
void Tessera::param_event(const clap_output_events_t *out, uint32_t at, int i, int32_t v)
{
    if (!out)
        return;
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
    if (again)                 // (a copy an instance gave back: its power-on project again, STILL TIDE in slot A, and
    {                          // its power-on MIDI setup, not the last instance's ROUT, NOTES OUT ..)
        t->core->slot_load(0);
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
        if (!sz.empty()) t->size_user = Panel::size_halves(std::atoi(sz.c_str()));
    }
    {   // GENERATE: the style (by name) and VARIATION last picked in any window
        std::string st = projfile::setting_get("gen_style"), var = projfile::setting_get("gen_variation");
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
    else if (!t->free_started)   // a free-running host (the standalone app): play at once
    {
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
            t->core->buttons(t->win_buttons | t->fm1_buttons);
            t->core->keys(t->win_keys | t->fm1_keys);
            if (pi.master >= 0)
                t->master_set(pi.master);
            pi.master = -1;
            if (pi.track >= 0)
                t->core->track_select((uint32_t)pi.track);
            pi.track = -1;
            for (int r = 0; r < TESS_NE; r++)
                if (pi.enc[r])
                {
                    t->core->enc((uint32_t)r, pi.enc[r]);
                    pi.enc[r] = 0;
                }
            for (auto &a : pi.zones)   // the screen's zones: what the mouse did there, in order
                t->core->zone_do(&a.z, a.action, a.n);
            pi.zones.clear();          // (keeps its room: no allocation on this thread)
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
            for (int r = 0; r < TESS_NE; r++)
                if (fi.enc[r])
                    t->core->enc((uint32_t)r, fi.enc[r]);
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

    t->params_out(proc->out_events, n ? n - 1 : 0, set);   // (a setting changed on the panel, by a project or a cord's source)

    if (t->view_due <= n)   // the view for the window, ~30 times a second
    {
        std::unique_lock<std::mutex> v(t->view_lock, std::try_to_lock);
        if (v.owns_lock())
        {
            t->core->view(&t->shared_view);
            t->view_due = (uint32_t)(t->rate / 30);
        }
    }
    else
        t->view_due -= n;
    return CLAP_PROCESS_CONTINUE;
}

// ---- audio ports: main (the mix, as the FM-1's output), then TRACK 1..8 (each alone: the FM-1's USB stems)

uint32_t ap_count(const clap_plugin_t *, bool is_input) { return is_input ? 0 : 1 + NSTEMS; }
bool ap_get(const clap_plugin_t *, uint32_t index, bool is_input, clap_audio_port_info_t *info)
{
    if (is_input || index > NSTEMS)
        return false;
    info->id = index;
    if (index == 0)
        std::strncpy(info->name, "Main", sizeof(info->name));
    else
        std::snprintf(info->name, sizeof(info->name), "Track %u", index);
    info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}
const clap_plugin_audio_ports_t audio_ports = {ap_count, ap_get};

// ---- note ports: one in

uint32_t np_count(const clap_plugin_t *, bool) { return 1; }
bool np_get(const clap_plugin_t *, uint32_t index, bool is_input, clap_note_port_info_t *info)
{
    if (index != 0)
        return false;
    info->id = is_input ? 0 : 1;
    info->supported_dialects = is_input ? CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI : CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    std::strncpy(info->name, is_input ? "MIDI in" : "MIDI out", sizeof(info->name));
    return true;
}
const clap_plugin_note_ports_t note_ports = {np_count, np_get};

// ---- state: the project as a .tsp record (tracks 1..4 in the parts an FM-1 reads, tracks 5..8 in its XTRK part,
// project_tsp.c), then the device's MIDI setup (ROUT, DRUM's channel, NOTES OUT, TUNE: tess_core_setup_save, never
// in the .tsp) as its bytes, their count and "TGLO", then MASTER (the device's, not the project's) as "TMST" and 2
// bytes. A state of a plugin build
// before XTRK may also hold "TX58" (tracks 5..8's settings by central ID: the count and the tag), read as before; none
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
    std::vector<uint8_t> b(t->core->tsp_max() + 6 + 260 + 8 + TX58_ENTRY * (uint32_t)t->params.size());
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
    t->changed_by_file();
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
    t->gui = Gui::make(t->host, t);
    if (t->gui)
        t->gui->set_scale(t->size_halves());
    return t->gui != nullptr;
}
void gui_destroy(const clap_plugin_t *p) { self(p)->gui.reset(); }
// the window's size: the panel at 100, 150 or 200 % (gui.h SIZE). A host's scale (Windows, X11: the screen's DPI) picks
// the nearest until SIZE is picked; a host that lets its frame be dragged gets the nearest of the three
bool gui_set_scale(const clap_plugin_t *p, double scale)
{
    auto *t = self(p);
    int n = scale >= 1.75 ? 4 : scale >= 1.25 ? 3 : 2;
    bool was = t->size_halves() == n;
    t->size_host = n;
    if (!was && t->gui && !t->size_user)
        t->gui->set_scale(n);
    return true;
}
bool gui_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h)
{
    int n = self(p)->size_halves();
    *w = (uint32_t)(Panel::W * n / 2);
    *h = (uint32_t)(Panel::H * n / 2);
    return true;
}
bool gui_can_resize(const clap_plugin_t *) { return true; }
bool gui_get_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h)
{
    h->can_resize_horizontally = h->can_resize_vertically = true;
    h->preserve_aspect_ratio = true;
    h->aspect_ratio_width = Panel::W;
    h->aspect_ratio_height = Panel::H;
    return true;
}
int size_nearest(uint32_t w)   // (the halves whose width is nearest w)
{
    int best = 2;
    for (int n = 3; n <= 4; n++)
        if (std::abs((int)w - Panel::W * n / 2) < std::abs((int)w - Panel::W * best / 2)) best = n;
    return best;
}
bool gui_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h)
{
    int n = size_nearest(*w);
    *w = (uint32_t)(Panel::W * n / 2);
    *h = (uint32_t)(Panel::H * n / 2);
    return true;
}
bool gui_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h)
{
    for (int n = 2; n <= 4; n++)
        if (w == (uint32_t)(Panel::W * n / 2) && h == (uint32_t)(Panel::H * n / 2))
            return self(p)->size_set(n);
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
    TESS_PLUGIN_ID,
    "Tessera",
    "Tessera",
    "https://github.com/Rastapopolix/tessera",
    "",
    "https://github.com/Rastapopolix/tessera",
    TESS_VERSION_STR,
    "A pocket rabbit hole: Tessera's generative sequencer and voices",
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

