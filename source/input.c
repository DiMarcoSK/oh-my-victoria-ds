#include "game.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "combat.h"
#include "diplomacy.h"
#include "gov.h"
#include "save.h"
#include <stdio.h>
// BOTOES (tudo sem menu): A marcha, X recruta/upa forte, Y obra (23, gira),
// L imposto, R paz/alianca (segura CIMA p/ alianca), B trade interno,
// B+BAIXO exporta, SELECT+B save, SELECT+R load, START pausa, SELECT vel.
static u8 cyc=BLD_FARM, good=GOOD_GRAIN;
static void Cam(GameState *g){
 if(g->selX<g->camX)g->camX=g->selX;
 if(g->selX>=g->camX+VIEW_W)g->camX=g->selX-VIEW_W+1;
 if(g->selY<g->camY)g->camY=g->selY;
 if(g->selY>=g->camY+VIEW_H)g->camY=g->selY-VIEW_H+1;
 g->dirty=true;
}
static void Msg(GameState *g,const char *m){
 int i=0;while(m[i]&&i<63){g->status[i]=m[i];i++;}g->status[i]=0;
}
void In_Handle(GameState *g){
 scanKeys();u32 d=keysDown(), h=keysHeld();
 if(g->inMenu){
  if(d&KEY_LEFT){g->diff=g->diff?g->diff-1:DIFF_COUNT-1;g->dirty=true;}
  if(d&KEY_RIGHT){g->diff=(g->diff+1)%DIFF_COUNT;g->dirty=true;}
  if(d&KEY_B){g->seed+=0x9E3779B9u;g->dirty=true;}
  if(d&KEY_A)Game_New(g,g->diff,g->seed?g->seed:DEFAULT_SEED);
  return;
 }
 if(g->gameOver){if(d&KEY_START){Save_Clr();Game_Init(g);}return;}
 if(d&KEY_START){g->paused=!g->paused;g->dirty=true;
  Msg(g,g->paused?"PAUSADO":"RODANDO");}
 if(d&KEY_SELECT){g->speed=g->speed>=3?1:g->speed+1;g->dirty=true;}
 // save/load sem menu: SELECT+B salva, SELECT+R carrega
 if((h&KEY_SELECT)&&(d&KEY_B)){Save_W(g);return;}
 if((h&KEY_SELECT)&&(d&KEY_R)){
  if(Save_R(g)){Cam(g);Msg(g,"LOAD OK");}else Msg(g,"SEM SAVE");return;}
 bool mv=false;
 if(d&KEY_UP&&g->selY>0){g->selY--;mv=true;}
 if(d&KEY_DOWN&&g->selY<MAP_H-1){g->selY++;mv=true;}
 if(d&KEY_LEFT&&g->selX>0){g->selX--;mv=true;}
 if(d&KEY_RIGHT&&g->selX<MAP_W-1){g->selX++;mv=true;}
 if(mv){Cam(g);return;}
 if(d&KEY_A){Tile*s=Map_Get(g,g->selX,g->selY);
  if(!s||s->owner!=1)Msg(g,"TILE SEU P/ MARCHA");
  else{bool a=false;for(int i=0;i<4&&!a;i++){
   int nx=g->selX+(i==0)-(i==1),ny=g->selY+(i==2)-(i==3);
   if(Combat_Order(g,g->selX,g->selY,nx,ny)){a=true;Msg(g,"MARCHA");}Cam(g);}
   if(!a)Msg(g,"SEM MARCHA");}g->dirty=true;}
 if(d&KEY_X){Tile*t=Map_Get(g,g->selX,g->selY);
  if(t&&(t->building==BLD_WALL||t->building==BLD_FORT)){
   if(Eco_FortUp(g,g->selX,g->selY))Msg(g,"FORTE");
   else Msg(g,"SEM OURO");}
  else if(Eco_Recruit(g,g->selX,g->selY))Msg(g,"RECRUTA");
  else Msg(g,"SEM ESTOQUE");
   g->dirty=true;}
 if(d&KEY_Y){
  if(Eco_Build(g,g->selX,g->selY,cyc))Msg(g,"OBRA OK");
  else Msg(g,"SEM TESOURO");
  cyc++;if(cyc>=BLD_COUNT)cyc=BLD_FARM;g->dirty=true;}
 if(d&KEY_L){
  if(h&KEY_SELECT){ // SELECT+L = troca de governo (MON > DEM > TEO)
   if(Gov_Cycle(g,1)){const char *gn=Gov_Name(g->factions[1].gov);Msg(g,gn);}
   else Msg(g,"GOV BLOQ +50 OURO");
  }else{
   u8 r=(g->factions[1].taxRate+1)%3;Eco_Tax(g,1,r);
   char m[10]={'I','M','P',' ','0'+r,0};Msg(g,m);
  }
  g->dirty=true;}
 if(d&KEY_R){u8 t=g->factions[2].tilesOwned<=g->factions[3].tilesOwned?2:3;
  if(h&KEY_UP){ // R+CIMA = alianca (uniao aduaneira)
   if(Dip_Ally(g,1,t))Msg(g,"UNIAO ADUANEIRA");
   else Msg(g,"REL 70 + 100 OURO");}
  else{if(Dip_Peace(g,1,t))Msg(g,"PAZ");else Msg(g,"SEM OURO");}
  g->dirty=true;}
 if(d&KEY_B){
  if(h&KEY_DOWN){Mkt_SellExt(g,1,good,5);Msg(g,"EXPORT");}
  else{Mkt_BuyInt(g,1,good,5);Msg(g,"COMPRA INT");}
  good=(good+1)%GOOD_COUNT;g->dirty=true;}
 if(d&KEY_TOUCH){touchPosition tc;touchRead(&tc);
  // viewport 16x16 tiles de 16px: 1:1 na tela cima (sem escala)
  int gx=g->camX+(((int)tc.px)/16), gy=g->camY+(((int)tc.py)/16);
  if(tileInside(gx,gy)){g->selX=(u8)gx;g->selY=(u8)gy;Cam(g);}}
}
