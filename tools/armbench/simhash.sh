#!/bin/sh
# Determinismo PC x ARM9 (ADR-36): compila tools/armbench/simhash.c p/ x86
# (gcc) e p/ ARM (devkitARM, flags do jogo) e compara as saidas. Requer
# unicorn (ver README). Uso: sh tools/armbench/simhash.sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd); R=$(cd "$HERE/../.." && pwd)
OUT=${TMPDIR:-/tmp}/omv_simhash; mkdir -p "$OUT/o"
CORE="calendar txt traits nations px_flag earth_mask world history city str settings wecon wdiplo wfort wlaw wstat wrank hflags"
gcc -std=gnu11 -O2 -w -I$R/host -I$R/include $(for c in $CORE; do echo $R/source/$c.c; done) $R/host/stub_px.c "$HERE/simhash.c" -o "$OUT/host"
"$OUT/host" > "$OUT/host.txt"
[ -n "$DEVKITARM" ] || . ~/devkitpro/env.sh
F="-march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork -O2 -ffunction-sections -fdata-sections -fno-tree-loop-distribute-patterns -DOMV_ARMBENCH -I$R/host -I$R/include -I$DEVKITARM/arm-none-eabi/include -w"
for c in $CORE; do arm-none-eabi-gcc $F -c $R/source/$c.c -o "$OUT/o/$c.o"; done
arm-none-eabi-gcc $F -c $R/host/stub_px.c -o "$OUT/o/stub_px.o"
arm-none-eabi-gcc $F -c "$HERE/simhash.c" -o "$OUT/o/simhash.o"
arm-none-eabi-gcc $F -c "$HERE/sys.c" -o "$OUT/o/sys.o"
arm-none-eabi-gcc -march=armv5te -mthumb -mthumb-interwork -nostartfiles -T "$HERE/link.ld" -Wl,--gc-sections "$OUT"/o/*.o -L$DEVKITARM/arm-none-eabi/lib/thumb -lc -lgcc -o "$OUT/arm.elf"
arm-none-eabi-objcopy -O binary "$OUT/arm.elf" "$OUT/arm.bin"
arm-none-eabi-nm "$OUT/arm.elf" > "$OUT/arm.sym"
python3 - "$OUT" > "$OUT/arm.txt" <<'PY'
import sys
from unicorn import *
from unicorn.arm_const import *
d=sys.argv[1];sym={}
for l in open(d+"/arm.sym"):
    p=l.split()
    if len(p)==3: sym[p[2]]=int(p[0],16)
BASE=0x02000000;SIZE=32<<20;HALT=0x0FFFFF00
mu=Uc(UC_ARCH_ARM,UC_MODE_THUMB);mu.mem_map(BASE,SIZE);mu.mem_map(HALT&~0xFFF,0x1000)
mu.mem_write(BASE,open(d+"/arm.bin","rb").read())
mu.reg_write(UC_ARM_REG_SP,BASE+SIZE-64);mu.reg_write(UC_ARM_REG_LR,HALT|1)
mu.emu_start(sym["main"]|1,HALT)
k=int.from_bytes(mu.mem_read(sym["outN"],4),"little")
print(bytes(mu.mem_read(sym["OUTB"],k)).decode("latin-1"),end="")
PY
echo "PC:";cat "$OUT/host.txt";echo "ARM9:";cat "$OUT/arm.txt"
cmp -s "$OUT/host.txt" "$OUT/arm.txt" && echo "IGUAL (deterministico PC x ARM9)" || { echo "DIVERGIU"; exit 1; }
