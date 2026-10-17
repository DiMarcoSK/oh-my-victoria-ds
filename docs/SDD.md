# OH MY VICTORIA DS! — SDD v0.7.0 (sociedade + governos)

> RTS Vic-like sem turnos. Ate v0.8: pixel-art 100% procedural. **v0.9 (planejado): UI
> ilustrada conforme a concept art (`concept_arts/image.png`) — ver sec 14, que e o contrato visual.**
> Compila com `make` -> `ohmyvictoria_ds.nds` -> flashcard (R4/Acekard) ou melonDS / DS real.
> Nome antigo: FRONTIERS DS (v0.5). Nome atual: **OH MY VICTORIA DS!** (v0.6+: sociedade; v0.7+: governos).
>
> **Estado do codigo:** secs 1-10 descrevem a v0.7 (jogavel). O codigo ja tem a v0.8 em
> andamento (touch-first: `include/ui.h`, `source/ui.c`, `source/px_ui.c`, `source/px_hud.c`,
> `source/px_map.c`; `px_tiles.c` removido). Sec 14 define o alvo v0.9 e manda sobre secs 8/9
> em conflito; secs 1/9 ainda descrevem a v0.7 e serao atualizadas na E0 de `docs/ROADMAP.md`.
>
> **Historico de bugs criticos:** (1) tela preta/branca no boot — jogo rodava invisivel,
> pixel Bmp16 sem bit de alpha — RESOLVIDO em 2026-10-16 (ADR-13). (2) tela preta DE NOVO na
> v0.8, mas por outra causa: laco infinito em `LineP` travava o bake (`Px_Tick`) antes do
> primeiro render, e `SNAP`/`TileKey()==0` deixava tiles sem bake — RESOLVIDO em 2026-10-16
> (ADR-21). Post-mortem completo com checklist de descartes em
> `docs/POSTMORTEM-TELA-BRANCA.md` (sec 1-9 incidente 1, sec 10 incidente 2). Leia antes de
> mexer em video.

## 0. LEIA ISTO PRIMEIRO (contexto em 60 segundos para agents)

- **O que e o jogo:** RTS continuo (dias fluem sozinhos, sem turnos). Voce e 2 bots disputam um mapa 32x24. Vence quem chegar a **600 prestigio** ou **200 tiles**. Perde quem ficar com 0 tiles.
- **Loop de 5 min:** menu pixel (L/R dificuldade, B seed, A comeca) -> constroi fazenda (Y) -> compra grao (B) -> recruta (X) -> marcha (A, leva dias, seta vermelha) -> evento surpresa -> clima muda colheita -> precos mexem -> paz (R) ou uniao aduaneira (R+CIMA) -> troca de governo (SELECT+L: MON/DEM/TEO) -> prestigio sobe -> vitoria.
- **Pilar Vic-like:** 8 bens em cadeia + 4 POPs com demandas + mercado interno/externo + tarifa + uniao + industria que precisa de insumo (throughput). Sem insumo = produz menos, nunca negativo.
- **Restricoes DS (nao quebrar):** sem `float`, **zero `malloc`** (buffers em `.bss` estatico), `GameState` ~13KB com `_Static_assert` de 14KB, single-pass por dia, RNG unico deterministico (`g->rng`, xorshift32), texto 3x5 sem acento, render sob demanda (`dirty` + 1x/15 frames, sec 9), `-march=armv5te -mtune=arm946e-s -mthumb`, build com zero warnings (`-Wall -Wextra`).
- **Onde mexer com seguranca:** `source/economy.c` (obras), `source/market.c` (precos), `source/pops.c` (POPs), `source/events.c` (eventos/clima), `source/ai.c` (bots), `source/gov.c` (governos). Ver sec 1 (mapa de arquivos) antes de editar.
- **UI / telas / arte:** o alvo e a concept art (`concept_arts/image.png`). Antes de tocar em menu, HUD, fonte ou cor, leia a **sec 14** (tokens de cor, maquina de telas, layout em pixels por tela, assets, fases F1-F8).

## 1. MAPA DO CODIGO (arquivo -> responsabilidade -> quem chama)

