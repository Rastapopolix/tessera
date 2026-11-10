#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# What the screen reader (MELODEE_ACCESS, ui_speak.c) says, checked with fm1-reader's audit (its tools/audit.mjs):
# every page with every engine and MODE, each knob, menu row and list value (tests/speak_test.c dump), and every
# message the firmware shows (ui_message / ui_say in the sources), put into words with firmware/words.tsv.
#   tools/speak_audit.sh [../fm1-reader] [--spoken | --all]   (after ./build.sh or the host suite's generate step)
# Writes build/speak/said.tsv and build/speak/words/<id>.json; exits 1 when the audit finds errors.
set -e
cd "$(dirname "$0")/.."
READER=../fm1-reader
case "$1" in --*|"") ;; *) READER=$1; shift ;; esac
OUT=build/speak
mkdir -p $OUT
python3 tools/gen_words.py firmware/words.tsv $OUT/words $OUT/ann_words.h >/dev/null
${CC:-cc} -O1 -w -Ibuild/gen -Ifirmware/src -Itests -DMELODEE_USB_AUDIO=1 -DMELODEE_LOOK=1 -DMELODEE_ACCESS=1 \
    -o $OUT/speak_test tests/speak_test.c -lm
$OUT/speak_test dump $OUT/dump.tsv
grep -hoE 'ui_(message|say)\("[^"]*"(, *"[^"]*")?' firmware/src/*.c |
    sed -E 's/ui_(message|say)\(//; s/", *"/ /; s/"//g; s/[][]//g; s/^/MSG\t/' >> $OUT/dump.tsv
printf 'MSG\t%s\n' "MIDI IN -> T2" "HOLD GLO QUICK" "HOLD SCL QUICK" "PLAY" "STOP" "USER1 F# IN" "USER4 C OUT" >> $OUT/dump.tsv   # (made as they go)
printf 'MSG\t%s\n' "SCOPE 2 BARS, CYCLING 2 A BAR, LOW -60 HIGH 60 %" "SCOPE 16 BARS, CYCLING 3 TIMES, LOW -80 HIGH 80 %" \
    "SCOPE 1 BAR, GATE OPENS 4 A BAR, OPEN NOW, LOW 0 HIGH 100 %" "SCOPE 1 BAR, GATE OPENS 0 TIMES, SHUT NOW, LOW 0 HIGH 0 %" \
    "SCOPE 64 BARS, RISING, LOW 0 HIGH 40 %" "SCOPE 4 BARS, FALLING, LOW -20 HIGH 0 %" "SCOPE 16 BARS, WANDERING, LOW -5 HIGH 30 %" \
    "SCOPE 1 BAR, STEADY, LOW 25 HIGH 25 %" "SCOPE 1 BAR, STILL, LOW 0 HIGH 0 %" >> $OUT/dump.tsv   # (MODS' scope, ui_speak.c sp_scope)
grep -v '^#' $OUT/dump.tsv | sort -u > $OUT/said.tsv
WORDS=$(sed -n 's/.*words\/\([0-9a-f]*\.json\).*/\1/p' $OUT/ann_words.h)
exec node "$READER/tools/audit.mjs" $OUT/said.tsv --words $OUT/words/$WORDS --allow firmware/words_allow.txt "$@"
