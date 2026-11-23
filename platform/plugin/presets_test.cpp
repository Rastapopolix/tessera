/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's projects in the DAW's preset browser (presets.cpp), as a minimal CLAP host and indexer over the built
 * Smalti.clap (POSIX), with HOME in a folder of its own:
 *   - the preset discovery factory: one provider; its init declares the .tsp file type, the demos inside the plugin
 *     (factory content) and the user's folder Documents/Tessera/Presets (user content, made by it);
 *   - the demos' metadata: the eight, each by its NAME and file name, for this plugin, with a description;
 *   - a .tsp saved into the user's folder: its metadata names it; a file that is not a project is refused with why;
 *   - preset-load: a demo by its load key, then the user's file, each loaded (the state is that project), the host
 *     told; an unknown key refused and the host told why.
 * usage: tessera-presets-test path/to/Smalti.clap */
#include <clap/clap.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static int fails = 0;
#define CHECK(c, ...)                          \
    do                                         \
    {                                          \
        if (!(c))                              \
        {                                      \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                 \
            fails++;                           \
        }                                      \
    } while (0)

// ---- the indexer: what the provider declares
struct Loc { uint32_t flags, kind; std::string name, where; };
static std::vector<std::string> filetypes;
static std::vector<Loc> locs;
static bool ix_filetype(const clap_preset_discovery_indexer_t *, const clap_preset_discovery_filetype_t *f)
{
    filetypes.push_back(f->file_extension ? f->file_extension : "");
    return true;
}
static bool ix_location(const clap_preset_discovery_indexer_t *, const clap_preset_discovery_location_t *l)
{
    locs.push_back({l->flags, l->kind, l->name ? l->name : "", l->location ? l->location : ""});
    return true;
}
static bool ix_soundpack(const clap_preset_discovery_indexer_t *, const clap_preset_discovery_soundpack_t *) { return true; }
static const void *ix_ext(const clap_preset_discovery_indexer_t *, const char *) { return nullptr; }
static const clap_preset_discovery_indexer_t indexer = {CLAP_VERSION_INIT, "tessera-presets-test", "Tessera", "", "0",
                                                        nullptr, ix_filetype, ix_location, ix_soundpack, ix_ext};

// ---- the receiver: the presets the provider tells
struct Preset { std::string name, key, plugin, about; uint32_t flags = 0; };
static std::vector<Preset> got;
static std::string error_said;
static void r_error(const clap_preset_discovery_metadata_receiver_t *, int32_t, const char *m) { error_said = m ? m : ""; }
static bool r_begin(const clap_preset_discovery_metadata_receiver_t *, const char *name, const char *key)
{
    got.push_back({name ? name : "", key ? key : ""});
    return true;
}
static void r_plugin(const clap_preset_discovery_metadata_receiver_t *, const clap_universal_plugin_id_t *id)
{
    got.back().plugin = id->id;
}
static void r_pack(const clap_preset_discovery_metadata_receiver_t *, const char *) {}
static void r_flags(const clap_preset_discovery_metadata_receiver_t *, uint32_t f) { got.back().flags = f; }
static void r_creator(const clap_preset_discovery_metadata_receiver_t *, const char *) {}
static void r_about(const clap_preset_discovery_metadata_receiver_t *, const char *d) { got.back().about = d; }
static void r_times(const clap_preset_discovery_metadata_receiver_t *, clap_timestamp, clap_timestamp) {}
static void r_feature(const clap_preset_discovery_metadata_receiver_t *, const char *) {}
static void r_extra(const clap_preset_discovery_metadata_receiver_t *, const char *, const char *) {}
static const clap_preset_discovery_metadata_receiver_t receiver = {
    nullptr, r_error, r_begin, r_plugin, r_pack, r_flags, r_creator, r_about, r_times, r_feature, r_extra};

