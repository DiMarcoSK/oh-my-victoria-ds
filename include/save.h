#pragma once
#include "types.h"
#define SAVE_MAGIC 0x46523530u
#define SAVE_VER 62 // v0.8: uiPage/uiPick/dirtyBot (caneta); RAM-only, sem migracao
bool Save_W(GameState *g);
bool Save_R(GameState *g);
bool Save_Ok(void);
void Save_Clr(void);
