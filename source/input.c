#include "game.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "combat.h"
#include "diplomacy.h"
#include "gov.h"
#include "save.h"
#include "ui.h"
#include "menu.h"
#include <stdio.h>
// input.c — v0.8 CANETA-FIRST.
// Toque: arrastar = pan do mapa (acumulador de pixel; a camera anda a cada
// 16px de arraste); tocar e soltar = tap despachado pela tabela ui.c (dock/
// abas/acoes do tile/destino de marcha). Botoes ficam como atalho legado:
// A marcha auto, X recruta, Y abre OBRAS, L imposto, R paz, SELECT+L governo,
// SELECT+B save, SELECT+R load, START pausa.
static s32 tPX=-1,tPY=-1,downX=0,downY=0,accX=0,accY=0;
static u32 tPrevHeld=0;
static bool dragging=false;

static void Cam(GameState *g){
 if(g->selX<g->camX)g->camX=g->selX;
 if(g->selX>=g->camX+VIEW_W)g->camX=g->selX-VIEW_W+1;
 if(g->selY<g->camY)g->camY=g->selY;
 if(g->selY>=g->camY+VIEW_H)g->camY=g->selY-VIEW_H+1;
 g->dirty=true;
}
static void Msg(GameState *g,const char *m){
 int i=0;while(m[i]&&i<63){g->status[i]=m[i];i++;}g->status[i]=0;
 g->dirty=true;
}
// pan: acumula pixels; a cada 16 a camera anda 1 tile na direcao do dedo
static void PanSlide(GameState *g,int dx,int dy){
 accX+=dx;accY+=dy;
 while(accX>=16){accX-=16;if(g->camX>0){g->camX--;g->dirtyBot=true;}}
 while(accX<=-16){accX+=16;if(g->camX+VIEW_W<MAP_W){g->camX++;g->dirtyBot=true;}}
 while(accY>=16){accY-=16;if(g->camY>0){g->camY--;g->dirtyBot=true;}}
 while(accY<=-16){accY+=16;if(g->camY+VIEW_H<MAP_H){g->camY++;g->dirtyBot=true;}}
}
// marcha do tile selecionado ate (dx,dy) relativo (distancia 1)
static void TryMarch(GameState *g,int dx,int dy){
 if(dx==0&&dy==0){Msg(g,"TOQUE UM VIZINHO");return;}
 if(abs(dx)>1||abs(dy)>1){Msg(g,"SO VIZINHO");return;}
 Tile*t=Map_Get(g,g->selX+dx,g->selY+dy);
 if(t&&Combat_Order(g,g->selX,g->selY,dx,dy)){
  Msg(g,"TROPAS EM MARCHA");g->uiPick=0;g->dirty=true;
 }else Msg(g,"DESTINO INVALIDO");
}
// despacho de tap (ao SOLTAR a caneta). id<0 so chega aqui quando uma aba
// esta aberta: significa "toque fora da aba" -> fecha.
static void Tap(GameState *g,int id,int tx,int ty){
 (void)ty; // so o mercado usa a coluna do toque (rel); resto e' por id
 Tile *sel=Map_Get(g,g->selX,g->selY);
 switch(id){
  case UI_CLOSE:  Ui_Close(g);return;
  case UI_D_OBRAS:Ui_Open(g,UI_BUILD);return;
  case UI_D_MERC: Ui_Open(g,UI_TRADE);return;
  case UI_D_DIPLO:Ui_Open(g,UI_DIPLO);return;
  case UI_D_SYS:  Ui_Open(g,UI_SYS);return;
  case UI_D_PAUSE:
   g->paused=!g->paused;Msg(g,g->paused?"PAUSE":"GO");g->dirtyBot=true;return;
  case UI_D_SPEED:
   g->speed=(u8)(g->speed<3?g->speed+1:1);
   Msg(g,"VELOCIDADE");g->dirtyBot=true;return;
  case UI_A_MARCH:
   if(!sel||sel->owner!=1||sel->troops<=0){Msg(g,"SEM TROPA AQUI");g->uiPick=0;return;}
   if(g->uiPick==1){g->uiPick=0;Msg(g,"MARCHA CANCELADA");return;}
   g->uiPick=1;Msg(g,"TOQUE O DESTINO VIZINHO");g->dirtyBot=true;return;
  case UI_A_REC:
   if(!sel||sel->owner!=1)return;
   if(Eco_Recruit(g,g->selX,g->selY))Msg(g,"TROPA RECRUTADA");
   else Msg(g,"SEM OURO OU POPULACAO");
   return;
  case UI_A_FORT:
   if(!sel||sel->owner!=1)return;
   if(sel->building==BLD_WALL||sel->building==BLD_FORT||sel->building==BLD_BARRACKS){
    if(Eco_FortUp(g,g->selX,g->selY))Msg(g,"FORTE MELHORADO");
    else Msg(g,"SEM OURO");
   }else Msg(g,"CONSTRUA MURALHA/FORTE");
   return;
  default:break;
 }
 if(id>=UI_GV_MON&&id<=UI_GV_TEO){
  if(Gov_Switch(g,1,(u8)(id-UI_GV_MON)))Msg(g,"GOVERNO TROCADO");
  else Msg(g,"EM CD OU SEM OURO");
  g->dirty=true;g->dirtyBot=true;return;
 }
 if(id==UI_SP_IMPT){
  Eco_Tax(g,1,(u8)((g->factions[1].taxRate+1)%3));
  Msg(g,"IMPOSTO AJUSTADO");g->dirty=true;g->dirtyBot=true;return;
 }
 if(id>=UI_SP_SP1&&id<=UI_SP_SP3){
  g->speed=(u8)(id-UI_SP_SP1+1);Msg(g,"VELOCIDADE");g->dirtyBot=true;return;
 }
 if(id==UI_SP_PAUSE){g->paused=!g->paused;Msg(g,g->paused?"PAUSE":"GO");g->dirtyBot=true;return;}
 if(id==UI_SP_SAVE){Save_W(g);return;}
 if(id==UI_SP_LOAD){if(!Save_R(g))Msg(g,"SEM SAVE");g->dirtyBot=true;return;}
 if(id==UI_SP_NEW){g->uiPage=UI_MAP;Game_Init(g);Menu_Go(g,SCR_MAIN);return;}
 if(id>=UI_DP_BASE&&id<UI_DP_BASE+6){
  u8 f=(u8)(2+(id-UI_DP_BASE)/2);
  bool ally=((id-UI_DP_BASE)%2)==1;
  if(ally){
   if(Dip_Ally(g,1,f)){Msg(g,"UNIAO PROPOR");}else Msg(g,"NAO ACEITOU");
  }else{
   if(Dip_Peace(g,1,f))Msg(g,"PAZ ASSINADA");else Msg(g,"JA EM PAZ");
  }
  g->dirty=true;g->dirtyBot=true;return;
 }
 if(id>=UI_TR_SHOP&&id<UI_TR_SHOP+GOOD_COUNT*3){
  u8 gd=(u8)((id-UI_TR_SHOP)/3);
  const Btn*row=&Ui_Tbl(UI_TRADE)[gd];
  int rel=tx-(int)row->x;
  if(rel<64){Msg(g,Mkt_Name(gd));return;} // toque no nome: informa o bem
  if(rel<100){if(Mkt_BuyInt(g,1,gd,5))Msg(g,"COMPRA NO MERC INT");else Msg(g,"SEM OURO");}
  else if(rel<136){if(Mkt_BuyExt(g,1,gd,5))Msg(g,"COMPRA NO MERC EXT");else Msg(g,"SEM OURO");}
  else{if(Mkt_SellExt(g,1,gd,5))Msg(g,"VENDEU 5");else Msg(g,"SEM ESTOQUE");}
  g->dirty=true;g->dirtyBot=true;return;
 }
 if(id>=1&&id<BLD_COUNT&&g->uiPage==UI_BUILD){
  if(Eco_Build(g,g->selX,g->selY,(u8)id))Msg(g,"OBRA INICIADA");
  else Msg(g,"AQUI NAO/SEM OURO");
  g->dirty=true;g->dirtyBot=true;return;
 }
}
void In_Handle(GameState *g){
 scanKeys();
 u32 d=keysDown(),h=keysHeld();
 touchPosition tc;touchRead(&tc);
 s32 ppx=tc.px,ppy=tc.py;
 bool tNow=(h&KEY_TOUCH)!=0;
 bool tWas=(tPrevHeld&KEY_TOUCH)!=0;

 // v0.9: menus e partida no mundo (menu.c); daqui p/ baixo so' o classico
 if(Menu_Owns(g->screen)){
  Menu_Input(g,d,h,tNow&&!tWas,!tNow&&tWas,tNow&&tWas,ppx,ppy);
  tPrevHeld=h;tPX=ppx;tPY=ppy;return;
 }
 if(g->gameOver){if(d&KEY_START){Game_Init(g);Menu_Go(g,SCR_MAIN);}tPrevHeld=h;return;}

 // ---------------- teclado legado ----------------
 if(d&KEY_START){g->paused=!g->paused;g->dirty=true;}
 if((d&KEY_SELECT)&&(h&KEY_L)){
  if(Gov_Cycle(g,1))Msg(g,"GOVERNO TROCADO");else Msg(g,"EM CD/SEM OURO");
  g->dirty=true;
 }
 if((d&KEY_SELECT)&&(h&KEY_B))Save_W(g);
 if((d&KEY_SELECT)&&(h&KEY_R)){if(!Save_R(g))Msg(g,"SEM SAVE");}
 if(d&KEY_LEFT){if(g->selX>0){g->selX--;Cam(g);}}
 if(d&KEY_RIGHT){if(g->selX+1<MAP_W){g->selX++;Cam(g);}}
 if(d&KEY_UP){if(g->selY>0){g->selY--;Cam(g);}}
 if(d&KEY_DOWN){if(g->selY+1<MAP_H){g->selY++;Cam(g);}}
 if(d&KEY_A){
  static const s8 EX[8]={1,1,1,0,0,-1,-1,-1},EY[8]={-1,0,1,-1,1,-1,0,1};
  bool ordered=false;
  for(int i=0;i<8&&!ordered;i++){
   int nx=(int)g->selX+EX[i],ny=(int)g->selY+EY[i];
   if(nx<0||ny<0||nx>=MAP_W||ny>=MAP_H)continue;
   Tile*nt=&g->tiles[ny][nx];
   if(!tileWalk(nt->terrain))continue; // nem p/ agua/rocha
   if(nt->owner!=1)ordered=Combat_Order(g,g->selX,g->selY,EX[i],EY[i]);
  }
  if(!ordered)Msg(g,"SEM TROPA OU ALVO");
 }
 if(d&KEY_Y){Ui_Open(g,UI_BUILD);}
 else if(d&KEY_X){if(Eco_Recruit(g,g->selX,g->selY))Msg(g,"TROPA OK");else Msg(g,"SEM OURO/POP");}
 if(d&KEY_L){Eco_Tax(g,1,(u8)((g->factions[1].taxRate+1)%3));Msg(g,"IMPOSTO AJUSTADO");g->dirty=true;}
 if(d&KEY_R){
  bool sent=false;
  for(u8 i=2;i<MAX_FACTIONS&&!sent;i++)
   if(Dip_Get(g,1,i)!=DIP_PEACE){sent=Dip_Peace(g,1,i);}
  Msg(g,sent?"EMISSARIO ENVIADO":"SEM GUERRA ATIVA");
 }
 // ---------------- caneta ----------------
 if(tNow&&!tWas){downX=ppx;downY=ppy;accX=0;accY=0;dragging=false;}
 if(tNow&&tPX>=0){
  if(!dragging&&(abs(ppx-downX)+abs(ppy-downY)>10)){
   dragging=true;g->uiPick=0; // arrastar cancela pick de marcha
  }
  if(dragging&&g->uiPage==UI_MAP)PanSlide(g,ppx-tPX,ppy-tPY);
 }
 if(!tNow&&tWas&&!dragging&&!g->gameOver){
  int id;
  if(ppy>=166)id=Ui_Hit(UI_MAP,ppx,ppy);          // dock vivo sob qualquer aba
  else if(g->uiPage==UI_MAP)id=Ui_Hit(UI_MAP,ppx,ppy);
  else id=Ui_Hit(g->uiPage,ppx,ppy);
  if(id>=0)Tap(g,id,ppx,ppy);
  else if(g->uiPage==UI_MAP){
   int mx=ppx/16,my=ppy/16;
   if(g->uiPick==1){
    TryMarch(g,mx-(int)g->selX+(int)g->camX,my-(int)g->selY+(int)g->camY);
    g->dirtyBot=true;
   }else{
    int nx=(int)g->camX+mx,ny=(int)g->camY+my;
    if(nx>=0&&ny>=0&&nx<MAP_W&&ny<MAP_H){
     g->selX=(u8)nx;g->selY=(u8)ny;Cam(g);g->dirtyBot=true;
    }
   }
  }else Ui_Close(g); // toque fora da aba fecha
 }
 if(!tNow&&tWas)dragging=false;
 tPrevHeld=h;tPX=ppx;tPY=ppy;
}
