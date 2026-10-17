#pragma once
#include "nations.h"
// flag.h — bandeiras procedurais (DESIGN.md sec 6.2.1, ADR-19).
// Desenha num buffer u16 qualquer (framebuffer ou buffer-mundo) com stride.
// Tamanhos de referencia: 16x11 (mapa/listas) e 40x58 (estandarte da selecao).
void Flag_Draw(u16 *buf,int stride,int x,int y,int w,int h,const FlagDef *f);
// Estandarte da tela "Escolha sua nação": haste dourada + ponta em V embaixo.
void Flag_Banner(u16 *buf,int stride,int x,int y,int w,int h,const FlagDef *f);
