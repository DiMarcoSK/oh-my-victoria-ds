#pragma once
#include <nds.h>
// settings.h — opcoes do jogador (v0.9, SDD sec 14.8). Vivem FORA do GameState
// (nao contam na trava de 14KB de types.h) e sao aplicadas no boot. Persistencia
// em SRAM na F8; ate la vivem so na RAM (default de fabrica a cada boot).
enum { LANG_PT=0, LANG_EN=1, LANG_COUNT };
// Tela: Normal = mapa em cima (como a concept); Invertida = mapa embaixo e
// tocavel (heranca touch-first da v0.8). A troca por lcdSwap() entra na F3
// (ADR-16). Ate la LAYOUT_NORMAL e' o comportamento atual.
enum { LAYOUT_NORMAL=0, LAYOUT_SWAP=1, LAYOUT_COUNT };

typedef struct { u8 music, sfx, lang, layout; } Settings; // 4B: 0..100, 0..100, LANG_*, LAYOUT_*

Settings *Set_Ptr(void);   // instancia unica (source/settings.c)
void Settings_Init(void);  // aplica os padroes: Musica 80, Efeitos 70, PT, Normal
