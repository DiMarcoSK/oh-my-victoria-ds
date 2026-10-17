#include <nds.h>
#include "game.h"
#include "px.h"
// OH MY VICTORIA DS! v0.7: pixel-art 100% procedural, sem console.
// make -> ohmyvictoria_ds.nds -> cartao/flashcard ou melonDS.
int main(void){
 Px_Init();
 GameState *g=Game_Ptr();
 Game_Init(g);
 g->seed=DEFAULT_SEED;g->dirty=true;
 u32 frame=0;
 while(1){
  In_Handle(g);
  Game_Frame(g);
  // OTIMIZACAO DS (a maior economia do jogo): so redesenha quando algo
  // mudou (dirty) ou 1x/15 frames p/ animacoes (cursor/agua/evento).
  // Antes: 2x dmaCopy 100KB + ~50k pixels TODO frame (60Hz).
  // Depois: ~4Hz + sob demanda -> ~93% menos DMA e pixels. VRAM retém.
  if(g->dirty||(frame%15)==0){Px_Top(g);Px_Bot(g);}
  frame++;
  swiWaitForVBlank();
 }
 return 0;
}
