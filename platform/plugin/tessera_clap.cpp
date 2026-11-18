/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera as a CLAP instrument (phase 0 spike). clap-wrapper turns the same code into VST3 and a standalone app
 * (CMakeLists.txt). What it does:
 *   - outputs: the main mix, then TRACK 1..4 alone (the FM-1's USB stems); a MIDI out with every track's notes on its
 *     own channel (Settings > NOTES OUT ALL by default here; GLO > MIDI OUT per track);
 *   - the core runs at 44.1 kHz (tess_core.h) and is resampled (linear) to the host's rate;
 *   - notes and MIDI in (CLAP or MIDI dialect) go to the core as USB MIDI does on the FM-1, at their sample;
 *   - the host's transport drives the song: PLAY starts every track where it would be from the DAW's zero, the DAW's
 *     beat position drives Tessera's clock (CLK INT), a loop or a locate seeks, STOP stops; with no transport (the
 *     standalone app) it plays at once on its own clock;
 *   - parameters (automatable): MASTER and PERF's MACRO 1..4; turned on the panel, they are sent back to the host;
 *   - the plugin's state is the project as a .tsp record, the same bytes SAVE > PROJECT writes on the FM-1;
 *   - the window (gui.h, Windows and Linux) shows the FM-1: its screen, buttons, keys and knobs. The audio thread owns
 *     the core: the window's input goes to it through a small queue, and it hands back a view ~30 times a second.
 * One instance per process: the firmware's state is global (a second instance is refused). */
#include <clap/clap.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>

#include "gui.h"
#include "tess_core.h"

namespace
{
std::mutex core_lock;                     // the core is not thread-safe: audio, state and boot all take it
std::atomic<bool> instance_live{false};   // one instance at a time
bool core_booted = false;

// the parameters: MASTER (0..1023) and MACRO 1..4 (0..100), as the core has them (the audio thread writes)
enum { PARAM_MASTER, PARAM_MACRO1, PARAM_COUNT = PARAM_MACRO1 + 4 };
std::atomic<int32_t> param_now[PARAM_COUNT];

// the window <-> the audio thread (one instance, so one of each)
std::mutex input_lock, view_lock;
PanelInput pending;           // input_lock: buttons, keys and MASTER as now, the detents summed
tess_view_t shared_view{};    // view_lock: the latest view

constexpr uint32_t NCH = 6;   // main L, R, then tracks 1..4 (the core's stems)
constexpr uint32_t NSTEMS = 4;

struct Tessera
{
    clap_plugin_t plugin{};
    const clap_host_t *host = nullptr;
    double rate = 48000.0;
    double step = 44100.0 / 48000.0;      // core frames per host frame
    double pos = 0.0;                     // between prev and cur, 0..1
    float prev[NCH] = {}, cur[NCH] = {};
    float buf[NCH][64];
    uint32_t buf_n = 0, buf_i = 0;
    uint32_t cur_frame = 0;               // the core frame in cur (tess_core_frames' count)
    bool stems = false;                   // a track output is active: the core makes the stems
    bool transport_known = false, was_playing = false, free_started = false;
    uint32_t view_due = 0;               // host frames until the next view
    std::vector<std::pair<uint32_t, uint32_t>> midi_wait;   // MIDI OUT packets (packet, core frame) not yet due
    std::unique_ptr<Gui> gui;

    void next_core_frame()
    {
        if (buf_i >= buf_n)
        {
            float *st[NSTEMS] = {buf[2], buf[3], buf[4], buf[5]};
            tess_core_render(buf[0], buf[1], stems ? st : nullptr, 64);
            if (!stems)
                for (uint32_t c = 2; c < NCH; c++) std::memset(buf[c], 0, sizeof buf[c]);
            buf_n = 64;
            buf_i = 0;
        }
        for (uint32_t c = 0; c < NCH; c++)
        {
            prev[c] = cur[c];
            cur[c] = buf[c][buf_i];
        }
        cur_frame = tess_core_frames() - buf_n + buf_i;
        buf_i++;
    }

    // n host frames into out[c] + at (null: a channel nobody listens to)
    void render(float *const *out, uint32_t at, uint32_t n)
    {
        for (uint32_t k = at; k < at + n; k++)
        {
            while (pos >= 1.0)
            {
                next_core_frame();
                pos -= 1.0;
            }
            float f = (float)pos;
            for (uint32_t c = 0; c < NCH; c++)
                if (out[c]) out[c][k] = prev[c] + (cur[c] - prev[c]) * f;
            pos += step;
        }
    }

