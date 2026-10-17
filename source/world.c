#include "world.h"
#include "hot.h"   // HOT_ARM: dia em ARM (divisao por constante = smull exato; ADR-36)
#include "city.h"
#include "earth.h"
#include "history.h"
#include "wecon.h"
#include "wdiplo.h"
// world.c — mundo celular M1 (DESIGN.md sec 4.1, 4.5, 4.6).
// Custos e taxas sao proposta de design: balancear aqui.

u8 W_OWNER[W_CELLS];
u32 W_OwnVer;          // sobe a cada dono trocado (SetOwner): o render sabe se precisa reler (ADR-36)
u8 W_TERR[W_CELLS];
u8 W_CORE[W_CELLS];
static u8 W_AUX[W_CELLS];   // geracao: distancia da semente | dia: carimbo de conquista
u8 W_LIM[W_CELLS];   // geracao: raio da semente | geracao aleatoria: elevacao

// custo base de tomar 1 celula, por terreno (agua = intransponivel)
static const u8 TER_COST[WT_COUNT]={0,10,16,18,30,14,20,20,16,10,40};
// populacao por terreno (x3 se nucleo): tundra vazia nao vira exercito
static const u8 TER_POP[WT_COUNT]={0,4,3,3,1,1,2,2,1,2,0};
static const s8 DIFF_ATK[4]={-15,0,15,30};          // bonus de ataque da IA
static const u8 ARCH_TH[WA_COUNT]={45,70,90,55,75,60}; // % do teto p/ atacar
static const u8 ARCH_PCT[WA_COUNT]={60,35,25,50,30,50};// % da reserva enviada

