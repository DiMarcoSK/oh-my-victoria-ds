#pragma once
#include "types.h"
void Map_Init(GameState *g);
void Map_Gen(GameState *g,u32 seed);
Tile *Map_Get(GameState *g,int x,int y);
void Map_Recalc(GameState *g);
int Map_Def(u8 t);
