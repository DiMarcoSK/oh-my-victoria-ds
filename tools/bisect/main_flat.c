#include <nds.h>
#include "game.h"
#include "px.h"
#include "pal.h"
#include "settings.h"
// main FLAT (bisect): roda init + In_Handle + Game_Frame + Px_Tick IGUAIS ao
// jogo, mas troca TODO o desenho do jogo (px_hud/px_ui/px_map) por um fill
// opaco em cada tela. Se isto aparecer dourado/carmesim, o caminho de video,
// buffers, init e logica estao bons -> o bug esta no desenho (v0.8).
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
  Px_Tick(g);
  if(g->dirty||(frame%15)==0){
   u16*t=Px_BackTop(); Px_Clear(t,PXC(25,20,9)); Px_FlipTop();   // dourado
   u16*b=Px_BackBot(); Px_Clear(b,PXC(19,3,3)); Px_FlipBot();   // carmesim
   g->dirty=false;g->dirtyBot=false;
  }
  frame++;
  swiWaitForVBlank();
 }
 return 0;
}