// ===================== RNG / utilidades =====================
u32 World_Rng(World *w){
 u32 x=w->rng?w->rng:1u;
 x^=x<<13;x^=x>>17;x^=x<<5;w->rng=x;return x;
}
int World_RngRange(World *w,int n){return n<=0?0:(int)(World_Rng(w)%(u32)n);}
static inline int Iabs(int v){return v<0?-v:v;}
static inline s32 Max32(s32 a,s32 b){return a>b?a:b;}
static int Isqrt(u32 v){u32 r=0,b=1u<<30;while(b>v)b>>=2;
 while(b){if(v>=r+b){v-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}return (int)r;}

static u8 H8(int x,int y,u32 s){
 u32 h=(u32)x*374761393u+(u32)y*668265263u+s*2246822519u;
 h=(h^(h>>13))*1274126177u;h^=h>>16;return (u8)(h>>24);
}
static int VNoise(int x,int y,int sc,u32 s){
 int gx=x/sc,gy=y/sc,fx=x%sc,fy=y%sc;
 int a=H8(gx,gy,s),b=H8(gx+1,gy,s),c=H8(gx,gy+1,s),d=H8(gx+1,gy+1,s);
 int ab=a*(sc-fx)+b*fx, cd=c*(sc-fx)+d*fx;
 return (ab*(sc-fy)+cd*fy)/(sc*sc);
}
static int Fractal(int x,int y,u32 s){
 return (VNoise(x,y,32,s)*8+VNoise(x,y,16,s+1)*4+VNoise(x,y,8,s+2)*2+VNoise(x,y,4,s+3))/15;
}

// vizinho n (0..3) de (x,y); -1 se fora do mapa (x da volta no mapa-mundi)
static int Nb(const World *w,int x,int y,int n){
 static const s8 DX[4]={1,-1,0,0},DY[4]={0,0,1,-1};
 int nx=x+DX[n],ny=y+DY[n];
 if(ny<0||ny>=w->h)return -1;
 if(nx<0||nx>=w->w){if(!w->wrap)return -1;nx=(nx+w->w)%w->w;}
 return (int)World_Idx(w,nx,ny);
}

// ===================== guerra =====================
static inline u32 WarBit(u8 a,u8 b){return (u32)a*(W_MAXC+1)+b;}
bool World_AtWar(const World *w,u8 a,u8 b){
 if(!a||!b||a==b)return false;
 u32 k=WarBit(a,b);return (w->war[k>>3]>>(k&7))&1;
}
static int HistWar(const World *w,u8 a,u8 b){
 for(int k=0;k<w->hwN;k++)if((w->hw[k][0]==a&&w->hw[k][1]==b)||(w->hw[k][0]==b&&w->hw[k][1]==a))return k;
 return -1;
}
static void SetWar(World *w,u8 a,u8 b,bool on){
 if(!on){int k=HistWar(w,a,b);if(k>=0){w->hwN--;w->hw[k][0]=w->hw[w->hwN][0];w->hw[k][1]=w->hw[w->hwN][1];}}
 u32 k1=WarBit(a,b),k2=WarBit(b,a);
 bool was=World_AtWar(w,a,b);
 if(on){w->war[k1>>3]|=(u8)(1<<(k1&7));w->war[k2>>3]|=(u8)(1<<(k2&7));}
 else{w->war[k1>>3]&=(u8)~(1<<(k1&7));w->war[k2>>3]&=(u8)~(1<<(k2&7));}
 if(on&&!was){w->c[a].warN++;w->c[b].warN++;w->nWars++;}
 if(!on&&was){if(w->c[a].warN)w->c[a].warN--;if(w->c[b].warN)w->c[b].warN--;}
}
void World_DeclareWar(World *w,u8 a,u8 b){
 if(!a||!b||a==b||!w->c[a].alive||!w->c[b].alive)return;
 SetWar(w,a,b,true);
}
// guerra de IA/jogador: os aliados de cada lado sao chamados (wdiplo.c)
// b (e quem e' chamado a defende-lo) passa a pensar todo dia por um tempo:
// a IA responde a ataque em 1-2 dias, nao no proximo turno de 8 dias.
static void Alert(World *w,u8 b){
 if(!b||b>=w->nC)return;
 w->c[b].alert=20;
 for(int k=0;k<2;k++){u8 a=w->c[b].ally[k];if(a&&a<w->nC&&w->c[a].alive)w->c[a].alert=20;}
}
void World_DeclareWarCall(World *w,u8 a,u8 b){
 if(!a||!b||a==b||!w->c[a].alive||!w->c[b].alive||World_AtWar(w,a,b))return;
 SetWar(w,a,b,true);
 WDip_Unally(w,a,b);
 WDip_OnWar(w,a,b);
}
void World_DeclareHistWar(World *w,u8 a,u8 b){
 if(!a||!b||a==b||!w->c[a].alive||!w->c[b].alive)return;
 SetWar(w,a,b,true);
 if(HistWar(w,a,b)<0&&w->hwN<W_HWMAX){w->hw[w->hwN][0]=a;w->hw[w->hwN][1]=b;w->hwN++;}
}
void World_Peace(World *w,u8 a,u8 b){
 if(!a||!b||a==b)return;
 SetWar(w,a,b,false);
 if(w->c[a].atkOn&&w->c[a].atkTo==b){w->c[a].troops+=w->c[a].atkTroops;w->c[a].atkOn=false;w->c[a].atkTroops=0;}
 if(w->c[b].atkOn&&w->c[b].atkTo==a){w->c[b].troops+=w->c[b].atkTroops;w->c[b].atkOn=false;w->c[b].atkTroops=0;}
}

// ===================== paises =====================
static u8 ArchOf(const u8 t[NAT_TRAITS]){
 bool iso=false,rev=false,exp=false,mer=false,dip=false;
 for(int i=0;i<NAT_TRAITS;i++)switch(t[i]){
  case TR_ISOLAMENTO:iso=true;break;
  case TR_REVOLUCAO:rev=true;break;
  case TR_EXERCITO:case TR_LEVEE:case TR_CAVALEIROS:case TR_FRONTEIRA:case TR_COLONIAL:exp=true;break;
  case TR_ENTREPOSTO:case TR_MARES:case TR_INDUSTRIAL:mer=true;break;
  case TR_CONFEDERACAO:case TR_COESA:case TR_FE:dip=true;break;
  default:break;
 }
 if(iso)return WA_ISOLACIONISTA;
 if(rev)return WA_REVOLUCIONARIO;
 if(exp)return WA_EXPANSIONISTA;
 if(mer)return WA_MERCADOR;
 if(dip)return WA_DIPLOMATA;
 return WA_OPORTUNISTA;
}
void World_SetTraits(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 Tr_Mods(c->traits,&c->m);
 WDip_IdeoMods(c->ideo,&c->m);
 c->archetype=ArchOf(c->traits);
}
u8 World_CountryOfNat(const World *w,int nat){
 if(nat<0||nat>255)return 0;
 u8 ci=w->natToC[nat];
 return (ci&&w->c[ci].alive&&w->c[ci].nat==nat)?ci:0;
}
void World_LatLonPos(const World *w,int lat,int lon,int *x,int *y){
 if(w->mode==WM_MUNDI){*x=Earth_X(lon);*y=Earth_Y(lat);return;}
 // fora do mapa-mundi: projeta no retangulo do mapa (so p/ eventos, raros)
 *x=((lon+180)*w->w)/360;*y=((90-lat)*w->h)/180;
 if(*x<0)*x=0;
 if(*x>=w->w)*x=w->w-1;
 if(*y<0)*y=0;
 if(*y>=w->h)*y=w->h-1;
}
void World_NatPos(const World *w,int nat,int *x,int *y){
 World_LatLonPos(w,NATIONS[nat].lat,NATIONS[nat].lon,x,y);
}
static void Grow(WCountry *c,int x,int y){
 if(x<c->bx0)c->bx0=(u16)x;
 if(x>c->bx1)c->bx1=(u16)x;
 if(y<c->by0)c->by0=(u16)y;
 if(y>c->by1)c->by1=(u16)y;
}
// Inicializa o slot ci como a nacao nat (sem celulas).
static void InitCountry(World *w,u8 ci,int nat,int cx,int cy){
 const NationDef *d=&NATIONS[nat];
 WCountry *c=&w->c[ci];
 u8 *z=(u8*)c;for(unsigned i=0;i<sizeof(*c);i++)z[i]=0;
 c->nat=(u8)nat;c->alive=true;c->kind=d->kind;c->gov=d->gov;
 c->ideo=c->ideo0=WDip_IdeoOfGov(d->gov);c->mood=(u8)(((u32)nat*2654435761u^w->seed)%101u);
 {WEco *e=&w->eco[ci];u8 *q=(u8*)e;for(unsigned i=0;i<sizeof(*e);i++)q[i]=0;
  e->tax=1;e->life=70;e->ind=100;e->hub=-1;e->tariff=1;e->edu=1;e->mil=1;e->rate=4;
  for(int g=0;g<GD_COUNT;g++){e->price[g]=GD_BASE[g];e->sat[g]=100;}}
 for(int i=0;i<NAT_TRAITS;i++)c->traits[i]=d->traits[i];
 World_SetTraits(w,ci);
 c->capX=(u16)cx;c->capY=(u16)cy;c->bx0=c->bx1=(u16)cx;c->by0=c->by1=(u16)cy;
 c->happy=(u8)(60+c->m.happy);c->militancy=(u8)(20+(c->m.militancy>-20?c->m.militancy:-20));
 c->prestige=d->kind==NK_ESTADO?20:10;
 // prestigio de 1800 das potencias (ordem de grandeza do concerto europeu e
 // dos grandes imperios asiaticos): o ranking comeca com a cara da epoca
 static const struct { const char *tag; u8 p; } PR[]={
  {"GBR",100},{"FRA",90},{"RUS",85},{"HAB",80},{"PRU",70},{"QNG",60},{"OTT",55},{"ESP",50},
  {"MAR",35},{"MUG",30},{"POR",30},{"SWE",30},{"NED",30},{"USA",30},{"DAN",25},{"JAP",25},
  {"QAJ",25},{"SAR",20},{"NAP",20},{"BAV",20},{"EIC",25},{"KOR",20},{"BRM",20},{"SIA",20}};
 for(unsigned k=0;k<sizeof PR/sizeof PR[0];k++){const char *t=PR[k].tag;
  if(t[0]==d->tag[0]&&t[1]==d->tag[1]&&t[2]==d->tag[2]){c->prestige=PR[k].p;break;}}
 w->natToC[nat]=ci;
}
static u8 NewSlot(World *w,int nat){
 u8 old=w->natToC[nat>=0&&nat<256?nat:0];
 if(old&&!w->c[old].alive)return old;   // reaproveita o slot de quem morreu
 for(u8 i=1;i<w->nC;i++)if(!w->c[i].alive&&w->c[i].cells==0&&w->natToC[w->c[i].nat]!=i)return i;
 if(w->nC>W_MAXC)return 0;
 return (u8)w->nC++;
}
static u32 CellW(const World *w,u32 i,u8 ci){
 u32 t=TER_POP[W_TERR[i]];
 return W_CORE[i]==w->c[ci].nat+1?t*3:t;
}
static void SetOwner(World *w,u32 i,u8 to){
 u8 from=W_OWNER[i];
 if(from==to)return;
 u8 t=W_TERR[i];
 u32 moved=0;   // populacao real que muda de dono com a celula (parte proporcional)
 if(from){WCountry *f=&w->c[from];if(f->cells)f->cells--;u32 cw=CellW(w,i,from);
  if(f->pop&&f->popK){moved=(u32)((u64)f->popK*cw/f->pop);if(moved>f->popK)moved=f->popK;f->popK-=moved;}
  f->pop=f->pop>cw?f->pop-cw:0;if(f->terr[t])f->terr[t]--;}
 else moved=TER_POP[t]*20;   // terra livre: poucos milhares de habitantes
 W_OWNER[i]=to;W_OwnVer++;
 // render incremental: so' o bloco 8x8 desta celula sera redesenhado
 {u32 b=(i/w->w/8)*W_BLK_W+(i%w->w)/8;w->dirtyBlk[b>>3]|=(u8)(1u<<(b&7));}
 if((W_CITYB[i>>3]>>(i&7))&1)City_OnOwner(w,i,to);
 if(to){w->c[to].cells++;w->c[to].pop+=CellW(w,i,to);w->c[to].terr[t]++;w->c[to].popK+=moved;Grow(&w->c[to],(int)(i%w->w),(int)(i/w->w));}
}
static void RecalcAllPop(World *w){
 for(u8 ci=1;ci<w->nC;ci++){w->c[ci].pop=0;for(int t=0;t<WT_COUNT;t++)w->c[ci].terr[t]=0;}
 u32 n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++){u8 o=W_OWNER[i];if(o){w->c[o].pop+=CellW(w,i,o);w->c[o].terr[W_TERR[i]]++;}}
}
void World_RecountTerr(World *w){RecalcAllPop(w);}
// troca o nucleo `from` (nacao) por `to` no mapa inteiro
static void Recore(World *w,int from,int to){
 u32 n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++)if(W_CORE[i]==from+1)W_CORE[i]=(u8)(to+1);
 RecalcAllPop(w);
}
// piso do mapa-mundi: a IA nao toma mais que 1/3 (historia ligada) ou 2/3 do pico de um pais;
// engolir paises inteiros fica para os eventos de anexacao
static bool AtFloor(const World *w,u8 o){
 const WCountry *d=&w->c[o];
 if(w->mode!=WM_MUNDI||!o||d->player)return false;
 return (u32)d->cells*3<=(u32)d->peak*(w->hist==WH_LIGADA?2:1);
}
static void Kill(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 c->alive=false;c->atkOn=false;c->troops=0;
 WDip_OnDeath(w,ci);
 for(u8 j=1;j<w->nC;j++){
  if(World_AtWar(w,ci,j))SetWar(w,ci,j,false);
  if(w->c[j].atkOn&&w->c[j].atkTo==ci){w->c[j].troops+=w->c[j].atkTroops;w->c[j].atkOn=false;}
  if(w->c[j].overlord==ci)w->c[j].overlord=0;
 }
}
static void Recap(WCountry *c){
 c->cap=(s32)(((s32)c->pop+50+c->cityCap)*(100+c->m.capPct)/100);
}
static void FindNewCapital(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 for(int y=c->by0;y<=c->by1;y++)for(int x=c->bx0;x<=c->bx1;x++)
  if(W_OWNER[World_Idx(w,x,y)]==ci){c->capX=(u16)x;c->capY=(u16)y;c->capLost=false;return;}
 c->capLost=false;
}
static bool IsCoastal(const World *w,int x,int y){
 for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
  int nx=x+dx,ny=y+dy;
  if(ny<0||ny>=w->h)continue;
  if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
  if(W_TERR[World_Idx(w,nx,ny)]==WT_WATER)return true;
 }
 return false;
}

