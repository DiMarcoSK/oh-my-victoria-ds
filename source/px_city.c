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
static void MiniTown(u16 *d,int cx,int cy,int size,const WCity *c,u8 st,u16 year,u32 seed,u8 nat){
 int n=1+c->tier*2;if(size<14)n=1+c->tier;
 for(int i=0;i<n;i++){
  u32 h=Hh(seed,i);
  int dx=(int)(h%(u32)size)-size/2,dy=(int)((h>>8)%(u32)(size/2+1))-size/4;
  int hw=size/2+(int)(h>>20)%3;
  D=d;OX=cx+dx-hw/2;OY=cy+dy+size/4;UW=hw;
  House(st,year,c->tier,h);
 }
 // edificio-simbolo no centro (templo/palacio) a partir de cidade
 u8 k=0;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==BK_PALACIO||(c->bld[s]==BK_TEMPLO&&!k))k=c->bld[s];
 if(k&&c->tier>=CT_CIDADE)PxC_Building(d,cx-size/3,cy+size/4+1,size*2/3,k,st,year,seed,-1,nat);
}
static inline bool ScreenOf(const World *w,const WGame *s,int cx,int cy,int *px,int *py){
 int rx=cx-s->camX;if(w->wrap){rx%=w->w;if(rx<0)rx+=w->w;}
 int ry=cy-s->camY;
 if(s->zoom==0){*px=rx/2;*py=ry/2;}
 else{int ppc=1<<(s->zoom-1);*px=rx*ppc+ppc/2;*py=ry*ppc+ppc/2;}
 *py+=WG_MAP_Y;
 return *px>-30&&*px<286&&*py>WG_MAP_Y-20&&*py<212;
}
void PxC_MapCities(u16 *d,const World *w,const WGame *s){
 D=d;YOFF=0;CY0=WG_MAP_Y;CY1=192;
 int ppc=s->zoom?1<<(s->zoom-1):0;
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
  // Z3/Z4: arredores com telhadinhos esparsos (so' em terra, pelo KEY do
  // cache) — a cidade "espalha" com o nivel — e a vila no centro
  if(c->tier>=CT_VILAREJO){
   int R=(c->tier+1)*ppc/2;
   const u8 *K=PxM_Key();
   for(int k=0;k<3+c->tier*4;k++){
    u32 h=Hh(seed,k+99);
    int dx=(int)(h%(u32)(2*R+1))-R,dy=(int)((h>>10)%(u32)(2*R+1))-R;
    if(dx*dx+dy*dy>R*R)continue;
    int x=px+dx,y=py+dy;
    if(y<WG_MAP_Y||y>=191||x<0||x>=255)continue;
    if(K[(y-WG_MAP_Y)*256+x]>=0xFE)continue;          // mar ou fora do mapa
    u16 roof=(h>>24)&1?C3(19,9,7):C3(14,13,14);
    Put(x,y,roof);Put(x+1,y,roof);Put(x,y+1,Dk(roof,5,8));Put(x+1,y+1,C3(25,23,18));
   }
  }
  int size=s->zoom==3?(c->tier>=CT_CIDADE?12:8):(c->tier>=CT_CIDADE?24:16);
  if(c->tier==CT_POVOADO&&s->zoom==3){D=d;OX=px-3;OY=py+2;UW=7;House(st,year,0,seed);continue;}
  MiniTown(d,px,py,size,c,st,year,seed,nat);
 }
 CY0=0;CY1=192;
}

