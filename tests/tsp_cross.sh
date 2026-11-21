#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
# One save format everywhere (ROADMAP standing decision): a .tsp saved by one target loads in every other one, and is
# written back byte for byte. Builds Tessera's core API (tests/tsp_cross.c over platform/core/core_unit.c) as each
# target builds it, then for every demo in demos/: each target loads it, plays it a few seconds and saves it; every
# target loads that save and saves it again at once, and the bytes must not change.
#
#   tests/tsp_cross.sh [OUTDIR]          (default build/tsp_cross; needs build/gen, and build/andamento/gen at 48 kHz:
#                                         web/andamento/build.sh --headers)
#
# Targets (their options as their own builds set them):
#   fm1     the FM-1's firmware options (screen reader, USB audio and host), 44.1 kHz: the device's project code on
#           the host HAL (the device itself runs the same project.c / project_tsp.c)
#   web     web/sim/build.sh's options, 44.1 kHz
#   plugin  platform/plugin/CMakeLists.txt's options (the DAW clock, track outputs, SID and MACRO), 44.1 kHz
#   daisy   platform/daisy's (no USB, no FM-1 UI or screen, Andamento's output hooks, SID and MACRO), 48 kHz
# The FM-1 reads a plugin's or a Daisy's SID or MACRO track as FM6's INIT: the demos have none. Then
# tests/engines_extra.c on the plugin and the Daisy: SID, the SID kit and MACRO play, save and load (needs c++).
set -u
cd "$(dirname "$0")/.."
OUT=${1:-build/tsp_cross}
CC=${CC:-clang}
SECS=${TSP_CROSS_SECS:-6}
mkdir -p "$OUT"
[ -f build/gen/melodee_tables.h ] || { echo "tsp_cross: build/gen missing (tools/build.py generate)"; exit 1; }
[ -f build/andamento/gen/melodee_tables.h ] || sh web/andamento/build.sh --headers > /dev/null || exit 1

COMMON="-O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core"
CXX=${CXX:-clang++}
# the engines the FM-1 leaves out (SID, DRUM's SID KIT, MACRO, PHASE): on in the plugin and the Daisy, as their builds set them
EXTRA="-DMELODEE_SID=1 -DMELODEE_SID_KIT=1 -DMELODEE_PLAITS=1 -DMELODEE_PHASE=1"
plaits() {  # rate: MACRO's Plaits voice (firmware/third_party/mi, C++) at that rate, into $OUT/mi<rate>/
    d=$OUT/mi$1; mkdir -p "$d"; M=firmware/third_party/mi
    for f in $M/plaits/dsp/*.cc $M/plaits/dsp/*/*.cc $M/plaits/resources.cc $M/stmlib/dsp/*.cc $M/stmlib/utils/*.cc \
        firmware/src/macro_glue.cc; do
        o=$d/$(echo "$f" | tr / _).o
        [ -f "$o" ] && [ "$o" -nt "$f" ] && continue
        $CXX -O2 -std=c++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics -w -DTEST -DPLAITS_LEAN \
            -DPLAITS_SAMPLE_RATE=$1.0f -DPLAITS_SAMPLE_RATE_EXACT -DPLAITS_BLOCK_SIZE=16 -include stdio.h -I$M \
            -Ifirmware/src -c "$f" -o "$o" || { echo "tsp_cross: Plaits ($f) failed"; exit 1; }
    done
}
build() {   # name gen flags... (with MELODEE_PLAITS=1: Plaits linked, at the rate of the name's tables)
    n=$1; g=$2; shift 2
    case " $* " in
    *" -DMELODEE_PLAITS=1 "*)
        rate=44100; [ "$n" = daisy ] && rate=48000
        plaits $rate
        for src in tsp_cross engines_extra; do
            $CC $COMMON -I"$g" "$@" -c tests/$src.c -o "$OUT/${src}_$n.o" &&
                $CXX -o "$OUT/${src}_$n" "$OUT/${src}_$n.o" "$OUT"/mi$rate/*.o -lm ||
                { echo "tsp_cross: the $n build ($src) failed"; exit 1; }
        done ;;
    *)
        $CC $COMMON -I"$g" "$@" tests/tsp_cross.c -o "$OUT/tsp_cross_$n" -lm || { echo "tsp_cross: the $n build failed"; exit 1; } ;;
    esac
}
build fm1 build/gen -DMELODEE_USB_AUDIO=1 -DMELODEE_USB_HOST=1 -DMELODEE_ACCESS=1 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 \
    -DMELODEE_PHYS=1 '-DMELODEE_VERSION="fm1"'
build web build/gen -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=1 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="web"'
build plugin build/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 \
    -DMELODEE_FM6=1 -DMELODEE_PHYS=1 '-DMELODEE_VERSION="plugin"' $EXTRA
build daisy build/andamento/gen -DMELODEE_USB_HOST=0 -DMELODEE_USB_AUDIO=0 -DMELODEE_ACCESS=0 -DHOST_HAL_SCREEN=0 \
    -DTESS_CORE_UI=0 -DCORE_OUT_HOOKS=1 '-DMELODEE_VERSION="daisy"' $EXTRA
TARGETS="fm1 web plugin daisy"
for t in $TARGETS; do printf '%-7s %s\n' "$t" "$("$OUT/tsp_cross_$t" info)"; done

fails=0; checks=0
for demo in demos/*.tsp; do
    d=$(basename "$demo" .tsp)
    for a in $TARGETS; do
        src="$OUT/$d.$a.tsp"
        if ! "$OUT/tsp_cross_$a" save "$demo" "$SECS" "$src"; then
            echo "FAIL $d: $a can't load the demo and save it"; fails=$((fails + 1)); continue
        fi
        for b in $TARGETS; do
            checks=$((checks + 1))
            dst="$OUT/$d.$a.$b.tsp"
            if ! "$OUT/tsp_cross_$b" load "$src" "$dst"; then
                echo "FAIL $d: saved by $a, refused by $b"; fails=$((fails + 1))
            elif ! cmp -s "$src" "$dst"; then
                echo "FAIL $d: saved by $a, loaded by $b, saved again: $(cmp "$src" "$dst" | head -1)"
                fails=$((fails + 1))
            fi
        done
    done
done
for t in plugin daisy; do   # SID, the SID kit, MACRO and PHASE play and save there (tests/engines_extra.c)
    "$OUT/engines_extra_$t" > "$OUT/engines_extra_$t.log" 2>&1 || { cat "$OUT/engines_extra_$t.log"; fails=$((fails + 1)); }
    echo "$t: $(tail -1 "$OUT/engines_extra_$t.log")"
done
echo "tsp_cross: $checks loads ($(ls demos/*.tsp | wc -l) demos x 4 x 4 targets), $fails failed"
[ $fails = 0 ]
