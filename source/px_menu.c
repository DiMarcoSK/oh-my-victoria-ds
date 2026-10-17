#include "menu.h"
#include "wgame.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "settings.h"
#include "flag.h"
#include "traits.h"
#include "px_art.h"
// px_menu.c — telas da v0.9 (SDD sec 14.4/14.5) com os componentes de
// px_ui.c. As pinturas (titulo, oficial, cidade, porto, brasao, gravura) sao
// recortadas da propria concept art e reamostradas p/ o pixel nativo
// (tools/genpics.py -> source/gen/pic_data.c, ADR-29); o texto e' sempre do
// jogo (str.c), nunca o da pintura. Rosa dos ventos segue procedural.

static inline int Cl(int v){return v<0?0:(v>31?31:v);}
static int Hh(int x,int y){
 unsigned h=((unsigned)(x*73856093u))^((unsigned)(y*19349663u));
 h^=h>>13;h*=0x9E37u;h^=h>>11;
 return (int)(h&0x7FFFFFFFu);
}
static bool Pulse(void){return (Menu_Ptr()->tick/30)&1;}

// ---- primitivas de gravura ----
static void FillTri(u16 *d,int x0,int y0,int x1,int y1,int x2,int y2,u16 c){
 int minx=x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2),maxx=x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2);
 int miny=y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2),maxy=y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2);
 int area=(x1-x0)*(y2-y0)-(x2-x0)*(y1-y0);
 if(!area)return;
 for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){
  int w0=(x1-x)*(y2-y)-(x2-x)*(y1-y),w1=(x2-x)*(y0-y)-(x0-x)*(y2-y),w2=(x0-x)*(y1-y)-(x1-x)*(y0-y);
  if(area>0?(w0>=0&&w1>=0&&w2>=0):(w0<=0&&w1<=0&&w2<=0))Px_PSet(d,x,y,c);
 }
}
static void Ring(u16 *d,int cx,int cy,int r,u16 c,int dotted){
 // circulo inteiro (ponto medio)
 int x=r,y=0,e=1-r,k=0;
 while(x>=y){
  if(!dotted||((k++)&1)){
   Px_PSet(d,cx+x,cy+y,c);Px_PSet(d,cx-x,cy+y,c);Px_PSet(d,cx+x,cy-y,c);Px_PSet(d,cx-x,cy-y,c);
   Px_PSet(d,cx+y,cy+x,c);Px_PSet(d,cx-y,cy+x,c);Px_PSet(d,cx+y,cy-x,c);Px_PSet(d,cx-y,cy-x,c);
  }
  y++;if(e<0)e+=2*y+1;else{x--;e+=2*(y-x)+1;}
 }
}
// rosa dos ventos de chancelaria: 4 pontas longas (tinta/ouro) + 4 curtas
static void Compass(u16 *d,int cx,int cy,int R){
 Ring(d,cx,cy,R+3,C_INK,1);
 Ring(d,cx,cy,R*3/4,C_GOLD_DIM,0);
 int r2=R*45/100,w2=R/8;
 static const s8 DG[4][2]={{1,-1},{1,1},{-1,1},{-1,-1}};
 for(int i=0;i<4;i++){
  int tx=cx+DG[i][0]*r2,ty=cy+DG[i][1]*r2;
  int px=-DG[i][1]*w2/2,py=DG[i][0]*w2/2;
  FillTri(d,cx,cy,tx,ty,cx+px+DG[i][0],cy+py+DG[i][1],C_GOLD_DIM);
  FillTri(d,cx,cy,tx,ty,cx-px+DG[i][0],cy-py+DG[i][1],C_PARCH_SH);
 }
 static const s8 DM[4][2]={{0,-1},{1,0},{0,1},{-1,0}};
 int w=R/6;
 for(int i=0;i<4;i++){
  int tx=cx+DM[i][0]*R,ty=cy+DM[i][1]*R;
  int px=-DM[i][1]*w,py=DM[i][0]*w;
  FillTri(d,cx,cy,tx,ty,cx+px,cy+py,C_INK);
  FillTri(d,cx,cy,tx,ty,cx-px,cy-py,C_GOLD);
 }
 Px_Rect(d,cx-1,cy-1,3,3,C_GOLD_HI);
 Ui_TextC(d,cx,cy-R-16,"N",0,C_INK);
}
static void Sailboat(u16 *d,int x,int y,u16 c,bool engr){
 // casco + mastro + 2 velas
 for(int i=0;i<22;i++)Px_PSet(d,x+i,y,c);
 for(int i=2;i<20;i++)Px_PSet(d,x+i,y+1,c);
 for(int i=4;i<18;i++)Px_PSet(d,x+i,y+2,c);
 for(int j=-24;j<0;j++)Px_PSet(d,x+11,y+j,c);
 if(engr){
  for(int j=0;j<18;j++){Px_PSet(d,x+10-j*8/18,y-22+j,c);}
  for(int k=0;k<9;k++)Px_PSet(d,x+2+k,y-4,c);
  for(int j=0;j<14;j++)Px_PSet(d,x+12+j*7/14,y-18+j,c);
  for(int k=0;k<7;k++)Px_PSet(d,x+12+k,y-4,c);
 }else{
  FillTri(d,x+10,y-22,x+10,y-3,x+2,y-3,c);
  FillTri(d,x+12,y-18,x+12,y-3,x+19,y-3,c);
 }
}

