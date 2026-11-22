/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's render test: a minimal CLAP host (POSIX) that loads the built Tessera.clap and checks
 *   - one instance: a second create is refused while the first lives;
 *   - with the host's transport playing at 48 kHz, STILL TIDE (the project it starts with) makes sound;
 *   - a note in on the selected track changes the output;
 *   - the state is a .tsp record (TSP1) and loads back;
 *   - the four track outputs sound, MIDI OUT sends the tracks' notes on their channels, in order;
 *   - the parameters: MASTER and MACRO 1..4, automation in at its sample, the value read back;
 *   - (Linux, with a DISPLAY, e.g. under xvfb-run) the window opens in a parent window and shows the FM-1's screen;
 *     the host sizes it to 200 % (a dragged frame snapped to the nearest of 100, 150, 200 %) and back;
 *     a click on GENERATE makes a new piece (the parameters change), held it brings back the project before it;
 *     (macOS: built as Objective-C++) the window opens in an NSWindow's view and draws the FM-1's screen into its layer;
 *   - optionally writes the first seconds as a 16-bit WAV (third argument) to listen to.
 * usage: tessera-render-test path/to/Tessera.clap [seconds] [out.wav] [window.ppm] */
#include <clap/clap.h>
#include <dlfcn.h>
#include <string>
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
static int callbacks = 0, rescans_text = 0;   // (the plugin asked for its main thread; the texts rescanned)
static void h_rescan(const clap_host_t *, clap_param_rescan_flags f)
{
    if (f & CLAP_PARAM_RESCAN_TEXT) rescans_text++;
}
static void h_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static void h_flush(const clap_host_t *) {}
static const clap_host_params_t host_params = {h_rescan, h_clear, h_flush};
static const void *h_get_extension(const clap_host_t *, const char *id)
{
    return !std::strcmp(id, CLAP_EXT_PARAMS) ? &host_params : nullptr;
}
static void h_nop(const clap_host_t *) {}
static void h_callback(const clap_host_t *) { callbacks++; }
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-render-test", "Tessera", "", "0",
                                 h_get_extension, h_nop, h_nop, h_callback};

