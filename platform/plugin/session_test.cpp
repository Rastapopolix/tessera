/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's session test: a DAW saves a session with three Tessera instances and opens it again, as a minimal
 * CLAP host (POSIX) over the built Tessera.clap:
 *   - three instances, each changed its own way (a track's level, a MACRO, a generator setting, MASTER, notes
 *     played), each played for a while with the host's transport;
 *   - each saves its state; all three close (the session is closed);
 *   - three new instances open in another order and each loads one state: every parameter reads as before, the
 *     host is asked to rescan the values, and saving again gives the same bytes;
 *   - the instances stay apart: what one instance changed is not in the others;
 *   - opened twice from the same state and played from the DAW's zero, two instances play the same audio.
 * usage: tessera-session-test path/to/Tessera.clap */
#include <clap/clap.h>
#include <dlfcn.h>
#include <string>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <array>
#include <vector>

static int fails = 0;
#define CHECK(c, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(c))                                                                                                      \
        {                                                                                                              \
            std::printf("FAIL: " __VA_ARGS__);                                                                         \
            std::printf("\n");                                                                                         \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

// ---- host
static int rescans_values = 0;
static void h_rescan(const clap_host_t *, clap_param_rescan_flags f)
{
    if (f & CLAP_PARAM_RESCAN_VALUES) rescans_values++;
}
static void h_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static void h_flush(const clap_host_t *) {}
static const clap_host_params_t host_params = {h_rescan, h_clear, h_flush};
static void h_dirty(const clap_host_t *) {}
static const clap_host_state_t host_state = {h_dirty};
static const void *h_get_extension(const clap_host_t *, const char *id)
{
    if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &host_params;
    if (!std::strcmp(id, CLAP_EXT_STATE)) return &host_state;
    return nullptr;
}
static void h_nop(const clap_host_t *) {}
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-session-test", "Tessera", "", "0",
                                 h_get_extension, h_nop, h_nop, h_nop};

// ---- events in (notes and parameter values, in time order), none kept out
union Ev
{
    clap_event_header_t h;
    clap_event_note_t note;
    clap_event_param_value_t param;
};
struct Events
{
    std::vector<Ev> ev;
    static uint32_t size(const clap_input_events_t *l) { return (uint32_t) static_cast<Events *>(l->ctx)->ev.size(); }
    static const clap_event_header_t *get(const clap_input_events_t *l, uint32_t i)
    {
        return &static_cast<Events *>(l->ctx)->ev[i].h;
    }
    void note(uint16_t type, int16_t key, int16_t channel = 0)
    {
        Ev e = {};
        e.note.header = {sizeof e.note, 0, CLAP_CORE_EVENT_SPACE_ID, type, 0};
        e.note.note_id = -1;
        e.note.key = key;
        e.note.channel = channel;
        e.note.velocity = 0.8;
        ev.push_back(e);
    }
    void param(clap_id id, double v)
    {
        Ev e = {};
        e.param.header = {sizeof e.param, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
        e.param.param_id = id;
        e.param.note_id = -1;
        e.param.port_index = e.param.channel = e.param.key = -1;
        e.param.value = v;
        ev.push_back(e);
    }
};
// ---- events out: the MIDI ones kept (status, data 1, data 2), the rest let go
static bool out_try_push(const clap_output_events_t *l, const clap_event_header_t *h)
{
    if (h->space_id == CLAP_CORE_EVENT_SPACE_ID && h->type == CLAP_EVENT_MIDI)
    {
        auto *m = reinterpret_cast<const clap_event_midi_t *>(h);
        static_cast<std::vector<std::array<uint8_t, 3>> *>(l->ctx)->push_back({m->data[0], m->data[1], m->data[2]});
    }
    return true;
}

// ---- streams
struct Buf
{
    std::vector<uint8_t> b;
    size_t r = 0;
};
static int64_t os_write(const clap_ostream_t *s, const void *p, uint64_t n)
{
    auto *b = static_cast<Buf *>(s->ctx);
    b->b.insert(b->b.end(), (const uint8_t *)p, (const uint8_t *)p + n);
    return (int64_t)n;
}
static int64_t is_read(const clap_istream_t *s, void *p, uint64_t n)
{
    auto *b = static_cast<Buf *>(s->ctx);
    size_t k = b->b.size() - b->r < n ? b->b.size() - b->r : (size_t)n;
    std::memcpy(p, b->b.data() + b->r, k);
    b->r += k;
    return (int64_t)k;
}

// ---- one instance, as the host keeps it
static const clap_plugin_factory_t *fac;
static const char *plugin_id;
constexpr uint32_t RATE = 48000, BLOCK = 480;
struct Inst
{
    const clap_plugin_t *p = nullptr;
    const clap_plugin_params_t *pa = nullptr;
    const clap_plugin_state_t *st = nullptr;
    std::vector<float> L = std::vector<float>(BLOCK), R = std::vector<float>(BLOCK);
    float *ch[2] = {L.data(), R.data()};
    clap_audio_buffer_t out = {};
    clap_event_transport_t tr = {};
    Events ev;
    clap_input_events_t in = {&ev, Events::size, Events::get};
    std::vector<std::array<uint8_t, 3>> midi;   // MIDI OUT so far
    clap_output_events_t oe = {&midi, out_try_push};
    clap_process_t proc = {};
    double beats = 0;

