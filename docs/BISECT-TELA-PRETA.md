# BISECT: tela preta (v0.8/v0.9) — RESOLVIDO

> Segue `docs/POSTMORTEM-TELA-BRANCA.md`. O sintoma era o mesmo do incidente 1
> (duas telas pretas desde o ligar), mas a **causa era outra** (travamento antes
> do primeiro render). Ver postmortem sec 10 (INCIDENTE 2). Este doc guarda o
> diagnostico e o metodo; a escada de ROMs virou apendice (nao foi precisa).

## Resultado: causa raiz (duas, na feature de bake)

O `main` chama `Px_Tick(g)` (bake do mapa) **antes** de `Px_Top`/`Px_Bot`. Um
travamento no bake = nenhum frame desenhado = fica o preto que o `Px_Init` pintou.

1. **Laco infinito no `LineP`** (`source/px_map.c`, Bresenham). So' parava em
   `x==x1 && y==y1`; em reta a **45 graus** (`|dx|==|dy|`) nunca chega no destino,
   `err` estoura o `int` e o `while(1)` nunca acaba. `BLD_FARM` (mapa padrao) desenha
   uma diagonal 45 -> bake do frame 0 travava -> **as duas telas pretas**.
2. **`SNAP` x `TileKey()==0`**: `SNAP[]` nasce zerado e o tile vazio (dono 0,
   planicie) tambem tem chave 0 -> `BakePass` pulava esses tiles -> ficavam
   transparentes (preto) no mapa. Nao trava; salpica o mapa de preto.

## Fix

- `LineP`: terminacao por contador (`passos = max(|dx|,|dy|)`), Bresenham canonico
  (`err=adx-ady`, condicoes `e2>=-ady` / `e2<=adx`). Sempre termina.
- `TileKey`: passa a `0x80000000u | ...` (bit 31 sempre setado) -> chave valida nunca
  e' 0 -> o `SNAP` zerado nao colide e todo tile e' baked no minimo 1x.

## Prova (sem melonDS): smoke host

`tools/hostpx/run.sh` compila `source/*.c` para x86 com stub de `<nds.h>`, sob
ASan/UBSan, e roda menu + jogo. Antes do fix: **trava** (UBSan em `px_map.c:48`).
Depois: termina com **0 pixel transparente** nas duas telas. E' a regressao oficial
de "tela preta" agora (ver `AGENTS.md` sec Testar e SDD sec 10).

## O que foi descartado (com evidencia, nao por chute)

1. **Bug de alpha/transparencia (ADR-13)**: `Px_Col` (px_core.c) seguia com
   `|BIT(15)`; `grep RGB15` em `source/` so' achava a excecao consciente do
   `Px_Init`. **Confirmado por valor**: dump dos tokens de `pal.h` mostrou todos com
   bit 15 (`0x9062`, `0x9c83`, `0xa699`...). Nao havia pixel transparente no desenho.
2. **ROM/empacotamento**: `ndstool -i` batia com o ROM do postmortem (ARM9 entry
   0x2004800 / RAM 0x2004000; ARM7 0x2380000). `make clean && make` dava md5
   identico ao incremental -> sem objeto velho com layout antigo de `GameState`.
3. **Codigo do outro dev (mundo)**: nao era chamado; `--gc-sections` removia
   (`nm` nao achava `World_`/`Nations_`/...). `__init_array` vazio. `.bss` = so' buffers
   de video (backTop/backBot/WORLD/G/SNAP).
4. **HEAD (git) NAO servia de "v0.7 boa"**: o `px_map.c` do commit 71b91ee esta
   truncado (`Lbrk2 elsewhere...`) e nao compila.

## Bug latente corrigido no caminho

`include/font.h` declarava `F_SERIF8`/`F_SERIF12`, mas `source/gen/font_data.c`
(gerado por `tools/genfont.py`) emitia `F8`/`F12`. Ninguem referencia os simbolos
ainda (a F1 nao esta ligada nas telas), entao o link nunca quebrou — mas quebraria
no primeiro uso. Nomes unificados no dado commitado e no gerador.

---

## Apendice: a escada de ROMs (nao foi necessaria; mantida para auditoria)

Fontes em `tools/bisect/`. Construidos na hora do diagnostico, ainda na raiz
(`*.nds` e' gitignored). Servem se o video voltar a falhar.

| ROM | O que testa | Leitura |
|---|---|---|
| `bisect_A_bmp.nds` | pipeline cru, caminho INDEPENDENTE (`videoSetMode`+`bgInit` Bmp16, igual ao `bmpoficial.nds`) | texto do console embaixo + tela de cima **VERMELHA opaca** = boot/toolchain/libnds OK |
| `bisect_B_pxcore.nds` | a PROPRIA engine do jogo: `px_core.c` + `pal.h` + Serifa | cima navy + texto dourado e acentos = engine+pal+fonte OK |
| `bisect_C_flat.nds` | o jogo INTEIRO (init+input+bake iguais), desenho trocado por fill opaco | cima **dourada** + baixo **carmesim** = video/init/logica OK; bug no desenho |
| `ohmyvictoria_ds.nds` | o jogo normal | — |

Interpretacao: **A preto** -> boot/toolchain/emulador. **A ok, B preto** ->
alpha/engine (`px_core`/`pal.h`). **A/B ok, C preto** -> init/input/bake do jogo.
**A/B/C ok, jogo preto** -> desenho (`px_hud.c`/`px_ui.c`/`px_map.c`).

> Dica: para "tela preta", `tools/hostpx/run.sh` responde a maior parte disso em
> segundos e sem emulador (pega travamento e pixel transparente). Use a escada so'
> quando o smoke host passar e o hardware ainda estiver preto (ai e' boot/VRAM/DS real).