// ---- the host: preset-load's answers, state's dirty
static std::string loaded_key, load_error;
static int loaded_n = 0;
static void hp_error(const clap_host_t *, uint32_t, const char *, const char *, int32_t, const char *m) { load_error = m; }
static void hp_loaded(const clap_host_t *, uint32_t, const char *, const char *key)
{
    loaded_n++;
    loaded_key = key ? key : "";
}
static const clap_host_preset_load_t host_pl = {hp_error, hp_loaded};
static void h_dirty(const clap_host_t *) {}
static const clap_host_state_t host_state = {h_dirty};
static void h_rescan(const clap_host_t *, clap_param_rescan_flags) {}
static void h_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static void h_flush(const clap_host_t *) {}
static const clap_host_params_t host_params = {h_rescan, h_clear, h_flush};
static const void *h_ext(const clap_host_t *, const char *id)
{
    if (!std::strcmp(id, CLAP_EXT_PRESET_LOAD)) return &host_pl;
    if (!std::strcmp(id, CLAP_EXT_STATE)) return &host_state;
    if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &host_params;
    return nullptr;
}
static void h_nop(const clap_host_t *) {}
static const clap_host_t host = {CLAP_VERSION_INIT, nullptr, "tessera-presets-test", "Tessera", "", "0", h_ext, h_nop,
                                 h_nop, h_nop};

struct Buf { std::vector<uint8_t> b; };
static int64_t os_write(const clap_ostream_t *s, const void *p, uint64_t n)
{
    auto *b = static_cast<Buf *>(s->ctx);
    b->b.insert(b->b.end(), (const uint8_t *)p, (const uint8_t *)p + n);
    return (int64_t)n;
}
// the state as text (its TSP1 record holds the project's NAME as letters), "" none
static std::string state_text(const clap_plugin_t *p, const clap_plugin_state_t *st)
{
    Buf b;
    clap_ostream_t os = {&b, os_write};
    if (!st->save(p, &os) || b.b.size() < 16) return "";
    std::string s(b.b.begin(), b.b.end());
    return s;
}

