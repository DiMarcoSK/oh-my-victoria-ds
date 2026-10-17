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
// px_ui.c. A arte que ainda nao existe em PNG (ADR-14) e' desenhada aqui de
// forma procedural e sobria (gravura/silhueta), no lugar do placeholder:
// por do sol de Westminster (titulo), rosa dos ventos, gravura do porto e
// brasao. A F4 pode trocar cada uma por PNG sem mexer no layout.

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
// silhueta de Westminster (Parlamento, Torre Victoria, Big Ben) sobre a
// linha de base (x0,base). engr=true: gravura (contorno de tinta + hachura)
typedef struct { s16 x,y,w,h; } RectS;
static const RectS WM[]={
 {0,-20,186,20},   // corpo do Parlamento
 {12,-58,15,58},   // Torre Victoria
 {88,-34,9,14},    // torre central
 {196,-64,13,64},  // Torre do Relogio (Big Ben)
 {197,-74,11,10},  // campanario
};
static void Westminster(u16 *d,int x0,int base,u16 fill,u16 lit,bool engr){
 for(unsigned i=0;i<sizeof WM/sizeof WM[0];i++){
  const RectS *r=&WM[i];
  int x=x0+r->x,y=base+r->y;
  if(engr){
   for(int j=0;j<r->h;j++)for(int k=0;k<r->w;k++)if(((k+x)&1)==0&&((j+y)%3)!=0)Px_PSet(d,x+k,y+j,C_PARCH_SH);
   Px_Border(d,x,y,r->w,r->h,C_INK);
  }else Px_Rect(d,x,y,r->w,r->h,fill);
 }
 u16 e=engr?C_INK:fill;
 // ameias do Parlamento e pinaculos
 for(int x=x0+2;x<x0+186;x+=6){Px_Rect(d,x,base-23,2,3,e);}
 for(int k=0;k<4;k++)Px_Rect(d,x0+12+k*4,base-62,2,4,e);
 FillTri(d,x0+87,base-34,x0+98,base-34,x0+92,base-46,e);       // agulha central
 FillTri(d,x0+195,base-74,x0+209,base-74,x0+202,base-96,e);    // agulha do Big Ben
 // relogio
 Px_Rect(d,x0+198,base-61,9,9,engr?C_PARCH:lit);
 Px_Border(d,x0+198,base-61,9,9,engr?C_INK:fill);
 Px_PSet(d,x0+202,base-57,C_INK);Px_PSet(d,x0+202,base-58,C_INK);Px_PSet(d,x0+203,base-57,C_INK);
 if(!engr)for(int y=base-17;y<base-2;y+=4)for(int x=x0+3;x<x0+184;x+=5)
  if(Hh(x,y)%3==0)Px_Rect(d,x,y,2,2,lit);
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

// ---- tela titulo (SCR_MAIN, cima): Westminster ao por do sol + logo ----
static void Title(u16 *d){
 static const u8 SKY[7][3]={{3,3,9},{6,4,12},{11,6,14},{18,8,13},{25,12,10},{29,18,9},{31,24,12}};
 static const u8 BND[8]={0,22,42,60,76,90,104,124};
 for(int y=0;y<124;y++){
  int b=0;while(b<6&&y>=BND[b+1])b++;
  int toNext=BND[b+1]-y;
  for(int x=0;x<256;x++){
   int k=b;
   if(b<6&&toNext<=2&&((x+y)&1))k=b+1;         // pontilhado entre faixas (sem gradiente)
   if(b<6&&toNext==3&&((x&1)==0&&(y&1)==0))k=b+1;
   d[y*256+x]=PXC(SKY[k][0],SKY[k][1],SKY[k][2]);
  }
 }
 // nuvens em faixas finas
 static const u8 CL[]={30,31,47,48,64,65,80,94};
 for(unsigned i=0;i<sizeof CL;i++){int y=CL[i];for(int x=0;x<256;x++)if(Hh(x>>4,y>>1)%3==0){
  u16 c=d[y*256+x];int r=c&31,g=(c>>5)&31,b=(c>>10)&31;d[y*256+x]=PXC(Cl(r-3),Cl(g-2),Cl(b-1));}}
 // sol
 for(int y=96;y<124;y++)for(int x=160;x<192;x++){
  int dx=x-176,dy=y-112,q=dx*dx+dy*dy;
  if(q<=13*13)d[y*256+x]=PXC(31,28,16);else if(q<=16*16&&((x+y)&1))d[y*256+x]=PXC(31,25,12);
 }
 // rio
 for(int y=124;y<192;y++)for(int x=0;x<256;x++){
  u16 c=PXC(5,4,11);
  if(y%3==0&&Hh(x>>3,y)%5==0)c=PXC(9,6,14);
  int half=16-(y-124)/5;
  if(half>0&&x>176-half&&x<176+half&&(y&1)==0&&Hh(x,y)%3)c=PXC(30,20,10);
  d[y*256+x]=c;
 }
 const u16 SIL=PXC(4,3,8);
 // cidade distante (camada mais clara): telhados, cupulas e agulhas goticas
 {const u16 FAR=PXC(9,6,13);
  for(int x=0;x<256;){
   int w=6+Hh(x,3)%10,h=6+Hh(x,5)%16,t=Hh(x,9)%6;
   Px_Rect(d,x,124-h,w,h,FAR);
   if(t==0)FillTri(d,x,124-h,x+w-1,124-h,x+w/2,124-h-12,FAR);          // agulha
   else if(t==1)for(int r=0;r<w/2;r++)Px_Rect(d,x+w/2-(w/2-r),124-h-(w/2)+r*r/(w/2+1)-1,2*(w/2-r),1,FAR); // cupula
   else if(t==2){Px_Rect(d,x+w/2-1,124-h-8,3,8,FAR);FillTri(d,x+w/2-2,124-h-8,x+w/2+2,124-h-8,x+w/2,124-h-14,FAR);}
   if(Hh(x,11)%3==0)Px_PSet(d,x+w/2,124-h/2,PXC(28,20,9));
   x+=w+(int)(Hh(x,13)%3);
  }}
 Westminster(d,22,124,SIL,PXC(30,23,10),false);
 // pinaculos goticos ao longo do telhado do Parlamento
 for(int x=28;x<206;x+=12){Px_Rect(d,x,124-26,2,4,SIL);Px_PSet(d,x,124-28,SIL);}
 // reflexo da silhueta (tremido)
 for(int y=124;y<136;y++)for(int x=22;x<208;x++)if(((x+y)&1)&&Hh(x,y)%4)d[y*256+x]=PXC(3,2,7);
 Sailboat(d,40,160,SIL,false);
 // logo da concept (tools/genart.py): coroa + Oh My Victoria DS com pincelada
 Ui_Crown(d,120,2);
 Art_Blit(d,&ART_LOGO,20,12);
 // "Aperte A" pulsando (30 frames visivel / 15 apagado)
 if((Menu_Ptr()->tick%45)<30){
  Px_Rect(d,176,164,72,20,C_NAVY);Px_Border(d,176,164,72,20,C_GOLD);
  for(int k=0;k<8;k++)Px_PSet(d,182+k,176-k,C_GOLD);   // pena
  Px_PSet(d,181,177,C_GOLD_HI);
  Px_Text2(d,196,170,Str(STR_APERTE_A),&F_SERIF8,C_TEXT);
 }
}

// ---- cada tela ----
static void MainBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Px_Border(d,2,2,252,188,C_GOLD);Px_Border(d,4,4,248,184,C_GOLD_DIM);
 // retrato do oficial em moldura dourada (concept, menu principal)
 Px_Rect(d,8,14,88,164,C_NAVY_BG);
 Px_Border(d,8,14,88,164,C_GOLD);Px_Border(d,10,16,84,160,C_GOLD_DIM);
 Art_Blit(d,&ART_OFFICER,14,20);
 Px_Rect(d,14,158,76,16,C_NAVY);Px_Rect(d,14,157,76,1,C_GOLD_DIM);
 Ui_TextC(d,52,161,"1800 · 2025",0,C_GOLD);
 Ui_Navy(d,104,14,144,162);
 static const u16 IT[4]={STR_NOVO_JOGO,STR_CARREGAR,STR_OPCOES,STR_CREDITOS};
 for(int i=0;i<4;i++){
  Ui_Item(d,108,22+i*30,136,Str(IT[i]),m->sel==i,i==1,Pulse(),false);
  if(i<3&&m->sel!=i&&m->sel!=i+1)Px_Rect(d,114,22+i*30+27,124,1,C_GOLD_DIM);
 }
 Ui_Crown(d,168,158);
}
static void ModeTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,12,6,232,180);
 // gravuras laterais tenues
 for(int y=40;y<160;y+=2){Px_PSet(d,24,y,C_PARCH_SH);Px_PSet(d,231,y,C_PARCH_SH);}
 for(int y=50;y<150;y+=12){Ui_Div(d,26,y,C_PARCH_SH);}
 Ui_TextC(d,128,16,Str(STR_ESCOLHA_MODO),1,C_INK);
 Ui_Div(d,128,32,C_INK);
 static const u16 IT[3]={STR_CAMPANHA,STR_CONTRA_IA,STR_MULTIJOGADOR};
 static const u8 IC[3]={ICO_CROWN,ICO_SWORDS,ICO_PEOPLE};
 for(int i=0;i<3;i++)Ui_BigBtn(d,40,46+i*36,Str(IT[i]),IC[i],m->sel==i,i==2,Pulse());
}
static void ModeBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,12,12,232,168);
 Px_Rect(d,21,33,82,98,C_GOLD);
 if(m->sel==0)PxW_Earth(d,22,34,80,96,Nat_Find("GBR"),0);
 else{Ui_Parch(d,22,34,80,96);if(m->sel==1)Compass(d,62,86,26);else{Ui_IconId(d,54,74,ICO_PEOPLE,false);}}
 static const u16 DS[3]={STR_DESC_CAMPANHA,STR_DESC_IA,STR_DESC_MULTI};
 int n=Px_WrapN(d,110,40,124,Str(DS[m->sel]),&F_SERIF8,C_INK,6,false);
 if(m->sel==2)Px_Text2(d,110,44+n*11,Str(STR_EM_BREVE),&F_SERIF8,C_GOLD_DIM);
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
 Ui_Backdrop(d);
 Ui_Parch(d,10,8,236,176);
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
 Ui_Backdrop(d);
 Ui_Parch(d,8,8,240,176);
 if(m->map==MAPK_MUNDI)PxW_Earth(d,16,18,224,84,-1,0);
 else{Compass(d,128,60,32);}
 static const u16 DS[MAPK_COUNT]={STR_DESC_MUNDI,STR_DESC_ALEAT,STR_DESC_BLITZ,STR_DESC_CLASSICO};
 Ui_Div(d,128,110,C_INK);
 Px_WrapN(d,18,122,220,Str(DS[m->map%MAPK_COUNT]),&F_SERIF8,C_INK,5,true);
}
static void NationTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,10,8,236,176);
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
static void NationBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,6,6,244,180);
 PxW_Earth(d,14,14,228,86,m->nat,(m->tick/20)&1);
 if(m->nat<0)return;
 const NationDef *nd=&NATIONS[m->nat];
 // ficha: governo · continente (+ endonimo)
 char t[96];int n=0;
 const char *a=Nat_GovName(nd->gov);while(*a&&n<90)t[n++]=*a++;
 const char *sep=" · ";while(*sep&&n<90)t[n++]=*sep++;
 a=Nat_ContName(nd->cont);while(*a&&n<90)t[n++]=*a++;
 if(nd->endo){sep=" · ";while(*sep&&n<90)t[n++]=*sep++;a=nd->endo;while(*a&&n<94)t[n++]=*a++;}
 t[n]=0;
 Px_WrapN(d,16,106,224,t,&F_SERIF8,C_INK,1,false);
 Px_Rect(d,16,119,224,1,C_PARCH_SH);
 int y=124;
 for(int i=0;i<NAT_TRAITS&&y<168;i++){
  u8 tr=nd->traits[i];if(!tr)continue;
  Px_Text2(d,16,y,Tr_Name(tr),&F_SERIF8,C_INK);
  int wn=Px_Width2(Tr_Name(tr),&F_SERIF8);
  int lines=Px_WrapN(d,16+wn+6,y,224-wn-6,Tr_Desc(tr),&F_SERIF8,C_GOLD_DIM,1,false);
  y+=11*(lines?lines:1)+1;
 }
 Px_WrapN(d,16,172,224,Str(STR_DICA_NACAO),&F_SERIF8,C_GOLD_DIM,1,true);
}
static void Dim(u16 *d){
 for(int i=0;i<256*192;i++)d[i]=(u16)(((d[i]>>1)&0x3DEF)|BIT(15));
}
static void PauseTop(u16 *d){
 Menu *m=Menu_Ptr();
 PxW_Map(d);Dim(d);
 Ui_Navy(d,64,30,140,126);
 static const u16 IT[4]={STR_CONTINUAR,STR_SALVAR,STR_CONFIG,STR_SAIR_MENU};
 for(int i=0;i<4;i++)Ui_Item(d,68,40+i*26,132,Str(IT[i]),m->sel==i,i==1,Pulse(),true);
 Ui_Crown(d,126,140);
}
static void PauseBot(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,10,10,236,172);
 static const u16 DS[4]={STR_DESC_CONTINUAR,STR_DESC_SALVAR,STR_DESC_CONFIG,STR_DESC_SAIR};
 const char *s=Str(DS[m->sel&3]);
 if(m->sel==1)s=Str(STR_EM_BREVE);
 if(m->sel==3&&m->confirm)s=Str(STR_CONFIRMA_SAIR);
 Px_WrapN(d,20,28,216,s,&F_SERIF8,C_INK,3,true);
 Ui_Div(d,128,66,C_INK);
 // gravura do porto: Westminster + veleiros em traco sepia
 for(int y=170;y<176;y+=2)for(int x=22;x<234;x+=4)if(Hh(x,y)%3)Px_Rect(d,x,y,2,1,C_INK);
 Westminster(d,32,168,C_INK,C_INK,true);
 Sailboat(d,214,166,C_INK,true);
 Sailboat(d,20,170,C_INK,true);
}
static void OptionsTop(u16 *d){
 Menu *m=Menu_Ptr();Settings *s=Set_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,10,8,236,176);
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
// brasao: escudo esquartelado + coroa + louros
static void Crest(u16 *d,int cx,int y0){
 for(int y=0;y<56;y++){
  int hw=y<36?24:24-(y-36)*24/20;
  for(int x=-hw;x<hw;x++){
   bool left=x<0,top=y<26;
   u16 c=(left==top)?C_NAVY:C_BRUSH;
   if(x==-hw||x==hw-1||y==0||(y>=36&&(x<=-hw+1||x>=hw-2)))c=C_GOLD;
   Px_PSet(d,cx+x,y0+y,c);
  }
 }
 for(int y=2;y<50;y++)Px_PSet(d,cx,y0+y,C_GOLD_DIM);
 for(int x=-22;x<22;x++)Px_PSet(d,cx+x,y0+26,C_GOLD_DIM);
 Ui_Crown(d,cx-8,y0-14);
 for(int i=0;i<8;i++){
  int ly=y0+50-i*6,lx=cx-34+(i*i)/6;
  Px_Rect(d,lx,ly,3,2,C_GOLD_DIM);Px_Rect(d,lx-2,ly-2,2,2,C_GOLD_DIM);
  Px_Rect(d,2*cx-lx-3,ly,3,2,C_GOLD_DIM);Px_Rect(d,2*cx-lx+2,ly-2,2,2,C_GOLD_DIM);
 }
}
static void OptionsBot(u16 *d){
 Ui_Backdrop(d);
 Ui_Parch(d,10,10,236,172);
 Crest(d,128,40);
 Px_WrapN(d,30,118,196,Str(STR_AJUSTE),&F_SERIF8,C_INK,2,true);
}
static void CreditsTop(u16 *d){
 Menu *m=Menu_Ptr();
 Ui_Backdrop(d);
 Ui_Parch(d,10,8,236,176);
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
  case SCR_CITY:   {WGame *s=WG_Ptr();PxC_Panel(d,s->cityIdx,s->citySlot,s->cityPick,s->pickK);}break;
  case SCR_LEDGER: PxL_Bot(d);break;
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
