#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# CORE_TRACKS (core.h): a build with 8 tracks (the plugin's, Andamento's) plays and saves the FM-1's projects as a
# 4-track build does. Each demo loads in both and plays SECS seconds. The 8-track save adds XTRK (tracks 5..8,
# project_tsp.c); read by the 4-track build and saved again, it must be byte for byte the 4-track build's own save
# (tracks 5..8 must not touch tracks 1..4). Then tests/xtrk_test.c: tracks 5..8 with content save, load and save
# again the same, and the 4-track build reads that record; tests/track_copy_test.c: a track copied, pasted, swapped
# (platform/core/tess_track.c); tests/egrid_test.c: ENTRIES' arrangement grid obeyed, saved and read by the 4-track
# build; tests/abank_test.c: ATLAS's genre banks played, saved and read by the 4-track build; tests/dub_test.c: DROP
# and THROW (core.h TESS_DUB); tests/dubform_test.c: the dub FORMs and the grid's DROP column. Needs build/gen
# (tools/build.py).
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
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/quad8_test.c -o "$OUT/quad8_test" -lm ||
    { echo "core_tracks: quad8_test failed to build"; exit 1; }
"$OUT/quad8_test" > "$OUT/quad8_test.log" || { cat "$OUT/quad8_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/quad8_test.log"
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/track_copy_test.c -o "$OUT/track_copy_test" -lm ||
    { echo "core_tracks: track_copy_test failed to build"; exit 1; }
"$OUT/track_copy_test" demos/still-tide.tsp > "$OUT/track_copy_test.log" || { cat "$OUT/track_copy_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/track_copy_test.log"
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/dub_test.c -o "$OUT/dub_test" -lm ||
    { echo "core_tracks: dub_test failed to build"; exit 1; }
"$OUT/dub_test" demos/still-tide.tsp > "$OUT/dub_test.log" || { cat "$OUT/dub_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/dub_test.log"
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/dubform_test.c -o "$OUT/dubform_test" -lm ||
    { echo "core_tracks: dubform_test failed to build"; exit 1; }
"$OUT/dubform_test" demos/still-tide.tsp > "$OUT/dubform_test.log" || { cat "$OUT/dubform_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/dubform_test.log"
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/egrid_test.c -o "$OUT/egrid_test" -lm ||
    { echo "core_tracks: egrid_test failed to build"; exit 1; }
"$OUT/egrid_test" demos/still-tide.tsp "$OUT/egrid.tsp" > "$OUT/egrid_test.log" || { cat "$OUT/egrid_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/egrid_test.log"
"$OUT/tracks4" load "$OUT/egrid.tsp" "$OUT/egrid.4.tsp" || { echo "FAIL: the 4-track build refuses an 8-track record with ENTRIES' grid"; fails=$((fails + 1)); }
$CC -O1 -w -ffp-contract=off -Ifirmware/src -Iplatform/core -Ibuild/gen -DTESS_HOST_CLOCK=1 -DTESS_PLUGIN_IO=1 \
    -DMELODEE_USB_AUDIO=1 -DMELODEE_ACCESS=0 -DMELODEE_LOOK=1 -DMELODEE_FM6=1 -DMELODEE_PHYS=1 \
    '-DMELODEE_VERSION="plugin"' -DCORE_TRACKS=8 tests/abank_test.c -o "$OUT/abank_test" -lm ||
    { echo "core_tracks: abank_test failed to build"; exit 1; }
"$OUT/abank_test" "$OUT/abank.tsp" > "$OUT/abank_test.log" || { cat "$OUT/abank_test.log"; fails=$((fails + 1)); }
tail -1 "$OUT/abank_test.log"
"$OUT/tracks4" load "$OUT/abank.tsp" "$OUT/abank.4.tsp" || { echo "FAIL: the 4-track build refuses an 8-track record with ATLAS banks"; fails=$((fails + 1)); }
"$OUT/tracks4" load "$OUT/xtrk.tsp" "$OUT/xtrk.4.tsp" || { echo "FAIL: the 4-track build refuses an 8-track record with XTRK"; fails=$((fails + 1)); }
[ "$fails" -eq 0 ]
