#!/bin/sh
# run_tests.sh — compila e roda os testes do nucleo do mundo no PC.
# Requer gcc (Linux, WSL ou MSYS2). Rode da raiz do repo: sh host/run_tests.sh
# Converte os .ppm de host/out/ em .png se houver python3 + Pillow.
set -e
mkdir -p host/out
gcc -std=gnu11 -O2 -Wall -Wextra -Ihost -Iinclude \
  source/calendar.c source/txt.c source/traits.c source/nations.c \
  source/px_flag.c source/earth_mask.c source/world.c source/history.c \
  source/city.c source/str.c source/settings.c source/wecon.c source/wdiplo.c source/wfort.c source/wlaw.c source/wstat.c source/wrank.c source/hflags.c \
  host/stub_px.c host/test_core.c -o host/out/test_core
./host/out/test_core
if command -v python3 >/dev/null 2>&1; then
  python3 - <<'PY' || true
import glob
try:
    from PIL import Image
except ImportError:
    raise SystemExit(0)
for p in glob.glob("host/out/*.ppm"):
    Image.open(p).save(p[:-4] + ".png")
PY
fi
