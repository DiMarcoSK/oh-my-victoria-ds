#pragma once
#include <nds.h>
// px_art.h — pipeline de arte (SDD sec 14.6, ADR-14). Um asset e' um bitmap
// indexado 1:1 em pixel nativo, com paleta ja' no formato do hardware
// (aBBBBBGGGGGRRRRR com BIT(15)) e indice 0 = transparente. Os pixels vem
// comprimidos em RLE simples (pares count,indice) gerados por tools/png2c.py.
//
// REGRA: codigo de tela NUNCA espera a arte ficar pronta. Asset sem PNG ainda
// vem com data==NULL e o Art_Blit desenha um placeholder (retangulo C_NAVY_HI
// + borda C_GOLD + nome em 3x5). Assim a F2 fecha layout antes da F4 (arte).
typedef struct {
 u16 w,h;          // dimensoes em pixels nativos
 const char *name; // rotulo do placeholder (3x5, ex: "art_title")
 const u16 *pal;   // paleta; pal[0] nao e' desenhado (transparente)
 const u8 *data;   // RLE: pares (count,indice) somando w*h; NULL = placeholder
} Art;

// Desenha a arte em (x,y) do backbuffer, pulando o indice 0. Recorta nos
// limites do buffer (256x192); zero malloc, decodifica direto no destino.
void Art_Blit(u16 *dst,const Art *a,int x,int y);
// Igual, espelhado na horizontal (cantos e ornamentos reusam o mesmo asset).
void Art_BlitH(u16 *dst,const Art *a,int x,int y);
// Recorta a arte: desenha so' a sub-janela (sx,sy,sw,sh) da arte em (x,y).
// Usado p/ faixas de fundo (ex: art_game_bg so' aparece em x>=182).
void Art_BlitClip(u16 *dst,const Art *a,int x,int y,int sx,int sy,int sw,int sh);

// Arte gerada por tools/genart.py (source/gen/art_data.c): logo da concept e
// retrato do oficial do menu principal.
extern const Art ART_LOGO, ART_OFFICER;
