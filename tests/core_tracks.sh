#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# CORE_TRACKS (core.h): a build with 8 tracks (the plugin's, Andamento's) plays and saves the FM-1's projects as a
# 4-track build does. Each demo loads in both and plays SECS seconds. The 8-track save adds XTRK (tracks 5..8,
# project_tsp.c); read by the 4-track build and saved again, it must be byte for byte the 4-track build's own save
# (tracks 5..8 must not touch tracks 1..4). Then tests/xtrk_test.c: tracks 5..8 with content save, load and save
# again the same, and the 4-track build reads that record. Needs build/gen (tools/build.py).
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
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/core_tracks_play.c -o "$OUT/play8" -lm ||
    { echo "core_tracks: the 8-track play test failed to build"; exit 1; }
fails=0; n=0
for demo in demos/*.tsp; do
    d=$(basename "$demo" .tsp); n=$((n + 1))
    "$OUT/tracks4" save "$demo" "$SECS" "$OUT/$d.4.tsp" || { echo "FAIL $d: the 4-track build"; fails=$((fails + 1)); continue; }
    "$OUT/tracks8" save "$demo" "$SECS" "$OUT/$d.8.tsp" || { echo "FAIL $d: the 8-track build"; fails=$((fails + 1)); continue; }
    "$OUT/tracks4" load "$OUT/$d.8.tsp" "$OUT/$d.84.tsp" || { echo "FAIL $d: the 4-track build refuses the 8-track save"; fails=$((fails + 1)); continue; }
    cmp -s "$OUT/$d.4.tsp" "$OUT/$d.84.tsp" || { echo "FAIL $d: $(cmp "$OUT/$d.4.tsp" "$OUT/$d.84.tsp" | head -1)"; fails=$((fails + 1)); }
    "$OUT/play8" "$demo" || { echo "FAIL $d: tracks 5..8 after the load (above)"; fails=$((fails + 1)); }
done
echo "core_tracks: $n demos played ${SECS} s at 4 and 8 tracks, $fails differ (or leave tracks 5..8 unplayable)"
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/xtrk_test.c -o "$OUT/xtrk_test" -lm ||
    { echo "core_tracks: xtrk_test failed to build"; exit 1; }
"$OUT/xtrk_test" demos/still-tide.tsp "$OUT/xtrk.tsp" > "$OUT/xtrk_test.log" || { cat "$OUT/xtrk_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/xtrk_test.log"
"$OUT/tracks4" load "$OUT/xtrk.tsp" "$OUT/xtrk.4.tsp" || { echo "FAIL: the 4-track build refuses an 8-track record with XTRK"; fails=$((fails + 1)); }
[ "$fails" -eq 0 ]
