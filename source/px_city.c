#include <string.h>
#include "wgame.h"
#include "city.h"
#include "wecon.h"
#include "wdiplo.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "flag.h"
// px_city.c — arte das cidades (DESIGN.md sec 4.7), 100% procedural.
// Um "pintor" de obras em unidades de projeto (32 de largura) escala para
// qualquer tamanho: o mesmo codigo desenha o icone no mapa (16px), a
// miniatura do slot (26px) e o predio grande da cena (46-56px). Estilo
// arquitetonico por cultura (europeu, leste asiatico, islamico, africano,
// indigena) e por era (madeira/estuque -> tijolo 1880 -> concreto 1950).
// Cena da cidade (Imperator Rome): fundo cacheado nos 88KB do LAYER do mapa
// (PxM_Scratch) e so' as pessoas/fumaca/trem/barcos redesenhados por quadro.

static inline int Cl(int v){return v<0?0:(v>31?31:v);}
static inline u16 C3(int r,int g,int b){return PXC(Cl(r),Cl(g),Cl(b));}
static inline u16 Dk(u16 c,int num,int den){return C3((c&31)*num/den,((c>>5)&31)*num/den,((c>>10)&31)*num/den);}
static u32 Hh(u32 a,u32 b){u32 h=a*2654435761u^b*40503u^0x7F4A7C15u;h^=h>>15;h*=0x2C1B3C6Du;h^=h>>12;return h;}

// ---- pintor em unidades de projeto: (ux,uy) com uy subindo do chao ----
static u16 *D;static int OX,OY,UW;static int CY0=0,CY1=192;  // recorte vertical
static int YOFF=0;  // fundo da cena: desenha no LAYER (linha 20 da tela = 0)
static inline int SX(int u){return OX+u*UW/32;}
static inline int SY(int u){return OY-u*UW/32;}
static void Put(int x,int y,u16 c){if(y>=CY0&&y<CY1)Px_PSet(D,x,y-YOFF,c);}
static void Fill(int x0,int y0,int x1,int y1,u16 c){for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)Put(x,y,c);}
// caixa com contorno 1px mais escuro (o "outline" da pixel art)
static void Box(int ux,int uy,int uw,int uh,u16 c){
 int x0=SX(ux),x1=SX(ux+uw),y1=SY(uy),y0=SY(uy+uh);
 if(x1<=x0)x1=x0+1;
 if(y1<=y0)y0=y1-1;
 Fill(x0,y0,x1,y1,c);
 u16 o=Dk(c,5,8);
 for(int x=x0;x<x1;x++){Put(x,y0,o);Put(x,y1-1,o);}
 for(int y=y0;y<y1;y++){Put(x0,y,o);Put(x1-1,y,o);}
 // luz de cima-esquerda
 if(x1-x0>3&&y1-y0>3)for(int y=y0+1;y<y1-1;y++)Put(x0+1,y,C3((c&31)+2,((c>>5)&31)+2,((c>>10)&31)+2));
}
static void Tri(int ux0,int ux1,int uy,int uh,u16 c){   // telhado de duas aguas
 int x0=SX(ux0),x1=SX(ux1),yb=SY(uy),yt=SY(uy+uh);
 int h=yb-yt;if(h<1)h=1;
 u16 o=Dk(c,5,8);
 for(int y=yt;y<yb;y++){
  int k=(y-yt)*(x1-x0)/(2*h);int a=(x0+x1)/2-k,b=(x0+x1)/2+k;
  for(int x=a;x<=b;x++)Put(x,y,(x==a||x==b)?o:(x<(x0+x1)/2?C3((c&31)+2,((c>>5)&31)+1,((c>>10)&31)+1):c));
 }
}
static void Curved(int ux0,int ux1,int uy,int uh,u16 c){ // telhado asiatico (pontas viradas)
 int x0=SX(ux0),x1=SX(ux1),yb=SY(uy),yt=SY(uy+uh);
 int h=yb-yt;if(h<1)h=1;
 u16 o=Dk(c,5,8);
 for(int y=yt;y<yb;y++){
  int t=y-yt,k=(t*t*(x1-x0))/(2*h*h)+1;int a=(x0+x1)/2-k,b=(x0+x1)/2+k;
  for(int x=a;x<=b;x++)Put(x,y,(x==a||x==b||y==yb-1)?o:c);
 }
 Put(x0-1,yb-2,o);Put(x1+1,yb-2,o);
}
static void Dome(int ucx,int uy,int ur,u16 c){
 int cx=SX(ucx),cy=SY(uy),r=ur*UW/32;if(r<1)r=1;
 u16 o=Dk(c,5,8);
 for(int y=-r;y<=0;y++)for(int x=-r;x<=r;x++){int q=x*x+y*y;if(q<=r*r)Put(cx+x,cy+y,q>(r-1)*(r-1)?o:(x<0&&y<-r/2?C3((c&31)+3,((c>>5)&31)+3,((c>>10)&31)+3):c));}
}
static void Wins(int ux,int uy,int uw,int uh,int nx,int ny,u16 c){
 for(int j=0;j<ny;j++)for(int i=0;i<nx;i++){
  int x=SX(ux+(uw*(2*i+1))/(2*nx)),y=SY(uy+(uh*(2*j+1))/(2*ny));
  int s=UW>=40?2:1;
  Fill(x,y-s+1,x+s,y+1,c);
 }
}
static void Pole(int ux,int uy,int uh,u16 c){int x=SX(ux);for(int y=SY(uy+uh);y<SY(uy);y++)Put(x,y,c);}

// paletas por estilo e era
typedef struct { u16 wall,roof,trim,accent; } Pal;
static Pal StylePal(u8 st,u16 year,u32 seed){
 Pal p;int v=(int)(seed%3)-1;
 switch(st){
  case ST_ASIA:  p.wall=C3(26+v,25,21);p.roof=C3(8,10+v,14);p.trim=C3(22,6,5);p.accent=C3(25,20,9);break;
  case ST_ISLAM: p.wall=C3(29,27+v,22);p.roof=C3(25,23,19);p.trim=C3(6,16,20);p.accent=C3(25,20,9);break;
  case ST_AFRICA:p.wall=C3(24+v,15,8);p.roof=C3(22,17,8);p.trim=C3(14,8,4);p.accent=C3(28,22,10);break;
  case ST_NATIVE:p.wall=C3(24,20+v,14);p.roof=C3(26,22,16);p.trim=C3(18,8,6);p.accent=C3(9,14,22);break;
  default:       p.wall=C3(27,24+v,18);p.roof=C3(21,8+v,6);p.trim=C3(10,11,14);p.accent=C3(25,20,9);break;
 }
 if(year>=1880&&(st==ST_EURO||seed%4==0)){p.wall=C3(21+v,11,8);p.roof=C3(10,11,14);}
 if(year>=1950){p.wall=C3(22+v,22,23);p.roof=C3(14,15,17);}
 return p;
}
static const u16 GLASS_LIT=PXC(28,24,12);

