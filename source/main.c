#include <nds.h>
#include "game.h"
#include "px.h"
#include "settings.h"
// OH MY VICTORIA DS! v0.8: mapa cartografico cozido (buffer-mundo) na tela
// baixo tocavel + HUD na cima. make -> ohmyvictoria_ds.nds (melonDS/DS).
int main(void){
 Settings_Init();
 Px_Init();
 GameState *g=Game_Ptr();
 Game_Init(g);
 g->seed=DEFAULT_SEED;g->dirty=true;
 u32 frame=0;
 while(1){
  In_Handle(g);
  Game_Frame(g);
  // bake do buffer-mundo: espalhado (budget) — barato por frame; e o unico
  // custo de pixel novo, pan/grafismos sao so copia+overlay.
  Px_Tick(g);
  // RENDER SOB DEMANDA: dirty (tudo), dirtyBot (so mapa, pan da caneta) ou
  // pulso 1x/15 frames p/ animacoes (agua/cursor/menu). Nada a 60Hz parado.
  if(g->dirty){Px_Top(g);Px_Bot(g);g->dirty=false;g->dirtyBot=false;Px_TakeTop();}
  else if(g->dirtyBot){Px_Bot(g);g->dirtyBot=false;if(Px_TakeTop())Px_Top(g);}
  else if(Px_TakeTop())Px_Top(g);
  else if((frame%15)==0){Px_Top(g);Px_Bot(g);}
  frame++;
  swiWaitForVBlank();
 }
 return 0;
}
