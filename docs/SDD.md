# OH MY VICTORIA DS! — SDD v0.7.0 (sociedade + governos)

> RTS Vic-like sem turnos. Ate v0.8: pixel-art 100% procedural. **v0.9 (planejado): UI
> ilustrada conforme a concept art (`concept_arts/image.png`) — ver sec 14, que e o contrato visual.**
> Compila com `make` -> `ohmyvictoria_ds.nds` -> flashcard (R4/Acekard) ou melonDS / DS real.
> Nome antigo: FRONTIERS DS (v0.5). Nome atual: **OH MY VICTORIA DS!** (v0.6+: sociedade; v0.7+: governos).
>
> **Estado do codigo:** secs 1-10 descrevem a v0.7 (jogavel). O codigo ja tem a v0.8 em
> andamento (touch-first: `include/ui.h`, `source/ui.c`, `source/px_map.c`; `px_ui.c` citado
> em `ui.h` ainda nao existe). Sec 14 define o alvo v0.9 e manda sobre secs 8/9 em conflito.
>
> **Historico de bugs criticos:** tela preta/branca no boot (jogo rodava invisivel — pixel
> Bmp16 sem bit de alpha) RESOLVIDO em 2026-10-16. Post-mortem completo com checklist de
> descartes em `docs/POSTMORTEM-TELA-BRANCA.md` (ADR-13). Leia antes de mexer em video.

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
| `source/save.c` | Slot unico em RAM (`Save_W/R/Ok/Clr`) | `input.c` | `SAVE_VER=61` (Faction +2B gov); copia byte-a-byte; SRAM real fica p/ v0.8 (ver roadmap) |
| `source/rng.c` | RNG unico xorshift32 (`Rng_Init/Next/Range/Chance`) | Todos | Mesma seed = mesma historia; nunca use `rand()` da libc |
| `source/px_core.c` | Framebuffer duplo, `Px_Init/Rect/Border/Clear/Flip` | `main.c`, `px_tiles.c` | 2x `malloc(256*192*2)` 1x no boot; `dmaCopy` por frame (~100KB/tela) |
| `source/px_font.c` | Fonte 3x5 propria (<2KB ROM), `Px_Text/TextTop/Bar/Width` | `px_tiles.c` | Sem acento; `A-Z 0-9 :;.,-+*/!@()<>%` + espaco; minusculas viram maiusculas |
| `source/px_tiles.c` | Mapa 16x12 tiles 16px + painel + menu + overlay vitoria (`Px_Top/Bot`) | `main.c` | Menu com seed hexa + controles; `Px_Bot` com governo, barra de prestigio e obra do tile; overlay `VITORIA!/DERROTA`; fase `s_tick` anima agua/cursor/evento |
| `source/main.c` | Boot + loop com render sob demanda | — | `Game_Init` 1x; `seed=DEFAULT_SEED`; redesenha so se `dirty` ou 1x/15 frames (ver sec 9) |
| `source/gov.c` + `include/gov.h` | 3 governos (`Gov_Init/Switch/Cycle/Name/Short`, custo 50, cd 15d) | `input.c` (SELECT+L), `ai.c`, `game.c` | Efeitos lidos em `eco/pop/combat`; ver sec 6 |
| `Makefile` | `make` -> `ohmyvictoria_ds.nds` (padrao r68: calico link + `-lnds9`) | dev | `TARGET=ohmyvictoria_ds`, `GAME_TITLE/SUBTITLE1/2`; `LOCAL_TOOLCHAIN=1` p/ build rootless (ver sec 10); `make clean` limpa |
| `docs/diagramas/*.mmd` | Mermaid: arquitetura, classes, fluxo do dia | docs | Atualize junto quando mudar cadeia ou ordem do dia |

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
- Save: slot unico em RAM (`save.c`), `SAVE_MAGIC 0x46523530`, `SAVE_VER 61`; `SELECT+B` salva, `SELECT+R` carrega; game-over + START limpa e volta ao menu. SRAM real (persistente no cartao) fica p/ v0.8.
- RNG: xorshift32 em `g->rng`; mesma seed = mesma historia (replay/teste). `Rng_Range(g,n)` = 0..n-1, `Rng_Chance(g,pct)` = 1 se <pct%. Nunca use `rand()`.
- Verificado no ELF v0.7: ARM9 entry `0x2004800`/RAM `0x2004000` (layout calico), `.text` 36KB, `.nds` 135KB, `ndstool -i` com header valido. Teste host: 200 dias x3 faccoes sem crash + replay deterministico + trade + governo (ver `SIM_OK` nos testes em `/tmp/opencode/sim_test*.c`).
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
17. **Maquina de telas `g->screen` (u8) substitui `bool inMenu` (v0.9):** sao 10+ estados com "voltar" e contexto (Config abre do menu e da pausa); bool nao escala. Custo 0B liquido (troca bool por u8 + `scrPrev`).

