#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The MACRO engine's C++ (Plaits and stmlib in firmware/third_party/mi, and firmware/src/macro_glue.cc), in one list
# for the Daisy build (platform/daisy/Makefile; andamento-fw also web/andamento/build.sh and its tests). The plugin
# (platform/plugin/CMakeLists.txt, 44.1 kHz) and tests/tsp_cross.sh build the same files at their own rate.
#   sh platform/daisy/plaits_sources.sh [ROOT]           the sources, as paths from ROOT (the repo; default .)
#   sh platform/daisy/plaits_sources.sh --flags [ROOT]   the compile flags they need (the -I paths from ROOT)
# Plaits at Andamento's 48 kHz, tuned by the true rate (PLAITS_SAMPLE_RATE_EXACT: Plaits' own corrects for its chip's
# 47,872 Hz), in renders of 16 samples (two a block of CTL 32: eng_macro.c), the lean set of models (third_party/mi
# README.md). TEST: stmlib's portable C in place of its STM32 inline assembly and Plaits' user data never read from a
# flash address (on the Seed 0x08007000 is the Daisy bootloader's); stdio.h for its TEST mocks' printf, never called.
if [ "$1" = --flags ]; then
    R=${2:-.}
    echo "-DTEST -DPLAITS_LEAN -DPLAITS_SAMPLE_RATE=48000.0f -DPLAITS_SAMPLE_RATE_EXACT -DPLAITS_BLOCK_SIZE=16 -include stdio.h" \
        "-I$R/firmware/third_party/mi -I$R/firmware/src"
    exit 0
fi
R=${1:-.}
P=$R/
[ "$R" = . ] && P=
M=${P}firmware/third_party/mi
for f in "$M"/plaits/dsp/*.cc "$M"/plaits/dsp/*/*.cc "$M"/plaits/resources.cc "$M"/stmlib/dsp/*.cc \
    "$M"/stmlib/utils/*.cc "${P}firmware/src/macro_glue.cc"; do
    echo "$f"
done
