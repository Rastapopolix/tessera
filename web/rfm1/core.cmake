# SPDX-License-Identifier: GPL-3.0-only
# Tessera as an FM1VST core (Quixotic7/FM1VST, cmake/FM1Cores.cmake fm1_add_core) and as an .rfm1 for RemoteFM1
# (cmake/FM1Rfm1.cmake fm1_add_rfm1; FM1VST's docs/RFM1.md). web/rfm1/build.sh puts this folder at
# core-glue/tessera/ and this repository at cores/tessera/ of a tree beside an FM1VST checkout and builds it.
# The firmware is the web simulator's (web/sim/sim.c): the device's engine set and the Mono look, as web/sim/build.sh
# builds it. The version: TESSERA_VERSION (build.sh: the tag, or --version), else melodee.c's string.
set(TESSERA_VERSION "" CACHE STRING "the Tessera version of the core (3.2.2); empty: firmware/src/melodee.c's")
if(TESSERA_VERSION)
    set(_ts_version VERSION ${TESSERA_VERSION})
    set(_ts_version_define "MELODEE_VERSION=\"v${TESSERA_VERSION}\"")
else()
    set(_ts_version VERSION_FILE firmware/src/melodee.c VERSION_REGEX "^#define MELODEE_VERSION \"v?([^\"]*)\"")
    set(_ts_version_define "")
endif()
fm1_add_core(tessera ${CMAKE_CURRENT_LIST_DIR} SOURCE ${CMAKE_SOURCE_DIR}/cores/tessera UNIT core_tessera.c
             DESC core_tessera_desc.c FAMILY tessera LICENSE GPL-3.0-only
             CREDITS "Tessera is by Theo Dainis (Rastapopolix). It grew out of Melodee by keremimo (Kerem Kilic, Ellic Studio), itself built on Felucca by Leo Kuroshita / Hugelton Instruments. GPL-3.0-only. The TIDE and SWARM mod sources are inspired by Triple Sloth by Nonlinear Circuits (Andrew Fitch) and ochd. The README names every original."
             GEN ui_fonts.h ui_icons.h ui_keycaps.h ui_palettes.h melodee_tables.h pid_help.h melodee_fm6.h
                 x0x_drum_tables.h sim_demos.h
             EXTRA_GEN "tools/gen_909_tables.py build/gen/x0x_drum_tables.h"
                       "tools/gen_sim_demos.py build/gen/sim_demos.h"
             ${_ts_version}
             INCLUDES ${CMAKE_SOURCE_DIR}/cores/tessera/tests
             DEFINES MELODEE_USB_AUDIO=1 MELODEE_LOOK=1 MELODEE_FM6=1 MELODEE_SID=0 MELODEE_PHYS=1 MELODEE_PLAITS=0
                     ${_ts_version_define})

# the .rfm1: the licence texts are the default set (LICENSE, each file of LICENSES/)
fm1_add_rfm1(tessera AUTHORS "Theo Dainis (Rastapopolix)" MEMORY_MIB 8)