    bool open()
    {
        p = fac->create_plugin(fac, &host, plugin_id);
        if (!p || !p->init(p)) return false;
        pa = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
        st = (const clap_plugin_state_t *)p->get_extension(p, CLAP_EXT_STATE);
        out.data32 = ch;
        out.channel_count = 2;
        tr.header = {sizeof tr, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
        tr.tempo = 120.0;
        proc.frames_count = BLOCK;
        proc.transport = &tr;
        proc.audio_outputs = &out;
        proc.audio_outputs_count = 1;
        proc.in_events = &in;
        proc.out_events = &oe;
        return pa && st && p->activate(p, RATE, BLOCK, BLOCK) && p->start_processing(p);
    }
    void close()
    {
        p->stop_processing(p);
        p->deactivate(p);
        p->destroy(p);
        p = nullptr;
    }
    // blocks of audio with the DAW's transport (playing: the beats go on from where they are); the energy
    double run(uint32_t blocks, bool playing, std::vector<float> *keep = nullptr)
    {
        double e = 0;
        for (uint32_t b = 0; b < blocks; b++)
        {
            tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | (playing ? CLAP_TRANSPORT_IS_PLAYING : 0);
            tr.song_pos_beats = (clap_beattime)std::llround(beats * CLAP_BEATTIME_FACTOR);
            p->process(p, &proc);
            ev.ev.clear();
            proc.steady_time += BLOCK;
            if (playing) beats += BLOCK * tr.tempo / 60.0 / RATE;
            for (uint32_t k = 0; k < BLOCK; k++)
            {
                e += (double)L[k] * L[k] + (double)R[k] * R[k];
                if (keep) keep->push_back(L[k]), keep->push_back(R[k]);
            }
        }
        return e;
    }
    std::vector<uint8_t> save()
    {
        Buf b;
        clap_ostream_t os = {&b, os_write};
        if (!st->save(p, &os)) b.b.clear();
        return b.b;
    }
    bool load(const std::vector<uint8_t> &bytes)
    {
        Buf b;
        b.b = bytes;
        clap_istream_t is = {&b, is_read};
        return st->load(p, &is);
    }
    std::map<clap_id, double> values()
    {
        std::map<clap_id, double> m;
        for (uint32_t i = 0, n = pa->count(p); i < n; i++)
        {
            clap_param_info_t info;
            double v;
            if (pa->get_info(p, i, &info) && pa->get_value(p, info.id, &v)) m[info.id] = v;
        }
        return m;
    }
    clap_id find(const char *name)
    {
        for (uint32_t i = 0, n = pa->count(p); i < n; i++)
        {
            clap_param_info_t info;
            if (pa->get_info(p, i, &info) && !std::strcmp(info.name, name)) return info.id;
        }
        return CLAP_INVALID_ID;
    }
};

static int differ(const std::map<clap_id, double> &a, const std::map<clap_id, double> &b, const char *what)
{
    int n = 0;
    for (auto &kv : a)
    {
        auto it = b.find(kv.first);
        if (it == b.end() || it->second != kv.second)
        {
            if (n < 6) std::printf("  %s: parameter %u: %g, then %g\n", what, (unsigned)kv.first, kv.second,
                                   it == b.end() ? NAN : it->second);
            n++;
        }
    }
    return n + (int)(b.size() > a.size() ? b.size() - a.size() : 0);
}

// the library inside a plugin path: on macOS Tessera.clap is a bundle, its code in Contents/MacOS/Tessera
static std::string clap_library(const char *path)
{
#ifdef __APPLE__
    std::string p(path);
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    std::string base = p.substr(p.find_last_of('/') == std::string::npos ? 0 : p.find_last_of('/') + 1);
    std::string stem = base.substr(0, base.rfind('.'));   // (Tessera.clap: Tessera)
    std::string inner = p + "/Contents/MacOS/" + stem;
    if (FILE *f = std::fopen(inner.c_str(), "rb")) { std::fclose(f); return inner; }
#endif
    return path;
}

// the device's MIDI setup in a state (the bytes before "TGLO", ahead of "TMST"): empty if there is none
static std::vector<uint8_t> setup_of(const std::vector<uint8_t> &st)
{
    size_t n = st.size();
    if (n < 6 + 5 || std::memcmp(&st[n - 6], "TMST", 4)) return {};
    n -= 6;
    if (std::memcmp(&st[n - 4], "TGLO", 4)) return {};
    size_t ns = st[n - 5];
    return std::vector<uint8_t>(st.end() - 6 - 5 - (long)ns, st.end() - 6 - 5);
}
// the same state with its setup's bytes changed to s (the same length), or taken out (s empty)
static std::vector<uint8_t> setup_put(std::vector<uint8_t> st, const std::vector<uint8_t> &s)
{
    std::vector<uint8_t> was = setup_of(st);
    size_t at = st.size() - 6 - 5 - was.size();
    if (s.empty())
        st.erase(st.begin() + (long)at, st.end() - 6);
    else
        std::copy(s.begin(), s.end(), st.begin() + (long)at);
    return st;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s Tessera.clap\n", argv[0]);
        return 2;
    }
    void *lib = dlopen(clap_library(argv[1]).c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!lib)
    {
        std::printf("FAIL: dlopen %s: %s\n", argv[1], dlerror());
        return 1;
    }
    auto *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
    CHECK(entry && entry->init(argv[1]), "clap_entry");
    if (fails) return 1;
    fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    plugin_id = fac->get_plugin_descriptor(fac, 0)->id;