| Arquivo | O que faz | Quem chama | Sai sem quebrar se... |
|---|---|---|---|
| `include/config.h` | Constantes: titulo, versao, mapa 32x24, 8 bens, 4 POPs, tesouro inicial, vitoria, dificuldades, eventos | Todos | Nao mude `GOOD_COUNT`/`MAP_W/H` sem revisar `types.h` + save |
| `include/types.h` | Structs `Tile` (12B), `Faction` (~100B), `GameState` (~13KB); enums `TerrainKind` (10), `BuildingKind` (23), `SiteKind` (7), `DipStance`; helpers `tileInside/tileWalk/popGet/popSet` | Todos | Nao reordene enum `BuildingKind` (save + AI dependem dos IDs); nao aumente structs sem medir RAM |
| `source/game.c` | `Game_Init/New/Frame/Day/CheckEnd`; orquestra o dia | `main.c`, `input.c` | Mantenha ordem do dia: `Evt -> Pop -> Eco x3 -> Combat -> Dip -> AI x2 -> Mkt_Close -> Recalc -> CheckEnd` |
| `source/map.c` | `Map_Gen/Init/Get/Recalc/Def`; gera terreno+sites por seed, capitais, recalcula dono/tropa/pop | `game.c`, todos | `Map_Gen` deve continuar deterministica por seed; `Recalc` e O(768) barato |
| `source/economy.c` | 23 obras (`Eco_Day/Cost/Build/FortUp/Recruit/Tax/BldName/BldDesc/DiffName/BotQ7`) | `Game_Day`, `input.c` (Y/X/L), `ai.c` | Precos/estoques via `market.h`; RNG so via `Rng_*`; sem float |
| `source/market.c` | 2 mercados (`Mkt_Init/Base/IntPrice/ExtPrice/IntSup/IntDem/ExtSup/ExtDem/CloseDay/BuyInt/BuyExt/SellExt/Name/Short`) | `Eco_Day`, `Pop_Day`, `input.c` (B), `ai.c` | `CloseDay` zera sup/dem todo dia; tarifa 0 so com ALLY |
| `source/pops.c` | 4 POPs (`Pop_Day/Need/Count/Add`) | `Game_Day` | Consome estoque de verdade mas limitado a `dem/4` por dia; felicidade suavizada (move 1/4 ao alvo) |
| `source/events.c` | Clima + 8 eventos (`Evt_Day/Name`) | `Game_Day` | Cooldown 20d + 4%/dia; sempre seta `evtX/Y`, `lastEvt`, `evtMsg`, `dirty` |
| `source/combat.c` | Marcha com delay (`Combat_Order/Pow/Day`) | `input.c` (A), `ai.c` | `MarchDays = 2 + terreno + forte`; dado RNG +-1; bonus se `stock[ARMS]>20` |
| `source/diplomacy.c` | Guerra/paz/uniao (`Dip_Init/Get/Set/CanAtk/Day/Peace/Ally/Name`) | `input.c` (R), `ai.c`, `combat.c` | Paz custa `40-12` por rel; uniao exige `rel>=70 + 100 ouro` |
| `source/ai.c` | Bots (`AI_Day/Budget`) | `Game_Day` | Gira 4 areas por `(day+f)%4`: eco/trade -> militar -> ataque -> diplomacia; budget limita varredura (40/80/120/200) |
| `source/input.c` | Todos os botoes sem menu (ver sec 8) | `main.c` | `Y` gira `cyc` em 1..22; `B` gira `good` em 0..7; `SELECT+B/R` save/load; `SELECT+L` governo |
| `source/save.c` | Slot unico em RAM (`Save_W/R/Ok/Clr`) | `input.c` | `SAVE_VER=63` (v0.9: `g->screen`/`scrPrev` no lugar de `inMenu`); copia byte-a-byte; SRAM real fica p/ F8 (ver roadmap) |
| `source/rng.c` | RNG unico xorshift32 (`Rng_Init/Next/Range/Chance`) | Todos | Mesma seed = mesma historia; nunca use `rand()` da libc |
| `source/px_core.c` | Framebuffer duplo (MODE_5_2D), `Px_Init/Col/PSet/Rect/Border/Clear/Flip`, `Px_BackTop/Bot` | `main.c`, `px_*.c` | Buffers ESTATICOS em `.bss` (2x 96KB, zero malloc); todo pixel via `Px_Col` (BIT(15), ADR-13) |
| `source/px_font.c` | Fonte 3x5 propria (<2KB ROM), `Px_Text/TextTop/Sc/Bar/BarT/Width/Glyph/Itoa` | `px_ui.c`, `px_hud.c`, `px_map.c` | Sem acento (a Serifa da v0.9 assume; vira so overlay de debug, sec 14.3); minusculas viram maiusculas |
| `source/px_ui.c` + `include/ui.h` | v0.8 TOUCH-FIRST: desenha dock/abas/menu de caneta a partir de tabelas (`source/ui.c`); `Ui_Hit` despacha taps | `input.c`, `px_map.c` | Geometria so' nas tabelas; alvo >=20px (dedo/caneta) |
| `source/px_hud.c` | Tela CIMA: HUD (dia/vel/gov, recursos, barras, card do tile, minimapa, relacoes, evento) + arte do menu; cores via `pal.h` | `main.c` (`Px_Top`) | F3 da v0.9 troca as telas de lugar |
| `source/px_map.c` | Tela BAIXO: mapa cartografico "cozido" em buffer-mundo + viewport (pan de caneta) + overlays; `Px_Bot/MiniCol/Tick` | `main.c`, `px_hud.c` | Bake espalhado por frame; redesenha so' sob `dirtyBot` |
| `include/pal.h` | **v0.9 F1:** tokens de cor `PXC(r,g,b)` com BIT(15); fonte unica de cor de UI (sec 14.1) | `px_hud.c` e telas v0.9 | Cor literal so' aqui/`px_map.c` |
| `source/settings.c` + `include/settings.h` | **v0.9 F1:** `Settings` (musica/efeitos/idioma/tela) fora do `GameState`, aplicado no boot (sec 14.8) | `main.c`, `str.c` | Persistencia em SRAM na F8 |
| `source/str.c` + `include/str.h` | **v0.9 F1:** tabelas PT/EN em UTF-8 + `Str(id)` (sec 14.3) | telas v0.9 | Literal de UI so' aqui |
| `source/px_art.c` + `include/px_art.h` | **v0.9 F1:** `Art_Blit/H/Clip` (RLE 8bpp, indice 0 transparente) + placeholder (sec 14.6, ADR-14) | telas v0.9 | Asset sem PNG = placeholder; o codigo nunca espera a arte |
| `source/px_font2.c` + `source/gen/font_data.c` | **v0.9 F1:** Serifa 8/12 proporcional 1bpp (UTF-8); `Px_Text2/Wrap/Width2` (sec 14.3) | telas v0.9 | `font_data.c` e' GERADO por `tools/genfont.py` e commitado; o build nao roda Python |
| `source/menu.c` + `include/menu.h` | **v0.9 F2:** maquina de telas (`Menu_Input/Go/Frame`, sec 14.2): Principal, Modo, Preparar, Nacao (carrossel com filtro de continente), Pausa, Configuracoes, Creditos e o despacho do `SCR_WORLD`. Estado `Menu` fora do `GameState` | `input.c` (`In_Handle`), `game.c` (`Game_Frame`) | `Menu_Go` troca a tela e o layout do LCD (`lcdMainOnTop/Bottom`, so' o `SCR_WORLD` usa a Invertida); o jogo classico (`SCR_GAME`) segue no input da v0.8 |
| `source/px_menu.c` | **v0.9 F2:** desenho das telas da concept com os componentes de `px_ui.c`; arte procedural no lugar dos placeholders (por do sol de Westminster + logo, rosa dos ventos, gravura do porto, brasao) | `px_hud.c` (`Px_Top`), `px_map.c` (`Px_Bot`) | A F4 troca a arte procedural por PNG sem mexer no layout |
| `source/wgame.c` + `include/wgame.h` | **Partida no mundo celular (`SCR_WORLD`, M1/M4):** `World` estatico + estado de UI `WGame` (camera, 5 zooms, mira, selecao, %, velocidade 1-5, avisos); **setas movem o mapa com a mira no centro** e A abre a **folha de acoes** do alvo (`WG_DipItems`/`DipDo`: pais, sua terra, sua cidade, terra livre — ADR-30); traduz toques em ordens (`World_Attack/Recruit/ProposePeace/Landing/Recall`, `City_Found`) | `menu.c` | Toda regra mora em `world.c`; aqui so' camera/mira/ordens. Painel so' caneta (layout Invertida: teclas). Geometria da folha `WG_ACT_Y/H` |
| `source/px_world.c` | **Render do mundo:** barra de recursos + mapa politico em 2 passadas (dono por pixel com cantos EPX, depois cor/fronteira/espuma/relevo), rotulos espacados sem sobreposicao, capitais, **mira** (anel com aberturas, ouro sobre alvo acionavel), minimapa, faixa de evento; painel (ficha, despacho, 5 botoes, % e velocidade) e a **folha de acoes** com preco no rotulo; `PxW_Earth` (Terra em sepia) | `px_menu.c` | Buffers `KEY/TKY` 2x 44KB em `.bss`; so' tokens/`PXC` (ADR-13); `PxW_Stats` 1x por dia |
| `source/px_mapr.c` | **Camada do mapa-mundi (ADR-24):** pintura suave por pixel (bilinear das 4 celulas + domain warp por tabela de ruido 64x64, hillshade do `W_LIM`, profundidade do mar, espuma, textura por hash, Bayer 4x4) num **cache** `LAYER` 88KB + `KEY` 44KB; `PxM_Update` repinta so' o que mudou (tela toda em zoom/modo/paleta; faixa nova no pan; blocos 8x8 de `World.dirtyBlk` na conquista); `PxM_Overlay` pisca frentes e contorna o pais sob o cursor a partir do `KEY` | `px_world.c` | Nunca pinte o mapa direto no backbuffer: passe pelo cache. `PxM_Scratch` empresta o `LAYER` p/ a cena da cidade (invalida o cache) e `PxM_Update` devolve: chama `PxC_SceneReset` (sem isso, reabrir a cidade mostrava o mapa com pedestres — ADR-30) |
| `source/city.c` + `include/city.h` | **Cidades (ADR-25, DESIGN 4.7):** tabela `BLD` (11 obras: custo, dias, nivel/ano minimo, ouro, comida, tropas, prestigio, defesa), ~190 cidades historicas de 1800, vilas geradas, `City_Day` (obra, renda, teto de tropas, crescimento logistico, nivel), fundar (`City_FoundCost` = 200 + 120 x cidades), captura, IA que constroi/funda sem usar o RNG do mundo, nomes por cultura | `world.c` (`World_Day`, `SetOwner`, `CellCost`, geracao), `wgame.c`, `px_*` | IDs `BK_*` e `CT_*` gravados: so' adicione antes de `*_COUNT`. Busca por celula passa pelo bitmap `W_CITYB` (12KB) |
| `source/px_city.c` | **Arte das cidades, procedural:** pintor em unidades de projeto (o mesmo codigo faz icone 6px, miniatura 26px e predio 56px) com 5 estilos (europeu, leste asiatico, islamico, africano, indigena) e 3 eras; cidades no mapa por zoom; cena viva da cidade (fundo cacheado + pessoas, carroca/carro, fumaca, trem, barcos); painel de slots | `px_world.c`, `px_menu.c`, `wgame.c` | Fundo da cena so' e' refeito quando muda a assinatura da cidade (nivel, obras, obra em curso, decada) |
| `source/wecon.c` + `include/wecon.h` | **Economia do mundo (ADR-26, DESIGN 4.8):** 9 bens (`GD_*`) em cadeia; producao rural pela contagem de terreno do pais (`WCountry.terr`, mantida no `SetOwner`) + obras das cidades (`WEco_CityFlows`); 4 mercados: local (comida da cidade), nacional, bloco (mercado comum) e mundial pelos portos; precos por oferta/demanda; industria limitada pelo insumo que falta; receita = imposto sobre o valor produzido + taxas urbanas + alfandega; despesa = soldo, frota e 1% do tesouro; rotas maritimas (3 por pais); frota (`WEco_BuildShip`, bloqueio) | `world.c` (`World_Day`: `WEco_Week` a cada 7 dias), `city.c`, `wdiplo.c`, `px_ledger.c`, `px_world.c` | 1x/semana, O(cidades + paises x bens), sem RNG. Buffers estaticos ~27KB. IDs `GD_*` gravados |
| `source/wdiplo.c` + `include/wdiplo.h` | **O mundo que conversa (ADR-27, DESIGN 6.5/7.7):** ideologia (`ID_*`) com modificadores e **bandeira transformada** (`WDip_Flag`: base do catalogo recolorida — Franca comunista = tricolor vermelha com estrela), revolucoes por risco (miseria, militancia, espirito da epoca) e separatismo (nacao adormecida do catalogo renasce); IA com rival, 2 aliados, chamada as armas, blocos, temperamento por decada, tributo e paz; propostas ao jogador (`WProp`); batalhas navais; ambicoes do jogador (`OB_*`) | `world.c` (`World_Day`, `World_DeclareWarCall`, `Kill`), `history.c` (`HE_IDEO`, decisoes), `wgame.c`, `px_*` | Usa `w->rng` (deterministico). Cada pais pensa 1x/mes escalonado ((ci+dia)%30) |
| `source/px_ledger.c` | **Gabinete (SCR_LEDGER):** 4 abas (Mercado, Comercio/Frota, Diplomacia, Politica/Ambicoes) na tela de cima; abas, explicacao, 1 acao e voltar embaixo; icones 8x8 dos bens (`PxW_GoodChip`) | `px_menu.c`, `wgame.c` (`WG_LedgerInput`) | Geometria `LG_*` em `wgame.h` (desenho e toque usam a mesma) |
| `tools/pixfont8.py` | **Serifa 8 desenhada a mao** (pixel a pixel, ASCII art): caixa alta 7, x-height 5, acentos compostos automaticamente | `tools/genfont.py` | Mudou um glifo: rode `genfont.py` e `font_check.py` |
| `tools/genpics.py` + `source/gen/pic_data.c` | **Pinturas dos menus recortadas da concept (ADR-29):** titulo (Westminster + logo + botao sem texto), oficial, cidade gotica, porto com estandarte (+ mascara de sombra do pano `HARBOR_FLAG*`), brasao, gravura do porto, papel e cantos (pergaminho e moldura navy), coroa; Lanczos p/ o pixel nativo, ate 255 cores de 15 bits, **8bpp cru** (`Pic`, `Pic_Blit/BlitClip` em `px_art.c`), indice 0 transparente; preview `docs/pics_preview.png` | `px_menu.c`, `px_ui.c` (`Ui_Parch*`, `Ui_NavyOrn`) | Gerado e commitado (ADR-14). ~175KB de ROM. Recorte/escala mudou: rode o script e confira o preview |
| `source/main.c` | Boot + loop com render sob demanda | — | `Game_Init` 1x; `seed=DEFAULT_SEED`; redesenha so se `dirty` ou 1x/15 frames (ver sec 9) |
| `source/gov.c` + `include/gov.h` | 3 governos (`Gov_Init/Switch/Cycle/Name/Short`, custo 50, cd 15d) | `input.c` (SELECT+L), `ai.c`, `game.c` | Efeitos lidos em `eco/pop/combat`; ver sec 6 |
| `Makefile` | `make` -> `ohmyvictoria_ds.nds` (padrao r68: calico link + `-lnds9`) | dev | `TARGET=ohmyvictoria_ds`, `GAME_TITLE/SUBTITLE1/2`; `LOCAL_TOOLCHAIN=1` p/ build rootless (ver sec 10); `make clean` limpa |
| `docs/diagramas/*.mmd` | Mermaid: arquitetura, classes, fluxo do dia | docs | Atualize junto quando mudar cadeia ou ordem do dia |

**Nucleo do mundo (branch `feat/mundo`, JOGAVEL desde 2026-10-17 pela tela `SCR_WORLD` — `wgame.c`/`px_world.c`, ADR-23):** `source/calendar.c` (calendario 1800-2025 + eras + Modo Eterno), `source/traits.c` (30 traits), `source/txt.c` (UTF-8 -> 3x5), `source/nations.c` (catalogo de paises/povos), `source/px_flag.c` (bandeiras procedurais, ADR-19), `source/earth_mask.c` (mascara de terra real), `source/history.c` (historia viva + eventos aleatorios) e `source/world.c` + `include/world.h` (mundo celular com frentes, marco M1, ADR-18). Testes de nucleo no PC em `host/` (`sh host/run_tests.sh`, verde). O caminho ate a tela sao os marcos M2-M7 (`DESIGN.md` sec 9) e as fases da v0.9 (sec 14.9); o plano ordenado esta em `docs/ROADMAP.md`.

Ordem do dia (`Game_Day`, `source/game.c:45`):

```
Evt_Day (clima+evento) -> Pop_Day (demandas+felicidade) -> Eco_Day x3 (producao)
-> Combat_Day (marchas) -> Dip_Day (rel 1x/2 dias) -> AI_Day x2 (bots)
-> Mkt_CloseDay (precos+uniao) -> Map_Recalc + Game_CheckEnd
```

## 2. ECONOMIA: 8 BENS + 23 OBRAS (sociedade completa)

### 2.1 Bens (`config.h:21-29`, `market.c:BASE`)

| # | Bem | Base | k | Papel na cadeia |
|---|---|---|---|---|
| 0 | grao (GR) | 2 | 12 | Alimento base; reage rapido a oferta/demanda |
| 1 | roupa (RO) | 4 | 6 | Feita em TEXTIL de grao; demanda de work |
| 2 | movel (MO) | 6 | 6 | Feito em SERRARIA; demanda de burg; DOCKYARD consome p/ exportar |
| 3 | arma (AR) | 10 | 3 | Feita em OFICINA de ferram+carvao; recruta consome; preco estavel |
| 4 | luxo (LX) | 12 | 3 | Feito em VIDRO de carvao+ferram (exige literacia 20); demanda burg/nobl; prestigio |
| 5 | ferram (FE) | 6 | 6 | Intermediario: ACO de carvao -> OFICINA/VIDRO/SERRARIA usam |
| 6 | carvao (CA) | 3 | 8 | Energia: MINA/POCo produzem; ACO/OFICINA/VIDRO consomem |
| 7 | vinho (VI) | 8 | 5 | Feito em VINICOLA/POMAR de grao; demanda burg/nobl; TEATRO consome |

Preco: `p += (dem-sup)*k>>4`, clamp `base/2 .. base*3` (`market.c:Mkt_CloseDay`).
Exemplo: grao base 2, `dem-sup=+10` -> `p += 10*12>>4 = +7` -> clamp em `1..6`.

### 2.2 Obras (`types.h:16-24`, `economy.c:Eco_Cost/Eco_Tile`)

Grupos: rural(4) industria(6) urbana(5) estado(5) mar(3). Custo em ouro, efeito por dia:

**Rural — produz primario (clima e site importam):**
| Obra | Custo | Efeito/dia |
|---|---|---|
| FAZENDA (1) | 30 | +8 grao (+-1 RNG; seca -2, chuva +2; FERTIL +2; DESERTO -2; AMALDICOADO -2) |
| POMAR (2) | 40 | +4 grao +1 vinho |
| MINA (3) | 50 | +3 carvao (+1 COLINA, +2 CARVAO-site, +1 FERRO-site; +3 ouro se OURO-site) |
| CARVAO/Poco (4) | 45 | +4 carvao (+1 FLORESTA, +2 CARVAO-site) |

**Industria — transforma (throughput: sem insumo produz menos):**
| Obra | Custo | Efeito/dia |
|---|---|---|
| SERRARIA (5) | 40 | +3 movel (+1 FLORESTA, +1 se `stock[FE]>10`) |
| TEXTIL (6) | 50 | 2 grao -> 3 roupa (sem grao: 1 grao -> 1 roupa) |
| ACO (7) | 70 | 2 carvao -> 2 ferram (+1 FERRO-site, +1 se `literacy>50`) |
| OFICINA (8) | 65 | 1 ferram + 1 carvao -> 2 armas (so ferram: -> 1 arma) |
| VINICOLA (9) | 55 | 2 grao -> 2 vinho (1 grao -> 1 vinho) |
| VIDRO (10) | 80 | 1 carvao + 1 ferram -> 2 luxo, +1 prestigio (exige `literacy>=20`) |

**Urbana — servicos (consomem bens, dao felicidade/prestigio/renda):**
| Obra | Custo | Efeito/dia |
|---|---|---|
| MERCADO (11) | 60 | +2 ouro; demanda 1 roupa + 1 movel |
| BANCO (12) | 100 | +`tesouro/100`+1 ouro (juros); +1 prestigio a cada 5 dias |
| UNIV (13) | 90 | +1 literacia (max 100); +1 prestigio/5 dias; demanda 1 roupa |
| TEATRO (14) | 70 | +felicidade no tile; +1 prestigio/7 dias; demanda 1 vinho + 1 luxo |
| HOSPITAL (15) | 80 | +felicidade; cresce pop mais rapido (ver sec 4); demanda 1 ferram |

**Estado — ordem (defesa custa ouro todo dia):**
| Obra | Custo | Efeito/dia |
|---|---|---|
| QUARTEL (16) | 60 | Recruta barato no tile e vizinhos (2 ouro+2 grao+1 arma vs 5+4+2); +moral; demanda 1 grao + 1 arma |
| MURALHA (17) | 50 | +3 defesa; -1 ouro/dia; upa p/ FORTE com X (custa 40) |
| FORTE (18) | 90 | +8 defesa, +2 dias de marcha p/ atacante; -1 ouro/dia; +1 prestigio/10 dias |
| TEMPLO (20) | 40 | +felicidade; -1 militancia/dia |
| (alfandega e estado fiscal ficam no grupo mar/estado — ver abaixo) | | |

**Mar/fiscal — liga com mercado externo:**
| Obra | Custo | Efeito/dia |
|---|---|---|
| PORTO (19) | 80 | +1 ouro; oferta externa +2 grao +1 roupa (mundo compra) |
| ESTALEIRO (21) | 110 | 1 movel -> +4 ouro + oferta ext 2 movel; +1 prestigio/7 dias |
| ALFANDEGA (22) | 90 | +1 ouro/dia; +1 ouro por unidade exportada (`Mkt_SellExt`) |

Notas de implementacao (`economy.c:Eco_Tile`, `Eco_Day`):
- `jit = Rng_Range(g,3)` (0..2) varia colheita sem `float`.
- `AddStock` capa em 9999; tesouro capa em `MAX_RES` (9999).
- Imposto: `taxRate` 0/1/2 -> renda `0 / pop/4 / pop/2`, com mult de governo: MONARQUIA x3/2, DEMOCRACIA x3/4, TEOCRACIA x1 (ver sec 6). Bots usam 1, ou 2 no HARD/INSANO. Imposto alto derruba felicidade (ver sec 4).
- Salarios: `troopsTotal/8` ouro/dia; sem caixa, moral cai.
- Tesouro negativo e zerado (nunca negativo persistente).
- `Eco_Build` so em tile proprio, transitavel, vazio e com tesouro; `Eco_FortUp` so MURALHA->FORTE (40 ouro); `Eco_Recruit` exige `troops<12`, consome grao+arma+ouro (ver tabela quartel acima), da +2 tropas e +1 moral.

### 2.3 Exemplo numerico (1 dia, faccao com 1 FAZENDA + 1 TEXTIL + 10 pops)

1. `Pop_Day`: 10 pops pedem ~20 grao + 5 roupa -> registra `dem/4` no interno; consome ate `dem/4` do estoque.
2. `Eco_Day`: FAZENDA +8 grao (`sup+8`); TEXTIL consome 2 grao, produz 3 roupa (`sup+3 roupa`, `dem+2 grao`).
3. `Mkt_CloseDay`: grao `dem-sup = 5+2-6 = +1` -> preco sobe pouco; roupa `dem-sup = 1-3 = -2` -> preco cai pouco.
4. Imposto: `popTotal/4` (taxa 1) entra no tesouro; salarios `troops/8` saem.

## 3. MERCADOS (`market.c`, `market.h`)

- **Interno por faccao:** `priceInt[f][g]`, `supInt/demInt`. Producao (`Eco_Day`) e POPs (`Pop_Day`) mexem nele. Compra interna (`B`, `Mkt_BuyInt`) paga preco interno, sem tarifa, e registra demanda (preco sobe).
- **Externo unico:** `priceExt[g]`, `supExt/demExt`. PORTO gera oferta; export (`B+BAIXO`, `Mkt_SellExt`) vende ao preco externo; import (`Mkt_BuyExt`, usado pela IA em expansoes futuras) paga externo + tarifa.
- **Tarifa:** 15% base, **0 se tem ALLY** com alguem (`TariffPct`). ALFANDEGA da +1 ouro/unidade exportada.
- **Uniao aduaneira (ALLY):** todo dia, precos internos dos aliados convergem p/ media (`Mkt_CloseDay` sec 3). Paz (`R`) custa `40-12` por rel (`Dip_Peace`); alianca (`R+CIMA`) exige `rel>=70 + 100 ouro` (`Dip_Ally`).
- **IA esperta:** compra interno barato (grao/carvao <=3), exporta excedente caro (movel/arma/vinho/luxo) — ver `ai.c` area 0.

## 4. POPS (`pops.c`, `pops.h`, `types.h:31-32`)

- 4 classes por tile em nibbles 0..15: `popA = peas|work`, `popB = burg|nobl`.
- Demandas/dia por pop (`Pop_Need`): peas=2 grao; work=2 grao+1 roupa+1 carvao; burg=1 grao+1 movel+1 vinho+1 luxo+1 ferram; nobl=1 arma+2 luxo+1 vinho.
- Atendimento: `sat` ponderada (grao vale 3x — subsistencia sustenta, luxo da bonus); consome de verdade ate `dem/4` por dia (nao zera estoque num dia).
- Felicidade: alvo = `sat +- humor RNG(-6..+6) +- clima/imposto/site/obra`; move 1/4 em direcao ao alvo (sem serrote). Seca -5, chuva +3, AMALDICOADO -10, FERTIL/OURO +3, TEATRO/TEMPLO +5, HOSPITAL +3, imposto 1 -3 / 2 -8 (MONARQUIA com imposto 2: -4 extra, total -12), DEMOCRACIA +6, militancia>60 -5.
- Crescimento/promocao (so com `sat>=80`): a cada 3 dias pode +1 peas/work; se feliz (>65) e `literacy>30`, promove peas->work->burg->nobl por RNG; HOSPITAL acelera.
- Fome (por GRAO: `grainSat<40`, estoque pre-consumo vs necessidade — nao por `sat` global): pode -1 pop, -moral, +militancia (militancia alta + infeliz = REVOLTA em `events.c`).
- Militancia: sobe na fome (+1, 25% por tile faminto/dia) e com caixa zerado; cai se feliz (>70, 20%/dia), com TEMPLO (-1/dia no tile), FESTA (-5) e deriva -1/5 dias sem fome aguda (`Eco_Day`).

## 5. CLIMA + EVENTOS (`events.c`, `config.h:43-51`)

- Clima (`g->weather`): 0 normal, 1 seca (S no painel), 2 chuva (C). 7%/dia muda. Seca -2 grao/FAZENDA e -5 felicidade; chuva +2 grao e +3 felicidade.
- Eventos: 4%/dia apos `evtCd` (efetivo ~1/25 dias), cooldown 20d, tile aleatorio transitavel, marcador laranja (`evtX/Y`, `lastEvt`) + mensagem (`evtMsg`, 64 chars, sem acento).
- Tabela (efeito + mensagem):

| # | Evento | Efeito | Msg |
|---|---|---|---|
| 1 | COLHEITA | +30 grao, +5 feliz | `COLHEITA +30 GRAO` |
| 2 | PRAGA | -2 pop (peas/work), -10 feliz, -moral | `PRAGA -POP -MORAL` |
| 3 | OURO | +60 ouro, site vira OURO | `OURO +60 +MINA` |
| 4 | REVOLTA | Se tropa e (militancia>30 ou feliz<40): -1 tropa -20 feliz, militancia -10; senao `PROTESTO CONTIDO` | `REVOLTA -TROPA` |
| 5 | ARSENAL | +10 arma | `ARSENAL +10 ARMA` |
| 6 | TEMPESTADE | 50% demolir obra, senao -5 feliz | `TEMPESTADE DEMOLIU` / `TEMPESTADE` |
| 7 | FESTA | +10 feliz, +5 prestigio, -10 ouro, -5 militancia | `FESTA +FELIZ +PREST` |
| 8 | CARVAO | +20 carvao; site vazio vira CARVAO | `CARVAO +20` |

## 6. GOVERNO / LEIS (`gov.c`, `gov.h`, `types.h:GovKind`)

- 3 governos, 1 ativo por faccao (`Faction.gov`, +`govCd` cooldown). Sul comeca TEOCRACIA (sabor), resto MONARQUIA (`Gov_Init`, chamado em `Game_Init/New`).
- Troca: `SELECT+L` gira MON -> DEM -> TEO (`Gov_Cycle`); custa 50 ouro, cooldown 15 dias (`GOV_COST/GOV_COOLDOWN`); bloqueios mostram `GOV BLOQ +50 OURO`. Bots: monarquia com `militancy>80` foge p/ teocracia (`ai.c` area 3).
- Tabela (efeitos somem se trocar — tudo lido ao vivo de `F->gov`):

| Governo | Buffs | Debuffs |
|---|---|---|
| MONARQUIA (default) | Imposto x3/2; +1 prestigio/5d (pompa real, `Eco_Day`); +1 ataque (`Combat_Pow`) | UNIV alfabetiza 1/2 dias (censura); imposto 2 da -12 feliz (-4 extra) |
| DEMOCRACIA | +6 feliz (voz do povo, `Pop_Day`); MERCADO +3 (vs +2); UNIV +2 literacia/dia | Imposto x3/4; -1 ataque (milicia cidada) |
| TEOCRACIA | TEMPLO +2 feliz/-2 militancia + dizimo +1 ouro +1 moral/3d; +1 defesa (fe defende o lar) | BANCO sem juros (+1 fixo, sem usura); UNIV sem prestigio (saber laico) |

- Painel mostra `MON/DEM/TEO` (`Gov_Short`) na linha do dia; troca mostra `MONARQUIA/DEMOCRACIA/TEOCRACIA` (`Gov_Name`) no status.
- Custo de RAM: 2 bytes/faccao (8B total). `SAVE_VER=61` por causa disso.

## 7. COMBATE + DIPLOMACIA + BOTS

### Combate/marcha (`combat.c`)
- `A` agenda marcha p/ vizinho ortogonal proprio->alvo (nao diagonal, precisa `troops>=2`, sem `moveCd/atkCd`, transitavel, e `Dip_CanAtk` se dono != neutro).
- `MarchDays = 2 + (COLINA/DESERTO/PANTANO +1) + (FORTE +2)`. Resolve na chegada.
- `Combat_Pow = troops*2 + defesa-terreno (+3 MURALHA / +8 FORTE na defesa; -1 atacante em COLINA/DESERTO/AMALDICOADO)`, +1 se `morale>=70`, +1 ataque se MONARQUIA / -1 se DEMOCRACIA / +1 defesa se TEOCRACIA (ver sec 6), escala por `Eco_BotQ7(diff)` p/ bots, +2 se `stock[ARMS]>20`, dado RNG +-1. Atacante vence se `atk > def+2`; senao defensor perde 1 tropa no max e atacante volta a 1.
- Neutro: se `atk.troops > def.troops`, conquista com resto-1 (min 1); RUINA com ARMA da +4 armas.
- `Map_Def`: FLORESTA +1, COLINA +2, PANTANO -1, RUINA +1.

### Diplomacia (`diplomacy.c`)
- Estados: GUERRA(1)/PAZ(2)/UNIAO(3). `Dip_Day` 1x/2 dias: rel sobe +1 com vizinho em paz/uniao, cai -1 em guerra.
- `Dip_Peace(me,to)`: custa `40-(rel*24/100)` (min 12), +5 rel.
- `Dip_Ally(me,to)`: exige `rel>=70` + 100 ouro (presente p/ o outro); tarifa 0 e precos convergem (ver sec 3).
- `Dip_CanAtk`: neutro sempre; faccao so em GUERRA.

### Bots/dificuldade (`ai.c`, `economy.c:Eco_BotQ7`)
- `AI_Budget`: FACIL 40, NORMAL 80, DIFICIL 120, INSANO 200 (limite de varredura/dia).
- `Eco_BotQ7`: 96/112/128/144 (bots faceis batem mais fraco, insanos mais forte).
- Gira `(day+f)%4`: 0 eco/trade (cadeia por gargalo: OURO->MINA, CARVAO->POCO, FERRO->ACO, FERTIL->FAZENDA, grao->FAZENDA, carvao->POCO, ferram->ACO, arma->OFICINA, roupa->TEXTIL, vinho->VINICOLA, literacia<40->UNIV, luxo->VIDRO, FLORESTA->SERRARIA, tesouro>150->MERCADO); 1 militar (FORTE na fronteira se tesouro>=90, senao recruta); 2 ataque (procura melhor `atk-def`, bonus neutro/ouro/ferro, penalidade AMALDICOADO/FORTE, ousadia RNG por dificuldade; limiar FACIL 4 / NORMAL 2 / DIFICIL 1 / INSANO 0); 3 diplomacia (alia se `rel>=70` e paz; pede paz se pequeno no DIFICIL+).
- Metas de balance (beta): facil vence 80%, insano 20%; dia <30ms, 60fps.

## 8. CONTROLES (tudo sem menu, `input.c`)

| Botao | Efeito |
|---|---|
| D-pad | Cursor + camera (segue cursor); touch = cursor 1:1 (tile 16px) |
| A | Marcha do tile proprio p/ vizinho (`MARCHA` / `SEM MARCHA` / `TILE SEU P/ MARCHA`) |
| X | Recruta (+2 tropas, consome grao+arma+ouro) ou upa MURALHA->FORTE (`RECRUTA`/`FORTE`/`SEM ESTOQUE`/`SEM OURO`) |
| Y | Obra: constroi `cyc` no tile e gira `cyc` 1..22 (`OBRA OK`/`SEM TESOURO`) |
| L | Imposto: gira taxa 0->1->2 (`IMP 0/1/2`) |
| SELECT+L | Governo: gira MON->DEM->TEO, 50 ouro, cd 15d (`MONARQUIA/...` / `GOV BLOQ +50 OURO`; ver sec 6) |
| R | Paz com o bot menor (`PAZ`/`SEM OURO`) |
| R+CIMA | Uniao aduaneira com o bot menor (`UNIAO ADUANEIRA`/`REL 70 + 100 OURO`) |
| B | Compra interna 5 un de `good` e gira `good` 0..7 (`COMPRA INT`) |
| B+BAIXO | Exporta 5 un de `good` e gira `good` (`EXPORT`) |
| SELECT | Velocidade 1->2->3 (15/8/4 frames/dia) |
| START | Pausa (`PAUSADO`/`RODANDO`); no game-over volta ao menu (limpa save) |
| SELECT+B | Save (slot RAM, `SAVE OK`) |
| SELECT+R | Load (`LOAD OK`/`SEM SAVE`) |

Menu inicial: L/R dificuldade, B seed (+`0x9E3779B9`), A comeca (`Game_New`). Menu mostra seed em hexa + dificuldade + `A COMECA B SEED` piscante.

> Nota: apertar SELECT sozinho troca a velocidade; segure SELECT e aperte L p/ governo (mesmo padrao do SELECT+B/R).

## 9. MAPA + PIXEL-ART + PERFORMANCE DS

- Mapa 32x24 (`MAP_W/H`), visao 16x12 tiles de 16px (256x192) na tela cima (`VIEW_W/H`); camera segue cursor (`Cam`).
- Terrenos (10): PLANICIE/FLORESTA/COLINA/AGUA/MONTANHA/DESERTO/PANTANO/VULCAO/RUINA/OASIS; intransitaveis: AGUA/MONTANHA/VULCAO (`tileWalk`). Capitais 3x3 planas e proprias (9 tiles cada) em (2,2)/(W-3,H-3)/(W-3,2) com pacote inicial (2 FAZENDAs + SERRARIA; sul troca 1 por MINA) e pops de subsistencia (0x22/0x00); 12 neutros com 1..3 tropas.
- Sites (7): OURO/FERTIL/FERRO/RUINA-ARMA/AMALDICOADO/CARVAO/VAZIO; RUINA vira RUINA-ARMA; AMALDICOADO capa feliz em 40 na geracao.
- `Tile` 12B (768 tiles = 9KB, `_Static_assert`); `GameState` ~13KB (trava de 14KB em `types.h`); **zero `malloc`** (framebuffers 2x100KB em `.bss` estatico); `Map_Recalc` O(768) por dia; fronteira cache 256 (`fX/fY` em `ai.c`); `Dip_Day` 1x/2 dias.
- Video: `MODE_5_2D` main+sub, `BgType_Bmp16 256x256`, `VRAM_A/C`, `dmaCopy` ~100KB/tela no VBlank. **Render sob demanda** (`main.c`): redesenha so se `dirty` ou 1x/15 frames p/ animacao (cursor/agua/evento) — ~93% menos DMA e pixels vs redesenhar todo frame a 60Hz. VRAM retem o ultimo frame.
- Custo medido (ELF v0.7): `.text` 36KB, `.data` 2KB, `.bss` 220KB (200KB buffers + estado) — folga em 4MB. Codigo em Thumb (`-mthumb`, barramento 16 bits), `-march=armv5te -mtune=arm946e-s`, `-O2 -ffunction-sections -fdata-sections` + `--gc-sections`, sem `float`, sem divisao em loop de pixel (so `&`/comparacoes; `TerPx` sem `%`). Build com **zero warnings** (`-Wall -Wextra`).
- Tiles procedurais (`px_tiles.c:TilePx/TerPx`): textura por terreno (agua com brilho animado por `phase`, floresta com copas, colina com curvas, montanha com neve, deserto/vulcao/ruina/oasis proprios) + sites (OURO amarelo, FERRO vermelho, FERTIL verde, RUINA-ARMA cinza, AMALDICOADO roxo, CARVAO preto) + 23 glifos de obra; tropas = quadradinhos brancos (max 6); **marcha = seta direcional** p/ `dst` (vermelha, pisca); borda por dono; **cursor piscante**, **evento piscante laranja/amarelo**; overlay de fim de jogo (`VITORIA!/DERROTA` + `START VOLTA AO MENU` em caixa com borda dupla).
- Painel (`Px_Bot`): dia + vel + **governo (`MON/DEM/TEO`)** + pausa/clima; tesouro + **prestigio**; **barra grao + barra prestigio (/600)**; precos int/ext grao + ext arma; dono + **obra do tile** (`VOCE FAZEND`); barra felicidade; `evtMsg` ou `status`; ajuda + `SELECT+L GOVERNO`.
- Fonte 3x5 (`px_font.c`): <2KB ROM, sem acento (textos do jogo sem acento por hardware, docs podem acentuar).

## 10. BUILD / TESTE / SAVE / RNG

- Oficial: devkitPro (devkitARM + libnds + calico r68, `DEVKITARM`/`LIBNDS` setados). `make` gera `ohmyvictoria_ds.nds` (+`.elf`/`.map`/`.lst`); `make clean` limpa. Copia p/ flashcard ou abre no melonDS.
- Local (este ambiente, rootless/sem sudo): `source ~/devkitpro/env.sh` (seta `DEVKITPRO/DEVKITARM/LOCAL_TOOLCHAIN=1` + PATH com `arm-none-eabi-gcc` 13.2 do Ubuntu, `libnds`/`calico`/`newlib` genuinos do devkitPro, `ndstool` 2.3.1 compilado do fonte). `make` funciona igual; bloco `LOCAL_TOOLCHAIN` no Makefile some no build oficial. Ver `~/devkitpro/env.sh` + ADRs 10-12 abaixo.
- Teste 5 min: menu -> A -> Y (fazenda) -> B (compra) -> X (recruta) -> A (marcha) -> evento aparece -> precos mexem -> START pausa/vel. Metas: dia <30ms, 60fps, facil 80% vitoria, insano 20%.
- Save: slot unico em RAM (`save.c`), `SAVE_MAGIC 0x46523530`, `SAVE_VER 63`; `SELECT+B` salva, `SELECT+R` carrega; game-over + START limpa e volta ao menu. SRAM real (persistente no cartao) fica p/ v0.8.
- RNG: xorshift32 em `g->rng`; mesma seed = mesma historia (replay/teste). `Rng_Range(g,n)` = 0..n-1, `Rng_Chance(g,pct)` = 1 se <pct%. Nunca use `rand()`.
- Verificado no ELF v0.7: ARM9 entry `0x2004800`/RAM `0x2004000` (layout calico), `.text` 36KB, `.nds` 135KB, `ndstool -i` com header valido. Teste host: 200 dias x3 faccoes sem crash + replay deterministico + trade + governo (ver `SIM_OK` nos testes em `/tmp/opencode/sim_test*.c`).
- **Smoke de render no host (regressao de "tela preta", ADR-21):** `tools/hostpx/run.sh` compila `source/*.c` para x86 com stub de `<nds.h>` (`tools/hostpx/nds.h`), sob ASan/UBSan, roda menu + `Game_New` e falha se **travar** (timeout) ou sobrar **pixel transparente** (bit 15 = 0) nas duas telas. Segundos, sem ROM, sem emulador — pega laco infinito no bake/desenho e cor sem `BIT(15)`. Use sempre que mexer em `px_*`/`input.c`.
- Riscos conhecidos: texto sem acento; `s16` capa tesouro/estoque em 9999 (`MAX_RES`); `u8` capa tropas em 12, pops em 15/nibble, rel/literacia/militancia em 100; `morale` so afeta combate (+1 se >=70) — sem efeito economico direto.

## 11. DECISOES (ADRs — por que assim e nao de outro jeito)

1. **23 obras em 5 cadeias, nao 11 soltas (v0.5 -> v0.6):** para "sociedade completa e complexa": rural produz primario, industria transforma com throughput, urbana consome e da feliz/prestigio, estado da ordem com custo, mar liga ao externo. Alternativa (mais obras soltas sem cadeia) foi rejeitada porque nao gera gargalo nem comercio.
2. **8 bens com intermediarios (ferram/carvao/vinho), nao 5 finais:** para forcar cadeia (carvao->ferram->arma/luxo; grao->roupa/vinho). Preco `k` maior no base (grao 12, carvao 8) e menor no avancado (arma/luxo 3) para base oscilar e luxo ser reserva de valor.
3. **Throughput parcial, nao tudo-ou-nada:** sem insumo produz menos (ex: TEXTIL sem 2 grao faz 1 roupa com 1 grao) para nao travar o jogo no DS e manter diversao.
4. **Precos inteiros, nao Q8 fracionado:** comentario antigo dizia "Q8" mas painel mostra 2 digitos e DS nao precisa de fracao aqui; inteiro e deterministico e cabe no `s16`. Mantido `s16` p/ compat com save v50.
5. **Imposto/militancia/literacia como numeros 0..100, nao POPs individuais:** 768 tiles x 4 nibbles ja da granularidade; simular POPs individuais estouraria RAM/CPU do DS.
6. **Uniao = ALLY com convergencia de precos + tarifa 0:** simples, visivel no painel (precos igualam) e estrategico (paga 100 ouro agora, lucra depois). Rejeitado mercado global unico (v0.5 ja tinha separado interno/externo; v0.6 mantem).
7. **Bots por gargalo, nao fila fixa:** IA olha estoque (grao->carvao->ferram->arma->roupa->vinho->univ->luxo) e site do tile, entao cada partida diverge por seed/mapa.
8. **Renomeacao FRONTIERS -> OH MY VICTORIA DS!:** marca a virada Vic-like (POPs/mercados/cadeias) e evita colisao com outro `frontiers_ds.nds`. `TARGET`, `GAME_TITLE/SUBTITLE`, menu, `README`, SDD e diagramas atualizados juntos; IDs de obra preservados (1..22) para nao quebrar save/AI.
9. **SDD verboso de proposito:** para outros agents entenderem contexto/decisoes/codigo sem ler os 17 `.c` inteiros. Sec 1 (mapa) + sec 2 (tabelas) + sec 12 (guia) sao o contrato.
10. **3 governos com 2 bytes, nao arvore de leis (v0.7):** MON/DEM/TEO mudam imposto/feliz/guerra/prestigio com poucos `if`s (custo CPU ~zero, RAM +8B). Efeitos lidos ao vivo — trocar e dest trocar e imediato e reversivel (com custo+cd). Alternativa (leis individuais votaveis) foi adiada p/ v0.8+ por UI (poucos botoes livres) e RAM.
11. **Zero malloc + render sob demanda (v0.7):** framebuffers viraram `.bss` estatico (200KB) — elimina `sbrk`/`_sbrk_r` do caminho do link e fragmentacao; `main.c` redesenha so com `dirty` ou 1x/15f (~93% menos DMA/pixels). Alternativa (malloc 1x no boot) funcionava mas puxava syscalls e escondia 200KB do mapa de memoria.
12. **Toolchain rootless documentado, nao escondido (v0.7):** este ambiente nao tem sudo e o Cloudflare bloqueia os hosts do devkitPro, entao o setup usa: GCC/binutils ARM do Ubuntu (apt, sem root) + `libnds`/`calico`/`newlib`/`regras` genuinos copiados da instalacao Windows + `ndstool` compilado do fonte. Pecas locais: `sync-none.specs` copiado, `libsyncnone.a` (stub `__sync_synchronize_none`, single-core), `-lsysbase` + `--start-group` + `-Wl,-u,_exit` (ordem single-pass do `ld`), tudo atras de `LOCAL_TOOLCHAIN` — o build oficial nao muda 1 byte.
13. **Pixel Bmp16 sempre com `|BIT(15)` (alpha opaco, v0.7):** em background bitmap 16bpp (`aBBBBBGGGGGRRRRR`), pixel sem bit 15 e TRANSPARENTE (mostra backdrop); `RGB15()` cru nunca seta o bit -> "tela preta com jogo rodando invisivel". Fabrica de cor UNICA = `px_core.c:Px_Col()` (runtime) e seu gemeo de compilacao `PXC()` em `include/pal.h` (v0.9, sec 14.1), ambos com `BIT(15)`; proibido `RGB15()` cru em pixel de framebuffer. Causa raiz + corrida de diagnosticos completa em `docs/POSTMORTEM-TELA-BRANCA.md` (inclui checklist de descartes com prova byte a byte e os 3 ROMs de bisect que ficam na pasta).

14. **Assets ilustrados nas telas (v0.9), fim do "100% procedural" na UI:** a concept art pede retratos, cenas, brasoes e estandartes que nao saem de codigo procedural com qualidade aceitavel. Mapa e efeitos continuam procedurais. Formato: PNG indexado -> `tools/png2c.py` -> `.c` com paleta 15 bits (+BIT 15) e pixels 8bpp RLE, **commitados** — build oficial e rootless ficam identicos, sem grit nem Python no build. Rejeitado: NitroFS (mais um ponto de falha no boot, justo onde ja sofremos; reavaliar se arte passar de 512KB).
15. **Fonte serifada com acento (Serifa 8 / Serifa 12) substitui a 3x5 nas telas do jogador (v0.9):** a concept e o PT-BR exigem acento e hierarquia de titulo. Custo ~7KB ROM. A 3x5 fica so p/ overlay de debug. Strings saem de `source/str.c` (PT/EN), nunca literais soltos.
16. **Layout Normal = mapa em cima (concept); Invertida = mapa embaixo e tocavel (heranca touch-first v0.8), via `lcdSwap()` (v0.9):** resolve o conflito v0.8 x concept sem manter duas UIs. O render desenha em "tela mapa" e "tela painel"; o hardware decide qual fica em cima. Exposto como a opcao "Tela" das Configurações.
17. **Maquina de telas `g->screen` (u8) substitui `bool inMenu` (v0.9):** sao 10+ estados com "voltar" e contexto (Config abre do menu e da pausa); bool nao escala. Custo +1B (`bool` vira `u8` de mesmo tamanho; `scrPrev` e o byte novo).
18. **Mundo celular + regioes, estilo Victoria 3 + frontwars.io (pos-v0.9, marcos M1-M7 do `DESIGN.md`):** territorio em celulas (mapa-mundi 512x192, dono `u8`) pintadas como em frontwars; economia/POPs/obras por **fatia de regiao** (~600 regioes x ate 4 donos), como os estados do Victoria 3. Celula = visual e militar, regiao = economica: a guerra fica fluida e a economia cabe no ARM9. O mundo sai do `GameState` (trava de 14KB) para estado proprio em `.bss` com `_Static_assert` novo (~200KB). Substitui o mapa 32x24 de `Tile` e a marcha tile-a-tile. Detalhes de design: `DESIGN.md` sec 4.
19. **Paises como dados em ROM com bandeira procedural:** >=250 paises/povos nao cabem com arte de bandeira individual; cada bandeira e um descritor de ~6B (padrao + 3 cores + emblema compartilhado) desenhado em 16x11 e 40x58. Cena de selecao por regiao cultural, nao por pais. `DESIGN.md` sec 6.
20. **Historia viva por eventos condicionais, nao por script (mapa-mundi 1800-2025):** cada evento historico tem ancora, janela e condicoes; se o mundo divergiu, o evento nao acontece (ou dispara a alternativa). Mantem o determinismo (RNG unico) e deixa o jogador mudar a historia. `DESIGN.md` sec 7.
21. **Terminacao garantida em laco de desenho/bake + smoke host de render (v0.8, incidente 2 da tela preta):** `LineP` (Bresenham de `px_map.c`) parava so' em `x==x1&&y==y1`; em reta a 45 graus (`|dx|==|dy|`) nunca chegava no destino, `err` estourava o `int` e o `while(1)` travava — como o `Px_Tick` (bake) roda ANTES do render, o frame 0 nunca desenhava e as duas telas ficavam com o preto do `Px_Init`. Fix: laco por **contador** (`passos = max(|dx|,|dy|)`) e Bresenham canonico. Junto, `TileKey()` passou a `0x80000000u|...` (chave valida nunca e' 0) porque `SNAP[]` zerado colidia com tile vazio (dono 0/planicie) e pulava o bake -> pixel transparente. **Regra derivada:** todo laco de desenho/bake tem limite explicito, e existe o smoke `tools/hostpx/run.sh` (host, sem libnds) que pega travamento e pixel transparente. "Tela preta" agora tem 2 familias a distinguir: pixel sem `BIT(15)` (item 13) e "nunca desenhou" (trava/excecao antes do render). Prova e licao em `docs/POSTMORTEM-TELA-BRANCA.md` sec 10.

