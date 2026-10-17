#include "wgame.h"
// px_sea.c — rotas maritimas pelo MAR (ADR-33). Antes a rota era um arco
// reto entre os dois portos e cortava continentes. Agora:
//  - grade grossa de mar: 1 no' = bloco 4x4 celulas (128x48 nos no mapa-mundi);
//    o no' e' mar se >= 3 das 16 celulas sao agua (estreitos de 1 celula, como
//    Gibraltar e o Bosforo, continuam abertos);
//  - porto -> no' de mar mais proximo da cidade (raio 2 nos);
//  - caminho = BFS 8-vizinhos (volta do mundo em X) + "puxar o fio" (pula
//    nos enquanto houver linha de mar direta), no maximo SEA_PTS pontos;
//  - cache de SEA_CACHE caminhos (LRU) e no maximo SEA_BUDGET buscas por
//    quadro: uma BFS custa ~6K nos (~1 ms no DS), nunca engasga.
// So' o render usa (a simulacao nao le): deterministico, sem RNG, sem malloc.
enum { SG=4, SEA_MAXN=(W_MAXW/SG)*(W_MAXH/SG), SEA_PTS=48, SEA_CACHE=32, SEA_BUDGET=2 };
static u8 SEA[SEA_MAXN];             // 1 = mar
static int SW,SH;                    // nos por linha/coluna
static u32 seaSeed=0;static bool seaOk=false;
static u16 Q[SEA_MAXN],PAR[SEA_MAXN];
typedef struct { s16 a,b; u8 n; u8 ok; u32 used; u16 p[SEA_PTS]; } SeaPath;
static SeaPath CACHE[SEA_CACHE];
static u32 useTick=1;static int budget=SEA_BUDGET;

static void Build(const World *w){
 SW=w->w/SG;SH=w->h/SG;
 for(int ny=0;ny<SH;ny++)for(int nx=0;nx<SW;nx++){
  int wat=0;
  for(int j=0;j<SG;j++)for(int i=0;i<SG;i++)if(W_TERR[World_Idx(w,nx*SG+i,ny*SG+j)]==WT_WATER)wat++;
  SEA[ny*SW+nx]=wat>=3;
 }
 for(int k=0;k<SEA_CACHE;k++)CACHE[k].ok=0;
 seaSeed=w->seed^((u32)w->w<<20)^w->mode;seaOk=true;
}
void PxS_Reset(void){seaOk=false;}
void PxS_Frame(void){budget=SEA_BUDGET;}
static void Need(const World *w){u32 k=w->seed^((u32)w->w<<20)^w->mode;if(!seaOk||k!=seaSeed)Build(w);}
static int WrapN(const World *w,int nx){if(w->wrap){nx%=SW;if(nx<0)nx+=SW;}return nx;}
// no' de mar mais perto da celula (x,y); -1 se nao ha mar em 2 nos
int PxS_Node(const World *w,int x,int y){
 Need(w);
 int cx=x/SG,cy=y/SG,best=-1,bd=99;
 for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
  int nx=WrapN(w,cx+dx),ny=cy+dy;
  if(ny<0||ny>=SH||nx<0||nx>=SW||!SEA[ny*SW+nx])continue;
  int d=dx*dx+dy*dy;if(d<bd){bd=d;best=ny*SW+nx;}
 }
 return best;
}
// linha de mar direta entre dois nos (Bresenham na grade grossa)
static bool Los(const World *w,int a,int b){
 int x0=a%SW,y0=a/SW,x1=b%SW,y1=b/SW;
 int dx=x1-x0;if(w->wrap){if(dx>SW/2)dx-=SW;else if(dx<-SW/2)dx+=SW;}
 int dy=y1-y0,n=(dx<0?-dx:dx)>(dy<0?-dy:dy)?(dx<0?-dx:dx):(dy<0?-dy:dy);
 for(int k=1;k<n;k++){
  int x=WrapN(w,x0+dx*k/n),y=y0+dy*k/n;
  if(!SEA[y*SW+x])return false;
 }
 return true;
}
static bool Bfs(const World *w,int a,int b,SeaPath *o){
 int n=SW*SH;
 for(int i=0;i<n;i++)PAR[i]=0xFFFF;
 int h=0,t=0;Q[t++]=(u16)a;PAR[a]=(u16)a;
 static const s8 DX[8]={1,-1,0,0,1,1,-1,-1},DY[8]={0,0,1,-1,1,-1,1,-1};
 while(h<t){
  int c=Q[h++];if(c==b)break;
  int cx=c%SW,cy=c/SW;
  for(int k=0;k<8;k++){
   int nx=cx+DX[k],ny=cy+DY[k];
   if(ny<0||ny>=SH)continue;
   if(nx<0||nx>=SW){if(!w->wrap)continue;nx=WrapN(w,nx);}
   int j=ny*SW+nx;
   if(!SEA[j]||PAR[j]!=0xFFFF)continue;
   // diagonal so' se um dos lados e' mar (nao atravessa istmo na quina)
   if(k>=4&&!SEA[cy*SW+WrapN(w,cx+DX[k])]&&!SEA[ny*SW+cx])continue;
   PAR[j]=(u16)c;Q[t++]=(u16)j;
  }
 }
 if(PAR[b]==0xFFFF)return false;
 // caminho de tras p/ frente em Q (reuso), depois puxa o fio
 int m=0;for(int c=b;;c=PAR[c]){Q[m++]=(u16)c;if(c==a||m>=SEA_MAXN)break;}
 int k=0;o->p[k++]=Q[m-1];
 int i=m-1;
 while(i>0&&k<SEA_PTS){
  int j=i-1,lim=i-12<0?0:i-12;
  for(int c=lim;c<i-1;c++)if(Los(w,Q[i],Q[c])){j=c;break;}
  o->p[k++]=Q[j];i=j;
 }
 if(i>0)o->p[k-1]=Q[0];       // estourou SEA_PTS: o ultimo ponto e' sempre o porto de destino
 o->n=(u8)k;
 return true;
}
// caminho maritimo entre as cidades-porto a e b (indices de cidade). Devolve
// o numero de pontos (nos) e *pts; 0 = sem caminho; -1 = ainda nao calculado
// (orcamento do quadro acabou: tente no proximo).
int PxS_Path(const World *w,int a,int b,const u16 **pts){
 Need(w);
 int lru=0;u32 lu=0xFFFFFFFF;
 for(int k=0;k<SEA_CACHE;k++){
  SeaPath *c=&CACHE[k];
  if(c->ok&&c->a==a&&c->b==b){c->used=useTick++;*pts=c->p;return c->ok==2?0:c->n;}
  if(!c->ok){lru=k;lu=0;}
  else if(c->used<lu){lu=c->used;lru=k;}
 }
 if(budget<=0)return -1;
 budget--;
 SeaPath *c=&CACHE[lru];c->a=(s16)a;c->b=(s16)b;c->used=useTick++;c->n=0;
 int na=PxS_Node(w,w->city[a].x,w->city[a].y),nb=PxS_Node(w,w->city[b].x,w->city[b].y);
 if(na<0||nb<0||!Bfs(w,na,nb,c)){c->ok=2;*pts=c->p;return 0;}
 c->ok=1;*pts=c->p;return c->n;
}
void PxS_NodeCell(int n,int *x,int *y){*x=(n%SW)*SG+SG/2;*y=(n/SW)*SG+SG/2;}
