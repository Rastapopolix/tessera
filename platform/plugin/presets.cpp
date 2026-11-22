/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's projects in the DAW's preset browser (presets.h). One provider, two locations: the demos inside the plugin
 * (kind PLUGIN, each by its load key) and the user's folder (kind FILE, the .tsp files in it: the host crawls it and
 * asks for each file's metadata). Thread-safe as the factory must be: nothing here is shared but constant data. */
#include "presets.h"

#include <cstring>

#include "projfile.h"

namespace
{
#include "plugin_demos.h"   // (build/gen, platform/plugin/gen.py: PLUGIN_DEMOS[])

const size_t NDEMOS = sizeof PLUGIN_DEMOS / sizeof PLUGIN_DEMOS[0];

const clap_preset_discovery_provider_descriptor_t provider_desc = {CLAP_VERSION_INIT, TESS_PLUGIN_ID ".presets",
                                                                   "Tessera projects", "Tessera"};

// a record's NAME, or the file's name without .tsp when it has none
std::string name_of(const std::vector<uint8_t> &rec, const std::string &fallback)
{
    std::string n;
    return projfile::tsp_name(rec, n) && !n.empty() ? n : fallback;
}

// one preset to the receiver: its name and load key, this plugin, flags, a description
bool tell(const clap_preset_discovery_metadata_receiver_t *r, const std::string &name, const char *key, uint32_t flags,
          const char *about)
{
    if (!r->begin_preset(r, name.c_str(), key))
        return false;
    clap_universal_plugin_id_t id = {"clap", TESS_PLUGIN_ID};
    r->add_plugin_id(r, &id);
    r->set_flags(r, flags);
    r->add_creator(r, "Tessera");
    if (about && *about)
        r->set_description(r, about);
    return true;
}

struct Provider
{
    clap_preset_discovery_provider_t p{};
    const clap_preset_discovery_indexer_t *indexer = nullptr;
    std::string user;   // the user's folder (empty: none)
};
Provider *of(const clap_preset_discovery_provider_t *p) { return static_cast<Provider *>(p->provider_data); }

bool p_init(const clap_preset_discovery_provider_t *p)
{
    Provider *v = of(p);
    const clap_preset_discovery_indexer_t *ix = v->indexer;
    clap_preset_discovery_filetype_t ft = {"Tessera project", "A whole Tessera project (SAVE in the plugin, the FM-1's "
                                           "editor)", "tsp"};
    if (!ix->declare_filetype(ix, &ft))
        return false;
    clap_preset_discovery_location_t demos = {CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT, "Tessera demos",
                                              CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr};
    if (!ix->declare_location(ix, &demos))
        return false;
    v->user = projfile::presets_dir();
    if (!v->user.empty())
    {
        clap_preset_discovery_location_t user = {CLAP_PRESET_DISCOVERY_IS_USER_CONTENT, "Tessera: my projects",
                                                 CLAP_PRESET_DISCOVERY_LOCATION_FILE, v->user.c_str()};
        ix->declare_location(ix, &user);
    }
    return true;
}
void p_destroy(const clap_preset_discovery_provider_t *p) { delete of(p); }
bool p_get_metadata(const clap_preset_discovery_provider_t *, uint32_t kind, const char *location,
                    const clap_preset_discovery_metadata_receiver_t *r)
{
    if (kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)   // the demos, each by its file name
    {
        for (size_t i = 0; i < NDEMOS; i++)
            if (!tell(r, presets::demo_name(i), PLUGIN_DEMOS[i].key, CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
                      PLUGIN_DEMOS[i].about))
                break;
        return true;
    }
    if (kind != CLAP_PRESET_DISCOVERY_LOCATION_FILE || !location)
        return false;
    std::vector<uint8_t> rec;
    std::string err;
    if (!presets::record(kind, location, nullptr, rec, err))
    {
        r->on_error(r, 0, err.c_str());
        return false;
    }
    std::string base = location;   // (the file's name without its folder and .tsp: a record with no NAME)
    size_t slash = base.find_last_of("/\\");
    if (slash != std::string::npos) base = base.substr(slash + 1);
    if (base.size() > 4 && base.compare(base.size() - 4, 4, ".tsp") == 0) base.resize(base.size() - 4);
    tell(r, name_of(rec, base), nullptr, CLAP_PRESET_DISCOVERY_IS_USER_CONTENT, nullptr);
    return true;
}
const void *p_get_extension(const clap_preset_discovery_provider_t *, const char *) { return nullptr; }

uint32_t f_count(const clap_preset_discovery_factory_t *) { return 1; }
const clap_preset_discovery_provider_descriptor_t *f_desc(const clap_preset_discovery_factory_t *, uint32_t i)
{
    return i == 0 ? &provider_desc : nullptr;
}
const clap_preset_discovery_provider_t *f_create(const clap_preset_discovery_factory_t *,
                                                 const clap_preset_discovery_indexer_t *indexer, const char *id)
{
    if (!indexer || !id || std::strcmp(id, provider_desc.id))
        return nullptr;
    auto *v = new Provider;
    v->indexer = indexer;
    v->p.desc = &provider_desc;
    v->p.provider_data = v;
    v->p.init = p_init;
    v->p.destroy = p_destroy;
    v->p.get_metadata = p_get_metadata;
    v->p.get_extension = p_get_extension;
    return &v->p;
}
const clap_preset_discovery_factory_t the_factory = {f_count, f_desc, f_create};
}   // namespace

namespace presets
{
const clap_preset_discovery_factory_t *factory() { return &the_factory; }

size_t demo_count() { return NDEMOS; }
const char *demo_key(size_t i) { return i < NDEMOS ? PLUGIN_DEMOS[i].key : ""; }
std::string demo_name(size_t i)
{
    if (i >= NDEMOS) return "";
    std::vector<uint8_t> rec(PLUGIN_DEMOS[i].data, PLUGIN_DEMOS[i].data + PLUGIN_DEMOS[i].size);
    return name_of(rec, PLUGIN_DEMOS[i].key);
}

bool record(uint32_t kind, const char *location, const char *load_key, std::vector<uint8_t> &out, std::string &err)
{
    if (kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)
    {
        for (size_t i = 0; load_key && i < NDEMOS; i++)
            if (!std::strcmp(load_key, PLUGIN_DEMOS[i].key))
            {
                out.assign(PLUGIN_DEMOS[i].data, PLUGIN_DEMOS[i].data + PLUGIN_DEMOS[i].size);
                return true;
            }
        err = "NO SUCH DEMO";
        return false;
    }
    if (kind != CLAP_PRESET_DISCOVERY_LOCATION_FILE || !location)
    {
        err = "NOT A TESSERA PRESET";
        return false;
    }
    if (!projfile::read_file(location, out, 4u << 20))
    {
        err = "CANNOT READ THAT FILE";
        return false;
    }
    std::string n;
    if (out.size() < 4 || std::memcmp(out.data(), "TSP1", 4) || !projfile::tsp_name(out, n))
    {
        err = "NOT A TESSERA PROJECT";
        return false;
    }
    return true;
}
}   // namespace presets
