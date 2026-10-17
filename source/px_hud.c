// px_hud.c — v0.8: tela CIMA = HUD executivo (informacao, minimapa, evento)
// ou arte do menu. Cores via tokens de include/pal.h (fonte unica, SDD sec
// 14.1; ADR-13). O mapa vive na tela baixo (caneta); F3 da v0.9 troca as telas.
#include "px.h"
#include "pal.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "gov.h"
#include "diplomacy.h"
#include "menu.h"

static void Panel(u16*d,int x,int y,int w,int h,u16 face,u16 edge){
 Px_Rect(d,x,y,w,h,face);
 Px_Border(d,x,y,w,h,edge);
}
// texto sequencial: desenha e devolve o x depois da palavra (+4 de espaco)
static int DrawTxt(u16*d,int x,int y,const char*s,u16 c){
 Px_TextSc(d,x,y,s,c,1);return x+Px_Width(s)+4;
}
static int DrawNum(u16*d,int x,int y,long v,u16 c){
 char b[10];Px_Itoa((int)v,b);Px_TextSc(d,x,y,b,c,1);return x+Px_Width(b)+4;
}

// icone: moeda dourada 8x8
static void IcoCoin(u16*d,int x,int y){
 for(int j=0;j<8;j++)for(int i=0;i<8;i++){
  int dx=i-3,dy=j-3,d2=dx*dx+dy*dy;
  u16 c=0;
  if(d2<=9)c=(dx<-1||dy<-1)?C_GOLD_HI:C_GOLD;
  else if(d2<=12)c=C_INK;
  if(c)d[(y+j)*256+x+i]=c;
 }
}
// icone: coroa 8x7
static void IcoCrown(u16*d,int x,int y,u16 c){
 for(int i=0;i<8;i++)d[(y+5)*256+x+i]=c;
 d[(y+1)*256+x]=c;d[(y+2)*256+x]=c;
 d[(y+1)*256+x+3]=c;
 d[(y+1)*256+x+6]=c;d[(y+2)*256+x+6]=c;
 d[(y+2)*256+x+2]=c;d[(y+2)*256+x+4]=c;
 d[(y+3)*256+x+1]=c;d[(y+3)*256+x+2]=c;d[(y+3)*256+x+3]=c;
 d[(y+3)*256+x+4]=c;d[(y+3)*256+x+5]=c;
 d[(y+4)*256+x+1]=c;d[(y+4)*256+x+5]=c;
}
static void IcoGrain(u16*d,int x,int y){
 for(int k=0;k<6;k++)d[(y+2+(k>3))*256+x+1+k]=(k&1)?C_WARM:C_WARM_DIM;
 d[(y+1)*256+x+2]=C_WARM;d[(y+1)*256+x+4]=C_WARM;
}
static const char *TerName(u8 t){
 switch(t){
  case TER_WATER:    return "MAR";     case TER_FOREST:  return "FLORES";
  case TER_HILLS:    return "COLINAS"; case TER_MOUNTAIN:return "SERRA";
  case TER_DESERT:   return "DESERTO"; case TER_SWAMP:   return "PANTANO";
  case TER_VOLCANO:  return "VULCAO";  case TER_RUINS:   return "RUINAS";
  case TER_OASIS:    return "OASIS";   default:          return "LAVOURA";
 }
}
// minimapa 2px/tile + moldura do viewport + ponto do evento
static void Minimap(u16*d,GameState*g,int mx,int my){
 Panel(d,mx-2,my-2,68,52,C_NAVY,C_GOLD);
 for(int ty=0;ty<MAP_H;ty++)for(int tx=0;tx<MAP_W;tx++){
  u16 c=Px_MiniCol(g,(u8)tx,(u8)ty);
  for(int j=0;j<2;j++)for(int i=0;i<2;i++)
   d[(my+ty*2+j)*256+mx+tx*2+i]=c;
 }
 int vx=mx+g->camX*2, vy=my+g->camY*2;
 for(int k=0;k<32;k++){
  d[(my+g->camY*2)*256+vx+k]=C_WHITE;
  d[(vy+24)*256+vx+k]=C_WHITE;
 }
 for(int k2=0;k2<25;k2++){
  d[(vy+k2)*256+vx]=C_WHITE;
  d[(vy+k2)*256+vx+32]=C_WHITE;
 }
 if(g->lastEvt&&g->evtX<MAP_W&&g->evtY<MAP_H){
  int ex=mx+g->evtX*2, ey=my+g->evtY*2;
  d[ey*256+ex]=C_CRIMSON;d[ey*256+ex+1]=C_CRIMSON;
  d[(ey+1)*256+ex]=C_CRIMSON;
 }
}