## 12. GUIA PARA AGENTS (como mexer sem quebrar o jogo)

- **Adicionar obra (24a):** 1) `types.h:BuildingKind` antes de `BLD_COUNT`; 2) `economy.c:Eco_Cost/BldName/BldDesc/Eco_Tile`; 3) `px_tiles.c:TilePx` (cor); 4) `ai.c` area 0 (quando construir); 5) SDD sec 2 + `diagramas/arquitetura.mmd`. Nao reordene IDs existentes.
- **Adicionar bem (9o):** 1) `config.h:GOOD_*` + `GOOD_COUNT`; 2) `market.c:BASE/KOf/Name/Short`; 3) `pops.c:Pop_Need`; 4) `economy.c` (quem produz/consome); 5) `input.c` gira `good` (automatico por `GOOD_COUNT`); 6) SDD sec 2. Atenção: `Faction.stock/priceInt/supInt/demInt` e `priceExt/supExt/demExt` crescem (mede `GameState`!).
- **Mudar balance:** mexa em `BASE/k` (`market.c`), custos (`Eco_Cost`), `throughput` (`Eco_Tile`), `Pop_Need`, `AI_Budget/BotQ7`, `MarchDays/Combat_Pow`. Teste: mesma seed deve dar mesma historia (RNG so via `Rng_*`); rode `gcc -fsyntax-only -Iinclude source/*.c` com stub `nds.h` p/ logica (input/px exigem libnds real).
- **Armadilhas:** `s16` estoura em 9999 (use `AddStock`/capa); `u8` estoura em 255 (pops nibble max 15 via `popSet`); `Mkt_CloseDay` zera sup/dem (nao acumule entre dias); `Eco_Build` exige tile proprio vazio transitavel; `Dip_Ally` exige rel+ouro; `Evt_Day` exige `dirty=true`; `tileWalk` bloqueia AGUA/MONTANHA/VULCAO; fonte sem acento (mensagens sem acento).
- **Adicionar governo (4o):** 1) `types.h:GovKind` antes de `GOV_COUNT`; 2) `gov.c:Name/Short`; 3) efeitos em `economy.c`/`pops.c`/`combat.c` (guarde com `F->gov==...`, barato); 4) `input.c` (gira sozinho por `GOV_COUNT`); 5) SDD sec 6. Nao reordene IDs existentes.
- **Checklist antes de commitar:** 1) logica compila (`gcc -fsyntax-only`); 2) `make` (local: `source ~/devkitpro/env.sh`) com **zero warnings** nos nossos arquivos; 3) sim host 200 dias + replay deterministico; 4) 5-min playtest (sec 10); 5) SDD + diagramas atualizados; 6) `BLD_COUNT`/`GOOD_COUNT`/`GOV_COUNT` consistentes (giro, desenho, IA).

## 13. ROADMAP (o que falta — v0.8+)

- **v0.9 = UI da concept art** (menus, selecao de modo/nacao, HUD, pausa, configuracoes, fonte com acento, assets ilustrados, 5 nacoes, audio, multijogador, SRAM). Plano completo e fases F1-F8 na **sec 14.9**.
- Leis especificas por governo (editos: alistamento, feriado, embargo) via SELECT+CIMA/BAIXO; 4o governo (ex: republica mercantil); IA troca governo com estrategia (nao so panico).
- SRAM persistente (save no cartao, nao so RAM); fila multi-marcha; som (MaxMod); mais eventos por bioma; painel com 8 precos (rolagem SELECT); data ano/mes/dia visivel; throughput visivel no painel (`FALTA CARVAO`); balance por telemetria (facil 80%/insano 20%); i18n (acentos: resolvido na v0.9 pela Serifa 8/12, sec 14.3).
- Glossario rapido: int/ext, tarifa, uniao=ALLY, clima (S seca/C chuva), evento, marcha, tesouro, prestigio, literacia (boost industria+prestigio), militancia (risco revolta), throughput (produz menos sem insumo), seed (historia deterministica).
