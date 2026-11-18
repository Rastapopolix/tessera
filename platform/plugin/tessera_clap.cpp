/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera as a CLAP instrument (phase 0 spike). clap-wrapper turns the same code into VST3 and a standalone app
 * (CMakeLists.txt). What it does:
 *   - one stereo out; the core runs at 44.1 kHz (tess_core.h) and is resampled (linear) to the host's rate;
 *   - notes and MIDI in (CLAP or MIDI dialect) go to the core as USB MIDI does on the FM-1, at their sample;
 *   - the host's transport starts and stops the song; with no transport (the standalone app) it plays at once;
 *   - the plugin's state is the project as a .tsp record, the same bytes SAVE > PROJECT writes on the FM-1.
 * One instance per process: the firmware's state is global (a second instance is refused). */
#include <clap/clap.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

#include "tess_core.h"

namespace
{
std::mutex core_lock;                     // the core is not thread-safe: audio, state and boot all take it
std::atomic<bool> instance_live{false};   // one instance at a time
bool core_booted = false;

struct Tessera
{
    clap_plugin_t plugin{};
    const clap_host_t *host = nullptr;
    double rate = 48000.0;
    double step = 44100.0 / 48000.0;      // core frames per host frame
    double pos = 0.0;                     // between prev and cur, 0..1
    float prev_l = 0, prev_r = 0, cur_l = 0, cur_r = 0;
    float buf_l[64], buf_r[64];
    uint32_t buf_n = 0, buf_i = 0;
    bool transport_known = false, was_playing = false, free_started = false;

    void next_core_frame()
    {
        if (buf_i >= buf_n)
        {
            tess_core_render(buf_l, buf_r, 64);
            buf_n = 64;
            buf_i = 0;
        }
        prev_l = cur_l;
        prev_r = cur_r;
        cur_l = buf_l[buf_i];
        cur_r = buf_r[buf_i];
        buf_i++;
    }

    void render(float *l, float *r, uint32_t n)
    {
        for (uint32_t k = 0; k < n; k++)
        {
            while (pos >= 1.0)
            {
                next_core_frame();
                pos -= 1.0;
            }
            float f = (float)pos;
            if (l) l[k] = prev_l + (cur_l - prev_l) * f;
            if (r) r[k] = prev_r + (cur_r - prev_r) * f;
            pos += step;
        }
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
    return true;
}

void p_destroy(const clap_plugin_t *p)
{
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
    float *l = nullptr, *r = nullptr;
    if (proc->audio_outputs_count > 0 && proc->audio_outputs[0].data32)
    {
        l = proc->audio_outputs[0].data32[0];
        r = proc->audio_outputs[0].channel_count > 1 ? proc->audio_outputs[0].data32[1] : nullptr;
    }

    std::unique_lock<std::mutex> g(core_lock, std::try_to_lock);
    if (!g.owns_lock())   // the main thread is saving or loading the project: a block of silence
    {
        if (l) std::memset(l, 0, n * sizeof(float));
        if (r) std::memset(r, 0, n * sizeof(float));
        return CLAP_PROCESS_CONTINUE;
    }

    if (proc->transport)
    {
        bool playing = (proc->transport->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
        if (!t->transport_known || playing != t->was_playing)
            tess_core_transport(playing);
        t->transport_known = true;
        t->was_playing = playing;
    }
    else if (!t->free_started)   // a free-running host (the standalone app): play at once
    {
        tess_core_transport(1);
        t->free_started = true;
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
            t->render(l ? l + done : nullptr, r ? r + done : nullptr, at - done);
            done = at;
        }
        midi_in(h);
    }
    if (done < n)
        t->render(l ? l + done : nullptr, r ? r + done : nullptr, n - done);
    if (l && !r && proc->audio_outputs[0].channel_count > 1)
        std::memcpy(proc->audio_outputs[0].data32[1], l, n * sizeof(float));
    return CLAP_PROCESS_CONTINUE;
}

// ---- audio ports: one stereo out

uint32_t ap_count(const clap_plugin_t *, bool is_input) { return is_input ? 0 : 1; }
bool ap_get(const clap_plugin_t *, uint32_t index, bool is_input, clap_audio_port_info_t *info)
{
    if (is_input || index != 0)
        return false;
    info->id = 0;
    std::strncpy(info->name, "Main", sizeof(info->name));
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}
const clap_plugin_audio_ports_t audio_ports = {ap_count, ap_get};

// ---- note ports: one in

uint32_t np_count(const clap_plugin_t *, bool is_input) { return is_input ? 1 : 0; }
bool np_get(const clap_plugin_t *, uint32_t index, bool is_input, clap_note_port_info_t *info)
{
    if (!is_input || index != 0)
        return false;
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    std::strncpy(info->name, "MIDI in", sizeof(info->name));
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
    return tess_core_load(b.data(), len) == 0;
}
const clap_plugin_state_t state = {st_save, st_load};

const void *p_get_extension(const clap_plugin_t *, const char *id)
{
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
