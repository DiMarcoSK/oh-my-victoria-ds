#pragma once
#include "types.h"
#define SAVE_MAGIC 0x46523530u
#define SAVE_VER 63 // v0.9: g->screen/g->scrPrev (ADR-17) no lugar do bool inMenu
bool Save_W(GameState *g);
bool Save_R(GameState *g);
bool Save_Ok(void);
void Save_Clr(void);
