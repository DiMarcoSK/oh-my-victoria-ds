#pragma once
#include "types.h"
bool Combat_Order(GameState *g,int sx,int sy,int dx,int dy);
void Combat_Day(GameState *g);
int Combat_Pow(GameState *g,Tile *t,bool atk,u8 fac);
