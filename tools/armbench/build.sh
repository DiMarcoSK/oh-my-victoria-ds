#!/bin/sh
# tools/armbench/build.sh: compila o JOGO INTEIRO (menos main.c) + o cenario
# de tools/hostpx/prof.c com o devkitARM (armv5te/Thumb + ARM na "ITCM", as
# mesmas flags do Makefile) num ELF "nu" p/ o emulador. Uso: sh build.sh [saida]
set -e
HERE=$(cd "$(dirname "$0")" && pwd); R=$(cd "$HERE/../.." && pwd)
OUT=${1:-${TMPDIR:-/tmp}/omv_armbench}; mkdir -p "$OUT/o"
[ -n "$DEVKITARM" ] || . ~/devkitpro/env.sh
F="-march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork -O2 -ffunction-sections -fdata-sections -fno-tree-loop-distribute-patterns -DOMV_PROF -DOMV_ARMBENCH -I$R/tools/hostpx -I$R/include -I$DEVKITARM/arm-none-eabi/include -w"
for c in $(ls "$R"/source/*.c "$R"/source/gen/*.c | grep -v '/main.c$') "$R/tools/hostpx/stub.c" "$R/tools/hostpx/prof.c" "$HERE/sys.c"; do
 arm-none-eabi-gcc $F -c "$c" -o "$OUT/o/$(basename "$(dirname "$c")")_$(basename "$c" .c).o"
done
arm-none-eabi-gcc -march=armv5te -mthumb -mthumb-interwork -nostartfiles -T "$HERE/link.ld" -Wl,--gc-sections "$OUT"/o/*.o -L$DEVKITARM/arm-none-eabi/lib/thumb -lc -lgcc -o "$OUT/game.elf"
arm-none-eabi-objcopy -O binary "$OUT/game.elf" "$OUT/game.bin"
arm-none-eabi-nm "$OUT/game.elf" > "$OUT/game.sym"
echo "$OUT/game.bin"