int World_Transfer(World *w,u8 to,int src,int cx,int cy,int radius){
 int n=0,r2=radius*radius;
 for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
  if(dx*dx+dy*dy>r2)continue;
  int x=cx+dx,y=cy+dy;
  if(y<0||y>=w->h)continue;
  if(x<0||x>=w->w){if(!w->wrap)continue;x=(x+w->w)%w->w;}
  u32 i=World_Idx(w,x,y);
  u8 o=W_OWNER[i];
  if(W_TERR[i]==WT_WATER||o==to)continue;
  if(src>=0&&o!=src)continue;
  if(o&&w->c[o].capX==x&&w->c[o].capY==y)continue; // nunca rouba capital alheia
  SetOwner(w,i,to);n++;
 }
 for(u8 j=1;j<w->nC;j++)if(w->c[j].alive&&w->c[j].cells==0&&j!=to)Kill(w,j);
 return n;
}
u8 World_Spawn(World *w,int nat,int src,int cx,int cy,int radius){
 if(nat<0||World_CountryOfNat(w,nat))return 0;
 u8 ci=NewSlot(w,nat);
 if(!ci)return 0;
 InitCountry(w,ci,nat,cx,cy);
 int n=World_Transfer(w,ci,src,cx,cy,radius);
 if(n==0){w->c[ci].alive=false;return 0;}
 {u32 all=(u32)w->w*w->h;for(u32 i=0;i<all;i++)if(W_OWNER[i]==ci)W_CORE[i]=(u8)(nat+1);}
 RecalcAllPop(w);
 if(W_OWNER[World_Idx(w,cx,cy)]!=ci)FindNewCapital(w,ci);
 WCountry *c=&w->c[ci];
 Recap(c);c->troops=c->cap/2;c->gold=(s32)c->cells*2;
 c->coastal=IsCoastal(w,c->capX,c->capY);
 const NationDef *d=&NATIONS[nat];
 if(d->overlord){int o=Nat_Find(d->overlord);c->overlord=World_CountryOfNat(w,o);}
 return ci;
}
void World_Annex(World *w,u8 a,u8 b){
 if(!a||!b||a==b||!w->c[a].alive||!w->c[b].alive)return;
 WCountry *cb=&w->c[b];
 for(int y=cb->by0;y<=cb->by1;y++)for(int x=cb->bx0;x<=cb->bx1;x++){
  u32 i=World_Idx(w,x,y);
  if(W_OWNER[i]==b)SetOwner(w,i,a);
 }
 w->c[a].troops+=cb->troops/2;w->c[a].gold+=cb->gold;
 Kill(w,b);
}
void World_Unify(World *w,u8 a,u8 b){
 if(!a||!b||a==b||!w->c[a].alive||!w->c[b].alive)return;
 int nb=w->c[b].nat;
 World_Annex(w,a,b);
 Recore(w,nb,w->c[a].nat);
}
void World_MoveAll(World *w,u8 from,u8 to){
 if(!from||!to||from==to)return;
 WCountry *cf=&w->c[from];
 for(int y=cf->by0;y<=cf->by1;y++)for(int x=cf->bx0;x<=cf->bx1;x++){
  u32 i=World_Idx(w,x,y);
  if(W_OWNER[i]==from)SetOwner(w,i,to);
 }
}
void World_Kill(World *w,u8 ci){if(ci&&w->c[ci].alive)Kill(w,ci);}
void World_Become(World *w,u8 ci,int nat){
 if(!ci||!w->c[ci].alive||nat<0)return;
 WCountry *c=&w->c[ci];
 const NationDef *d=&NATIONS[nat];
 int old=c->nat;
 if(w->natToC[c->nat]==ci)w->natToC[c->nat]=0;
 c->nat=(u8)nat;w->natToC[nat]=ci;
 Recore(w,old,nat);
 c->kind=d->kind;c->gov=d->gov;
 // nova bandeira de base: se a ideologia era a de base, segue a nova
 {u8 o0=c->ideo0;c->ideo0=WDip_IdeoOfGov(d->gov);if(c->ideo==o0)c->ideo=c->ideo0;}
 for(int i=0;i<NAT_TRAITS;i++)c->traits[i]=d->traits[i];
 World_SetTraits(w,ci);
 c->overlord=0;
 if(d->overlord){int o=Nat_Find(d->overlord);c->overlord=World_CountryOfNat(w,o);}
}
void World_Msg(World *w,const char *a,const char *b){
 int p=0;
 while(a&&*a&&p<127)w->msg[p++]=*a++;
 if(b&&*b&&p<125){w->msg[p++]=':';w->msg[p++]=' ';while(*b&&p<127)w->msg[p++]=*b++;}
 w->msg[p]=0;w->msgDay=w->day;
}

