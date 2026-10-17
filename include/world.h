#pragma once
#include <nds.h>
#include "calendar.h"
#include "traits.h"
#include "nations.h"
// world.h — o mundo celular (DESIGN.md sec 4, marco M1; ADR-18).
// Territorio = celulas pintadas (dono u8); guerra = frente que avanca celula
// a celula gastando tropas (frontwars.io); exercito = reserva por pais que
// regenera ate um teto. Tudo inteiro, RNG proprio (w->rng), zero malloc.
// Grades ficam fora do struct (W_OWNER/W_TERR em .bss) p/ o World caber no
// save sem as grades e p/ o mapa-mundi (512x192) e o aleatorio usarem o mesmo
// espaco (stride = w->w).

#define W_MAXW 512
#define W_MAXH 192
#define W_CELLS (W_MAXW*W_MAXH)
#define W_HWMAX 32         // guerras historicas simultaneas (lista, nao matriz: 64B)
#define W_MAXC 250          // paises simultaneos (dono 1..250; 0 = sem dono)
#define W_MAXCITY 512       // cidades no mundo (24B cada: 12KB)
#define CITY_SLOTS 8        // slots de obra (liberados por nivel: 2/3/5/8)
#define W_BLK_W (W_MAXW/8)  // blocos 8x8 celulas p/ redesenho incremental do mapa
#define W_BLK_H (W_MAXH/8)

typedef enum {
 WT_WATER=0, WT_PLAINS, WT_FOREST, WT_HILLS, WT_MOUNTAIN, WT_DESERT,
 WT_SWAMP, WT_JUNGLE, WT_TUNDRA, WT_STEPPE, WT_ICE, WT_COUNT
} WTerrain;
typedef enum { WM_ALEATORIO=0, WM_MUNDI, WM_BLITZ } WMode;
typedef enum { WH_LIGADA=0, WH_FLAVOR, WH_DESLIGADA } WHistMode;
typedef enum { WS_PEQUENO=0, WS_MEDIO, WS_GRANDE } WSize;
// Arquetipo da IA, derivado dos traits (DESIGN.md sec 2.1 item 6).
typedef enum {
 WA_EXPANSIONISTA=0, WA_MERCADOR, WA_ISOLACIONISTA, WA_REVOLUCIONARIO,
 WA_DIPLOMATA, WA_OPORTUNISTA, WA_COUNT
} WArchetype;

typedef struct {
 u8 nat;                 // indice em NATIONS
 bool alive, player, coastal, capLost;
 u8 kind, gov, overlord, archetype; // overlord = indice do pais (0 = nenhum)
 u8 traits[NAT_TRAITS];
 TraitMods m;
 u16 capX, capY;
 u16 bx0, by0, bx1, by1; // caixa envolvente aproximada (so cresce)
 u32 cells, pop;          // pop = soma ponderada (terreno; nucleo vale 3x)
 s32 troops, cap, gold, prestige;
 u16 warDays;             // cansaco de guerra (sobe em guerra, cai em paz)
 u16 warGoal, warGain;    // objetivo de guerra da IA (celulas) e quanto ja tomou
 u8 warTarget;            // contra quem e o objetivo
 u16 peak;                // maior tamanho ja atingido (piso de perdas p/ IA no mapa-mundi)
 u8 happy, militancy, warN, rndCd;
 s32 cityGold, cityCap;   // somas das cidades (City_Day): ouro/dia e teto de tropas
 u16 nCity;               // cidades do pais
 bool atkOn; u8 atkTo; s32 atkTroops; // frente ativa (1 por pais)
} WCountry;

// Cidade (DESIGN 4.7: Victoria 3 + Imperator). Nivel sobe com populacao e
// obras: povoado -> vilarejo -> cidade -> metropole. Obras em slots.
enum { CT_POVOADO=0, CT_VILAREJO, CT_CIDADE, CT_METROPOLE, CT_COUNT };
// Obras da cidade: IDs gravados (save/IA). So' ADICIONE antes de BK_COUNT.
enum { BK_NONE=0, BK_FAZENDA, BK_MERCADO, BK_OFICINA, BK_QUARTEL, BK_PORTO,
 BK_TEMPLO, BK_MURALHA, BK_UNIVERSIDADE, BK_FABRICA, BK_FERROVIA, BK_PALACIO, BK_COUNT };
