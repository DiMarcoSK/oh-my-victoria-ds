#pragma once
#include "types.h"
// economy.h — ONDE: source/economy.c. O QUE: 23 obras produzem/consomem bens por
// dia + salarios/imposto/prestigio/alfabetizacao. QUEM CHAMA: Game_Day p/ cada
// faccao viva; input Y constroi, X recruta/upa forte; AI constroi/recruta.
// CUSTOS: ver tabela no SDD sec 2 (fazenda 30 ... alfandega 90).
void Eco_Day(GameState *g,u8 f);
int Eco_Cost(u8 b);
bool Eco_Build(GameState *g,int x,int y,u8 b);
bool Eco_FortUp(GameState *g,int x,int y);
bool Eco_Recruit(GameState *g,int x,int y);
void Eco_Tax(GameState *g,u8 f,u8 r);
const char *Eco_BldName(u8 b); // nome curto p/ pixel (6 chars)
const char *Eco_BldDesc(u8 b); // efeito p/ SDD/painel
const char *Eco_DiffName(u8 d);
int Eco_BotQ7(u8 d);
