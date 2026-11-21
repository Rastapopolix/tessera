#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# CORE_TRACKS (core.h): a build with 8 tracks (the plugin's, Andamento's) plays and saves the FM-1's projects as a
# 4-track build does. Each demo loads in both, plays SECS seconds and is saved: the two .tsp files must be byte for
# byte the same (tracks 5..8 are not in a .tsp and must not touch tracks 1..4). Needs build/gen (tools/build.py).
# usage: tests/core_tracks.sh [out-dir]
set -u
cd "$(dirname "$0")/.."
OUT=${1:-build/core_tracks}
CC=${CC:-clang}
SECS=${CORE_TRACKS_SECS:-6}
mkdir -p "$OUT"
[ -f build/gen/melodee_tables.h ] || { echo "core_tracks: build/gen missing (tools/build.py generate)"; exit 1; }
for n in 4 8; do
    $CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
        -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
        '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=$n tests/tsp_cross.c -o "$OUT/tracks$n" -lm ||
        { echo "core_tracks: the $n-track build failed"; exit 1; }
done
fails=0; n=0
for demo in demos/*.tsp; do
    d=$(basename "$demo" .tsp); n=$((n + 1))
    "$OUT/tracks4" save "$demo" "$SECS" "$OUT/$d.4.tsp" || { echo "FAIL $d: the 4-track build"; fails=$((fails + 1)); continue; }
    "$OUT/tracks8" save "$demo" "$SECS" "$OUT/$d.8.tsp" || { echo "FAIL $d: the 8-track build"; fails=$((fails + 1)); continue; }
    cmp -s "$OUT/$d.4.tsp" "$OUT/$d.8.tsp" || { echo "FAIL $d: $(cmp "$OUT/$d.4.tsp" "$OUT/$d.8.tsp" | head -1)"; fails=$((fails + 1)); }
done
echo "core_tracks: $n demos played ${SECS} s at 4 and 8 tracks, $fails differ"
[ "$fails" -eq 0 ]