// casa do povo (preenche a cidade conforme a populacao)
static void House(u8 st,u16 year,u8 tier,u32 seed){
 Pal p=StylePal(st,year,seed);
 int fl=1+(int)(seed%(tier+1));                       // andares
 if((st==ST_AFRICA||st==ST_NATIVE)&&tier<=CT_VILAREJO){
  // cabana redonda de barro com telhado conico de palha
  Box(6,0,20,10,p.wall);Tri(2,30,10,14,p.roof);
  Box(14,0,4,6,Dk(p.wall,1,2));return;
 }
 if(year>=1950&&tier>=CT_METROPOLE&&seed%3==0){        // torre moderna
  Box(8,0,16,44,C3(14,18,22));Wins(8,2,16,40,3,10,(seed>>3)%2?GLASS_LIT:C3(20,26,28));return;
 }
 int h=8+fl*7;
 Box(4,0,24,h,p.wall);
 Wins(4,2,24,h-4,3,fl,(seed>>4)%3?C3(9,10,14):GLASS_LIT);
 Box(14,0,4,6,Dk(p.wall,1,2));
 if(st==ST_ASIA)Curved(0,32,h,8,p.roof);
 else if(st==ST_ISLAM){Box(2,h,28,2,p.wall);if(seed%5==0)Dome(16,h+2,6,p.roof);}
 else Tri(2,30,h,10,p.roof);
}
static void Smoke(int ux,int uy,int tick,u32 seed){
 for(int k=0;k<4;k++){
  int t=(tick/2+k*9+(int)(seed%9))%36;
  int x=SX(ux)+(t/6)-(k&1),y=SY(uy)-t;
  int r=1+t/14;u16 c=t<18?C3(20,20,21):C3(25,25,26);
  for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++)if(dx*dx+dy*dy<=r*r)Put(x+dx,y+dy,c);
 }
}
// Pinta a obra k com base em (x,yb) e largura w. tick<0 = sem animacao.
// bandeira atual de uma nacao (com a ideologia de quem a governa hoje)
static void NatFlag(u8 nat,int x,int y,int w,int h){
 const World *W=WG_World();FlagDef f;
 u8 ci=World_CountryOfNat(W,nat);
 if(ci)WDip_Flag(W,ci,&f);else f=NATIONS[nat].flag;
 Flag_Draw(D,256,x,y,w,h,&f);
}
static void Tree(int x,int yb,int sz,int kind,u32 h);
void PxC_Building(u16 *d,int x,int yb,int w,u8 k,u8 st,u16 year,u32 seed,int tick,u8 natFlag){
 D=d;OX=x;OY=yb;UW=w;
 Pal p=StylePal(st,year,seed);
 const u16 STONE=C3(20,19,17),WOOD=C3(18,12,7),RED=C3(22,7,5),WHITE=C3(29,29,27);
 switch(k){
  case BK_FAZENDA:
   for(int r=0;r<4;r++)Box(0,r*2,18,2,(r&1)?C3(24,22,8):C3(12,20,6));    // plantacao
   Box(17,0,13,12,st==ST_EURO?RED:p.wall);Tri(15,32,12,8,p.roof);
   Box(21,0,5,6,WOOD);
   if(st==ST_EURO){Box(3,0,6,18,C3(22,22,22));Dome(6,18,3,C3(20,20,20));}
   break;
  case BK_MERCADO:
   for(int i=0;i<3;i++){
    int ux=1+i*10;u16 a=i==1?C3(6,10,20):RED;
    Pole(ux,0,10,WOOD);Pole(ux+8,0,10,WOOD);
    for(int s=0;s<4;s++)Box(ux+s*2,10,2,3,(s&1)?WHITE:a);
    Box(ux+1,0,7,4,WOOD);
    Put(SX(ux+3),SY(5),C3(28,20,6));Put(SX(ux+5),SY(5),C3(12,22,6));
   }
   break;
  case BK_OFICINA:
   Box(3,0,22,14,st==ST_EURO&&year<1880?WOOD:p.wall);Tri(1,27,14,8,p.roof);
   Box(20,14,4,12,STONE);Box(8,0,6,9,Dk(p.wall,1,2));
   if(tick>=0)Smoke(22,26,tick,seed);
   break;
  case BK_FABRICA:
   Box(0,0,32,16,C3(21,10,7));
   for(int i=0;i<4;i++)Tri(i*8,i*8+8,16,5,C3(12,12,14));
   Wins(0,3,32,10,6,2,GLASS_LIT);
   Box(5,16,3,18,C3(18,9,6));Box(22,16,3,24,C3(18,9,6));
   if(tick>=0){Smoke(6,34,tick,seed);Smoke(23,40,tick,seed+5);}
   break;
  case BK_QUARTEL:
   Box(2,0,28,13,STONE);for(int i=0;i<7;i++)Box(2+i*4,13,2,2,STONE);
   Box(13,0,6,8,Dk(STONE,1,2));Wins(2,4,28,6,4,1,C3(9,10,12));
   Pole(28,13,14,C3(10,10,10));
   if(natFlag<255&&w>=24)NatFlag(natFlag,SX(28)+1,SY(27)-YOFF,w/5,w/7);
   else Box(28,22,5,4,RED);
   break;
  case BK_PORTO:
   Box(0,0,32,3,WOOD);for(int i=0;i<5;i++)Pole(2+i*7,-3,3,WOOD);
   Pole(24,3,20,C3(12,12,12));Box(14,22,12,1,C3(12,12,12));Pole(15,14,8,C3(30,30,30));
   Box(3,3,8,6,C3(20,14,7));Box(12,3,6,4,C3(16,10,6));
   break;
  case BK_TEMPLO:
   if(st==ST_ASIA){ // pagode de 3 telhados
    Box(10,0,12,8,p.trim);Curved(4,28,8,5,p.roof);Box(12,13,8,6,p.trim);Curved(6,26,19,4,p.roof);
    Box(14,23,4,5,p.trim);Curved(9,23,28,4,p.roof);Pole(16,32,5,p.accent);
   }else if(st==ST_ISLAM){ // mesquita: cupula + minarete
    Box(4,0,20,12,p.wall);Dome(14,12,8,p.trim);Box(25,0,4,28,p.wall);Dome(27,28,2,p.trim);
    Box(12,0,4,7,Dk(p.wall,1,2));
   }else if(st==ST_AFRICA){ // mesquita de barro (Djenne)
    Box(4,0,24,16,p.wall);for(int i=0;i<4;i++)Box(5+i*7,16,3,6,p.wall);
    for(int i=0;i<6;i++){Put(SX(6+i*4),SY(10),WOOD);}
    Box(13,0,6,8,Dk(p.wall,1,2));
   }else if(st==ST_NATIVE){ // casa cerimonial + totem
    Box(4,0,20,10,WOOD);Tri(2,26,10,10,p.roof);Box(27,0,4,22,RED);Box(26,18,6,2,p.accent);
   }else{ // igreja: nave + torre com agulha
    Box(3,0,18,14,p.wall==C3(22,22,23)?STONE:p.wall);Tri(1,23,14,8,p.roof);
    Box(20,0,9,26,STONE);Tri(19,30,26,12,p.roof);Box(23,16,3,4,C3(9,10,14));
    Box(10,0,4,8,Dk(p.wall,1,2));
   }
   break;
  case BK_MURALHA:
   Box(0,0,32,12,STONE);for(int i=0;i<8;i++)Box(i*4,12,2,2,STONE);
   Box(0,0,7,19,STONE);Box(25,0,7,19,STONE);Tri(-1,8,19,6,p.roof);Tri(24,33,19,6,p.roof);
   Box(13,0,6,8,C3(8,7,6));
   break;
  case BK_UNIVERSIDADE:
   Box(0,0,32,3,STONE);Box(2,3,28,14,WHITE);
   for(int i=0;i<6;i++)Box(4+i*4,3,2,12,C3(25,24,22));
   Tri(1,31,17,6,WHITE);Dome(16,23,6,st==ST_ISLAM?p.trim:C3(12,16,14));Pole(16,29,4,p.accent);
   break;
  case BK_FERROVIA:
   Box(2,0,28,12,p.wall);Tri(0,32,12,7,p.roof);Box(13,12,6,8,p.wall);
   Dome(16,16,2,WHITE);Wins(2,2,28,8,4,1,GLASS_LIT);
   break;
  case BK_PALACIO:
   if(st==ST_ASIA){Box(0,0,32,10,p.trim);Curved(-3,35,10,7,p.roof);Box(8,17,16,7,p.trim);Curved(4,28,24,6,p.roof);}
   else{
    Box(0,0,32,16,p.wall==C3(22,22,23)?WHITE:p.wall);Wins(0,2,32,12,8,2,C3(9,10,14));
    Box(11,0,10,22,WHITE);Dome(16,22,6,st==ST_ISLAM?p.trim:C3(12,16,14));Pole(16,28,8,C3(10,10,10));
    if(natFlag<255&&w>=24)NatFlag(natFlag,SX(16)+1,SY(36)-YOFF,w/5,w/7);
   }
   break;
  case BK_SERRARIA:{ // galpao de madeira, roda de agua e pilhas de toras
   Box(4,0,18,10,WOOD);Tri(2,24,10,6,Dk(WOOD,3,4));
   Dome(26,6,5,C3(12,9,6));Box(25,0,2,12,C3(8,6,4));
   for(int i=0;i<3;i++){Box(i*5,0,4,3,C3(20,13,6));Put(SX(i*5+2),SY(2),C3(26,20,10));}
   Box(10,10,1,6,STONE);
   if(tick>=0)Smoke(10,16,tick,seed);
  }break;
  case BK_MINA:{ // morro com boca de mina, castelete (apos 1850) e vagonete
   for(int j=0;j<14;j++)Box(j,j,32-2*j,1,j<10?C3(15,13,11):C3(18,16,13));
   Box(11,0,8,7,C3(4,3,3));Box(10,7,10,1,WOOD);Box(10,0,1,8,WOOD);Box(19,0,1,8,WOOD);
   if(year>=1850){Pole(25,0,20,C3(10,10,11));Pole(30,0,20,C3(10,10,11));Box(24,20,8,2,C3(10,10,11));Dome(28,22,2,C3(14,14,15));}
   Box(0,0,7,3,C3(12,12,13));Put(SX(1),SY(0),C3(4,4,4));Put(SX(5),SY(0),C3(4,4,4));
   Box(1,3,5,2,C3(20,12,8));
  }break;
  case BK_ARSENAL:{ // pavilhao de pedra com canhao e balas
   Box(2,0,26,14,STONE);Box(2,14,26,2,Dk(STONE,2,3));for(int i=0;i<6;i++)Box(3+i*4,16,2,2,STONE);
   Box(11,0,8,9,C3(8,7,6));Wins(2,6,26,5,4,1,C3(9,10,12));
   Box(0,1,10,2,C3(6,6,7));Dome(2,2,3,C3(10,7,4));
   for(int i=0;i<3;i++)Dome(27+(i%2)*2,1+(i/2)*2,1,C3(5,5,6));
   if(tick>=0&&year>=1860)Smoke(24,18,tick,seed);
  }break;
  case BK_ESTALEIRO:{ // carreira com casco em obra e guindaste
   Box(0,0,32,2,C3(14,11,8));
   for(int j=0;j<6;j++)Box(4+j,2+j,22-2*j,1,year>=1860?C3(8,9,10):C3(18,12,6));
   for(int i=0;i<6;i++)Pole(6+i*4,8,6,WOOD);
   Pole(29,2,24,C3(12,12,12));Box(18,25,12,1,C3(12,12,12));Pole(19,18,7,C3(30,30,30));
   Box(17,16,4,2,C3(20,14,7));
  }break;
  case BK_POCO:{ // torre trelicada + tanque
   for(int j=0;j<26;j++){int hw=8-j*7/26;Put(SX(10-hw),SY(j),C3(8,8,9));Put(SX(10+hw),SY(j),C3(8,8,9));
    if((j%4)==0)for(int i=-hw;i<=hw;i++)Put(SX(10+i),SY(j),C3(8,8,9));}
   Box(9,26,3,2,C3(8,8,9));
   Box(20,0,11,9,C3(24,24,22));Box(20,9,11,1,C3(14,14,15));Box(21,4,9,1,C3(22,6,4));
  }break;
  case BK_ESCOLA:{ // casinha escolar com sineira e patio
   Box(4,0,22,12,p.wall);Tri(2,28,12,7,p.roof);Wins(4,3,22,6,3,1,C3(9,10,14));
   Box(13,0,4,6,Dk(p.wall,1,2));Box(13,19,4,6,STONE);Tri(12,18,25,4,p.roof);Put(SX(15),SY(22),C3(25,20,9));
   for(int i=0;i<3;i++)Box(27+i*2,0,1,4,WOOD);
  }break;
  case BK_HOSPITAL:{ // pavilhao branco com a cruz vermelha
   Box(1,0,30,16,WHITE);Box(1,16,30,2,C3(20,20,22));Wins(1,3,30,10,5,2,C3(12,16,20));
   Box(13,0,6,8,C3(14,18,22));Box(14,20,4,10,RED);Box(11,23,10,4,RED);
  }break;
  case BK_BANCO:{ // fachada classica: frontao, colunas e portas de bronze
   Box(2,0,28,3,STONE);Box(3,3,26,14,C3(25,24,21));
   for(int i=0;i<5;i++)Box(5+i*5,3,2,12,C3(29,28,26));
   Tri(2,30,17,7,C3(25,24,21));Box(13,0,6,7,C3(16,12,5));Put(SX(16),SY(20),C3(28,22,8));
  }break;
  case BK_TEATRO:{ // casa de opera: arcos, cupula e cortina vermelha
   Box(0,0,32,14,C3(26,23,17));for(int i=0;i<4;i++){Box(3+i*7,0,5,9,C3(18,4,6));Dome(5+i*7,9,2,C3(26,23,17));}
   Box(0,14,32,2,STONE);Dome(16,16,9,st==ST_ISLAM?p.trim:C3(13,17,15));Pole(16,25,4,C3(25,20,9));
  }break;
  case BK_PARQUE:{ // gramado, lago, arvores e um banco
   for(int j=0;j<4;j++)Box(0,j,32,1,C3(11+j,20,8));
   Dome(10,1,6,C3(10,17,26));
   D=d;Tree(SX(23),SY(0),3,0,seed);Tree(SX(30),SY(0),2,0,seed>>4);Tree(SX(3),SY(0),2,3,seed>>8);
   Box(17,0,6,2,WOOD);
  }break;
  case BK_SIDERURGICA:{ // alto-forno com boca acesa, galpao e chamine
   Box(0,0,18,14,C3(16,9,7));Tri(0,18,14,5,C3(10,10,12));Box(3,0,5,7,C3(29,16,4));
   Box(19,0,10,24,C3(12,11,12));Box(20,24,8,3,C3(10,9,10));Box(21,3,6,5,C3(31,20,5));Put(SX(24),SY(5),C3(31,29,16));
   Box(29,0,3,30,C3(18,9,6));
   if(tick>=0){Smoke(30,30,tick,seed);Smoke(24,27,tick,seed+3);}
  }break;
  case BK_MANUFATURA:{ // oficina comprida com telhado serrilhado e caixotes
   Box(1,0,28,13,p.wall);for(int i=0;i<3;i++)Tri(1+i*9,10+i*9+1,13,5,p.roof);
   Wins(1,3,28,7,4,1,GLASS_LIT);Box(12,0,5,6,Dk(p.wall,1,2));
   Box(26,0,5,4,WOOD);Box(27,4,3,2,WOOD);
  }break;
  case BK_MAQUINARIA:{ // pavilhao de ferro e vidro com engrenagem e chamine
   Box(0,0,30,15,C3(14,16,18));Tri(0,30,15,6,C3(10,10,12));Wins(0,2,30,10,5,2,C3(22,24,26));
   Dome(16,17,4,C3(22,22,24));for(int a=0;a<4;a++)Put(SX(16)+(a<2?a*4-2:0),SY(17)-(a>=2?(a-2)*4+1:0)-2,C3(10,10,11));
   Box(4,15,3,16,C3(18,9,6));
   if(tick>=0)Smoke(5,31,tick,seed);
  }break;
  case BK_ELETRONICA:{ // bloco moderno de vidro com antena
   Box(2,0,28,22,C3(20,24,26));Wins(2,2,28,18,6,4,(seed&1)?GLASS_LIT:C3(10,18,26));
   Box(2,22,28,2,C3(12,14,16));Pole(25,24,10,C3(10,10,11));Put(SX(25),SY(34),C3(31,6,4));
  }break;
  case BK_ARMAMENTOS:{ // fabrica escura, chamine, canhoes e (1914+) um tanque no patio
   Box(0,0,24,14,C3(13,12,12));for(int i=0;i<3;i++)Tri(i*8,i*8+8,14,5,C3(9,9,10));
   Wins(0,3,24,7,4,1,C3(28,16,6));Box(26,0,5,28,C3(16,8,6));
   if(year>=1914){Box(1,0,11,4,C3(10,12,8));Box(4,4,5,2,C3(10,12,8));Box(9,5,7,1,C3(8,9,7));}
   else{Box(0,1,10,2,C3(6,6,7));Dome(2,2,3,C3(10,7,4));}
   if(tick>=0)Smoke(28,28,tick,seed);
  }break;
  case BK_BOLSA:{ // templo do capital: colunata, frontao e cupula dourada
   Box(1,0,30,3,STONE);Box(2,3,28,15,C3(27,26,23));
   for(int i=0;i<6;i++)Box(4+i*4,3,2,13,WHITE);
   Tri(1,31,18,7,C3(27,26,23));Dome(16,25,5,C3(27,22,8));Pole(16,30,3,C3(10,10,10));
  }break;
  case BK_USINA:{ // galpao de tijolo, chamine alta e torre de transmissao
   Box(0,0,22,14,C3(21,10,7));Tri(0,22,14,5,C3(12,12,14));Wins(0,3,22,8,4,1,GLASS_LIT);
   Box(24,0,5,38,C3(18,9,6));Box(23,38,7,2,C3(14,7,5));
   for(int j=0;j<20;j+=4){Put(SX(31),SY(j),C3(10,10,11));Put(SX(30),SY(j+2),C3(10,10,11));}
  }break;
  default:break;
 }
}
// andaime de obra em construcao (pct 0..100)
static void Scaffold(u16 *d,int x,int yb,int w,int pct){
 D=d;OX=x;OY=yb;UW=w;
 const u16 W1=C3(20,14,7);
 int h=6+pct*18/100;
 Box(4,0,24,h,C3(22,20,16));
 for(int i=0;i<=4;i++)Pole(4+i*6,0,24,W1);
 for(int j=0;j<=3;j++){int y=SY(j*8);for(int xx=SX(3);xx<SX(29);xx++)Put(xx,y,W1);}
 Pole(30,0,30,C3(12,12,12));for(int xx=SX(18);xx<SX(31);xx++)Put(xx,SY(30),C3(12,12,12));
}