    // ---- the session: three instances, each changed its own way, each played
    const int N = 3;
    Inst a[N];
    for (int k = 0; k < N; k++)
        CHECK(a[k].open(), "instance %d opens", k + 1);
    if (fails) return 1;
    clap_id level = a[0].find("T1 LEVEL"), macro1 = a[0].find("MACRO 1"), master = a[0].find("MASTER"),
            tlen = a[0].find("T1 TURING LENGTH");
    CHECK(level != CLAP_INVALID_ID && macro1 != CLAP_INVALID_ID && master != CLAP_INVALID_ID && tlen != CLAP_INVALID_ID,
          "T1 LEVEL, MACRO 1, MASTER and T1 TURING LENGTH are parameters");
    a[0].ev.param(level, 80);
    a[1].ev.param(macro1, 63);
    a[1].ev.param(tlen, 7);
    a[2].ev.param(master, 512);
    a[2].ev.note(CLAP_EVENT_NOTE_ON, 64);
    for (int k = 0; k < N; k++)
        a[k].run(1, false);   // (the changes in, stopped)
    double e[N];
    for (int k = 0; k < N; k++)
        e[k] = a[k].run(400, true);   // 4 s playing
    std::printf("session: energy %.3f %.3f %.3f\n", e[0], e[1], e[2]);
    for (int k = 0; k < N; k++)
        CHECK(e[k] > 1e-3, "instance %d plays", k + 1);

    std::vector<uint8_t> saved[N];
    std::map<clap_id, double> was[N];
    for (int k = 0; k < N; k++)
    {
        a[k].run(1, false);   // (stopped, as a DAW saves)
        saved[k] = a[k].save();
        was[k] = a[k].values();
        CHECK(saved[k].size() > 4 && !std::memcmp(saved[k].data(), "TSP1", 4), "instance %d saves a .tsp state", k + 1);
    }
    std::printf("states: %zu %zu %zu bytes; %zu parameters\n", saved[0].size(), saved[1].size(), saved[2].size(),
                was[0].size());
    CHECK(was[0][level] == 80 && was[1][macro1] == 63 && was[1][tlen] == 7 && was[2][master] == 512,
          "each instance's own change reads back before the save");
    CHECK(was[1][level] != 80 && was[0][macro1] != 63 && was[0][master] != 512 && was[2][tlen] != 7,
          "one instance's change is not in the others");
    for (int k = 0; k < N; k++)
        a[k].close();   // the session closes