// ===================== geracao =====================
static void Reset(World *w,u32 seed,u8 mode,u16 W,u16 H,u8 diff){
 u8 *z=(u8*)w;for(unsigned i=0;i<sizeof(*w);i++)z[i]=0;
 w->w=W;w->h=H;w->mode=mode;w->diff=diff>3?3:diff;w->seed=seed;w->rng=seed?seed:0x9E3779B9u;
 w->wrap=(mode==WM_MUNDI);w->nC=1;
 w->date.y=CAL_START_YEAR;w->date.m=1;w->date.d=1;w->endDay=Cal_EndDay();
 w->hist=WH_DESLIGADA;
 for(u32 i=0;i<(u32)W*H;i++){W_OWNER[i]=0;W_TERR[i]=WT_WATER;W_AUX[i]=255;W_LIM[i]=0;W_CORE[i]=0;}
}
// t = 0 (equador) .. 100 (polo); e = percentil de elevacao da terra (0..100)
static u8 TerrAt(int t,int e,int m){
 if(t>=92)return WT_ICE;
 if(t>=74)return e>=94?WT_MOUNTAIN:WT_TUNDRA;
 if(e>=94)return WT_MOUNTAIN;
 if(e>=84)return WT_HILLS;
 if(t<18&&m>=130)return WT_JUNGLE;
 if(t>=16&&t<40&&m<110)return WT_DESERT;
 if(t>=40&&t<72&&m<105)return WT_STEPPE;
 if(m>=185)return WT_SWAMP;
 if(m>=135)return WT_FOREST;
 return WT_PLAINS;
}
// Atribui terreno a toda celula de terra (W_TERR!=agua) usando elevacao em
// W_LIM (0..255) convertida p/ percentil entre as celulas de terra.
static void Biomes(World *w,int (*lat100)(const World*,int)){
 u32 hist[256];for(int i=0;i<256;i++)hist[i]=0;
 u32 land=0,n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++)if(W_TERR[i]!=WT_WATER){hist[W_LIM[i]]++;land++;}
 u8 pct[256];u32 acc=0;
 for(int v=0;v<256;v++){pct[v]=(u8)(land?(acc*100)/land:0);acc+=hist[v];}
 for(int y=0;y<w->h;y++){
  int t=lat100(w,y);
  for(int x=0;x<w->w;x++){
   u32 i=World_Idx(w,x,y);
   if(W_TERR[i]==WT_WATER)continue;
   int m=Fractal(x,y,w->seed*7u+11u);
   W_TERR[i]=TerrAt(t,pct[W_LIM[i]],m);
  }
 }
}
static int LatRandom(const World *w,int y){
 int half=w->h/2;return (Iabs(y-half)*100)/(half?half:1);
}
static int LatMundi(const World *w,int y){
 int lat=EARTH_LAT_TOP-(y*EARTH_LAT_SPAN)/w->h;
 return (Iabs(lat)*100)/90;
}

// Expansao sincronizada de todas as sementes (W_AUX = distancia, W_LIM =
// raio da semente). fill: depois do raio, continua ate cobrir a terra.
static void Expand(World *w,bool fill){
 u32 n=(u32)w->w*w->h;
 for(int pass=0;pass<(fill?2:1);pass++){
  if(pass==1)for(u32 i=0;i<n;i++)if(W_OWNER[i])W_LIM[i]=255;
  for(int d=0;d<254;d++){
   bool any=false;
   for(int y=0;y<w->h;y++)for(int x=0;x<w->w;x++){
    u32 i=World_Idx(w,x,y);
    if(W_AUX[i]!=d||!W_OWNER[i]||d>=W_LIM[i])continue;
    for(int k=0;k<4;k++){
     int j=Nb(w,x,y,k);
     if(j<0||W_TERR[j]==WT_WATER||W_OWNER[j]||W_AUX[j]!=255)continue;
     W_OWNER[j]=W_OWNER[i];W_AUX[j]=(u8)(d+1);W_LIM[j]=W_LIM[i];any=true;
    }
   }
   if(!any&&d>24)break;
  }
 }
}
// Depois da expansao: conta celulas, caixas, tropas iniciais.
static void Settle(World *w){
 for(u8 ci=1;ci<w->nC;ci++){w->c[ci].cells=0;w->c[ci].pop=0;for(int t=0;t<WT_COUNT;t++)w->c[ci].terr[t]=0;}
 for(int y=0;y<w->h;y++)for(int x=0;x<w->w;x++){
  u32 i=World_Idx(w,x,y);
  u8 o=W_OWNER[i];
  if(o){W_CORE[i]=(u8)(w->c[o].nat+1);w->c[o].cells++;w->c[o].pop+=CellW(w,i,o);w->c[o].terr[W_TERR[i]]++;Grow(&w->c[o],x,y);}
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  if(!c->cells){c->alive=false;continue;}
  Recap(c);c->troops=c->cap*6/10;c->gold=100+(s32)c->cells/2;c->peak=(u16)(c->cells>65535?65535:c->cells);
  c->coastal=IsCoastal(w,c->capX,c->capY);
 }
 u32 n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++){W_AUX[i]=0;W_LIM[i]=0;}
}
static void Seed(World *w,u8 ci,int x,int y,int r){
 u32 i=World_Idx(w,x,y);
 if(W_OWNER[i])return;
 W_OWNER[i]=ci;W_AUX[i]=0;W_LIM[i]=(u8)(r>250?250:r);
}

void World_GenRandom(World *w,u32 seed,u8 size,u8 nNations,u8 diff,int playerNat){
 static const u16 SW[3]={256,384,512},SH[3]={96,144,192};
 if(size>WS_GRANDE)size=WS_GRANDE;
 Reset(w,seed,WM_ALEATORIO,SW[size],SH[size],diff);
 // elevacao com queda nas bordas -> continentes e ilhas
 u32 n=(u32)w->w*w->h;
 u32 hist[256];for(int i=0;i<256;i++)hist[i]=0;
 int mx=w->w/6,my=w->h/6;
 for(int y=0;y<w->h;y++)for(int x=0;x<w->w;x++){
  int e=Fractal(x,y,seed);
  int dx=x<w->w-1-x?x:w->w-1-x, dy=y<w->h-1-y?y:w->h-1-y;
  if(dx<mx)e-=((mx-dx)*90)/mx;
  if(dy<my)e-=((my-dy)*90)/my;
  if(e<0)e=0;
  if(e>255)e=255;
  W_LIM[World_Idx(w,x,y)]=(u8)e;hist[e]++;
 }
 u32 want=(n*40)/100,acc=0;int th=255;
 while(th>0&&acc+hist[th]<want){acc+=hist[th];th--;}
 for(u32 i=0;i<n;i++)W_TERR[i]=W_LIM[i]>=th?WT_PLAINS:WT_WATER;
 Biomes(w,LatRandom);
 // nacoes candidatas: independentes que existem em 1800 (nao autonomas)
 u8 pool[256];int np=0;
 for(int i=0;i<NATION_COUNT&&np<256;i++){
  const NationDef *d=&NATIONS[i];
  if(d->yFrom||d->kind==NK_AUTONOMO||d->kind==NK_SUDITO||i==playerNat)continue;
  pool[np++]=(u8)i;
 }
 for(int i=np-1;i>0;i--){int j=World_RngRange(w,i+1);u8 t=pool[i];pool[i]=pool[j];pool[j]=t;}
 if(nNations<2)nNations=2;
 if(nNations>W_MAXC)nNations=W_MAXC;
 int want_n=nNations;
 u8 pick[W_MAXC];int pk=0;
 if(playerNat>=0&&playerNat<NATION_COUNT)pick[pk++]=(u8)playerNat;
 for(int i=0;i<np&&pk<want_n;i++)pick[pk++]=pool[i];
 // capitais espalhadas
 u32 land=0;for(u32 i=0;i<n;i++)if(W_TERR[i]!=WT_WATER)land++;
 int dmin=Isqrt(land/(u32)pk)*6/10;if(dmin<3)dmin=3;
 int rad=Isqrt(land/((u32)pk*10));if(rad<2)rad=2;
 for(int k=0;k<pk;k++){
  int bx=-1,by=-1;
  for(int tries=0;tries<6000&&bx<0;tries++){
   int x=World_RngRange(w,w->w),y=World_RngRange(w,w->h);
   u32 i=World_Idx(w,x,y);
   if(W_TERR[i]==WT_WATER||W_TERR[i]==WT_ICE||W_OWNER[i])continue;
   int dm=dmin-(tries/1500);
   bool ok=true;
   for(u8 c=1;c<w->nC&&ok;c++){int ddx=w->c[c].capX-x,ddy=w->c[c].capY-y;if(ddx*ddx+ddy*ddy<dm*dm)ok=false;}
   if(ok){bx=x;by=y;}
  }
  if(bx<0)break;
  u8 ci=(u8)w->nC++;
  InitCountry(w,ci,pick[k],bx,by);
  if(k==0&&playerNat>=0)w->c[ci].player=true;
  Seed(w,ci,bx,by,rad);
 }
 Expand(w,false);
 Settle(w);
 World_Field(w);City_Gen(w);
 WEco_Init(w);WEco_PopInit(w);WDip_Init(w);WEco_Week(w);
 Hist_Init(w);
}

