/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's render test: a minimal CLAP host (POSIX) that loads the built Tessera.clap and checks
 *   - one instance: a second create is refused while the first lives;
 *   - with the host's transport playing at 48 kHz, STILL TIDE (the project it starts with) makes sound;
 *   - a note in on the selected track changes the output;
 *   - the state is a .tsp record (TSP1) and loads back;
 *   - the parameters: MASTER and MACRO 1..4, automation in at its sample, the value read back;
 *   - (Linux, with a DISPLAY, e.g. under xvfb-run) the window opens in a parent window and shows the FM-1's screen;
 *   - optionally writes the first seconds as a 16-bit WAV (third argument) to listen to.
 * usage: tessera-render-test path/to/Tessera.clap [seconds] [out.wav] [window.ppm] */
#include <clap/clap.h>
#include <dlfcn.h>
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
static const void *h_get_extension(const clap_host_t *, const char *) { return nullptr; }
static void h_nop(const clap_host_t *) {}
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-render-test", "Tessera", "", "0",
                                 h_get_extension, h_nop, h_nop, h_nop};

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
static bool out_try_push(const clap_output_events_t *, const clap_event_header_t *) { return true; }

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

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s Tessera.clap [seconds] [out.wav]\n", argv[0]);
        return 2;
    }
    double seconds = argc > 2 ? std::atof(argv[2]) : 12.0;
    const uint32_t rate = 48000, block = 480;

    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
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
    CHECK(fac->create_plugin(fac, &host, id) == nullptr, "a second instance must be refused");
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

    // parameters: MASTER + MACRO 1..4; an automation event sets MACRO 2
    auto *pa = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
    CHECK(pa && pa->count(p) == 5, "5 parameters");
    if (pa && pa->count(p) == 5)
    {
        clap_param_info_t info;
        pa->get_info(p, 2, &info);
        CHECK(!std::strcmp(info.name, "MACRO 2") && (info.flags & CLAP_PARAM_IS_AUTOMATABLE), "MACRO 2 automatable");
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
        std::printf("params: MACRO 2 = %.0f after automation; MASTER 1023 reads \"%s\"\n", v, txt);
        CHECK(v == 73, "MACRO 2 should be 73 (%.0f)", v);
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
            gui->destroy(p);
            XDestroyWindow(d, parent);
            XCloseDisplay(d);
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
