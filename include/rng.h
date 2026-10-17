#pragma once
#include "types.h"
// RNG unico (xorshift32 em g->rng): diversao deterministica por seed.
void Rng_Init(GameState *g,u32 seed);
u32 Rng_Next(GameState *g);
int Rng_Range(GameState *g,int n); // 0..n-1
int Rng_Chance(GameState *g,int pct); // 1 se <pct%
