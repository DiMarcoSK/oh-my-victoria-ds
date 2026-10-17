# tools/armbench — o jogo num ARM emulado, para medir sem DS

O PC é ~50–100× mais rápido que o ARM9 do DS e, pior, tem divisor de hardware,
SIMD e cache enorme: o que pesa no DS (divisão em software, `memcpy` byte a byte,
spill de registrador) quase não aparece no `tools/hostpx/prof.sh`. Aqui o **jogo
inteiro** (menos `main.c`) é compilado pelo devkitARM com as mesmas flags do
`Makefile` (Thumb + trechos ARM do `hot.h`) e roda no emulador de CPU
[unicorn](https://www.unicorn-engine.org/). O cenário é o mesmo do
`tools/hostpx/prof.c` (pan em cada zoom, diagonal, parado, velocidade 5, com
frente de guerra) e o relógio do `prof.h` passa a ser o **contador de instruções
ARM executadas**.

```sh
pip install unicorn            # 1x (ex.: pip install --target=/tmp/pylib unicorn)
sh tools/armbench/build.sh /tmp/omv_armbench
PYTHONPATH=/tmp/pylib python3 tools/armbench/run.py /tmp/omv_armbench
```

Leitura do relatório: "us" = instruções/2 em ticks de 33,5 MHz, isto é, o tempo
no DS **se cada instrução custasse 1 ciclo a 67 MHz**. É um piso: o ARM946E-S real
gasta ~1,3–1,6 ciclos por instrução (carga, multiplicação, desvio) mais a espera da
RAM principal. Use para comparar ANTES × DEPOIS e achar o gargalo; o número final
é o da caixa do `make PROF=1` no melonDS/DS.

Detalhes que já morderam:

- O unicorn guarda blocos já traduzidos **sem** os hooks instalados depois:
  o `run.py` roda a geração do mundo sem contar e, ao ligar o contador, chama
  `ctl_remove_cache` (sem isso a divisão da libgcc, usada na geração, some da conta).
- `sys.c` dá à newlib do devkitARM as chamadas reentrantes (`_write_r`,
  `_sbrk_r`, ...); o `printf` do relatório vai para um buffer que o `run.py` imprime.
- Nada de vídeo de verdade: o stub de `tools/hostpx/nds.h` faz as "VRAMs" serem
  arrays. Mede CPU, não DMA nem espera de barramento.
