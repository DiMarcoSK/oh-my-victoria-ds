# POST-MORTEM: tela branca/preta no boot do jogo (v0.7)

> Data: 2026-10-16. Estado final: **RESOLVIDO** — `ohmyvictoria_ds.nds` roda com as duas
> telas funcionais no melonDS (confirmado pelo usuario). Fix de 1 linha + corrida de
> diagnosticos que provou o ROM perfeito byte a byte antes de achar o verdadeiro culpado.
> Este doc existe para: (a) qualquer agent futuro nao repetir a maratona de 10+ builds
> de teste, (b) servir de manual de debug de video NDS, (c) registrar aprendizados do
> toolchain rootless.

---

## 1. TL;DR (resumo em 30 segundos)

- **Sintoma:** todos os nossos `.nds` ficavam com tela branca (primeiros builds) ou preta
  (builds depois que o bg foi ativado) no melonDS; no DS real via nds-bootstrap aparecia
  "An error has occurred". ROMs oficiais de homebrew (dslink, ftpd) rodavam perfeitamente
  no mesmo setup.
- **Causa raiz:** no modo bitmap 16bpp da DS (`BgType_Bmp16`, formato `aBBBBBGGGGGRRRRR`),
  um pixel **sem o bit de alpha (bit 15) e TRANSPARENTE** — ele nao renderiza e mostra o
  backdrop (cor de fundo). `RGB15(r,g,b)` da libnds **nunca seta o bit 15**, entao 100% dos
  pixels do jogo eram transparentes. O jogo **rodava inteiro, invisivel**: logica, save,
  input tudo funcionando sobre um fundo preto.
- **Fix:** 1 linha em `source/px_core.c` — `Px_Col()` agora retorna `RGB15(r,g,b)|BIT(15)`.
  Todo pixel do jogo passa por `Px_Col`, entao o OR centralizado resolve de uma vez.
- **Por que demorou:** todos os ROMs de teste que construi para bisectar o problema
  (R5 "tela vermelha estatica", bmtest, etc.) **tinham o mesmo bug** — preenchiam o
  framebuffer com `RGB15(31,0,0)` = `0x001F`, sem alpha, portanto transparentes. O
  bisect nunca podia dar positivo porque o "padrao de controle" estava contaminado.

---

## 2. Sintomas completos (o que foi observado)

| Ambiente | Resultado | ROMs testados |
|---|---|---|
| melonDS (usuario) | Tela branca | versoes antigas do jogo, `r1test`, `t1_red`, `r4test`, `r4dk` |
| melonDS (usuario) | Tela preta | `bmtest_video` (ambas as telas), versoes do jogo com bg ativado |
| DS real + nds-bootstrap | "An error has occurred" | todos os nossos ROMs |
| DS real + nds-bootstrap | **Funciona** | dslink, ftpd (homebrew oficial de referencia) |
| melonDS (usuario) | **Funciona** | `bmpoficial.nds` (exemplo oficial recompilado por nos), `ohmyvictoria_ds.nds` (fix) |
| desmume (aqui no ambiente, SDL dummy) | rodava, mas screendump saia branco/blurred | inconclusivo (metodologia ruim, ver sec 6) |

Distribuicao branco vs preta importa e foi pista, mas so fez sentido no fim:

- **Branco** = display ligado, nenhuma layer de BG ativa (ou nos builds em que
  `REG_DISPCNT`/`VRAM*_CR` nao chegaram a ser configurados) -> melonDS mostra branco.
- **Preta** = BG de bitmap ativo mas todos os pixels transparentes -> backdrop preto
  (`BG_PALETTE[0]` nunca escrito = 0x0000).

---

## 3. O que foi descartado (com evidencia, nao por chute)

Tudo abaixo foi **provado** antes de ser descartado — vale como checklist se um dia
um ROM voltar a nao bootar:

1. **Empacotamento do ROM (`ndstool`)** — comparado byte a byte contra o dslink oficial:
   - Header MOD9 no blob ARM9 (`ROM@arm9offset+0x800`): identico em estrutura.
   - Loadlist do ARM9 e do ARM7: VMAs/LMAs batem com as secoes do ELF
     (`cmp` do `.main` extraido do ROM vs do ELF = `R5_MAIN_OK`).
   - ARM7: **byte-identico ao do ftpd oficial** (`ARM7_BYTE_IDENTICO_AO_FTPD`).
   - Bloco DSi7: **byte-identico ao do ftpd** (`DSI7_BYTE_IDENTICO`).
   - Bootstub ARM9 (`__ds9_bootstub` em `0x0e000800`): mesmo opcode inicial
     (`d3f021e3 msr CPSR_c,#0xD3`...) e mesmos MCRs de CP15 nos primeiros bytes.
