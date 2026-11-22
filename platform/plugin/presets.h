/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's projects in the DAW's preset browser (CLAP preset discovery and preset-load): the eight demos inside the
 * plugin (factory content, build/gen/plugin_demos.h from demos/*.tsp) and the user's own .tsp files in
 * Documents/Tessera/Presets (projfile::presets_dir). A preset is a whole project, as OPEN loads it. */
#pragma once
#include <clap/clap.h>

#include <cstdint>
#include <string>
#include <vector>

#define TESS_PLUGIN_ID "io.github.rastapopolix.tessera"   // (the plugin's CLAP id: tessera_clap.cpp desc)

namespace presets
{
/* the factory: tessera_entry_get_factory hands it out for CLAP_PRESET_DISCOVERY_FACTORY_ID (and the draft's id) */
const clap_preset_discovery_factory_t *factory();
/* a preset's record, as the browser names it: a file (location: its path) or a demo (location null, load_key its
 * file name: "undertow"). false and why in err if there is none, or it is not a project */
bool record(uint32_t location_kind, const char *location, const char *load_key, std::vector<uint8_t> &out,
            std::string &err);
/* the demos inside the plugin: how many, demo i's load key and name ("UNDERTOW") */
size_t demo_count();
const char *demo_key(size_t i);
std::string demo_name(size_t i);
}   // namespace presets
