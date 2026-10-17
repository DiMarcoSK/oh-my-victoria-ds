#pragma once
#include "types.h"
// gov.h — ONDE: source/gov.c. O QUE: 3 governos com buffs/debuffs.
// Troca: SELECT+L (custa 50 ouro, cooldown 15 dias). EFEITOS (ver SDD sec 6):
// MONARQUIA: imposto +50%, +1 prestigio/5d, +1 ataque; univ alfabetiza 1/2d, imposto max -12 feliz.
// DEMOCRACIA: +6 feliz, MERCADO +3, UNIV +2 literacia/d; imposto -25%, -1 ataque.
// TEOCRACIA: TEMPLO +2 feliz/-2 militancia + dizimo +1 ouro, +1 moral/3d c/ templo;
//   BANCO sem juros (+1 fixo), UNIV sem prestigio; +1 defesa.
// QUEM CHAMA: input (SELECT+L), AI (foge p/ teocracia se mil alta), eco/pop/combat leem gov.
#define GOV_COST 50
#define GOV_COOLDOWN 15
void Gov_Init(GameState *g);
bool Gov_Switch(GameState *g,u8 f,u8 gov);
bool Gov_Cycle(GameState *g,u8 f);
const char *Gov_Name(u8 gov);
const char *Gov_Short(u8 gov);
