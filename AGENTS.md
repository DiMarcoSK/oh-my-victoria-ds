# AGENTS.md — OH MY VICTORIA DS!

Contexto para qualquer agent que abrir este repo. Leia inteiro antes de editar (2 min).

## O que e

Grande estrategia estilo Victoria para **Nintendo DS** (homebrew, C puro, libnds/devkitARM).
Saida: `ohmyvictoria_ds.nds` (melonDS ou DS real com flashcard).

- **Visao (`DESIGN.md`):** Victoria 3 no bolso. Mapa politico estilo Victoria 3 com
  territorio pintado e frentes que avancam celula a celula como em frontwars.io. O jogador
  escolhe **mapa aleatorio** ou **mapa-mundi de 1800** com >=150 paises historicos e povos
  indigenas, cada um com traits e flavor historicos, e a historia real acontecendo ate 2025
  (depois, Modo Eterno) — a menos que o mundo divirja.
- **Hoje (v0.7):** tempo continuo, mapa 32x24, voce contra 2 bots, vitoria com 600 de
  prestigio ou 200 tiles. 8 bens em cadeia, 23 obras, 4 POPs, mercado interno/externo,
  tarifa, uniao aduaneira, 3 governos, clima, eventos, marcha com atraso.
- **Plano ordenado (`docs/ROADMAP.md`, imagem `docs/roadmap.png`):** o que falta e em que
  ordem — baseline honesto, fases e marcos, dependencias e riscos. Agrega as fases F1-F8
  (SDD sec 14.9) e os marcos M1-M7 (`DESIGN.md` sec 9).

## Estado atual (2026-10-16)

| Versao | Status |
|---|---|
| v0.7 | **Jogavel.** Descrita nas secs 1-10 do SDD. Bug da tela preta resolvido (ADR-13). |
| v0.8 touch-first | **Em andamento no codigo**, ainda nao documentada nas secs 1-10: `include/ui.h`, `source/ui.c`, `source/px_ui.c`, `source/px_hud.c`, `source/px_map.c` (mapa "cozido" em buffer 512x384; tela baixo tocavel). `source/px_tiles.c` foi removido; `input.c` e caneta-first (pan + tap por tabela). Build verde. **Tela preta de novo (mesmo sintoma, outra causa: laco infinito no `LineP` do bake + `SNAP`/`TileKey()==0`) RESOLVIDA em 2026-10-16 — ADR-21, postmortem sec 10.** |
| v0.9 UI da concept art | **F1 (Fundacao) em andamento no codigo:** `include/pal.h` (tokens), `source/settings.c`, `source/str.c` (PT/EN), `source/px_art.c` (placeholder de arte), `source/px_font2.c` + `source/gen/font_data.c` (Serifa 8/12; preview `docs/font_preview.png`) e `g->screen`/`enum Screen` (ADR-17, migrou `inMenu`; `SAVE_VER` 63). Telas F2+ planejadas. Contrato: **SDD sec 14** (fases F1-F8). |
| v0.9+M1 mapa-mundi jogavel | **Jogavel no codigo (2026-10-17):** Menu -> Modo -> Preparar -> Nacao -> `SCR_WORLD` (`source/menu.c`, `px_menu.c`, `wgame.c`, `px_world.c`). Mapa politico suave e realista em 5 zooms (pintura bilinear + cache incremental, `px_mapr.c`, ADR-24), cidades com obras/niveis/fundacao e cena viva por cultura (`city.c`, `px_city.c`, ADR-25, DESIGN 4.7), economia de 9 bens em 4 mercados com portos, rotas e marinha (`wecon.c`, ADR-26, DESIGN 4.8), ideologia com bandeira, revolucoes, diplomacia da IA, despachos/decisoes e ambicoes (`wdiplo.c`, ADR-27), Gabinete (X, `px_ledger.c`), guerra por %, desembarque, paz, recrutar, historia ate 2025. SDD ADR-23. **Controles (ADR-30/31, DESIGN 3.6):** setas movem o mapa com inercia pixel a pixel, mira no centro, A abre a folha de acoes, tela de baixo so' caneta, bolinha do ranking (`px_rank.c`). Balanco fiscal/populacao/alfabetizacao (ADR-32), rotas pelo mar (`px_sea.c`, ADR-33), bandeiras historicas (`hflags.c`, ADR-34), cidade como mapa de lotes (ADR-35). Menus com as pinturas da concept em cor direta (ADR-29). **Fluidez (ADR-36, 2026-10-17):** tela de cima em page flip na VRAM (sem tearing), camera/relogio por VBlank, DSi a 134 MHz, mapa em anel 512x256 com render-ahead na folga do quadro, pixel em ARM/ITCM sem divisao (bit a bit igual ao antigo), overlay por indice de arestas, painel e dia rapido por orcamento; medido no ARM9 emulado (`tools/armbench`). Falta screenshot/medida no melonDS (`make PROF=1`). |
| v1.x o mundo | **Planejado; o nucleo do M1 ja existe no codigo** (`source/world.c` mundo celular/frentes + `nations.c`/`history.c`/`calendar.c`/`traits.c`/`px_flag.c`/`earth_mask.c`), sem tela ainda. Mapa-mundi 1800 ou aleatorio, catalogo de paises, historia viva: **`DESIGN.md`** (marcos M1-M7) + ADRs 18-20 do SDD. |

