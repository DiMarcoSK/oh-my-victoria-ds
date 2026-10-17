#include <nds.h>
#include "game.h"
#include "px.h"
#include "settings.h"
#include "prof.h"
#include "wgame.h"
// OH MY VICTORIA DS! — laco principal (SDD sec 1, ADR-36).
// Quadro: entrada -> simulacao -> desenho -> VBlank -> troca de pagina.
// Desenho so' sob demanda (dirty/dirtyBot/WantTop ou pulso 1x/15 quadros);
// as trocas de tela acontecem SEMPRE no VBlank (Px_VSync), nunca no meio do
// feixe. Px_Vb() diz quantos VBlanks o quadro levou: camera e relogio do jogo
// andam por ele, entao um quadro pesado nao deixa o mapa "em camera lenta".
// Telas cujo painel de baixo nao tem animacao no tempo: o pulso de 1x/15
// quadros so' redesenha a de cima (mapa: frentes, navios, mira). O painel
// delas muda por dirtyBot (dia, toque, aviso). Antes: painel inteiro + DMA
// de 96KB 4x por segundo parado (ADR-36).
static bool BotStill(u8 scr){return scr==SCR_WORLD||scr==SCR_CITY||scr==SCR_LEDGER||scr==SCR_RANK;}
int main(void){
 // DSi (ROM DSi-enhanced): ARM9 a 134 MHz, o dobro do DS. No DS nao faz nada.
 if(isDSiMode())setCpuClock(true);
 Settings_Init();
 Px_Init();
 Prof_Init();
 GameState *g=Game_Ptr();
 Game_Init(g);
 g->seed=DEFAULT_SEED;g->dirty=true;
 u32 frame=0;
 while(1){
  PROF(PF_IN,In_Handle(g));
  PROF(PF_SIM,Game_Frame(g));
  // bake do buffer-mundo do classico: espalhado (budget) por quadro
  Px_Tick(g);
  if(g->dirty){PROF(PF_TOP,Px_Top(g));PROF(PF_BOT,Px_Bot(g));g->dirty=false;g->dirtyBot=false;Px_TakeTop();}
  else if(g->dirtyBot){PROF(PF_BOT,Px_Bot(g));g->dirtyBot=false;if(Px_TakeTop())PROF(PF_TOP,Px_Top(g));}
  else if(Px_TakeTop())PROF(PF_TOP,Px_Top(g));
  else if((frame%15)==0){PROF(PF_TOP,Px_Top(g));if(!BotStill(g->screen))PROF(PF_BOT,Px_Bot(g));}
  WG_Idle(g);          // folga do quadro: mapa pintado a' frente da camera (ADR-36)
  frame++;
  Prof_Busy();
  swiWaitForVBlank();
  Px_VSync();
  Prof_EndFrame(Px_Vb());
 }
 return 0;
}