enum { CF_COASTAL=1, CF_HIST=2, CF_FOUNDED=4 };
typedef struct {
 u16 x, y;               // celula
 u8 owner, tier, flags;
 u8 bq;                  // obra em construcao (BK_*), 0 = nada
 u16 bqDays;             // dias restantes da obra
 u16 pop;                // milhares de habitantes
 u16 grow;               // acumulador de crescimento (0..999)
 u16 name;               // CF_HIST: indice da tabela; senao semente do nome
 u8 bld[CITY_SLOTS];     // obra por slot (BK_*)
 u8 bqSlot;              // slot reservado p/ a obra
} WCity;
typedef struct {
 u16 w, h; u8 mode, hist, diff; bool wrap;
 u32 seed, rng;
 u32 day, endDay; Date date; bool ended, eternal;
 u16 nC;                 // c[0] = sem dono; paises em 1..nC-1
 WCountry c[W_MAXC+1];
 u8 natToC[256];         // nacao -> pais (0 = nunca existiu)
 u8 war[((W_MAXC+1)*(W_MAXC+1)+7)/8];
 u8 hwN, hw[W_HWMAX][2];  // guerras historicas em curso: so o tratado (evento) encerra
 u8 hState[64];          // 2 bits por evento historico (ate 256)
 u32 flags;
 u32 nEvents, nWars, nConquests; // contadores (Gazeta/testes)
 char msg[128]; u32 msgDay;
 u16 nCity;                // cidades em city[0..nCity)
 WCity city[W_MAXCITY];
 u8 dirtyBlk[(W_BLK_W*W_BLK_H+7)/8]; // bloco 8x8 mudou de dono (px_world limpa)
} World;

extern u8 W_OWNER[W_CELLS];
extern u8 W_TERR[W_CELLS];
extern u8 W_CORE[W_CELLS]; // nucleo: nacao+1 dona "de direito" (0 = nenhuma)
// Depois da geracao: campo p/ o render (World_Field) — terra = relevo 0..255
// suavizado; agua = distancia da costa 1..31. A simulacao nao le.
extern u8 W_LIM[W_CELLS];
extern u8 W_CITYB[W_CELLS/8];  // bit: ha cidade nesta celula (busca rapida)

static inline u32 World_Idx(const World *w,int x,int y){return (u32)y*w->w+(u32)x;}

// Geracao. playerNat = indice em NATIONS (-1 = so IA, p/ testes/demo).
void World_GenRandom(World *w,u32 seed,u8 size,u8 nNations,u8 diff,int playerNat);
void World_GenBlitz(World *w,u32 seed,u8 diff,int playerNat);
void World_GenMundi(World *w,u32 seed,u8 hist,u8 diff,int playerNat);

void World_Day(World *w);
void World_Continue(World *w);              // depois de 31/12/2025: Modo Eterno

// Ordem do jogador (ou da IA): manda pct% da reserva contra `to` (0 = terra
// sem dono). Exige guerra se `to` for um pais. Repetir reforca a frente.
bool World_Attack(World *w,u8 from,u8 to,u8 pct);
void World_Recall(World *w,u8 ci);           // recolhe a frente aberta
s32 World_RecruitCost(const World *w,u8 ci);
bool World_Recruit(World *w,u8 ci);          // ouro -> regimentos
bool World_ProposePeace(World *w,u8 a,u8 b); // b aceita ou recusa
bool World_Borders(const World *w,u8 a,u8 b);// fronteira terrestre (b=0: terra livre)
void World_SetPlayer(World *w,u8 ci);
void World_Field(World *w);   // campo de relevo/profundidade (W_LIM) p/ o render
bool World_Landing(World *w,u8 a,u8 b,s32 *cost); // praia do alvo (sem fronteira terrestre)
void World_DeclareWar(World *w,u8 a,u8 b);
void World_DeclareHistWar(World *w,u8 a,u8 b); // guerra de evento: IA nao assina paz
void World_Peace(World *w,u8 a,u8 b);
bool World_AtWar(const World *w,u8 a,u8 b);

// Operacoes usadas pelos eventos (history.c).
u8 World_CountryOfNat(const World *w,int nat);   // 0 se nao existe/morto
u8 World_Spawn(World *w,int nat,int src,int cx,int cy,int radius); // src<0 = qualquer dono
int World_Transfer(World *w,u8 to,int src,int cx,int cy,int radius);
void World_Annex(World *w,u8 a,u8 b);
void World_Unify(World *w,u8 a,u8 b);       // anexa e torna as terras nucleo de a
void World_MoveAll(World *w,u8 from,u8 to); // todas as celulas; `from` segue vivo
void World_Kill(World *w,u8 ci);
void World_Become(World *w,u8 ci,int nat);
void World_SetTraits(World *w,u8 ci);       // recalcula TraitMods + arquetipo
void World_NatPos(const World *w,int nat,int *x,int *y); // capital projetada
void World_LatLonPos(const World *w,int lat,int lon,int *x,int *y);
void World_Msg(World *w,const char *a,const char *b);

s32 World_Score(const World *w,u8 ci);
u32 World_Hash(const World *w);
u32 World_Rng(World *w);
int World_RngRange(World *w,int n);
