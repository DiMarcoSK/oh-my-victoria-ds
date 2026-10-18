#pragma once
#include "types.h"
#include "hot.h"
// px.h — engine de video v0.8: mapa pintado (cartografia, 16px/tile) num
// buffer-mundo 512x384 "cozido" por tile sob demanda; a tela baixo e um
// VIEWPORT com copia por row (pan suave por caneta) + overlays animados.
// Tela cima = HUD executivo (informacao, minimapa, evento) ou arte do menu.
// fonte 3x5 propria (+ escala 2x), primitivas pixel, zero malloc.
enum { PX_WORLD_W=MAP_W*16, PX_WORLD_H=MAP_H*16 };
void Px_Init(void);
u16 *Px_BackTop(void);
u16 *Px_BackBot(void);
u16 Px_Col(u8 r,u8 g,u8 b);
void Px_PSet(u16 *b,int x,int y,u16 c);
void Px_Rect(u16 *b,int x,int y,int w,int h,u16 c);
void Px_Border(u16 *b,int x,int y,int w,int h,u16 c);
void Px_Clear(u16 *b,u16 c);
// Flip = PEDIDO: a troca acontece em Px_VSync, logo apos o VBlank (ADR-36).
// Px_BackTop e' a pagina ESCONDIDA na VRAM (so' acessos de 16/32 bits:
// nada de memcpy/memset da newlib nela; use Px_Copy16).
void Px_FlipTop(void);
void Px_FlipBot(void);
void Px_VSync(void);    // main.c: logo apos swiWaitForVBlank
void Px_Present(void);  // espera o VBlank e troca ja' (tela antes de trabalho longo)
// transicao: as duas telas comecam escurecidas `lv` (1..16) e clareiam no
// VBlank (brilho mestre do hardware, custo zero de CPU). Px_VSync sempre
// devolve o brilho a 0 em poucos quadros: nunca fica preto (postmortem).
void Px_FadeIn(int lv);
int Px_Fade(void);      // brilho atual (0 = normal; negativo = escurecendo)
int Px_Vb(void);        // VBlanks do ultimo quadro (1 = 60 fps; teto 4): passo da camera/simulacao
u32 Px_Ticks(void);      // relogio de orcamento (523656/s no DS; 0 no host)
u32 Px_FrameTicks(void); // ticks desde o ultimo VBlank (quanto o quadro ja' gastou)
HOT_CALL void Px_Copy16(u16 *d,const u16 *s,int n);   // copia segura p/ VRAM (ARM, ITCM)
// Redesenho so' da tela de cima no proximo quadro (pan do mapa: o painel de
// baixo fica como esta'). main.c consome com Px_TakeTop.
void Px_WantTop(void);
bool Px_TakeTop(void);
// Telas: Px_Top = HUD (ou arte do menu); Px_Bot = mapa tocavel (ou menu).
// Px_Tick: continua o "bakery" do buffer-mundo (barato espalhado por frames).
void Px_Top(GameState *g);
void Px_Bot(GameState *g);
void Px_Tick(GameState *g);
// Fonte: Px_Text= painel baixo; Px_TextTop= tela cima; Px_TextSc= explcito
// em buffer, com escala (1=3x5, 2=6x10) para numeros grandes.
void Px_Text(int x,int y,const char *s,u16 col);
void Px_TextTop(int x,int y,const char *s,u16 col);
const u8 *Px_Glyph(char c); // acesso aos glifos da fonte 3x5 (px_ui desenha chips)
void Px_TextSc(u16 *dst,int x,int y,const char *s,u16 col,int sc);
// Barras: Px_Bar desenha no painel baixo; Px_BarT na tela cima.
void Px_Bar(int x,int y,int w,int pct,u16 col);
void Px_BarT(int x,int y,int w,int h,int pct,u16 col);
int Px_Width(const char *s);
u16 Px_MiniCol(GameState *g,u8 tx,u8 ty); // cor p/ minimapa (px_hud)
// util de formato compartilhado (HUD/UI)
void Px_Itoa(int v,char *out); // decimal, sem zeros a esquerda