2. **Entry point / load address** — header: `ARM9 entry 0x2004800`, `RAM 0x2004000`,
   `size 0x4000`; o codigo em `0x2004800` existe e e executavel (verificado no disasm).
3. **CRCs e flags do header** — CRC OK, secure area CRC16 consistente, logotipo ok
   (e o melonDS de qualquer forma faz direct boot com FreeBIOS).
4. **`.init_array` / construtores** — vazio nos ROMs oficiais tambem; nao e o caminho.
5. **MPU/TCM no bootstub** — sequencia de MCRs igual a dos ROMs que funcionam;
   nenhuma regiao MPU diferente.
6. **POWCNT (energia do LCD/GPU)** — melonDS reseta `PowerControl9 = 0x820F`
   (`NDS.cpp:418`), ou seja telas e 2D cores JA ligados no reset. Nenhum ROM precisa
   (nem deveria) mexer em POWCNT para aparecer video.
7. **Boot NTR vs DSi** — melonDS decide por firmware/BIOS (`NeedsDirectBoot` em
   `NDS.cpp:280`), nao pelo ROM; o fluxo de direct boot copia o logo, escreve os
   cart headers em `0x027FFxxx` e o loadlist. Nada dependia do nosso ROM ser especial.
8. **Falta de `_sbrk_r`/syscalls** — problema REAL que apareceu no caminho (link quebrou
   com stdio/malloc do exemplo oficial), mas era um problema de **link do toolchain
   rootless**, nao de runtime: resolvido com `-Wl,-u,_sbrk_r` + pre-marcao dos
   `_read_r/_write_r/_lseek_r/_close_r/_fstat_r/_isatty_r` e locks no Makefile (ver sec 7).
9. **`malloc` no jogo** — sempre foi zero; buffers de video sao estaticos em `.bss`.

---

## 4. A causa raiz, em detalhe

### 4.1 O formato Bmp16 da DS

No 2D da DS, um background bitmap 16bpp usa pixels de 15 bits com 1 bit de controle:

```
bit  15  14-10  9-5   4-0
     a   B      G     R
```

`a` (bit 15) = **opaco/visivel**. Se `a == 0`, o pixel e **transparente**: nao e
"preto", ele simplesmente nao e desenhado e o **backdrop** (a cor de fundo da tela,
definida por `BG_PALETTE[0]` na paleta principal) aparece no lugar.

Isso esta documentado no proprio header da libnds que usamos
(`libnds/include/nds/arm9/background.h`, linha ~451):

> `BgType_Bmp16` — Bitmap background with 16 bit color values of the form
> `aBBBBBGGGGGRRRRR` (**if 'a' is set the pixel will be rendered...if not the pixel
> will be transparent**)

### 4.2 Onde o bug nasceu

`RGB15(r,g,b)` da libnds gera `b | (g<<5) | (r<<10)` — **15 bits, bit 15 sempre 0**.
Nossa engine de pixel (`source/px_core.c` v0.5-0.7) fazia:

```c
u16 Px_Col(u8 r,u8 g,u8 b){return RGB15(r,g,b);}   // BUG: alpha=0 -> transparente
```

Como **todo** pixel (terreno, edificios, texto, barras, bordas, menus) passa por
`Px_Col`, o framebuffer inteiro ficava "transparente". O `Px_Clear`, `Px_Rect`,
`Px_Text`, tiles, tudo: invisivel.

### 4.3 Por que as telas ficavam BRANCAS nos builds antigos e PRETAS nos novos

- Nos builds antigos (antes da v0.7 de video), parte da configuracao de display
  (`videoSetMode`/`vramSetBank`) nao completava o caminho — sem layer ativa, o
  melonDS renderiza branco.
- Depois, com `bgInit(3, BgType_Bmp16, ...)` + VRAM banks corretos, o layer existia
  (logo display ligado = tela PRETA), e os pixels transparentes deixavam passar o
  backdrop preto.