// ---- tela titulo (SCR_MAIN, cima): a pintura da concept (Westminster ao
// por do sol + logo), tools/genpics.py. O botao vem pintado sem texto: o
// "Aperte A" e' escrito aqui no idioma escolhido e pulsa do ouro ao creme.
static void Title(u16 *d){
 Pic_Blit(d,&PIC_TITLE,0,0);
 const char *s=Str(STR_APERTE_A);
 int w=Px_Width2(s,&F_SERIF12);
 bool on=(Menu_Ptr()->tick%60)<40;
 Px_Text2(d,214-w/2+1,170,s,&F_SERIF12,PXC(1,1,2));
 Px_Text2(d,214-w/2,169,s,&F_SERIF12,on?C_TEXT:C_GOLD);
}

// ---- cada tela ----
static void MainBot(u16 *d){
 Menu *m=Menu_Ptr();
 // moldura navy da tela inteira + retrato do oficial (concept, menu principal)
 Ui_NavyOrn(d,0,0,256,192);
 Pic_Blit(d,&PIC_OFFICER,9,22);
 Px_Border(d,8,21,PIC_OFFICER.w+2,PIC_OFFICER.h+2,C_GOLD_DIM);
 Ui_TextC(d,9+PIC_OFFICER.w/2,160,"1800 · 2025",0,C_GOLD);
 Ui_NavyOrn(d,86,14,162,150);
 static const u16 IT[4]={STR_NOVO_JOGO,STR_CARREGAR,STR_OPCOES,STR_CREDITOS};
 for(int i=0;i<4;i++){
  Ui_Item(d,92,24+i*34,150,Str(IT[i]),m->sel==i,i==1,Pulse(),false);
  if(i<3&&m->sel!=i&&m->sel!=i+1)Px_Rect(d,100,24+i*34+29,136,1,C_GOLD_DIM);
 }
 Pic_Blit(d,&PIC_CROWN,128-PIC_CROWN.w/2+39,170);
}
static void ModeTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 Ui_TextC(d,128,16,Str(STR_ESCOLHA_MODO),1,C_INK);
 Ui_Div(d,128,32,C_INK);
 static const u16 IT[3]={STR_CAMPANHA,STR_CONTRA_IA,STR_MULTIJOGADOR};
 static const u8 IC[3]={ICO_CROWN,ICO_SWORDS,ICO_PEOPLE};
 for(int i=0;i<3;i++)Ui_BigBtn(d,40,46+i*36,Str(IT[i]),IC[i],m->sel==i,i==2,Pulse());
}
static void ModeBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 // gravura colorida da cidade em moldura de tinta (concept, selecao de modo)
 int px=18,py=(192-PIC_TOWN.h)/2;
 Px_Rect(d,px-2,py-2,PIC_TOWN.w+4,PIC_TOWN.h+4,PXC(9,7,5));
 Pic_Blit(d,&PIC_TOWN,px,py);
 static const u16 DS[3]={STR_DESC_CAMPANHA,STR_DESC_IA,STR_DESC_MULTI};
 int n=Px_WrapN(d,116,58,124,Str(DS[m->sel]),&F_SERIF12,C_INK,5,false);
 if(m->sel==2)Px_Text2(d,116,62+n*14,Str(STR_EM_BREVE),&F_SERIF8,C_GOLD_DIM);
}
static const char *MapName(u8 k){
 static const u16 S[MAPK_COUNT]={STR_MAPA_MUNDI,STR_ALEATORIO,STR_BLITZ,STR_CLASSICO};
 return Str(S[k%MAPK_COUNT]);
}
static void Hex5(u32 v,char *o){
 static const char *HX="0123456789ABCDEF";
 for(int i=0;i<5;i++)o[i]=HX[(v>>((4-i)*4))&15];
 o[5]=0;
}
static void SetupTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 Ui_TextC(d,128,16,Str(STR_PREPARAR),1,C_INK);
 Ui_Div(d,128,32,C_INK);
 u8 rows[8];int n=Menu_SetupRows(rows);
 static const u16 HS[3]={STR_LIGADA,STR_FLAVOR,STR_DESLIGADA};
 static const u16 SZ[3]={STR_PEQUENO,STR_MEDIO,STR_GRANDE};
 static const u16 DF[3]={STR_FACIL,STR_NORMAL,STR_DIFICIL};
 static const char *const NN[3]={"8","16","32"};
 int y=44;
 for(int i=0;i<n;i++){
  bool sel=(m->sel==i);
  char hx[8];
  switch(rows[i]){
   case ROW_MAP: Ui_ValueRow(d,14,y,228,Str(STR_MAPA),MapName(m->map),sel);break;
   case ROW_HIST:Ui_ValueRow(d,14,y,228,Str(STR_HISTORIA),Str(HS[m->hist%3]),sel);break;
   case ROW_SIZE:Ui_ValueRow(d,14,y,228,Str(STR_TAMANHO),Str(SZ[m->size%3]),sel);break;
   case ROW_NNAT:Ui_ValueRow(d,14,y,228,Str(STR_NACOES),NN[m->nNat%3],sel);break;
   case ROW_DIFF:Ui_ValueRow(d,14,y,228,Str(STR_DIFICULDADE),Str(DF[m->diff%3]),sel);break;
   case ROW_SEED:Hex5(m->seed,hx);Ui_ValueRow(d,14,y,228,Str(STR_SEMENTE),hx,sel);break;
   case ROW_GO:
    Ui_BigBtn(d,40,150,Str(m->map==MAPK_CLASSICO?STR_NOVO_JOGO:STR_ESCOLHER_NACAO),ICO_FLAG,sel,false,Pulse());
    break;
  }
  if(rows[i]!=ROW_GO)y+=18;
 }
}
static void SetupBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 if(m->map==MAPK_MUNDI)PxW_Earth(d,16,18,224,84,-1,0);
 else{Compass(d,128,60,32);}
 static const u16 DS[MAPK_COUNT]={STR_DESC_MUNDI,STR_DESC_ALEAT,STR_DESC_BLITZ,STR_DESC_CLASSICO};
 Ui_Div(d,128,110,C_INK);
 Px_WrapN(d,18,122,220,Str(DS[m->map%MAPK_COUNT]),&F_SERIF8,C_INK,5,true);
}
static void NationTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 Ui_TextC(d,128,15,Str(STR_ESCOLHA_NACAO),1,C_INK);
 // filtro de continente: ◂ Europa ▸
 const char *cn=m->cont?Nat_ContName((u8)(m->cont-1)):Str(STR_TODOS);
 int wc=Px_Width2(cn,&F_SERIF8);
 Ui_Arrows(d,128-wc/2-12,128+wc/2+8,31,C_INK);
 Px_Text2(d,128-wc/2,31,cn,&F_SERIF8,C_INK);
 int n=Menu_NatCount(),k=Menu_NatIndex(m->nat);
 if(k<0)k=0;
 // 5 estandartes, o selecionado no centro (sobe 3px + contorno ouro)
 for(int s=-2;s<=2;s++){
  if(n<5&&(s<-k||s>=n-k))continue;
  int idx=((k+s)%n+n)%n,nat=Menu_NatAt(idx);
  if(nat<0)continue;
  int x=16+(s+2)*46,y=s==0?43:46;
  Flag_Banner(d,256,x,y,40,58,&NATIONS[nat].flag);
  if(s==0){Px_Border(d,x-3,y-3,46,64,C_GOLD);}
 }
 if(n>5)Ui_Arrows(d,12,240,70,C_INK);
 if(m->nat<0)return;
 const NationDef *nd=&NATIONS[m->nat];
 int wn=Px_Width2(nd->name,&F_SERIF12);
 Px_Text2(d,128-wn/2,116,nd->name,&F_SERIF12,C_INK);
 Px_Text2(d,128-wn/2-14,118,"✦",&F_SERIF8,C_GOLD_DIM);
 Px_Text2(d,128+wn/2+6,118,"✦",&F_SERIF8,C_GOLD_DIM);
 Px_Rect(d,40,131,176,1,C_PARCH_SH);
 if(nd->desc)Px_WrapN(d,22,136,212,nd->desc,&F_SERIF8,C_INK,3,true);
}
// repinta o estandarte do PIC_HARBOR: cada pixel de pano pega a cor da
// bandeira em (u,v) e a sombra da pintura (HARBOR_FLAG: 12 = luz normal)
static void HarborFlag(u16 *d,const FlagDef *f){
 static u16 tmp[64*48];
 int bx=HARBOR_FLAG_BOX[0],by=HARBOR_FLAG_BOX[1],bw=HARBOR_FLAG_BOX[2],bh=HARBOR_FLAG_BOX[3];
 int fh=bh-12;                       // altura util do pano (a barra de baixo some atras do soldado)
 Flag_Draw(tmp,64,0,0,bw,fh,f);
 for(int y=0;y<bh;y++)for(int x=0;x<bw;x++){
  int k=HARBOR_FLAG[y*bw+x];if(!k)continue;
  int t=HARBOR_FLAG_TOP[x],v=(y-t)*fh/(bh-t>1?bh-t:1);
  if(v<0)v=0;
  if(v>=fh)v=fh-1;
  u16 c=tmp[v*64+x];
  int r=(c&31)*k/12,g=((c>>5)&31)*k/12,b=((c>>10)&31)*k/12;
  d[(by+y)*256+bx+x]=PXC(Cl(r),Cl(g),Cl(b));
 }
}
static void NationBot(u16 *d){
 Menu *m=Menu_Ptr();
 // pintura do porto com o estandarte (concept, selecao de nacao) + ficha
 Ui_Navy(d,0,0,256,192);
 Pic_Blit(d,&PIC_HARBOR,0,0);
 Px_Rect(d,0,PIC_HARBOR.h,256,1,C_GOLD);
 Px_Border(d,0,0,256,192,C_GOLD);
 if(m->nat<0)return;
 const NationDef *nd=&NATIONS[m->nat];
 // o pano pintado vira a bandeira da nacao escolhida, com as dobras da pintura
 HarborFlag(d,&nd->flag);
 // ficha: governo · continente (+ endonimo) e os traits
 char t[96];int n=0;
 const char *a=Nat_GovName(nd->gov);while(*a&&n<90)t[n++]=*a++;
 const char *sep=" · ";while(*sep&&n<90)t[n++]=*sep++;
 a=Nat_ContName(nd->cont);while(*a&&n<90)t[n++]=*a++;
 if(nd->endo){sep=" · ";while(*sep&&n<90)t[n++]=*sep++;a=nd->endo;while(*a&&n<94)t[n++]=*a++;}
 t[n]=0;
 int y=PIC_HARBOR.h+4;
 Px_WrapN(d,8,y,240,t,&F_SERIF8,C_GOLD,1,false);
 y+=11;
 for(int i=0;i<NAT_TRAITS&&y<186;i++){
  u8 tr=nd->traits[i];if(!tr)continue;
  Px_Text2(d,8,y,Tr_Name(tr),&F_SERIF8,C_TEXT);
  int wn=Px_Width2(Tr_Name(tr),&F_SERIF8);
  Px_WrapN(d,8+wn+6,y,240-wn-6,Tr_Desc(tr),&F_SERIF8,C_TEXT_DIM,1,false);
  y+=11;
 }
}
static void Dim(u16 *d){
 for(int i=0;i<256*192;i++)d[i]=(u16)(((d[i]>>1)&0x3DEF)|BIT(15));
}
static void PauseTop(u16 *d){
 Menu *m=Menu_Ptr();
 PxW_Map(d);Dim(d);
 Ui_NavyOrn(d,64,30,140,126);
 static const u16 IT[4]={STR_CONTINUAR,STR_SALVAR,STR_CONFIG,STR_SAIR_MENU};
 for(int i=0;i<4;i++)Ui_Item(d,68,40+i*26,132,Str(IT[i]),m->sel==i,i==1,Pulse(),true);
 Ui_Crown(d,126,140);
}
static void PauseBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 static const u16 DS[4]={STR_DESC_CONTINUAR,STR_DESC_SALVAR,STR_DESC_CONFIG,STR_DESC_SAIR};
 const char *s=Str(DS[m->sel&3]);
 if(m->sel==1)s=Str(STR_EM_BREVE);
 if(m->sel==3&&m->confirm)s=Str(STR_CONFIRMA_SAIR);
 Px_WrapN(d,24,26,208,s,&F_SERIF12,C_INK,3,true);
 Ui_Div(d,128,74,C_INK);
 // gravura do porto em sepia (concept, pausa)
 Pic_Blit(d,&PIC_SKETCH,(256-PIC_SKETCH.w)/2,192-PIC_SKETCH.h-12);
}
static void OptionsTop(u16 *d){
 Menu *m=Menu_Ptr();Settings *s=Set_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 Ui_TextC(d,128,16,Str(STR_CONFIG),1,C_INK);
 Ui_Div(d,128,32,C_INK);
 static const u16 LB[4]={STR_MUSICA,STR_EFEITOS,STR_IDIOMA,STR_TELA};
 for(int i=0;i<4;i++){
  int y=46+i*30;bool sel=m->sel==i;
  char pc[8];
  if(i<2){
   int v=i==0?s->music:s->sfx;
   Ui_ValueRow(d,10,y,236,Str(LB[i]),0,sel);
   Ui_Slider(d,82,y+4,v);
   Px_Itoa(v,pc);int n=0;while(pc[n])n++;pc[n++]='%';pc[n]=0;
   Px_Text2(d,234-Px_Width2(pc,&F_SERIF8),y,pc,&F_SERIF8,C_INK);
  }else if(i==2)Ui_ValueRow(d,10,y,236,Str(LB[i]),Str(s->lang==LANG_EN?STR_ENGLISH:STR_PORTUGUES),sel);
  else Ui_ValueRow(d,10,y,236,Str(LB[i]),Str(s->layout==LAYOUT_SWAP?STR_LAYOUT_INVERTIDA:STR_LAYOUT_NORMAL),sel);
 }
}
static void OptionsBot(u16 *d){
 Ui_ParchOrn(d,0,0,256,192);
 // brasao com leoes (concept, configuracoes)
 Pic_Blit(d,&PIC_CREST,(256-PIC_CREST.w)/2,22);
 Px_WrapN(d,30,122,196,Str(STR_AJUSTE),&F_SERIF12,C_INK,2,true);
}
static void CreditsTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_ParchOrn(d,0,0,256,192);
 static const u16 L[5]={STR_CRED1,STR_CRED2,STR_CRED3,STR_CRED4,STR_CRED5};
 int y=184-(m->credY%260);
 for(int i=0;i<5;i++){
  int yy=y+i*34;
  if(yy>14&&yy<170){
   Ui_TextC(d,128,yy,Str(L[i]),i==0,C_INK);
   if(i==0)Ui_Div(d,128,yy+16,C_INK);
  }
 }
}
static void CreditsBot(u16 *d){
 Ui_Backdrop(d);
 Px_Border(d,2,2,252,188,C_GOLD_DIM);
 for(int y=150;y<160;y+=3)for(int x=40;x<216;x+=5)if(Hh(x,y)%3)Px_Rect(d,x,y,3,1,C_GOLD_DIM);
 Sailboat(d,112,146,C_GOLD_DIM,true);
 Px_WrapN(d,30,60,196,Str(STR_FRASE),&F_SERIF8,C_TEXT,3,true);
}

