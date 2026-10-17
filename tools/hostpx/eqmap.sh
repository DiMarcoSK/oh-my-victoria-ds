#!/bin/sh
# Prova de equivalencia do pintor do mapa (ADR-36): compila o px_mapr.c da
# revisao <ref> do git (simbolos O_*) e o atual no mesmo binario e compara
# pixel a pixel. Uso: tools/hostpx/eqmap.sh [ref]   (padrao: HEAD)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
REF=${1:-HEAD}
OUT=${TMPDIR:-/tmp}/omv_eqmap; mkdir -p "$OUT"
git -C "$ROOT" show "$REF:source/px_mapr.c" > "$OUT/ref_mapr.c"
F="-std=gnu11 -O2 -w -I$ROOT/host -I$ROOT/include"
CORE="calendar txt traits nations px_flag earth_mask world history city str settings wecon wdiplo hflags"
gcc $F -DPxM_Update=O_Update -DPxM_Blit=O_Blit -DPxM_Overlay=O_Overlay -DPxM_Invalidate=O_Inv \
    -DPxM_Scratch=O_Scratch -DPxM_Key=O_Key -DPxM_KeyAt=O_KeyAt -c "$OUT/ref_mapr.c" -o "$OUT/ref.o"
gcc $F -c "$ROOT/source/px_mapr.c" -o "$OUT/cur.o"
gcc $F "$ROOT/tools/hostpx/eqmap.c" "$OUT/ref.o" "$OUT/cur.o" "$ROOT/host/stub_px.c" \
    $(for c in $CORE; do echo "$ROOT/source/$c.c"; done) -o "$OUT/eqmap"
"$OUT/eqmap"
