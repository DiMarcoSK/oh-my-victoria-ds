#include "game.h"
#include "map.h"
#include "market.h"
#include "pops.h"
#include "economy.h"
#include "combat.h"
#include "diplomacy.h"
#include "events.h"
#include "gov.h"
#include "ai.h"
#include "save.h"
#include "menu.h"
#include "wgame.h"
#include <stdio.h>
static GameState G;
GameState *Game_Ptr(void){return &G;}
static const char *FN(u8 i){return i==1?"Voce":(i==2?"Norte":(i==3?"Sul":"Neutro"));}
void Game_Init(GameState *g){
 // limpa sem memset gigante? memset ok 1x no boot (fora do loop).
 for(unsigned i=0;i<sizeof(*g);i++)((u8*)g)[i]=0;
 for(int i=0;i<MAX_FACTIONS;i++){g->factions[i].id=(u8)i;g->factions[i].alive=(i<4);}
 g->factions[1].isPlayer=true;
 Map_Init(g);Dip_Init(g);Mkt_Init(g);Gov_Init(g);
 const char *m="L/R DIFICULDADE A COMECA";
 for(int i=0;m[i]&&i<63;i++)g->status[i]=m[i];
 g->status[63]=0;g->screen=SCR_MAIN;g->scrPrev=SCR_MAIN;
}
void Game_New(GameState *g,u8 diff,u32 seed){
 Map_Init(g);Dip_Init(g);Mkt_Init(g);Gov_Init(g);
 g->diff=diff;g->seed=seed;
 if(seed!=DEFAULT_SEED)Map_Gen(g,seed);
 Map_Recalc(g);g->screen=SCR_GAME;g->scrPrev=SCR_MAIN;g->paused=false;g->speed=2;g->dirty=true;g->dirtyBot=true;
 const char *m="BOA GUERRA START PAUSA";
 for(int i=0;m[i]&&i<63;i++)g->status[i]=m[i];
 g->status[63]=0;
}
void Game_CheckEnd(GameState *g){
 Map_Recalc(g);
 for(int i=1;i<MAX_FACTIONS;i++)
  if(g->factions[i].prestige>=WIN_PRESTIGE||g->factions[i].tilesOwned>=200){
   g->gameOver=true;g->winner=(u8)i;g->dirty=true;
   const char *m=i==1?"VITORIA START":"DERROTA START";
   int k=0;while(m[k]&&k<63){g->status[k]=m[k];k++;}g->status[k]=0;return;}
 if(g->factions[1].tilesOwned==0){g->gameOver=true;g->winner=2;g->dirty=true;
  const char *m="DERROTA START";int k=0;while(m[k]&&k<63){g->status[k]=m[k];k++;}g->status[k]=0;}
 (void)FN;
}
void Game_Day(GameState *g){
 g->day++;
 Evt_Day(g);
 Pop_Day(g);
 Eco_Day(g,1);Eco_Day(g,2);Eco_Day(g,3);
 Combat_Day(g);
 Dip_Day(g);
 AI_Day(g,2);AI_Day(g,3);
 Mkt_CloseDay(g);
 Map_Recalc(g);g->dirty=true;
 Game_CheckEnd(g);
}
void Game_Frame(GameState *g){
 if(g->screen==SCR_WORLD||g->screen==SCR_CITY){WG_Frame(g);return;}
 if(g->screen!=SCR_GAME){Menu_Frame(g);return;}
 if(g->gameOver||g->paused)return;
 int fpd=FRAMES_PER_DAY_2;
 if(g->speed==1)fpd=FRAMES_PER_DAY_1;
 else if(g->speed==3)fpd=FRAMES_PER_DAY_3;
 g->frameAcc++;
 if(g->frameAcc>=fpd){g->frameAcc=0;Game_Day(g);}
}
