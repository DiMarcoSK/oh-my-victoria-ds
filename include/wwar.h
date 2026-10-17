#pragma once
#include "world.h"
// wwar.h — crises diplomaticas e guerras com objetivo (DESIGN 4.5, SDD ADR-45).
// Estilo Victoria 3: ninguem declara guerra do nada. Quem quer algo abre uma
// CRISE com uma EXIGENCIA (territorio, tributo ou humilhacao) e um prazo
// (CR_DAYS dias de AQUECIMENTO). Durante a crise:
//  * cada lado chama aliados e terceiros escolhem lado (aliados, rivais do
//    outro lider, ideologia, bloco, suditos seguem o suserano);
//  * os lideres se MOBILIZAM (teto do exercito +25%, custo semanal no caixa);
//  * da' p/ propor ACORDO (metade da exigencia), CEDER (defensor paga tudo)
//    ou RECUAR (atacante perde prestigio).
// Prazo vencido sem acordo = GUERRA entre os dois lados, com PLACAR (-100..100,
// terra tomada x perdida + forca dos exercitos). A paz cobra o objetivo de
// quem venceu (o que falta da terra, o tributo, o prestigio) ou indeniza o
// defensor; o perdedor pode oferecer CONCESSAO (ouro) p/ parar antes.
// Guerras historicas (eventos) e revolucoes seguem fora daqui (imediatas).
// Usa o RNG do mundo: mesma semente = mesma historia.
void War_Init(World *w);
void War_Day(World *w);                         // fase 2 do dia (depois da diplomacia)
int War_Start(World *w,u8 att,u8 def,u8 goal);  // abre a crise; indice ou -1
int War_Find(const World *w,u8 a,u8 b);         // crise/guerra entre os lideres a e b (qualquer ordem), -1
int War_Of(const World *w,u8 ci);               // a mais urgente em que ci e' lider ou apoia, -1
int War_Side(const World *w,int k,u8 ci);       // 0 lado do atacante, 1 do defensor, -1 fora
bool War_Managed(const World *w,u8 a,u8 b);     // a guerra a x b pertence a uma guerra com objetivo
bool War_Mobilized(const World *w,u8 ci);       // ci esta' mobilizado (crise ou guerra com objetivo)
s32 War_Strength(const World *w,int k,int side);// tropas + frota do lado
int War_Score(const World *w,int k,u8 ci);      // placar do ponto de vista de ci (+ = ci vencendo)
s32 War_DemandGold(const World *w,int k);       // exigencia em ouro de hoje (tributo) p/ a UI
u16 War_GoalStr(u8 goal);                       // StrId do objetivo
// Acoes (jogador ou IA). Retorno: quantos entraram / se o outro aceitou.
int War_CallAllies(World *w,int k,u8 ci);
bool War_Deal(World *w,int k,u8 ci);            // acordo: metade da exigencia
void War_Yield(World *w,int k,u8 ci);           // defensor cede tudo / atacante recua
void War_Settle(World *w,int k,int pct);        // fecha a crise com pct% da exigencia paga (acordo aceito)
bool War_JoinSide(World *w,int k,u8 ci,int side); // ci entra num lado (jogador aceitou o pedido)
int War_PeaceTry(World *w,int k,u8 ci);         // 2 paz com termos a favor de ci, 1 paz branca, 0 recusada
s32 War_ConcedeCost(const World *w,int k,u8 ci);// ouro de hoje p/ comprar a paz
bool War_Concede(World *w,int k,u8 ci);         // paga e fecha a paz
void War_PeaceAnswer(World *w,u8 me,u8 from,s32 gold,bool yes); // resposta do jogador a PR_PAZ
void War_OnPeace(World *w,u8 a,u8 b);           // World_Peace: fecha a guerra com objetivo dos lideres
void War_OnDeath(World *w,u8 ci);
