#include "wgame.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "flag.h"
#include "earth.h"
#include "calendar.h"
#include "txt.h"
#include "city.h"
#include "wecon.h"
#include "wdiplo.h"
#include "history.h"
#include "settings.h"
// px_world.c — render da partida no mundo celular (DESIGN.md sec 4.2/4.3).
// Tela do MAPA: barra de recursos 20px + mapa politico 256x172 em 5 zooms:
//  Z0 mundo inteiro (2 celulas/px) .. Z4 detalhe (8 px/celula).
//  Mar = pergaminho azulado com hachura de gravura; plataforma clara na costa.
//  Pais = cor chapada; borda escura nos dois lados (2px, o "brilho" do
//  Victoria 3); relevo/floresta em micro-padroes a partir de Z2; povos
//  autonomos com hachura diagonal leve; nomes espacados em caixa alta.
//  Frente de guerra pisca em fogo; seu reino tem contorno dourado; o pais sob
//  o cursor pulsa em branco.
// Tela do PAINEL: ficha do pais sob o cursor, despacho, 4 acoes + % + velocidade.
// Cor: so' PXC/tokens (ADR-13). Sem float. Render sob demanda (main.c).

static inline int Cl(int v){return v<0?0:(v>31?31:v);}
static inline u16 C3(int r,int g,int b){return PXC(Cl(r),Cl(g),Cl(b));}

// ---- estatisticas p/ rotulos e minimapa (1x por dia) ----
typedef struct { u32 sx,sy,n; u16 x0,x1,y0,y1; } CStat;
static CStat ST[W_MAXC+1];
enum { MINI_W=64, MINI_H=24 };
static u16 MINI[MINI_W*MINI_H];
void PxW_Stats(void){
 const World *w=WG_World();
 for(int i=0;i<=W_MAXC;i++){ST[i].sx=ST[i].sy=ST[i].n=0;ST[i].x0=ST[i].y0=0xFFFF;ST[i].x1=ST[i].y1=0;}
 for(int y=0;y<w->h;y++){
  const u8 *row=&W_OWNER[World_Idx(w,0,y)];
  for(int x=0;x<w->w;x++){
   u8 o=row[x];if(!o)continue;
   CStat *s=&ST[o];
   s->sx+=(u32)x;s->sy+=(u32)y;s->n++;
   if(x<s->x0)s->x0=(u16)x;
   if(x>s->x1)s->x1=(u16)x;
   if(y<s->y0)s->y0=(u16)y;
   if(y>s->y1)s->y1=(u16)y;
  }
 }
 // minimapa: sempre politico (DESIGN 4.2), 1 amostra por bloco
 int sx=(w->w<<8)/MINI_W, sy=(w->h<<8)/MINI_H;
 for(int j=0;j<MINI_H;j++)for(int i=0;i<MINI_W;i++){
  int x=(i*sx+sx/2)>>8,y=(j*sy+sy/2)>>8;
  u32 k=World_Idx(w,x,y);u8 o=W_OWNER[k];
  u16 c;
  if(W_TERR[k]==WT_WATER)c=PXC(9,13,17);
  else if(o){const NationDef *d=&NATIONS[w->c[o].nat];c=C3(d->r,d->g,d->b);}
  else c=PXC(24,22,16);
  MINI[j*MINI_W+i]=c;
 }
}

// ---- utilitarios de texto ----
static void Cat(char *dst,int max,const char *s){
 int n=0;while(dst[n])n++;
 while(*s&&n<max-1)dst[n++]=*s++;
 dst[n]=0;
}
// 1234 / 12,3k / 123k / 1,2M — numero grande legivel (frontwars)
static void FmtK(s32 v,char *o){
 char t[16];bool neg=v<0;if(neg)v=-v;
 if(v<10000){Px_Itoa((int)v,t);}
 else if(v<100000){int k=(int)(v/1000),d=(int)((v%1000)/100);Px_Itoa(k,t);int n=0;while(t[n])n++;t[n++]=',';t[n++]=(char)('0'+d);t[n++]='k';t[n]=0;}
 else if(v<1000000){Px_Itoa((int)(v/1000),t);Cat(t,16,"k");}
 else{int m=(int)(v/1000000),d=(int)((v%1000000)/100000);Px_Itoa(m,t);int n=0;while(t[n])n++;t[n++]=',';t[n++]=(char)('0'+d);t[n++]='M';t[n]=0;}
 o[0]=0;if(neg)Cat(o,16,"-");Cat(o,16,t);
}
static const char *NatName(const World *w,u8 ci){return ci?NATIONS[w->c[ci].nat].name:"";}

