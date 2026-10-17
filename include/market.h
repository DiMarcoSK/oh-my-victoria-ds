#pragma once
#include "types.h"
// market.h — ONDE: source/market.c. O QUE: 2 mercados (interno/faccao + externo
// unico), preco Q8 move por oferta-demanda, tarifa, uniao ALLY sincroniza.
// QUEM CHAMA: Eco_Day produz/demanda, Pop_Day demanda, input/AI compram/vendem,
// Game_Day fecha o dia. BEM = indice 0..7 (ver config.h).
void Mkt_Init(GameState *g);
int Mkt_Base(u8 good);
int Mkt_IntPrice(GameState *g,u8 f,u8 good);
int Mkt_ExtPrice(GameState *g,u8 good);
void Mkt_IntSup(GameState *g,u8 f,u8 good,int q);
void Mkt_IntDem(GameState *g,u8 f,u8 good,int q);
void Mkt_ExtSup(GameState *g,u8 good,int q);
void Mkt_ExtDem(GameState *g,u8 good,int q);
void Mkt_CloseDay(GameState *g);
bool Mkt_BuyInt(GameState *g,u8 me,u8 good,int qty);
bool Mkt_BuyExt(GameState *g,u8 me,u8 good,int qty);
bool Mkt_SellExt(GameState *g,u8 me,u8 good,int qty);
const char *Mkt_Name(u8 good);
const char *Mkt_Short(u8 good);
