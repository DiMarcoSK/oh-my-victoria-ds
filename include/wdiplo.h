#pragma once
#include "world.h"
// wdiplo.h — o mundo que conversa (DESIGN.md sec 6.5 e 7.7; SDD ADR-27).
//  * IDEOLOGIA: cada pais tem uma (tradicional, liberal, conservador,
//    comunista, autoritario, teocratico). Revolucao troca a ideologia, o
//    governo, os modificadores e a BANDEIRA (padrao de base recolorido: a
//    "Franca comunista" e' a tricolor em vermelho com a estrela).
//  * DIPLOMACIA DA IA: rival, ate 2 aliados (chamada as armas), blocos
//    (mercado comum, wecon.c), temperamento sorteado por decada.
//  * PROPOSTAS ao jogador: alianca, bloco, pedido de ajuda, tributo, paz e
//    decisoes de evento — um modal com Aceitar/Recusar.
//  * MARINHA: batalhas navais mensais entre paises em guerra.
//  * AMBICOES do jogador: 3 objetivos ativos com recompensa.
// Cada pais pensa 1x por mes, escalonado ((ci+dia)%30): ~8 paises/dia.
void WDip_Init(World *w);
void WDip_Day(World *w);
void WDip_OnWar(World *w,u8 a,u8 b);
void WDip_OnDeath(World *w,u8 ci);              // limpa referencias a um pais morto          // guerra nova (nao historica): aliados
bool WDip_Allied(const World *w,u8 a,u8 b);
void WDip_Ally(World *w,u8 a,u8 b);
void WDip_Unally(World *w,u8 a,u8 b);
int WDip_Allies(const World *w,u8 a);
// Acoes do jogador (retorno = a IA aceitou)
bool WDip_AskAlliance(World *w,u8 me,u8 to);
bool WDip_AskBloc(World *w,u8 me,u8 leader);   // entrar no bloco de leader
bool WDip_InviteBloc(World *w,u8 me,u8 to);    // convidar ao seu bloco (cria se preciso)
void WDip_LeaveBloc(World *w,u8 ci);
bool WDip_FoundBloc(World *w,u8 ci);
int WDip_BlocSize(const World *w,u8 leader);
// Ideologia
void WDip_SetIdeo(World *w,u8 ci,u8 ideo,bool revolution);
u8 WDip_IdeoOfGov(u8 gov);
void WDip_IdeoMods(u8 ideo,TraitMods *m);           // somado aos traits (World_SetTraits)
void WDip_Flag(const World *w,u8 ci,FlagDef *out);   // bandeira atual (base + ideologia)
void WDip_FlagIdeo(const FlagDef *base,u8 from,u8 to,bool islam,FlagDef *out);
int WDip_RevoltRisk(const World *w,u8 ci);          // 0..100 (painel)
bool WDip_Separatism(World *w,u8 ci);               // tenta libertar uma nacao do territorio
u8 WDip_RandomIdeo(World *w,u8 ci);                 // sorteio por era + contagio
// Proposta pendente ao jogador
bool WDip_Propose(World *w,u8 kind,u8 from,u8 about,s32 gold,u16 idx);
void WDip_Answer(World *w,bool yes);
// Intriga (ADR-37): 1 = guerra acesa, 0 = falhou (vazou), -1 alvo invalido,
// -2 recarga, -3 sem ouro. victim = quem foi atacado.
s32 WDip_InstigateCost(const World *w,u8 t);
int WDip_Instigate(World *w,u8 me,u8 t,u8 *victim);
// Ambicoes
enum { OB_FUNDAR=0, OB_PORTO, OB_NAVIOS, OB_FABRICA, OB_METROPOLE, OB_ALIADO, OB_BLOCO,
 OB_VIDA, OB_COMERCIO, OB_CIDADES, OB_EXERCITO, OB_MINA, OB_FERROVIA, OB_PRESTIGIO,
 OB_ARMAS, OB_ACO, OB_COUNT };   // objMask e' u16: no maximo 16 ambicoes
u16 Obj_Str(u8 ob);                 // StrId do texto da ambicao
int Obj_Pct(const World *w,u8 ci,u8 ob);   // progresso 0..100
s32 Obj_Reward(u8 ob);             // ouro (positivo) ou prestigio (negativo)