**Consequencia importantissima:** o jogo nunca travou. Logica de dia, economia, save,
input, tudo rodava. So a saida visual era 100% transparente.

---

## 5. Por que o bisect demorou tanto (licao metodologica)

Eu construi uma escada de ROMs cada vez menores (R1 -> R5 -> R6 -> bmtest) para isolar
"boot" vs "video" vs "VRAM mapping". A escada era metodologicamente correta, mas
**todos os degraus tinham o mesmo defeito do degrau de cima**:

- `r5test` (o "tela vermelha estatica"): disassembly de `main` em `0x20013f0`
  confirma — ele escreve `REG_DISPCNT = 0x0403` (MODE5|BG3), `VRAM_A_CR = 0x81`
  (enable + MAIN_BG), e preenche `0x06000000..0x06018000` com **`0x001F`** —
  vermelho puro `RGB15(31,0,0)`, SEM bit 15. Pixels transparentes -> backdrop.
  O teste que deveria "acender vermelho" acende... nada, do mesmo jeito que o jogo.
- `bmtest_video` (console + bitmap via libnds): mesmo defeito, agora visualizavel como
  "preta em ambas" porque o layer estava de fato ativo.
- `r6test` (paleta pura, sem VRAM): teste correto e sem o bug, mas **MODE0 sem BG
  nenhum ativo** — resultado inconclusivo no melonDS (ele nao renderiza backdrop
  util nesse estado).

Ou seja: o controle do experimento estava contaminado pelo mesmo fator que eu tentava
medir. **Aula: ao bisectar um bug de renderizacao, o teste minimo tem que ser gerado por
um caminho DIFERENTE do codigo que falha** — no caso, o exemplo oficial da devkitPro.

### 5.1 O degrau que quebrou o impasse: `bmpoficial.nds`

Recompilei o exemplo **oficial** da devkitPro (`16bit_color_bmp`) com o meu toolchain,
sem mudar nada do pipeline (só troquei o logo por fill vermelho):

```c
videoSetMode(MODE_5_2D);
videoSetModeSub(MODE_0_2D);
vramSetBankA(VRAM_A_MAIN_BG);
consoleDemoInit();
bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
for(int i=0;i<256*192;i++) BG_GFX[i]=RGB15(31,0,0);
```

Resultado reportado pelo usuario: **texto "Hello DS devers / 16 bit bitmap demo"
aparece embaixo; tela de cima preta**.

Isso dividiu o universo ao meio, na hora:

- **Texto apareceu** -> boot OK, toolchain OK, libnds OK, sub screen OK, execução de
  `main` OK. Descartado de vez: bootstub, loadlist, MPU, entry point, ndstool, BIOS.
- **Texto e BG tile com paleta** -> cores de paleta nao precisam de bit de alpha
  (transparencia em tile 4bpp e por indice de paleta, indice 0). Por isso o texto via.
- **Bitmap preto** -> o BG bitmap estava ativo (tela ligada = preta, nao branca) mas os
  pixels nao renderizavam -> **transparentes** -> bit 15 ausente. Confirma o formato
  `aBBBBBGGGGGRRRRR` do `background.h`.

Segui o rastro e corrigi. O jogo com o fix apareceu imediatamente (menu azul com titulo
dourado, duas telas, jogavel).

---

## 6. O fix (e por que ele e centralizado)

`source/px_core.c` (antes/depois):

```c
// ANTES (bug):
u16 Px_Col(u8 r,u8 g,u8 b){return RGB15(r,g,b);}

// DEPOIS (fix):
// BIT(15)=alpha opaco: em Bmp16 (aBBBBBGGGGGRRRRR) pixel sem 'a' e'
// TRANSPARENTE (vira backdrop). RGB15() puro nunca seta o bit -> tela preta.
// Todo pixel do jogo passa por aqui, entao OR uma vez resolve tudo.
u16 Px_Col(u8 r,u8 g,u8 b){return (u16)(RGB15(r,g,b)|BIT(15));}
```

Por que centralizado e seguro:

- `Px_Col` e a **unica** fabrica de cor do jogo (84 chamadas em `px_tiles.c`,
  `px_font.c`, `px_core.c` — verificado por grep). Nenhum outro lugar monta cor de
  pixel "na mao".
