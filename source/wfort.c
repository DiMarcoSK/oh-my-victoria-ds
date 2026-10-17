#include "wfort.h"
#include "city.h"
#include "str.h"
#include "wecon.h"
// wfort.c — fortificacoes (ver wfort.h; SDD ADR-38). Custo: Fort_Day e'
// O(fortes) por dia + a IA de 1 pais por dia (amostra de 48 celulas). A
// cobertura (W_FORTD) e' pintada 1x quando a obra termina: (2r+1)^2 <= 81
// celulas. Forte pronto nunca some (so' troca de dono), entao nao ha' recalculo;
// canteiro perdido libera o slot (owner 0) sem mexer nos indices dos outros.
u8 W_FORTD[W_CELLS];

//                    nome              raio bonus custo dias ano
const FortDef FORT[FT_COUNT]={
 [FT_FORTE]     ={STR_FT_FORTE,       3,  80, 180, 60,    0},
 [FT_TRINCHEIRA]={STR_FT_TRINCHEIRA,  2, 120,  90, 20, 1860},
 [FT_BATERIA]   ={STR_FT_BATERIA,     4,  40, 150, 45,    0},
};

static int Dist(const World *w,int ax,int ay,int bx,int by){
 int dx=ax-bx;if(dx<0)dx=-dx;
 if(w->wrap&&dx>w->w/2)dx=w->w-dx;
 int dy=ay-by;if(dy<0)dy=-dy;
 return dx>dy?dx:dy;
}
static bool Coast(const World *w,int x,int y){
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  int nx=x+dx,ny=y+dy;if(ny<0||ny>=w->h)continue;
  if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
  if(W_TERR[World_Idx(w,nx,ny)]==WT_WATER)return true;
 }
 return false;
}
void Fort_Reset(World *w){
 w->nFort=0;
 for(u32 i=0;i<W_CELLS;i++)W_FORTD[i]=0;
}
int Fort_Count(const World *w,u8 ci){int n=0;if(!ci)return 0;for(int i=0;i<w->nFort;i++)if(w->fort[i].owner==ci)n++;return n;}
s32 Fort_Cost(const World *w,u8 ci,u8 kind){
 if(kind>=FT_COUNT)return 0;
 // cada fortificacao a mais encarece (a fronteira inteira murada sai cara)
 return WEco_Nom(w,ci,(s32)FORT[kind].cost+(s32)Fort_Count(w,ci)*25+(w->date.y-1800)/2);
}
int Fort_At(const World *w,int x,int y){
 if(x<0||y<0||x>=w->w||y>=w->h)return -1;
 for(int i=0;i<w->nFort;i++)if(w->fort[i].owner&&w->fort[i].x==x&&w->fort[i].y==y)return i;
 return -1;
}
int Fort_Near(const World *w,int x,int y,int r){
 int best=-1,bd=r+1;
 for(int i=0;i<w->nFort;i++){if(!w->fort[i].owner)continue;int d=Dist(w,x,y,w->fort[i].x,w->fort[i].y);if(d<bd){bd=d;best=i;}}
 return best;
}
bool Fort_CanBuild(const World *w,u8 ci,int x,int y,u8 kind){
 if(kind>=FT_COUNT||x<0||y<0||x>=w->w||y>=w->h)return false;
 if(w->date.y<FORT[kind].minYear)return false;
 u32 i=World_Idx(w,x,y);
 if(W_OWNER[i]!=ci||W_TERR[i]==WT_WATER||W_TERR[i]==WT_ICE)return false;
 if(kind==FT_BATERIA&&!Coast(w,x,y))return false;
 // espaco: nada a menos de 3 celulas (o mapa nao vira um paliteiro)
 int fr=w->nFort<W_MAXFORT;
 for(int k=0;k<w->nFort;k++){if(!w->fort[k].owner){fr=1;continue;}if(Dist(w,x,y,w->fort[k].x,w->fort[k].y)<3)return false;}
 return fr!=0;
}
int Fort_Build(World *w,u8 ci,int x,int y,u8 kind){
 if(!Fort_CanBuild(w,ci,x,y,kind))return -1;
 s32 cost=Fort_Cost(w,ci,kind);
 if(w->c[ci].gold<cost)return -1;
 int k=0;while(k<w->nFort&&w->fort[k].owner)k++;     // reusa slot de canteiro perdido
 if(k>=W_MAXFORT)return -1;
 w->c[ci].gold-=cost;
 WFort *f=&w->fort[k];
 f->x=(u16)x;f->y=(u16)y;f->owner=ci;f->kind=kind;f->days=FORT[kind].days;f->lvl=1;
 if(k==w->nFort)w->nFort++;
 // a celula do forte fica marcada desde o canteiro (Fort_OnOwner filtra por
 // ela); Fort_DefPct ignora obra inacabada
 {u32 i=World_Idx(w,x,y);if(!W_FORTD[i])W_FORTD[i]=(u8)(k+1);}
 return k;
}
// pinta a cobertura do forte k (so' quando fica pronto)
static void Cover(World *w,int k){
 const WFort *f=&w->fort[k];int r=FORT[f->kind].radius;
 for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){
  int x=f->x+dx,y=f->y+dy;if(y<0||y>=w->h)continue;
  if(x<0||x>=w->w){if(!w->wrap)continue;x=(x+w->w)%w->w;}
  u32 i=World_Idx(w,x,y);
  u8 o=W_FORTD[i];
  // ja' coberta por outro: fica com o de maior bonus (ou o de outro dono, mais novo)
  if(o){const WFort *g=&w->fort[o-1];if(g->owner==f->owner&&FORT[g->kind].bonus>=FORT[f->kind].bonus)continue;}
  W_FORTD[i]=(u8)(k+1);
 }
}
int Fort_DefPct(const World *w,u32 cell,u8 def){
 u8 o=W_FORTD[cell];if(!o||!def)return 0;
 const WFort *f=&w->fort[o-1];
 if(f->owner!=def||f->days)return 0;
 int b=FORT[f->kind].bonus;
 // a propria celula do forte: o dobro (cerco)
 if((u32)f->y*w->w+f->x==cell)b=b*2+100;
 return b;
}
bool Fort_Battery(const World *w,u32 cell,u8 def){
 u8 o=W_FORTD[cell];if(!o)return false;
 const WFort *f=&w->fort[o-1];
 return f->owner==def&&!f->days&&f->kind==FT_BATERIA;
}
void Fort_OnOwner(World *w,u32 cell,u8 to){
 if(!W_FORTD[cell])return;
 int k=Fort_At(w,(int)(cell%w->w),(int)(cell/w->w));
 if(k<0)return;
 WFort *f=&w->fort[k];
 u8 from=f->owner;f->owner=to;
 if(f->days){f->days=0;f->owner=0;return;}   // obra inacabada: o canteiro se perde (sem cobertura, slot livre)
 bool pl=(from&&w->c[from].player)||(to&&w->c[to].player);
 if(pl)World_Msg(w,Str(STR_FORTE_CAIU),Str(FORT[f->kind].nameStr));
 Cover(w,k);   // agora cobre as celulas do novo dono
}
// IA: fortifica a fronteira com quem esta' em guerra com ela (ou com o rival),
// 1 pais por dia, com reserva de ouro. Celula da amostra: dela, na borda.
static void AiFort(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(!c->alive||c->player||(!c->warN&&!c->alert&&!c->rival))return;
 int have=Fort_Count(w,ci),cap=1+(int)(c->cells/160);
 if(have>=cap)return;
 u8 kind=(c->warN&&w->date.y>=1860)?FT_TRINCHEIRA:FT_FORTE;
 s32 cost=Fort_Cost(w,ci,kind);
 if(c->gold<cost*3)return;
 int bw=c->bx1-c->bx0+1,bh=c->by1-c->by0+1;
 for(int s=0;s<48;s++){
  int x=c->bx0+World_RngRange(w,bw),y=c->by0+World_RngRange(w,bh);
  if(x>=w->w||y>=w->h)continue;
  if(W_OWNER[World_Idx(w,x,y)]!=ci)continue;
  bool front=false;
  for(int dy=-1;dy<=1&&!front;dy++)for(int dx=-1;dx<=1;dx++){
   int nx=x+dx,ny=y+dy;if(ny<0||ny>=w->h)continue;
   if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
   u8 o=W_OWNER[World_Idx(w,nx,ny)];
   if(o&&o!=ci&&(World_AtWar(w,ci,o)||o==c->rival)){front=true;break;}
  }
  if(front&&Fort_Build(w,ci,x,y,kind)>=0)return;
 }
}
void Fort_Day(World *w){
 for(int k=0;k<w->nFort;k++){
  WFort *f=&w->fort[k];
  if(!f->days||!f->owner)continue;
  if(!w->c[f->owner].alive){f->days=0;f->owner=0;continue;}
  if(--f->days)continue;
  Cover(w,k);
  if(w->c[f->owner].player)World_Msg(w,Str(STR_FORTE_PRONTO),Str(FORT[f->kind].nameStr));
 }
 if(w->nC>1){u8 ci=(u8)(1+w->day%(u32)(w->nC-1));AiFort(w,ci);}
}
