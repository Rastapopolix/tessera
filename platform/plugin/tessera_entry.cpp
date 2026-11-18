/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The exported clap_entry, compiled once per format by clap-wrapper's make_clapfirst_plugins (CMakeLists.txt) */
#include <clap/clap.h>

extern bool tessera_entry_init(const char *plugin_path);
extern void tessera_entry_deinit(void);
extern const void *tessera_entry_get_factory(const char *factory_id);

extern "C"
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {CLAP_VERSION, tessera_entry_init, tessera_entry_deinit,
                                                             tessera_entry_get_factory};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