22. **IA do mapa-mundi faz guerra limitada; anexacao inteira e coisa de evento (`world.c`, M1):** sem freio, as frentes engoliam potencias inteiras (EUA pela BNA em 1820, Franca pela HAB em 1850, Portugal pela ESP em 1841) e a historia viva quebrava em cascata (so 138/194 eventos). Tres regras: (a) **piso** `AtFloor()` — no mapa-mundi a IA nao ataca nem continua frente contra pais que ja caiu a 2/3 do seu pico de celulas (`WCountry.peak`; 1/3 com historia desligada; jogador nunca e protegido nem limitado); (b) **objetivo de guerra** `warGoal` = 1/3 do alvo + 4 na guerra declarada pela IA, que assina a paz ao cumprir; (c) **guerra historica** (`World_DeclareHistWar`, usada por `HE_WAR` e pelas guerras de 1/1/1800) entra numa lista `hw[32]` (64B, nao uma 2a matriz de 8KB) e a IA nao assina paz nela — so o tratado do evento, ou cansaco no teto (`warDays`=3000) com 1%/decisao como rede de seguranca se o tratado falhar. Declarar guerra no mapa-mundi ficou por mil (3/1000 com historia, 10/1000 sem). Resultado no teste host (seed 42): 183/194 eventos historicos, 143 paises vivos em 2025, toda morte de potencia e historica e na data. Mapa aleatorio inalterado. Alternativa rejeitada: objetivo unico por pais — nao cobre o defensor nem guerras multiplas (Franca x coalizoes).