void Menu_DrawTop(GameState *g,u16 *d){
 switch(g->screen){
  case SCR_MAIN:   Title(d);break;
  case SCR_MODE:   ModeTop(d);break;
  case SCR_SETUP:  SetupTop(d);break;
  case SCR_NATION: NationTop(d);break;
  case SCR_WORLD:  PxW_Map(d);break;
  case SCR_CITY:   PxC_Scene(d,WG_Ptr()->cityIdx);break;
  case SCR_LEDGER: PxL_Top(d);break;
  case SCR_RANK:   PxR_Top(d);break;
  case SCR_PAUSE:  PauseTop(d);break;
  case SCR_OPTIONS:OptionsTop(d);break;
  case SCR_CREDITS:CreditsTop(d);break;
  default:         Ui_Backdrop(d);break;
 }
}
void Menu_DrawBot(GameState *g,u16 *d){
 switch(g->screen){
  case SCR_MAIN:   MainBot(d);break;
  case SCR_MODE:   ModeBot(d);break;
  case SCR_SETUP:  SetupBot(d);break;
  case SCR_NATION: NationBot(d);break;
  case SCR_WORLD:  PxW_Panel(d);break;
  case SCR_CITY:   {WGame *s=WG_Ptr();PxC_Panel(d,s->cityIdx,s->cityLot,s->cityPick,s->pickK);}break;
  case SCR_LEDGER: PxL_Bot(d);break;
  case SCR_RANK:   PxR_Bot(d);break;
  case SCR_PAUSE:  PauseBot(d);break;
  case SCR_OPTIONS:OptionsBot(d);break;
  case SCR_CREDITS:CreditsBot(d);break;
  default:         Ui_Backdrop(d);break;
 }
}
void Menu_DrawLoading(GameState *g){
 (void)g;
 u16 *t=Px_BackTop(),*b=Px_BackBot();
 Ui_Backdrop(t);Ui_Parch(t,48,70,160,52);
 Ui_TextC(t,128,84,Str(STR_PREPARANDO),1,C_INK);
 Ui_Div(t,128,102,C_INK);
 Ui_Backdrop(b);
 Ui_Parch(b,8,8,240,176);
 Compass(b,128,96,40);
 Px_FlipTop();Px_FlipBot();
}