void World_GenBlitz(World *w,u32 seed,u8 diff,int playerNat){
 World_GenRandom(w,seed,WS_PEQUENO,8,diff,playerNat);
 w->mode=WM_BLITZ;
}

void World_GenMundi(World *w,u32 seed,u8 hist,u8 diff,int playerNat){
 Reset(w,seed,WM_MUNDI,EARTH_W,EARTH_H,diff);
 w->hist=hist;
 u8 row[EARTH_W];
 for(int y=0;y<EARTH_H;y++){
  Earth_Row(y,row);
  for(int x=0;x<EARTH_W;x++){
   u32 i=World_Idx(w,x,y);
   W_TERR[i]=row[x]?WT_PLAINS:WT_WATER;
   W_LIM[i]=(u8)Fractal(x,y,seed^0xA5A5u);
  }
 }
 Biomes(w,LatMundi);
 // paises de 1800 nas capitais reais + centros extras dos imperios
 for(int nat=0;nat<NATION_COUNT;nat++){
  const NationDef *d=&NATIONS[nat];
  if(d->yFrom)continue;
  int x,y;World_NatPos(w,nat,&x,&y);
  // capital na agua (ilha menor que a celula): procura terra perto ou cria
  if(W_TERR[World_Idx(w,x,y)]==WT_WATER){
   int fx=-1,fy=-1;
   for(int r=1;r<=3&&fx<0;r++)for(int dy=-r;dy<=r&&fx<0;dy++)for(int dx=-r;dx<=r;dx++){
    int nx=(x+dx+w->w)%w->w,ny=y+dy;
    if(ny<0||ny>=w->h)continue;
    u32 j=World_Idx(w,nx,ny);
    if(W_TERR[j]!=WT_WATER&&!W_OWNER[j]){fx=nx;fy=ny;break;}
   }
   if(fx<0)W_TERR[World_Idx(w,x,y)]=WT_PLAINS;else{x=fx;y=fy;}
  }
  if(W_OWNER[World_Idx(w,x,y)])continue; // duas capitais na mesma celula
  if(w->nC>W_MAXC)break;
  u8 ci=(u8)w->nC++;
  InitCountry(w,ci,nat,x,y);
  if(nat==playerNat)w->c[ci].player=true;
  Seed(w,ci,x,y,d->size);
 }
 for(int a=0;a<NAT_ANCHOR_COUNT;a++){
  const NatAnchor *an=&NAT_ANCHORS[a];
  u8 ci=World_CountryOfNat(w,Nat_Find(an->tag));
  if(!ci)continue;
  int x=Earth_X(an->lon),y=Earth_Y(an->lat);
  if(W_TERR[World_Idx(w,x,y)]==WT_WATER)continue;
  Seed(w,ci,x,y,an->r);
 }
 Expand(w,true);
 Settle(w);
 // suseranos
 for(u8 ci=1;ci<w->nC;ci++){
  const NationDef *d=&NATIONS[w->c[ci].nat];
  if(d->overlord)w->c[ci].overlord=World_CountryOfNat(w,Nat_Find(d->overlord));
 }
 // guerras em curso em 1/1/1800 (Segunda Coalizao; Nguyen x Tay Son)
 static const char *const WARS[][2]={{"FRA","GBR"},{"FRA","HAB"},{"FRA","OTT"},
  {"FRA","NAP"},{"FRA","POR"},{"NGU","TSN"}};
 for(unsigned k=0;k<sizeof(WARS)/sizeof(WARS[0]);k++)
  World_DeclareHistWar(w,World_CountryOfNat(w,Nat_Find(WARS[k][0])),World_CountryOfNat(w,Nat_Find(WARS[k][1])));
 w->nWars=0;
 World_Field(w);City_Gen(w);
 WEco_Init(w);WEco_PopInit(w);WDip_Init(w);WEco_Week(w);
 Hist_Init(w);
}

// ===================== guerra por frente =====================
static bool Winter(const World *w,int y){
 if(w->mode!=WM_MUNDI){int t=LatRandom(w,y);return t>45&&(w->date.m==12||w->date.m<=2);}
 int lat=EARTH_LAT_TOP-(y*EARTH_LAT_SPAN)/w->h;
 if(lat>=40)return w->date.m==12||w->date.m<=2;
 if(lat<=-40)return w->date.m>=6&&w->date.m<=8;
 return false;
}
static s32 CellCost(const World *w,u32 i,u8 att,u8 def){
 u8 t=W_TERR[i];
 s32 base=TER_COST[t];
 const WCountry *a=&w->c[att];
 s32 cost;
 if(!def){
  int col=a->m.colonizePct;if(col>80)col=80;
  cost=base*(100-col)/200;
 }else{
  const WCountry *d=&w->c[def];
  s32 dens=d->troops/Max32(1,(s32)d->cells);
  cost=base+dens*4;
  s32 dm=100+d->m.defPct;
  if(t==WT_FOREST||t==WT_SWAMP||t==WT_HILLS||t==WT_DESERT||t==WT_JUNGLE)dm+=d->m.defRoughPct;
  if(t==WT_HILLS||t==WT_MOUNTAIN)dm+=d->m.defMountPct;
  cost=cost*dm/100;
  if(d->capX==i%w->w&&d->capY==i/w->w)cost*=3;
  // cidade: ruas custam o dobro; muralha/quartel/palacio somam defesa
  if((W_CITYB[i>>3]>>(i&7))&1){
   int ci2=City_At(w,(int)(i%w->w),(int)(i/w->w));
   int def=100;
   if(ci2>=0)for(int s=0;s<CITY_SLOTS;s++)def+=BLD[w->city[ci2].bld[s]].def;
   cost=cost*2*def/100;
  }
 }
 // linha de suprimento: longe da capital custa mais (Mares/Colonial reduzem)
 int dx=Iabs((int)(i%w->w)-a->capX),dy=Iabs((int)(i/w->w)-a->capY);
 if(w->wrap&&dx>w->w/2)dx=w->w-dx;
 s32 sup=(s32)(dx+dy)*(100+a->m.supplyPct)/100;
 if(sup>300)sup=300;
 if(sup>0)cost=cost*(100+sup)/100;
 s32 am=100+a->m.atkPct+(a->player?0:DIFF_ATK[w->diff]);
 if(am<40)am=40;
 cost=cost*100/am;
 if(t==WT_PLAINS||t==WT_STEPPE||t==WT_DESERT)cost=cost*100/(100+a->m.plainsSpeedPct);
 return cost<2?2:cost;
}
bool World_Attack(World *w,u8 from,u8 to,u8 pct){
 if(!from||from>=w->nC||!w->c[from].alive||from==to)return false;
 if(to&&(to>=w->nC||!w->c[to].alive||!World_AtWar(w,from,to)))return false;
 WCountry *c=&w->c[from];
 if(c->atkOn&&c->atkTo!=to)return false; // 1 frente por vez
 if(pct>100)pct=100;
 s32 send=c->troops*pct/100;
 if(send<10)return false;
 c->troops-=send;c->atkTroops+=send;c->atkOn=true;c->atkTo=to;
 if(to&&c->player)Alert(w,to);   // so' as acoes do jogador: a historia da IA segue o ritmo dela
 return true;
}

