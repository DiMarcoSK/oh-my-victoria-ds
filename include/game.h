#pragma once
#include "types.h"
void In_Handle(GameState *g);
void Game_Init(GameState *g);
void Game_New(GameState *g,u8 diff,u32 seed);
void Game_Frame(GameState *g);
void Game_Day(GameState *g);
void Game_CheckEnd(GameState *g);
GameState *Game_Ptr(void);
