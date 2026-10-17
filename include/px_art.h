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


// Pintura em cor direta (tools/genpics.py, recortada da concept art e
// reamostrada p/ o pixel nativo do DS): u16 ja' com BIT(15); 0 = transparente.
// Sem paleta e sem RLE: o blit e' uma copia por linha.
typedef struct {
 u16 w,h;
 const u16 *px;    // w*h pixels (0 = nao desenha)
} Pic;
void Pic_Blit(u16 *dst,const Pic *p,int x,int y);
// So' a janela (sx,sy,sw,sh) da pintura em (x,y) (fundo de pergaminho em
// sub-retangulos: a textura fica presa a' tela, nao ao painel).
void Pic_BlitClip(u16 *dst,const Pic *p,int x,int y,int sx,int sy,int sw,int sh);
extern const Pic PIC_TITLE, PIC_OFFICER, PIC_TOWN, PIC_SKETCH, PIC_CREST, PIC_HARBOR, PIC_PAPER;
extern const Pic PIC_PC_TL, PIC_PC_TR, PIC_PC_BL, PIC_PC_BR;   // cantos do pergaminho
extern const Pic PIC_NC_TL, PIC_NC_TR, PIC_NC_BL, PIC_NC_BR;   // cantos da moldura navy
extern const Pic PIC_CROWN;
// Bandeira pintada no PIC_HARBOR: caixa (x,y,w,h) e sombra por pixel (0 =
// nao e' pano, 12 = luz normal, ate' 15 = brilho). O jogo repinta o pano com
// a bandeira da nacao escolhida mantendo as dobras da pintura.
extern const u8 HARBOR_FLAG[], HARBOR_FLAG_BOX[4], HARBOR_FLAG_TOP[];  // TOP: 1a linha do pano por coluna