- Custo zero: o OR e compilado para uma instrucao `orr` imediata que oThumb absorve no
  fluxo; 200KB/frame x 1x/15 frames nao sente 1 OR por pixel.
- Excecoes conscientes: `Px_Init` ainda usa `RGB15(0,0,0)` puro para o fundo do
  **backbuffer** — e inofensivo: preto sem alpha = backdrop preto = o mesmo visual, e
  qualquer pixel redesenhado depois ja sai opaco. (Se um dia quiser escrever preto
  "opaco" sobre coisas, use `Px_Col(0,0,0)`.)

### 6.1 ROMs de confirmacao que ficaram prontos (nao foram necessarios)

Estavam na pasta quando o fix confirmou, mantidos aqui como trilha de auditoria:

- `r7test_alpha.nds` — MODE3 cru com alpha (`0x83E0/0x801F` piscando).
- `t6test_opaco.nds` — MODE5 via libnds com pixel opaco.
- `bmpoficial.nds` — exemplo oficial (o degrau decisivo).

Se um dia o video voltar a falhar, rode esses 3 na ordem: eles isolam
"pipeline cru" -> "libnds" -> "codigo do jogo" em 5 minutos.

---

## 7. Bônus descoberto no caminho (fixes reais que ficam)

1. **Link do toolchain rootless com stdio/malloc:** `consoleDemoInit`/`iprintf` puxam
   stdio -> malloc -> `_sbrk_r` + locks newlib. O Makefile agora pre-marca
   (`-Wl,-u,`) `_sbrk_r`, `_read_r`, `_write_r`, `_lseek_r`, `_close_r`, `_fstat_r`,
   `_isatty_r` e os 4 `__libc_lock_*` para o grupo de libs resolver do
   `libsysbase.a`. Sem isso, qualquer codigo que use stdio nao linka no toolchain
   local (o jogo em si continua zero-malloc, mas a rota precisa existir).
2. **Provas de integridade de empacotamento:** existem comandos reprodutiveis para
   conferir qualquer ROM futuro byte a byte contra o ELF (ver sec 3 item 1). Reuso
   garantido em CI manual.
3. **Screendump do desmume via SDL dummy nao serve** como evidencia (frames saem
   brancos/blurred mesmo para ROMs que funcionam no hardware). Evidencia de video tem
   que vir do usuario no melonDS/DS real ou de gdb com breakpoints comprovados.
4. **melonDS funciona como bisturi:** texto do console (BG tile) vs bitmap (Bmp16)
   separam "boot/execucao" de "formato de pixel" em uma unica rodada. Esse par
   `consoleDemoInit + bgInit Bmp16` e o teste de bisect mais barato e decisivo que
   existe para esse sintoma.

---

## 8. Status de validacao e proximos passos

| Item | Status |
|---|---|
| melonDS: jogo completo, 2 telas, jogavel | **CONFIRMADO pelo usuario** (pos-fix) |
| DS real via nds-bootstrap: re-testar | **PENDENTE** — rode o `ohmyvictoria_ds.nds` novo no cartao e me diga se passou da tela de erro |
| desmume aqui no ambiente | nao usar como oraculo de video (sec 7 item 3) |

Checklist se o DS real ainda der "An error has occurred" ( improvavel agora):
1. Confirme que o arquivo no cartao e o **novo** (data/hora do build de 2026-10-16,
   143360 bytes, ou simplesmente recopie).
2. Rode `bmpoficial.nds` no DS real: se o texto aparecer, o nds-bootstrap esta OK e
   qualquer falha e especifica do jogo (me traga a tela de erro com `PC=`).
3. Se nem o bmpoficial rodar, o problema e do bootstrap/setup do cartao, nao do ROM.

---

## 9. ADR-13 (registro no SDD)

Registrado como **ADR-13** no `docs/SDD.md`: pixel Bmp16 exige `|BIT(15)` (alpha opaco);
`Px_Col` e o ponto unico de fabrica de cor; NUNCA use `RGB15()` cru em pixel de
framebuffer. Ver sec 4/6 deste doc para a prova.

---

*Licoes de ouro resumidas: (1) leia o comentario do proprio header da lib antes de
assumir o formato; (2) em bisect de video, o teste minimo deve vir de um caminho
independente (exemplo oficial); (3) tela "funciona mas invisivel" existe, e ela cheira
a transparencia/alpha; (4) teste com o mesmo bug do sistema nao valida nada.*