23. **Mapa-mundi jogavel no DS: tela `SCR_WORLD` propria, nao o mapa 32x24 (v0.9 + M1/M4, 2026-10-17):** o mundo celular ja era o jogo do `DESIGN.md`; esperar a F5 (ponte no 32x24) atrasaria tudo. Entao o fluxo da concept (Modo -> Preparar -> Nacao) leva direto a `SCR_WORLD`, e o classico 32x24 vira a opcao "Clássico (v0.7)" do Preparar. Decisoes: (a) `World` + grades em `.bss` (fora do `GameState`, DESIGN 9.1); (b) **mapa em 2 passadas** — passada 1 grava o dono efetivo de cada pixel num buffer 256x172 (`KEY`), com cantos EPX a partir de 2 px/celula (diagonal no lugar da escadinha); passada 2 pinta e acha fronteira comparando pixels vizinhos (borda de 2px nos dois lados = o brilho do Victoria 3, igual em todo zoom), espuma de costa e relevo por hash de celula (nao vira papel de parede). Custo: 88KB de `.bss`, leitura local. Alternativa rejeitada: borda por celula (custava 4 leituras de grade por pixel e nao suavizava). (c) **5 zooms** (Z0 mundo 0,5 px/celula, Z1 1, Z2 2, Z3 4, Z4 8) — o DESIGN 4.2 ganhou o Z2 de 2px entre continente e regiao. (d) Ficha = pais sob o cursor (sem A para selecionar); **A ataca** com a % escolhida, e guerra nova pede o 2o A e custa 5 de prestigio. (e) **Desembarque** (`World_Landing`): sem fronteira terrestre, a frente comeca na praia do alvo mais perto da sua capital (ate 96 celulas, 1/10 da reserva) — sem isso uma ilha (Grã-Bretanha, Japao) nunca jogava. So' o jogador desembarca; a IA segue so' por terra (deterministico igual). (f) Ouro do mundo ganhou gasto: `World_Recruit` (metade do teto -> +1/4 do teto, ate 150%). (g) Paz proposta (`World_ProposePeace`) vale ate em guerra historica: o jogador sempre pode mudar a historia. (h) Layout Invertida (Settings) so' na partida do mundo: `lcdMainOnBottom` poe o mapa na tela de toque (arrastar = pan, tocar = cursor); menus ficam como a concept. (i) Velocidade 5 redesenha o mapa 1 dia sim, 1 nao. Smoke host (`tools/hostpx`) agora navega como um jogador e salva screenshot de cada tela. **Nao medido ainda:** ms/dia e ms/render no DS real (no PC: 0,045 ms/dia).