// ======================= cidades no mapa =======================
// ---- cidade organica no mapa (Z3+, ADR-39) ----
// A cidade cresce como cidade de verdade: cada casa nova nasce GRUDADA numa
// casa que ja' existe (pai sorteado por hash), de preferencia ao longo das
// duas ruas principais; nada nasce no mar (KEY do cache do mapa). Quanto
// mais gente, mais casas e mais longe a mancha vai: o contorno sai
// irregular. Desenho de tras p/ frente (ordem de y) p/ os telhados
// sobreporem direito. Hash fixo da posicao: a mesma cidade tem sempre o
// mesmo desenho, e o bairro novo aparece colado no velho quando ela cresce.
enum { TG=25, TG2=TG/2, TMAX=96 };
static u8 TOCC[TG*TG];
static s8 THX[TMAX],THY[TMAX];
static bool TownLand(int x,int y){return y>WG_MAP_Y&&y<191&&x>=0&&x<256&&PxM_KeyAt(x,y-WG_MAP_Y)<0xFE;}
static int townBud;   // casas por quadro (todas as cidades): o pan no Z4 nao pesa (ADR-36)
static void Town(u16 *d,int cx,int cy,int zoom,const WCity *c,u8 st,u16 year,u32 seed,u8 nat){
 // tamanho da casa e passo da grade por zoom (Z3 4px/celula .. Z5 16px)
 D=d;
 // casas coladas: passo menor que a largura (telhado encosta no vizinho)
 int hw=zoom==3?5:(zoom==4?9:14),sx=hw*4/5,sy=hw*3/5;
 int n=3+c->tier*4+(c->pop>1500?30:c->pop/50);
 if(zoom==3)n=n/3+1;
 {static const u8 CAP[WZ_COUNT]={0,0,0,10,28,70};if(n>CAP[zoom])n=CAP[zoom];}
 if(n>townBud)n=townBud;
 if(n<1)n=1;
 townBud-=n;
 for(int i=0;i<TG*TG;i++)TOCC[i]=0;
 // ruas principais: eixo x sempre; eixo y no sentido sorteado
 int ry=(seed>>5)&1?1:-1;
 int m=0;THX[0]=0;THY[0]=0;TOCC[TG2*TG+TG2]=1;m=1;
 for(int k=1;k<n*3&&m<n;k++){
  u32 h=Hh(seed,(u32)k*7919u);
  int p=(int)(h%(u32)m);                       // pai: qualquer casa ja' de pe'
  // novas casas preferem as ruas (vizinho no eixo); depois as quadras
  int r=(int)((h>>8)&15),dx,dy;
  if(r<6){dx=(h>>12)&1?1:-1;dy=0;}
  else if(r<9){dx=0;dy=(h>>12)&1?1:-1;if(THX[p]==0)dy=ry*((h>>13)&1?1:-1);}
  else if(r<13){dx=(h>>12)&1?1:-1;dy=(h>>13)&1?1:-1;}
  else{dx=0;dy=(h>>12)&1?1:-1;}
  int gx=THX[p]+dx,gy=THY[p]+dy;
  if(gx<-TG2||gx>TG2||gy<-TG2||gy>TG2)continue;
  u8 *o=&TOCC[(gy+TG2)*TG+gx+TG2];
  if(*o)continue;
  // ilha de 1 casa nao: precisa de 2 vizinhos depois que a vila cresceu (compacta)
  if(m>8){int nb=0;for(int a=-1;a<=1;a++)for(int b=-1;b<=1;b++){int qx=gx+a+TG2,qy=gy+b+TG2;if(qx>=0&&qy>=0&&qx<TG&&qy<TG&&TOCC[qy*TG+qx])nb++;}
   if(nb<2&&((h>>20)&3))continue;}
  int x=cx+gx*sx,y=cy+gy*sy;
  if(!TownLand(x,y))continue;
  *o=1;THX[m]=(s8)gx;THY[m]=(s8)gy;m++;
 }
 // ruas de terra (Z4+): so' onde ha' casa na linha/coluna central
 if(zoom>=4){
  u16 road=C3(21,18,12);
  int x0=0,x1=0,y0=0,y1=0;
  for(int i=0;i<m;i++){if(THY[i]==0){if(THX[i]<x0)x0=THX[i];if(THX[i]>x1)x1=THX[i];}if(THX[i]==0){if(THY[i]<y0)y0=THY[i];if(THY[i]>y1)y1=THY[i];}}
  for(int x=cx+x0*sx-sx;x<=cx+x1*sx+sx;x++)if(TownLand(x,cy+sy/2+1))Put(x,cy+sy/2+1,road);
  for(int y=cy+y0*sy-sy;y<=cy+y1*sy+sy;y++)if(TownLand(cx+sx/2,y))Put(cx+sx/2,y,road);
  // hortas e campos na borda (fora da mancha), listrados
  for(int k=0;k<2+c->tier;k++){
   u32 h=Hh(seed,(u32)k+501u);int a=(int)(h%(u32)m);
   int fx=cx+THX[a]*sx+((h>>8)&1?sx+2:-sx-hw),fy=cy+THY[a]*sy+((h>>9)&1?2:-hw/2);
   for(int j=0;j<hw/2+1;j++)for(int i=0;i<hw;i++)if(TownLand(fx+i,fy+j))Put(fx+i,fy+j,(j&1)?C3(19,20,7):C3(12,18,6));
  }
 }
 // ordena por y (tras p/ frente)
 for(int i=1;i<m;i++){s8 x=THX[i],y=THY[i];int j=i-1;while(j>=0&&THY[j]>y){THX[j+1]=THX[j];THY[j+1]=THY[j];j--;}THX[j+1]=x;THY[j+1]=y;}
 // centro: obra-simbolo (palacio/templo) no lugar da casa (0,0); industria
 // (fabrica/siderurgica/arsenal) na periferia, com fumaca
 u8 sym=0,indk=0;
 for(int s=0;s<CITY_SLOTS;s++){u8 b=c->bld[s];
  if(b==BK_PALACIO||(b==BK_TEMPLO&&!sym))sym=b;
  if(b==BK_FABRICA||b==BK_SIDERURGICA||b==BK_ARMAMENTOS||b==BK_MAQUINARIA||(b==BK_OFICINA&&!indk))indk=b;}
 int far=-1,fd=-1;
 for(int i=0;i<m;i++){int dd=THX[i]*THX[i]+THY[i]*THY[i];if(dd>fd&&THY[i]>=0){fd=dd;far=i;}}
 for(int i=0;i<m;i++){
  u32 h=Hh(seed,(u32)(THX[i]*31+THY[i])+977u);
  int x=cx+THX[i]*sx+(int)(h&1),y=cy+THY[i]*sy+(int)((h>>1)&1);
  int w=hw+(int)((h>>3)%3)-1;
  // telhado mais alto/denso perto do centro (cidade grande: predios no miolo)
  int dd=THX[i]*THX[i]+THY[i]*THY[i];
  u8 t=(u8)(dd<=2?c->tier:(dd<=8&&c->tier>0?c->tier-1:0));
  if(THX[i]==0&&THY[i]==0&&sym&&c->tier>=CT_CIDADE&&zoom>=4){PxC_Building(d,x-w*3/4,y+sy/2,w*3/2,sym,st,year,seed,-1,nat);D=d;continue;}
  if(i==far&&indk&&zoom>=4&&m>6){PxC_Building(d,x-w/2,y+sy/2,w+2,indk,st,year,seed,zoom==5?(int)(WG_Ptr()->tick):-1,nat);D=d;continue;}
  D=d;OX=x-w/2;OY=y+sy/2;UW=w;House(st,year,t,h);
 }
 if(sym&&c->tier>=CT_CIDADE&&zoom==3)PxC_Building(d,cx-4,cy+3,8,sym,st,year,seed,-1,nat);
}
static inline bool ScreenOf(const World *w,const WGame *s,int cx,int cy,int *px,int *py){
 // cx e camX em [0,w): |rx|<w, sem o % (divisao em software) p/ cada cidade a cada quadro
 int rx=cx-s->camX;if(w->wrap){if(rx<=-w->w||rx>=w->w)rx%=w->w;if(rx<0)rx+=w->w;}
 int ry=cy-s->camY;
 if(s->zoom==0){*px=rx/2;*py=ry/2;}
 else{int ppc=1<<(s->zoom-1);*px=rx*ppc+ppc/2-s->subX;*py=ry*ppc+ppc/2-s->subY;}
 *py+=WG_MAP_Y;
 return *px>-30&&*px<286&&*py>WG_MAP_Y-20&&*py<212;
}
void PxC_MapCities(u16 *d,const World *w,const WGame *s){
 D=d;YOFF=0;CY0=WG_MAP_Y;CY1=192;
 int ppc=s->zoom?1<<(s->zoom-1):0;
 townBud=s->zoom==3?120:(s->zoom==4?240:200);
 for(int i=0;i<w->nCity;i++){
  const WCity *c=&w->city[i];
  int px,py;
  if(!ScreenOf(w,s,c->x,c->y,&px,&py))continue;
  u16 year=w->date.y;u32 seed=Hh(c->x,c->y);
  u8 st=City_Style(w,i);u8 nat=c->owner?w->c[c->owner].nat:255;
  if(s->zoom<=1){
   // afastado: so' metropoles, ponto discreto (o mapa e' dos paises)
   if(s->zoom==0||c->tier<CT_METROPOLE)continue;
   Put(px,py,C3(8,6,5));Put(px+1,py,C3(8,6,5));Put(px,py+1,C3(8,6,5));Put(px+1,py+1,C3(26,24,18));
   continue;
  }
  if(s->zoom==2){
   // continente: so' cidades e metropoles ganham casinha; vilarejo = ponto
   if(c->tier==CT_POVOADO)continue;
   if(c->tier==CT_VILAREJO){Put(px,py,C3(10,8,7));Put(px+1,py,C3(24,22,18));continue;}
   D=d;OX=px-3;OY=py+2;UW=c->tier==CT_METROPOLE?8:6;House(st,year,0,seed);
   continue;
  }
  // Z3+: a cidade cresce organica, casa colada em casa (ADR-39)
  if(c->tier==CT_POVOADO&&s->zoom==3){D=d;OX=px-3;OY=py+2;UW=7;House(st,year,0,seed);continue;}
  (void)ppc;
  Town(d,px,py,s->zoom,c,st,year,seed,nat);
 }
 CY0=0;CY1=192;
}

