#pragma once
#include "world.h"
// wstat.h — series historicas da aba Graficos (Gabinete) e marcos que dao
// recompensa (SDD ADR-41). Guarda o jogador e o lider do ranking em ate'
// ST_N pontos; ao encher, tira a media dos pares e dobra o passo: o grafico
// sempre mostra a partida inteira. Dinheiro sai em ouro de 1800 (real).
void Stat_Day(World *w);                 // amostra + marcos (fim da fase 2 do dia)
s32 Stat_Val(const World *w,u8 ci,u8 sr);// valor atual de uma serie p/ o pais
u16 Stat_Str(u8 sr);                     // StrId do nome da serie
u32 Stat_DayOf(const World *w,int i);    // dia (World.day) do ponto i