// the library inside a plugin path: on macOS Smalti.clap is a bundle, its code in Contents/MacOS/Tessera
static std::string clap_library(const char *path)
{
#ifdef __APPLE__
    std::string p(path);
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    std::string base = p.substr(p.find_last_of('/') == std::string::npos ? 0 : p.find_last_of('/') + 1);
    std::string stem = base.substr(0, base.rfind('.'));
    std::string inner = p + "/Contents/MacOS/" + stem;
    if (FILE *f = std::fopen(inner.c_str(), "rb")) { std::fclose(f); return inner; }
#endif
    return path;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: tessera-presets-test path/to/Smalti.clap\n");
        return 2;
    }
    char tmpl[] = "/tmp/tessera-presets-XXXXXX";
    const char *home = mkdtemp(tmpl);
    CHECK(home, "a home of its own");
    setenv("HOME", home, 1);
    unsetenv("XDG_CONFIG_HOME");
    void *lib = dlopen(clap_library(argv[1]).c_str(), RTLD_NOW | RTLD_LOCAL);
    CHECK(lib, "dlopen %s: %s", argv[1], dlerror());
    if (!lib) return 1;
    auto *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
    CHECK(entry && entry->init(argv[1]), "clap_entry");
    if (!entry) return 1;
    auto *pf = (const clap_preset_discovery_factory_t *)entry->get_factory(CLAP_PRESET_DISCOVERY_FACTORY_ID);
    CHECK(pf && pf->count(pf) == 1, "one preset provider");
    if (!pf) return 1;
    const clap_preset_discovery_provider_descriptor_t *pd = pf->get_descriptor(pf, 0);
    const clap_preset_discovery_provider_t *prov = pf->create(pf, &indexer, pd->id);
    CHECK(prov && prov->init(prov), "the provider made and initialised");
    if (!prov) return 1;
    std::string user = std::string(home) + "/Documents/Tessera/Presets";
    struct stat sb;
    CHECK(filetypes.size() == 1 && filetypes[0] == "tsp", "the .tsp file type declared");
    CHECK(locs.size() == 2 && locs[0].kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN &&
              (locs[0].flags & CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT) && locs[1].kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE &&
              locs[1].where == user && (locs[1].flags & CLAP_PRESET_DISCOVERY_IS_USER_CONTENT),
          "the demos inside the plugin and the user's folder declared");
    CHECK(stat(user.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode), "the user's folder made");

    // the demos
    CHECK(prov->get_metadata(prov, CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr, &receiver), "the demos' metadata");
    std::printf("demos:");
    for (auto &g : got) std::printf(" %s (%s)", g.name.c_str(), g.key.c_str());
    std::printf("\n");
    bool named = got.size() == 8;
    for (auto &g : got)
        named = named && !g.name.empty() && !g.key.empty() && g.plugin == "io.github.rastapopolix.tessera" && !g.about.empty() &&
                (g.flags & CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT);
    CHECK(named, "eight demos, each named, keyed, for Tessera, described (%zu)", got.size());
    bool undertow = false;
    for (auto &g : got) undertow = undertow || (g.key == "undertow" && g.name == "UNDERTOW");
    CHECK(undertow, "UNDERTOW among them, by its file name");

    // the plugin: a demo loaded from the browser, then a file of the user's
    auto *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    const clap_plugin_t *p = fac->create_plugin(fac, &host, "io.github.rastapopolix.tessera");
    CHECK(p && p->init(p) && p->activate(p, 48000, 64, 512), "an instance");
    if (!p) return 1;
    auto *pl = (const clap_plugin_preset_load_t *)p->get_extension(p, CLAP_EXT_PRESET_LOAD);
    auto *st = (const clap_plugin_state_t *)p->get_extension(p, CLAP_EXT_STATE);
    CHECK(pl && st, "preset-load and state extensions");
    if (!pl || !st) return 1;
    CHECK(pl->from_location(p, CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr, "undertow") && loaded_n == 1 &&
              loaded_key == "undertow", "UNDERTOW loaded from the browser, the host told");
    std::string s1 = state_text(p, st);
    CHECK(s1.find("UNDERTOW") != std::string::npos, "the state is UNDERTOW's project");
    // that project saved into the user's folder under another name: the browser finds it, loading it works
    Buf b;
    clap_ostream_t os = {&b, os_write};
    st->save(p, &os);
    size_t tsp = 4 <= b.b.size() ? (size_t)(b.b[4] | b.b[5] << 8 | b.b[6] << 16 | (uint32_t)b.b[7] << 24) : 0;   // (the record's own length)
    std::vector<uint8_t> rec(b.b.begin(), b.b.begin() + (tsp <= b.b.size() ? tsp : 0));
    std::string mine = user + "/mine.tsp";
    if (FILE *f = std::fopen(mine.c_str(), "wb")) { std::fwrite(rec.data(), 1, rec.size(), f); std::fclose(f); }
    got.clear();
    CHECK(prov->get_metadata(prov, CLAP_PRESET_DISCOVERY_LOCATION_FILE, mine.c_str(), &receiver) && got.size() == 1 &&
              got[0].name == "UNDERTOW" && (got[0].flags & CLAP_PRESET_DISCOVERY_IS_USER_CONTENT),
          "a .tsp in the user's folder: named by its NAME, user content (%s)", got.empty() ? "" : got[0].name.c_str());
    std::string junk = user + "/junk.tsp";
    if (FILE *f = std::fopen(junk.c_str(), "wb")) { std::fputs("not a project", f); std::fclose(f); }
    got.clear();
    CHECK(!prov->get_metadata(prov, CLAP_PRESET_DISCOVERY_LOCATION_FILE, junk.c_str(), &receiver) && got.empty() &&
              !error_said.empty(), "a file that is not a project refused (%s)", error_said.c_str());
    CHECK(pl->from_location(p, CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr, "still-tide"), "STILL TIDE loaded");
    CHECK(pl->from_location(p, CLAP_PRESET_DISCOVERY_LOCATION_FILE, mine.c_str(), nullptr) && loaded_n == 3,
          "the user's file loaded from the browser");
    CHECK(state_text(p, st).find("UNDERTOW") != std::string::npos, "the state is the user's file's project");
    CHECK(!pl->from_location(p, CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr, "no-such-demo") && !load_error.empty(),
          "an unknown key refused, the host told why (%s)", load_error.c_str());

    p->deactivate(p);
    p->destroy(p);
    prov->destroy(prov);
    entry->deinit();
    std::remove(mine.c_str());
    std::remove(junk.c_str());
    std::printf(fails ? "presets test: %d FAILED\n" : "presets test: ok\n", fails);
    return fails ? 1 : 0;
}
