// px_ui.c — v0.8: desenha a interface de CANETA a partir das tabelas ui.c.
// Tela baixo: dock de abas sempre acessivel; abas = painel quase fullscreen
// (OBRAS grade de 23, MERCADO 8 bens, RELACOES, SISTEMA). Menu inicial
// tambem aqui (pagina UI_MENU) com alvos grandes para toque.
#include "px.h"
#include "ui.h"
#include "px_art.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "gov.h"
#include "diplomacy.h"
#include "pal.h"
#include "font.h"

static inline int Clamp31(int v){return v<0?0:(v>31?31:v);}
static inline u16 Shc(int r,int g,int b,int d){
 return Px_Col((u8)Clamp31(r+d),(u8)Clamp31(g+d),(u8)Clamp31(b+d));
}

// paleta UI
static u16 VPanel(void){return Shc(10,11,15,0);}
static u16 VPaper(void){return Shc(26,24,18,0);}
static u16 VGold(void){return Shc(30,26,8,0);}
static u16 VGoldDim(void){return Shc(15,13,7,0);}
static u16 VInk(void){return Shc(8,8,9,0);}
static u16 VRed(void){return Shc(27,9,7,0);}
static u16 VGreen(void){return Shc(9,25,13,0);}
static u16 VFace(void){return Shc(13,14,19,0);}
static u16 VChip(void){return Shc(20,21,27,0);}

static void Chip(u16*d,int x,int y,int w,int h,u16 face,u16 edge){
 Px_Rect(d,x,y,w,h,face);
 for(int i=0;i<w;i++){d[y*256+x+i]=edge;d[(y+h-1)*256+x+i]=edge;}
 for(int j=0;j<h;j++){d[(y+j)*256+x]=edge;d[(y+j)*256+x+w-1]=edge;}
}
static void ChipLbl(u16*d,int x,int y,int w,int h,const char*s,u16 tcol){
 if(!s)return;
 int px=x+(w-Px_Width(s))/2, py=y+(h-5)/2;
 if(px<x+2)px=x+2;
 int cx=px;
 while(*s&&cx<x+w-2){
  const u8*gl=Px_Glyph(*s);
  for(int r=0;r<5;r++)for(int c2=0;c2<3;c2++)
   if(gl[r]&(1<<(2-c2)))Px_PSet(d,cx+c2,py+r,tcol);
  cx+=4;s++;
 }
}
static void LblC(u16*d,int cx,int y,const char*s,u16 col){
 Px_TextSc(d,cx-Px_Width(s)/2,y,s,col,1);
}
static void LblC2x(u16*d,int cx,int y,const char*s,u16 col){
 Px_TextSc(d,cx-Px_Width(s),y,s,col,2);
}
static int H2p(int x,int y){
 unsigned h=((unsigned)(x*73856093u))^((unsigned)(y*19349663u));
 h^=h>>13;h*=0x9E37u;h^=h>>11;
 return (int)(h&0x7FFFFFFFu);
}

