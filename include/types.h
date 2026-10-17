#pragma once
#include <nds.h>
#include "config.h"
// OMV-DS v0.6 — GLOSSARIO RAPIDO P/ AGENTS:
// Tile = 1 territorio do mapa. Faction = pais (1 voce, 2/3 bots, 0 neutro).
// Bem = mercadoria (8). POP = classe social (4/tile em nibbles).
// Mercado interno = preco do pais; externo = preco mundial; uniao = ALLY.
// Tesouro = caixa do pais. Prestigio = pontos de vitoria. Marcha = ataque c/ delay.
// Obra = construcao (23). Clima/evento = RNG divertido. dirty = redesenhar tela.
// Tile 12B; Faction ~100B; GameState ~13KB (cabe no DS 4MB).
typedef enum {
 TER_PLAINS=0, TER_FOREST=1, TER_HILLS=2, TER_WATER=3, TER_MOUNTAIN=4,
 TER_DESERT=5, TER_SWAMP=6, TER_VOLCANO=7, TER_RUINS=8, TER_OASIS=9, TER_COUNT=10
} TerrainKind;
// 23 OBRAS (cadeia completa): rural(4) industria(6) urbana(5) estado(5) mar(3)
typedef enum {
 BLD_NONE=0,
 BLD_FARM=1, BLD_ORCHARD=2, BLD_MINE=3, BLD_COALPIT=4,
 BLD_SAWMILL=5, BLD_TEXTILE=6, BLD_STEEL=7, BLD_TOOLSHOP=8, BLD_WINERY=9,
 BLD_GLASSWORKS=10,
 BLD_MARKET=11, BLD_BANK=12, BLD_UNIV=13, BLD_THEATER=14, BLD_HOSPITAL=15,
 BLD_BARRACKS=16, BLD_WALL=17, BLD_FORT=18, BLD_PORT=19, BLD_SHRINE=20,
 BLD_DOCKYARD=21, BLD_CUSTOMS=22, BLD_COUNT=23
} BuildingKind;
typedef enum {
 SITE_NONE=0, SITE_GOLDMINE=1, SITE_FERTILE=2, SITE_IRON=3,
 SITE_RUINARM=4, SITE_CURSED=5, SITE_COAL=6, SITE_COUNT=7
} SiteKind;
typedef enum { DIP_WAR=1, DIP_PEACE=2, DIP_ALLY=3 } DipStance;
// Governo/leis (v0.7): cada faccao escolhe 1 forma de governo com buffs/debuffs.
// Troca via SELECT+L (custa 50 ouro, cooldown 15 dias). Ver gov.h/gov.c.
typedef enum { GOV_MONARCHY=0, GOV_DEMOCRACY=1, GOV_THEOCRACY=2, GOV_COUNT=3 } GovKind;
// Maquina de telas (SDD sec 14.2, ADR-17): g->screen substitui o antigo bool
// inMenu. Toda troca de tela seta dirty + dirtyBot (SDD sec 14.2).
typedef enum {
 SCR_MAIN=0, SCR_MODE, SCR_SETUP, SCR_NATION, SCR_GAME,
 SCR_PAUSE, SCR_OPTIONS, SCR_LOAD, SCR_CREDITS, SCR_END,
 SCR_WORLD,  // partida no mundo celular (mapa-mundi/aleatorio, wgame.c)
 SCR_CITY,   // cidade aberta (cena viva + slots de obras, px_city.c)
 SCR_LEDGER, // Gabinete: mercado, comercio/frota, diplomacia, politica (px_ledger.c)
 SCR_COUNT
} Screen;
typedef struct {
 u8 owner, troops, terrain, building, site, happiness;
 u8 popA, popB; // peas|work, burg|nobl (0..15 cada)
 u8 moveCd, atkCd;
 u8 dstX, dstY;
} Tile;
typedef struct {
 u8 id; bool isPlayer, alive;
 s16 treasury;
 u8 taxRate, morale, rel[MAX_FACTIONS], tariff[MAX_FACTIONS];
 s16 stock[GOOD_COUNT];
 s16 priceInt[GOOD_COUNT];
 s16 supInt[GOOD_COUNT], demInt[GOOD_COUNT];
 u16 tilesOwned, troopsTotal, prestige, popTotal;
 u8 literacy; // 0..100: sobe c/ univ, boosta industria+prestigio
 u8 militancy; // 0..100: sobe c/ fome/imposto, risco revolta
 u8 gov, govCd; // governo (GovKind) + cooldown de troca em dias
} Faction;
typedef struct {
 Tile tiles[MAP_H][MAP_W];
 Faction factions[MAX_FACTIONS];
 u8 dip[MAX_FACTIONS][MAX_FACTIONS];
 s16 priceExt[GOOD_COUNT];
 s16 supExt[GOOD_COUNT], demExt[GOOD_COUNT];
 u32 seed, day, rng; u16 frameAcc;
 u8 selX, selY, camX, camY, diff, speed, winner, weather, evtCd, lastEvt;
 u8 evtX, evtY;
 // v0.9 UI: tela corrente (Screen, ADR-17) + quem abriu Configuracoes; pagina
 // aberta (UiPage, v0.8 de caneta) + entrada (1=marcha esperando destino) +
 // flags de redraw seletivo (caneta pan so redesenha o mapa).
 u8 screen, scrPrev, uiPage, uiPick;
 bool gameOver, dirty, dirtyBot, paused;
 char status[64];
 char evtMsg[64];
} GameState;
static inline bool tileInside(int x,int y){return x>=0&&x<MAP_W&&y>=0&&y<MAP_H;}
static inline bool tileWalk(u8 t){return t!=TER_WATER&&t!=TER_MOUNTAIN&&t!=TER_VOLCANO;}
static inline u8 popGet(u8 p,bool hi){return hi?(p>>4):(p&15);}
static inline u8 popSet(u8 p,bool hi,u8 v){if(v>15)v=15;
 return hi?(u8)((p&15)|(v<<4)):(u8)((p&0xF0)|v);}

// Trava de RAM: DS tem 4MB; GameState deve ficar em ~13-14KB.
// Se estourar, o assert quebra o build na hora (nao no DS).
_Static_assert(sizeof(GameState) <= 14*1024, "GameState > 14KB!");
_Static_assert(sizeof(Tile) == 12, "Tile != 12B!");