24. **Mapa suave e realista com cache incremental, nao mapa de blocos (2026-10-17):** o pedido foi "menos pixelado, mais suave e realista" sem estourar o DS. (a) **Pintura:** cada pixel interpola as 4 celulas vizinhas (pesos bilineares inteiros) depois de um *domain warp* de ate' ~0,4 celula lido de uma tabela de ruido 64x64 gerada 1x por hash (nao usa o RNG do jogo): costa e fronteira saem curvas. Terra = cor do pais 78% + terreno 22% + **hillshade** (derivada do relevo, luz de noroeste) + faixa de praia + textura de copa/areia/neve por hash; mar = raso turquesa -> fundo pela distancia da costa + espuma; tudo com **Bayer 4x4** (gradiente liso em 15 bits). O campo relevo/profundidade e' o `W_LIM` (98KB que ja existiam e ficavam ociosos apos a geracao: `World_Field`, 0 byte novo). Z0 tira a media de 2x2 celulas (antisserrilhado). (b) **Cache:** pintar assim custa ~150-300 ciclos/px (~0,1-0,2 s a tela no ARM9), entao a camada vive em `LAYER` (88KB) + `KEY` (44KB, dono por pixel). Tela toda so' em troca de zoom/modo/paleta; pan desloca o cache e pinta a faixa nova (~1000 px); conquista marca o bloco 8x8 em `World.dirtyBlk` (192B) dentro do `SetOwner` e so' ele e' repintado. Fronteira = pixel com vizinho de outro dono (2px, ouro no seu reino) e e' feita na mesma passada. (c) **Overlay** (frente piscando, contorno do pais sob o cursor) sai do `KEY` a cada desenho sem tocar no cache. Medido no PC: 10 anos com render caiu de 9,8 s para ~6 s (ASan). Alternativas rejeitadas: buffer-mundo inteiro pre-pintado (512x192 x 64 = 6MB em Z4, nao cabe) e smoothing so' por EPX (ainda escadinha em 8px). A regra "sem gradiente" do DESIGN 3.5 vale para a UI; o mapa usa gradiente pontilhado por realismo (DESIGN 4.3).

25. **Cidades estilo Victoria 3 + Imperator, com fundacao (2026-10-17):** o mundo ganhou `WCity` (24B; ate 512 = 12KB dentro do `World`). Mapa-mundi: ~190 cidades historicas de 1800 (tabela em `city.c`, populacao aproximada) + capital de quem nao tem + vilas geradas (~1 a cada 140 celulas). Niveis povoado -> vilarejo -> cidade -> metropole por populacao (10k/50k/400k) e obras exigidas (mercado; universidade ou fabrica). Slots 2/3/5/8; 11 obras com efeito real no motor: ouro/dia, comida (crescimento), teto de tropas (entra no `Recap`), prestigio/ano e defesa (multiplica o `CellCost` da celula da cidade). Crescimento logistico (~1,5%/ano base, ~4% com fazenda/porto/ferrovia). Fundar: celula propria a 5+ celulas de outra cidade, preco **200 + 120 x cidades que voce ja tem** (mostrado antes do 2o toque). Captura saqueia 1/8 da populacao e cancela a obra. IA constroi pela ordem do caminho de nivel e funda se tem ouro e terra vazia — sem consumir `w->rng` (hash), entao o replay continua deterministico. Arte 100% procedural (`px_city.c`, sem PNG): o mesmo pintor faz icone, miniatura e predio, em 5 estilos de cultura e 3 eras; a cena reusa os 88KB do `LAYER` do mapa e so' os sprites vivos (pessoas, carroca/carro, fumaca, trem, barcos) sao redesenhados, ~20 quadros/s. Efeito no mundo (teste host, seed 42): 186/194 eventos historicos (antes 183), replay ok, 0,06 ms/dia no PC.

26. **Economia em 4 mercados com portos, nao renda por celula (2026-10-17):** o ouro do mundo era `celulas/16` por dia e acumulava sem uso (59k em 1833). Agora: **9 bens** (graos, madeira, fibra, minerio, luxo, texteis, ferramentas, armas, petroleo a partir de 1860). O campo produz pela contagem de terreno do pais (`WCountry.terr`, 22B, atualizada O(1) no `SetOwner`) com produtividade subindo com a era; as cidades produzem pelas obras (fazenda, serraria, mina, oficina, fabrica, arsenal, poco) e pelo potencial do entorno medido 1x na criacao (`WCity.agri/wood/ore/res`, raio 3). Manufatura consome insumo: a industria do mercado roda a `ind%` do insumo que mais falta (`WEco.lack`). Mercados: **local** (a cidade come primeiro do entorno: `WCity.food`), **nacional**, **bloco** (mercado comum: soma oferta/demanda, preco unico, alfandega menor) e **mundial** (sobra/falta cruza o mar ate a capacidade dos portos; preco mundial = importacao pedida / exportacao oferecida, 0,4x-3x). Receita = imposto sobre o valor produzido (3 niveis) + taxas urbanas + **alfandega** sobre o volume externo (porto da' dinheiro); despesa = soldo (tropas/60), frota e 1% do tesouro (cofre parado custa: teto natural, sem inflacao). Efeitos: comida -> crescimento da cidade e fome; nivel de vida -> felicidade; armas -> regeneracao do exercito e preco do recrutamento; ferramentas -> ritmo de obra; madeira+ferramentas -> preco das obras. Custo: fecha 1x/semana em O(cidades + paises x bens) + rotas O(portos x 3 x paises), ~27KB de buffers estaticos, sem RNG (replay intacto). Medido no host: 0,104-0,109 ms/dia no PC (era 0,06 so' com cidades); vida media 73, precos mundiais dentro de 1/3x-3x ate 2025. Alternativa rejeitada: POPs por celula (Victoria 3 de verdade) — 98k celulas x estratos nao cabem nem em RAM nem em CPU; o pais + cidades e' a granularidade certa no DS.

27. **Ideologia com bandeira, IA que conversa e marinha (2026-10-17):** pedido: mais interacao entre IAs, imprevisibilidade e "a Franca vira comunista e troca a bandeira". (a) **Ideologia** (`ID_*`: tradicional, liberal, conservador, comunista, autoritario, teocratico) soma modificadores aos traits e transforma a bandeira de base *por regra*, sem asset novo (`WDip_FlagIdeo`): comunista recolore cores frias para vermelho e poe a estrela dourada; autoritario troca azul por preto e poe a aguia; liberal tira a coroa; tradicional poe a coroa; teocratico, crescente ou cruz. Vale para os 211 paises do catalogo. (b) **Revolucao** por risco (militancia, felicidade, nivel de vida, guerra longa, imposto alto e um *espirito da epoca*: o velho regime envelhece depois de 1848, o partido unico cansa depois de 1975) sorteia a nova ideologia por era + contagio (bloco e aliados); 1 em 3 vira **separatismo** (nacao adormecida do catalogo com capital dentro do pais renasce e luta pela independencia). Eventos historicos novos/ajustados poem a ideologia real (1917, 1922, 1933, 1939, 1949, 1959, 1964, 1973, 1989-91). Medido: ~60-150 revolucoes/separacoes em 225 anos (seed 42). (c) **Diplomacia da IA**: rival, ate 2 aliados com **chamada as armas** nas guerras nao historicas (as historicas seguem o roteiro), **blocos** regionais (mercado comum, ate 12, lider mercantil ou grande), temperamento sorteado por decada (muda a chance de guerra e de alianca), **tributo** exigido do jogador por rival forte e paz oferecida por quem perde. (d) **Propostas ao jogador** (`WProp`, 1 por vez, recusa automatica em 30 dias): alianca, bloco, pedido de ajuda, tributo, paz e **decisoes** de evento com 2 opcoes (22 novas) — o jogo pausa e mostra um despacho com Aceitar/Recusar. (e) **Marinha**: estaleiro (exige porto) constroi navios; frota amplia o comercio, **bloqueia** o inimigo (comercio a 1/3), barateia o desembarque (sem navio custa o dobro; frota inimiga maior afunda transportes) e trava batalhas navais mensais. Custo: cada pais pensa 1x/mes escalonado (~8 paises/dia, varreduras de no maximo 32 amostras ou 250 paises); usa `w->rng`, replay deterministico.