// ============ dock + aba corrente sobre o MAPA ============
void Ui_DrawBot(GameState*g,u16 *d){
 u8 page=g->uiPage;
 // faixa contextual do tile selecionado (acoes de caneta sem abrir aba)
 if(page==UI_MAP&&!g->gameOver){
  Tile*t=Map_Get(g,g->selX,g->selY);
  if(t&&t->owner==1){
   u16 face=t->troops?VChip():Shc(9,10,13,0);
   for(const Btn*b=Ui_Ctx();b->id;b++){
    Chip(d,b->x,b->y,b->w,b->h,face,VGoldDim());
    u16 bc=t->troops?VGold():Shc(13,14,17,0);
    ChipLbl(d,b->x,b->y,b->w,b->h,b->lab,bc);
   }
  }
 }
 // dock: 4 abas + chips topo do mapa (pausa/velocidade)
 for(const Btn*b=Ui_Tbl(UI_MAP);b->id;b++){
  bool isDock=(b->w==59);
  u16 face=VFace(),txt=Shc(21,22,28,0);
  if(isDock&&(int)(b->id-10)==(int)page){face=VChip();txt=VGold();}
  else if(b->id==UI_D_PAUSE)txt=g->paused?VRed():Shc(21,22,28,0);
  Chip(d,b->x,b->y,b->w,b->h,face,VGoldDim());
  ChipLbl(d,b->x,b->y,b->w,b->h,b->lab,txt);
  if(b->id==UI_D_PAUSE){
   u16 ic=g->paused?Shc(27,9,7,0):Shc(9,25,13,0);
   // icone: play (seta) ou pausa (2 barras)
   if(g->paused){Chip(d,b->x+6,b->y+4,4,12,ic,ic);Chip(d,b->x+13,b->y+4,4,12,ic,ic);}
   else{
    for(int m=0;m<8;m++)for(int n=0;n<m;n++)
     d[(b->y+6+n)*256+b->x+8+m]=Shc(14,23,14,0);
   }
  }
 }
 if(page==UI_MAP)return;

 // aba aberta: painel de aba (deixa dock visivel embaixo)
 Chip(d,0,0,256,164,VPanel(),VGoldDim());
 for(int i=1;i<255;i++){d[163*256+i]=VGold();}
 const Btn*bt=Ui_Tbl(page);
 if(page==UI_BUILD){
  LblC(d,128,6,"TOQUE A OBRA P/ CONSTRUIR",VGoldDim());
  for(const Btn*b=bt;b->id;b++){
   int cost=Eco_Cost(b->id);
   if(g->factions[1].treasury<cost){
    Chip(d,b->x,b->y,b->w,b->h,Shc(10,10,13,0),Shc(9,9,11,0));
    ChipLbl(d,b->x,b->y,b->w,b->h,Eco_BldName(b->id),Shc(12,13,16,0));
   }else{
    Chip(d,b->x,b->y,b->w,b->h,VFace(),VGoldDim());
    ChipLbl(d,b->x,b->y+2,b->w,b->h/2+2,Eco_BldName(b->id),Shc(27,25,17,0));
    char cg[9]={'$',0};Px_Itoa(cost,cg+1);
    int cw=Px_Width(cg);
    Px_TextSc(d,b->x+(b->w-cw)/2,b->y+14,cg,VGold(),1);
   }
  }
  return;
 }
 if(page==UI_TRADE){
  LblC(d,84,6,"BEM",VGoldDim());
  LblC(d,132,6,"COMPRA INT",VGoldDim());
  LblC(d,182,6,"COMPRA EXT",VGoldDim());
  LblC(d,228,6,"VENDA",VGoldDim());
  for(const Btn*b=bt;b->id;b++){
   u8 gd=(b->id-UI_TR_SHOP)/3;
   Chip(d,b->x,b->y,b->w,b->h,VFace(),Shc(11,12,16,0));
   LblC(d,b->x+20,b->y+5,Mkt_Short(gd),VGold());
   // 3 chips visiveis: colunas batem com o input (borda x+64/100/136)
   u16 bface=Shc(22,23,28,0);
   Chip(d,b->x+66,b->y+2,32,b->h-4,bface,Shc(14,15,10,0));
   Chip(d,b->x+102,b->y+2,32,b->h-4,bface,Shc(14,15,10,0));
   Chip(d,b->x+138,b->y+2,32,b->h-4,bface,Shc(14,15,10,0));
   char pr[8];
   Px_Itoa(Mkt_IntPrice(g,1,gd),pr);
   LblC(d,b->x+82,b->y+5,pr,Shc(26,24,16,0));
   Px_Itoa(Mkt_ExtPrice(g,gd),pr);
   LblC(d,b->x+118,b->y+5,pr,Shc(26,24,16,0));
   LblC(d,b->x+154,b->y+5,"5",Shc(26,24,16,0));
  }
  // rodape: estoque do jogador por bem? compacto: sem
  return;
 }
 if(page==UI_DIPLO){
  LblC(d,128,6,"RELACOES EXTERNAS",VGoldDim());
  u8 fs[2]={2,3};
  for(int i=0;i<2;i++){
   u8 f=fs[i];
   int cy=i?96:24;
   Chip(d,4,cy,248,60,VFace(),Shc(11,12,16,0));
   LblC(d,44,cy+10,(f==2)?"NORTE":"SUL",Shc(27,25,17,0));
   u8 st=Dip_Get(g,1,f);
   u16 sc=st==DIP_WAR?VRed():(st==DIP_ALLY?VGreen():Shc(24,22,15,0));
   LblC(d,140,cy+10,Dip_Name(st),sc);
   LblC(d,44,cy+22,(f==2)?"CUSTO PAZ 28":"CUSTO PAZ 28",Shc(13,14,18,0));
  }
  for(const Btn*b=bt;b->id;b++){
   Chip(d,b->x,b->y,b->w,b->h,VGold(),VInk());
   ChipLbl(d,b->x,b->y,b->w,b->h,b->lab,VInk());
  }
  return;
 }
 if(page==UI_SYS){
  for(const Btn*b=bt;b->id;b++){
   u16 face=VFace(),edge=VGoldDim(),txt=Shc(21,22,28,0);
   if((b->id==UI_GV_MON&&g->factions[1].gov==GOV_MONARCHY)||
      (b->id==UI_GV_DEM&&g->factions[1].gov==GOV_DEMOCRACY)||
      (b->id==UI_GV_TEO&&g->factions[1].gov==GOV_THEOCRACY)){
    face=VGold();txt=VInk();edge=VInk();
   }
   if((b->id==UI_SP_SP1&&g->speed==1)||(b->id==UI_SP_SP2&&g->speed==2)||
      (b->id==UI_SP_SP3&&g->speed==3)||(b->id==UI_SP_IMPT)){
    face=VChip();txt=VGold();
   }
   Chip(d,b->x,b->y,b->w,b->h,face,edge);
   if(b->id==UI_SP_IMPT){
    char lb[8]={'I','M','P',' ',(char)('0'+g->factions[1].taxRate),0};
    ChipLbl(d,b->x,b->y,b->w,b->h,lb,txt);
   }else ChipLbl(d,b->x,b->y,b->w,b->h,b->lab,txt);
  }
  LblC(d,128,120,"GOVERNO: 50 OURO, CD 15D",Shc(13,14,18,0));
  LblC(d,128,130,"TABELA NA DATA DO IMPOSTO",Shc(13,14,18,0));
 }
}

