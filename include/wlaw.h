#pragma once
#include "world.h"
// wlaw.h — leis (DESIGN 4.10, SDD ADR-40). 9 grandes assuntos x 5 leis, cada
// uma com bonus e penalidades (LFX_* em world.h). O efeito somado das leis em
// vigor fica em WEco.fx (Law_Apply) e e' lido por wecon.c (receita, despesa,
// industria, investimento, inflacao, crescimento, alfabetizacao), world.c
// (teto/recrutamento/ataque/colonizacao) e wdiplo.c (felicidade, militancia).
// Jogador: mudar uma lei abre um DEBATE (15 + 15 x distancia dias, 3 x
// distancia de prestigio), um por vez. IA: anda 1 passo por mes rumo ao alvo
// dela (arquetipo, ideologia, era, caixa). A ideologia de quem governa gosta
// ou nao das leis (Law_Approval: -6..+6 de felicidade).
enum { LW_DEBATE0=15, LW_DEBATE_STEP=15, LW_PREST_STEP=3 };

u16 Law_CatStr(u8 cat);                 // StrId do assunto
u16 Law_Str(u8 cat,u8 opt);             // StrId do nome da lei
u16 Law_Year(u8 cat,u8 opt);            // ano minimo
s8 Law_FxOf(u8 cat,u8 opt,u8 f);        // efeito de uma lei num campo
u16 Law_FxStr(u8 f);                    // StrId do rotulo do campo
bool Law_FxBad(u8 f,int v);             // o valor e' ruim p/ o pais? (vermelho na UI)
void Law_Apply(WEco *e);                // recalcula e->fx a partir de e->law
void Law_Init(World *w,u8 ci);          // leis de 1800 (alvo da IA no ano 1800)
u8 Law_Pref(u8 ideo,u8 cat);            // lei preferida da ideologia (0xFF = nenhuma)
int Law_Approval(const World *w,u8 ci); // -6..+6: a ideologia gosta das leis?
int Law_Days(const WEco *e,u8 cat,u8 opt);
int Law_Prestige(const WEco *e,u8 cat,u8 opt);
// 0 ok; senao StrId do motivo (ano, debate em curso, prestigio, ja' em vigor)
u16 Law_Why(const World *w,u8 ci,u8 cat,u8 opt);
bool Law_Propose(World *w,u8 ci,u8 cat,u8 opt); // jogador: abre debate; IA: muda ja'
void Law_Cancel(World *w,u8 ci);
void Law_Day(World *w);                 // debates do jogador andam
void Law_Ai(World *w,u8 ci);            // IA: 1 passo por mes rumo ao alvo
