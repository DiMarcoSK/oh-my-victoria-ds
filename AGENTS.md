# AGENTS.md — OH MY VICTORIA DS!

Contexto para qualquer agent que abrir este repo. Leia inteiro antes de editar (2 min).

## O que e

Grande estrategia estilo Victoria para **Nintendo DS** (homebrew, C puro, libnds/devkitARM).
Saida: `ohmyvictoria_ds.nds` (melonDS ou DS real com flashcard).

- **Visao (`DESIGN.md`):** Victoria 3 no bolso. Mapa politico estilo Victoria 3 com
  territorio pintado e frentes que avancam celula a celula como em frontwars.io. O jogador
  escolhe **mapa aleatorio** ou **mapa-mundi de 1800** com >=150 paises historicos e povos
  indigenas, cada um com traits e flavor historicos, e a historia real acontecendo ate 1936
  — a menos que o mundo divirja.
- **Hoje (v0.7):** tempo continuo, mapa 32x24, voce contra 2 bots, vitoria com 600 de
  prestigio ou 200 tiles. 8 bens em cadeia, 23 obras, 4 POPs, mercado interno/externo,
  tarifa, uniao aduaneira, 3 governos, clima, eventos, marcha com atraso.

## Estado atual (2026-10-16)

| Versao | Status |
|---|---|
| v0.7 | **Jogavel.** Descrita nas secs 1-10 do SDD. Bug da tela preta resolvido (ADR-13). |
| v0.8 touch-first | **Em andamento no codigo**, ainda nao documentada: `include/ui.h`, `source/ui.c`, `source/px_map.c` (mapa "cozido" em buffer 512x384). `source/px_ui.c`, citado em `ui.h`, ainda nao existe; `input.c` segue no esquema v0.7. |
| v0.9 UI da concept art | **Planejada.** `concept_arts/image.png` e o alvo visual; o contrato esta no **SDD sec 14** (fases F1-F8). |
| v1.x o mundo | **Planejado.** Mapa celular Victoria 3 + frontwars, mapa-mundi 1800 ou aleatorio, catalogo de paises, historia viva: **`DESIGN.md`** (marcos M1-M6) + ADRs 18-20 do SDD. |

## Ordem de leitura

1. Este arquivo.
2. **`DESIGN.md` sec 1-2** (visao e pilares). E a fonte de verdade de design: o *que* e o *por que*.
3. `docs/SDD.md` sec 0 (60 segundos de contexto) e sec 1 (mapa dos arquivos): o *como* no codigo.
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
- **Zero warnings** com `-Wall -Wextra`.
- **Texto do jogo:** ate a F1 da v0.9 entrar, fonte 3x5 **sem acento** (minusculas viram maiusculas). Depois da F1: Serifa 8/12 com acento e **toda string de UI em `source/str.c`**.

## UI: a concept art e o plano

- `concept_arts/image.png` mostra 8 estados de tela (menu principal, selecao de modo, jogo com minimapa e HUD, pausa, configuracoes, selecao de nacao) com as duas telas do DS.
- O SDD sec 14 traduz cada painel em: tokens de cor (14.1), maquina de telas (14.2), fontes (14.3), **coordenadas em pixel por tela** (14.4), gramatica p/ telas que a concept nao mostra (14.5), pipeline e inventario de assets (14.6), audio (14.7), Settings (14.8), **fases F1-F8 com criterio de pronto** (14.9) e checklist de fidelidade (14.10).
- Nao renderize a carcaca do DS nem as legendas/colunas laterais da concept (sao branding).
- Nao recorte a concept para usar como asset: ela nao esta em resolucao nativa. Asset faltando = placeholder do `Art_Blit`; nao trave codigo esperando arte.
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
- Playtest de 5 min: SDD sec 10.
- Logica: sim host de 200 dias x 3 faccoes sem crash + replay deterministico (mesma seed, mesmo resultado).
- Bug de video: rode os ROMs de bisect na ordem do postmortem sec 6.1 antes de inventar teoria nova; o teste minimo tem que vir de um caminho de codigo diferente do que falha.

## Pronto quer dizer

1. `make` sem warnings nos nossos arquivos.
2. Sim 200 dias + replay deterministico (se tocou logica).
3. Playtest no melonDS (se tocou tela) e, para UI v0.9, o item da sec 14.10.
4. **`DESIGN.md` e SDD atualizados** (no SDD: tabela da sec 1, secao do sistema, ADR se houve decisao; no DESIGN: catalogo, traits, eventos, marcos) e `docs/diagramas/*.mmd` se mudou cadeia ou ordem do dia. O SDD e verboso de proposito (ADR-9): e ele que da contexto ao proximo agent.
5. `BLD_COUNT`/`GOOD_COUNT`/`GOV_COUNT` consistentes em giro, desenho e IA.

## Convencoes

- Idioma: PT-BR em tudo (codigo, comentarios, docs, mensagens). Comentarios de codigo sem acento; docs podem ter acento.
- Estilo C do repo: compacto, funcoes `Modulo_Acao` (`Eco_Build`, `Mkt_CloseDay`), `static` para o que e interno, comentario curto explicando o porque.
- Geometria de UI fica em tabelas (`ui.c`), nunca espalhada pelo codigo de desenho.
- Repo ainda nao e git: nao assuma historico; descreva o que mudou na resposta e no SDD.
- `ref_*.nds` na raiz sao ROMs oficiais de referencia (dslink, ftpd) usados no diagnostico do boot. Nao apague.
