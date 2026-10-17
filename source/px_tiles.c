// px_tiles.c — tiles 16x16px procedurais + painel profissional.
// OTIMIZACAO DS: TilePx usa so soma/bitwise nos pixels (sem divisao/mod);
// agua/cursor/evento animam por `phase` (redesenho 1x/15 frames, ver main.c).
// Paleta: borda dono (P verde, N vermelho, S amarelo, neutro cinza).
#include "px.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "diplomacy.h"
#include "gov.h"
void Px_TextTop(int x,int y,const char *s,u16 col);
static u32 s_tick=0; // avanca 1 por redesenho; phase anima agua/cursor/evento

// Textura do terreno (barata: comparacoes e mascaras, sem mod/div).
static u16 TerPx(u8 ter,int i,int j,int tx,int ty,int phase){
 int hash=(tx*7+ty*13+i-j)&3;
 switch(ter){
  case TER_WATER: // brilho animado correndo na agua
   if((((i+j+phase*3)&7)==0)) return Px_Col(10,16,31);
   return Px_Col(6,10,28);
  case TER_FOREST: // copas: pontos escuros + clareira por hash
   if(((i&3)==3)&&(j==2||j==8||j==14)) return Px_Col(3,11,3);
   if(hash==0) return Px_Col(6,18,6);
   return Px_Col(5,16,5);
  case TER_HILLS: // curvas de nivel
   if(j==4||j==11) return Px_Col(12,10,6);
   return Px_Col(16,14,8);
  case TER_MOUNTAIN: // neve no topo alternada
   if(((i+j)&1)==0) return Px_Col(22,22,24);
   return Px_Col(16,16,18);
  case TER_DESERT: // graos esparsos
   if(((i+j)&7)==0) return Px_Col(31,28,18);
   return Px_Col(28,24,14);
  case TER_SWAMP:
   if((i&1)&&(j&1)) return Px_Col(5,11,8);
   return Px_Col(8,16,12);
  case TER_VOLCANO: // lava embaixo
   if(j>12) return Px_Col(30,12,4);
   if(((i+j)&3)==0) return Px_Col(18,6,5);
   return Px_Col(24,8,6);
  case TER_RUINS: // pilares
   if(i==3||i==12) return Px_Col(20,18,20);
   return Px_Col(14,12,14);
  case TER_OASIS: // agua no centro
   if(i>5&&i<9&&j>5&&j<9) return Px_Col(8,14,30);
   return Px_Col(12,24,18);
  default: // planicie: dither + variacao por hash
   if(((i+j)&3)==0) return Px_Col(8,19,6);
   if(hash==0) return Px_Col(11,23,9);
   return Px_Col(10,22,8);
 }
}

static void TilePx(u16 *b,int px,int py,Tile *t,u8 owner,int tx,int ty,int phase){
 u8 ter=t->terrain, site=t->site, bld=t->building;
 for(int j=0;j<16;j++)for(int i=0;i<16;i++){
  u16 cc=TerPx(ter,i,j,tx,ty,phase);
  if(site==SITE_GOLDMINE&&(i==7||i==8)&&(j>5&&j<10))
   cc=Px_Col(30,26,8);
  else if(site==SITE_IRON&&(i==7||i==8)&&(j>5&&j<10))
   cc=Px_Col(24,10,8);
  else if(site==SITE_FERTILE&&(((i+j)&7)==0))
   cc=Px_Col(14,28,10);
  else if(site==SITE_RUINARM&&(i==j||i+j==15))
   cc=Px_Col(22,22,22);
  else if(site==SITE_CURSED&&(i==j||i+j==15))
   cc=Px_Col(18,4,18);
  else if(site==SITE_COAL&&(i==6||i==9)&&(j>5&&j<10))
   cc=Px_Col(8,8,8);
  if(bld==BLD_FARM||bld==BLD_ORCHARD){ if(j==3||j==12)cc=Px_Col(24,22,6); }
  else if(bld==BLD_STEEL||bld==BLD_TOOLSHOP){ if(i>4&&i<11&&j>4&&j<11)cc=Px_Col(20,20,22); }
  else if(bld==BLD_FORT){ if(i==0||j==0||i==15||j==15)cc=Px_Col(22,22,22); }
  else if(bld==BLD_WALL){ if(i==0||j==0||i==15||j==15)cc=Px_Col(14,14,14); }
  else if(bld==BLD_PORT||bld==BLD_DOCKYARD){ if(j>12)cc=Px_Col(6,10,28); }
  else if(bld==BLD_MARKET||bld==BLD_BANK||bld==BLD_CUSTOMS){ if(i==7||i==8)cc=Px_Col(30,26,8); }
  else if(bld==BLD_UNIV||bld==BLD_THEATER||bld==BLD_SHRINE){ if(i>5&&i<10&&j>5&&j<10)cc=Px_Col(26,10,26); }
  else if(bld==BLD_MINE||bld==BLD_COALPIT){ if(i>4&&i<11&&j>8)cc=Px_Col(12,12,12); }
  else if(bld==BLD_TEXTILE||bld==BLD_WINERY||bld==BLD_GLASSWORKS){ if(j==7||j==8)cc=Px_Col(28,12,12); }
  else if(bld==BLD_BARRACKS){ if(i==2||i==13||j==2||j==13)cc=Px_Col(28,8,8); }
  else if(bld==BLD_HOSPITAL){ if(i==7||i==8||j==7||j==8)cc=Px_Col(28,28,28); }
  Px_PSet(b,px+i,py+j,cc);
 }
 // tropas = quadradinhos brancos (max 6 visiveis)
 for(int k=0;k<t->troops&&k<6;k++)
  Px_Rect(b,px+2+k*2,py+2,1,2,Px_Col(31,31,31));
 // marcha = seta direcional p/ o destino (usa dst do tile + posicao)
 if(t->moveCd>0&&t->dstX!=0xFF){
  int dx=(int)t->dstX-tx, dy=(int)t->dstY-ty, ax=6, ay=7;
  if(dx>0)ax=9; else if(dx<0)ax=3;
  if(dy>0)ay=10; else if(dy<0)ay=4;
  u16 rc=(phase)?Px_Col(31,8,8):Px_Col(31,18,8);
  Px_Rect(b,px+ax,py+ay,4,2,rc);
 }
 // borda dono
 u16 oc=Px_Col(10,10,10);
 if(owner==1)oc=Px_Col(8,28,8);
 else if(owner==2)oc=Px_Col(28,8,8);
 else if(owner==3)oc=Px_Col(28,24,8);
 Px_Border(b,px,py,16,16,oc);
}