// ======================= mapa da cidade (ADR-35) =======================
// Tela de cima do SCR_CITY: a cidade vista do alto em CG_W x CG_H lotes de
// 16x16 px (y 26..186). Fundo (chao, agua, ruas, praca, campos, bairros e
// obras) e' pintado 1x no LAYER emprestado do mapa (PxM_Scratch) e so' volta
// a ser pintado quando a cidade muda (assinatura); por quadro so' entram os
// vivos: gente nas avenidas, carroca/carro, barcos, fumaca e a mira do lote.
// Bairros nascem sozinhos: sementes ativadas pela populacao puxam casas para
// perto delas (hash fixo: o mesmo bairro aparece na mesma hora em toda partida).
enum { MX=0, MY=26, LS=16 };
static u32 sceneSig;static int sceneCity=-1;
static u8 DEN[CG_N];          // 0 nada, 1-3 casas (densidade), 4 campo, 5 mata
static int roadRow=4,roadCol=7;
static u32 SceneSig(const World *w,int ci){
 const WCity *c=&w->city[ci];
 u32 h=Hh(ci,c->tier)^(c->pop/10)^((u32)c->bq<<12)^((u32)c->bqLot<<20)^((u32)(c->bqDays/10)<<16)^(w->date.y/10);
 for(int s=0;s<CITY_SLOTS;s++)h=h*31+c->bld[s]*977u+c->bpos[s];
 return h^((u32)c->owner<<24);
}
// ---- pintura: helpers (so' inteiro; tudo cai no LAYER em cache) ----
static const u8 B4[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
static u16 Get(int x,int y){if(x<0||x>255||y<CY0||y>=CY1)return 0;return D[(y-YOFF)*256+x];}
static void Darken(int x,int y,int num,int den){u16 c=Get(x,y);if(c)Put(x,y,Dk(c,num,den));}
// arvore por clima: 0 copa redonda, 1 conifera, 2 palmeira, 3 choupo
static void Tree(int x,int yb,int sz,int kind,u32 h){
 u16 trunk=C3(10,7,4);
 if(kind==2){
  sz+=2;
  for(int k=0;k<sz*2;k++){Put(x+(k*k)/(sz*6),yb-k,trunk);}
  int tx=x+(4*sz*sz)/(sz*6),ty=yb-sz*2;
  static const s8 FX[6]={-5,-3,0,3,5,1},FY[6]={2,-2,-3,-2,2,3};
  for(int f=0;f<6;f++)for(int k=0;k<=sz;k++)Put(tx+FX[f]*k/5,ty+FY[f]*k/5+(k*k)/(sz*3),k<2?C3(6,13,5):C3(8,17,6));
  return;
 }
 if(kind==1){
  for(int y=0;y<sz*2;y++){int hw=y/3+(y%3==2);
   for(int i=-hw;i<=hw;i++)Put(x+i,yb-sz*2-1+y,i<0?C3(5,11,7):C3(3,8,5));}
  Put(x,yb,trunk);return;
 }
 if(kind==3){
  for(int y=-sz*2;y<=0;y++){int hw=y<-sz*2+2?0:1;for(int i=-hw;i<=hw;i++)Put(x+i,yb-2+y,i<0?C3(9,17,7):C3(6,12,5));}
  Put(x,yb-1,trunk);return;
 }
 Put(x,yb,trunk);Put(x,yb-1,trunk);
 int r=sz;
 for(int y=-r;y<=r;y++)for(int i=-r;i<=r;i++){int q=i*i+y*y;if(q>r*r)continue;
  int lit=(i+y)<-r/2,dark=(i+y)>r/2;
  u16 c=lit?C3(11,20,8):dark?C3(5,11,4):C3(8,16,6);
  if(((h>>((i&3)*2))&3)==0&&q<r*r-r)c=C3(10,18,7);
  Put(x+i,yb-2-r+y,c);}
}
// ---- bairros: densidade de casas por lote ----
static void Quarters(const World *w,int ci){
 const WCity *c=&w->city[ci];
 int pl=City_Plaza(w,ci),px=pl%CG_W,py=pl/CG_W;
 u32 seed=Hh(c->x*31u,c->y);
 // sementes de bairro: a praca e mais 1 a cada ~60 mil habitantes (ate 7)
 int K=1+c->pop/60;if(K>8)K=8;
 int sx[8],sy[8];sx[0]=px;sy[0]=py;
 for(int k=1;k<K;k++){
  sx[k]=px;sy[k]=py;
  for(int t=0;t<12;t++){u32 h=Hh(seed,k*16+t);int x=(int)(h%CG_W),y=(int)((h>>8)%CG_H);
   int l=y*CG_W+x;if(City_LotBase(w,ci,l)!=LK_TERRA)continue;
   bool nearRoad=false;
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int nx=x+dx,ny=y+dy;
    if(nx>=0&&ny>=0&&nx<CG_W&&ny<CG_H&&City_LotBase(w,ci,ny*CG_W+nx)==LK_RUA)nearRoad=true;}
   if(nearRoad){sx[k]=x;sy[k]=y;break;}}
 }
 static u16 sc[CG_N];
 int land=0;
 for(int i=0;i<CG_N;i++){
  DEN[i]=0;sc[i]=0xFFFF;
  if(City_LotBase(w,ci,i)!=LK_TERRA||City_LotSlot(c,i)>=0||(c->bq&&c->bqLot==i))continue;
  int x=i%CG_W,y=i/CG_W,best=1<<30;
  for(int k=0;k<K;k++){int dx=x-sx[k],dy=y-sy[k];int d=(dx*dx+dy*dy)*(k?3:2);if(d<best)best=d;}
  sc[i]=(u16)(best*8+(int)(Hh(seed,i)%11));land++;
 }
 // casas: tantos lotes quanto a populacao pede, dos mais "atraentes" p/ fora
 int H=c->pop/7+c->tier*3+2;if(H>land*3/4)H=land*3/4;
 int dense=c->tier>=CT_CIDADE?H/4:0;
 for(int r=0;r<H;r++){
  int bi=-1;u16 bv=0xFFFF;
  for(int i=0;i<CG_N;i++)if(!DEN[i]&&sc[i]<bv){bv=sc[i];bi=i;}
  if(bi<0)break;
  DEN[bi]=(u8)(r<dense?3:(r<H/2?2:1));
 }
 // arredores: campos perto da cidade (se a terra da' lavoura), mata alem
 for(int i=0;i<CG_N;i++){
  if(DEN[i]||sc[i]==0xFFFF)continue;
  int x=i%CG_W-px,y=i/CG_W-py,d2=x*x+y*y;
  u32 h=Hh(seed,i+500);
  int rin=c->tier>=CT_METROPOLE?3:0;          // metropole: jardins no centro, campo so' fora
  if(d2<=rin*rin)DEN[i]=5;
  else if(d2<=(5+c->tier)*(5+c->tier)&&c->agri>=8)DEN[i]=4;
  else if((c->wood>=8&&(h&3)!=0)||(h&7)==0)DEN[i]=5;
 }
 // avenidas principais (gente e carroca andam nelas)
 int bestR=0,bestC=0;roadRow=4;roadCol=7;
 for(int y=0;y<CG_H;y++){int n=0;for(int x=0;x<CG_W;x++)if(City_LotBase(w,ci,y*CG_W+x)==LK_RUA)n++;if(n>bestR){bestR=n;roadRow=y;}}
 for(int x=0;x<CG_W;x++){int n=0;for(int y=0;y<CG_H;y++)if(City_LotBase(w,ci,y*CG_W+x)==LK_RUA)n++;if(n>bestC){bestC=n;roadCol=x;}}
}
static bool IsRoad(const World *w,int ci,int x,int y){
 if(x<0||y<0||x>=CG_W||y>=CG_H)return false;
 u8 b=City_LotBase(w,ci,y*CG_W+x);return b==LK_RUA||b==LK_PRACA;
}
static bool IsWater(const World *w,int ci,int x,int y){
 if(x<0||y<0||x>=CG_W||y>=CG_H)return false;
 return City_LotBase(w,ci,y*CG_W+x)==LK_AGUA;
}
// chao de um lote (x0,y0 = canto da tela)
static void Ground(const World *w,int ci,int lot,u8 terr,u16 year,u32 seed){
 int lx=lot%CG_W,ly=lot/CG_W,x0=MX+lx*LS,y0=MY+ly*LS;
 u8 base=City_LotBase(w,ci,lot);
 int gr=13,gg=20,gb=9;
 if(terr==WT_DESERT){gr=27;gg=23;gb=14;}else if(terr==WT_TUNDRA||terr==WT_ICE){gr=24;gg=26;gb=26;}
 else if(terr==WT_JUNGLE){gr=8;gg=17;gb=6;}else if(terr==WT_STEPPE){gr=18;gg=20;gb=10;}
 if(base==LK_AGUA){
  for(int y=0;y<LS;y++)for(int x=0;x<LS;x++){
   int X=x0+x,Y=y0+y;int t=B4[Y&3][X&3];
   // agua continua (sem degrau por lote): ondinha pela diagonal + Bayer
   int wv=((X+Y*2)>>2)&3;
   u16 c=C3(8+(t>12)+(wv==0),14+(t>13)+(wv==0),23+(wv==0));
   if(((X*3+Y*7)&31)==0)c=C3(22,26,28);
   Put(X,Y,c);
  }
  // espuma/areia nas bordas com terra
  for(int k=0;k<LS;k++){
   if(!IsWater(w,ci,lx-1,ly)&&lx>0)Put(x0,y0+k,C3(26,27,26));
   if(!IsWater(w,ci,lx+1,ly)&&lx<CG_W-1)Put(x0+LS-1,y0+k,C3(26,27,26));
   if(!IsWater(w,ci,lx,ly-1)&&ly>0)Put(x0+k,y0,C3(26,27,26));
   if(!IsWater(w,ci,lx,ly+1)&&ly<CG_H-1)Put(x0+k,y0+LS-1,C3(26,27,26));
  }
  return;
 }
 // terra: grama pontilhada com tufos
 for(int y=0;y<LS;y++)for(int x=0;x<LS;x++){
  int X=x0+x,Y=y0+y;u32 h=Hh(X*7+Y*131,seed);int v=(h&15)==0?-3:((h&15)==1?2:0);
  int t=B4[Y&3][X&3]>>3;
  Put(X,Y,C3(gr+v+t,gg+v+t,gb+v));
 }
 // praia: terra encostada na agua
 for(int k=0;k<LS;k++){
  if(IsWater(w,ci,lx-1,ly)){Put(x0,y0+k,C3(27,25,18));Put(x0+1,y0+k,C3(25,23,16));}
  if(IsWater(w,ci,lx+1,ly)){Put(x0+LS-1,y0+k,C3(27,25,18));Put(x0+LS-2,y0+k,C3(25,23,16));}
  if(IsWater(w,ci,lx,ly-1)){Put(x0+k,y0,C3(27,25,18));Put(x0+k,y0+1,C3(25,23,16));}
  if(IsWater(w,ci,lx,ly+1)){Put(x0+k,y0+LS-1,C3(27,25,18));Put(x0+k,y0+LS-2,C3(25,23,16));}
 }
 if(base==LK_RUA||base==LK_PRACA){
  bool n=IsRoad(w,ci,lx,ly-1),s=IsRoad(w,ci,lx,ly+1),e=IsRoad(w,ci,lx+1,ly),o=IsRoad(w,ci,lx-1,ly);
  bool asf=year>=1910;
  // leito: faixa de 8px no centro + bracos ate' os vizinhos
  for(int y=0;y<LS;y++)for(int x=0;x<LS;x++){
   bool on=(x>=4&&x<12&&y>=4&&y<12)||(n&&x>=4&&x<12&&y<4)||(s&&x>=4&&x<12&&y>=12)||(o&&y>=4&&y<12&&x<4)||(e&&y>=4&&y<12&&x>=12);
   if(base==LK_PRACA)on=x>=1&&x<15&&y>=1&&y<15;
   if(!on)continue;
   int X=x0+x,Y=y0+y;u16 c;
   if(base==LK_PRACA){c=((X>>1)+(Y>>1))&1?C3(25,23,19):C3(23,21,17);}
   else if(asf)c=C3(9,9,10);
   else{int row=Y/3,off=(row&1)*2;c=((X+off)%4==3||Y%3==2)?C3(13,12,10):C3(19,17,14);}
   Put(X,Y,c);
  }
  // meio-fio
  if(base==LK_RUA){
   u16 cb=C3(24,23,20);
   for(int k=4;k<12;k++){
    if(!n){Put(x0+k,y0+3,cb);}
    if(!s){Put(x0+k,y0+12,cb);}
    if(!o){Put(x0+3,y0+k,cb);}
    if(!e){Put(x0+12,y0+k,cb);}
   }
   if(asf&&(e||o)&&!(n||s))for(int k=1;k<LS;k+=4)Put(x0+k,y0+8,C3(27,25,14));
  }else{
   // praca: chafariz (ou estatua depois de 1850)
   int cx=x0+8,cy=y0+8;
   for(int y=-4;y<=4;y++)for(int x=-4;x<=4;x++){int q=x*x+y*y;
    if(q<=16)Put(cx+x,cy+y,q>=12?C3(20,19,17):C3(10,17,26));}
   Put(cx,cy-2,C3(28,30,31));Put(cx,cy-3,C3(28,30,31));
   if(year>=1850){Put(cx,cy-4,C3(25,20,9));Put(cx,cy-5,C3(25,20,9));}
  }
  return;
 }
 // terra livre: campo (fileiras) ou mata
 u8 dn=DEN[lot];
 if(dn==4){
  u32 h=Hh(seed,lot);bool gold=(h&1);
  for(int y=1;y<LS-1;y++)for(int x=1;x<LS-1;x++){
   if((y&1)==0)Put(x0+x,y0+y,gold?C3(25,21,8):C3(11,20,6));
   else if((x+y)%5==0)Put(x0+x,y0+y,gold?C3(21,17,5):C3(8,16,5));
  }
  for(int k=0;k<LS;k+=3){Put(x0+k,y0,C3(16,12,7));}
 }
}
// casas do lote (densidade 1-3) e a mata (5)
static void Houses(const World *w,int ci,int lot,u8 st,u16 year,u32 seed,int tier,u8 terr){
 int lx=lot%CG_W,ly=lot/CG_W,x0=MX+lx*LS,y0=MY+ly*LS;
 u8 dn=DEN[lot];u32 h=Hh(seed,lot+77);
 int tk=(terr==WT_DESERT||terr==WT_JUNGLE||st==ST_AFRICA||st==ST_ISLAM)?2:(terr==WT_TUNDRA?1:((h>>6)&1?0:3));
 if(dn==5){Tree(x0+4,y0+12,3,tk,h);Tree(x0+11,y0+14,3,tk,h>>4);if(h&1)Tree(x0+8,y0+8,2,tk,h>>8);return;}
 if(dn<1||dn>3)return;
 (void)w;(void)ci;
 if(dn==1){
  OX=x0+2+(int)(h%3);OY=y0+14;UW=11;House(st,year,0,h);
  Tree(x0+13,y0+14,2,tk,h>>3);
 }else if(dn==2){
  OX=x0;OY=y0+15;UW=8;House(st,year,(u8)(tier>1?1:0),h);
  OX=x0+8;OY=y0+15;UW=8;House(st,year,(u8)(tier>1?1:0),h>>5);
 }else{
  // quarteirao denso: predio de 2-4 andares (ou torre depois de 1950)
  OX=x0;OY=y0+15;UW=16;House(st,year,(u8)(tier>=CT_METROPOLE?3:2),h|1);
 }
}
// sombra curta a sudeste de quem tem altura
static void Shadow(int lot){
 int x0=MX+(lot%CG_W)*LS,y0=MY+(lot/CG_W)*LS;
 for(int k=3;k<LS;k++){Darken(x0+k,y0+LS-1,12,16);Darken(x0+LS-1,y0+k,12,16);}
}
static void CityBg(u16 *L,const World *w,int ci){
 u16 *d=L;YOFF=20;D=d;CY0=20;CY1=192;
 const WCity *c=&w->city[ci];
 u8 st=City_Style(w,ci);u16 year=w->date.y;u32 seed=Hh(c->x,c->y);
 u8 terr=W_TERR[World_Idx(w,c->x,c->y)];
 Quarters(w,ci);
 // faixa de 6px acima do mapa: horizonte
 for(int y=20;y<MY;y++)for(int x=0;x<256;x++)Put(x,y,C3(14+(y-20),19+(y-20),26));
 for(int i=0;i<CG_N;i++)Ground(w,ci,i,terr,year,seed);
 u8 nat=c->owner?w->c[c->owner].nat:255;
 // objetos de cima p/ baixo (quem esta' mais embaixo cobre o de cima)
 for(int ly=0;ly<CG_H;ly++)for(int lx=0;lx<CG_W;lx++){
  int i=ly*CG_W+lx,x0=MX+lx*LS,yb=MY+ly*LS+15;
  int s=City_LotSlot(c,i);
  D=d;
  if(c->bq&&c->bqLot==i){Shadow(i);Scaffold(d,x0,yb,LS,100-c->bqDays*100/(BLD[c->bq].days?BLD[c->bq].days:1));}
  else if(s>=0){
   if(c->bld[s]!=BK_PARQUE&&c->bld[s]!=BK_FAZENDA)Shadow(i);
   PxC_Building(d,x0,yb,LS,c->bld[s],st,year,seed+s,-1,nat);
  }
  else if(City_LotBase(w,ci,i)==LK_TERRA){if(DEN[i]>=1&&DEN[i]<=3)Shadow(i);Houses(w,ci,i,st,year,seed,c->tier,terr);}
 }
 // faixa de baixo (186..191)
 for(int y=MY+CG_H*LS;y<192;y++)for(int x=0;x<256;x++)Put(x,y,C_NAVY);
 for(int x=0;x<256;x++)Put(x,MY+CG_H*LS,C_GOLD_DIM);
 CY0=0;CY1=192;YOFF=0;
}
// vivos: gente nas avenidas, carroca/carro, barcos, fumaca, mira do lote
static void CityLive(u16 *d,const World *w,int ci,int tick,int sel){
 const WCity *c=&w->city[ci];u32 seed=Hh(c->x,c->y);u16 year=w->date.y;
 D=d;YOFF=0;CY0=MY;CY1=MY+CG_H*LS;
 static const u8 SKIN[4][3]={{28,22,16},{24,17,11},{18,12,8},{12,8,5}};
 u8 st=City_Style(w,ci);
 int sk0=st==ST_AFRICA?2:(st==ST_ASIA||st==ST_NATIVE||st==ST_ISLAM?1:0);
 // gente: metade na avenida horizontal, metade na vertical; so' pinta em rua
 int np=4+c->pop/25;if(np>28)np=28;
 for(int i=0;i<np;i++){
  u32 h=Hh(seed,i+1000);
  bool hor=i&1;int len=hor?CG_W*LS:CG_H*LS;
  int dir=(h&2)?1:-1,sp=1+(int)((h>>2)%2);
  int p=(int)((h>>4)%(u32)len)+dir*(tick*sp/4);p=((p%len)+len)%len;
  int lane=(int)((h>>12)%5)-2;
  int x=hor?MX+p:MX+roadCol*LS+8+lane,y=hor?MY+roadRow*LS+8+lane:MY+p;
  int lx=(x-MX)/LS,ly=(y-MY)/LS;
  if(!IsRoad(w,ci,lx,ly))continue;
  const u8 *k=SKIN[(sk0+(int)((h>>16)&1))&3];
  u16 cl=C3(6+(int)((h>>18)%22),6+(int)((h>>22)%16),6+(int)((h>>26)%20));
  Put(x,y-2,C3(k[0],k[1],k[2]));Put(x,y-1,cl);Put(x,y,cl);Put(x+1,y,Dk(cl,3,4));
  Put(x,y+1,((tick/6+i)&1)?C3(5,5,6):cl);
 }
 // carroca (carro depois de 1910) na avenida horizontal
 if(c->tier>=CT_VILAREJO){
  int len=CG_W*LS,p=(tick/2+(int)(seed%len))%len,x=MX+p,y=MY+roadRow*LS+5;
  if(IsRoad(w,ci,(x-MX)/LS,roadRow)){
   if(year>=1910){Fill(x,y,x+6,y+3,C3(20,4,4));Fill(x+1,y-2,x+5,y,C3(14,16,18));}
   else{Fill(x,y,x+5,y+3,C3(18,12,6));Fill(x+5,y,x+8,y+2,C3(14,9,5));}
  }
 }
 // barcos: ate' 3 lotes de agua (sorteio fixo), balancando
 int nb=0;
 for(int i=0;i<CG_N&&nb<1+c->tier;i++){
  if(City_LotBase(w,ci,i)!=LK_AGUA||(Hh(seed,i+3000)%5))continue;
  int x=MX+(i%CG_W)*LS+4+((tick/30+i)%5),y=MY+(i/CG_W)*LS+10+((tick/20+i)&1);
  for(int k=0;k<8;k++)Put(x+k,y,C3(12,8,5));
  for(int k=1;k<7;k++)Put(x+k,y+1,C3(12,8,5));
  if(year<1880){for(int j=1;j<6;j++)for(int k=0;k<j/2+1;k++)Put(x+4-k,y-j,C3(29,28,24));}
  else{Fill(x+2,y-3,x+6,y,C3(20,20,22));Put(x+4,y-4,C3(22,7,5));}
  nb++;
 }
 // fumaca das chamines
 for(int s=0;s<CITY_SLOTS;s++){
  u8 k=c->bld[s];if(c->bpos[s]>=CG_N)continue;
  if(k!=BK_OFICINA&&k!=BK_FABRICA&&k!=BK_USINA&&k!=BK_ARSENAL&&k!=BK_SERRARIA)continue;
  OX=MX+(c->bpos[s]%CG_W)*LS;OY=MY+(c->bpos[s]/CG_W)*LS+15;UW=LS;
  if(k==BK_FABRICA){Smoke(6,34,tick,seed+s);Smoke(23,40,tick,seed+s+5);}
  else if(k==BK_USINA)Smoke(26,40,tick,seed+s);
  else Smoke(22,26,tick,seed+s);
 }
 // mira do lote: moldura ouro que pulsa
 if(sel>=0&&sel<CG_N){
  int x0=MX+(sel%CG_W)*LS,y0=MY+(sel/CG_W)*LS;bool ph=(tick/16)&1;
  u16 a=ph?C_GOLD_HI:C_WHITE;
  for(int k=0;k<LS;k++){
   if(k<4||k>=LS-4){Put(x0+k,y0,a);Put(x0+k,y0+LS-1,a);Put(x0,y0+k,a);Put(x0+LS-1,y0+k,a);}
  }
  for(int k=1;k<LS-1;k++){if(k<4||k>=LS-4){Put(x0+k,y0+1,C_INK);Put(x0+k,y0+LS-2,C_INK);Put(x0+1,y0+k,C_INK);Put(x0+LS-2,y0+k,C_INK);}}
 }
 CY0=0;CY1=192;
}
void PxC_Scene(u16 *d,int ci){
 const World *w=WG_World();
 if(ci<0||ci>=w->nCity)return;
 u16 *L=PxM_Scratch();
 u32 sig=SceneSig(w,ci);
 if(sceneCity!=ci||sceneSig!=sig){CityBg(L,w,ci);sceneCity=ci;sceneSig=sig;}
 Px_Copy16(d+WG_MAP_Y*256,L,WG_MAP_H*256);   // VRAM: so' 16/32 bits
 Quarters(w,ci);      // DEN/avenidas da cidade aberta (baratinho: 160 lotes)
 CityLive(d,w,ci,(int)WG_Ptr()->tick,WG_Ptr()->cityLot);
 // faixa do titulo: nome + nivel + bandeira
 const WCity *c=&w->city[ci];
 Px_Rect(d,0,0,256,20,C_NAVY);Px_Rect(d,0,19,256,1,C_GOLD);
 if(c->owner){Px_Rect(d,5,4,18,12,C_GOLD);D=d;NatFlag(w->c[c->owner].nat,6,5,16,10);}
 char nm[40];City_Name(w,ci,nm,40);
 Px_Text2(d,28,4,nm,&F_SERIF12,C_TEXT);
 const char *tn=Str((u16)(STR_TIER_0+c->tier));
 Px_Text2(d,250-Px_Width2(tn,&F_SERIF8),6,tn,&F_SERIF8,C_GOLD);
}
void PxC_SceneReset(void){sceneCity=-1;}
u8 PxC_LotDen(const World *w,int ci,int lot){
 if(sceneCity!=ci)Quarters(w,ci);
 return lot>=0&&lot<CG_N?DEN[lot]:0;
}