// ---- mapa ----
static int RelX(const World *w,int a,int b){
 int d=b-a;
 if(w->wrap){d%=w->w;if(d<-w->w/2)d+=w->w;else if(d>=w->w/2)d-=w->w;}
 return d;
}
// celula -> pixel do viewport (centro da celula); false se fora da vista
static bool ToScreen(const World *w,const WGame *s,int cx,int cy,int *px,int *py){
 int rx=RelX(w,s->camX,cx),ry=cy-s->camY;
 if(w->wrap&&rx<0)rx+=w->w;   // a vista sempre comeca em camX
 if(s->zoom==0){*px=rx/2;*py=ry/2;}
 else{int ppc=1<<(s->zoom-1);*px=rx*ppc+ppc/2;*py=ry*ppc+ppc/2;}
 return *px>=0&&*px<256&&*py>=0&&*py<WG_MAP_H;
}
// estrela de capital 5x5 com contorno de tinta
static void Star(u16 *d,int x,int y,u16 col){
 static const char *const S[5]={"..g..",".ggg.","ggggg",".ggg.","g...g"};
 for(int r=0;r<5;r++)for(int c=0;c<5;c++)if(S[r][c]=='g'){
  for(int k=0;k<4;k++){int dx=(k==0)-(k==1),dy=(k==2)-(k==3);
   int px=x+c-2+dx,py=y+r-2+dy;
   if(py>=WG_MAP_Y&&py<192&&!(px>=x-2&&px<=x+2&&py>=y-2&&py<=y+2&&S[py-y+2][px-x+2]=='g'))Px_PSet(d,px,py,C_INK);}
 }
 for(int r=0;r<5;r++)for(int c=0;c<5;c++)if(S[r][c]=='g'&&y+r-2>=WG_MAP_Y)Px_PSet(d,x+c-2,y+r-2,col);
}
// rotulos: maiores paises primeiro; sem sobreposicao; so' se couber
typedef struct { s16 x0,y0,x1,y1; } Rect;
static void Labels(u16 *d,const World *w,const WGame *s){
 static u8 ord[W_MAXC];int n=0;
 for(u8 ci=1;ci<w->nC;ci++)if(w->c[ci].alive&&ST[ci].n)ord[n++]=ci;
 // ordena por tamanho (insercao; n<=250 e so' 1x por render)
 for(int i=1;i<n;i++){u8 v=ord[i];int j=i-1;while(j>=0&&ST[ord[j]].n<ST[v].n){ord[j+1]=ord[j];j--;}ord[j+1]=v;}
 static Rect used[48];static u8 labd[48];int nu=0;
 // areas reservadas: chip de velocidade, minimapa e faixa de evento
 used[nu]=(Rect){220,WG_MAP_Y,256,WG_MAP_Y+19};labd[nu++]=0;
 if(s->zoom)used[nu]=(Rect){182,159,256,192},labd[nu++]=0;
 if(s->bannerT)used[nu]=(Rect){6,WG_MAP_Y+2,250,WG_MAP_Y+52},labd[nu++]=0;
 int ppc2=s->zoom==0?1:(2<<(s->zoom-1));   // px por celula x2 (Z0 = 0,5 px)
 bool ter=(s->mapMode!=WMAP_POL);
 for(int k=0;k<n&&k<80&&nu<48;k++){
  u8 ci=ord[k];const CStat *st=&ST[ci];
  int cx=(int)(st->sx/st->n),cy=(int)(st->sy/st->n);
  int px,py;
  if(!ToScreen(w,s,cx,cy,&px,&py))continue;
  int spanW=(st->x1-st->x0+1);if(spanW>w->w/2)spanW=(int)(st->n/(st->y1-st->y0+1))*2; // pais que da' a volta
  int availW=spanW*ppc2/2*3/4, availH=(st->y1-st->y0+1)*ppc2/2;
  if(availW>220)availW=220;
  const char *nm=NATIONS[w->c[ci].nat].name;
  const Font *f=&F_SERIF12;int maxsp=6;
  int w0=Px_Width2Ex(nm,f,0,1,TX_UPPER);
  if(w0>availW||availH<18){f=&F_SERIF8;maxsp=3;w0=Px_Width2Ex(nm,f,0,1,TX_UPPER);}
  if(w0>availW||availH<11)continue;
  int letters=Txt_Len(nm);
  int sp=letters>1?(availW-w0)/(letters-1):0;
  if(sp>maxsp)sp=maxsp;
  if(sp<0)sp=0;
  int tw=Px_Width2Ex(nm,f,sp,1,TX_UPPER);
  bool troops=(s->zoom>=1&&availH>=f->caph+16);
  if(tw>250)continue;
  int lx=px-tw/2;          // rotulo inteiro dentro da tela
  if(lx<2)lx=2;
  if(lx+tw>254)lx=254-tw;
  px=lx+tw/2;
  Rect r={(s16)(lx-2),(s16)(WG_MAP_Y+py-f->caph/2-2),(s16)(lx+tw+2),(s16)(WG_MAP_Y+py+f->caph/2+(troops?13:2))};
  bool clash=false;
  for(int i=0;i<nu&&!clash;i++)if(r.x0<used[i].x1&&r.x1>used[i].x0&&r.y0<used[i].y1&&r.y1>used[i].y0)clash=true;
  if(clash||r.y0<WG_MAP_Y+1)continue;
  used[nu]=r;labd[nu++]=ci;
  const NationDef *nd=&NATIONS[w->c[ci].nat];
  u16 fill=ter?C_INK:C3(nd->r*3/8,nd->g*3/8,nd->b*3/8);
  u16 halo=ter?C_PARCH:C3(nd->r+(31-nd->r)*2/3,nd->g+(31-nd->g)*2/3,nd->b+(31-nd->b)*2/3);
  Px_Text2Out(d,lx,WG_MAP_Y+py-f->caph/2,nm,f,fill,halo,sp,1,TX_UPPER);
  if(troops){
   char t[16];FmtK(w->c[ci].troops,t);
   int tw2=Px_Width2(t,&F_SERIF8);
   Px_Text2Out(d,px-tw2/2,WG_MAP_Y+py+f->caph/2+3,t,&F_SERIF8,C_TEXT,C_INK,0,1,0);
  }
 }
 // nomes de cidades (Z3: cidade e metropole; Z4: todas), sem sobrepor nada
 if(s->zoom>=3){
  for(int pass=CT_METROPOLE;pass>=(s->zoom==4?CT_POVOADO:CT_CIDADE)&&nu<48;pass--)
   for(int i=0;i<w->nCity&&nu<48;i++){
    const WCity *c=&w->city[i];if(c->tier!=pass)continue;
    int px,py;if(!ToScreen(w,s,c->x,c->y,&px,&py))continue;
    char nm[40];City_Name(w,i,nm,40);
    int tw=Px_Width2(nm,&F_SERIF8);
    int ty=WG_MAP_Y+py+(s->zoom==4?(c->tier>=CT_CIDADE?8:6):5);
    Rect r={(s16)(px-tw/2-1),(s16)(ty-1),(s16)(px+tw/2+1),(s16)(ty+9)};
    bool clash=r.y1>190||r.x0<0||r.x1>256;
    for(int k=0;k<nu&&!clash;k++)if(r.x0<used[k].x1&&r.x1>used[k].x0&&r.y0<used[k].y1&&r.y1>used[k].y0)clash=true;
    if(clash)continue;
    used[nu]=r;labd[nu++]=0;
    Px_Text2Out(d,px-tw/2,ty,nm,&F_SERIF8,c->tier>=CT_CIDADE?C_TEXT:C_PARCH,C_INK,0,1,0);
   }
 }
 // capitais: so' dos paises rotulados e sempre a sua (menos ruido)
 bool meDone=false;
 for(int k=0;k<=nu;k++){
  u8 ci=k<nu?labd[k]:s->me;
  if(!ci)continue;
  if(k==nu&&meDone)break;
  if(ci==s->me)meDone=true;
  int px,py;
  if(s->zoom<2&&w->c[ci].alive&&ToScreen(w,s,w->c[ci].capX,w->c[ci].capY,&px,&py))  // de Z2 em diante a arte da cidade assume
   Star(d,px,WG_MAP_Y+py,ci==s->me?C_GOLD_HI:C_GOLD);
 }
}
// celula -> pixel da tela (pode cair fora: linhas que cruzam a borda)
static void CellPx(const World *w,const WGame *s,int cx,int cy,int *px,int *py){
 int rx=RelX(w,s->camX,cx),ry=cy-s->camY;
 int vw=WG_ViewW();
 if(w->wrap&&rx<-vw/2)rx+=w->w;            // a copia mais perto da vista
 if(s->zoom==0){*px=rx/2;*py=ry/2;}
 else{int ppc=1<<(s->zoom-1);*px=rx*ppc+ppc/2;*py=ry*ppc+ppc/2;}
 *py+=WG_MAP_Y;
}
static inline void MPut(u16 *d,int x,int y,u16 c){if(x>=0&&x<256&&y>WG_MAP_Y&&y<192)d[y*256+x]=c;}
// rota maritima: arco (Bezier quadratica inteira) tracejado + navio que anda
static void Arc(u16 *d,int x0,int y0,int x2,int y2,int side,u16 col,int phase,bool ship){
 int mx=(x0+x2)/2,my=(y0+y2)/2,dx=x2-x0,dy=y2-y0;
 int x1=mx-dy*side/4,y1=my+dx*side/4;      // ponto de controle: perpendicular
 enum{N=40};
 int lx=x0,ly=y0;
 for(int i=1;i<=N;i++){
  int a=N-i,b=i;
  int x=(a*a*x0+2*a*b*x1+b*b*x2)/(N*N),y=(a*a*y0+2*a*b*y1+b*b*y2)/(N*N);
  if(((i+phase/4)&3)<2){  // tracejado que corre
   int sx=lx,sy=ly,ex=x,ey=y,ddx=ex-sx,ddy=ey-sy,st=(ddx<0?-ddx:ddx)>(ddy<0?-ddy:ddy)?(ddx<0?-ddx:ddx):(ddy<0?-ddy:ddy);
   if(st>40)st=40;
   for(int k=0;k<=st;k++)MPut(d,sx+(st?ddx*k/st:0),sy+(st?ddy*k/st:0),col);
  }
  lx=x;ly=y;
 }
 if(!ship)return;
 int i=phase%N,a=N-i,b=i;
 int x=(a*a*x0+2*a*b*x1+b*b*x2)/(N*N),y=(a*a*y0+2*a*b*y1+b*b*y2)/(N*N);
 // navio mercante 5x4: casco escuro, vela clara
 MPut(d,x-2,y+1,C_INK);MPut(d,x-1,y+1,C_INK);MPut(d,x,y+1,C_INK);MPut(d,x+1,y+1,C_INK);MPut(d,x+2,y+1,C_INK);
 MPut(d,x-1,y+2,C_INK);MPut(d,x,y+2,C_INK);MPut(d,x+1,y+2,C_INK);
 MPut(d,x,y-2,C_TEXT);MPut(d,x,y-1,C_TEXT);MPut(d,x+1,y-1,C_TEXT);MPut(d,x,y,C_INK);
}
static void Routes(u16 *d,const World *w,const WGame *s){
 bool all=s->mapMode==WMAP_COM;
 // so' as maiores rotas (o mapa nao vira teia): 12 afastado, 24 de perto
 enum{RMAX=24};
 u8 rc[RMAX],rk[RMAX];u16 rv[RMAX];int n=0,lim=s->zoom>=2?24:12;
 for(u8 ci=1;ci<w->nC;ci++){
  const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
  if(!c->alive||e->hub<0||(!all&&ci!=s->me))continue;
  for(int k=0;k<3;k++){
   u8 p=e->route[k];if(!p||w->eco[p].hub<0)break;
   u16 v=ci==s->me?60000:e->routeV[k];          // as suas sempre aparecem
   int j;
   if(n<lim)j=n++;
   else{if(rv[lim-1]>=v)continue;j=lim-1;}
   while(j>0&&rv[j-1]<v){rc[j]=rc[j-1];rk[j]=rk[j-1];rv[j]=rv[j-1];j--;}
   rc[j]=ci;rk[j]=(u8)k;rv[j]=v;
  }
 }
 for(int i=0;i<n;i++){
  u8 ci=rc[i];int k=rk[i];const WEco *e=&w->eco[ci];
  const WCity *h=&w->city[e->hub];
  int ax,ay;CellPx(w,s,h->x,h->y,&ax,&ay);
  const WCity *ph=&w->city[w->eco[e->route[k]].hub];
  int bx,by;CellPx(w,s,ph->x,ph->y,&bx,&by);
  if((ax<-40&&bx<-40)||(ax>296&&bx>296)||(ay<0&&by<0)||(ay>232&&by>232))continue;
  int ddx=bx-ax;if(ddx>400||ddx<-400)continue;
  u16 col=ci==s->me?C_GOLD_HI:PxW_GoodCol(e->routeG[k]);
  Arc(d,ax,ay,bx,by,((ci+k)&1)?1:-1,col,(int)(s->tick/2)+ci*7+k*11,s->zoom>=1);
 }
}
// frotas: navio de guerra ao lado do porto principal (Z2+), com o numero
static void Fleets(u16 *d,const World *w,const WGame *s){
 if(s->zoom<2)return;
 bool all=s->mapMode==WMAP_COM;
 for(u8 ci=1;ci<w->nC;ci++){
  const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
  if(!c->alive||c->ships<=0||e->hub<0)continue;
  if(!all&&ci!=s->me&&!World_AtWar(w,s->me,ci))continue;
  const WCity *h=&w->city[e->hub];
  int x,y;CellPx(w,s,h->x,h->y,&x,&y);
  x+=6;y+=6;
  if(x<0||x>244||y<WG_MAP_Y+4||y>184)continue;
  bool foe=World_AtWar(w,s->me,ci);
  u16 hull=foe?C_EVENT:(ci==s->me?C_GOLD:C_NAVY);
  for(int i=-4;i<=4;i++)MPut(d,x+i,y,hull);
  for(int i=-3;i<=3;i++)MPut(d,x+i,y+1,hull);
  for(int j=1;j<5;j++)MPut(d,x,y-j,C_INK);
  MPut(d,x-1,y-3,C_TEXT);MPut(d,x+1,y-3,C_TEXT);MPut(d,x-1,y-2,C_TEXT);MPut(d,x+1,y-2,C_TEXT);MPut(d,x+2,y-2,C_TEXT);
  char t[8];Px_Itoa(c->ships,t);
  Px_Text2Out(d,x+6,y-6,t,&F_SERIF8,C_TEXT,C_INK,0,1,0);
 }
}
// MIRA: ponto no centro da vista (as setas movem o mapa por baixo dela).
// Anel de 2px com 4 aberturas (o alvo fica visivel) e contorno de tinta +
// ponto central; sobre algo acionavel o anel fica ouro e respira (A abre a
// folha de acoes).
static void Cursor(u16 *d,const World *w,const WGame *s){
 int px,py;
 if(!ToScreen(w,s,s->curX,s->curY,&px,&py))return;
 py+=WG_MAP_Y;
 bool tgt=!s->dead&&(s->hcity>=0||s->selLand||s->sel);
 int R=tgt&&((s->tick/20)&1)?9:8;
 u16 ring=tgt?C_GOLD_HI:C_TEXT;
 // 2 passadas: contorno de tinta (anel R-1..R+1 + 1) e o anel em si
 for(int pass=0;pass<2;pass++)for(int dy=-R-2;dy<=R+2;dy++)for(int dx=-R-2;dx<=R+2;dx++){
  int q=dx*dx+dy*dy,ad=dx<0?-dx:dx,bd=dy<0?-dy:dy;
  if(ad<=2||bd<=2)continue;                       // aberturas em cruz
  int lo=pass?(R-1)*(R-1):(R-2)*(R-2),hi=pass?R*R+R:(R+1)*(R+1)+R+1;
  if(q<lo||q>hi)continue;
  if(py+dy<=WG_MAP_Y)continue;
  Px_PSet(d,px+dx,py+dy,pass?ring:C_INK);
 }
 // tracos da cruz (fora do anel) apontando p/ o centro
 for(int k=0;k<4;k++){
  int ux=(k==0)-(k==1),uy=(k==2)-(k==3);
  for(int t=R-2;t<=R+3;t++){int x=px+ux*t,y=py+uy*t;
   if(y<=WG_MAP_Y)continue;
   Px_PSet(d,x+uy,y+ux,C_INK);Px_PSet(d,x-uy,y-ux,C_INK);Px_PSet(d,x,y,ring);}
 }
 for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i++)if(py+j>WG_MAP_Y&&(i*i+j*j)<=5)Px_PSet(d,px+i,py+j,C_INK);
 for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++)if(py+j>WG_MAP_Y&&(i==0||j==0))Px_PSet(d,px+i,py+j,ring);
}
static void Minimap(u16 *d,const World *w,const WGame *s){
 if(s->zoom==0)return;
 int x0=186,y0=163;
 Px_Rect(d,x0-2,y0-2,MINI_W+4,MINI_H+4,C_NAVY);
 Px_Border(d,x0-2,y0-2,MINI_W+4,MINI_H+4,C_GOLD);
 for(int j=0;j<MINI_H;j++)for(int i=0;i<MINI_W;i++)d[(y0+j)*256+x0+i]=MINI[j*MINI_W+i];
 // retangulo da vista
 int vx=s->camX*MINI_W/w->w, vy=s->camY*MINI_H/w->h;
 int vw=WG_ViewW()*MINI_W/w->w, vh=WG_ViewH()*MINI_H/w->h;
 if(vw<2)vw=2;
 if(vh<2)vh=2;
 for(int i=0;i<=vw;i++){
  int x=x0+(vx+i)%MINI_W;
  if(vy>=0&&vy<MINI_H)d[(y0+vy)*256+x]=C_WHITE;
  if(vy+vh>=0&&vy+vh<MINI_H)d[(y0+vy+vh)*256+x]=C_WHITE;
 }
 for(int j=0;j<=vh;j++){
  int y=vy+j;if(y<0||y>=MINI_H)continue;
  d[(y0+y)*256+x0+vx%MINI_W]=C_WHITE;
  d[(y0+y)*256+x0+(vx+vw)%MINI_W]=C_WHITE;
 }
}
// barra de recursos (SDD 14.4): ouro, tropas, prestigio, data
static void Bar(u16 *d,const World *w,const WGame *s){
 Px_Rect(d,0,0,256,19,C_NAVY);
 Px_Rect(d,0,19,256,1,C_GOLD);
 const WCountry *me=&w->c[s->me];
 static const s16 SX[4]={0,78,130,176};
 for(int i=1;i<4;i++)Px_Rect(d,SX[i],3,1,13,C_GOLD_DIM);
 char t[24];
 // ouro + saldo da semana (verde/vermelho): a economia sempre a vista
 Ui_IconId(d,3,4,ICO_COIN_S,false);FmtK(me->gold,t);Px_Text2(d,18,6,t,&F_SERIF8,C_TEXT);
 {s32 wk=WEco_GoldWeek(w,s->me);char q[16];FmtK(wk,q);char q2[18]={0};if(wk>=0)Cat(q2,18,"+");Cat(q2,18,q);
  Px_Text2(d,75-Px_Width2(q2,&F_SERIF8),6,q2,&F_SERIF8,wk<0?C_EVENT:C_PLAYER);}
 Ui_IconId(d,82,4,ICO_SWORDS_S,false);FmtK(me->troops+(me->atkOn?me->atkTroops:0),t);Px_Text2(d,97,6,t,&F_SERIF8,C_TEXT);
 Ui_IconId(d,134,4,ICO_CROWN_S,false);FmtK(me->prestige,t);Px_Text2(d,149,6,t,&F_SERIF8,C_TEXT);
 Ui_IconId(d,180,4,ICO_CAL_S,false);Cal_Format(&w->date,t,24);Px_Text2(d,195,6,t,&F_SERIF8,C_TEXT);
}
// chip de velocidade/pausa no canto do mapa
static void SpeedChip(u16 *d,const WGame *s){
 Px_Rect(d,224,23,29,14,C_NAVY);
 Px_Border(d,224,23,29,14,C_GOLD_DIM);
 if(s->paused){Px_Rect(d,234,26,3,8,C_GOLD);Px_Rect(d,240,26,3,8,C_GOLD);return;}
 for(int i=0;i<s->speed;i++){int x=228+i*5;for(int k=0;k<3;k++)for(int j=k;j<7-k;j++)Px_PSet(d,x+k,26+j,C_TEXT);}
}
// faixa de evento historico (pergaminho sobre o mapa por ~3 s)
static void Banner(u16 *d,const World *w){
 char title[64];int i=0;const char *m=w->msg;
 while(m[i]&&!(m[i]==':'&&m[i+1]==' ')&&i<63){title[i]=m[i];i++;}
 title[i]=0;
 const char *body=m[i]?m+i+2:"";
 Ui_Parch(d,8,24,240,46);
 Px_Text2(d,16,31,title,&F_SERIF12,C_INK);
 Px_WrapN(d,16,46,224,body,&F_SERIF8,C_INK,2,false);
}
void PxW_Map(u16 *d){
 World *w=WG_World();WGame *s=WG_Ptr();
 if(s->stale){PxW_Stats();s->stale=false;}
 int phase=(int)((s->tick/15)&1);
 PxM_Update(WG_World(),s);   // cache: so' repinta o que mudou (px_mapr.c)
 PxM_Blit(d);
 PxM_Overlay(d,w,s,phase);
 PxC_MapCities(d,w,s);
 Routes(d,w,s);
 Fleets(d,w,s);
 Labels(d,w,s);
 Cursor(d,w,s);
 Minimap(d,w,s);
 SpeedChip(d,s);
 if(s->bannerT&&w->msg[0])Banner(d,w);
 Bar(d,w,s);
}

