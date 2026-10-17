#pragma once
#include "types.h"
// pops.h — ONDE: source/pops.c. O QUE: 4 classes/tile (nibbles 0..15) com
// demandas/dia distintas; felicidade=mix atendimento; cresce/promove/migra;
// militancia sobe na fome (risco revolta em events.c). QUEM CHAMA: Game_Day.
void Pop_Day(GameState *g);
int Pop_Need(u8 pop,u8 good);
int Pop_Count(Tile *t,u8 pop);
void Pop_Add(Tile *t,u8 pop,int dq);