// ---- acoes do jogador (tela do mapa-mundi, wgame.c) ----
// Recolhe a frente aberta: as tropas voltam para a reserva.
void World_Recall(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(!c->atkOn)return;
 c->troops+=c->atkTroops>0?c->atkTroops:0;c->atkTroops=0;c->atkOn=false;
}
// Recrutar: ouro vira regimentos. Custa metade do teto e da' 1/4 dele, ate
// 150% do teto (o excesso decai no World_Day). Unico gasto de ouro do mundo.
s32 World_RecruitCost(const World *w,u8 ci){
 // armas caras (falta no mercado) encarecem o recrutamento
 s32 p=w->eco[ci].price[GD_ARMAS],b=GD_BASE[GD_ARMAS];
 return (w->c[ci].cap/2+20)*(100+(p-b)*50/b)/100;
}
bool World_Recruit(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 s32 cost=World_RecruitCost(w,ci);
 if(!c->alive||c->gold<cost||c->troops>=c->cap*3/2)return false;
 c->gold-=cost;c->troops+=c->cap/4+5;
 return true;
}
// Proposta de paz de a para b: b aceita se nao esta muito mais forte, se a
// guerra ja cansou ou se ja perdeu terra. Vale ate em guerra historica: o
// jogador sempre pode mudar a historia (pilar 3).
bool World_ProposePeace(World *w,u8 a,u8 b){
 if(!World_AtWar(w,a,b))return false;
 const WCountry *A=&w->c[a],*B=&w->c[b];
 bool ok=(s64)B->troops*2<(s64)A->troops*3||B->warDays>720||(u32)B->cells*10<(u32)B->peak*9;
 if(ok)World_Peace(w,a,b);
 return ok;
}
// a tem fronteira terrestre com b? (b = 0: com terra sem dono). Varre a caixa
// do menor dos dois; chamado so' quando o jogador da' a ordem.
bool World_Borders(const World *w,u8 a,u8 b){
 const WCountry *s=(b&&w->c[b].cells<w->c[a].cells)?&w->c[b]:&w->c[a];
 u8 own=(s==&w->c[a])?a:b, oth=(own==a)?b:a;
 for(int y=s->by0;y<=s->by1&&y<w->h;y++)for(int x=s->bx0;x<=s->bx1&&x<w->w;x++){
  if(W_OWNER[World_Idx(w,x,y)]!=own)continue;
  for(int k=0;k<4;k++){
   int j=Nb(w,x,y,k);
   if(j>=0&&W_OWNER[j]==oth&&W_TERR[j]!=WT_WATER)return true;
  }
 }
 return false;
}
// Desembarque (os "barcos" do frontwars): sem fronteira terrestre, a frente
// comeca numa praia do alvo — a celula costeira dele mais perto da capital do
// atacante, ate 96 celulas por mar. Custa 1/10 da reserva. b = 0: terra livre.
bool World_Landing(World *w,u8 a,u8 b,s32 *cost){
 const WCountry *A=&w->c[a];
 int x0,x1,y0,y1;
 if(b){const WCountry *B=&w->c[b];x0=B->bx0;x1=B->bx1;y0=B->by0;y1=B->by1;}
 else{x0=A->capX-96;x1=A->capX+96;y0=A->capY-96;y1=A->capY+96;}
 if(y0<0)y0=0;
 if(y1>=w->h)y1=w->h-1;
 if(!w->wrap){if(x0<0)x0=0;if(x1>=w->w)x1=w->w-1;}
 int best=-1,bd=97;
 for(int y=y0;y<=y1;y++)for(int xx=x0;xx<=x1;xx++){
  int x=xx;if(w->wrap){x%=w->w;if(x<0)x+=w->w;}
  u32 i=World_Idx(w,x,y);
  if(W_TERR[i]==WT_WATER||W_OWNER[i]!=b)continue;
  int dx=x-(int)A->capX;if(dx<0)dx=-dx;
  if(w->wrap&&dx>w->w/2)dx=w->w-dx;
  int dy=y-(int)A->capY;if(dy<0)dy=-dy;
  if(dx+dy>=bd)continue;
  bool coast=false;
  for(int k=0;k<4&&!coast;k++){int j=Nb(w,x,y,k);if(j>=0&&W_TERR[j]==WT_WATER)coast=true;}
  if(coast){bd=dx+dy;best=(int)i;}
 }
 if(best<0)return false;
 SetOwner(w,(u32)best,a);w->nConquests++;
 // a frota escolta: sem navios custa o dobro; frota inimiga maior afunda transportes
 s32 k=w->c[a].troops/10;
 if(w->c[a].ships<=0)k*=2;
 if(b&&w->c[b].ships>w->c[a].ships)k+=w->c[a].troops/8;
 if(cost){*cost=k;}
 w->c[a].troops-=k;if(w->c[a].troops<0)w->c[a].troops=0;
 return true;
}
// Troca o pais do jogador (governo no exilio / "jogar como outro", DESIGN 2.1.7).
void World_SetPlayer(World *w,u8 ci){
 for(u8 i=1;i<w->nC;i++)w->c[i].player=false;
 if(ci&&ci<w->nC)w->c[ci].player=true;
}