// ---- painel ----
static void StatLine(u16 *d,int x,int y,u8 ico,u8 lab,const char *val){
 Ui_IconId(d,x,y-1,ico,false);
 Px_Text2(d,x+16,y,Str(lab),&F_SERIF8,C_TEXT_DIM);
 Px_Text2(d,x+18+Px_Width2(Str(lab),&F_SERIF8),y,val,&F_SERIF8,C_TEXT);
}
static void Card(u16 *d,const World *w,const WGame *s,u8 ci){
 const WCountry *c=&w->c[ci];const NationDef *nd=&NATIONS[c->nat];
 Px_Rect(d,10,10,40,28,C_GOLD);
 {FlagDef f;WDip_Flag(w,ci,&f);Flag_Draw(d,256,12,12,36,24,&f);}
 const Font *f=Px_Width2(nd->name,&F_SERIF12)<=188?&F_SERIF12:&F_SERIF8;
 Px_Text2(d,56,f==&F_SERIF12?11:13,nd->name,f,C_TEXT);
 char sub[96]={0};
 if(c->overlord&&w->c[c->overlord].alive){Cat(sub,96,Str(STR_SUDITO_DE));Cat(sub,96,NatName(w,c->overlord));}
 else{Cat(sub,96,Nat_GovName(c->gov));Cat(sub,96," · ");Cat(sub,96,Str((u16)(STR_ID_0+c->ideo)));}
 if(ci==s->me){Px_Text2(d,56,27,Str(STR_SUA_NACAO),&F_SERIF8,C_GOLD);
  Px_Text2(d,60+Px_Width2(Str(STR_SUA_NACAO),&F_SERIF8),27,sub,&F_SERIF8,C_TEXT_DIM);}
 else Px_Text2(d,56,27,sub,&F_SERIF8,C_TEXT_DIM);
 Px_Rect(d,10,41,236,1,C_GOLD_DIM);
 char a[16],b[16],t[40];
 FmtK(c->troops,a);FmtK(c->cap,b);t[0]=0;Cat(t,40,a);Cat(t,40," / ");Cat(t,40,b);
 StatLine(d,10,47,ICO_SWORDS_S,STR_TROPAS,t);
 Ui_Bar(d,26,59,96,c->cap?(int)(c->troops*100/c->cap):0,C_GOLD);
 FmtK((s32)c->cells,a);StatLine(d,132,47,ICO_LAND_S,STR_TERRITORIO,a);
 FmtK(c->gold,a);StatLine(d,10,66,ICO_COIN_S,STR_OURO,a);
 FmtK(c->prestige,a);StatLine(d,132,66,ICO_CROWN_S,STR_PRESTIGIO,a);
 // relacao / frente
 char rel[96]={0};u16 rc=C_TEXT_DIM;
 if(ci==s->me){
  if(c->atkOn){
   if(c->atkTo){Cat(rel,96,Str(STR_FRENTE_CONTRA));Cat(rel,96,NatName(w,c->atkTo));}
   else Cat(rel,96,Str(STR_COLONIZANDO));
   FmtK(c->atkTroops,a);Cat(rel,96," (");Cat(rel,96,a);Cat(rel,96,")");rc=C_GOLD;
  }else{
   Cat(rel,96,Str(STR_GUERRAS));int k=0;
   for(u8 j=1;j<w->nC&&k<3;j++)if(World_AtWar(w,ci,j)){if(k)Cat(rel,96,", ");Cat(rel,96,NatName(w,j));k++;}
   if(!k)Cat(rel,96,Str(STR_NENHUMA));
  }
 }else{
  if(World_AtWar(w,s->me,ci)){Cat(rel,96,Str(STR_EM_GUERRA));rc=C_EVENT;}
  else Cat(rel,96,Str(STR_EM_PAZ));
  if(WDip_Allied(w,s->me,ci)){Cat(rel,96," · ");Cat(rel,96,Str(STR_ALIADO));rc=C_PLAYER;}
  if(c->rival==s->me){Cat(rel,96," · ");Cat(rel,96,Str(STR_RIVAL));}
  if(c->bloc&&c->bloc==w->c[s->me].bloc){Cat(rel,96," · ");Cat(rel,96,Str(STR_SEU_BLOCO));}
  if(c->ships>0){char q[8];Px_Itoa(c->ships,q);Cat(rel,96," · ");Cat(rel,96,Str(STR_FROTA));Cat(rel,96," ");Cat(rel,96,q);}
  {char q[8];Px_Itoa(c->opin,q);Cat(rel,96," · ");Cat(rel,96,Str(STR_OPINIAO));Cat(rel,96," ");Cat(rel,96,q);}
 }
 Px_WrapN(d,10,80,236,rel,&F_SERIF8,rc,1,false);
 // traits
 char tr[128]={0};int k=0;
 for(int i=0;i<NAT_TRAITS;i++)if(c->traits[i]){if(k++)Cat(tr,128," · ");Cat(tr,128,Tr_Name(c->traits[i]));}
 Px_WrapN(d,10,91,236,tr,&F_SERIF8,C_GOLD,1,false);
}
// ficha da cidade sob o cursor: bandeira, nome, nivel, populacao e as obras
static void CityCard(u16 *d,const World *w,const WGame *s,int ci){
 const WCity *c=&w->city[ci];
 if(c->owner){Px_Rect(d,10,10,40,28,C_GOLD);Flag_Draw(d,256,12,12,36,24,&NATIONS[w->c[c->owner].nat].flag);}
 char nm[40];City_Name(w,ci,nm,40);
 Px_Text2(d,56,11,nm,&F_SERIF12,C_TEXT);
 char sub[80]={0};Cat(sub,80,Str((u16)(STR_TIER_0+c->tier)));
 if(c->owner){Cat(sub,80," · ");Cat(sub,80,NatName(w,c->owner));}
 Px_Text2(d,56,27,sub,&F_SERIF8,c->owner==s->me?C_GOLD:C_TEXT_DIM);
 Px_Rect(d,10,41,236,1,C_GOLD_DIM);
 char t[40];
 if(c->pop>=1000){Px_Itoa(c->pop/1000,t);Cat(t,40,",");char q[2]={(char)('0'+(c->pop%1000)/100),0};Cat(t,40,q);Cat(t,40," M");}
 else{Px_Itoa(c->pop,t);Cat(t,40,Str(STR_MIL));}
 StatLine(d,10,47,ICO_LAND_S,STR_POPULACAO,t);
 int np=City_NextPop(c->tier);
 Ui_Bar(d,26,59,96,np?c->pop*100/np:100,C_GOLD);
 // obras construidas em miniatura (a cidade "aparece" no painel)
 u8 st=City_Style(w,ci);int x=132;
 for(int k=0;k<CITY_SLOTS;k++){
  u8 b=c->bld[k];if(!b)continue;
  PxC_Building(d,x,62,14,b,st,w->date.y,(u32)ci*7+k,-1,255);x+=15;
 }
 if(c->bq){char q[48]={0};Cat(q,48,Str(STR_EM_OBRA));Cat(q,48,": ");Cat(q,48,Str(BLD[c->bq].nameStr));
  Px_Text2(d,10,68,q,&F_SERIF8,C_GOLD);}
 Px_WrapN(d,10,82,236,c->owner==s->me?Str(STR_ABRIR_CIDADE):Str(STR_CIDADE_DE),&F_SERIF8,C_TEXT_DIM,1,false);
 if(c->owner!=s->me&&c->owner)Px_Text2(d,10+Px_Width2(Str(STR_CIDADE_DE),&F_SERIF8),82,NatName(w,c->owner),&F_SERIF8,C_TEXT_DIM);
}
// despacho diplomatico / decisao: o jogador toca Aceitar ou Recusar
static void PropModal(u16 *d,const World *w){
 const WProp *p=&w->prop;
 Ui_Parch(d,4,4,248,142);
 char title[64]={0},body[200]={0};
 const char *a=Str(STR_ACEITAR),*b=Str(STR_RECUSAR);
 if(p->kind==PR_DECISAO){
  Cat(title,64,Rnd_Title(p->idx));Cat(body,200,Rnd_Body(p->idx));
  a=Rnd_Opt(p->idx,true);b=Rnd_Opt(p->idx,false);
 }else{
  Cat(title,64,Str(STR_PR_TITULO));
  Cat(body,200,NatName(w,p->from));
  switch(p->kind){
   case PR_ALIANCA:Cat(body,200,Str(STR_PR_ALIANCA));break;
   case PR_BLOCO:Cat(body,200,Str(p->about?STR_PR_BLOCO_PEDE:STR_PR_BLOCO_CONV));break;
   case PR_AJUDA:Cat(body,200,Str(STR_PR_AJUDA));Cat(body,200,NatName(w,p->about));Cat(body,200,".");break;
   case PR_TRIBUTO:{char n[16];Px_Itoa((int)p->gold,n);Cat(body,200,Str(STR_PR_TRIBUTO));Cat(body,200,n);Cat(body,200,Str(STR_PR_TRIBUTO2));}break;
   case PR_PAZ:Cat(body,200,Str(STR_PR_PAZ));break;
   default:break;
  }
  if(p->from&&w->c[p->from].alive){FlagDef f;WDip_Flag(w,p->from,&f);Px_Rect(d,205,11,40,28,C_GOLD);Flag_Draw(d,256,207,13,36,24,&f);}
 }
 Px_Text2(d,14,14,title,&F_SERIF12,C_INK);
 Ui_Div(d,64,34,C_INK);
 Px_WrapN(d,14,44,p->kind==PR_DECISAO?226:186,body,&F_SERIF8,C_INK,7,false);
 Px_Rect(d,8,PM_BTN_Y,116,PM_BTN_H,C_CRIMSON);Px_Border(d,8,PM_BTN_Y,116,PM_BTN_H,C_GOLD);
 for(int i=1;i<115;i++)Px_PSet(d,8+i,PM_BTN_Y+1,C_CRIMSON_HI);
 Px_Rect(d,132,PM_BTN_Y,116,PM_BTN_H,C_NAVY);Px_Border(d,132,PM_BTN_Y,116,PM_BTN_H,C_GOLD);
 Px_WrapN(d,12,PM_BTN_Y+6,108,a,&F_SERIF8,C_TEXT,2,true);
 Px_WrapN(d,136,PM_BTN_Y+6,108,b,&F_SERIF8,C_TEXT,2,true);
 // painel sem toque (layout Invertida) responde por A/B: mostra a tecla
 if(Set_Ptr()->layout==LAYOUT_SWAP){Px_Text2(d,114,PM_BTN_Y+26,"A",&F_SERIF8,C_GOLD);Px_Text2(d,238,PM_BTN_Y+26,"B",&F_SERIF8,C_GOLD_DIM);}
}
// folha de acoes (A sobre a mira): titulo = alvo; uma linha tocavel por
// acao, com o preco/% no rotulo. Geometria em WG_ACT_Y/H (wgame.h).
static void DipMenu(u16 *d,const World *w,const WGame *s){
 static const u16 LB[DM_COUNT]={STR_DM_PAZ,STR_DM_ALIANCA,STR_DM_ROMPER,STR_DM_ENTRAR,STR_DM_CONVIDAR,STR_DM_GUERRA,STR_CANCELAR,
  STR_DM_CIDADE,STR_DM_FUNDAR,STR_DM_RECRUTAR,STR_DM_ATACAR,STR_DM_COLONIZAR};
 Ui_Navy(d,4,4,248,184);
 char t[64]={0};
 switch(s->actKind){
  case AK_PAIS:Cat(t,64,Str(STR_DM_TITULO));Cat(t,64,NatName(w,s->dipTo));break;
  case AK_CIDADE:if(s->hcity>=0)City_Name(w,s->hcity,t,64);break;
  case AK_TERRA:Cat(t,64,Str(STR_AC_TERRA));break;
  default:Cat(t,64,Str(STR_TERRA_LIVRE));break;
 }
 if(s->actKind==AK_PAIS&&s->dipTo){FlagDef f;WDip_Flag(w,s->dipTo,&f);Px_Rect(d,10,9,22,15,C_GOLD);Flag_Draw(d,256,11,10,20,13,&f);}
 else Ui_IconId(d,12,9,s->actKind==AK_LIVRE?ICO_LAND_S:ICO_HOUSE,false);
 Px_Text2(d,38,10,t,&F_SERIF12,C_TEXT);
 Px_Rect(d,10,WG_ACT_Y-4,236,1,C_GOLD_DIM);
 for(int i=0;i<s->dipN;i++){
  u8 it=s->dipIt[i];
  int y=WG_ACT_Y+i*WG_ACT_H;
  bool war=it==DM_GUERRA, cancel=it==DM_CANCELAR;
  Px_Rect(d,8,y,240,WG_ACT_H-3,war?C_CRIMSON:(cancel?C_NAVY_BG:C_NAVY_HI));
  Px_Border(d,8,y,240,WG_ACT_H-3,cancel?C_GOLD_DIM:C_GOLD);
  if(!cancel)for(int k=0;k<4;k++)for(int j=k;j<7-k;j++)Px_PSet(d,14+k,y+5+j,C_GOLD);
  char lb[80]={0};Cat(lb,80,Str(LB[it]));
  char n[16];
  if(it==DM_FUNDAR){Px_Itoa((int)City_FoundCost(w,s->me),n);Cat(lb,80,n);Cat(lb,80,Str(STR_AC_OURO));}
  else if(it==DM_RECRUTAR){Px_Itoa((int)World_RecruitCost(w,s->me),n);Cat(lb,80,n);Cat(lb,80,Str(STR_AC_OURO));}
  else if(it==DM_ATACAR||it==DM_COLONIZAR){Px_Itoa(s->pct,n);Cat(lb,80,n);Cat(lb,80,Str(STR_AC_RESERVA));}
  Px_Text2(d,24,y+5,lb,&F_SERIF8,cancel?C_TEXT_DIM:C_TEXT);
 }
}
void PxW_Panel(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();
 Ui_Backdrop(d);
 if(w->prop.kind&&!s->dead){PropModal(d,w);return;}
 Ui_Navy(d,4,4,248,100);
 u8 ci=s->selLand?0:s->sel;
 if(s->hcity>=0)CityCard(d,w,s,s->hcity);
 else if(ci&&w->c[ci].alive)Card(d,w,s,ci);
 else if(s->selLand){
  Ui_Parch(d,10,10,40,28);
  Ui_IconId(d,24,18,ICO_LAND_S,false);
  Px_Text2(d,56,11,Str(STR_TERRA_LIVRE),&F_SERIF12,C_TEXT);
  Px_Rect(d,10,41,236,1,C_GOLD_DIM);
  Px_WrapN(d,10,48,236,Str(STR_DESC_TERRA),&F_SERIF8,C_TEXT_DIM,2,false);
  if(w->c[s->me].atkOn&&!w->c[s->me].atkTo)Px_Text2(d,10,80,Str(STR_COLONIZANDO),&F_SERIF8,C_GOLD);
 }else Card(d,w,s,s->me);
 // despacho: aviso da ultima acao, senao a ultima noticia do mundo
 Ui_Parch(d,4,106,248,42);
 char t[160]={0};
 if(s->toastT){Cat(t,160,Str(s->toastId));if(s->toastArg)Cat(t,160,NatName(w,s->toastArg));
  if(s->toastNum>=0){char n[16];Px_Itoa((int)s->toastNum,n);Cat(t,160,n);Cat(t,160,Str(STR_T_FUNDAR_CONF2));}
  if(s->toastCity>=0){char n[40];City_Name(w,s->toastCity,n,40);Cat(t,160,n);}
  Px_WrapN(d,12,113,232,t,&F_SERIF8,C_INK,3,false);}
 else if(w->msg[0]){
  Date dt=Cal_Date(w->msgDay);char ds[24];Cal_Format(&dt,ds,24);
  Cat(t,160,ds);Cat(t,160," — ");Cat(t,160,w->msg);
  Px_WrapN(d,12,113,232,t,&F_SERIF8,C_INK,3,false);
 }else Px_WrapN(d,12,113,232,Str(STR_DICA_JOGO),&F_SERIF8,C_INK,3,false);
 // acoes
 const WCountry *me=&w->c[s->me];
 bool target=!s->dead&&((s->selLand)||(s->sel&&s->sel!=s->me));
 bool atWar=!s->dead&&s->sel&&!s->selLand&&World_AtWar(w,s->me,s->sel);
 bool canRec=!s->dead&&me->gold>=World_RecruitCost(w,s->me);
 bool ownCity=s->hcity>=0&&w->city[s->hcity].owner==s->me;
 bool canCity=!s->dead&&(ownCity||City_CanFound(w,s->me,s->curX,s->curY));
 bool dip=!s->dead&&s->sel&&!s->selLand&&s->sel!=s->me;
 static const u8 ICO[5]={ICO_SWORD,ICO_FLAG,ICO_SHIELD,ICO_HOUSE,ICO_BOOK};
 const bool en[5]={target,dip||atWar,canRec,canCity,!s->dead};
 const Btn *b=Ui_Tbl(UI_WORLD);
 for(int i=0;i<5;i++)Ui_IconBtn(d,b[i].x,b[i].y,ICO[i],s->btnT&&s->btnDown==b[i].id,!en[i]);
 // % da reserva
 const Btn *bp=&b[5],*bs=&b[6];
 Ui_Navy(d,bp->x,bp->y,bp->w,bp->h);
 char pc[8];Px_Itoa(s->pct,pc);Cat(pc,8,"%");
 Ui_TextC(d,bp->x+bp->w/2,bp->y+7,pc,1,C_TEXT);
 Ui_TextC(d,bp->x+bp->w/2,bp->y+22,Str(STR_TROPAS),0,C_TEXT_DIM);
 // velocidade
 Ui_Navy(d,bs->x,bs->y,bs->w,bs->h);
 if(s->paused){
  Px_Rect(d,bs->x+17,bs->y+7,3,10,C_GOLD);Px_Rect(d,bs->x+24,bs->y+7,3,10,C_GOLD);
  Ui_TextC(d,bs->x+bs->w/2,bs->y+22,Str(STR_PAUSADO),0,C_TEXT_DIM);
 }else{
  int x0=bs->x+bs->w/2-s->speed*5/2;
  for(int i=0;i<s->speed;i++)for(int k=0;k<4;k++)for(int j=k;j<9-k;j++)Px_PSet(d,x0+i*5+k,bs->y+8+j,C_TEXT);
  char sp[4]={(char)('0'+s->speed),'x',0};
  Ui_TextC(d,bs->x+bs->w/2,bs->y+22,sp,0,C_TEXT_DIM);
 }
 if(s->dipOpen)DipMenu(d,w,s);
}