// ============ menu inicial (caneta) ============
static void FmtHex(u32 v,char*out){
 static const char *HX="0123456789ABCDEF";
 int i=0;out[i++]='S';out[i++]='E';
 for(int n=6;n>=0;n--)out[i++]=HX[(v>>(n*4))&15];
 out[i]=0;
}
void Ui_DrawMenu(GameState*g,u16 *d){
 u16 gold=VGold();
 for(int y=0;y<192;y++)for(int x=0;x<256;x++){
  int n=H2p(x,y);
  d[y*256+x]=Shc(26,24,18,((n&3)==0)?1:0);
 }
 Chip(d,2,2,252,188,VPaper(),VInk());
 Chip(d,6,6,244,180,VPaper(),Shc(20,18,12,0));
 LblC(d,128,16,"DIFICULDADE",Shc(24,20,10,0));
 for(const Btn*b=Ui_Tbl(UI_MENU);b->id;b++){
  u16 face=gold,txt=VInk();
  if(b->id==UI_MN_PREVD||b->id==UI_MN_NEXTD||b->id==UI_MN_SEED){
   face=Shc(31,29,23,0);txt=Shc(24,20,10,0);
  }
  Chip(d,b->x,b->y,b->w,b->h,face,VInk());
  ChipLbl(d,b->x,b->y,b->w,b->h,b->lab,txt);
 }
 LblC2x(d,128,66,Eco_DiffName(g->diff),Shc(18,15,8,0));
 char hex[12];FmtHex(g->seed?g->seed:DEFAULT_SEED,hex);
 LblC(d,128,84,hex,Shc(24,20,10,0));
 LblC(d,128,170,"CADA SEED GERA MAPA NOVO",Shc(20,18,12,0));
 LblC(d,128,180,"V0.8 CANETA SOBRE O MAPA",Shc(20,18,12,0));
}

// ======================================================================
// v0.9 — COMPONENTES DA CONCEPT (SDD sec 14.1). Tudo com tokens de pal.h e
// texto Serifa (font.h). As telas (px_menu.c, px_world.c) so' chamam isto.
// ======================================================================