---

## 10. INCIDENTE 2 (v0.8): "tela preta" de novo — mas a causa NAO era alpha

**Sintoma:** as **duas** telas pretas desde o ligar no melonDS (v0.8 touch-first).
Identico ao incidente 1 na cara. **NAO era o mesmo bug.**

### 10.1 Por que o sintoma enganava

Tela preta = "BG de bitmap ativo + todos os pixels transparentes" (sec 2). Mas ha'
uma segunda maneira de chegar na MESMA imagem: **o jogo nunca chega a desenhar**.
O `Px_Init` pinta o backbuffer de preto (`RGB15(0,0,0)`, a excecao consciente da
sec 6) e faz o primeiro DMA. Se o primeiro frame travar ANTES do bloco de render,
as duas telas ficam com esse preto e o resultado e' indistinguivel do bug de alpha.

### 10.2 A causa raiz (duas, na mesma feature de bake)

O loop do `main` faz `Px_Tick(g)` (o *bake* do buffer-mundo) **antes** de
`Px_Top`/`Px_Bot`. Logo, um travamento no bake = nenhum render = tela preta.

1. **Laco infinito no `LineP` (Bresenham de `px_map.c`).** A versao antiga so'
   parava quando `x==x1 && y==y1`; em retas a **45 graus** (`|dx|==|dy|`) ela nunca
   chega no destino, `err` estoura o `int` (UBSan acusa *signed integer overflow*)
   e o `while(1)` roda para sempre. O `BLD_FARM` (presente no mapa padrao) desenha
   exatamente uma diagonal 45 — o bake do frame 0 travava ali.
2. **`SNAP` x `TileKey()` = 0.** O cache de bake `SNAP[]` nasce zerado em `.bss`
   e `TileKey()` de um tile "vazio" (dono 0, `TER_PLAINS`) tambem e' 0. Como
   `BakePass` pula o tile quando `SNAP==key`, esses tiles **nunca eram baked** ->
   ficavam transparentes (preto) no mapa. Isso nao trava: deixa o mapa salpicado
   de preto (no smoke host eram ~20k pixels do fundo).

### 10.3 O fix

- `LineP` reescrito com **terminacao garantida por contador** (`passos = max(|dx|,|dy|)`,
  `for i in [0..passos]`), Bresenham canonico (`err=adx-ady`, `e2>=-ady` / `e2<=adx`).
  Termina sempre, `err` fica limitado, desenho correto (ver sec 10.4).
- `TileKey()` passa a devolver `0x80000000u | ...` (bit 31 sempre setado): uma chave
  valida nunca vale 0, entao o `SNAP` zerado do boot nao colide e todo tile e' baked
  pelo menos 1x.

### 10.4 Como foi provado (sem melonDS): o smoke host de render

Nao havia emulador nem gdb no ambiente (postmortem sec 7 item 3 nao se aplicava).
Em vez de inventar teoria, compilamos **o jogo de verdade para x86** com um stub
minimo de `<nds.h>` (`tools/hostpx/`), sob **ASan + UBSan**, rodando menu e jogo:

- **Antes do fix:** `timeout` → o processo **travava** no frame 0; UBSan apontava
  `source/px_map.c:48` (*signed integer overflow* em `err*2`) — o laco infinito.
- **Depois do fix:** termina, e a checagem de pixel diz **0 transparente** nas duas
  telas, no menu E no jogo.

Esse teste (`tools/hostpx/run.sh`) fica no repo como **regressao de "tela preta"**:
falha se travar (timeout) ou se sobrar pixel com bit 15 = 0. Roda em segundos, sem
ROM, sem emulador.

### 10.5 Licao nova (soma as 4 da sec 9)

**"Tela preta" tem (pelo menos) duas familias: pixel transparente (ADR-13) e "nunca
desenhou" (trava/excecao antes do render).** Antes de caçar alpha, prove que o bloco
de render *roda* — o smoke host faz isso de graca. E: o `Px_Init` preto e' um
*tell* — se o preto e' uniforme nas duas telas desde o ligar, suspeite de travamento
antes do primeiro `Px_Top`, nao de transparencia. Ver ADR-21 no `docs/SDD.md`.
