#pragma once
#include "world.h"
// wfort.h — fortificacoes no mapa (DESIGN 4.5, SDD ADR-38). O jogador (botao
// Construir, wgame.c) ou a IA erguem na propria terra: forte (raio 3),
// trincheira (raio 2, 1860+, a mais forte e barata, mas so' de campanha) e
// bateria costeira (raio 4, so' na costa: desembarque no raio custa o triplo).
// A defesa vale para as celulas DO DONO DO FORTE no raio; quem toma a celula
// do forte fica com ele. W_FORTD (.bss) diz qual forte cobre cada celula:
// a frente le 1 byte por celula, sem varrer a lista.
typedef struct {
 u16 nameStr;   // StrId
 u8 radius;     // celulas (Chebyshev)
 u8 bonus;      // % a mais no custo de tomar a celula coberta
 u16 cost;      // ouro (sobe com quantas o pais ja tem)
 u8 days;       // dias de obra
 u16 minYear;
} FortDef;
extern const FortDef FORT[FT_COUNT];

void Fort_Reset(World *w);                          // geracao: nenhuma
s32 Fort_Cost(const World *w,u8 ci,u8 kind);
bool Fort_CanBuild(const World *w,u8 ci,int x,int y,u8 kind);
int Fort_Build(World *w,u8 ci,int x,int y,u8 kind); // paga e poe em obra; indice ou -1
int Fort_At(const World *w,int x,int y);            // forte na celula, -1
int Fort_Near(const World *w,int x,int y,int r);    // forte a ate' r celulas, -1
int Fort_DefPct(const World *w,u32 cell,u8 def);    // bonus de defesa (%) da celula p/ o dono def
bool Fort_Battery(const World *w,u32 cell,u8 def);  // celula coberta por bateria pronta de def
void Fort_OnOwner(World *w,u32 cell,u8 to);         // celula do forte tomada: muda de dono
void Fort_Day(World *w);                            // obras andam; IA fortifica a fronteira
int Fort_Count(const World *w,u8 ci);