HOT_ARM static void Fronts(World *w){
 // carimbo 1..255; no inicio de cada ciclo de 255 dias os carimbos sao
 // limpos, entao "W_AUX == stamp" significa "conquistada hoje"
 u8 stamp=(u8)(w->day%255u+1u);
 if(stamp==1){u32 n=(u32)w->w*w->h;for(u32 i=0;i<n;i++)W_AUX[i]=0;}
 // caixa de varredura: uniao das caixas de quem ataca (+1)
 int x0=w->w,y0=w->h,x1=-1,y1=-1;bool any=false;
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive||!c->atkOn)continue;
  // atrito do defensor (inverno / terreno dificil)
  if(c->atkTo){
   WCountry *d=&w->c[c->atkTo];
   if(d->m.winterAttrition&&Winter(w,d->capY))c->atkTroops-=c->atkTroops*d->m.winterAttrition/100;
   if(d->m.roughAttrition)c->atkTroops-=c->atkTroops*d->m.roughAttrition/100;
  }
  any=true;
  int ax0=c->bx0-1,ay0=c->by0-1,ax1=c->bx1+1,ay1=c->by1+1;
  if(ax0<x0)x0=ax0;
  if(ay0<y0)y0=ay0;
  if(ax1>x1)x1=ax1;
  if(ay1>y1)y1=ay1;
 }
 if(!any)return;
 if(x0<0)x0=0;
 if(y0<0)y0=0;
 if(x1>=w->w)x1=w->w-1;
 if(y1>=w->h)y1=w->h-1;
 if(w->wrap){x0=0;x1=w->w-1;}
 static u16 took[W_MAXC+1];
 for(u8 ci=0;ci<=W_MAXC&&ci<w->nC;ci++)took[ci]=0;
 // varre a partir de uma linha aleatoria, alternando o sentido por dia
 int hgt=y1-y0+1,off=World_RngRange(w,hgt);
 bool rev=(w->day&1);
 for(int k=0;k<hgt;k++){
  int y=y0+(off+k)%hgt;
  for(int kx=x0;kx<=x1;kx++){
   int x=rev?(x1-(kx-x0)):kx;
   u32 i=World_Idx(w,x,y);
   if(W_TERR[i]==WT_WATER)continue;
   u8 o=W_OWNER[i];
   if(W_AUX[i]==stamp)continue;      // conquistada hoje: nao vira base
   for(int n=0;n<4;n++){
    int j=Nb(w,x,y,n);
    if(j<0)continue;
    u8 q=W_OWNER[j];
    if(!q||q==o||W_AUX[j]==stamp)continue;
    WCountry *a=&w->c[q];
    if(!a->atkOn||a->atkTo!=o)continue;
    s32 cost=CellCost(w,i,q,o);
    if(a->atkTroops<cost)continue;
    a->atkTroops-=cost;
    if(o){
     WCountry *d=&w->c[o];
     d->troops-=cost/2;if(d->troops<0)d->troops=0;
     if(d->capX==x&&d->capY==y){
      if(!d->m.capitalNotFatal){d->troops-=d->troops/4;}
      s32 pr=d->prestige/10;d->prestige-=pr;a->prestige+=pr;
      d->capLost=true;
     }
    }
    SetOwner(w,i,q);W_AUX[i]=stamp;took[q]++;w->nConquests++;
    break;
   }
  }
 }
 // fecha frentes sem tropas ou sem fronteira; mata quem ficou sem celulas
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  if(c->warGoal&&c->atkOn&&c->atkTo==c->warTarget)c->warGain=(u16)(c->warGain+took[ci]);
  if(c->cells>c->peak)c->peak=(u16)(c->cells>65535?65535:c->cells);
  if(c->atkOn&&(c->atkTroops<8||!took[ci]||(!c->player&&AtFloor(w,c->atkTo)))){
   c->troops+=c->atkTroops>0?c->atkTroops:0;c->atkTroops=0;c->atkOn=false;
  }
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  if(!c->cells){Kill(w,ci);continue;}
  if(c->capLost)FindNewCapital(w,ci);
 }
}

// ===================== IA =====================
HOT_ARM static void AI(World *w,u8 ci,bool declare){
 WCountry *c=&w->c[ci];
 // paz quando a guerra vai mal
 // objetivo de guerra cumprido: a IA assina a paz e fica com o que tomou
 if(c->warGoal&&c->warGain>=c->warGoal&&World_AtWar(w,ci,c->warTarget)){
  World_Peace(w,ci,c->warTarget);c->warGoal=0;
 }
 bool mundi=w->mode==WM_MUNDI;
 int tiredDays=mundi?1500:720;
 if(declare&&c->warN&&(c->troops<c->cap/4||(c->warDays>tiredDays&&World_RngRange(w,100)<10)||(!mundi&&World_RngRange(w,100)<2))){
  for(u8 j=1;j<w->nC;j++)if(World_AtWar(w,ci,j)&&(!w->c[j].player||World_RngRange(w,3)==0)){
   if(HistWar(w,ci,j)>=0&&(c->warDays<3000||World_RngRange(w,100)!=0))continue;
   if(w->c[j].troops>w->c[j].cap/2&&c->troops<c->cap/4&&World_RngRange(w,2))continue;
   World_Peace(w,ci,j);
  }
 }
 if(c->atkOn)return;
 // alerta: alguem abriu frente contra nos -> contra-ataca ja' (sem esperar
 // o limiar do arquetipo), contra quem esta' atacando e faz fronteira
 if(c->alert&&c->warN&&c->troops*100>=c->cap*20){
  for(u8 j=1;j<w->nC;j++){
   const WCountry *e=&w->c[j];
   if(!e->alive||!World_AtWar(w,ci,j))continue;
   if(!(e->atkOn&&e->atkTo==ci)&&!WDip_Allied(w,ci,e->atkTo))continue;
   if(!World_Borders(w,ci,j))continue;
   World_Attack(w,ci,j,c->troops*100>=c->cap*60?60:40);
   return;
  }
 }
 if(c->troops*100<c->cap*ARCH_TH[c->archetype])return;
 // amostra a fronteira: quem sao os vizinhos?
 u8 cand[8];u8 cn=0;
 int bw=c->bx1-c->bx0+1,bh=c->by1-c->by0+1;
 for(int s=0;s<64&&cn<8;s++){
  int x=c->bx0+World_RngRange(w,bw),y=c->by0+World_RngRange(w,bh);
  if(W_OWNER[World_Idx(w,x,y)]!=ci)continue;
  for(int k=0;k<4&&cn<8;k++){
   int j=Nb(w,x,y,k);
   if(j<0||W_TERR[j]==WT_WATER)continue;
   u8 o=W_OWNER[j];
   if(o==ci)continue;
   bool dup=false;for(int q=0;q<cn;q++)if(cand[q]==o)dup=true;
   if(!dup)cand[cn++]=o;
  }
 }
 if(!cn)return;
 int best=-1,bs=-1000;
 s32 myD=c->troops/Max32(1,(s32)c->cells);
 for(int q=0;q<cn;q++){
  u8 o=cand[q];int s;
  if(!o)s=100;
  else{
   WCountry *d=&w->c[o];
   if(d->overlord==ci||c->overlord==o)continue;
   if(AtFloor(w,o))continue;
   if(c->m.confed&&d->m.confed)continue;
   s32 dD=d->troops/Max32(1,(s32)d->cells);
   if(World_AtWar(w,ci,o))s=80+(int)(myD-dD);
   else{
    bool bold=c->archetype==WA_EXPANSIONISTA||c->archetype==WA_OPORTUNISTA||c->archetype==WA_REVOLUCIONARIO;
    // por mil, por decisao (a cada 8 dias): mapa-mundi com historia e bem mais calmo
    int chance=w->mode!=WM_MUNDI?80:(w->hist==WH_LIGADA?3:10);
    // rival, ideologia oposta e temperamento mudam a conta (imprevisivel)
    if(o==c->rival)chance*=3;
    if(WDip_Allied(w,ci,o))continue;
    if((c->ideo==ID_COMUNISTA)!=(d->ideo==ID_COMUNISTA))chance*=2;
    chance=chance*(50+c->mood)/100;
    int maxWars=w->mode==WM_MUNDI?0:1;
    // guerra nova so' no turno de 8 dias (a IA reage mais vezes, mas nao declara mais)
    if(!declare||!bold||c->warN>maxWars||d->troops*10>c->troops*5||World_RngRange(w,1000)>=chance)continue;
    s=40+(int)(myD-dD);
   }
  }
  if(s>bs){bs=s;best=o;}
 }
 if(best<0)return;
 if(best&&!World_AtWar(w,ci,(u8)best)){
  World_DeclareWarCall(w,ci,(u8)best);
  c->warTarget=(u8)best;c->warGain=0;
  c->warGoal=(u16)(w->c[best].cells/3+4); // guerra limitada, nao aniquilacao
 }
 World_Attack(w,ci,(u8)best,ARCH_PCT[c->archetype]);
}