28. **Fonte pequena desenhada a mao + arte de menu gerada (2026-10-17):** a Serifa 8 vinha de Cambria rasterizada em 7px com limiar de 50% sobre o antisserrilhado: hastes e serifas sumiam ("fonte bugada"). Agora a Serifa 8 e' **pixel art** (`tools/pixfont8.py`, ASCII art, acentos compostos por regra) e a Serifa 12 usa o hinting monocromatico do FreeType (`fontmode="1"`). O logo e o retrato do oficial da concept saem de `tools/genart.py`: desenho a 4x e reducao por **voto de maioria** na paleta (a media misturava tinta e creme numa terceira cor). Custo: ~16KB de ROM (RLE 4bpp). **Logo e oficial substituidos pelas pinturas da concept no ADR-29** (a fonte pequena segue valendo).

29. **Menus com as pinturas da propria concept + Serifa 12 antisserrilhada (2026-10-17):** o pedido foi "arte dos menus IGUAL a concept, mais moderna e menos pixelada". Desenhar de novo em pixel art (ADR-28) nunca chegaria la'. Decisao: **recortar a concept** (cada tela dela tem ~290x200 px, 1,13x o DS) e **reamostrar com Lanczos** para o pixel nativo — some o bloco duplicado e fica a pintura. `tools/genpics.py` gera 16 imagens 8bpp (ate 255 cores 15 bits, indice 0 transparente; RLE nao compensa em pintura com degrade e o blit por linha e' ~1 ms a tela): titulo inteiro (o texto "Aperte A" e' apagado do botao e escrito pelo jogo no idioma escolhido), oficial, cidade, porto, brasao e gravura (estes dois com o papel transparente: so' a tinta), papel sintetico com as cores medidas da concept, 4 cantos do pergaminho e 4 da moldura navy, coroa. O estandarte do porto vira **mascara de sombra** (64x48, 0 = nao e' pano, 12 = luz normal): o jogo pinta a bandeira da nacao escolhida modulada pelas dobras. `Ui_Parch` passou a usar o papel (preso a' tela: paineis vizinhos casam a textura) com borda queimada; `Ui_ParchOrn`/`Ui_NavyOrn` (so' menus) poem os cantos — no jogo os cantos cobriam texto. Botoes e a faixa de selecao ganharam degrade sutil e ponta de fita (como na concept). A **Serifa 12 virou 2bpp** (Cambria 13px com antisserrilhado do FreeType, 4 niveis) misturada com o fundo: titulos e itens deixam de serrilhar; a Serifa 8 (7px) segue 1bpp desenhada a mao, onde AA so' borraria. Custo: ~175KB de ROM (ROM 624KB, cabe com folga nos 4MB), 0 byte de `.bss`. Removido `tools/genart.py`/`art_data.c` (logo/oficial desenhados, ADR-28). Alternativa rejeitada: arte nova feita a mao — mais lenta e nunca "igual".

30. **Mira no centro, setas movem o mapa, painel so' caneta (2026-10-17):** antes as setas andavam um cursor e a camera seguia com margem; A atacava/fundava direto e o painel aceitava D-pad e caneta ao mesmo tempo. Pedido: "as setinhas movem o mapa, o menu de baixo so' com a caneta, um pontinho no meio da tela de cima; ponho o pontinho sobre o pais e clico um botao". Agora: (a) a celula alvo (`curX/curY`) fica no **centro da vista** (`Center()` a cada passo; nas bordas sem volta do mundo a camera encosta e a mira sai do centro), passo ~4 px/quadro em todo zoom (dobra apos 24 quadros); zoom mantem a mira no alvo. (b) **A abre a folha de acoes** do alvo (sua cidade / sua terra / terra livre / pais) com so' o que e' possivel e o preco no rotulo; declarar guerra ja' manda a % da reserva (um gesto so'); recrutar mantem a folha aberta. (c) Painel, despachos, Gabinete e lotes da cidade **so' caneta**; no layout Invertida (painel sem toque) a folha e os despachos voltam a aceitar teclas. (d) **Bugs corrigidos na mesma rodada:** (1) *pedestres sobre o mapa* — a cena da cidade cacheava o fundo no `LAYER` emprestado do mapa; o mapa repintava o `LAYER` mas o cache da cena continuava "valido", entao reabrir a cidade mostrava o mapa com a gente andando por cima. `PxM_Update` agora invalida a cena (teste no hostpx: reabrir difere em 664 px animados; sem a correcao, 43.908 px); (2) despacho da IA chegando na cidade/Gabinete pausava o jogo sem mostrar nada (agora espera a volta ao mapa); (3) cidade tomada com a tela aberta continuava aberta; (4) Gabinete agia com o jogador derrotado; (5) aliado no 2o slot com o 1o vago aparecia como "nenhum". Smoke host navega tudo por toque.

## 12. GUIA PARA AGENTS (como mexer sem quebrar o jogo)

- **Adicionar obra (24a):** 1) `types.h:BuildingKind` antes de `BLD_COUNT`; 2) `economy.c:Eco_Cost/BldName/BldDesc/Eco_Tile`; 3) `px_tiles.c:TilePx` (cor); 4) `ai.c` area 0 (quando construir); 5) SDD sec 2 + `diagramas/arquitetura.mmd`. Nao reordene IDs existentes.
- **Adicionar bem (9o):** 1) `config.h:GOOD_*` + `GOOD_COUNT`; 2) `market.c:BASE/KOf/Name/Short`; 3) `pops.c:Pop_Need`; 4) `economy.c` (quem produz/consome); 5) `input.c` gira `good` (automatico por `GOOD_COUNT`); 6) SDD sec 2. Atenção: `Faction.stock/priceInt/supInt/demInt` e `priceExt/supExt/demExt` crescem (mede `GameState`!).
- **Mudar balance:** mexa em `BASE/k` (`market.c`), custos (`Eco_Cost`), `throughput` (`Eco_Tile`), `Pop_Need`, `AI_Budget/BotQ7`, `MarchDays/Combat_Pow`. Teste: mesma seed deve dar mesma historia (RNG so via `Rng_*`); rode `gcc -fsyntax-only -Iinclude source/*.c` com stub `nds.h` p/ logica (input/px exigem libnds real).
- **Armadilhas:** `s16` estoura em 9999 (use `AddStock`/capa); `u8` estoura em 255 (pops nibble max 15 via `popSet`); `Mkt_CloseDay` zera sup/dem (nao acumule entre dias); `Eco_Build` exige tile proprio vazio transitavel; `Dip_Ally` exige rel+ouro; `Evt_Day` exige `dirty=true`; `tileWalk` bloqueia AGUA/MONTANHA/VULCAO; fonte sem acento (mensagens sem acento).
- **Adicionar governo (4o):** 1) `types.h:GovKind` antes de `GOV_COUNT`; 2) `gov.c:Name/Short`; 3) efeitos em `economy.c`/`pops.c`/`combat.c` (guarde com `F->gov==...`, barato); 4) `input.c` (gira sozinho por `GOV_COUNT`); 5) SDD sec 6. Nao reordene IDs existentes.
- **Checklist antes de commitar:** 1) logica compila (`gcc -fsyntax-only`); 2) `make` (local: `source ~/devkitpro/env.sh`) com **zero warnings** nos nossos arquivos; 3) sim host 200 dias + replay deterministico; 3b) mexeu em `px_*`/`input.c`? rode `tools/hostpx/run.sh` (smoke de render: hang + pixel transparente); 4) 5-min playtest (sec 10); 5) SDD + diagramas atualizados; 6) `BLD_COUNT`/`GOOD_COUNT`/`GOV_COUNT` consistentes (giro, desenho, IA).

## 13. ROADMAP (o que falta — v0.8+)

> O plano **ordenado** (baseline honesto, dependencias, riscos e criterio de pronto) esta em
> **`docs/ROADMAP.md`** (+ `docs/roadmap.png`). Esta secao e o resumo tecnico.

- **v0.9 = UI da concept art** (menus, selecao de modo/nacao, HUD, pausa, configuracoes, fonte com acento, assets ilustrados, audio, multijogador, SRAM). Plano completo e fases F1-F8 na **sec 14.9**.
- **v1.x = o mundo** (`DESIGN.md`): mapa celular estilo Victoria 3 + frontwars.io, mapa aleatorio ou mapa-mundi 1800, >=150 paises historicos e povos indigenas com traits e flavor, historia viva ate 2025 (depois, Modo Eterno). Marcos M1-M7 em `DESIGN.md` sec 9; ADRs 18-20.
- Leis especificas por governo (editos: alistamento, feriado, embargo) via SELECT+CIMA/BAIXO; 4o governo (ex: republica mercantil); IA troca governo com estrategia (nao so panico).
- SRAM persistente (save no cartao, nao so RAM); fila multi-marcha; som (MaxMod); mais eventos por bioma; painel com 8 precos (rolagem SELECT); data ano/mes/dia visivel; throughput visivel no painel (`FALTA CARVAO`); balance por telemetria (facil 80%/insano 20%); i18n (acentos: resolvido na v0.9 pela Serifa 8/12, sec 14.3).
- Glossario rapido: int/ext, tarifa, uniao=ALLY, clima (S seca/C chuva), evento, marcha, tesouro, prestigio, literacia (boost industria+prestigio), militancia (risco revolta), throughput (produz menos sem insumo), seed (historia deterministica).

## 14. DIRECAO DE ARTE + TELAS (alvo v0.9 — a concept art e o contrato)

> Referencia unica: `concept_arts/image.png` (8 paineis de tela dupla). Status (2026-10-17): **F1
> fechada no codigo e F2 em andamento** — componentes em `px_ui.c` (`Ui_Parch/Navy/Item/BigBtn/IconBtn/ValueRow/Slider/Div/Crown/Icon`) e as telas MAIN, MODE, SETUP, NATION, PAUSE, OPTIONS, CREDITS em `px_menu.c`/`menu.c`, conferidas por screenshot do smoke host (`tools/hostpx`, sec 10); falta o screenshot do melonDS. Status anterior: **F1 EM
> ANDAMENTO** — a fundacao ja' esta no codigo (`include/pal.h`, `source/settings.c`,
> `source/str.c`, `source/px_art.c`, `source/px_font2.c` + `source/gen/font_data.c`, e
> `g->screen` no `types.h`); as telas (F2+) ainda nao. O preview da Serifa gerada esta em
> `docs/font_preview.png`. Em conflito, esta secao manda sobre sec 8 (controles)
> e sec 9 (render), que descrevem o estado atual. Medidas em pixels nativos do DS (256x192
> por tela), tolerancia +-4px. Textos entre aspas sao o texto FINAL da tela (com acento).
>
> **`DESIGN.md` (raiz) manda no design:** espirito visual e regras de composicao (sec 3),
> o mapa estilo Victoria 3 + frontwars.io (sec 4), paises/povos/traits (sec 6) e historia
> 1800-2025 (sec 7). Esta secao traduz isso em pixels. Se as duas divergirem, corrija esta.

### 14.0 Como ler a concept art

- Cada painel = 1 estado de tela, com as DUAS telas (cima e baixo). A legenda embaixo do painel e o nome do estado (sec 14.2).
- **NAO renderizar:** a carcaca do DS, as legendas ("MENU PRINCIPAL", ...), a coluna da esquerda (logo + "CONCEPT ART MENUS" + "Grandes impérios não nascem do acaso.") e a da direita (coroa, "OH MY VICTORIA DS / ESTRATÉGIA · IMPÉRIO · HISTÓRIA", navio em traco, "Porque o mundo é maior do que um único reino."). Isso e branding: README, capa, tela de creditos (sec 14.5).
- A concept **nao esta em resolucao nativa** (cada tela tem ~290px de largura na imagem, com anti-aliasing). Nunca recorte e reescale: todo asset e redesenhado 1:1 em pixel nativo (sec 14.6). As medidas abaixo ja estao convertidas.
- Telas que a concept nao mostra (carregar, creditos, preparar partida, vitoria, abas de obras/mercado/relacoes) seguem a gramatica da sec 14.5.

### 14.1 Linguagem visual (tokens de cor e componentes)

Todos os tokens vivem em `include/pal.h` como constantes `PXC(r,g,b)` (5 bits por canal, **ja com `BIT(15)`**, gemeo de compilacao do `Px_Col`, ADR-13). Proibido cor literal fora de `pal.h`/`px_map.c`.

| Token | Uso | Hex aprox. | `PXC(r,g,b)` |
|---|---|---|---|
| `C_NAVY_BG` | fundo de tela, moldura externa | `#121826` | `(2,3,4)` |
| `C_NAVY` | paineis e botoes escuros, barra do HUD | `#1E2738` | `(3,4,7)` |
| `C_NAVY_HI` | botao pressionado, placeholder de arte | `#2C3850` | `(5,7,10)` |
| `C_PARCH` | pergaminho (conteudo) | `#E4D2A6` | `(28,26,20)` |
| `C_PARCH_SH` | manchas/sombra do pergaminho, gravuras | `#C4AC7A` | `(24,21,15)` |
| `C_INK` | texto e tracos sobre pergaminho | `#2A2018` | `(5,4,3)` |
| `C_CRIMSON` | item selecionado | `#9A1C1F` | `(19,3,3)` |
| `C_CRIMSON_HI` | brilho de 1px no topo da selecao | `#C2302E` | `(24,6,5)` |
| `C_GOLD` | bordas, ornamentos, icones | `#C8A04A` | `(25,20,9)` |
| `C_GOLD_DIM` | separadores, borda interna, icone desabilitado | `#7E6532` | `(15,12,6)` |
| `C_TEXT` | texto sobre navy | `#EFE6D2` | `(29,28,26)` |
| `C_TEXT_DIM` | item desabilitado | `#8A8F99` | `(17,17,19)` |