// ---- events
struct Events
{
    std::vector<clap_event_note_t> notes;
    static uint32_t size(const clap_input_events_t *l) { return (uint32_t) static_cast<Events *>(l->ctx)->notes.size(); }
    static const clap_event_header_t *get(const clap_input_events_t *l, uint32_t i)
    {
        return &static_cast<Events *>(l->ctx)->notes[i].header;
    }
};
// what the plugin sends: MIDI OUT's note-ons by channel, and the events out of order (there must be none)
static uint32_t midi_on[16], out_of_order, out_last, out_frames;
static bool out_try_push(const clap_output_events_t *, const clap_event_header_t *h)
{
    if (h->time < out_last) out_of_order++;
    if (h->time >= out_frames) out_of_order++;
    out_last = h->time;
    if (h->type == CLAP_EVENT_MIDI)
    {
        auto *e = reinterpret_cast<const clap_event_midi_t *>(h);
        if ((e->data[0] & 0xF0) == 0x90 && e->data[2]) midi_on[e->data[0] & 15]++;
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

static void wav_write(const char *path, const std::vector<float> &l, const std::vector<float> &r, uint32_t rate)
{
    FILE *f = std::fopen(path, "wb");
    if (!f)
        return;
    uint32_t n = (uint32_t)l.size(), data = n * 4, v;
    uint16_t s;
    std::fwrite("RIFF", 1, 4, f);
    v = 36 + data; std::fwrite(&v, 4, 1, f);
    std::fwrite("WAVEfmt ", 1, 8, f);
    v = 16; std::fwrite(&v, 4, 1, f);
    s = 1; std::fwrite(&s, 2, 1, f);
    s = 2; std::fwrite(&s, 2, 1, f);
    std::fwrite(&rate, 4, 1, f);
    v = rate * 4; std::fwrite(&v, 4, 1, f);
    s = 4; std::fwrite(&s, 2, 1, f);
    s = 16; std::fwrite(&s, 2, 1, f);
    std::fwrite("data", 1, 4, f);
    std::fwrite(&data, 4, 1, f);
    for (uint32_t k = 0; k < n; k++)
        for (float x : {l[k], r[k]})
        {
            float c = x > 1.f ? 1.f : x < -1.f ? -1.f : x;
            int16_t q = (int16_t)std::lrint(c * 32767.f);
            std::fwrite(&q, 2, 1, f);
        }
    std::fclose(f);
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

int main(int argc, char **argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);   // (every line out before a crash, in CI's log too)
    if (argc < 2)
    {
        std::printf("usage: %s Tessera.clap [seconds] [out.wav]\n", argv[0]);
        return 2;
    }
    double seconds = argc > 2 ? std::atof(argv[2]) : 12.0;
    const uint32_t rate = 48000, block = 480;

    void *lib = dlopen(clap_library(argv[1]).c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!lib)
    {
        std::printf("FAIL: dlopen %s: %s\n", argv[1], dlerror());
        return 1;
    }
    auto *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
    CHECK(entry && entry->init(argv[1]), "clap_entry");
    if (fails)
        return 1;
    auto *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    CHECK(fac && fac->get_plugin_count(fac) == 1, "factory");
    const char *id = fac->get_plugin_descriptor(fac, 0)->id;

    const clap_plugin_t *p = fac->create_plugin(fac, &host, id);
    CHECK(p && p->init(p), "create + init");
    if (fails)
        return 1;
    // several instances: each its own copy of the core (core_copies.h), 8 at once, the 9th refused
    std::vector<const clap_plugin_t *> more;
    for (const clap_plugin_t *q; more.size() < 32 && (q = fac->create_plugin(fac, &host, id));)
        more.push_back(q);
    std::printf("instances: %zu at once\n", 1 + more.size());
    CHECK(more.size() == 7, "8 instances at once, the 9th refused (%zu)", 1 + more.size());
    const clap_plugin_t *q = more.empty() ? nullptr : more[0];
    CHECK(q && q->init(q) && q->activate(q, rate, block, block) && q->start_processing(q), "a second instance starts");
    CHECK(p->activate(p, rate, block, block), "activate");
    CHECK(p->start_processing(p), "start_processing");

    std::vector<float> L(block), R(block), allL, allR;
    float *chans[2] = {L.data(), R.data()};
    clap_audio_buffer_t out = {};
    out.data32 = chans;
    out.channel_count = 2;
    clap_event_transport_t tr = {};
    tr.header = {sizeof tr, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
    tr.flags = CLAP_TRANSPORT_IS_PLAYING | CLAP_TRANSPORT_HAS_TEMPO;
    tr.tempo = 120.0;
    Events ev;
    clap_input_events_t in = {&ev, Events::size, Events::get};
    clap_output_events_t oe = {nullptr, out_try_push};
    clap_process_t proc = {};
    proc.steady_time = 0;
    proc.frames_count = block;
    proc.transport = &tr;
    proc.audio_outputs = &out;
    proc.audio_outputs_count = 1;
    out_frames = block;
    proc.in_events = &in;
    proc.out_events = &oe;

    uint32_t blocks = (uint32_t)(seconds * rate / block);
    double sum2 = 0, peak = 0;
    for (uint32_t b = 0; b < blocks; b++)
    {
        CHECK(p->process(p, &proc) == CLAP_PROCESS_CONTINUE, "process");
        proc.steady_time += block;
        for (uint32_t k = 0; k < block; k++)
        {
            CHECK(std::isfinite(L[k]) && std::isfinite(R[k]), "finite output");
            if (b >= blocks / 4)
            {
                sum2 += (double)L[k] * L[k] + (double)R[k] * R[k];
                peak = std::fmax(peak, std::fabs(L[k]));
            }
        }
        allL.insert(allL.end(), L.begin(), L.end());
        allR.insert(allR.end(), R.begin(), R.end());
        if (fails)
            break;
    }
    if (q)   // the second instance, stopped, beside the first playing: silent, and the first goes on sounding
    {
        std::vector<float> qL(block), qR(block);
        float *qch[2] = {qL.data(), qR.data()};
        clap_audio_buffer_t qout = {};
        qout.data32 = qch;
        qout.channel_count = 2;
        clap_event_transport_t qtr = tr;
        qtr.flags = CLAP_TRANSPORT_HAS_TEMPO;   // (stopped)
        Events qev;
        clap_input_events_t qin = {&qev, Events::size, Events::get};
        clap_process_t qp = proc;
        qp.transport = &qtr;
        qp.audio_outputs = &qout;
        qp.in_events = &qin;
        qp.out_events = nullptr;
        double eq = 0, ep = 0;
        for (int b = 0; b < 100; b++)
        {
            CHECK(q->process(q, &qp) == CLAP_PROCESS_CONTINUE, "second instance process");
            p->process(p, &proc);
            for (uint32_t k = 0; k < block; k++)
                eq += (double)qL[k] * qL[k] + (double)qR[k] * qR[k], ep += (double)L[k] * L[k] + (double)R[k] * R[k];
        }
        std::printf("two instances: energy %.4f playing, %.6f stopped\n", ep, eq);
        CHECK(ep > 1e-3 && eq < 1e-9, "the second instance, stopped, is silent while the first plays");
        q->stop_processing(q);
        q->deactivate(q);
    }
    for (auto *x : more)
        x->destroy(x);

    double rms = std::sqrt(sum2 / (2.0 * (blocks - blocks / 4) * block));
    std::printf("STILL TIDE at %u Hz, %.1f s: rms %.4f, peak %.4f\n", rate, seconds, rms, peak);
    CHECK(rms > 0.002, "STILL TIDE should be audible (rms %.5f)", rms);
    CHECK(peak < 1.0, "no clipping (peak %.4f)", peak);

    // a note in: the selected track plays it
    std::vector<float> withNote;
    {
        clap_event_note_t on = {};
        on.header = {sizeof on, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_ON, 0};
        on.note_id = -1;
        on.port_index = 0;
        on.channel = 0;
        on.key = 60;
        on.velocity = 1.0;
        tr.flags = CLAP_TRANSPORT_HAS_TEMPO;   // stopped: only the note sounds
        for (int b = 0; b < 600; b++)   // 6 s for the song's tails to die
            p->process(p, &proc);
        double quiet = 0, loud = 0;
        for (int b = 0; b < 20; b++)
        {
            p->process(p, &proc);
            for (uint32_t k = 0; k < block; k++) quiet += (double)L[k] * L[k];
        }
        ev.notes.push_back(on);
        p->process(p, &proc);
        ev.notes.clear();
        for (int b = 0; b < 20; b++)
        {
            p->process(p, &proc);
            for (uint32_t k = 0; k < block; k++) loud += (double)L[k] * L[k];
        }
        std::printf("note in: energy %.4f stopped, %.4f with C4\n", quiet, loud);
        CHECK(loud > quiet * 4 + 1e-4, "a note in should sound");
        clap_event_note_t off = on;
        off.header.type = CLAP_EVENT_NOTE_OFF;
        ev.notes.push_back(off);
        p->process(p, &proc);
        ev.notes.clear();
    }

    // the track outputs and MIDI OUT: 8 s of STILL TIDE from the top with all nine outputs (main + 8 tracks); then,
    // stopped, a note on MIDI channel 6 sounds on the selected track's output alone (ROUT SEL, the plugin's default:
    // track 1; ROUT's channel ranges are the session test's)
    {
        constexpr int NT = 8;
        auto *ap = (const clap_plugin_audio_ports_t *)p->get_extension(p, CLAP_EXT_AUDIO_PORTS);
        CHECK(ap && ap->count(p, false) == 1 + NT, "9 audio outputs (main + 8 tracks)");
        clap_audio_port_info_t pi8 = {};
        CHECK(ap && ap->get(p, NT, false, &pi8) && !std::strcmp(pi8.name, "Track 8"), "the last output is Track 8");
        static float tb[NT][2][480];
        float *tch[NT][2];
        clap_audio_buffer_t outs[1 + NT] = {out};
        for (int k = 0; k < NT; k++)
        {
            tch[k][0] = tb[k][0];
            tch[k][1] = tb[k][1];
            outs[k + 1].data32 = tch[k];
            outs[k + 1].channel_count = 2;
        }
        proc.audio_outputs = outs;
        proc.audio_outputs_count = 1 + NT;
        std::memset(midi_on, 0, sizeof midi_on);
        out_of_order = 0;
        double e[NT] = {};
        tr.flags = CLAP_TRANSPORT_HAS_TEMPO;
        out_last = 0;
        p->process(p, &proc);
        tr.flags = CLAP_TRANSPORT_IS_PLAYING | CLAP_TRANSPORT_HAS_TEMPO;
        out_of_order = 0;
        for (int b = 0; b < 800; b++)
        {
            out_last = 0;
            p->process(p, &proc);
            for (int k = 0; k < NT; k++)
                for (uint32_t i = 0; i < block; i++) e[k] += (double)tb[k][0][i] * tb[k][0][i];
        }
        int sounding = 0, chans = 0, ons = 0;
        for (int k = 0; k < 4; k++) sounding += e[k] > 1e-3;
        for (int c = 0; c < 16; c++) { chans += midi_on[c] > 0; ons += (int)midi_on[c]; }
        std::printf("track outputs: energy %.3f %.3f %.3f %.3f (5-8: %.3f %.3f %.3f %.3f); MIDI OUT: %d note-ons on %d "
                    "channels (1-4: %u %u %u %u)\n", e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], ons, chans,
                    midi_on[0], midi_on[1], midi_on[2], midi_on[3]);
        CHECK(sounding >= 3, "STILL TIDE's tracks should sound on their own outputs (%d)", sounding);
        CHECK(ons >= 4 && chans >= 3, "MIDI OUT should send the tracks' notes on their channels");
        CHECK(!out_of_order, "events out must be in order and inside the block (%u)", out_of_order);

        tr.flags = CLAP_TRANSPORT_HAS_TEMPO;   // stopped, the tails gone: a note on channel 6
        for (int b = 0; b < 600; b++)
            p->process(p, &proc);
        std::memset(midi_on, 0, sizeof midi_on);
        clap_event_note_t on6 = {};
        on6.header = {sizeof on6, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_ON, 0};
        on6.note_id = -1;
        on6.port_index = 0;
        on6.channel = 5;
        on6.key = 60;
        on6.velocity = 1.0;
        ev.notes.push_back(on6);
        p->process(p, &proc);
        ev.notes.clear();
        double e6[NT] = {};
        for (int b = 0; b < 20; b++)
        {
            p->process(p, &proc);
            for (int k = 0; k < NT; k++)
                for (uint32_t i = 0; i < block; i++) e6[k] += (double)tb[k][0][i] * tb[k][0][i];
        }
        double others = 0;
        for (int k = 1; k < NT; k++) others += e6[k];
        std::printf("note on channel 6: TRACK 1 (selected) energy %.4f, the other tracks %.4f; MIDI OUT channel 6: %u "
                    "note-ons\n", e6[0], others, midi_on[5]);
        CHECK(e6[0] > 1e-4 && e6[0] > others * 4,
              "a note on channel 6 should play the selected track (ROUT SEL), on its output");
        clap_event_note_t off6 = on6;
        off6.header.type = CLAP_EVENT_NOTE_OFF;
        ev.notes.push_back(off6);
        p->process(p, &proc);
        ev.notes.clear();
        proc.audio_outputs = &out;
        proc.audio_outputs_count = 1;
    }

    // parameters: MASTER + MACRO 1..4 (ids 0..4, as before), then every project setting by central ID; an automation
    // event sets MACRO 2, another T1 LEVEL; values read as the FM-1 shows them, and back
    auto *pa = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
    uint32_t np = pa ? pa->count(p) : 0;
    CHECK(np > 500, "the project's settings as parameters (%u)", np);
    if (pa && np > 5)
    {
        clap_param_info_t info;
        bool order = true;
        clap_id last = 0;
        int level = -1, plen = -1, level6 = -1;
        long song_id = -1;
        for (uint32_t i = 0; i < np; i++)
        {
            pa->get_info(p, i, &info);
            order &= i == 0 || info.id > last;
            last = info.id;
            if (!std::strcmp(info.name, "T1 LEVEL")) level = (int)i;
            if (!std::strcmp(info.name, "T2 PATTERN LENGTH")) plen = (int)i;
            if (!std::strcmp(info.name, "T6 LEVEL")) level6 = (int)i;
            if (!std::strcmp(info.module, "Song") && song_id < 0) song_id = (long)info.id;
        }
        CHECK(order, "parameter ids unique, in order");
        for (uint32_t i = 0; i < 5; i++)
        {
            pa->get_info(p, i, &info);
            CHECK(info.id == i, "parameter %u keeps id %u", i, i);
        }
        pa->get_info(p, 2, &info);
        CHECK(!std::strcmp(info.name, "MACRO 2") && (info.flags & CLAP_PARAM_IS_AUTOMATABLE), "MACRO 2 automatable");
        CHECK(level >= 0 && plen >= 0, "T1 LEVEL and T2 PATTERN LENGTH are parameters");
        if (level >= 0 && level6 >= 0)
        {
            clap_param_info_t l1, l6;
            pa->get_info(p, (uint32_t)level, &l1);
            pa->get_info(p, (uint32_t)level6, &l6);
            std::printf("params: T1 LEVEL id 0x%x, T6 LEVEL id 0x%x (module %s), the song's first 0x%lx\n", l1.id, l6.id,
                        l6.module, song_id);
            CHECK(l6.id == l1.id + 6 * 0x1000u, "T6 LEVEL at slot 6 (tracks 5..8 after the song's slot 4)");
        }
        CHECK(level6 >= 0, "T6 LEVEL is a parameter (8 tracks)");
        CHECK(song_id >= 0x14000 && song_id < 0x15000, "the song's settings keep slot 4's ids (0x%lx)", song_id);
        struct PEvents
        {
            clap_event_param_value_t e;
            static uint32_t size(const clap_input_events_t *) { return 1; }
            static const clap_event_header_t *get(const clap_input_events_t *l, uint32_t)
            {
                return &static_cast<PEvents *>(l->ctx)->e.header;
            }
        } pe{};
        pe.e.header = {sizeof pe.e, 100, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
        pe.e.param_id = 2;
        pe.e.note_id = pe.e.port_index = pe.e.channel = pe.e.key = -1;
        pe.e.value = 73;
        clap_input_events_t pin = {&pe, PEvents::size, PEvents::get};
        proc.in_events = &pin;
        p->process(p, &proc);
        proc.in_events = &in;
        double v = -1;
        pa->get_value(p, 2, &v);
        char txt[64];
        pa->value_to_text(p, 0, 1023, txt, sizeof txt);
        std::printf("params: %u; MACRO 2 = %.0f after automation; MASTER 1023 reads \"%s\"\n", np, v, txt);
        CHECK(v == 73, "MACRO 2 should be 73 (%.0f)", v);
        if (level >= 0 && plen >= 0)
        {
            clap_param_info_t li, pi;
            pa->get_info(p, (uint32_t)level, &li);
            pa->get_info(p, (uint32_t)plen, &pi);
            pe.e.param_id = li.id;
            pe.e.value = 100;
            proc.in_events = &pin;
            p->process(p, &proc);
            proc.in_events = &in;
            double lv = -1, back = -1, len = -1;
            pa->get_value(p, li.id, &lv);
            char lt[64], pt[64];
            pa->value_to_text(p, li.id, lv, lt, sizeof lt);
            pa->value_to_text(p, pi.id, 32, pt, sizeof pt);
            pa->text_to_value(p, pi.id, pt, &len);
            pa->text_to_value(p, li.id, lt, &back);
            std::printf("params: T1 LEVEL = %.0f \"%s\" (module %s); T2 PATTERN LENGTH 32 reads \"%s\", back %.0f\n", lv, lt,
                        li.module, pt, len);
            CHECK(lv == 100 && back == 100, "T1 LEVEL automated to 100, its text back to 100 (%.0f, %.0f)", lv, back);
            CHECK(len == 32, "T2 PATTERN LENGTH's text back to 32 (%.0f)", len);
        }
        // an FX type changes how its RATE reads (DELAY: TIME in ms): the host is asked to rescan the texts
        int fxt = -1, fxr = -1;
        for (uint32_t i = 0; i < np; i++)
        {
            pa->get_info(p, i, &info);
            if (!std::strcmp(info.name, "T1 FX 1 TYPE")) fxt = (int)i;
            if (!std::strcmp(info.name, "T1 FX 1 RATE")) fxr = (int)i;
        }
        CHECK(fxt >= 0 && fxr >= 0, "T1 FX 1 TYPE and RATE are parameters");
        if (fxt >= 0 && fxr >= 0)
        {
            clap_param_info_t ti, ri;
            pa->get_info(p, (uint32_t)fxt, &ti);
            pa->get_info(p, (uint32_t)fxr, &ri);
            double was = -1;
            pa->get_value(p, ti.id, &was);
            char before[64], after[64];
            pa->value_to_text(p, ri.id, 50, before, sizeof before);
            for (int pass = 0; pass < 2; pass++)   // (DELAY, then back: the texts each time)
            {
                int c0 = callbacks, r0 = rescans_text;
                pe.e.param_id = ti.id;
                pe.e.value = pass == 0 ? (was == 6 ? 10 : 6) : was;
                proc.in_events = &pin;
                p->process(p, &proc);
                proc.in_events = &in;
                for (int b = 0; b < 40; b++) p->process(p, &proc);   // (more than a round of the list)
                CHECK(callbacks == c0 + 1, "an FX type change asks for the main thread once (%d)", callbacks - c0);
                p->on_main_thread(p);
                p->on_main_thread(p);
                CHECK(rescans_text == r0 + 1, "the texts rescanned once (%d)", rescans_text - r0);
                if (pass == 0)
                    pa->value_to_text(p, ri.id, 50, after, sizeof after);
            }
            std::printf("params: T1 FX 1 RATE 50 reads \"%s\", \"%s\" after its TYPE changed\n", before, after);
            CHECK(std::strcmp(before, after), "T1 FX 1 RATE reads differently with another FX type");
            int c0 = callbacks;
            for (int b = 0; b < 40; b++) p->process(p, &proc);
            CHECK(callbacks == c0, "no rescan while nothing changes (%d)", callbacks - c0);
        }
    }

    // state: a .tsp record that loads back
    auto *st = (const clap_plugin_state_t *)p->get_extension(p, CLAP_EXT_STATE);
    CHECK(st != nullptr, "state extension");
    if (st)
    {
        Buf b;
        clap_ostream_t os = {&b, os_write};
        CHECK(st->save(p, &os), "state save");
        std::printf("state: %zu bytes, magic %.4s\n", b.b.size(), b.b.size() >= 4 ? (const char *)b.b.data() : "");
        CHECK(b.b.size() > 64 && !std::memcmp(b.b.data(), "TSP1", 4), "state is a TSP1 record");
        clap_istream_t is = {&b, is_read};
        CHECK(st->load(p, &is), "state load");
        Buf junk;
        junk.b.assign(100, 0x55);
        clap_istream_t js = {&junk, is_read};
        CHECK(!st->load(p, &js), "junk state refused");
    }

#if defined(__linux__)
    // the window, in a parent window of ours, driven by its timer as a host would
    if (std::getenv("DISPLAY"))
    {
        auto *gui = (const clap_plugin_gui_t *)p->get_extension(p, CLAP_EXT_GUI);
        auto *tim = (const clap_plugin_timer_support_t *)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT);
        Display *d = XOpenDisplay(nullptr);
        CHECK(gui && tim && d, "gui + timer extensions and a display");
        if (gui && tim && d)
        {
            uint32_t w = 0, h = 0;
            CHECK(gui->is_api_supported(p, CLAP_WINDOW_API_X11, false), "X11 supported");
            CHECK(gui->create(p, CLAP_WINDOW_API_X11, false), "gui create");
            gui->get_size(p, &w, &h);
            Window parent = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0, w, h, 0, 0, 0);
            XMapWindow(d, parent);
            XSync(d, False);
            clap_window_t win = {CLAP_WINDOW_API_X11, {}};
            win.x11 = parent;
            CHECK(gui->set_parent(p, &win) && gui->show(p), "set_parent + show");
            tr.flags = CLAP_TRANSPORT_IS_PLAYING | CLAP_TRANSPORT_HAS_TEMPO;
            for (int i = 0; i < 30; i++)   // 1 s: audio blocks and the window's timer, as a host interleaves them
            {
                for (int b = 0; b < 3; b++) p->process(p, &proc);
                tim->on_timer(p, 0);
            }
            XSync(d, False);
            XImage *img = XGetImage(d, parent, 300, 52, 240, 240, AllPlanes, ZPixmap);
            int lit = 0;
            for (int y = 0; img && y < 240; y++)
                for (int x = 0; x < 240; x++)
                    lit += (XGetPixel(img, x, y) & 0xFFFFFF) != 0;
            std::printf("window: %ux%u, %d of 57600 screen pixels lit\n", w, h, lit);
            CHECK(lit > 500, "the window should show the FM-1's screen");
            if (argc > 4 && img)
            {
                XImage *all = XGetImage(d, parent, 0, 0, w, h, AllPlanes, ZPixmap);
                FILE *f = std::fopen(argv[4], "wb");
                if (f && all)
                {
                    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
                    for (uint32_t y = 0; y < h; y++)
                        for (uint32_t x = 0; x < w; x++)
                        {
                            unsigned long c = XGetPixel(all, (int)x, (int)y);
                            unsigned char rgb[3] = {(unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c};
                            std::fwrite(rgb, 1, 3, f);
                        }
                    std::fclose(f);
                }
                if (all) XDestroyImage(all);
            }
            if (img) XDestroyImage(img);
            // SIZE: the host sets 200 % (its frame dragged near 2000 wide: adjust_size snaps), then 100 % again
            {
                uint32_t aw = 1950, ah = 1200, gw = 0, gh = 0;
                CHECK(gui->can_resize(p) && gui->adjust_size(p, &aw, &ah) && aw == 2000 && ah == 1248,
                      "a frame dragged to 1950 x 1200 snaps to 200 %% (%ux%u)", aw, ah);
                CHECK(gui->set_size(p, aw, ah) && gui->get_size(p, &gw, &gh) && gw == 2000 && gh == 1248, "the window at 200 %%");
                for (int i = 0; i < 4; i++) { for (int b = 0; b < 3; b++) p->process(p, &proc); tim->on_timer(p, 0); }
                Window root_r, parent_r, *kids = nullptr;
                unsigned nk = 0, cw = 0, ch = 0, bw, dp;
                int cx, cy;
                XQueryTree(d, parent, &root_r, &parent_r, &kids, &nk);
                if (nk) XGetGeometry(d, kids[0], &root_r, &cx, &cy, &cw, &ch, &bw, &dp);
                if (kids) XFree(kids);
                std::printf("size: the window %ux%u at 200 %%\n", cw, ch);
                CHECK(cw == 2000 && ch == 1248, "the plugin's X window at 200 %% (%ux%u)", cw, ch);
                CHECK(!gui->set_size(p, 1234, 567), "a size not one of the three refused");
                CHECK(gui->set_size(p, 1000, 624), "back to 100 %%");
            }
            // GENERATE (TESSERA's strip): a click on it makes a new piece, held half a second the project before it
            // comes back (the DAW's state: another project, then the one before again)
            {
                Window root_r, parent_r, *kids = nullptr;
                unsigned nk = 0;
                XQueryTree(d, parent, &root_r, &parent_r, &kids, &nk);
                Window child = nk ? kids[0] : 0;
                if (kids) XFree(kids);
                auto state_now = [&] {   // (the project's settings, as the DAW reads them: the parameters past the first 5)
                    std::vector<double> vals;
                    clap_param_info_t pi;
                    for (uint32_t i = 5; pa && i < pa->count(p); i++)
                    {
                        double x = 0;
                        pa->get_info(p, i, &pi);
                        pa->get_value(p, pi.id, &x);
                        vals.push_back(x);
                    }
                    return vals;
                };
                auto differ = [](const std::vector<double> &a, const std::vector<double> &b) {
                    size_t n = 0;
                    for (size_t i = 0; i < a.size() && i < b.size(); i++) n += a[i] != b[i];
                    return n;
                };
                auto press = [&](bool down) {
                    XEvent e{};
                    e.type = down ? ButtonPress : ButtonRelease;
                    e.xbutton.window = child;
                    e.xbutton.x = 370, e.xbutton.y = 24, e.xbutton.button = Button1;
                    XSendEvent(d, child, False, down ? ButtonPressMask : ButtonReleaseMask, &e);
                    XSync(d, False);
                };
                auto ticks = [&](int n) {
                    for (int i = 0; i < n; i++)
                    {
                        for (int b = 0; b < 3; b++) p->process(p, &proc);
                        tim->on_timer(p, 0);
                    }
                };
                CHECK(child != 0, "the plugin's window in the parent");
                std::vector<double> s0 = state_now();
                press(true), ticks(2), press(false), ticks(40);
                std::vector<double> s1 = state_now();
                press(true), ticks(20), press(false), ticks(40);
                std::vector<double> s2 = state_now();
                std::printf("generate: a click changed %zu of %zu settings; held, %zu differ from before it\n",
                            differ(s0, s1), s0.size(), differ(s0, s2));
                CHECK(differ(s0, s1) > 50, "GENERATE clicked: a new piece (%zu settings changed)", differ(s0, s1));
                CHECK(differ(s0, s2) == 0, "GENERATE held: the project before the piece again (%zu differ)", differ(s0, s2));
            }
            gui->destroy(p);
            XDestroyWindow(d, parent);
            XCloseDisplay(d);
        }
    }
#endif

#if defined(__APPLE__)
    // the window, in an NSWindow's content view, on its own timer (the main run loop) as in a host
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        auto *gui = (const clap_plugin_gui_t *)p->get_extension(p, CLAP_EXT_GUI);
        CHECK(gui, "gui extension");
        if (gui)
        {
            uint32_t w = 0, h = 0;
            CHECK(gui->is_api_supported(p, CLAP_WINDOW_API_COCOA, false), "Cocoa supported");
            CHECK(gui->create(p, CLAP_WINDOW_API_COCOA, false), "gui create");
            gui->get_size(p, &w, &h);
            NSWindow *nw = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, w, h)
                                                       styleMask:NSWindowStyleMaskBorderless
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
            nw.releasedWhenClosed = NO;   // (ARC owns it: closing must not release it a second time)
            clap_window_t win = {CLAP_WINDOW_API_COCOA, {}};
            win.cocoa = (__bridge void *)nw.contentView;
            CHECK(gui->set_parent(p, &win) && gui->show(p), "set_parent + show");
            tr.flags = CLAP_TRANSPORT_IS_PLAYING | CLAP_TRANSPORT_HAS_TEMPO;
            for (int i = 0; i < 30; i++)   // 1 s: audio blocks and the window's own timer in the run loop
            {
                for (int b = 0; b < 3; b++) p->process(p, &proc);
                [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:1.0 / 30]];
            }
            NSView *v = nw.contentView.subviews.count ? nw.contentView.subviews[0] : nil;
            CHECK(v && NSEqualSizes(v.frame.size, NSMakeSize(w, h)), "the plugin's view in the parent, %ux%u", w, h);
            CGImageRef img = v ? (__bridge CGImageRef)v.layer.contents : nullptr;
            int lit = 0;
            std::vector<uint32_t> all(w * h);
            if (img && CGImageGetWidth(img) == w && CGImageGetHeight(img) == h)
            {
                CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
                CGContextRef c = CGBitmapContextCreate(all.data(), w, h, 8, w * 4, cs,
                                                       kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaNoneSkipFirst);
                CGContextDrawImage(c, CGRectMake(0, 0, w, h), img);
                CGContextRelease(c);
                CGColorSpaceRelease(cs);
                for (int y = 52; y < 52 + 240; y++)
                    for (int x = 300; x < 300 + 240; x++)
                        lit += (all[y * w + x] & 0xFFFFFF) != 0;
            }
            std::printf("window: %ux%u, %d of 57600 screen pixels lit\n", w, h, lit);
            CHECK(lit > 500, "the window should show the FM-1's screen");
            if (argc > 4 && img)
                if (FILE *f = std::fopen(argv[4], "wb"))
                {
                    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
                    for (uint32_t i = 0; i < w * h; i++)
                    {
                        unsigned char rgb[3] = {(unsigned char)(all[i] >> 16), (unsigned char)(all[i] >> 8), (unsigned char)all[i]};
                        std::fwrite(rgb, 1, 3, f);
                    }
                    std::fclose(f);
                }
            // a key and a click reach the panel: Z (F3) held, then the panel's view lets go of the keys on losing focus
            if (v)
            {
                NSEvent *kd = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0
                                              timestamp:0 windowNumber:nw.windowNumber context:nil characters:@"z"
                            charactersIgnoringModifiers:@"z" isARepeat:NO keyCode:6];
                NSEvent *ku = [NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0
                                              timestamp:0 windowNumber:nw.windowNumber context:nil characters:@"z"
                            charactersIgnoringModifiers:@"z" isARepeat:NO keyCode:6];
                [v keyDown:kd];
                for (int b = 0; b < 3; b++) p->process(p, &proc);
                [v keyUp:ku];
                for (int b = 0; b < 3; b++) p->process(p, &proc);
            }
            gui->destroy(p);
            CHECK(nw.contentView.subviews.count == 0, "the view taken out of the parent on destroy");
            [nw close];
        }
    }
#endif

    if (argc > 3)
    {
        wav_write(argv[3], allL, allR, rate);
        std::printf("wrote %s\n", argv[3]);
    }
    p->stop_processing(p);
    p->deactivate(p);
    p->destroy(p);
    const clap_plugin_t *again = fac->create_plugin(fac, &host, id);
    CHECK(again != nullptr, "a new instance once the first is gone");
    if (again)
        again->destroy(again);
    entry->deinit();
    std::printf(fails ? "render test: %d FAILED\n" : "render test: ok\n", fails);
    return fails ? 1 : 0;
}