## Ordem de leitura

1. Este arquivo.
2. **`DESIGN.md` sec 1-2** (visao e pilares). E a fonte de verdade de design: o *que* e o *por que*.
3. `docs/SDD.md` sec 0 (60 segundos de contexto) e sec 1 (mapa dos arquivos): o *como* no codigo. O **plano ordenado** (o que falta, em que ordem: baseline, fases, riscos) esta em `docs/ROADMAP.md`.
4. A secao do assunto da tarefa: economia (2), mercado (3), POPs (4), eventos (5), governo (6), combate/IA (7), controles (8), render (9), build (10), ADRs (11), guia de mudancas (12).
5. **Tarefa de UI, menu, HUD, fonte, cor ou arte -> `DESIGN.md` sec 3 + SDD sec 14, sempre.** Abra `concept_arts/image.png` junto.
6. **Tarefa de mapa, guerra, paises, traits, eventos historicos -> `DESIGN.md` sec 4-8.** Cadastro de pais/evento/trait segue os checklists da sec 8, incluindo a revisao de representacao de povos indigenas (8.2).
7. **Mexer em video/VRAM/cor/boot -> `docs/POSTMORTEM-TELA-BRANCA.md` antes.**

Hierarquia: em conflito de design, `DESIGN.md` vence e o SDD e corrigido; em conflito tecnico (RAM, CPU, pixel), o SDD e as restricoes do DS vencem e o `DESIGN.md` e corrigido. Nunca deixe os dois se contradizendo.

## Regras invioláveis (quebrar qualquer uma = bug no DS)