    // MIDI OUT for a block of n host frames that ended with cur_frame: each packet at the host frame its core frame plays
    void midi_out(const clap_output_events_t *out, uint32_t n)
    {
        uint32_t pkt, frame;
        while (tess_core_midi_out(&pkt, &frame))
            midi_wait.emplace_back(pkt, frame);
        size_t keep = 0;
        for (auto &m : midi_wait)
        {
            // host frames after this block's last one that core frame m.second plays at (cur_frame: the last)
            double ahead = (double)(int32_t)(m.second - cur_frame) / step;
            if (ahead > 0.5)
            {
                midi_wait[keep++] = m;   // (not yet: the next block)
                continue;
            }
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
};

Tessera *self(const clap_plugin_t *p) { return static_cast<Tessera *>(p->plugin_data); }

void midi_in(const clap_event_header_t *h)
{
    if (h->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;
    switch (h->type)
    {
    case CLAP_EVENT_NOTE_ON:
    case CLAP_EVENT_NOTE_OFF:
    {
        auto *e = reinterpret_cast<const clap_event_note_t *>(h);
        if (e->key < 0 || e->key > 127)
            return;
        uint32_t ch = e->channel < 0 ? 0u : (uint32_t)e->channel & 15u;
        uint32_t vel = (uint32_t)(e->velocity * 127.0 + 0.5);
        if (h->type == CLAP_EVENT_NOTE_ON)
            tess_core_midi(0x90u | ch, (uint32_t)e->key, vel ? vel : 1u);
        else
            tess_core_midi(0x80u | ch, (uint32_t)e->key, vel);
        break;
    }
    case CLAP_EVENT_MIDI:
    {
        auto *e = reinterpret_cast<const clap_event_midi_t *>(h);
        tess_core_midi(e->data[0], e->data[1], e->data[2]);
        break;
    }
    default:
        break;
    }
}

// ---- parameters (audio thread: the core is locked)

int32_t param_core(int id)
{
    return id == PARAM_MASTER ? (int32_t)tess_core_master_get() : (int32_t)tess_core_macro((uint32_t)(id - PARAM_MACRO1));
}
void param_in(const clap_event_param_value_t *e)
{
    int id = (int)e->param_id;
    if (id < 0 || id >= PARAM_COUNT)
        return;
    int32_t v = (int32_t)std::lround(e->value);
    if (id == PARAM_MASTER)
        tess_core_master((uint32_t)(v < 0 ? 0 : v > 1023 ? 1023 : v));
    else
        tess_core_macro_set((uint32_t)(id - PARAM_MACRO1), (uint32_t)(v < 0 ? 0 : v));
    param_now[id] = param_core(id);
}
void params_out(const clap_output_events_t *out, uint32_t at)
{
    for (int id = 0; id < PARAM_COUNT; id++)
    {
        int32_t v = param_core(id);
        if (v == param_now[id])
            continue;
        param_now[id] = v;
        if (!out)
            continue;
        clap_event_param_value_t e = {};
        e.header = {sizeof e, at, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
        e.param_id = (clap_id)id;
        e.cookie = nullptr;
        e.note_id = -1;
        e.port_index = -1;
        e.channel = -1;
        e.key = -1;
        e.value = v;
        out->try_push(out, &e.header);
    }
}

// ---- plugin

bool p_init(const clap_plugin_t *p)
{
    (void)p;
    std::lock_guard<std::mutex> g(core_lock);
    if (!core_booted)
    {
        tess_core_boot();
        core_booted = true;
    }
    for (int id = 0; id < PARAM_COUNT; id++)
        param_now[id] = id == PARAM_MASTER ? 700 : param_core(id);
    return true;
}

void p_destroy(const clap_plugin_t *p)
{
    self(p)->gui.reset();
    delete self(p);
    instance_live = false;
}

bool p_activate(const clap_plugin_t *p, double rate, uint32_t, uint32_t)
{
    auto *t = self(p);
    t->rate = rate;
    t->step = (double)tess_core_rate() / rate;
    t->pos = 1.0;
    t->buf_n = t->buf_i = 0;
    t->transport_known = t->free_started = false;
    t->view_due = 0;
    {
        std::lock_guard<std::mutex> g(input_lock);   // MASTER where the window's knob starts
        pending.changed = true;
    }
    return true;
}

void p_deactivate(const clap_plugin_t *) {}
bool p_start_processing(const clap_plugin_t *) { return true; }
void p_stop_processing(const clap_plugin_t *) {}
void p_reset(const clap_plugin_t *p) { self(p)->pos = 1.0; }

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

    std::unique_lock<std::mutex> g(core_lock, std::try_to_lock);
    if (!g.owns_lock())   // the main thread is saving or loading the project: a block of silence
    {
        for (uint32_t port = 0; port < proc->audio_outputs_count; port++)
            for (uint32_t c = 0; c < proc->audio_outputs[port].channel_count; c++)
                if (proc->audio_outputs[port].data32)
                    std::memset(proc->audio_outputs[port].data32[c], 0, n * sizeof(float));
        return CLAP_PROCESS_CONTINUE;
    }

    if (proc->transport)
    {
        const clap_event_transport_t *tr = proc->transport;
        bool playing = (tr->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
        bool has_pos = (tr->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) && (tr->flags & CLAP_TRANSPORT_HAS_TEMPO);
        if (playing || (t->transport_known && t->was_playing))   // (stopped: once, so the panel's PLAY still works)
            tess_core_host(playing, has_pos, tr->tempo, (double)tr->song_pos_beats / CLAP_BEATTIME_FACTOR);
        t->transport_known = true;
        t->was_playing = playing;
    }
    else if (!t->free_started)   // a free-running host (the standalone app): play at once
    {
        tess_core_transport(1);
        t->free_started = true;
    }

    {   // the window's input, if the window is not handing some in right now
        std::unique_lock<std::mutex> in(input_lock, std::try_to_lock);
        if (in.owns_lock() && pending.changed)
        {
            tess_core_buttons(pending.buttons);
            tess_core_keys(pending.keys);
            if (pending.master >= 0)
                tess_core_master((uint32_t)pending.master);
            pending.master = -1;
            for (int r = 0; r < TESS_NE; r++)
                if (pending.enc[r])
                {
                    tess_core_enc((uint32_t)r, pending.enc[r]);
                    pending.enc[r] = 0;
                }
            pending.changed = false;
        }
    }

    // events at their sample: render up to each, then hand it in
    uint32_t done = 0;
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
            param_in(reinterpret_cast<const clap_event_param_value_t *>(h));
        else
            midi_in(h);
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

    params_out(proc->out_events, n ? n - 1 : 0);   // (MASTER or a MACRO turned on the panel or by a cord's source)

    if (t->view_due <= n)   // the view for the window, ~30 times a second
    {
        std::unique_lock<std::mutex> v(view_lock, std::try_to_lock);
        if (v.owns_lock())
        {
            tess_core_view(&shared_view);
            t->view_due = (uint32_t)(t->rate / 30);
        }
    }
    else
        t->view_due -= n;
    return CLAP_PROCESS_CONTINUE;
}

// ---- audio ports: main (the mix, as the FM-1's output), then TRACK 1..4 (each alone: the FM-1's USB stems)

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

// ---- state: the project as a .tsp record

bool st_save(const clap_plugin_t *, const clap_ostream_t *s)
{
    std::vector<uint8_t> b(tess_core_tsp_max());
    uint32_t len;
    {
        std::lock_guard<std::mutex> g(core_lock);
        len = tess_core_save(b.data(), (uint32_t)b.size());
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

bool st_load(const clap_plugin_t *, const clap_istream_t *s)
{
    std::vector<uint8_t> b(tess_core_tsp_max());
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
    std::lock_guard<std::mutex> g(core_lock);
    bool ok = tess_core_load(b.data(), len) == 0;
    for (int id = PARAM_MACRO1; id < PARAM_COUNT; id++)   // (the project's MACROs; the host rescans)
        param_now[id] = param_core(id);
    return ok;
}
const clap_plugin_state_t state = {st_save, st_load};

// ---- the parameters' extension

uint32_t pr_count(const clap_plugin_t *) { return PARAM_COUNT; }
bool pr_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info)
{
    if (i >= PARAM_COUNT)
        return false;
    std::memset(info, 0, sizeof *info);
    info->id = i;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
    if (i == PARAM_MASTER)
    {
        std::strncpy(info->name, "MASTER", sizeof info->name);
        info->max_value = 1023;
        info->default_value = 700;
    }
    else
    {
        std::snprintf(info->name, sizeof info->name, "MACRO %u", i - PARAM_MACRO1 + 1);
        std::strncpy(info->module, "PERF", sizeof info->module);
        info->max_value = 100;
    }
    return true;
}
bool pr_value(const clap_plugin_t *, clap_id id, double *v)
{
    if (id >= PARAM_COUNT)
        return false;
    *v = param_now[id];
    return true;
}
bool pr_to_text(const clap_plugin_t *, clap_id id, double v, char *out, uint32_t size)
{
    if (id >= PARAM_COUNT)
        return false;
    if (id == PARAM_MASTER)
        std::snprintf(out, size, "%d %%", (int)std::lround(v * 100 / 1023));
    else
        std::snprintf(out, size, "%d", (int)std::lround(v));
    return true;
}
bool pr_from_text(const clap_plugin_t *, clap_id id, const char *text, double *v)
{
    if (id >= PARAM_COUNT)
        return false;
    double x = std::atof(text);
    *v = id == PARAM_MASTER ? x * 1023 / 100 : x;
    return true;
}
void pr_flush(const clap_plugin_t *, const clap_input_events_t *in, const clap_output_events_t *out)
{
    std::lock_guard<std::mutex> g(core_lock);   // (not processing: the main thread, or the audio thread between blocks)
    uint32_t n = in ? in->size(in) : 0;
    for (uint32_t i = 0; i < n; i++)
    {
        const clap_event_header_t *h = in->get(in, i);
        if (h->space_id == CLAP_CORE_EVENT_SPACE_ID && h->type == CLAP_EVENT_PARAM_VALUE)
            param_in(reinterpret_cast<const clap_event_param_value_t *>(h));
    }
    params_out(out, 0);
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
    t->gui = Gui::make(t->host);
    return t->gui != nullptr;
}
void gui_destroy(const clap_plugin_t *p) { self(p)->gui.reset(); }
bool gui_set_scale(const clap_plugin_t *, double) { return false; }   // (fixed size, 1:1)
bool gui_get_size(const clap_plugin_t *, uint32_t *w, uint32_t *h)
{
    *w = Panel::W;
    *h = Panel::H;
    return true;
}
bool gui_can_resize(const clap_plugin_t *) { return false; }
bool gui_get_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *) { return false; }
bool gui_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h)
{
    *w = Panel::W;
    *h = Panel::H;
    return true;
}
bool gui_set_size(const clap_plugin_t *, uint32_t w, uint32_t h) { return w == Panel::W && h == Panel::H; }
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
    return nullptr;
}

void p_on_main_thread(const clap_plugin_t *) {}

// ---- factory

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER,
                          CLAP_PLUGIN_FEATURE_STEREO, nullptr};
const clap_plugin_descriptor_t desc = {
    CLAP_VERSION_INIT,
    "io.github.rastapopolix.tessera",
    "Tessera",
    "Tessera",
    "https://github.com/Rastapopolix/tessera",
    "",
    "https://github.com/Rastapopolix/tessera",
    "0.0.1",
    "A pocket rabbit hole: Tessera's generative sequencer and voices (spike)",
    features,
};

uint32_t f_count(const clap_plugin_factory_t *) { return 1; }
const clap_plugin_descriptor_t *f_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &desc : nullptr; }
const clap_plugin_t *f_create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id)
{
    if (!clap_version_is_compatible(host->clap_version) || std::strcmp(id, desc.id))
        return nullptr;
    bool expected = false;
    if (!instance_live.compare_exchange_strong(expected, true))
        return nullptr;   // one Tessera per process (tess_core.h)
    auto *t = new Tessera();
    t->host = host;
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
    return !std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? &factory : nullptr;
}

// ---- gui.h: the window's side of the queue
void tess_gui_view(tess_view_t &v)
{
    std::lock_guard<std::mutex> g(view_lock);
    v = shared_view;
}
void tess_gui_input(PanelInput &in)
{
    std::lock_guard<std::mutex> g(input_lock);
    pending.buttons = in.buttons;
    pending.keys = in.keys;
    if (in.master >= 0)
        pending.master = in.master;
    in.master = -1;
    for (int r = 0; r < TESS_NE; r++)
    {
        pending.enc[r] += in.enc[r];
        in.enc[r] = 0;
    }
    pending.changed = true;
    in.changed = false;
}