// Pergaminho da concept: papel pintado (PIC_PAPER, preso a' tela: paineis
// vizinhos casam a textura), borda queimada e filete de tinta. Ui_ParchOrn
// (telas de menu) acrescenta o 2o filete e os 4 cantos ornamentados
// recortados da concept; no jogo o conteudo encosta na borda e o ornamento
// cobriria texto.
static inline u16 Darken(u16 c,int k){ // k/16 de escurecimento
 int r=(c&31)*(16-k)/16,g=((c>>5)&31)*(16-k)/16,b=((c>>10)&31)*(16-k)/16;
 return (u16)(BIT(15)|(b<<10)|(g<<5)|r);
}
static void Burn(u16*d,int x,int y,int w,int h,int e0){
 for(int j=0;j<h;j++){
  int py=y+j;if(py<0||py>191)continue;
  int ey=j<h-1-j?j:h-1-j;
  for(int i=0;i<w;i++){
   int ex=i<w-1-i?i:w-1-i;
   int e=ex<ey?ex:ey;if(e>=e0)continue;
   int px=x+i;if(px<0||px>255)continue;
   int k=(e0-e)*(e0-e)*9/(e0*e0);if(ex<e0&&ey<e0)k+=1;
   d[py*256+px]=Darken(d[py*256+px],k>9?9:k);
  }
 }
}
void Ui_Parch(u16*d,int x,int y,int w,int h){
 Pic_BlitClip(d,&PIC_PAPER,x,y,x,y,w,h);
 Burn(d,x,y,w,h,4);
 Px_Border(d,x,y,w,h,PXC(9,7,5));
}
void Ui_ParchOrn(u16*d,int x,int y,int w,int h){
 Pic_BlitClip(d,&PIC_PAPER,x,y,x,y,w,h);
 Burn(d,x,y,w,h,6);
 Px_Border(d,x,y,w,h,PXC(9,7,5));
 Px_Border(d,x+5,y+5,w-10,h-10,PXC(12,9,6));
 Pic_Blit(d,&PIC_PC_TL,x+3,y+3);
 Pic_Blit(d,&PIC_PC_TR,x+w-3-PIC_PC_TR.w,y+3);
 Pic_Blit(d,&PIC_PC_BL,x+3,y+h-3-PIC_PC_BL.h);
 Pic_Blit(d,&PIC_PC_BR,x+w-3-PIC_PC_BR.w,y+h-3-PIC_PC_BR.h);
}
// Painel navy da concept: ardosia com degrade suave e borda ouro dupla.
// Ui_NavyOrn acrescenta os cantos dourados (telas de menu; no jogo o painel
// tem conteudo encostado nas bordas).
void Ui_Navy(u16*d,int x,int y,int w,int h){
 for(int j=0;j<h;j++){
  int py=y+j;if(py<0||py>191)continue;
  int t=j*6/(h?h:1);
  u16 c=PXC(4-t/3,5-t/3,7-t/3+0);
  int x0=x<0?0:x,x1=x+w>256?256:x+w;
  for(int px=x0;px<x1;px++)d[py*256+px]=((H2p(px,py)&15)==0)?PXC(5,6,8):c;
 }
 Px_Border(d,x,y,w,h,C_GOLD);
 Px_Border(d,x+2,y+2,w-4,h-4,C_GOLD_DIM);
}
void Ui_NavyOrn(u16*d,int x,int y,int w,int h){
 Ui_Navy(d,x,y,w,h);
 {
  Pic_Blit(d,&PIC_NC_TL,x+1,y+1);
  Pic_Blit(d,&PIC_NC_TR,x+w-1-PIC_NC_TR.w,y+1);
  Pic_Blit(d,&PIC_NC_BL,x+1,y+h-1-PIC_NC_BL.h);
  Pic_Blit(d,&PIC_NC_BR,x+w-1-PIC_NC_BR.w,y+h-1-PIC_NC_BR.h);
 }
}
// Fundo de tela navy com grao de 1 passo (couro da mesa do ministro).
void Ui_Backdrop(u16*d){
 for(int y=0;y<192;y++)for(int x=0;x<256;x++)
  d[y*256+x]=((H2p(x,y)&7)==0)?C_NAVY:C_NAVY_BG;
}
void Ui_TextC(u16*d,int cx,int y,const char*s,int big,u16 col){
 const Font*f=big?&F_SERIF12:&F_SERIF8;
 Px_Text2(d,cx-Px_Width2(s,f)/2,y,s,f,col);
}
// Divisor —◆— 48x5 centrado em cx.
void Ui_Div(u16*d,int cx,int y,u16 col){
 for(int i=0;i<20;i++){Px_PSet(d,cx-24+i,y+2,col);Px_PSet(d,cx+4+i,y+2,col);}
 for(int k=0;k<3;k++)for(int i=-k;i<=k;i++){Px_PSet(d,cx+i,y+k,col);Px_PSet(d,cx+i,y+4-k,col);}
}
// Icones em pixel art: '.' vazio, k tinta, g ouro, G brilho, d ouro velho,
// w texto claro, s texto apagado, r pincel vermelho, p pergaminho, n navy,
// v verde (papel do jogador). dim = tudo em ouro velho (desabilitado).
void Ui_Icon(u16*d,int x,int y,const char*const*rows,int h,bool dim){
 for(int r=0;r<h;r++)for(int c=0;rows[r][c];c++){
  char ch=rows[r][c];if(ch=='.')continue;
  u16 col;
  switch(ch){
   case 'k':col=C_INK;break; case 'g':col=C_GOLD;break; case 'G':col=C_GOLD_HI;break;
   case 'd':col=C_GOLD_DIM;break; case 'w':col=C_TEXT;break; case 's':col=C_TEXT_DIM;break;
   case 'r':col=C_BRUSH;break; case 'p':col=C_PARCH;break; case 'n':col=C_NAVY_BG;break;
   case 'v':col=C_PLAYER;break; default:col=C_GOLD;break;
  }
  if(dim&&ch!='k'&&ch!='n')col=C_GOLD_DIM;
  Px_PSet(d,x+c,y+r,col);
 }
}
static const char *const ICO_CROWN16[12]={
 ".......g........",
 "......gGg.......",
 ".g.....g.....g..",
 "ggg...ggg...ggg.",
 ".g...ggggg...g..",
 ".gg.ggggggg.gg..",
 ".ggggggGgggggg..",
 ".gggGgggggGggg..",
 ".gggggggggggggg.",
 ".dddddddddddddd.",
 ".gGgrgGgrgGgrgg.",
 ".gggggggggggggg.",
};
static const char *const ICO_SWORD16[16]={
 "..............ww",
 ".............wws",
 "............wws.",
 "...........wws..",
 "..........wws...",
 ".........wws....",
 "........wws.....",
 ".......wws......",
 "..gg..wws.......",
 "..ggGwws........",
 "....Gws.........",
 "...dGGgg........",
 "..ddd.gg........",
 ".ddd............",
 "GGd.............",
 "GG..............",
};
static const char *const ICO_SWORDS16[16]={
 "w..............w",
 "sw............ws",
 ".sw..........ws.",
 "..sw........ws..",
 "...sw......ws...",
 "....sw....ws....",
 ".....sw..ws.....",
 "......swws......",
 "......wsws......",
 ".....ws..sw.....",
 "..g.ws....sw.g..",
 "...gs......sg...",
 "...dg......gd...",
 "..d..g....g..d..",
 ".G............G.",
 "................",
};
static const char *const ICO_PEOPLE16[14]={
 "................",
 "...ww......ww...",
 "..wwww....wwww..",
 "..wwww....wwww..",
 "...ww......ww...",
 "................",
 ".wwwwww..wwwwww.",
 "wwwwwwww.wwwwwww",
 "wwwwwwww.wwwwwww",
 "wwwwwwww.wwwwwww",
 "ssssssss.sssssss",
 "................",
 "...gggggggggg...",
 "................",
};
static const char *const ICO_FLAG16[16]={
 "g...............",
 "gwwwwwww........",
 "gwwwwwwwwww.....",
 "gwwwwwwwwwwwww..",
 "gwwwwwwwwwwwwww.",
 "gwwwwwwwwwwwww..",
 "gwwwwwwwwwss....",
 "gwwwwwss........",
 "gss.............",
 "g...............",
 "g...............",
 "g...............",
 "g...............",
 "g...............",
 "G...............",
 "................",
};
static const char *const ICO_SHIELD16[16]={
 "..gggggggggggg..",
 "..grrrrrrrrrrg..",
 "..grrrrGGrrrrg..",
 "..grrrrGGrrrrg..",
 "..grrGGGGGGrrg..",
 "..grrGGGGGGrrg..",
 "..grrrrGGrrrrg..",
 "..grrrrGGrrrrg..",
 "..grrrrrrrrrrg..",
 "...grrrrrrrrg...",
 "....grrrrrrg....",
 ".....grrrrg.....",
 "......grrg......",
 ".......gg.......",
 "................",
 "................",
};
static const char *const ICO_GLOBE16[16]={
 "......gggg......",
 "....gg.gg.gg....",
 "...g..g..g..g...",
 "..g..g....g..g..",
 ".g..g......g..g.",
 ".g..g......g..g.",
 "gggggggggggggggg",
 "g...g......g...g",
 "g...g......g...g",
 "gggggggggggggggg",
 ".g..g......g..g.",
 ".g..g......g..g.",
 "..g..g....g..g..",
 "...g..g..g..g...",
 "....gg.gg.gg....",
 "......gggg......",
};
// barra de recursos (12x12)
static const char *const ICO_COIN12[12]={
 "....kkkk....",
 "..kkGGggkk..",
 ".kGGggggddk.",
 ".kGggggggdk.",
 "kGggddddggdk",
 "kGggdggggddk",
 "kGgggddgggdk",
 "kGgggggdggdk",
 ".kggddddgddk",
 ".kgggggggdk.",
 "..kkddddkk..",
 "....kkkk....",
};
static const char *const ICO_SWORDS12[12]={
 "w..........w",
 "sw........ws",
 ".sw......ws.",
 "..sw....ws..",
 "...sw..ws...",
 "....swws....",
 "....wsws....",
 "...ws..sw...",
 ".g.s....s.g.",
 "..gd....dg..",
 ".d..g..g..d.",
 "G..........G",
};
static const char *const ICO_CROWN12[12]={
 "............",
 "g....g....g.",
 "gg..ggg..gg.",
 "ggg.ggg.ggg.",
 "gGggggggGgg.",
 "ggggGgGgggg.",
 "ggggggggggg.",
 "ddddddddddd.",
 "gGgrgGgrgGg.",
 "ggggggggggg.",
 "............",
 "............",
};
static const char *const ICO_CAL12[12]={
 "..g.....g...",
 "gggggggggggg",
 "grrrrrrrrrrg",
 "gggggggggggg",
 "gppppppppppg",
 "gpkpkpkpkppg",
 "gppppppppppg",
 "gpkpkpkpkppg",
 "gppppppppppg",
 "gpkpkppppppg",
 "gppppppppppg",
 "gggggggggggg",
};
static const char *const ICO_LAND12[12]={
 "g...........",
 "grrrrr......",
 "grrrrrrr....",
 "grrrrrrrrr..",
 "grrrrrrr....",
 "grrrrr......",
 "g...........",
 "g...........",
 "g...........",
 "g...........",
 "g...........",
 "G...........",
};
static const char *const ICO_HOUSE16[16]={
 ".......g........",
 "......ggg....gg.",
 ".....gGGGg...gg.",
 "....gGrrrGg..gg.",
 "...gGrrrrrGg.gg.",
 "..gGrrrrrrrGggg.",
 ".gGrrrrrrrrrGg..",
 "gggggggggggggggg",
 ".gwwwwwwwwwwwwg.",
 ".gwkkwwwwwwkkwg.",
 ".gwkkwwddwwkkwg.",
 ".gwwwwwddwwwwwg.",
 ".gwwwwwddwwwwwg.",
 ".gwwwwwddwwwwwg.",
 ".gggggggggggggg.",
 "................",
};
static const char *const ICO_BOOK16[16]={
 "................",
 ".gggggg..gggggg.",
 "gppppppggppppppg",
 "gpkkkkpggpkkkkpg",
 "gppppppggppppppg",
 "gpkkkppggpkkkppg",
 "gppppppggppppppg",
 "gpkkkkpggpkkkkpg",
 "gppppppggppppppg",
 "gpkkppprgpkkkppg",
 "gppppppggppppppg",
 "gppppppggppppppg",
 ".ggggggggggggggg",
 "......rr........",
 "......rr........",
 "......r.r.......",
};
static const char *const ICO_SHIP16[16]={
 ".......g........",
 ".......gr.......",
 "......wgw.......",
 ".....wwgww......",
 "....wwwgwww.....",
 "...wwwwgwwww....",
 "..wwwwwgwwwww...",
 ".......g........",
 "gggggggggggggggg",
 ".gGGGGGGGGGGGGg.",
 "..gggggggggggg..",
 "...gggggggggg...",
 "................",
 ".dd..dd..dd..dd.",
 "d..dd..dd..dd..d",
 "................",
};
void Ui_IconId(u16*d,int x,int y,u8 id,bool dim){
 switch(id){
  case ICO_CROWN:  Ui_Icon(d,x,y,ICO_CROWN16,12,dim);break;
  case ICO_SWORD:  Ui_Icon(d,x,y,ICO_SWORD16,16,dim);break;
  case ICO_SWORDS: Ui_Icon(d,x,y,ICO_SWORDS16,16,dim);break;
  case ICO_PEOPLE: Ui_Icon(d,x,y,ICO_PEOPLE16,14,dim);break;
  case ICO_FLAG:   Ui_Icon(d,x,y,ICO_FLAG16,16,dim);break;
  case ICO_SHIELD: Ui_Icon(d,x,y,ICO_SHIELD16,16,dim);break;
  case ICO_GLOBE:  Ui_Icon(d,x,y,ICO_GLOBE16,16,dim);break;
  case ICO_COIN_S: Ui_Icon(d,x,y,ICO_COIN12,12,dim);break;
  case ICO_SWORDS_S:Ui_Icon(d,x,y,ICO_SWORDS12,12,dim);break;
  case ICO_CROWN_S:Ui_Icon(d,x,y,ICO_CROWN12,12,dim);break;
  case ICO_CAL_S:  Ui_Icon(d,x,y,ICO_CAL12,12,dim);break;
  case ICO_LAND_S: Ui_Icon(d,x,y,ICO_LAND12,12,dim);break;
  case ICO_HOUSE:  Ui_Icon(d,x,y,ICO_HOUSE16,16,dim);break;
  case ICO_BOOK:   Ui_Icon(d,x,y,ICO_BOOK16,16,dim);break;
  case ICO_SHIP:   Ui_Icon(d,x,y,ICO_SHIP16,16,dim);break;
  default:break;
 }
}
// Coroa de rodape (16x12), no maximo 1 por painel (DESIGN 3.2).
void Ui_Crown(u16*d,int x,int y){Ui_Icon(d,x,y,ICO_CROWN16,12,false);}
// Seta ▶ desenhada (4x7), cor livre: branca na selecao. TriL = ◂.
static void Tri(u16*d,int x,int y,u16 c){
 for(int k=0;k<4;k++)for(int j=k;j<7-k;j++)Px_PSet(d,x+k,y+j,c);
}
static void TriL(u16*d,int x,int y,u16 c){
 for(int k=0;k<4;k++)for(int j=k;j<7-k;j++)Px_PSet(d,x+3-k,y+j,c);
}
void Ui_Arrows(u16*d,int xl,int xr,int y,u16 c){TriL(d,xl,y,c);Tri(d,xr,y,c);}
// Faixa crimson da concept: degrade vertical, brilho no topo, borda ouro e
// pontas em "fita" (recorte em V na direita). Compartilhada por Item/BigBtn.
static void Ribbon(u16*d,int x,int y,int w,int h,bool pulse){
 for(int j=0;j<h;j++){
  int t=j*8/h;u16 c=PXC(21-t,4-t/4,4-t/4);
  int cut=j<h/2?j:h-1-j;cut=(h/2-cut)/3;   // V raso na ponta direita
  Px_Rect(d,x,y+j,w-cut,1,c);
 }
 if(pulse)Px_Rect(d,x+1,y+1,w-4,1,C_CRIMSON_HI);
 Px_Rect(d,x,y,w-2,1,C_GOLD);Px_Rect(d,x,y+h-1,w-2,1,C_GOLD);Px_Rect(d,x,y,1,h,C_GOLD);
 for(int j=0;j<h;j++){int cut=j<h/2?j:h-1-j;cut=(h/2-cut)/3;Px_PSet(d,x+w-1-cut,y+j,C_GOLD);}
}
// Botao navy da concept: degrade, borda ouro com cantos chanfrados.
static void NavyBtn(u16*d,int x,int y,int w,int h,bool hi){
 for(int j=0;j<h;j++){int t=j*6/h;Px_Rect(d,x+1,y+j,w-2,1,hi?PXC(7-t/2,9-t/2,13-t/2):PXC(5-t/2,6-t/2,9-t/2));}
 Px_Rect(d,x+1,y,w-2,1,C_GOLD);Px_Rect(d,x+1,y+h-1,w-2,1,C_GOLD);
 Px_Rect(d,x,y+1,1,h-2,C_GOLD);Px_Rect(d,x+w-1,y+1,1,h-2,C_GOLD);
 Px_Rect(d,x+2,y+2,w-4,1,C_GOLD_DIM);
}
// Item de lista sobre navy (h=24): selecionado = fita crimson + ▶ ouro;
// pulse liga/desliga o brilho (SDD 14.1). star = ✦ nos nao selecionados.
void Ui_Item(u16*d,int x,int y,int w,const char*s,bool sel,bool dim,bool pulse,bool star){
 if(sel){
  Ribbon(d,x,y,w,24,pulse);
  Tri(d,x+6,y+8,C_GOLD_HI);
 }else if(star){
  Px_Text2(d,x+5,y+7,"✦",&F_SERIF8,C_GOLD);
 }
 u16 col=dim?C_TEXT_DIM:C_TEXT;
 Px_Text2(d,x+19,y+7,s,&F_SERIF12,PXC(1,1,2));
 Px_Text2(d,x+18,y+6,s,&F_SERIF12,col);
}
// Botao grande (Selecao de Modo): 176x26, icone 16x16 a x+10, texto a x+44.
void Ui_BigBtn(u16*d,int x,int y,const char*s,u8 icon,bool sel,bool dim,bool pulse){
 if(sel)Ribbon(d,x,y,176,26,pulse);else NavyBtn(d,x,y,176,26,false);
 Ui_IconId(d,x+10,y+5,icon,dim);
 Px_Text2(d,x+45,y+8,s,&F_SERIF12,PXC(1,1,2));
 Px_Text2(d,x+44,y+7,s,&F_SERIF12,dim?C_TEXT_DIM:C_TEXT);
}
// Botao icone do HUD: 34x34; pressionado afunda 1px; desabilitado em ouro velho.
void Ui_IconBtn(u16*d,int x,int y,u8 icon,bool pressed,bool dim){
 NavyBtn(d,x,y,34,34,pressed);
 if(dim){Px_Rect(d,x+1,y,32,1,C_GOLD_DIM);Px_Rect(d,x+1,y+33,32,1,C_GOLD_DIM);Px_Rect(d,x,y+1,1,32,C_GOLD_DIM);Px_Rect(d,x+33,y+1,1,32,C_GOLD_DIM);}
 Ui_IconId(d,x+9,y+9+(pressed?1:0),icon,dim);
}
// Linha "rotulo  ◂ valor ▸" (Configuracoes/Preparar): rotulo Serifa 8 em
// x+12, ◂ em x+76, ▸ em x+w-20, valor centrado entre eles.
void Ui_ValueRow(u16*d,int x,int y,int w,const char*lab,const char*val,bool sel){
 if(sel)Tri(d,x+2,y+1,C_CRIMSON);
 Px_Text2(d,x+12,y,lab,&F_SERIF8,sel?C_CRIMSON:C_INK);
 if(!val)return;
 int a=x+76,b=x+w-20;
 Ui_Arrows(d,a,b,y,C_INK);
 int cx=(a+b+6)/2;
 Px_Text2(d,cx-Px_Width2(val,&F_SERIF8)/2,y,val,&F_SERIF8,C_INK);
}
// Slider 0..100 (Musica/Efeitos): trilho 112x3 + puxador 6x9 ouro.
void Ui_Slider(u16*d,int x,int y,int v){
 Px_Rect(d,x,y,112,3,C_NAVY);
 Px_Rect(d,x,y,112*v/100,3,C_INK);
 int k=x+(106*v)/100;
 Px_Rect(d,k,y-3,6,9,C_GOLD);
 Px_Border(d,k,y-3,6,9,C_INK);
}
// Barra fina de progresso (tropas/teto) sobre navy.
void Ui_Bar(u16*d,int x,int y,int w,int pct,u16 col){
 if(pct<0)pct=0;
 if(pct>100)pct=100;
 Px_Rect(d,x,y,w,3,C_NAVY_BG);
 Px_Rect(d,x,y,w*pct/100,3,col);
}