    // ---- the session opens again: new instances in another order, each with its state
    const int order[N] = {2, 0, 1};
    Inst b[N];
    for (int k = 0; k < N; k++)
        CHECK(b[k].open(), "instance %d opens again", k + 1);
    if (fails) return 1;
    for (int k = 0; k < N; k++)
    {
        int s = order[k];
        int r0 = rescans_values;
        CHECK(b[k].load(saved[s]), "state %d loads", s + 1);
        CHECK(rescans_values > r0, "state %d: the host is asked to rescan the values", s + 1);
        char what[32];
        std::snprintf(what, sizeof what, "state %d", s + 1);
        int d = differ(was[s], b[k].values(), what);
        CHECK(!d, "state %d: every parameter reads as before the save (%d differ)", s + 1, d);
        std::vector<uint8_t> again = b[k].save();
        CHECK(again == saved[s], "state %d: saving again gives the same bytes (%zu, %zu)", s + 1, again.size(),
              saved[s].size());
    }

    // ---- reproducible: the same state in two fresh instances, played from the DAW's zero, sounds the same
    {
        Inst c[2];
        std::vector<float> audio[2];
        for (int k = 0; k < 2; k++)
        {
            CHECK(c[k].open(), "a fresh instance opens");
            if (fails) return 1;
            c[k].load(saved[1]);
            c[k].run(1, false);
            c[k].run(300, true, &audio[k]);   // 3 s
        }
        size_t at = 0;
        double peak = 0;
        while (at < audio[0].size() && audio[0][at] == audio[1][at]) at++;
        for (float x : audio[0]) peak = std::fmax(peak, std::fabs(x));
        std::printf("opened twice: %s (peak %.3f)\n",
                    at == audio[0].size() ? "the same audio" : "differs", peak);
        if (at < audio[0].size())
            std::printf("  first difference at %.3f s\n", (double)(at / 2) / RATE);
        CHECK(peak > 1e-3 && at == audio[0].size(), "the same state plays the same audio from the DAW's zero");
        for (auto &x : c) x.close();
    }
    for (auto &x : b) x.close();