// ======================= painel da cidade (tela de baixo) =======================
static void Cat(char *dst,int max,const char *s){int n=0;while(dst[n])n++;while(*s&&n<max-1)dst[n++]=*s++;dst[n]=0;}
static void Num(char *dst,int max,s32 v){char t[16];Px_Itoa((int)v,t);Cat(dst,max,t);}
static void FmtPop(char *o,int max,u16 pop){
 o[0]=0;
 if(pop>=1000){Num(o,max,pop/1000);Cat(o,max,",");char t[4]={(char)('0'+(pop%1000)/100),0};Cat(o,max,t);Cat(o,max," M");}
 else{Num(o,max,pop);Cat(o,max,Str(STR_MIL));}
}
// por que a obra k nao pode ser feita no lote (StrId) ou 0
static u16 WhyNot(const World *w,int ci,int lot,u8 k,u16 *year){
 const WCity *c=&w->city[ci];const BldDef *b=&BLD[k];
 if(City_Count(c,k)>=City_MaxOf(k))return City_MaxOf(k)>1?STR_LIMITE_OBRA:STR_JA_TEM;
 if(c->tier<b->minTier)return STR_REQ_NIVEL;
 if(w->date.y<b->minYear){*year=b->minYear;return STR_REQ_ANO;}
 if(b->need){bool h=false;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==b->need)h=true;if(!h)return STR_REQ_OBRA;}
 if((b->flags&BF_COAST)&&!(c->flags&CF_COASTAL))return STR_SO_COSTA;
 if((b->flags&BF_CAPITAL)&&!City_IsCapital(w,ci))return STR_SO_CAPITAL;
 if((b->flags&BF_WOOD)&&c->wood<6)return STR_REQ_RECURSO;
 if((b->flags&BF_ORE)&&c->ore<6)return STR_REQ_RECURSO;
 if((b->flags&BF_OIL)&&!(c->res&RS_OLEO))return STR_REQ_RECURSO;
 int used=0;for(int s=0;s<City_Slots(c->tier);s++)if(c->bld[s])used++;
 if(used>=City_Slots(c->tier))return STR_SEM_SLOT;
 if(City_LotBase(w,ci,lot)!=LK_TERRA)return STR_D_LOTE_RUA;
 if(City_LotSlot(c,lot)>=0||(c->bq&&c->bqLot==lot))return STR_LOTE_OCUPADO;
 if((b->flags&BF_COAST)&&!City_LotCoast(w,ci,lot))return STR_SO_LOTE_AGUA;
 return 0;
}
static u16 LotCol(const World *w,int ci,int i){
 const WCity *c=&w->city[ci];
 if(c->bq&&c->bqLot==i)return C3(28,16,6);
 if(City_LotSlot(c,i)>=0)return C_GOLD;
 switch(City_LotBase(w,ci,i)){
  case LK_AGUA:return C3(8,14,24);
  case LK_RUA:return C3(18,17,15);
  case LK_PRACA:return C3(25,23,19);
  default:break;
 }
 switch(DEN[i]){case 1:return C3(20,12,8);case 2:return C3(17,9,6);case 3:return C3(13,7,5);
  case 4:return C3(20,20,7);case 5:return C3(5,12,5);default:return C3(10,17,8);}
}
void PxC_Panel(u16 *d,int ci,int lot,bool picking,u8 pk){
 const World *w=WG_World();
 Ui_Backdrop(d);
 if(ci<0||ci>=w->nCity)return;
 const WCity *c=&w->city[ci];
 u8 st=City_Style(w,ci);u16 year=w->date.y;u32 seed=Hh(c->x,c->y);
 u8 nat=c->owner?w->c[c->owner].nat:255;
 if(sceneCity!=ci)Quarters(w,ci);
 // cabecalho: populacao, comida, ouro e rumo ao proximo nivel
 Ui_Navy(d,2,2,252,38);
 char t[112];
 t[0]=0;Cat(t,112,Str(STR_POPULACAO));Cat(t,112," ");char p[24];FmtPop(p,24,c->pop);Cat(t,112,p);
 Px_Text2(d,8,6,t,&F_SERIF8,C_TEXT);
 int gold=c->tier;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s])gold+=BLD[c->bld[s]].gold;
 t[0]=0;Cat(t,112,Str(STR_COMIDA));Cat(t,112," ");Num(t,112,c->food);Cat(t,112,"% · +");Num(t,112,gold);Cat(t,112,Str(STR_OURO_DIA));
 Px_Text2(d,248-Px_Width2(t,&F_SERIF8),6,t,&F_SERIF8,c->food<60?C_EVENT:C_GOLD);
 int np=City_NextPop(c->tier);
 if(np){t[0]=0;Cat(t,112,Str(STR_RUMO));Cat(t,112,Str((u16)(STR_TIER_0+c->tier+1)));Cat(t,112,": ");Num(t,112,c->pop);Cat(t,112,"/");Num(t,112,np);Cat(t,112,Str(STR_MIL));
  Px_Text2(d,8,18,t,&F_SERIF8,C_TEXT_DIM);Ui_Bar(d,8,31,240,c->pop*100/np,C_GOLD);}
 else{Px_Text2(d,8,18,Str(STR_TIER_3),&F_SERIF8,C_GOLD);Ui_Bar(d,8,31,240,100,C_GOLD);}
 // minimapa dos lotes (toque escolhe o lote)
 Px_Rect(d,CY_MINI_X-2,CY_MINI_Y-2,CG_W*CY_CELL+4,CG_H*CY_CELL+4,C_GOLD_DIM);
 for(int i=0;i<CG_N;i++){
  int x=CY_MINI_X+(i%CG_W)*CY_CELL,y=CY_MINI_Y+(i/CG_W)*CY_CELL;
  Px_Rect(d,x,y,CY_CELL,CY_CELL,LotCol(w,ci,i));
  if(City_LotSlot(c,i)>=0)Px_Border(d,x,y,CY_CELL,CY_CELL,C3(14,10,4));
 }
 if(lot>=0&&lot<CG_N){int x=CY_MINI_X+(lot%CG_W)*CY_CELL,y=CY_MINI_Y+(lot/CG_W)*CY_CELL;
  Px_Border(d,x-1,y-1,CY_CELL+2,CY_CELL+2,C_CRIMSON_HI);Px_Border(d,x-2,y-2,CY_CELL+4,CY_CELL+4,C_INK);}
 // ficha do lote
 int ix=CY_MINI_X+CG_W*CY_CELL+6,iw=252-ix;
 Ui_Navy(d,ix,CY_MINI_Y-2,iw,CG_H*CY_CELL+4);
 int s=City_LotSlot(c,lot);u8 base=City_LotBase(w,ci,lot);
 u16 tt,dd;
 bool bq=c->bq&&c->bqLot==lot;
 if(bq){tt=BLD[c->bq].nameStr;dd=BLD[c->bq].descStr;}
 else if(s>=0){tt=BLD[c->bld[s]].nameStr;dd=BLD[c->bld[s]].descStr;}
 else if(base==LK_AGUA){tt=STR_LOTE_AGUA;dd=STR_D_LOTE_RUA;}
 else if(base==LK_RUA){tt=STR_LOTE_RUA;dd=STR_D_LOTE_RUA;}
 else if(base==LK_PRACA){tt=STR_LOTE_PRACA;dd=STR_D_LOTE_RUA;}
 else if(DEN[lot]>=1&&DEN[lot]<=3){tt=STR_LOTE_CASAS;dd=STR_D_LOTE_CASAS;}
 else if(DEN[lot]==4){tt=STR_LOTE_CAMPO;dd=STR_D_LOTE_LIVRE;}
 else{tt=STR_LOTE_LIVRE;dd=STR_D_LOTE_LIVRE;}
 Px_WrapN(d,ix+5,CY_MINI_Y+2,iw-10,Str(tt),&F_SERIF8,C_GOLD,1,false);
 if(bq){t[0]=0;Cat(t,112,Str(STR_EM_OBRA));Cat(t,112,": ");Num(t,112,c->bqDays);Cat(t,112,Str(STR_DIAS));
  Px_WrapN(d,ix+5,CY_MINI_Y+14,iw-10,t,&F_SERIF8,C_TEXT,1,false);}
 Px_WrapN(d,ix+5,CY_MINI_Y+(bq?26:14),iw-10,Str(dd),&F_SERIF8,C_TEXT_DIM,bq?3:4,false);
 if(s>=0&&City_IndustryBonus(c,s))Px_WrapN(d,ix+5,CY_MINI_Y+58,iw-10,Str(STR_VIZ_BONUS),&F_SERIF8,C_PLAYER,1,false);
 // acao / catalogo
 if(picking){
  const BldDef *b=&BLD[pk];
  Ui_Navy(d,2,CY_PICK_Y,252,190-CY_PICK_Y);
  Ui_Arrows(d,10,240,CY_PICK_Y+18,C_GOLD);
  PxC_Building(d,24,CY_PICK_Y+34,26,pk,st,year,seed,-1,nat);
  t[0]=0;Cat(t,112,Str(b->nameStr));
  Px_Text2(d,56,CY_PICK_Y+5,t,&F_SERIF12,C_TEXT);
  t[0]=0;Num(t,112,City_Cost(w,ci,pk));Cat(t,112," ");Cat(t,112,Str(STR_OURO));Cat(t,112," · ");Num(t,112,b->days);Cat(t,112,Str(STR_DIAS));
  Px_Text2(d,56,CY_PICK_Y+20,t,&F_SERIF8,C_GOLD);
  u16 yr=0;u16 why=WhyNot(w,ci,lot,pk,&yr);
  if(why){t[0]=0;Cat(t,112,Str(why));if(why==STR_REQ_ANO)Num(t,112,yr);
   else if(why==STR_REQ_NIVEL)Cat(t,112,Str((u16)(STR_TIER_0+b->minTier)));
   else if(why==STR_REQ_OBRA)Cat(t,112,Str(BLD[b->need].nameStr));
   Px_WrapN(d,56,CY_PICK_Y+31,186,t,&F_SERIF8,C_EVENT,1,false);}
  else Px_WrapN(d,56,CY_PICK_Y+31,186,Str(b->descStr),&F_SERIF8,C_TEXT_DIM,1,false);
  bool ok=!why&&w->c[c->owner].gold>=City_Cost(w,ci,pk);
  Px_Rect(d,CY_OK_X,CY_OK_Y,CY_OK_W,CY_OK_H,ok?C_CRIMSON:C_NAVY_HI);Px_Border(d,CY_OK_X,CY_OK_Y,CY_OK_W,CY_OK_H,C_GOLD);
  Ui_TextC(d,CY_OK_X+CY_OK_W/2,CY_OK_Y+7,Str(STR_CONSTRUIR),0,ok?C_TEXT:C_TEXT_DIM);
  Px_Rect(d,CY_NO_X,CY_OK_Y,CY_NO_W,CY_OK_H,C_NAVY);Px_Border(d,CY_NO_X,CY_OK_Y,CY_NO_W,CY_OK_H,C_GOLD_DIM);
  Ui_TextC(d,CY_NO_X+CY_NO_W/2,CY_OK_Y+7,Str(STR_CANCELAR),0,C_TEXT);
  return;
 }
 // botao do lote: construir aqui / lancar navio
 bool canHere=!bq&&s<0&&base==LK_TERRA;
 if(s>=0&&c->bld[s]==BK_ESTALEIRO){
  const WCountry *C=&w->c[c->owner];
  t[0]=0;Cat(t,112,Str(STR_A_NAVIO));Cat(t,112," · ");Num(t,112,WEco_ShipCost(w,c->owner));Cat(t,112," ");Cat(t,112,Str(STR_OURO));
  Px_Rect(d,4,CY_BTN_Y,248,CY_BTN_H,C->shipQ?C_NAVY_HI:C_CRIMSON);Px_Border(d,4,CY_BTN_Y,248,CY_BTN_H,C_GOLD);
  Ui_TextC(d,128,CY_BTN_Y+8,t,0,C_TEXT);
 }else if(canHere){
  Px_Rect(d,4,CY_BTN_Y,248,CY_BTN_H,C_CRIMSON);Px_Border(d,4,CY_BTN_Y,248,CY_BTN_H,C_GOLD);
  for(int k=1;k<247;k++)Px_PSet(d,4+k,CY_BTN_Y+1,C_CRIMSON_HI);
  Ui_TextC(d,128,CY_BTN_Y+8,Str(STR_CONSTRUIR_AQUI),0,C_TEXT);
 }
 // producao da cidade + dica
 s32 pr[GD_COUNT],nd[GD_COUNT],dm[GD_COUNT];WEco_CityFlows(w,ci,pr,nd,dm);
 t[0]=0;Cat(t,112,Str(STR_PRODUZ));Cat(t,112,":");int sh=0;
 for(int r=0;r<3;r++){
  int bg=-1;s32 bv=0;
  for(int g=0;g<GD_COUNT;g++)if(pr[g]>bv){bv=pr[g];bg=g;}
  if(bg<0)break;
  Cat(t,112," ");Cat(t,112,Str(Gd_Str((u8)bg)));Cat(t,112," +");Num(t,112,bv);pr[bg]=0;sh++;
 }
 if(!sh)Cat(t,112," —");
 Cat(t,112,Str(STR_POR_SEM));
 Px_WrapN(d,6,CY_BTN_Y+CY_BTN_H+5,244,t,&F_SERIF8,C_TEXT_DIM,1,false);
 Px_WrapN(d,6,CY_BTN_Y+CY_BTN_H+18,244,Str(STR_DICA_CIDADE),&F_SERIF8,C_GOLD_DIM,2,false);
}
