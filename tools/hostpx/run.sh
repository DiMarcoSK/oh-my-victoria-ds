#!/bin/sh
# Smoke de render no host: compila source/*.c com o stub nds.h (sem libnds),
# roda o menu e o jogo sob ASan/UBSan e falha se travar (timeout) ou sobrar
# pixel transparente (classe "tela preta", ADR-13). Ver docs/POSTMORTEM-TELA-BRANCA.md sec 10.
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/omv_px_smoke
mkdir -p "$OUT"
SRC=$(ls "$ROOT"/source/*.c | grep -v '/main.c$')
gcc -std=gnu11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$ROOT/tools/hostpx" -I"$ROOT/include" \
    $SRC "$ROOT"/source/gen/*.c \
    "$ROOT/tools/hostpx/stub.c" "$ROOT/tools/hostpx/main.c" -o "$OUT/run"
timeout 60 "$OUT/run"