    // ---- MIDI in a DAW: ROUT, the setup kept in the project, MIDI OUT's channels, wildcards and restarts
    {
        Inst m;
        CHECK(m.open(), "an instance for MIDI opens");
        if (fails) return 1;
        std::vector<uint8_t> st = m.save(), su = setup_of(st);
        CHECK(su.size() >= 5 && su[0] == 1 && su[1] == 1 && su[3] == 1,
              "a fresh state keeps the MIDI setup: ROUT SEL, NOTES OUT ALL (%zu bytes)", su.size());
        m.run(10, false);
        m.ev.note(CLAP_EVENT_NOTE_ON, 60, 0);
        double sel = m.run(30, false);
        m.ev.note(CLAP_EVENT_NOTE_OFF, 60, 0);
        m.run(200, false);
        CHECK(sel > 1e-3, "ROUT SEL: a note on channel 1 plays the selected track (%.4f)", sel);
        m.close();

        // ROUT CH5-8 (2), DRUM channel 10, NOTES OUT ALL, TUNE +7: kept through a save and a reopen
        std::vector<uint8_t> want = su;
        if (want.size() >= 5) want[1] = 2, want[2] = 10, want[4] = 128 + 7;
        std::vector<uint8_t> st2 = setup_put(st, want);
        Inst r;
        CHECK(r.open(), "an instance opens again");
        if (fails) return 1;
        CHECK(r.load(st2), "a state with a changed MIDI setup loads");
        CHECK(setup_of(r.save()) == want, "the MIDI setup reads back as it was saved");
        r.run(600, false);   // (the loaded project's tail dies away)
        double quiet = r.run(30, false);
        r.ev.note(CLAP_EVENT_NOTE_ON, 60, 0);
        double ch1 = r.run(30, false);
        r.ev.note(CLAP_EVENT_NOTE_OFF, 60, 0);
        r.run(200, false);
        r.ev.note(CLAP_EVENT_NOTE_ON, 60, 4);
        double ch5 = r.run(30, false);
        r.ev.note(CLAP_EVENT_NOTE_OFF, 60, 4);
        r.run(200, false);
        CHECK(ch1 <= quiet && ch5 > 1e-3 && ch5 > 100 * quiet,
              "ROUT CH5-8 after a reopen: channel 1 silent (%.6f, the tail %.6f), channel 5 plays (%.4f)", ch1, quiet,
              ch5);

        // MIDI OUT while the DAW plays: only notes, each track on its own channel 1..8 whatever ROUT takes in; a
        // restart of processing ends every note it left on
        r.midi.clear();
        r.run(600, true);   // 6 s
        int notes = 0, system = 0, high = 0;
        for (auto &e : r.midi)
        {
            if ((e[0] & 0xF0) == 0xF0) system++;
            else if ((e[0] & 0xF0) == 0x90 || (e[0] & 0xF0) == 0x80) notes++, high += (e[0] & 15) >= 8;
        }
        CHECK(notes > 0 && !system && !high, "MIDI OUT: %d notes, %d on channels past 8, %d system messages", notes,
              high, system);
        std::set<int> on;
        for (auto &e : r.midi)
        {
            int k = (e[0] & 15) << 7 | e[1];
            if ((e[0] & 0xF0) == 0x90 && e[2]) on.insert(k);
            else if ((e[0] & 0xF0) == 0x80 || (e[0] & 0xF0) == 0x90) on.erase(k);
        }
        int held = 0, left = 0;
        for (int tries = 0; tries < 100 && on.empty(); tries++)   // (a moment with a note on)
        {
            r.midi.clear();
            r.run(1, true);
            for (auto &e : r.midi)
            {
                int k = (e[0] & 15) << 7 | e[1];
                if ((e[0] & 0xF0) == 0x90 && e[2]) on.insert(k);
                else on.erase(k);
            }
        }
        held = (int)on.size();
        r.p->stop_processing(r.p);
        r.p->start_processing(r.p);
        r.midi.clear();
        r.run(1, true);
        for (int k : on)
        {
            bool off = false;
            for (auto &e : r.midi)
                if (((e[0] & 15) << 7 | e[1]) == k && ((e[0] & 0xF0) == 0x80 || ((e[0] & 0xF0) == 0x90 && !e[2])))
                    off = true;
            left += !off;
        }
        CHECK(held > 0 && !left, "a restart of processing: %d notes were on, %d left on", held, left);
        r.close();

        // a state from before TGLO keeps the instance's own setup (here a fresh one's: ROUT SEL)
        Inst o;
        CHECK(o.open(), "an instance opens for an older state");
        if (fails) return 1;
        CHECK(o.load(setup_put(st2, {})), "a state without the MIDI setup loads");
        CHECK(setup_of(o.save()) == su, "with no MIDI setup in the state, the instance keeps its own");

        // a note-off for every key (key -1) ends a held note as its own note-off does: 3 s after, the same tail, and
        // far less than a note still held
        double after[3];
        o.run(600, false);   // (the loaded project's tail)
        for (int w = 0; w < 3; w++)
        {
            o.ev.note(CLAP_EVENT_NOTE_ON, 60, 0);
            o.run(50, false);
            if (w < 2) o.ev.note(CLAP_EVENT_NOTE_OFF, w ? -1 : 60, w ? -1 : 0);
            o.run(300, false);
            after[w] = o.run(20, false);
            if (w == 2) o.ev.note(CLAP_EVENT_NOTE_OFF, 60, 0);
            o.run(900, false);
        }
        std::printf("note-offs: its own %.6f, the wildcard %.6f, none %.6f\n", after[0], after[1], after[2]);
        CHECK(after[2] > 1e-3 && after[0] < after[2] * 0.1 && after[1] < after[2] * 0.1,
              "a wildcard note-off ends a held note as its own note-off does");
        o.close();
    }

    entry->deinit();
    std::printf(fails ? "session test: %d FAILED\n" : "session test: ok\n", fails);
    return fails ? 1 : 0;
}