- **Cor:** todo pixel passa por `Px_Col()` (runtime) ou `PXC()` (constante em `include/pal.h`, v0.9). Os dois setam `BIT(15)`. `RGB15()` cru em framebuffer Bmp16 = pixel transparente = tela preta (ADR-13).
- **Sem `float`, sem `double`.** Inteiro e shift.
- **Zero `malloc`.** Buffers sao estaticos em `.bss`.
- **`GameState` <= 14KB** (`_Static_assert` em `types.h`); `Tile` == 12B. Mediu antes de crescer struct.
- **RNG unico:** `g->rng` via `Rng_*`. Nunca `rand()`. Mesma seed = mesma partida (replay e testes dependem disso).
- **Nao reordene enums** `BuildingKind`, `GovKind`, bens (`GOOD_*`): save e IA usam os IDs. Adicione antes do `*_COUNT`.
- **Mudou layout de struct salva -> `SAVE_VER++`.**
- **Ordem do dia** em `Game_Day` e fixa (SDD sec 1).
- **Render sob demanda:** redesenha so com `dirty`/`dirtyBot` ou 1x/15 frames. Menu parado nao redesenha a 60Hz.
- **Tela de cima = VRAM (page flip, ADR-36):** `Px_BackTop()` aponta p/ a pagina escondida na VRAM, que **ignora escrita de 8 bits**. Nada de `memcpy`/`memset` da newlib nela (desalinhados, escrevem byte): copie com `Px_Copy16`. Toda tela de cima redesenha por inteiro (a pagina tem o quadro de 2 trocas atras). Flip so' pede; a troca e' em `Px_VSync` (VBlank).
- **Tempo = VBlanks, nao quadros:** camera, relogio do jogo e animacao andam por `Px_Vb()`. Coisa nova que "anda por quadro" tambem.
- **Laco por pixel sem divisao:** o ARM9 nao tem divisor e Thumb nem divide por constante sem chamar biblioteca. Use shift/tabela/reciproco; laco quente vai em `HOT_BEGIN`/`HOT_ITCM` (`include/hot.h`, ARM na ITCM). Mediu antes e depois (`make PROF=1`, `tools/hostpx/prof.sh`, `tools/armbench`).
- **Zero warnings** com `-Wall -Wextra`.
- **Texto do jogo:** ate a F1 da v0.9 entrar, fonte 3x5 **sem acento** (minusculas viram maiusculas). Depois da F1: Serifa 8/12 com acento e **toda string de UI em `source/str.c`**.

## UI: a concept art e o plano

- `concept_arts/image.png` mostra 8 estados de tela (menu principal, selecao de modo, jogo com minimapa e HUD, pausa, configuracoes, selecao de nacao) com as duas telas do DS.
- O SDD sec 14 traduz cada painel em: tokens de cor (14.1), maquina de telas (14.2), fontes (14.3), **coordenadas em pixel por tela** (14.4), gramatica p/ telas que a concept nao mostra (14.5), pipeline e inventario de assets (14.6), audio (14.7), Settings (14.8), **fases F1-F8 com criterio de pronto** (14.9) e checklist de fidelidade (14.10).
- Nao renderize a carcaca do DS nem as legendas/colunas laterais da concept (sao branding).
- As **pinturas dos menus sao recortadas da propria concept** e reamostradas (Lanczos) para o pixel nativo por `tools/genpics.py` (ADR-29, pedido do dono: "igual a concept, menos pixelada"). Regras: nunca recorte texto da concept (o texto e' sempre do jogo, `str.c`; o gerador apaga o "Aperte A" do botao) e nunca amplie/reduza por vizinho mais proximo. Asset faltando = placeholder do `Art_Blit`; nao trave codigo esperando arte.
- Os 5 estandartes da concept sao **exemplos**, nao a lista de paises. O catalogo (>=150 em 1800) esta no `DESIGN.md` sec 6; a tela de selecao vira carrossel com filtro por continente (SDD sec 14.4).
- Traits e textos do catalogo sao proposta de design: balanceie, mas mantenha o fundamento historico e preencha o campo `fonte`.
- Trabalhe por fase (F1 antes de F2...). Ao fechar uma fase, marque-a no SDD sec 14.9 e atualize as secs 8/9 (controles/render) que ela tornou obsoletas.

## Build

- **devkitPro oficial** (Windows: shell MSYS2 do devkitPro; Linux/macOS: `DEVKITPRO`/`DEVKITARM` setados): `make` -> `ohmyvictoria_ds.nds`. `make clean` limpa.
- **Toolchain rootless (Linux sem sudo):** `source ~/devkitpro/env.sh` e depois `make`. Ativa `LOCAL_TOOLCHAIN=1` no Makefile (ADR-12). Nao mexa nesse bloco sem ler o postmortem sec 7.
- **Checagem de logica sem libnds:** `gcc -fsyntax-only -Iinclude source/*.c` com stub de `nds.h` (input/px exigem libnds real).
- `Makefile`: `TARGET`, `GAME_TITLE`, `GAME_SUBTITLE1/2`. Arte gerada (v0.9) entra por `source/gen`.

## Testar

- **Video so vale no melonDS ou no DS real.** Screendump do desmume em SDL dummy NAO e evidencia (postmortem sec 7.3). Se nao puder rodar, peca ao usuario o screenshot do melonDS.
- **Smoke de render no host (antes de pedir screenshot):** `tools/hostpx/run.sh` navega o jogo como um jogador (teclas injetadas: menus, mapa-mundi, zooms, guerra, pausa, opcoes, classico); com `SHOTS=<dir>` salva cada tela em PPM p/ revisar layout. Roda o jogo no PC (stub de `nds.h`, sem libnds, ASan/UBSan) e falha se **travar** ou sobrar **pixel transparente**. Em segundos diz se "tela preta" e' "nunca desenhou" (trava/excecao) ou cor sem `BIT(15)` — ver `docs/POSTMORTEM-TELA-BRANCA.md` sec 10. Use sempre que mexer em `px_*`/`input.c`.
- **Desempenho (ADR-36):** `make PROF=1` gera a ROM com a caixa de ms por secao no canto da tela de cima (QUADRO verde = cabe em 16,7 ms) — e' o numero que vale (melonDS/DS). Sem hardware: `tools/hostpx/prof.sh` (PC, so' relativo) e `tools/armbench` (o jogo inteiro compilado pelo devkitARM num ARM emulado; relogio = instrucoes). Mexeu no pixel do mapa: `tools/hostpx/eqmap.sh <ref>` prova que e' bit a bit igual. Mexeu na simulacao (ex.: `HOT_ARM`, troca de conta): `tools/armbench/simhash.sh` (mesmo `World_Hash` no PC e no ARM9).
- Playtest de 5 min: SDD sec 10.
- Logica: sim host de 200 dias x 3 faccoes sem crash + replay deterministico (mesma seed, mesmo resultado).
- Bug de video: rode `tools/hostpx/run.sh` primeiro; se ele passar e o hardware ainda estiver preto, rode os ROMs de bisect na ordem do postmortem sec 6.1 antes de inventar teoria nova; o teste minimo tem que vir de um caminho de codigo diferente do que falha.