// Caixa com borda dupla p/ menus/overlays (visual profissional barato).
static void Box(int x,int y,int w,int h){
 u16 *b=Px_BackTop();
 Px_Rect(b,x,y,w,h,Px_Col(3,4,10));
 Px_Border(b,x,y,w,h,Px_Col(30,26,8));
 Px_Border(b,x+2,y+2,w-4,h-4,Px_Col(12,14,24));
}

void Px_Top(GameState *g){
 s_tick++;
 int phase=(s_tick>>1)&1;
 u16 *b=Px_BackTop();
 Px_Clear(b,Px_Col(0,0,0));
 if(g->inMenu){
  Px_Rect(b,0,0,256,192,Px_Col(4,6,14));
  // fileira d'agua decorativa embaixo (anima com phase)
  for(int i=0;i<256;i+=4)
   Px_Rect(b,i,182+(((i+phase*2)&3)==0),4,2,Px_Col(6,10,28));
  Box(56,18,144,80);
  Px_TextTop(92,30,"OH MY VICTORIA DS!",Px_Col(30,26,8));
  Px_TextTop(96,44,"V0.7 GOVERNO+LEIS",Px_Col(20,22,30));
  Px_TextTop(76,62,"L/R DIFICULDADE",Px_Col(22,22,22));
  Px_TextTop(76,74,Eco_DiffName(g->diff),Px_Col(8,28,8));
  // seed em hexa (itoa manual, 8 nibbles)
  char sh[14]={'S','E','E','D',' ',0};
  u32 sd=g->seed?g->seed:DEFAULT_SEED;
  int n=5;
  for(int i2=7;i2>=0;i2--){int nb=(sd>>(i2*4))&15;sh[n++]=(char)(nb<10?'0'+nb:'A'+nb-10);}
  sh[n]=0;
  Px_TextTop(76,86,sh,Px_Col(16,20,28));
  if(phase) Px_TextTop(76,110,"A COMECA  B SEED",Px_Col(28,28,20));
  else Px_TextTop(76,110,"A COMECA  B SEED",Px_Col(14,14,14));
  Px_TextTop(58,140,"8 BENS 23 OBRAS 4 POPS 3 GOV",Px_Col(16,20,28));
  Px_TextTop(52,152,"MON DEM TEO: ESCOLHA SUA LEI",Px_Col(18,22,28));
  Px_FlipTop(); g->dirty=false; return;
 }
 for(int vy=0;vy<VIEW_H;vy++)for(int vx=0;vx<VIEW_W;vx++){
  int x=g->camX+vx, y=g->camY+vy;
  if(x>=MAP_W||y>=MAP_H)continue;
  TilePx(b,vx*16,vy*16,&g->tiles[y][x],g->tiles[y][x].owner,x,y,phase);
 }
 // cursor piscante + marcador de evento piscante
 int cx=(g->selX-g->camX)*16, cy=(g->selY-g->camY)*16;
 if(cx>=0&&cx<256&&cy>=0&&cy<192)
  Px_Border(b,cx,cy,16,16,phase?Px_Col(31,31,31):Px_Col(16,16,16));
 if(g->lastEvt!=0){
  int ex=(g->evtX-g->camX)*16+6, ey=(g->evtY-g->camY)*16+2;
  if(ex>=0&&ex<250&&ey>=0&&ey<186)
   Px_Rect(b,ex,ey,5,5,phase?Px_Col(31,16,0):Px_Col(31,28,8));
 }
 // overlay de fim de jogo (profissional: caixa + resultado + instrucao)
 if(g->gameOver){
  Box(48,66,160,60);
  const char *t=g->winner==1?"VITORIA!":"DERROTA";
  u16 col=g->winner==1?Px_Col(30,26,8):Px_Col(28,8,8);
  Px_TextTop(113,80,t,col);
  Px_TextTop(72,96,"START VOLTA AO MENU",Px_Col(22,22,22));
 }
 Px_FlipTop(); g->dirty=false;
}
void Px_Bot(GameState *g){
 u16 *b=Px_BackBot();
 Px_Clear(b,Px_Col(2,2,6));
 Faction*p=&g->factions[1];
 char line[48]; int n=0;
 // linha 1: dia + vel + governo + clima/pausa
 line[n++]='D';unsigned long d=g->day;
 char tmp[12];int tl=0;if(d==0)tmp[tl++]='0';
 while(d>0&&tl<10){tmp[tl++]='0'+(d%10);d/=10;}
 for(int i=tl-1;i>=0&&n<10;i--)line[n++]=tmp[i];
 line[n++]=' ';line[n++]='V';line[n++]='0'+g->speed;
 line[n++]=' ';
 const char *gs=Gov_Short(p->gov);
 line[n++]=gs[0];line[n++]=gs[1];line[n++]=gs[2];
 if(g->paused){line[n++]=' ';line[n++]='P';}
 if(g->weather==1){line[n++]=' ';line[n++]='S';}
 if(g->weather==2){line[n++]=' ';line[n++]='C';}
 line[n++]=0;
 Px_Text(4,4,line,Px_Col(28,28,20));
 // linha 2: tesouro + prestigio
 char l2[48];n=0;
 l2[n++]='T';int v=p->treasury;char t2[8];int k=0;
 if(v==0)t2[k++]='0';
 while(v>0&&k<6){t2[k++]='0'+(v%10);v/=10;}
 for(int i=k-1;i>=0&&n<20;i--)l2[n++]=t2[i];
 l2[n++]=' ';l2[n++]='P';int pv=p->prestige;char p2[8];k=0;
 if(pv==0)p2[k++]='0';
 while(pv>0&&k<6){p2[k++]='0'+(pv%10);pv/=10;}
 for(int i=k-1;i>=0&&n<40;i--)l2[n++]=p2[i];
 l2[n++]=0;
 Px_Text(4,14,l2,Px_Col(30,26,8));
 // barras: grao (sobrevivencia) + prestigio (vitoria 600)
 Px_Bar(4,24,60,(p->stock[0]*100)/(p->stock[0]+50),Px_Col(24,20,6));
 Px_Text(68,22,"GR",Px_Col(24,20,6));
 int pp=(p->prestige*100)/WIN_PRESTIGE;
 Px_Bar(100,24,60,pp,Px_Col(30,26,8));
 Px_Text(164,22,"PR",Px_Col(30,26,8));
 // precos interno/ext grao + ext arma
 char l3[48];n=0;
 int pi=Mkt_IntPrice(g,1,0), pe=Mkt_ExtPrice(g,0);
 l3[n++]='I';l3[n++]='0'+(pi/10);l3[n++]='0'+(pi%10);
 l3[n++]=' ';l3[n++]='E';l3[n++]='0'+(pe/10);l3[n++]='0'+(pe%10);
 l3[n++]=' ';l3[n++]='A';int pa=Mkt_ExtPrice(g,3);
 l3[n++]='0'+(pa/10);l3[n++]='0'+(pa%10);l3[n++]=0;
 Px_Text(4,34,l3,Px_Col(20,24,30));
 // tile selecionado: dono + obra
 Tile*t=Map_Get(g,g->selX,g->selY);
 const char *ow=t->owner==1?"VOCE":(t->owner==2?"NORTE":(t->owner==3?"SUL":"NEUTRO"));
 const char *bn=Eco_BldName(t->building);
 char l4[32];n=0;
 while(*ow&&n<10)l4[n++]=*ow++;
 l4[n++]=' ';
 for(int i=0;i<6&&bn[i]&&n<20;i++)l4[n++]=bn[i];
 l4[n++]=0;
 Px_Text(4,46,l4,Px_Col(22,22,22));
 Px_Bar(4,56,60,t->happiness,Px_Col(8,28,8));
 if(g->evtMsg[0])Px_Text(4,68,g->evtMsg,Px_Col(31,16,0));
 else Px_Text(4,68,g->status,Px_Col(18,20,26));
 Px_Text(4,80,"A MARCHA X REC Y OBRA",Px_Col(16,18,24));
 Px_Text(4,90,"L IMP R PAZ B TRADE",Px_Col(16,18,24));
 Px_Text(4,100,"START PAUSA SEL VEL",Px_Col(16,18,24));
 Px_Text(4,110,"SELECT+B SAVE R LOAD",Px_Col(16,18,24));
 Px_Text(4,120,"SELECT+L GOVERNO",Px_Col(20,26,16));
 Px_FlipBot();
}
