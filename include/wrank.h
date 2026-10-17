#pragma once
#include "world.h"
// wrank.h — ranking das potencias (estilo Victoria 3, DESIGN 4.9). Pontuacao
// geral = prestigio (pesa mais) + economia (PIB + populacao letrada) + forca
// (exercito, frota). Faixas pela posicao: 1-8 grandes potencias,
// 9-20 potencias maiores, 21-50 potencias menores, o resto insignificante.
// So' le o World (nenhum estado proprio); ordenar 250 paises custa ~30K
// comparacoes e so' roda quando a tela do ranking desenha.
enum { RK_GERAL=0, RK_ECON, RK_MIL, RK_POP, RK_COUNT };       // criterio (abas)
enum { RT_GRANDE=0, RT_MAIOR, RT_MENOR, RT_INSIG, RT_COUNT }; // faixas
s32 WRank_Score(const World *w,u8 ci,u8 mode);
int WRank_Sort(const World *w,u8 mode,u8 *out);   // paises vivos em ordem; devolve quantos
u8 WRank_Tier(int pos);                           // pos 0-based -> RT_*
int WRank_Pos(const World *w,u8 ci,u8 mode);      // posicao 0-based (-1 = morto)