## Pronto quer dizer

1. `make` sem warnings nos nossos arquivos.
2. Sim 200 dias + replay deterministico (se tocou logica).
3. Playtest no melonDS (se tocou tela) e, para UI v0.9, o item da sec 14.10.
3b. Se tocou `px_*`/`input.c`: `tools/hostpx/run.sh` verde (sem hang, sem pixel transparente, anel == repintura). Se tocou laco quente: `tools/hostpx/prof.sh` nao piorou.
4. **`DESIGN.md` e SDD atualizados** (no SDD: tabela da sec 1, secao do sistema, ADR se houve decisao; no DESIGN: catalogo, traits, eventos, marcos) e `docs/diagramas/*.mmd` se mudou cadeia ou ordem do dia. O SDD e verboso de proposito (ADR-9): e ele que da contexto ao proximo agent.
5. `BLD_COUNT`/`GOOD_COUNT`/`GOV_COUNT` consistentes em giro, desenho e IA.

## Convencoes

- Idioma: PT-BR em tudo (codigo, comentarios, docs, mensagens). Comentarios de codigo sem acento; docs podem ter acento.
- Estilo C do repo: compacto, funcoes `Modulo_Acao` (`Eco_Build`, `Mkt_CloseDay`), `static` para o que e interno, comentario curto explicando o porque.
- Geometria de UI fica em tabelas (`ui.c`), nunca espalhada pelo codigo de desenho.
- Repo tem git (branch atual `feat/mundo`): descreva o que mudou na resposta e no SDD; nao reescreva commits existentes.
- `ref_*.nds` na raiz sao ROMs oficiais de referencia (dslink, ftpd) usados no diagnostico do boot. Nao apague.