// ============ HUD ============
void Px_Top(GameState *g){
 u16*d=Px_BackTop();
 if(Menu_Owns(g->screen)){Menu_DrawTop(g,d);Px_FlipTop();return;}
 Faction *P=&g->factions[1];
 for(int j=0;j<192;j++)for(int i=0;i<256;i++)d[j*256+i]=C_NAVY_BG;
 // header: dia, velocidade, gov, pausa
 Panel(d,0,0,256,16,C_NAVY,C_GOLD_DIM);
 int hx=6;
 hx=DrawTxt(d,hx,6,"DIA",C_TEXT);
 hx=DrawNum(d,hx,6,(long)g->day,C_TEXT);
 {char vs[3]={'V',(char)('0'+g->speed),0};hx=DrawTxt(d,hx,6,vs,C_TEXT);}
 hx=DrawTxt(d,hx,6,Gov_Short(P->gov),C_TEXT);
 if(g->paused)DrawTxt(d,hx,6,"PAUSA",C_CRIMSON);
 // linha de recursos
 IcoCoin(d,10,23);
 char nb[10];Px_Itoa(P->treasury,nb);
 Px_TextSc(d,22,23,nb,C_GOLD,2);
 IcoCrown(d,90,23,C_GOLD);
 char pb[10];Px_Itoa(P->prestige,pb);
 Px_TextSc(d,102,23,pb,C_GOLD,2);
 char pd[10]={'/',0};Px_Itoa(WIN_PRESTIGE,pd+1);
 Px_TextSc(d,102+Px_Width(pb)*2+3,29,pd,C_TEXT_DIM,1);
 IcoGrain(d,174,25);
 char gb[10];Px_Itoa(P->stock[GOOD_GRAIN],gb);
 Px_TextSc(d,186,23,gb,C_WARM,2);
 // barras de progresso
 Px_BarT(6,50,120,6,(P->prestige*100)/WIN_PRESTIGE,C_GOLD);
 Px_TextSc(d,6,59,"PRESTIGE",C_TEXT_DIM,1);
 Px_BarT(134,50,116,6,(P->stock[GOOD_GRAIN]*100)/500,C_WARM);
 Px_TextSc(d,134,59,"CESTA DE GRAO",C_TEXT_DIM,1);
 // card do tile selecionado
 Panel(d,4,72,120,76,C_NAVY,C_GOLD_DIM);
 Px_TextSc(d,10,78,"POS",C_GOLD_DIM,1);
 Tile*t=Map_Get(g,g->selX,g->selY);
 if(t){
  u8 o=t->owner;
  u16 oc=(o==1)?C_PLAYER:(o==2)?C_RIVAL_A:(o==3)?C_RIVAL_B:C_TEXT_DIM;
  Px_Rect(d,10,86,6,6,oc);
  const char*on=(o==1)?"VOCE":(o==2)?"NORTE":(o==3)?"SUL":"NEUTRO";
  Px_TextSc(d,20,86,on,oc,1);
  Px_TextSc(d,10,97,TerName(t->terrain),C_TEXT,1);
  if(t->building)Px_TextSc(d,10,108,Eco_BldName(t->building),C_GOLD,1);
  if(t->troops){
   int dx3=10;
   dx3=DrawTxt(d,dx3,119,"TROPA",C_TEXT);
   DrawNum(d,dx3,119,t->troops,C_TEXT);
  }
  if(t->dstX!=0xFF){
   int mx3=10;
   mx3=DrawTxt(d,mx3,130,"MARCHA",C_GOLD);
   mx3=DrawNum(d,mx3,130,t->dstX,C_GOLD);
   DrawNum(d,mx3,130,t->dstY,C_GOLD);
  }
 }
 // minimapa na direita
 Minimap(d,g,186,50);
 // relacoes (panel debaixo do card)
 Panel(d,4,152,120,26,C_NAVY,C_GOLD_DIM);
 for(int i=0;i<2;i++){
  u8 f=(u8)(i+2);
  u8 st=Dip_Get(g,1,f);
  u16 sc=(st==DIP_WAR)?C_CRIMSON:(st==DIP_ALLY)?C_PLAYER:C_GOLD_DIM;
  Px_TextSc(d,10,154+i*10,(f==2)?"NORTE":"SUL   ",C_TEXT_DIM,1);
  Px_TextSc(d,56,154+i*10,Dip_Name(st),sc,1);
 }
 // log evento/status
 Panel(d,128,152,124,26,C_NAVY,C_GOLD_DIM);
 if(g->evtCd>0)Px_TextSc(d,134,157,g->evtMsg,C_EVENT,1);
 else Px_TextSc(d,134,157,g->status,C_TEXT,1);
 Px_TextSc(d,134,168,"ARRASTA O MAPA",C_TEXT_DIM,1);
 Px_FlipTop();
}
