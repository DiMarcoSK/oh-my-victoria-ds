#pragma once
#include "world.h"
// city.h — cidades do mundo (DESIGN.md sec 4.7: Victoria 3 + Imperator Rome).
// Cada pais tem cidades (historicas no mapa-mundi de 1800, geradas no
// aleatorio). Obras em slots fazem a cidade crescer e a sociedade mudar:
// povoado -> vilarejo -> cidade -> metropole. O jogador pode fundar cidades
// por um preco claro (City_FoundCost). Tudo inteiro, sem RNG do jogo
// (hash deterministico): o replay da historia nao muda por causa das cidades.
typedef struct {
 u16 nameStr;     // StrId do nome
 u16 cost, days;  // ouro e dias de obra
 u8 minTier;      // nivel minimo da cidade
 u16 minYear;     // ano minimo (industria/ferrovia)
 u8 gold, food;   // ouro/dia e comida (crescimento)
 u8 cap;          // teto de tropas
 u8 prestige;     // prestigio por ano
 u8 def;          // defesa % da celula da cidade
 u8 need;         // obra exigida (BK_*), 0 = nenhuma
 u8 flags;        // BF_COAST (so' cidade costeira), BF_CAPITAL (so' capital)
 u16 descStr;     // StrId do efeito (1 linha)
} BldDef;
enum { BF_COAST=1, BF_CAPITAL=2, BF_WOOD=4, BF_ORE=8, BF_OIL=16, BF_MULTI=32 }; // MULTI: ate' 3 por cidade (especializacao, ADR-37)
extern const BldDef BLD[BK_COUNT];

void City_Gen(World *w);                  // fim da geracao (historicas + vilas)
void City_Day(World *w);                  // crescimento, obras, renda, IA
void City_OnOwner(World *w,u32 cell,u8 to); // celula com cidade mudou de dono
int City_At(const World *w,int x,int y);  // indice da cidade na celula, -1
int City_Near(const World *w,int x,int y,int r); // cidade a ate r celulas, -1
int City_Slots(u8 tier);                  // slots liberados pelo nivel
int City_MaxOf(u8 k);                     // copias permitidas por cidade (1 ou 3)
int City_Count(const WCity *c,u8 k);      // copias erguidas
void City_Invest(World *w,u8 ci);         // investimento privado: o fundo vira obra (ADR-37)
s32 City_FoundCost(const World *w,u8 ci); // preco de fundar (sobe por cidade)
bool City_CanFound(const World *w,u8 ci,int x,int y);
int City_Found(World *w,u8 ci,int x,int y); // indice da nova cidade, -1
// Pode construir a obra k no slot s? (ouro nao entra: e' checado no Build)
bool City_CanBuild(const World *w,int city,u8 k);
bool City_Build(World *w,int city,int slot,u8 k); // paga e enfileira
s32 City_Cost(const World *w,int city,u8 k);      // preco da obra (sobe com madeira/ferramentas caras)
bool City_IsCapital(const World *w,int city);
int City_PopCap(const WCity *c);
int City_NextPop(u8 tier);                // populacao p/ o proximo nivel (0 = topo)
void City_Name(const World *w,int city,char *out,int max);
u8 City_Style(const World *w,int city);   // estilo arquitetonico (px_city.c)
// ---- mapa da cidade (ADR-35, DESIGN 4.7): grade de lotes CG_W x CG_H ----
// O traçado (agua do lado do mar, rio, avenidas, ruas e praca) sai de um hash
// da posicao da cidade: fixo, sem estado. Cada obra ocupa um lote (bpos); o
// jogador escolhe onde, a IA usa City_AutoLot. Casas/bairros sao do render.
enum { CG_W=16, CG_H=10, CG_N=CG_W*CG_H };
enum { LK_TERRA=0, LK_AGUA, LK_RUA, LK_PRACA };
u8 City_LotBase(const World *w,int city,int lot);   // LK_* do tracado
bool City_LotCoast(const World *w,int city,int lot);// terra encostada na agua
int City_LotSlot(const WCity *c,int lot);           // slot da obra no lote, -1 = nenhuma
int City_Plaza(const World *w,int city);            // lote da praca central
bool City_CanBuildAt(const World *w,int city,int lot,u8 k);
bool City_BuildAt(World *w,int city,int lot,u8 k);  // jogador escolhe o lote
int City_AutoLot(const World *w,int city,u8 k);     // melhor lote livre (IA/inicio), -1
int City_IndustryBonus(const WCity *c,int slot);    // % extra por vizinhanca (ferrovia/porto)
enum { ST_EURO=0, ST_ASIA, ST_ISLAM, ST_AFRICA, ST_NATIVE, ST_COUNT };
