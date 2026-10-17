#pragma once
#include "world.h"
// city.h — cidades do mundo (DESIGN.md sec 4.7: Victoria 3 + Imperator Rome).
// Cada pais tem cidades (historicas no mapa-mundi de 1800, geradas no
// aleatorio). Obras em slots fazem a cidade crescer e a sociedade mudar:
// povoado -> vilarejo -> cidade -> metropole. O jogador pode fundar cidades
// por um preco claro (City_FoundCost). Tudo inteiro, sem RNG do jogo
// (hash deterministico): o replay da historia nao muda por causa das cidades.
typedef struct {
 u8 nameStr;      // StrId do nome
 u16 cost, days;  // ouro e dias de obra
 u8 minTier;      // nivel minimo da cidade
 u16 minYear;     // ano minimo (industria/ferrovia)
 u8 gold, food;   // ouro/dia e comida (crescimento)
 u8 cap;          // teto de tropas
 u8 prestige;     // prestigio por ano
 u8 def;          // defesa % da celula da cidade
 u8 need;         // obra exigida (BK_*), 0 = nenhuma
 u8 flags;        // BF_COAST (so' cidade costeira), BF_CAPITAL (so' capital)
 u8 descStr;      // StrId do efeito (1 linha)
} BldDef;
enum { BF_COAST=1, BF_CAPITAL=2 };
extern const BldDef BLD[BK_COUNT];

void City_Gen(World *w);                  // fim da geracao (historicas + vilas)
void City_Day(World *w);                  // crescimento, obras, renda, IA
void City_OnOwner(World *w,u32 cell,u8 to); // celula com cidade mudou de dono
int City_At(const World *w,int x,int y);  // indice da cidade na celula, -1
int City_Near(const World *w,int x,int y,int r); // cidade a ate r celulas, -1
int City_Slots(u8 tier);                  // slots liberados pelo nivel
s32 City_FoundCost(const World *w,u8 ci); // preco de fundar (sobe por cidade)
bool City_CanFound(const World *w,u8 ci,int x,int y);
int City_Found(World *w,u8 ci,int x,int y); // indice da nova cidade, -1
// Pode construir a obra k no slot s? (ouro nao entra: e' checado no Build)
bool City_CanBuild(const World *w,int city,u8 k);
bool City_Build(World *w,int city,int slot,u8 k); // paga e enfileira
bool City_IsCapital(const World *w,int city);
int City_PopCap(const WCity *c);
int City_NextPop(u8 tier);                // populacao p/ o proximo nivel (0 = topo)
void City_Name(const World *w,int city,char *out,int max);
u8 City_Style(const World *w,int city);   // estilo arquitetonico (px_city.c)
enum { ST_EURO=0, ST_ASIA, ST_ISLAM, ST_AFRICA, ST_NATIVE, ST_COUNT };