// ======================= cena da cidade (Imperator) =======================
// layout em coordenadas da tela (0..191); o fundo estatico vive no LAYER
// (linhas 20..191 da tela) e o titulo 0..19 e' desenhado ao vivo.
enum { HORIZ=82, HOUSE_Y=106, BACK_Y=128, ROAD_Y0=130, ROAD_Y1=144, RAIL_Y=148, FRONT_Y=190 };
static const s16 SLOT_X[CITY_SLOTS]={4,67,130,193, 16,74,132,190};
static const u8 SLOT_W[CITY_SLOTS]={56,56,56,56, 46,46,46,46};
static int SlotBase(int s){return s<4?FRONT_Y:BACK_Y;}
static u32 sceneSig;static int sceneCity=-1;
static u32 SceneSig(const World *w,int ci){
 const WCity *c=&w->city[ci];
 u32 h=Hh(ci,c->tier)^(c->pop/8)^((u32)c->bq<<12)^((u32)(c->bqDays/10)<<16)^(w->date.y/10);
 for(int s=0;s<CITY_SLOTS;s++)h=h*31+c->bld[s];
 return h^((u32)c->owner<<24);
}
// ---- pintura da cena: helpers (so' inteiro; tudo cai no LAYER em cache) ----
static const u8 B4[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
// cor em 1/16 de passo por canal + Bayer 4x4: degrade liso em 15 bits
static u16 Dth(int r16,int g16,int b16,int x,int y){
 int t=B4[y&3][x&3];
 return C3((r16+t)>>4,(g16+t)>>4,(b16+t)>>4);
}
// mistura a->b em t/n (canais 0..31) devolvida em 1/16 de passo
static void Mix16(const u8 *a,const u8 *b,int t,int n,int *o){
 for(int k=0;k<3;k++)o[k]=(a[k]*16*(n-t)+b[k]*16*t)/(n?n:1);
}
static u16 Get(int x,int y){if(x<0||x>255||y<CY0||y>=CY1)return 0;return D[(y-YOFF)*256+x];}
static void Darken(int x,int y,int num,int den){u16 c=Get(x,y);if(c)Put(x,y,Dk(c,num,den));}
// arvore por clima: 0 copa redonda, 1 conifera, 2 palmeira, 3 choupo
static void Tree(int x,int yb,int sz,int kind,u32 h){
 u16 trunk=C3(10,7,4);
 if(kind==2){                                   // palmeira: tronco curvo + leque
  sz+=3;
  for(int k=0;k<sz*2;k++){Put(x+(k*k)/(sz*6),yb-k,trunk);Put(x+1+(k*k)/(sz*6),yb-k,C3(14,10,6));}
  int tx=x+(4*sz*sz)/(sz*6),ty=yb-sz*2;
  static const s8 FX[6]={-5,-3,0,3,5,1},FY[6]={2,-2,-3,-2,2,3};
  for(int f=0;f<6;f++)for(int k=0;k<=sz;k++)Put(tx+FX[f]*k/5,ty+FY[f]*k/5+(k*k)/(sz*3),k<2?C3(6,13,5):C3(8,17,6));
  return;
 }
 if(kind==1){                                   // conifera: triangulos empilhados
  for(int y=0;y<sz*2;y++){int hw=1+(y%(sz/2+1))*sz/(sz/2+2)/2+y/4;
   for(int i=-hw;i<=hw;i++)Put(x+i,yb-sz*2-1+y,i<0?C3(5,11,7):C3(3,8,5));}
  Put(x,yb,trunk);Put(x,yb-1,trunk);return;
 }
 if(kind==3){                                   // choupo: elipse alta
  for(int y=-sz*2;y<=0;y++){int hw=(sz*(sz*2-(y<0?-y:y)/1))/(sz*2)/2+1;if(y<-sz*2+2)hw=1;
   for(int i=-hw;i<=hw;i++)Put(x+i,yb-3+y,i<0?C3(9,17,7):C3(6,12,5));}
  Put(x,yb-1,trunk);Put(x,yb-2,trunk);return;
 }
 for(int k=0;k<3;k++)Put(x,yb-k,trunk);       // copa redonda: 3 bolas com luz NO
 int r=sz;
 static const s8 OX[3]={-1,1,0},OY[3]={0,0,-1};
 for(int b=0;b<3;b++){int cx=x+OX[b]*r*2/3,cy=yb-2-r+OY[b]*r*2/3;
  for(int y=-r;y<=r;y++)for(int i=-r;i<=r;i++){int q=i*i+y*y;if(q>r*r)continue;
   int lit=(i+y)<-r/2,dark=(i+y)>r/2;
   u16 c=lit?C3(11,20,8):dark?C3(5,11,4):C3(8,16,6);
   if(((h>>(b*3+(i&3)))&5)==0&&q<r*r-r)c=C3(10,18,7);
   Put(cx+i,cy+y,c);}}
}
static void Cloud(int cx,int cy,int w,u32 h){
 int n=3+(int)(h%3);
 for(int k=0;k<n;k++){
  int r=4+(int)((h>>(k*3))%5)+(k==n/2?3:0),x=cx-w/2+k*w/(n-1?n-1:1),y=cy-(k==n/2?2:0);
  for(int j=-r;j<=r/2;j++)for(int i=-r;i<=r;i++){if(i*i+j*j>r*r)continue;
   int sh=j>r/4?2:(j>-r/3?1:0);
   u16 c=sh==0?C3(31,31,30):sh==1?C3(28,28,29):C3(23,24,27);
   Put(x+i,y+j,c);}
 }
}
static void SceneBg(u16 *L,const World *w,int ci){
 // L e' um buffer 256x172 = linhas 20..191 da tela; desenhamos num alvo
 // de tela "virtual": D aponta 20 linhas antes p/ usar coordenadas de tela
 u16 *d=L;YOFF=20;
 const WCity *c=&w->city[ci];
 u8 st=City_Style(w,ci);u16 year=w->date.y;u32 seed=Hh(c->x,c->y);
 CY0=20;CY1=192;D=d;
 u8 t=W_TERR[World_Idx(w,c->x,c->y)];
 bool mount=false,hills=false,forest=false,desert=t==WT_DESERT,cold=t==WT_TUNDRA||t==WT_ICE,jungle=t==WT_JUNGLE;
 for(int dy=-6;dy<=6;dy++)for(int dx=-6;dx<=6;dx++){
  int x=c->x+dx,y=c->y+dy;if(y<0||y>=w->h)continue;if(x<0||x>=w->w){if(!w->wrap)continue;x=(x+w->w)%w->w;}
  u8 q=W_TERR[World_Idx(w,x,y)];if(q==WT_MOUNTAIN)mount=true;if(q==WT_HILLS)hills=true;if(q==WT_FOREST||q==WT_JUNGLE)forest=true;}
 // ceu: azul fundo -> azul claro -> bruma quente no horizonte (Bayer em 1/16)
 static const u8 SK0[3]={7,12,23},SK1[3]={15,21,29},SK2[3]={27,26,23};
 const u8 *HZ=desert?(const u8[3]){29,25,19}:SK2;
 int o[3];
 for(int y=20;y<HORIZ;y++){
  int f=y-20,n=HORIZ-20;
  if(f<n*2/3)Mix16(SK0,SK1,f,n*2/3,o);else Mix16(SK1,HZ,f-n*2/3,n-n*2/3,o);
  for(int x=0;x<256;x++)Put(x,y,Dth(o[0],o[1],o[2],x,y));
 }
 // sol com halo (mistura com o ceu por distancia)
 int sx=150+(int)(seed%80),sy=34+(int)((seed>>8)%10);
 for(int y=sy-22;y<sy+22&&y<HORIZ;y++)for(int x=sx-22;x<sx+22;x++){
  int q=(x-sx)*(x-sx)+(y-sy)*(y-sy);if(q>22*22)continue;
  if(q<=36){Put(x,y,C3(31,31,26));continue;}
  u16 cc=Get(x,y);int k=(22*22-q)*6/(22*22);
  int r=(cc&31)*16+k*((31-(cc&31))*16)/8,g=((cc>>5)&31)*16+k*((30-((cc>>5)&31))*16)/8,b=((cc>>10)&31)*16+k*((22-((cc>>10)&31))*16)/8;
  Put(x,y,Dth(r<0?0:r,g<0?0:g,b<0?0:b,x,y));
 }
 // nuvens fofas (3 tons, luz de cima)
 for(int n=0;n<4;n++){u32 h=Hh(seed,n);Cloud((int)(h%256),30+(int)((h>>8)%22),20+(int)((h>>16)%20),h>>4);}
 // 2 camadas de relevo: a distante some na bruma (perspectiva aerea)
 static const s8 WAVE[16]={0,2,4,5,6,5,4,2,0,-2,-4,-5,-6,-5,-4,-2};
 for(int layer=0;layer<2;layer++){
  int haze=layer?5:10;   // de 16
  u8 base[3];
  if(mount){base[0]=13;base[1]=15;base[2]=20;}
  else if(desert){base[0]=24;base[1]=19;base[2]=13;}
  else if(cold){base[0]=22;base[1]=24;base[2]=26;}
  else{base[0]=10;base[1]=16;base[2]=12;}
  for(int x=0;x<256;x++){
   int hh;
   if(layer==0)hh=mount?(int)(16+((Hh(x/22,seed)%24))*(1+(x%22<11?x%22:22-x%22))/11):(hills?13:8)+WAVE[(x/9+(int)(seed%16))&15]+WAVE[(x/23)&15]/2;
   else hh=(hills||mount?8:4)+WAVE[(x/6+(int)(seed>>4)%16)&15]/2+WAVE[(x/17+3)&15]/2;
   if(hh<1)hh=1;
   for(int y=HORIZ-hh;y<HORIZ;y++){
    int m[3];Mix16(base,HZ,haze,16,m);
    // sombra no lado direito das encostas + neve no pico das montanhas
    int slope=layer==0&&mount&&(x%22)>=11;
    if(slope){m[0]=m[0]*13/16;m[1]=m[1]*13/16;m[2]=m[2]*14/16;}
    if(layer==0&&mount&&y<HORIZ-hh+3&&hh>22){m[0]=29*16;m[1]=30*16;m[2]=31*16;}
    Put(x,y,Dth(m[0],m[1],m[2],x,y));
   }
   // fileira de mata na camada da frente
   if(layer==1&&(forest||jungle)&&!desert&&(Hh(x,seed+3)&3)==0){int r=2+(int)(Hh(x,seed)%2);
    for(int j=-r;j<=0;j++)for(int i=-r;i<=r;i++)if(i*i+j*j<=r*r)Put(x+i,HORIZ-hh+j,cold?C3(6,12,9):C3(7,13,7));}
  }
 }
 // chao: bruma clara no horizonte -> cor cheia na frente, tufos de grama
 u8 g0[3],g1[3];
 if(desert){g0[0]=28;g0[1]=24;g0[2]=17;g1[0]=26;g1[1]=20;g1[2]=12;}
 else if(cold){g0[0]=25;g0[1]=27;g0[2]=27;g1[0]=21;g1[1]=24;g1[2]=24;}
 else if(jungle){g0[0]=12;g0[1]=19;g0[2]=10;g1[0]=6;g1[1]=14;g1[2]=5;}
 else if(t==WT_STEPPE){g0[0]=20;g0[1]=21;g0[2]=12;g1[0]=16;g1[1]=17;g1[2]=7;}
 else{g0[0]=16;g0[1]=22;g0[2]=12;g1[0]=9;g1[1]=17;g1[2]=6;}
 for(int y=HORIZ;y<192;y++){
  Mix16(g0,g1,y-HORIZ,192-HORIZ,o);
  for(int x=0;x<256;x++){
   u32 h=Hh(x*7+y*131,seed);int v=0;
   if((h&15)==0)v=-24;else if((h&15)==1)v=20;           // tufo escuro / folha clara
   else if((h&511)==2&&y>150&&!desert&&!cold){Put(x,y,(h>>8)&1?C3(30,28,10):C3(30,30,28));continue;}  // florzinha
   Put(x,y,Dth(o[0]+v,o[1]+v,o[2]+v,x,y));
  }
 }
 // mar (cidade costeira): agua com reflexo do ceu, cintilacao e o sol na agua
 if(c->flags&CF_COASTAL){
  static const u8 W0[3]={17,22,27},W1[3]={6,12,21};
  for(int y=HORIZ-1;y<ROAD_Y0-2;y++){
   int x0=180-(y-HORIZ)/2;
   Mix16(W0,W1,y-HORIZ+1,ROAD_Y0-1-HORIZ,o);
   for(int x=x0;x<256;x++){
    u32 h=Hh(x/3+y*977,seed);
    u16 col=Dth(o[0],o[1],o[2],x,y);
    if((h&31)==0)col=C3(27,29,30);                     // crista
    int ds=x-sx;if(ds<0)ds=-ds;
    if(ds<6+(y-HORIZ)/3&&((h>>5)&3)==0)col=C3(31,29,20);  // reflexo do sol
    Put(x,y,col);
   }
   Put(x0,y,C3(28,25,18));Put(x0+1,y,C3(26,23,16));    // praia
  }
 }
 // casario ao fundo (cidade grande): fileira de predios menores, mais densa
 if(c->tier>=CT_CIDADE){
  int n2=c->tier*10;
  for(int i=0;i<n2;i++){u32 h=Hh(seed,i+300);
   int x=(int)(h%250),y=HOUSE_Y-14-(int)((h>>8)%4);
   if((c->flags&CF_COASTAL)&&x>165)x-=100;
   OX=x;OY=y;UW=9+(int)((h>>16)%4)+c->tier;D=d;House(st,year,c->tier,h);}
 }
 int tk=desert||st==ST_ISLAM?2:(jungle||st==ST_AFRICA?2:(cold?1:((seed&1)?0:3)));
 // arvores entre as casas (atras)
 for(int i=0;i<6;i++){u32 h=Hh(seed,i+40);int x=(int)(h%250);
  if((c->flags&CF_COASTAL)&&x>165)x-=110;
  Tree(x,HOUSE_Y-2-(int)((h>>8)%6),3,tk,h);}
 // casario: quantidade e altura seguem a populacao e o nivel
 int nh=4+c->pop/6;if(nh>34)nh=34;
 for(int i=0;i<nh;i++){
  u32 h=Hh(seed,i+7);
  int x=(int)(h%240),y=HOUSE_Y-(int)((h>>8)%14);
  if((c->flags&CF_COASTAL)&&x>165)x-=90;
  OX=x;OY=y;UW=12+(int)((h>>16)%6)+c->tier*2;D=d;
  // sombra curta no chao a' direita (sol a oeste/alto)
  for(int k=0;k<UW*28/32;k++){Darken(x+4+k,y,13,16);Darken(x+5+k,y+1,14,16);}
  House(st,year,c->tier,h);
 }
 // calcadas + rua (paralelepipedo em fiadas; asfalto com faixa apos 1910)
 for(int x=0;x<256;x++){Put(x,ROAD_Y0-2,C3(24,23,20));Put(x,ROAD_Y0-1,C3(19,18,16));Put(x,ROAD_Y1,C3(24,23,20));Put(x,ROAD_Y1+1,C3(17,16,14));}
 for(int y=ROAD_Y0;y<ROAD_Y1;y++)for(int x=0;x<256;x++){
  u16 col;
  if(year>=1910){u32 h=Hh(x+y*256,seed);int v=(h&7)==0?1:0;col=C3(9+v,9+v,10+v);
   if(y==(ROAD_Y0+ROAD_Y1)/2&&(x&8))col=C3(27,25,15);}
  else{
   int row=(y-ROAD_Y0)/3,off=(row&1)*3,sx2=(x+off)%6;
   if((y-ROAD_Y0)%3==2||sx2==5)col=C3(12,11,10);          // rejunte
   else{u32 h=Hh((x+off)/6+row*61,seed);int v=(int)(h%4);col=C3(17+v,15+v,12+v);}
   if(sx2==0&&(y-ROAD_Y0)%3==0)col=C3(22,20,17);           // brilho na quina
  }
  Put(x,y,col);
 }
 // lampioes na calcada de tras (gas ate' 1900, eletricos depois)
 if(year>=1820)for(int x=10+(int)(seed%20);x<256;x+=52){
  if((c->flags&CF_COASTAL)&&x>176)break;
  for(int y=ROAD_Y0-13;y<ROAD_Y0-1;y++)Put(x,y,C3(5,5,6));
  Put(x-1,ROAD_Y0-2,C3(5,5,6));Put(x+1,ROAD_Y0-2,C3(5,5,6));
  Fill(x-1,ROAD_Y0-16,x+2,ROAD_Y0-13,year>=1900?C3(31,31,26):C3(31,27,14));
  Put(x-1,ROAD_Y0-17,C3(5,5,6));Put(x,ROAD_Y0-17,C3(5,5,6));Put(x+1,ROAD_Y0-17,C3(5,5,6));
 }
 bool rail=false;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==BK_FERROVIA)rail=true;
 if(rail){
  for(int x=0;x<256;x++){for(int y=RAIL_Y-1;y<=RAIL_Y+4;y++)Put(x,y,((x+y)&3)?C3(14,13,12):C3(17,16,14));}  // brita
  for(int x=0;x<256;x++){if((x&3)==0)for(int y=RAIL_Y;y<=RAIL_Y+3;y++)Put(x,y,C3(14,10,6));
   Put(x,RAIL_Y,C3(20,20,22));Put(x,RAIL_Y+3,C3(20,20,22));}
 }
 // arvores nos vaos entre os lotes de tras
 static const u8 GAP[5]={6,66,124,182,244};
 for(int i=0;i<5;i++){u32 h=Hh(seed,i+90);if((h&3)==0)continue;Tree(GAP[i],BACK_Y,4,tk,h);}
 // obras: fileira de tras primeiro, depois a da frente (profundidade); cada
 // obra projeta uma sombra no chao antes de ser pintada
 u8 nat=c->owner?w->c[c->owner].nat:255;
 for(int pass=0;pass<2;pass++)for(int s=pass?0:4;s<(pass?4:8);s++){
  if(s>=City_Slots(c->tier)&&!(c->bq&&c->bqSlot==s))continue;
  int x=SLOT_X[s],wid=SLOT_W[s],yb=SlotBase(s);
  D=d;
  if(c->bld[s]||(c->bq&&c->bqSlot==s))for(int k=2;k<wid-2;k++)for(int j=0;j<3;j++)Darken(x+k+j+2,yb-1+j,11+j,16);
  if(c->bq&&c->bqSlot==s)Scaffold(d,x,yb,wid,100-c->bqDays*100/(BLD[c->bq].days?BLD[c->bq].days:1));
  else if(c->bld[s])PxC_Building(d,x,yb,wid,c->bld[s],st,year,seed+s,-1,nat);
  else{ // lote vazio: cerquinha + terra batida
   D=d;
   for(int y=yb-3;y<yb;y++)for(int xx=x+5;xx<x+wid-5;xx++)if(Hh(xx,y)&1)Put(xx,y,C3(16,13,8));
   for(int xx=x+4;xx<x+wid-4;xx+=3)for(int y=yb-6;y<yb;y++)Put(xx,y,C3(18,13,8));
   for(int xx=x+4;xx<x+wid-4;xx++)Put(xx,yb-5,C3(20,15,9));
  }
 }
 CY0=0;CY1=192;YOFF=0;
}
// sprites vivos: pessoas, carroca/carro, fumaca, trem, barcos
static void SceneLive(u16 *d,const World *w,int ci,int tick){
 const WCity *c=&w->city[ci];u32 seed=Hh(c->x,c->y);u16 year=w->date.y;u8 st=City_Style(w,ci);
 CY0=20;CY1=192;D=d;
 // fumaca das chamines (por cima do fundo)
 for(int s=0;s<CITY_SLOTS;s++){
  u8 k=c->bld[s];if(k!=BK_OFICINA&&k!=BK_FABRICA)continue;
  OX=SLOT_X[s];OY=SlotBase(s);UW=SLOT_W[s];
  if(k==BK_OFICINA)Smoke(22,26,tick,seed+s);else{Smoke(6,34,tick,seed+s);Smoke(23,40,tick,seed+s+5);}
 }
 // barcos no porto
 if(c->flags&CF_COASTAL){
  int nb=1+c->tier;
  for(int i=0;i<nb;i++){
   int x=196+i*18,y=HORIZ+12+i*9+((tick/20+i)&1);
   u16 hull=C3(12,8,5),sail=C3(29,28,24);
   for(int k=0;k<12;k++){Put(x+k,y,hull);}
   for(int k=1;k<11;k++){Put(x+k,y+1,hull);}
   if(year<1880){for(int j=1;j<9;j++)for(int k=0;k<j/2+1;k++)Put(x+6-k,y-j,sail);for(int j=0;j<10;j++)Put(x+6,y-j,hull);}
   else{Fill(x+3,y-4,x+9,y,C3(20,20,22));Fill(x+5,y-8,x+7,y-4,C3(22,7,5));}
  }
 }
 // trem
 bool rail=false;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==BK_FERROVIA)rail=true;
 if(rail){
  int x=(tick*2)%420-110;
  for(int k=0;k<4;k++){
   int wx=x+k*22;u16 col=k==0?C3(6,6,8):C3(18,6,5);
   Fill(wx,RAIL_Y-8,wx+20,RAIL_Y,col);Fill(wx+2,RAIL_Y-6,wx+18,RAIL_Y-4,k?C3(26,24,16):C3(12,12,14));
   Put(wx+4,RAIL_Y,C3(4,4,4));Put(wx+15,RAIL_Y,C3(4,4,4));
   if(k==0){Fill(wx+14,RAIL_Y-12,wx+17,RAIL_Y-8,C3(6,6,8));OX=wx;OY=RAIL_Y-12;UW=32;Smoke(16,0,tick,seed);}
  }
 }
 // pessoas (e uma carroca ou carro) na estrada: a sociedade em movimento.
 // 9px de altura, sombra no chao, chapeu/vestido; desenhadas de tras p/ frente
 int np=3+c->pop/12;if(np>22)np=22;
 static const u8 SKIN[4][3]={{28,22,16},{24,17,11},{18,12,8},{12,8,5}};
 int sk0=st==ST_AFRICA?2:(st==ST_ASIA||st==ST_NATIVE||st==ST_ISLAM?1:0);
 s16 px[22],py[22];u8 ord[22];
 for(int i=0;i<np;i++){
  u32 h=Hh(seed,i+1000);
  int dir=(h&1)?1:-1,sp=1+(int)((h>>1)%2);
  int x=(int)((h>>3)%300)+dir*(tick*sp/3);x=((x%300)+300)%300-20;
  px[i]=(s16)x;py[i]=(s16)(ROAD_Y0+3+(int)((h>>12)%9));ord[i]=(u8)i;
 }
 for(int i=1;i<np;i++){u8 k=ord[i];int j=i;while(j>0&&py[ord[j-1]]>py[k]){ord[j]=ord[j-1];j--;}ord[j]=k;}
 for(int n=0;n<np;n++){
  int i=ord[n];u32 h=Hh(seed,i+1000);
  int x=px[i],y=py[i];
  const u8 *skc=SKIN[(sk0+(int)((h>>16)%2))&3];
  u16 skin=C3(skc[0],skc[1],skc[2]);
  int cr=6+(int)((h>>18)%20),cg=6+(int)((h>>22)%14),cb=6+(int)((h>>26)%18);
  u16 cloth=C3(cr,cg,cb),clothL=C3(cr+4,cg+4,cb+4),clothD=C3(cr*3/4,cg*3/4,cb*3/4);
  bool dress=(h>>5)&1,hat=!dress&&((h>>6)&1);
  int fr=((tick/6)+i)&1;
  for(int k=-1;k<=3;k++)Darken(x+k,y+1,10,16);          // sombra
  if(dress){
   for(int r=0;r<4;r++)for(int k=-r/2;k<3+r/2;k++)Put(x+k,y-1-3+r,k<=0?clothL:(k>=2+r/2?clothD:cloth));
   Put(x+(fr?0:2),y,C3(6,5,4));
   Fill(x,y-7,x+3,y-4,cloth);Put(x,y-6,clothL);
  }else{
   u16 leg=C3(6,6,8);
   Put(x+(fr?0:1),y-1,leg);Put(x+(fr?2:1),y-1,leg);Put(x+(fr?0:1),y,leg);Put(x+(fr?2:1),y,leg);
   Fill(x,y-6,x+3,y-2,cloth);for(int r=y-6;r<y-2;r++){Put(x,r,clothL);Put(x+2,r,clothD);}
   Put(x+(fr?-1:3),y-4,skin);                         // mao balancando
  }
  Put(x+1,y-8,skin);Put(x+2,y-8,skin);Put(x+1,y-7,skin);Put(x+2,y-7,Dk(skin,13,16));
  if(hat){Fill(x,y-9,x+4,y-8,C3(4,4,5));Put(x+1,y-10,C3(4,4,5));Put(x+2,y-10,C3(4,4,5));}
  else Put(x+1,y-9,(h>>7)&1?C3(6,4,3):C3(18,14,6));    // cabelo
 }
 if(c->tier>=CT_VILAREJO){
  int x=(tick+(int)(seed%200))%320-30,y=ROAD_Y0+8;
  if(year>=1910){Fill(x,y-5,x+12,y-1,C3(18,4,4));Fill(x+3,y-8,x+9,y-5,C3(14,16,18));Put(x+2,y,C3(3,3,3));Put(x+9,y,C3(3,3,3));}
  else{Fill(x,y-5,x+9,y-1,C3(18,12,6));Put(x+2,y,C3(6,4,2));Put(x+7,y,C3(6,4,2));
   Fill(x+10,y-6,x+16,y-2,C3(14,9,5));Put(x+16,y-7,C3(14,9,5));Put(x+11,y-1,C3(6,4,2));Put(x+15,y-1,C3(6,4,2));}
 }
 CY0=0;CY1=192;
}
void PxC_Scene(u16 *d,int ci){
 const World *w=WG_World();
 if(ci<0||ci>=w->nCity)return;
 u16 *L=PxM_Scratch();
 u32 sig=SceneSig(w,ci);
 if(sceneCity!=ci||sceneSig!=sig){SceneBg(L,w,ci);sceneCity=ci;sceneSig=sig;}
 for(int i=0;i<WG_MAP_H*256;i++)d[WG_MAP_Y*256+i]=L[i];
 SceneLive(d,w,ci,(int)WG_Ptr()->tick);
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

// ======================= painel da cidade (tela de baixo) =======================
static void Cat(char *dst,int max,const char *s){int n=0;while(dst[n])n++;while(*s&&n<max-1)dst[n++]=*s++;dst[n]=0;}
static void Num(char *dst,int max,s32 v){char t[16];Px_Itoa((int)v,t);Cat(dst,max,t);}
static void FmtPop(char *o,int max,u16 pop){
 o[0]=0;
 if(pop>=1000){Num(o,max,pop/1000);Cat(o,max,",");char t[4]={(char)('0'+(pop%1000)/100),0};Cat(o,max,t);Cat(o,max," M");}
 else{Num(o,max,pop);Cat(o,max,Str(STR_MIL));}
}
// por que a obra k nao pode ser feita (StrId) ou 0
static u16 WhyNot(const World *w,int ci,u8 k,u16 *year){
 const WCity *c=&w->city[ci];const BldDef *b=&BLD[k];
 for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==k)return STR_JA_TEM;
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
 return 0;
}
void PxC_Panel(u16 *d,int ci,int slot,bool picking,u8 pk){
 const World *w=WG_World();
 Ui_Backdrop(d);
 if(ci<0||ci>=w->nCity)return;
 const WCity *c=&w->city[ci];
 u8 st=City_Style(w,ci);u16 year=w->date.y;u32 seed=Hh(c->x,c->y);
 u8 nat=c->owner?w->c[c->owner].nat:255;
 Ui_Navy(d,4,4,248,58);
 char t[96];
 // populacao e crescimento
 t[0]=0;Cat(t,96,Str(STR_POPULACAO));Cat(t,96," ");char p[24];FmtPop(p,24,c->pop);Cat(t,96,p);
 Px_Text2(d,10,9,t,&F_SERIF8,C_TEXT);
 int gold=c->tier,cap=c->pop/8;
 for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]){gold+=BLD[c->bld[s]].gold;cap+=BLD[c->bld[s]].cap;}
 t[0]=0;Cat(t,96,Str(STR_COMIDA));Cat(t,96," ");Num(t,96,c->food);Cat(t,96,"% · +");Num(t,96,gold);Cat(t,96,Str(STR_OURO_DIA));
 Px_Text2(d,246-Px_Width2(t,&F_SERIF8),9,t,&F_SERIF8,c->food<60?C_EVENT:C_GOLD);
 (void)cap;
 int np=City_NextPop(c->tier);
 if(np){
  t[0]=0;Cat(t,96,Str(STR_RUMO));Cat(t,96,Str((u16)(STR_TIER_0+c->tier+1)));Cat(t,96,": ");Num(t,96,c->pop);Cat(t,96,"/");Num(t,96,np);Cat(t,96,Str(STR_MIL));
  Px_Text2(d,10,23,t,&F_SERIF8,C_TEXT_DIM);
  Ui_Bar(d,10,36,236,c->pop*100/np,C_GOLD);
 }else{Px_Text2(d,10,23,Str(STR_TIER_3),&F_SERIF8,C_GOLD);Ui_Bar(d,10,36,236,100,C_GOLD);}
 // obra em andamento
 if(c->bq){t[0]=0;Cat(t,96,Str(STR_EM_OBRA));Cat(t,96,": ");Cat(t,96,Str(BLD[c->bq].nameStr));Cat(t,96," · ");Num(t,96,c->bqDays);Cat(t,96,Str(STR_DIAS));
  Px_Text2(d,10,45,t,&F_SERIF8,C_TEXT);}
 else{
  // o que a cidade produz por semana (ate 3 bens)
  s32 pr[GD_COUNT],nd[GD_COUNT],dm[GD_COUNT];WEco_CityFlows(w,ci,pr,nd,dm);
  t[0]=0;Cat(t,96,Str(STR_PRODUZ));Cat(t,96,":");int sh=0;
  for(int r=0;r<3;r++){
   int bg=-1;s32 bv=0;
   for(int g=0;g<GD_COUNT;g++)if(pr[g]>bv){bv=pr[g];bg=g;}
   if(bg<0)break;
   Cat(t,96," ");Cat(t,96,Str(Gd_Str((u8)bg)));Cat(t,96," +");Num(t,96,bv);pr[bg]=0;sh++;
  }
  if(!sh)Cat(t,96," —");
  Cat(t,96,Str(STR_POR_SEM));
  Px_Text2(d,10,45,t,&F_SERIF8,C_TEXT_DIM);
 }
 // slots 4x2
 for(int s=0;s<CITY_SLOTS;s++){
  int x=4+(s%4)*62,y=66+(s/4)*40;
  bool open=s<City_Slots(c->tier),sel=s==slot;
  Px_Rect(d,x,y,60,38,open?C_NAVY:C_NAVY_BG);
  Px_Border(d,x,y,60,38,sel?C_CRIMSON:(open?C_GOLD_DIM:C_NAVY));
  if(sel)Px_Border(d,x+1,y+1,58,36,C_CRIMSON_HI);
  if(!open){Ui_TextC(d,x+30,y+14,"—",0,C_TEXT_DIM);continue;}
  u8 k=c->bld[s];
  if(c->bq&&c->bqSlot==s){Scaffold(d,x+17,y+26,26,100-c->bqDays*100/(BLD[c->bq].days?BLD[c->bq].days:1));k=c->bq;}
  else if(k)PxC_Building(d,x+17,y+26,26,k,st,year,seed+s,-1,nat);
  const char *nm=k?Str(BLD[k].nameStr):Str(STR_VAZIO);
  Ui_TextC(d,x+30,y+27,nm,0,k?C_TEXT:C_TEXT_DIM);
 }
 // faixa de contexto: obra escolhida ou explicacao
 Ui_Navy(d,4,148,248,40);
 if(picking){
  const BldDef *b=&BLD[pk];
  Ui_Arrows(d,10,240,154,C_GOLD);
  t[0]=0;Cat(t,96,Str(b->nameStr));Cat(t,96," · ");Num(t,96,City_Cost(w,ci,pk));Cat(t,96," ");Cat(t,96,Str(STR_OURO));
  Cat(t,96," · ");Num(t,96,b->days);Cat(t,96,Str(STR_DIAS));
  Ui_TextC(d,128,153,t,0,C_TEXT);
  u16 yr=0;u16 why=WhyNot(w,ci,pk,&yr);
  if(why){t[0]=0;Cat(t,96,Str(why));if(why==STR_REQ_ANO)Num(t,96,yr);
   else if(why==STR_REQ_NIVEL)Cat(t,96,Str((u16)(STR_TIER_0+b->minTier)));
   else if(why==STR_REQ_OBRA)Cat(t,96,Str(BLD[b->need].nameStr));
   Ui_TextC(d,128,166,t,0,C_EVENT);}
  else Ui_TextC(d,128,166,Str(b->descStr),0,C_TEXT_DIM);
  Ui_TextC(d,128,177,Str(STR_A_CONFIRMA),0,C_GOLD_DIM);
 }else{
  bool open=slot<City_Slots(c->tier);u8 k=c->bld[slot];
  if(!open)Ui_TextC(d,128,160,Str(STR_SLOT_FECHADO),0,C_TEXT_DIM);
  else if(k==BK_ESTALEIRO){
   const WCountry *C=&w->c[c->owner];
   t[0]=0;Cat(t,96,Str(STR_FROTA));Cat(t,96,": ");Num(t,96,C->ships);Cat(t,96," ");Cat(t,96,Str(STR_NAVIOS));
   if(C->shipQ){Cat(t,96," · ");Cat(t,96,Str(STR_NAVIO_FILA));Num(t,96,C->shipQ);Cat(t,96,Str(STR_DIAS));}
   Ui_TextC(d,128,155,t,0,C_TEXT);
   t[0]=0;Cat(t,96,Str(STR_A_NAVIO));Cat(t,96," · ");Num(t,96,WEco_ShipCost(w,c->owner));Cat(t,96," ");Cat(t,96,Str(STR_OURO));
   Ui_TextC(d,128,170,t,0,C->shipQ?C_TEXT_DIM:C_GOLD);
  }
  else if(k){Ui_TextC(d,128,155,Str(BLD[k].nameStr),1,C_TEXT);Ui_TextC(d,128,172,Str(BLD[k].descStr),0,C_TEXT_DIM);}
  else Ui_TextC(d,128,160,Str(STR_A_CONSTRUIR),0,C_TEXT);
 }
}
