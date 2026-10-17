#pragma once
#include "types.h"
// menu.h — telas da v0.9 (SDD sec 14.2/14.4/14.5): Principal, Modo, Preparar,
// Nacao, Pausa, Configuracoes, Creditos + a partida no mundo (SCR_WORLD).
// O jogo classico 32x24 (SCR_GAME) continua com o input/render da v0.8.
// Estado de menu fora do GameState (nao e' salvo; nao conta nos 14KB).
enum { MAPK_MUNDI=0, MAPK_ALEAT, MAPK_BLITZ, MAPK_CLASSICO, MAPK_COUNT };
typedef struct {
 u8 sel;          // item destacado na tela corrente
 u8 campaign;     // 1 = veio da Campanha (pula Preparar; mundi + historia ligada)
 u8 map, hist, size, nNat, diff; // escolhas do Preparar (nNat: 0/1/2 = 8/16/32)
 u32 seed;
 u8 cont;         // filtro do carrossel: 0 = todos, 1+Continent
 s16 nat;         // nacao destacada (indice em NATIONS)
 u8 confirm;      // Sair para o Menu pede o 2o A
 u16 tick;        // animacao (pulso da selecao, "Aperte A")
 u16 credY;       // rolagem dos creditos
} Menu;
Menu *Menu_Ptr(void);
// Input das telas v0.9 e do SCR_WORLD. Chamado pelo In_Handle quando a tela
// nao e' o jogo classico.
void Menu_Input(GameState *g,u32 down,u32 held,bool tDown,bool tUp,bool tHeld,int tx,int ty);
void Menu_Frame(GameState *g);         // pulso (animacao sob demanda)
void Menu_Go(GameState *g,u8 screen);  // troca de tela + layout do LCD
// Render (source/px_menu.c): tela de cima e de baixo por estado.
void Menu_DrawTop(GameState *g,u16 *d);
void Menu_DrawBot(GameState *g,u16 *d);
bool Menu_Owns(u8 screen);             // true p/ toda tela fora do classico
// Linhas do Preparar partida (dependem do mapa escolhido).
enum { ROW_MAP=0, ROW_HIST, ROW_SIZE, ROW_NNAT, ROW_DIFF, ROW_SEED, ROW_GO };
int Menu_SetupRows(u8 *rows);
void Menu_DrawLoading(GameState *g);   // "Preparando o mundo…" (antes de gerar)
// Lista de nacoes jogaveis com o filtro atual (carrossel).
int Menu_NatCount(void);
int Menu_NatAt(int i);
int Menu_NatIndex(int nat);            // posicao de nat na lista (-1 se fora)
