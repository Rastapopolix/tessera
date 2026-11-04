#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Tessera as an .rfm1 (RemoteFM1's firmware file: FM1VST's docs/RFM1.md): web/rfm1/ is the core's glue, built by
# FM1VST's fm1_add_rfm1 with Emscripten. FM1VST is not changed: build/rfm1/tree/ is a tree of links to its folders,
# with core-glue/ holding only its common/ and this glue (core-glue/tessera) and cores/tessera this repository.
#   web/rfm1/build.sh [--version X.Y[.Z]]     (default: the v* tag at HEAD, else the newest v* tag + "-dev")
# Needs: an FM1VST checkout with fm1_add_rfm1 ($FM1VST, default ~/src/FM1VST), Emscripten (emcc on the PATH, or
# emsdk in $EMSDK, default ~/emsdk), CMake 3.22 or newer, Python 3 with Pillow (the generate step: tools/build.py).
# Makes build/rfm1/tessera-<version>.rfm1, checks it (tools/rfm1_check.py) and runs it (rfm1_run).
set -e
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
VERSION=
[ "$1" = --version ] && VERSION=$2
[ -n "$VERSION" ] || VERSION=$(git describe --tags --exact-match --match 'v[0-9]*' 2>/dev/null | sed 's/^v//')
[ -n "$VERSION" ] || VERSION=$(git tag -l 'v[0-9]*' --sort=-v:refname | head -n 1 | sed 's/^v//')-dev
FM1VST=${FM1VST:-$HOME/src/FM1VST}
[ -f "$FM1VST/cmake/FM1Rfm1.cmake" ] || { echo "no FM1VST with fm1_add_rfm1 at $FM1VST (set FM1VST)"; exit 1; }
command -v emcc >/dev/null 2>&1 || PATH="${EMSDK:-$HOME/emsdk}/upstream/emscripten:$PATH"   # emsdk, not activated
command -v emcc >/dev/null 2>&1 || { echo "emcc not found: install Emscripten (emsdk)"; exit 1; }
OUT=build/rfm1
TREE=$OUT/tree
rm -rf "$TREE"
mkdir -p "$TREE/core-glue" "$TREE/cores"
for f in "$FM1VST"/* "$FM1VST"/.git*; do
    case $(basename "$f") in core-glue|cores|build*) ;; *) ln -s "$f" "$TREE/" ;; esac
done
ln -s "$FM1VST/core-glue/common" "$TREE/core-glue/common"
ln -s "$ROOT/web/rfm1" "$TREE/core-glue/tessera"
ln -s "$ROOT" "$TREE/cores/tessera"
cmake -S "$TREE" -B "$OUT/cmake" -DFM1_RFM1_ONLY=ON -DFM1_PUBLIC_BUILD=ON -DFM1_ONLY_CORES=tessera \
      -DTESSERA_VERSION="$VERSION" -DCR_PYTHON="$(command -v python3)" >"$OUT/cmake.log" || { cat "$OUT/cmake.log"; exit 1; }
cmake --build "$OUT/cmake" --target rfm1_tessera rfm1_run -j"$(nproc 2>/dev/null || echo 4)"
F=tessera-$VERSION.rfm1
cp "$OUT/cmake/rfm1/$F" "$OUT/"
python3 "$FM1VST/tools/rfm1_check.py" "$OUT/$F"
"$OUT/cmake/rfm1_run" "$OUT/$F"
echo "rfm1     $OUT/$F  $(wc -c < "$OUT/$F") B"