// ---- Terra em sepia (selecao de nacao / preparar partida) ----
void PxW_Earth(u16 *d,int x,int y,int w,int h,int markNat,int pulse){
 static u8 row[EARTH_W],prev[256];
 const u16 LAND=PXC(21,17,11),COAST=PXC(12,9,6);
 int lastEy=-1;
 for(int i=0;i<256;i++)prev[i]=0;
 for(int j=0;j<h;j++){
  int ey=j*EARTH_H/h;
  if(ey!=lastEy){Earth_Row(ey,row);lastEy=ey;}
  u8 cur[256];
  for(int i=0;i<w&&i<256;i++){
   int ex=i*EARTH_W/w;
   cur[i]=row[ex];
   u16 c;
   if(row[ex]){
    bool coast=(i>0&&!cur[i-1])||(j>0&&!prev[i]);
    c=coast?COAST:LAND;
   }else{
    bool coast=(i>0&&cur[i-1])||(j>0&&prev[i]);
    c=coast?COAST:(((y+j)%3)==0?C_PARCH_SH:C_PARCH);
   }
   Px_PSet(d,x+i,y+j,c);
  }
  for(int i=0;i<w&&i<256;i++)prev[i]=cur[i];
 }
 Px_Border(d,x-1,y-1,w+2,h+2,C_GOLD_DIM);
 if(markNat>=0){
  const NationDef *nd=&NATIONS[markNat];
  int px=x+Earth_X(nd->lon)*w/EARTH_W, py=y+Earth_Y(nd->lat)*h/EARTH_H;
  int r=pulse?5:4;
  for(int a=0;a<32;a++){
   // anel por octantes (inteiro): 8 pontos de raio r + diagonais
   static const s8 OX[16]={4,4,3,2,0,-2,-3,-4,-4,-4,-3,-2,0,2,3,4},OY[16]={0,2,3,4,4,4,3,2,0,-2,-3,-4,-4,-4,-3,-2};
   if(a<16)Px_PSet(d,px+OX[a]*r/4,py+OY[a]*r/4,C_CRIMSON);
  }
  Px_Rect(d,px-1,py-1,3,3,C_GOLD_HI);
  Px_PSet(d,px,py,C_INK);
 }
}