// ===================== dia =====================
// PROF_DAY (so' no host, host/prof_day.c): tempo por subsistema do dia.
#ifdef PROF_DAY
#include <time.h>
double PROF_T[8];
static double Pnow(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
#define PT(k,stmt) do{double _t=Pnow();stmt;PROF_T[k]+=Pnow()-_t;}while(0)
#else
#define PT(k,stmt) stmt
#endif
void World_Day(World *w){
 if(w->ended&&!w->eternal)return;
 for(int ph=0;ph<WD_PHASES;ph++)World_DayPhase(w,ph);
}
HOT_ARM static void DayBegin(World *w){
 w->day++;Cal_Next(&w->date);
 bool newYear=(w->date.m==1&&w->date.d==1);
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  Recap(c);
  if(c->warN){if(c->warDays<3000)c->warDays++;}
  else if(c->warDays)c->warDays=c->warDays>2?c->warDays-2:0;
  s32 tired=c->warDays/20;
  if(tired>50)tired=50;
  static const u8 MILR[3]={60,100,140};   // gasto militar (balanco fiscal) acelera o recrutamento
  s32 regen=(c->cap/60+1)*(100+c->m.regenPct)/100*(100-tired)/100*WEco_ArmsPct(w,ci)/100*MILR[w->eco[ci].mil%3]/100;
  if(c->troops<c->cap){s32 g=regen*(c->cap-c->troops)/Max32(1,c->cap);c->troops+=g>0?g:1;}
  else c->troops-=(c->troops-c->cap)/50+1;
  if(newYear)c->prestige+=c->m.prestigeYear+(s32)c->cells/200+(c->kind==NK_ESTADO?1:0);
  if(c->rndCd)c->rndCd--;
 }
 PT(0,City_Day(w));
}
HOT_ARM void World_DayPhase(World *w,int ph){
 switch(ph){
  case 0:if(w->ended&&!w->eternal)return;DayBegin(w);break;
  case 1:if((w->day%7)==0)PT(1,WEco_Week(w));break;
  case 2:
   PT(2,WDip_Day(w));
   // IA: cada pais pensa a cada 4 dias (escalonado); em alerta, todo dia
   PT(3,for(u8 ci=1;ci<w->nC;ci++){
    WCountry *c=&w->c[ci];
    if(!c->alive||c->player)continue;
    bool decl=((ci+w->day)&7)==0;
    if(c->alert){c->alert--;AI(w,ci,decl);}
    else if(((ci+w->day)&3)==0)AI(w,ci,decl);
   });
   break;
  default:
   PT(4,Fronts(w));
   if(w->mode==WM_MUNDI&&w->hist!=WH_DESLIGADA)PT(5,Hist_Day(w));
   PT(6,Rnd_Day(w));
   if(!w->eternal&&w->day>=w->endDay&&!w->ended){
    w->ended=true;
    World_Msg(w,"Fim da história","31 de dezembro de 2025. Continuar jogando?");
   }
   break;
 }
}
void World_Continue(World *w){w->eternal=true;w->ended=false;}

s32 World_Score(const World *w,u8 ci){
 const WCountry *c=&w->c[ci];
 if(!c->alive)return 0;
 return c->prestige+(s32)c->pop/10+c->gold/1000+c->troops/100;
}
// Campo do render (W_LIM), calculado 1x apos a geracao: relevo suavizado na
// terra (0..255) e distancia da costa na agua (1..31, chanfro 2 passadas).
// So' o px_world le; a simulacao nunca. Reusa W_AUX como rascunho.
static const u8 ELEV[WT_COUNT]={0,40,70,120,230,60,20,70,90,45,110};
void World_Field(World *w){
 const int W=w->w,H=w->h;u32 n=(u32)W*H;
 for(u32 i=0;i<n;i++)W_LIM[i]=W_TERR[i]==WT_WATER?31:ELEV[W_TERR[i]];
 // relevo: 2 passadas de media 3x3 so' entre celulas de terra
 for(int pass=0;pass<2;pass++){
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){
   u32 i=World_Idx(w,x,y);
   if(W_TERR[i]==WT_WATER){W_AUX[i]=0;continue;}
   int s=0,k=0;
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
    int nx=x+dx,ny=y+dy;
    if(ny<0||ny>=H)continue;
    if(nx<0||nx>=W){if(!w->wrap)continue;nx=(nx+W)%W;}
    u32 j=World_Idx(w,nx,ny);
    if(W_TERR[j]==WT_WATER)continue;
    s+=W_LIM[j];k++;
   }
   W_AUX[i]=(u8)(s/k);
  }
  for(u32 i=0;i<n;i++)if(W_TERR[i]!=WT_WATER)W_LIM[i]=W_AUX[i];
 }
 // profundidade: distancia de chanfro ate a terra, teto 31
 for(u32 i=0;i<n;i++)if(W_TERR[i]==WT_WATER)W_LIM[i]=31;
 for(int y=0;y<H;y++)for(int x=0;x<W;x++){
  u32 i=World_Idx(w,x,y);if(W_TERR[i]!=WT_WATER)continue;
  int m=W_LIM[i];
  if(x>0){u32 j=i-1;int v=W_TERR[j]==WT_WATER?W_LIM[j]+1:1;if(v<m)m=v;}
  if(y>0){u32 j=i-W;int v=W_TERR[j]==WT_WATER?W_LIM[j]+1:1;if(v<m)m=v;}
  W_LIM[i]=(u8)m;
 }
 for(int y=H-1;y>=0;y--)for(int x=W-1;x>=0;x--){
  u32 i=World_Idx(w,x,y);if(W_TERR[i]!=WT_WATER)continue;
  int m=W_LIM[i];
  if(x<W-1){u32 j=i+1;int v=W_TERR[j]==WT_WATER?W_LIM[j]+1:1;if(v<m)m=v;}
  if(y<H-1){u32 j=i+W;int v=W_TERR[j]==WT_WATER?W_LIM[j]+1:1;if(v<m)m=v;}
  W_LIM[i]=(u8)m;
 }
 for(u32 i=0;i<n;i++)W_AUX[i]=0;
 for(u32 i=0;i<sizeof w->dirtyBlk;i++)w->dirtyBlk[i]=0;
}
u32 World_Hash(const World *w){
 u32 h=2166136261u;
 u32 n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++){h^=W_OWNER[i];h*=16777619u;}
 for(u8 ci=1;ci<w->nC;ci++){h^=(u32)w->c[ci].troops;h*=16777619u;h^=(u32)w->c[ci].gold;h*=16777619u;
  h^=(u32)w->c[ci].ideo|((u32)w->c[ci].bloc<<8)|((u32)w->c[ci].ally[0]<<16)|((u32)(u16)w->c[ci].ships<<24);h*=16777619u;}
 for(int i=0;i<w->nCity;i++){const WCity *c=&w->city[i];h^=(u32)c->pop|((u32)c->tier<<16)|((u32)c->owner<<20);h*=16777619u;
  for(int s=0;s<CITY_SLOTS;s++){h^=c->bld[s];h*=16777619u;}}
 h^=w->rng;
 return h;
}
