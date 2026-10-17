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

// Bens do mercado mundial (DESIGN 4.8). IDs gravados: so' ADICIONE antes de GD_COUNT.
enum { GD_GRAOS=0, GD_MADEIRA, GD_FIBRA, GD_MINERIO, GD_LUXO, GD_TEXTEIS,
 GD_FERRAMENTAS, GD_ARMAS, GD_PETROLEO, GD_COUNT };
// Ideologias (DESIGN 6.5): mudam bandeira, governo e modificadores. IDs gravados.
enum { ID_TRADICIONAL=0, ID_LIBERAL, ID_CONSERVADOR, ID_COMUNISTA, ID_AUTORITARIO,
 ID_TEOCRATICO, ID_COUNT };
// Economia de um pais (wecon.c), fechada 1x por semana. Unidades = cargas/semana.
typedef struct {
 s32 sup[GD_COUNT], dem[GD_COUNT]; // oferta e demanda da semana
 s16 net[GD_COUNT];       // + importou / - exportou (mercado mundial ou bloco)
 u16 price[GD_COUNT];     // preco no mercado do pais (ou do bloco)
 u8 sat[GD_COUNT];        // % da demanda atendida
 u8 life;                 // nivel de vida 0..100 (media ponderada das necessidades)
 u8 ind, lack;            // industria rodando a ind% (insumo que falta: lack = bem)
 s16 hub;                 // cidade-porto principal (rotas no mapa), -1 = nenhuma
 u8 tax;                  // 0 baixo, 1 medio, 2 alto
 s32 inc, exp;            // receita e despesa da semana (ouro)
 s32 trade;               // saldo do comercio externo da semana (ouro)
 u16 cap, used;           // capacidade maritima de comercio (cargas/semana) e uso
 u8 ports, yards;         // cidades com porto / estaleiro
 u8 route[3], routeG[3];  // rotas: parceiro (pais) e bem
 u16 routeV[3];           // volume da rota
} WEco;
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
 // diplomacia, ideologia e marinha (wdiplo.c)
 u8 ideo, ideo0;          // ideologia atual / da bandeira de base (catalogo)
 u8 ally[2], rival, bloc; // aliados, rival, lider do bloco (mercado comum; 0 = nenhum)
 u8 mood;                 // temperamento 0..100 (sorteado a cada decada): imprevisibilidade
 s8 opin;                 // opiniao sobre o jogador -100..100
 u16 unrestT;             // dias de turbulencia pos-revolucao (producao cai)
 s16 ships;               // navios de guerra
 u16 shipQ;               // dias ate o proximo navio (0 = estaleiro parado)
 u16 terr[WT_COUNT];      // celulas por terreno (producao rural; mantido no SetOwner)
} WCountry;
// Proposta/decisao pendente para o jogador (modal no painel).
enum { PR_NONE=0, PR_ALIANCA, PR_BLOCO, PR_AJUDA, PR_TRIBUTO, PR_PAZ, PR_DECISAO, PR_COUNT };
typedef struct { u8 kind, from, about; u16 idx; s32 gold; u32 day; } WProp;

// Cidade (DESIGN 4.7: Victoria 3 + Imperator). Nivel sobe com populacao e
// obras: povoado -> vilarejo -> cidade -> metropole. Obras em slots.
enum { CT_POVOADO=0, CT_VILAREJO, CT_CIDADE, CT_METROPOLE, CT_COUNT };
// Obras da cidade: IDs gravados (save/IA). So' ADICIONE antes de BK_COUNT.
enum { BK_NONE=0, BK_FAZENDA, BK_MERCADO, BK_OFICINA, BK_QUARTEL, BK_PORTO,
 BK_TEMPLO, BK_MURALHA, BK_UNIVERSIDADE, BK_FABRICA, BK_FERROVIA, BK_PALACIO,
 BK_SERRARIA, BK_MINA, BK_ARSENAL, BK_ESTALEIRO, BK_POCO, BK_COUNT };
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
 u8 agri, wood, ore;     // potencial do entorno (raio 3): lavoura, mata, minerio
 u8 res;                 // RS_*: tropical, petroleo
 u8 food;                // % de comida atendida na ultima semana (local)
} WCity;
enum { RS_TROPICAL=1, RS_OLEO=2 };
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
 WEco eco[W_MAXC+1];       // economia por pais (wecon.c)
 u16 wprice[GD_COUNT];     // preco mundial
 s32 wvol[GD_COUNT];       // volume negociado no mundo na semana
 WProp prop;               // proposta/decisao esperando o jogador
 u8 obj[3]; u8 objDone;    // ambicoes do jogador (wdiplo.c) e quantas cumpriu
 u16 objMask;              // ambicoes ja cumpridas (bit por OB_*)
 u32 nRevol, nAlliances, nNaval; // contadores (testes/Gazeta)
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
void World_DeclareWarCall(World *w,u8 a,u8 b); // + chama aliados (guerras nao historicas)
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
void World_RecountTerr(World *w); // recontagem das celulas por terreno
u32 World_Hash(const World *w);
u32 World_Rng(World *w);
int World_RngRange(World *w,int n);
