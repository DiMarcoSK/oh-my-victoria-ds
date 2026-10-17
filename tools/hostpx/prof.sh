#!/bin/sh
# Benchmark de render no host (tools/hostpx/prof.c). -O2 sem sanitizer:
# o numero e' relativo (antes x depois no mesmo PC), nao o tempo do DS.
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/omv_px_prof
mkdir -p "$OUT"
SRC=$(ls "$ROOT"/source/*.c | grep -v '/main.c$')
gcc -std=gnu11 -O2 -DOMV_PROF -I"$ROOT/tools/hostpx" -I"$ROOT/include" \
    $SRC "$ROOT"/source/gen/*.c \
    "$ROOT/tools/hostpx/stub.c" "$ROOT/tools/hostpx/prof.c" -o "$OUT/prof"
"$OUT/prof"