Componentes (todos em `source/px_ui.c`, desenhados a partir de tabelas como o `ui.c` da v0.8 — nada de geometria solta):

- **Painel pergaminho:** fill `C_PARCH` + manchas deterministicas (hash por pixel, mesmo truque do `px_map.c`, sem usar o RNG do jogo) + borda dupla (1px `C_INK`, 1px vazio, 1px `C_GOLD_DIM`) + cantos ornamentados `orn_corner` 12x12 espelhado nos 4 cantos.
- **Painel navy:** fill `C_NAVY`, borda 1px `C_GOLD` + 1px interna `C_GOLD_DIM`.
- **Item de lista (sobre navy):** altura 24, texto `C_TEXT` com sombra 1px `C_NAVY_BG`; separador 1px `C_GOLD_DIM` entre itens. **Selecionado:** barra `C_CRIMSON`, 1px `C_CRIMSON_HI` no topo, borda `C_GOLD`, seta `▶` branca a 6px da esquerda.
- **Botao grande (Selecao de Modo):** 176x26, fill `C_NAVY` + borda `C_GOLD`; icone 18x18 a x+10; texto a x+44. Selecionado = fill `C_CRIMSON`.
- **Botao icone (HUD):** 34x34, `C_NAVY` + borda `C_GOLD`, icone 20x20 centrado. Pressionado = `C_NAVY_HI` e desloca 1px p/ baixo. Desabilitado = icone em `C_GOLD_DIM`.
- **Ornamentos:** coroa `orn_crown` 16x12 no rodape de paineis de menu; divisor `orn_div` (—◆—) 48x5 sob titulos. No maximo 1 coroa por painel.
- **Animacao em menu:** so a barra selecionada pulsa (`C_CRIMSON_HI` liga/desliga a cada 30 frames) e o "Aperte A". Menu parado NAO redesenha a 60Hz (regra `dirty` da sec 9).
- **Transicao entre telas:** fade por `REG_MASTER_BRIGHT` / `REG_MASTER_BRIGHT_SUB` em 8 frames (hardware, custo zero de pixel).

### 14.2 Maquina de telas

`g->screen` (u8, enum `Screen` em `types.h`) substitui `g->inMenu` (ADR-17). `g->scrPrev` guarda quem abriu Configurações. Toda troca seta `dirty` + `dirtyBot`.

| Estado | Tela de cima | Tela de baixo | A | B |
|---|---|---|---|---|
| `SCR_MAIN` | splash do titulo + "Aperte A" | retrato do oficial + lista | abre item | — |
| `SCR_MODE` | lista de modos | miniatura + descricao do modo destacado | Contra IA -> `SCR_SETUP`; Campanha -> `SCR_NATION`; Multijogador -> `SCR_MULTI` (F7) | `SCR_MAIN` |
| `SCR_SETUP`* | mapa (mundi/aleatorio), tamanho, historia, dificuldade, semente | previa do mapa + resumo | `SCR_NATION` | `SCR_MODE` |
| `SCR_NATION` | carrossel de 5 estandartes + nome + descricao | cena regional + mapa do pais | `Game_New` | `SCR_SETUP` (ou `SCR_MODE` na Campanha) |
| `SCR_GAME` | barra de recursos + mapa + minimapa | ficha do tile/unidade + 4 acoes | acao contextual | cancela alvo |
| `SCR_PAUSE` | mapa congelado escurecido + lista | descricao do item + gravura | abre item | Continuar |
| `SCR_OPTIONS` | pergaminho com 4 ajustes | brasao + frase | — | volta p/ `scrPrev` (grava Settings) |
| `SCR_LOAD`* | lista de slots | resumo do slot | carrega | `SCR_MAIN` |
| `SCR_CREDITS`* | creditos rolando | navio em traco + frase | — | `SCR_MAIN` |
| `SCR_END`* | overlay "Vitória!"/"Derrota" no mapa | retrato + "A — Voltar ao menu" | `SCR_MAIN` | — |

\* nao aparece na concept; segue a sec 14.5.

**Input em menus:** CIMA/BAIXO move a selecao (com wrap), A confirma, B volta, ESQ/DIR muda valor (Nação, Configurações, Preparar). START no jogo abre `SCR_PAUSE`. **Toque:** lista na tela de baixo = tap seleciona e confirma; quando a lista esta na tela de cima (Modo, Nação, Pausa, Configurações), tap em qualquer ponto da tela de baixo = A.

### 14.3 Tipografia

- **Serifa 12** (titulos e itens de lista/botao): bitmap proporcional, caixa alta 9px, linha 14px. Ex.: "Novo Jogo", "Escolha o Modo", "Configurações", "Reino Unido", "Infantaria".
- **Serifa 8** (descricoes, rotulos, numeros do HUD): proporcional, caixa alta 7px, linha 11px. **Desenhada a mao** em `tools/pixfont8.py` (ADR-28).
- Conjunto de glifos (ambas): ASCII 32-126 + `ÁÀÂÃÉÊÍÓÔÕÚÜÇáàâãéêíóôõúüçºª` + `◂ ▸ ▶ ◆ ✦ … · —` + **toda letra usada nos nomes de paises/povos/regioes** (`ā ē ī ō ū ŋ č š ž ñ ł ʻ Đ ạ ệ ...`, `DESIGN.md` sec 3.4). O conjunto final e **gerado a partir de `str.c`**: uma ferramenta lista os caracteres usados e o build falha se faltar glifo. Serifa 8 em 1bpp; **Serifa 12 em 2bpp antisserrilhada** (4 niveis de cobertura misturados com o fundo em `px_font2.c:Mix`, ADR-29). ~12KB ROM no total (`source/px_font2.c`).
- **Fonte 3x5 atual:** sai das telas do jogador; fica so p/ overlay de debug e placeholders.
- **Logo** "Oh My Victoria DS" (com coroa e pincelada vermelha) e **arte**, nunca fonte.
- **Strings:** todas em `source/str.c` (`Str(STR_NOVO_JOGO)`), tabelas PT e EN, UTF-8 no fonte, decodificadas por `Px_Text2` p/ indice de glifo. Idioma vem de `Settings.lang`. Proibido literal de UI fora de `str.c`.
- Quebra por palavra: `Px_Wrap(dst,x,y,w,str,font,col)`. As descricoes da concept cabem em 4 linhas de 124px em Serifa 8.

### 14.4 Telas da concept (layout por tela)

Coordenadas `(x,y,w,h)` em pixels da tela respectiva.

#### MENU PRINCIPAL (`SCR_MAIN`)
- **Cima:** `art_title` 256x192 em tela cheia — Westminster/Big Ben ao por do sol, Tamisa com veleiro, logo com coroa no terco superior. Caixa "Aperte A" em `(176,164,72,20)`: painel navy, icone pena 10x10 a x+5, texto Serifa 8 `C_TEXT`; pulsa (30 frames visivel / 15 apagada).
- **Baixo:** fundo `C_NAVY_BG`; moldura ornamentada `C_GOLD` em `(2,2,252,188)`. Retrato `art_officer` 88x160 em `(8,16)` (oficial vitoriano de quepe, meio-perfil virado p/ a direita). Painel navy da lista em `(104,14,144,162)`. Itens Serifa 12 (altura 24, primeiro em y=22, passo 30): "Novo Jogo", "Carregar", "Opções", "Créditos". Coroa em `(168,166)`. "Carregar" fica `C_TEXT_DIM` e nao seleciona se nao existe save.

#### SELEÇÃO DE MODO (`SCR_MODE`)
- **Cima:** fundo navy; pergaminho `(12,6,232,180)`; gravuras laterais tenues `orn_engrave_l/r` 24x120 em `C_PARCH_SH`. Titulo "Escolha o Modo" Serifa 12 centrado em y=16; divisor em y=32. 3 botoes grandes em x=40, y=46/82/118: [coroa] "Campanha", [espadas cruzadas] "Contra IA", [grupo de pessoas] "Multijogador".
- **Baixo:** fundo navy; pergaminho `(12,12,232,168)`; miniatura `art_mode_<modo>` 80x96 em `(22,34)` com borda `C_GOLD`; texto Serifa 8 `C_INK` em `(110,40)`, largura 124, quebra por palavra:
  - Campanha: "Conquiste territórios, desenvolva sua nação e escreva sua história." (texto da concept)
  - Contra IA: "Partida livre no mapa-múndi de 1800 ou num mundo aleatório. A história é sua."
  - Multijogador: "Desafie um amigo em outro DS, sem fio." + linha "Em breve." enquanto F7 nao entrar (item fica `C_TEXT_DIM` e nao confirma).

#### SELEÇÃO DE NAÇÃO (`SCR_NATION`)
- **Cima:** fundo navy; pergaminho `(10,8,236,176)`; titulo "Escolha sua nação" Serifa 12 em y=16 + divisor. **Carrossel:** 5 estandartes visiveis (bandeira procedural 40x58, haste dourada, ponta em V — `DESIGN.md` sec 6.2.1) em y=34, x=16+i*46, de um catalogo de >=150. ESQ/DIR move 1; L/R troca o **filtro de continente** (Todos, Europa, Américas, África, Ásia, Oceania), mostrado como `◂ Europa ▸` em Serifa 8 logo abaixo do titulo. Selecionado: sobe 3px + contorno 1px `C_GOLD`; o carrossel rola p/ manter o selecionado no centro. Setas `◂ ▸` nas laterais quando ha mais. Nome Serifa 12 centrado em y=126, ladeado por `✦` e linha fina `C_INK`. Descricao Serifa 8, 2 linhas centradas a partir de y=144 (texto do campo `descrição` da ficha do pais).
- **Baixo:** cena da **regiao cultural** do pais (`art_region_<r>` 240x176 em `(8,8)`, moldura dupla `C_GOLD`) ou cena propria dos paises de destaque (Reino Unido = a da concept: soldado de casaca vermelha com a Union Jack no Tamisa, Westminster ao fundo). Sobre a cena, canto inferior direito: mini-mapa 64x48 do mundo com o territorio do pais piscando. **Tap no mini-mapa** abre a escolha direto no mapa-mundi (cursor sobre o pais + A).
- **Paises:** catalogo, traits e textos em `DESIGN.md` sec 6 (a concept mostra Reino Unido, França e 3 estandartes que sao exemplos, nao uma lista fechada).

#### JOGO EM ANDAMENTO (`SCR_GAME`) — layout Normal (Invertida troca as telas, ADR-16)
- **Cima — barra de recursos** `(0,0,256,20)`: fill `C_NAVY`, linha inferior `C_GOLD`. 4 slots de 64px separados por 1px `C_GOLD_DIM`; em cada um, icone 12x12 a x+4 e numero Serifa 8 `C_TEXT` a x+20:
  1. [moeda] tesouro — concept: "1200"
  2. [saco] grao em estoque — concept: "850"
  3. [pedras] carvao em estoque — concept: "430"
  4. [coroa] prestigio "atual/600" (rumo a vitoria) — a concept mostra icone de pessoas com "12/20"; trocado p/ prestigio porque o jogo nao tem limite de populacao/exercito. Se um dia houver, volta ao icone de pessoas.
- **Cima — mapa** `(0,20,256,172)`. Direcao de arte: **`DESIGN.md` sec 4** (Victoria 3 + frontwars.io). Resumo: zoom afastado (Z0/Z1) = mapa de papel com cor chapada por pais, fronteiras grossas e nomes espacados; zoom proximo (Z2/Z3) = terreno pintado no estilo da concept (pinheiros densos, costa com areia, castelos nas capitais com bandeira tremulando, casinhas, navios) com a cor do pais por cima a ~30%. Frente de guerra pisca e avanca celula a celula. Continua "cozido" em buffer (v0.8). Ate o M1, vale o mapa 32x24 atual com essa mesma direcao de arte.
- **Cima — minimapa** `(186,138,68,52)`: borda 1px `C_GOLD` + 1px `C_NAVY`; mapa 32x24 a 2px/tile (64x48) com `Px_MiniCol`; retangulo 1px branco = viewport.
- **Baixo — fundo:** `art_game_bg` (paisagem urbana com rio; so a faixa x>=182 aparece de verdade).
- **Baixo — ficha** (painel navy `(6,6,174,132)`): retrato 56x72 em `(12,12)` com borda `C_GOLD`; nome Serifa 12 em `(76,14)` + divisor; 3 linhas Serifa 8 em x=76, y=36/52/68 com icone 10x10: [espadas cruzadas] Ataque (`Combat_Pow` como atacante), [espada] Tropas, [escudo] Defesa (`Map_Def` + muralha/forte). Conteudo contextual:
  - tile proprio com tropas -> "Infantaria" + `unit_infantaria` (como na concept);
  - tile proprio sem tropas, com obra -> nome da obra + glifo grande + producao/dia;
  - tile inimigo -> nome da nacao dona + estandarte + tropas;
  - neutro/vazio -> nome do terreno + site.
- **Baixo — 4 acoes** (botoes icone 34x34 em y=146, x=10/50/90/130): [espada] Marchar (pede alvo: cursor no mapa + A), [escudo] Fortificar (MURALHA -> FORTE), [brasao] Recrutar, [ferramentas] Construir (abre a grade de obras). Mesmas regras de `combat.c`/`economy.c`; acao invalida = botao desabilitado.
- **Abas v0.8** (Obras, Mercado, Relações, Sistema): "Construir" abre Obras; abas tocaveis de 20px no topo do painel de baixo alternam [Ficha] <-> [Mercado] <-> [Relações], com a gramatica da sec 14.5. **L/R = zoom do mapa** (afastar/aproximar) e **Y = modo de mapa** (`DESIGN.md` sec 4.2/4.4). Sistema (governo, imposto, velocidade, save) vai p/ a Pausa. Os combos (SELECT+L, R+CIMA, B+BAIXO...) viram atalhos opcionais.

