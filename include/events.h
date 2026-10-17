#pragma once
#include "types.h"
// Eventos RNG divertidos (1 a cada ~25 dias + clima seca/chuva).
void Evt_Day(GameState *g);
const char *Evt_Name(u8 e);
