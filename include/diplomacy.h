#pragma once
#include "types.h"
void Dip_Init(GameState *g);
u8 Dip_Get(GameState *g,u8 a,u8 b);
void Dip_Set(GameState *g,u8 a,u8 b,u8 s);
bool Dip_CanAtk(GameState *g,u8 a,u8 d);
void Dip_Day(GameState *g);
bool Dip_Peace(GameState *g,u8 me,u8 to);
bool Dip_Ally(GameState *g,u8 me,u8 to);
const char *Dip_Name(u8 s);