#### PAUSA / MENU DO JOGO (`SCR_PAUSE`)
- **Cima:** ultimo frame do mapa escurecido 1 vez ao entrar (`((c>>1)&0x3DEF)|BIT(15)` por pixel, nao por frame); painel navy `(64,30,140,126)`; itens Serifa 12 com `✦` dourado a esquerda dos nao selecionados (como na concept), passo 26 a partir de y=40: "Continuar", "Salvar", "Configurações", "Sair para o Menu". Coroa em `(126,142)`. A simulacao para enquanto a pausa estiver aberta.
- **Baixo:** pergaminho `(10,10,236,172)`; descricao do item destacado Serifa 8 centrada a partir de y=28, divisor, gravura `art_engrave_port` 216x84 em `(20,92)` (porto com veleiros e catedrais, traco sepia `C_INK` sobre pergaminho).
  - Continuar: "Retome o jogo de onde parou." (texto da concept)
  - Salvar: "Grave seu império no cartão." -> apos A: "Jogo salvo." por 60 frames.
  - Configurações: "Ajuste som, idioma e tela."
  - Sair para o Menu: "Volte ao menu principal. O que não foi salvo será perdido." (pede A de novo p/ confirmar)

#### CONFIGURAÇÕES (`SCR_OPTIONS`)
- **Cima:** fundo navy; pergaminho `(10,8,236,176)`; titulo "Configurações" Serifa 12 em y=16 + divisor. 4 linhas, rotulo Serifa 8 em x=22, passo 30 a partir de y=46:
  - "Música": trilho 112x3 em x=82 (`C_NAVY`, parte cheia `C_INK`), puxador 6x9 `C_GOLD` com borda `C_INK`; valor "80%" alinhado a direita em x=234. ESQ/DIR = -/+10.
  - "Efeitos": idem, "70%".
  - "Idioma": `◂ Português ▸` (◂ em x=86, ▸ em x=226, valor centrado). Opcoes: Português, English.
  - "Tela": `◂ Normal ▸`. Opcoes: Normal (mapa em cima, como a concept), Invertida (mapa embaixo, tocavel) — aplica `lcdSwap()` na hora (ADR-16).
  - Linha selecionada: rotulo em `C_CRIMSON` + `▶` em x=12. Toda mudanca vale ao vivo.
- **Baixo:** fundo navy; pergaminho `(10,10,236,172)`; brasao `art_crest` 96x72 em `(80,28)` (escudo coroado com dois leoes); "Ajuste sua experiência de jogo." Serifa 8 centrado em 2 linhas a partir de y=118.
- Padroes: Música 80, Efeitos 70, Idioma Português, Tela Normal.

### 14.5 Telas fora da concept (mesma gramatica)

Regra de ouro p/ qualquer tela nova: (1) uma tela tem a lista/controle, a outra tem o contexto (descricao ou ilustracao do item destacado); (2) pergaminho p/ conteudo, navy p/ moldura e HUD; (3) selecao sempre crimson com `▶`; (4) no maximo 1 coroa e 1 divisor por painel; (5) texto vindo de `str.c`, com acento.

- **Preparar partida (`SCR_SETUP`):** vem **antes** de `SCR_NATION` (o mapa escolhido define o catalogo). Cima = pergaminho "Preparar partida" com linhas `◂ valor ▸` (mesmo componente das Configurações): "Mapa `◂ Mapa-múndi 1800 ▸`" / "Aleatório"; se aleatorio: "Tamanho `◂ Pequeno ▸`" (Pequeno/Médio/Grande) e "Nações `◂ 16 ▸`" (8/16/32); se mapa-mundi: "História `◂ Ligada ▸`" (Ligada/Só flavor/Desligada); sempre "Dificuldade `◂ Normal ▸`" e "Semente `◂ 0C1A6 ▸`" (ESQ/DIR gera a proxima com `+0x9E3779B9`); botao crimson "Escolher nação". Baixo = previa do mapa (mundo real ou continentes gerados pela seed) em 240x120 + resumo. Campanha pula esta tela (sempre mapa-mundi, historia ligada). Fluxo: `SCR_MODE` -> `SCR_SETUP` -> `SCR_NATION` -> `Game_New`.
- **Carregar (`SCR_LOAD`):** cima = pergaminho "Carregar" com slots ("Dia 142 · Reino Unido · Normal"; 1 slot ate F8, 3 depois); baixo = cena da nacao do slot.
- **Créditos (`SCR_CREDITS`):** cima = pergaminho com creditos rolando 1px a cada 2 frames, `art_logo` no topo; baixo = navy com `art_ship_line` e "Porque o mundo é maior do que um único reino." (frase da concept).
- **Vitória/Derrota (`SCR_END`):** cima = pergaminho central `(48,60,160,72)` sobre o mapa com "Vitória!" ou "Derrota" (Serifa 12) + motivo (prestigio/territorio/eliminado); baixo = retrato do oficial + "A — Voltar ao menu". Substitui o overlay `VITORIA!/DERROTA` atual.
- **Obras / Mercado / Relações:** pergaminho na tela do painel; Obras = grade de botoes icone 34x34 (23 glifos, paginada); Mercado = lista dos 8 bens com preco int/ext e botoes Comprar/Vender; Relações = estandarte de cada rival + estado (Guerra/Paz/União) + botoes Paz/União.

### 14.6 Assets (pipeline, inventario, orcamento)

**Pipeline (ADR-14):**
1. Fonte: `gfx/<nome>.png`, pixel nativo 1:1, indexado (<=256 cores, ideal <=64), **indice 0 = transparente**.
2. `tools/png2c.py <png>` (Python 3 + Pillow) gera `source/gen/art_<nome>.c/.h`: struct `Art {w,h,pal[],data[]}`, paleta ja no formato `aBBBBBGGGGGRRRRR` com `BIT(15)` (exceto indice 0), pixels 8bpp com RLE simples. **Os gerados sao commitados** — o build nao roda Python. Makefile: `SOURCES := source source/gen`.
3. `source/px_art.c`: `Art_Blit(u16 *dst,const Art *a,int x,int y)` decodifica o RLE direto no backbuffer (sem buffer intermediario, zero malloc) e pula o indice 0; `Art_BlitH` espelha na horizontal (cantos, ornamentos).
4. **Placeholder:** asset ainda sem PNG = `Art` com `data==NULL` -> `Art_Blit` desenha retangulo `C_NAVY_HI` com borda `C_GOLD` e o nome em 3x5. Codigo de tela nunca espera a arte ficar pronta.

**Regras p/ quem desenha:** pixel art nativo, sem anti-aliasing contra o transparente, outline escuro de 1px, luz vindo de cima-esquerda, cores ja quantizadas p/ 15 bits (multiplos de 8 em RGB888) antes de exportar, tons de UI tirados da tabela 14.1.

**Inventario** (P1 = necessario p/ F2/F3 sem placeholder; P3 = pode ficar com fallback):

| Asset | Tamanho | Onde | Prio |
|---|---|---|---|
| `art_title` (Westminster ao por do sol + logo) | 256x192 | Menu, cima | P1 |
| `art_officer` | 88x160 | Menu, baixo; Vitória/Derrota | P1 |
| emblemas de bandeira (aguia, aguia dupla, coroa, crescente+estrela, sol, leao, estrela, cruz, dragao...) — bandeiras sao procedurais (ADR-19) | 8x8 e 20x20 | Nação, Relações, mapa, Carregar | P1 |
| `ico_*` recursos (moeda, saco, pedras, coroa) | 12x12 | barra do HUD | P1 |
| `ico_*` stats (espadas cruzadas, espada, escudo) | 10x10 | ficha | P1 |
| `ico_*` modo (coroa, espadas, pessoas) | 18x18 | Modo | P1 |
| `ico_*` acoes (espada, escudo, brasao, ferramentas) + pena | 20x20 / 10x10 | HUD, "Aperte A" | P1 |
| `orn_corner`, `orn_crown`, `orn_div` | 12x12, 16x12, 48x5 | todos os paineis | P1 |
| `unit_infantaria` | 56x72 | ficha | P2 |
| `art_mode_<modo>` x3 (minimo 1 compartilhado) | 80x96 | Modo, baixo | P2 |
| `art_engrave_port` | 216x84 | Pausa, baixo | P2 |
| `art_crest` | 96x72 | Configurações, baixo | P2 |
| `art_game_bg` (faixa direita) | 74x192 | Jogo, baixo | P2 |
| `orn_engrave_l/r` | 24x120 | Modo, cima | P2 |
| sprites do mapa (castelo por nacao, cidade, navio 2 frames, pinheiro) | 16x16 | mapa | P2 |
| `art_region_<r>` x~12 cenas por regiao cultural (`DESIGN.md` sec 6.7) | 240x176 | Nação, baixo; Carregar | P2 |
| `art_nation_<tag>` cenas proprias dos paises de destaque (GBR = a da concept; FRA, RUS, QNG, USA, JAP...) | 240x176 | Nação, baixo | P3 |
| `art_logo`, `art_ship_line` | 160x80, 120x64 | Créditos | P3 |

**Orcamento:** ~420KB brutos em 8bpp -> ~220KB com RLE. Teto: **512KB de `.rodata` de arte** (o binario ARM9 vive na RAM principal de 4MB; o `.bss` hoje passa de ~600KB com o buffer-mundo da v0.8). Conferir no `.map` a cada asset novo. VRAM nao muda: as duas telas seguem `Bmp16`.

### 14.7 Audio (Música / Efeitos)

- MaxMod (`libmm9`, vem com devkitPro; no toolchain rootless precisa ser copiado como foi feito com libnds, ADR-12). Soundbank embutido via `mmInitDefaultMem`.
- Volume: `mmSetModuleVolume(music*1024/100)`, `mmSetJingleVolume`, efeitos com volume escalado por `sfx`.
- Faixas: tema do menu, 2 temas de jogo, jingle de vitoria e de derrota. Efeitos: mover selecao, confirmar, voltar, erro, marcha, obra concluida, evento.
- Ate F6 os sliders funcionam e gravam o valor, mas nao ha som — nada quebra.

### 14.8 Settings (fora do GameState)

```c
typedef struct { u8 music, sfx, lang, layout; } Settings; // 4B: 0..100, 0..100, LANG_PT/LANG_EN, LAYOUT_NORMAL/LAYOUT_SWAP
```

Estatico em `source/settings.c`, fora do `GameState` (nao conta no `_Static_assert` de 14KB), aplicado no boot. Persistencia em SRAM com magic proprio na F8; ate la vive so na RAM.

### 14.9 Fases de entrega (cada fase fecha com build verde + playtest)

| Fase | Entrega | Pronto quando |
|---|---|---|
| F1 Fundacao | `pal.h`, componentes de `px_ui.c` com placeholders, Serifa 8/12 com acento, `str.c` PT/EN, `g->screen` + `Screen`, `Settings`, `px_art.c` com placeholder | Navega MAIN -> MODE -> SETUP -> NATION -> GAME -> PAUSE -> OPTIONS -> volta, so com D-pad/A/B; texto acentuado legivel; zero warnings |
| F2 Menus | layouts finais das secs 14.4/14.5 (com placeholders de arte) | Screenshot melonDS de cada tela sobreposto ao painel da concept: mesma geometria (+-4px) e mesmos textos |
| F3 HUD | barra de recursos, mapa na tela de cima, minimapa, ficha + 4 acoes, paginas L/R, Tela Normal/Invertida | Playtest de 5 min (sec 10) sem usar nenhum combo de botoes |
| F4 Arte | PNGs P1/P2, `png2c.py`, troca dos placeholders, direcao de arte do mapa | Checklist 14.10 inteiro marcado |
| F5 Paises (ponte) | ficha de pais + traits + bandeira procedural (`DESIGN.md` sec 6) aplicados as 3 faccoes do mapa atual, sorteadas do catalogo; `SAVE_VER++`. O mundo completo vem depois, nos marcos M1-M7 do `DESIGN.md` sec 9 | Sim host 200 dias com 10 combinacoes de paises sem crash + replay deterministico |
| F6 Audio | MaxMod + sliders ligados | Volume 0 = silencio; sem estalo na troca de tela |
| F7 Multijogador | 2 DS sem fio local, lockstep deterministico (RNG unico ja ajuda) | 2 instancias melonDS sincronizadas por 30 min |
| F8 SRAM | save + Settings persistentes, 3 slots | Desliga, liga e continua; "Carregar" lista os slots |

### 14.10 Checklist de fidelidade (por tela, antes de fechar F4)

- [ ] Mesma divisao cima/baixo da concept (no layout Normal)
- [ ] Textos identicos aos da concept, com acento, em PT
- [ ] Selecao em crimson com `▶`; nao selecionados como na concept
- [ ] Pergaminho com cantos ornamentados onde a concept tem; coroa/divisor nos mesmos lugares
- [ ] Nenhum `RGB15()` cru; toda cor via `Px_Col`/`PXC` (ADR-13)
- [ ] Menu parado nao redesenha a 60Hz (so `dirty` + pulso da selecao)
- [ ] Screenshot do melonDS salvo em `docs/screens/<estado>.png`
