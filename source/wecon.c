#include "wecon.h"
#include "hot.h"
#include "city.h"
#include "str.h"
#include "wdiplo.h"
// wecon.c — economia do mundo (ver wecon.h; DESIGN 4.8; SDD ADR-26).
// Custo: 1x por semana, O(cidades) + O(paises x bens) + rotas O(portos x 3 x
// paises). Buffers estaticos (.bss, ~27KB), zero malloc, sem float, sem RNG.

//                              graos madeira fibra minerio luxo texteis ferram. armas petroleo
const u16 GD_BASE[GD_COUNT]={  10,   12,     14,   16,     30,  26,     34,     44,   40};
static const u16 GD_YEAR[GD_COUNT]={0,0,0,0,0,0,0,0,1860};
u16 Gd_Str(u8 g){return (u16)(STR_GD_GRAOS+(g<GD_COUNT?g:0));}
u16 Gd_Year(u8 g){return g<GD_COUNT?GD_YEAR[g]:0;}
static inline bool Manuf(int g){return g>=GD_TEXTEIS&&g<=GD_ARMAS;}
static inline s32 Mn(s32 a,s32 b){return a<b?a:b;}
static inline s32 Mx(s32 a,s32 b){return a>b?a:b;}

// ---------------- cidade: producao, insumo e consumo ----------------
void WEco_CityFlows(const World *w,int i,s32 prod[GD_COUNT],s32 need[GD_COUNT],s32 dem[GD_COUNT]){
 for(int g=0;g<GD_COUNT;g++)prod[g]=need[g]=dem[g]=0;
 const WCity *c=&w->city[i];
 int y=w->date.y,pop=c->pop;
 // consumo da populacao (milhares): comer, vestir, luxo; a cidade grande
 // da era industrial pede ferramentas e, no seculo XX, petroleo
 dem[GD_GRAOS]+=pop/10+1;
 dem[GD_TEXTEIS]+=pop/80;
 dem[GD_MADEIRA]+=pop/60;                       // lenha e construcao
 if(c->tier>=CT_VILAREJO)dem[GD_LUXO]+=pop/150;
 if(c->tier>=CT_CIDADE&&y>=1830){dem[GD_FERRAMENTAS]+=pop/80;dem[GD_MINERIO]+=pop/120;} // carvao das casas
 if(c->tier>=CT_CIDADE&&y>=1900)dem[GD_PETROLEO]+=pop/200;
 // subsistencia: a lavoura em volta alimenta a vila mesmo sem obra
 prod[GD_GRAOS]+=c->agri/12;
 int ind=y<1880?100:(y<1950?200:300);   // produtividade industrial por era
 int agr=100+(y-1800)*3/5;              // e a do campo
 bool rail=false;int usina=0,viz=0;
 for(int s=0;s<CITY_SLOTS;s++){if(c->bld[s]==BK_USINA&&y>=1890)usina=25;viz+=City_IndustryBonus(c,s);}
 int man=100+usina+(viz>25?25:viz);       // usina e obras coladas na ferrovia/porto
 for(int s=0;s<CITY_SLOTS;s++)switch(c->bld[s]){
  case BK_FAZENDA:prod[GD_GRAOS]+=(c->agri/6+4)*agr/100;
   if(c->res&RS_TROPICAL)prod[GD_LUXO]+=(4+c->agri/10)*agr/100;    // plantacao: cafe, acucar, cha
   else prod[GD_FIBRA]+=(2+c->agri/16)*agr/100;                     // la e algodao
   break;
  case BK_SERRARIA:prod[GD_MADEIRA]+=(c->wood/4+5)*agr/100;break;
  case BK_MINA:prod[GD_MINERIO]+=(c->ore/3+8)*ind/100;break;
  case BK_PORTO:prod[GD_GRAOS]+=4;break;                   // pesca
  case BK_OFICINA:need[GD_MADEIRA]+=3;need[GD_MINERIO]+=3;need[GD_FIBRA]+=4;
   prod[GD_FERRAMENTAS]+=5*ind/100;prod[GD_TEXTEIS]+=6*ind/100;break;
  case BK_FABRICA:need[GD_FIBRA]+=10;need[GD_MINERIO]+=8;need[GD_MADEIRA]+=2;
   prod[GD_TEXTEIS]+=20*ind/100;prod[GD_FERRAMENTAS]+=14*ind/100;break;
  case BK_ARSENAL:need[GD_MINERIO]+=4;need[GD_FERRAMENTAS]+=3;prod[GD_ARMAS]+=10*ind/100;break;
  case BK_POCO:if(y>=1860)prod[GD_PETROLEO]+=(10+((c->res&RS_OLEO)?20:0))*ind/100;break;
  case BK_FERROVIA:rail=true;break;
  default:break;
 }
 if(man>100){prod[GD_TEXTEIS]=prod[GD_TEXTEIS]*man/100;prod[GD_FERRAMENTAS]=prod[GD_FERRAMENTAS]*man/100;prod[GD_ARMAS]=prod[GD_ARMAS]*man/100;}
 if(rail)for(int g=0;g<GD_COUNT;g++)prod[g]+=prod[g]/5;
}

// ---------------- marinha ----------------
s32 WEco_ShipCost(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];
 u8 g=w->date.y>=1860?GD_MINERIO:GD_MADEIRA;   // vela de madeira -> couracado de ferro
 return 60+(s32)e->price[g]*3+(w->date.y-1800)/4;
}
int WEco_ShipDays(const World *w,u8 ci){
 int y=w->eco[ci].yards;if(y<1)y=1;
 int d=90/y;return d<15?15:d;
}
bool WEco_BuildShip(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 s32 cost=WEco_ShipCost(w,ci);
 if(!c->alive||!w->eco[ci].yards||c->shipQ||c->gold<cost)return false;
 c->gold-=cost;c->shipQ=(u16)WEco_ShipDays(w,ci);
 return true;
}
bool WEco_Blockaded(const World *w,u8 ci){
 const WCountry *c=&w->c[ci];
 if(!c->warN)return false;
 for(u8 j=1;j<w->nC;j++)
  if(w->c[j].alive&&w->c[j].ships>c->ships*3/2+2&&World_AtWar(w,ci,j))return true;
 return false;
}

s32 WEco_Revenue(const World *w,u8 ci){const WEco *e=&w->eco[ci];return e->rProd+e->rPop+e->rCity+e->rLand+e->rTariff;}
s32 WEco_Expense(const World *w,u8 ci){const WEco *e=&w->eco[ci];return e->xArmy+e->xNavy+e->xAdmin+e->xEdu+e->xUpkeep+e->xDebt+e->xCorrupt;}
s32 WEco_GoldWeek(const World *w,u8 ci){return WEco_Revenue(w,ci)-WEco_Expense(w,ci);}
// limite de credito: meio ano de receita (minimo 300); cai a cada calote
s32 WEco_DebtLimit(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];
 s32 r=WEco_Revenue(w,ci);if(r<12)r=12;
 return r*26/(1+e->defaults);
}

// ---------------- populacao e alfabetizacao de 1800 ----------------
// Estimativas historicas (milhares de habitantes; % alfabetizado). Fontes de
// ordem de grandeza: Maddison (populacao) e Buringh/van Zanden e
// Our World in Data (alfabetizacao). Quem nao esta' na tabela usa regra por
// continente e tipo de entidade.
typedef struct { const char *tag; u32 popK; u8 lit; } PopRow;
static const PopRow POP1800[]={
 {"GBR",10900,55},{"IRL",5000,30},{"FRA",29000,37},{"ESP",11000,20},{"POR",3000,18},
 {"NED",2100,60},{"HEL",1700,60},{"PRU",8700,55},{"HAB",23000,30},{"BAV",3300,45},
 {"SAX",2000,62},{"WUR",1400,55},{"HAN",1000,50},{"SWE",2400,75},{"DAN",1800,70},
 {"RUS",37000,8},{"OTT",24000,5},{"SAR",3800,22},{"NAP",5000,14},{"PAP",2500,20},
 {"MNE",120,5},{"GEO",700,5},{"EGY",4000,4},{"MOR",3000,4},{"ALG",2500,4},{"TUN",1000,4},
 {"TRP",500,3},{"USA",5300,60},{"HAI",550,2},{"CUB",270,10},{"NSP",6000,10},
 {"NGR",2000,8},{"VPE",1500,8},{"RDP",1300,10},{"BRA",3300,6},{"BNA",500,40},
 {"QAJ",6000,5},{"AFG",5000,3},{"BUK",2500,4},{"KHI",800,3},{"KOK",1000,3},{"KAZ",1500,2},
 {"SIK",3500,5},{"MAR",30000,4},{"MUG",1500,5},{"EIC",45000,4},{"HYD",10000,4},
 {"NEP",4000,3},{"QNG",320000,20},{"JAP",30000,40},{"KOR",14000,25},{"NGU",4000,12},
 {"TSN",3000,10},{"SIA",4000,15},{"BRM",4000,15},{"DEI",11000,5},{"PHI",2000,10},
 {"ETH",3000,5},{"ASH",3000,3},{"OYO",4000,2},{"DAH",300,2},{"KON",1500,2},{"MER",1500,4},
 {"OMA",500,4},{"YEM",2500,4},{"DRY",1500,6},{"CAP",60,30},{"HAW",250,5},{"NSW",5,40},
};
static void PopDefault(const WCountry *c,u32 *p,u8 *l){
 const NationDef *d=&NATIONS[c->nat];
 static const u8 DENS[CONT_COUNT]={24,3,6,14,1};   // milhares por unidade de c->pop
 static const u8 LIT[CONT_COUNT]={20,12,3,8,2};
 u32 k=DENS[d->cont];if(d->kind==NK_POVO||d->kind==NK_AUTONOMO)k=1;
 *p=c->pop*k/10+20;*l=d->kind==NK_POVO?2:LIT[d->cont];
}
void WEco_PopInit(World *w){
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  u32 p=0;u8 l=0;bool hit=false;
  const char *t=NATIONS[c->nat].tag;
  for(unsigned k=0;k<sizeof POP1800/sizeof POP1800[0];k++){const char *q=POP1800[k].tag;
   if(q[0]==t[0]&&q[1]==t[1]&&q[2]==t[2]){p=POP1800[k].popK;l=POP1800[k].lit;hit=true;break;}}
  if(!hit)PopDefault(c,&p,&l);
  c->popK=p;c->lit=(u16)(l*10);
 }
}
// crescimento anual em por mil: transicao demografica (sobe com a era, cai
// com a alfabetizacao alta depois de 1950), fome e guerra puxam para baixo
static int GrowthPermil(const World *w,const WCountry *c,const WEco *e){
 int y=w->date.y;
 int base=y<1850?5:(y<1900?7:(y<1950?10:(y<1990?19:12)));
 if(y>=1950&&c->lit>850)base-=(c->lit-850)/30;       // pais rico e letrado cresce menos
 base+=(e->life-60)/6;
 if(e->sat[GD_GRAOS]<60)base-=(60-e->sat[GD_GRAOS])/3; // fome
 if(c->warN)base-=4;
 if(c->unrestT)base-=3;
 return base<-30?-30:(base>35?35:base);
}
// alfabetizacao (decimos de %): teto possivel na era; a escola (gasto em
// educacao + universidades) puxa em direcao ao teto, e a difusao do mundo
// garante um piso que sobe com o tempo. Quase nunca cai.
static int LitCap(int y){return y<1850?350:(y<1900?350+(y-1850)*6:(y<1950?650+(y-1900)*5:(y<2000?900+(y-1950)*9/5:990)));}
static int LitFloor(int y){return y<1850?10:(y<1900?10+(y-1850)*2/5:(y<1950?30+(y-1900)*2:(y<2000?130+(y-1950)*8:530+(y-2000)*8)));}
static void LitWeek(const World *w,WCountry *c,const WEco *e,int unis){
 int y=w->date.y,l=c->lit,cap=LitCap(y),fl=LitFloor(y);
 static const u8 PERMIL[3]={0,6,15};           // fracao (por mil) do vao ate' o teto fechada por ano
 int r=PERMIL[e->edu%3]+unis*3;if(c->m.prestigeYear>=2)r+=5;
 // ganho do ano em decimos; espalhado nas 52 semanas sem estado (o resto
 // cai nas primeiras semanas do ano): deterministico, nada fora do World
 int gy=(cap>l?(cap-l):0)*r/1000,wk=(int)((w->day/7)%52);
 l+=gy/52+(wk<gy%52?1:0);
 if(l<fl)l+=(fl-l+49)/50;                       // difusao: o piso alcanca em ~1 ano
 c->lit=(u16)(l>1000?1000:l);
}
int WEco_ArmsPct(const World *w,u8 ci){return 50+w->eco[ci].sat[GD_ARMAS]/2;}
int WEco_ToolsPct(const World *w,u8 ci){
 if(w->date.y<1830)return 100;
 return 50+w->eco[ci].sat[GD_FERRAMENTAS]/2;
}

void WEco_Init(World *w){
 for(int g=0;g<GD_COUNT;g++){w->wprice[g]=GD_BASE[g];w->wvol[g]=0;}
 for(u8 ci=0;ci<=W_MAXC;ci++){
  WEco *e=&w->eco[ci];
  u8 *z=(u8*)e;for(unsigned k=0;k<sizeof *e;k++)z[k]=0;
  e->tax=1;e->life=70;e->ind=100;e->hub=-1;e->tariff=1;e->edu=1;e->mil=1;e->rate=4;
  for(int g=0;g<GD_COUNT;g++){e->price[g]=GD_BASE[g];e->sat[g]=100;}
 }
}

// ---------------- semana ----------------
static s32 NEED[W_MAXC+1][GD_COUNT];      // insumo pedido pela industria do pais
static s32 RUR[W_MAXC+1][GD_COUNT];       // parte artesanal (rural) da manufatura: nao depende de insumo
static s32 MS[W_MAXC+1][GD_COUNT],MD[W_MAXC+1][GD_COUNT],MN[W_MAXC+1][GD_COUNT]; // mercado (bloco)
static s32 MCAP[W_MAXC+1];
static s32 EXP[W_MAXC+1][GD_COUNT],IMP[W_MAXC+1][GD_COUNT];
static u8 MIND[W_MAXC+1],MLACK[W_MAXC+1];
static s32 POS[W_MAXC+1][GD_COUNT],NEG[W_MAXC+1][GD_COUNT];

static u8 MarketOf(const World *w,u8 ci){
 u8 b=w->c[ci].bloc;
 return (b&&b<w->nC&&w->c[b].alive&&w->c[b].bloc==b)?b:ci;
}
static bool Tropical(const World *w,const WCountry *c){
 int lat=NATIONS[c->nat].lat;
 return (lat<=25&&lat>=-25)||c->terr[WT_JUNGLE]*6>(int)c->cells;
 (void)w;
}
// preco a partir da razao demanda/oferta (Q8, entre 1/4 e 4x o base)
static u16 PriceOf(int g,s32 d,s32 s){
 s32 r=(d+8)*256/(s+8);
 if(r<64)r=64;
 if(r>1024)r=1024;
 return (u16)(GD_BASE[g]*r/256);
}

HOT_ARM void WEco_Week(World *w){
 const int Y=w->date.y;
 // 1) campo + necessidades rurais, por pais
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
  for(int g=0;g<GD_COUNT;g++){e->sup[g]=e->dem[g]=0;NEED[ci][g]=0;}
  e->ports=e->yards=0;e->hub=-1;
  if(!c->alive)continue;
  const u16 *T=c->terr;
  // campo: a populacao rural (c->pop) planta, corta lenha, fia e forja;
  // o que sobra disso vira oferta, o resto ela mesma consome
  s32 rp=(s32)c->pop;
  // produtividade do campo sobe com a era (adubo, maquinas): 1800 = 100%, 2025 = 190%
  s32 ag=100+(Y-1800)*3/5;
  e->sup[GD_GRAOS]=(s32)(T[WT_PLAINS]*3+T[WT_STEPPE]*2+T[WT_FOREST]+T[WT_JUNGLE]+T[WT_HILLS]+T[WT_SWAMP])*3/2*ag/100;
  e->sup[GD_MADEIRA]=(s32)(T[WT_FOREST]*2+T[WT_JUNGLE]*2+T[WT_TUNDRA]+T[WT_SWAMP])/4*ag/100;
  e->sup[GD_FIBRA]=(s32)(T[WT_STEPPE]*2+T[WT_PLAINS]+T[WT_DESERT]/2)/8*ag/100;
  e->sup[GD_MINERIO]=(s32)(T[WT_HILLS]+T[WT_MOUNTAIN]*2)/4*ag/100;
  if(Tropical(w,c))e->sup[GD_LUXO]=(s32)(T[WT_JUNGLE]*2+T[WT_PLAINS])/6*ag/100;
  if(Y<1900)e->sup[GD_TEXTEIS]=rp/300;            // tear domestico
  e->sup[GD_ARMAS]=rp/600;                         // ferreiros e armeiros
  e->dem[GD_GRAOS]=rp/4;
  e->dem[GD_MADEIRA]=rp/40;
  e->dem[GD_FIBRA]=rp/400;
  e->dem[GD_TEXTEIS]=rp/150;
  e->dem[GD_LUXO]=rp/(Y<1870?1000:600);
  e->dem[GD_ARMAS]=(c->troops+c->atkTroops)/600+c->ships/4;
  if(Y>=1900)e->dem[GD_PETROLEO]=rp/1500+c->ships/4;
  for(int g=0;g<GD_COUNT;g++)RUR[ci][g]=Manuf(g)?e->sup[g]:0;
 }
 // 2) cidades (mercado local: a vila come primeiro do proprio entorno)
 static s32 cg[W_MAXCITY];
 for(int i=0;i<w->nCity;i++){
  WCity *ct=&w->city[i];u8 o=ct->owner;cg[i]=0;
  if(!o||!w->c[o].alive)continue;
  WEco *e=&w->eco[o];
  s32 p[GD_COUNT],n[GD_COUNT],d[GD_COUNT];
  WEco_CityFlows(w,i,p,n,d);
  for(int g=0;g<GD_COUNT;g++){e->sup[g]+=p[g];e->dem[g]+=d[g];NEED[o][g]+=n[g];}
  cg[i]=d[GD_GRAOS]?Mn(100,p[GD_GRAOS]*100/d[GD_GRAOS]):100;
  bool port=false;
  for(int s=0;s<CITY_SLOTS;s++){if(ct->bld[s]==BK_PORTO)port=true;if(ct->bld[s]==BK_ESTALEIRO)e->yards++;}
  if(port){e->ports++;if(e->hub<0||w->city[e->hub].pop<ct->pop)e->hub=(s16)i;}
 }
 // 3) capacidade de comercio externo e mercado (bloco)
 for(u8 m=0;m<w->nC;m++){MCAP[m]=0;for(int g=0;g<GD_COUNT;g++){MS[m][g]=MD[m][g]=MN[m][g]=0;POS[m][g]=NEG[m][g]=0;}}
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
  if(!c->alive)continue;
  s32 sea=e->ports*30+e->yards*10+Mn(c->ships,20)*3;
  if(WEco_Blockaded(w,ci))sea/=3;
  s32 cap=6+(s32)c->cells/50+sea;
  static const u8 TARC[3]={120,100,80};    // livre comercio abre o porto; protecionismo fecha
  cap=cap*(100+c->m.tradePct)/100*TARC[e->tariff%3]/100;
  e->cap=(u16)Mn(cap,60000);
  u8 m=MarketOf(w,ci);MCAP[m]+=e->cap;
  for(int g=0;g<GD_COUNT;g++){
   if(!Manuf(g))MS[m][g]+=e->sup[g];
   MD[m][g]+=e->dem[g];MN[m][g]+=NEED[ci][g];
  }
 }
 // 4) industria do mercado: roda a ind% do insumo que mais falta
 for(u8 m=1;m<w->nC;m++){
  MIND[m]=100;MLACK[m]=0;
  for(int g=0;g<GD_COUNT;g++){
   if(!MN[m][g])continue;
   s32 av=Mx(0,MS[m][g]-MD[m][g]/2)+MCAP[m]/4;
   s32 pct=av*100/MN[m][g];
   if(pct<MIND[m]){MIND[m]=(u8)Mx(0,pct);MLACK[m]=(u8)g;}
  }
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
  if(!c->alive)continue;
  u8 m=MarketOf(w,ci);
  int ef=MIND[m];
  int bonus=100+c->m.prodPct+c->lit/40;if(c->unrestT)bonus=bonus*70/100;   // letrados produzem mais (ate' +25%)
  e->ind=(u8)ef;e->lack=MLACK[m];
  for(int g=0;g<GD_COUNT;g++){
   if(Manuf(g)){s32 r=RUR[ci][g];e->sup[g]=r+(e->sup[g]-r)*ef/100*bonus/100;MS[m][g]+=e->sup[g];}
   s32 used=NEED[ci][g]*ef/100;
   e->dem[g]+=used;MD[m][g]+=used;
   if(Y<Gd_Year((u8)g)){e->sup[g]=e->dem[g]=0;}
  }
 }
 // 5) mercado mundial: o que sobra/falta no bloco cruza o mar ate a capacidade
 s32 E[GD_COUNT],I[GD_COUNT];
 for(int g=0;g<GD_COUNT;g++){E[g]=I[g]=0;}
 for(u8 m=1;m<w->nC;m++){
  if(!MCAP[m])continue;
  s32 tot=0;
  for(int g=0;g<GD_COUNT;g++){s32 d=MS[m][g]-MD[m][g];tot+=d<0?-d:d;}
  for(int g=0;g<GD_COUNT;g++){
   s32 d=MS[m][g]-MD[m][g];
   s32 v=d<0?-d:d;
   if(tot>MCAP[m])v=(s32)((s64)v*MCAP[m]/tot);
   EXP[m][g]=d>0?v:0;IMP[m][g]=d<0?v:0;
   E[g]+=EXP[m][g];I[g]+=IMP[m][g];
  }
 }
 for(int g=0;g<GD_COUNT;g++){
  s32 r=(I[g]+30)*256/(E[g]+30);
  if(r<102)r=102;
  if(r>768)r=768;
  u16 np=(u16)(GD_BASE[g]*r/256);
  w->wprice[g]=(u16)((w->wprice[g]*3+np)/4);
  w->wvol[g]=Mn(E[g],I[g]);
 }
 // 6) fecha cada mercado: oferta final, satisfacao e preco
 for(u8 m=1;m<w->nC;m++){
  if(!MCAP[m])continue;
  for(int g=0;g<GD_COUNT;g++){
   s32 ex=E[g]?(s32)((s64)EXP[m][g]*w->wvol[g]/E[g]):0;
   s32 im=I[g]?(s32)((s64)IMP[m][g]*w->wvol[g]/I[g]):0;
   EXP[m][g]=ex;IMP[m][g]=im;
   MS[m][g]+=im-ex;
  }
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WEco *e=&w->eco[ci];
  if(!w->c[ci].alive)continue;
  u8 m=MarketOf(w,ci);
  for(int g=0;g<GD_COUNT;g++){s32 b=e->sup[g]-e->dem[g];if(b>0)POS[m][g]+=b;else NEG[m][g]-=b;}
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
  if(!c->alive)continue;
  u8 m=MarketOf(w,ci);
  s32 val=0,trade=0;e->used=0;
  for(int g=0;g<GD_COUNT;g++){
   s32 S=MS[m][g],D=MD[m][g];
   e->sat[g]=(u8)(D>0?Mn(100,Mx(0,S)*100/D):100);
   u16 pl=PriceOf(g,D,S);
   e->price[g]=(u16)((IMP[m][g]||EXP[m][g])?(pl+2*w->wprice[g])/3:pl);
   if(Y<Gd_Year((u8)g)){e->sat[g]=100;e->price[g]=GD_BASE[g];}
   // vende a sobra e compra a falta no preco do mercado: o que o bloco
   // nao absorve nem exporta encalha (e derruba o preco)
   s32 b=e->sup[g]-e->dem[g];
   s32 P=POS[m][g],N=NEG[m][g];
   s32 sold=Mn(P,N+EXP[m][g]),bought=Mn(N,P+IMP[m][g]);
   s32 q=0;
   if(b>0&&P)q=(s32)((s64)b*sold/P);
   else if(b<0&&N)q=-(s32)((s64)(-b)*bought/N);
   e->net[g]=(s16)Mx(-32000,Mn(32000,-q));
   // alfandega: o Estado cobra sobre o que entra e sai do pais (dentro do
   // bloco a taxa e' menor: e' o atrativo do mercado comum)
   s32 aq=q<0?-q:q;
   trade+=aq*e->price[g]*(m!=ci||MCAP[m]==0?2:4);
   val+=e->sup[g]*e->price[g];
  }
  // uso dos portos: a parte deste pais no comercio externo do mercado
  {s32 ext=0;for(int g=0;g<GD_COUNT;g++)ext+=EXP[m][g]+IMP[m][g];
   e->used=(u16)Mn(e->cap,MCAP[m]?(s32)((s64)ext*e->cap/MCAP[m]):0);}
  // ---- balanco fiscal (ADR-32) ----
  static const u8 RATE[3]={1,2,3};
  static const u8 TARR[3]={1,2,3};          // alfandega por nivel de tarifa
  static const u8 MILX[3]={60,100,150};     // soldo por nivel de gasto militar
  static const u8 EDUX[3]={0,3,8};          // por mil habitantes x mil por semana
  e->gdp=val/100;
  e->rProd=val/1000*RATE[e->tax%3]+val%1000*RATE[e->tax%3]/1000;
  e->rCity=c->cityGold*7/2*(100+c->m.goldPct)/100;               // taxas urbanas (City_Day soma por dia)
  e->rLand=((s32)c->cells/48+1)*7*(100+c->m.goldPct)/100;         // renda da terra
  // imposto per capita: quem tem mais gente (e mais letrada) arrecada mais
  e->rPop=(s32)(c->popK/600)*RATE[e->tax%3]*(20+c->lit/10)/40;
  e->rTariff=trade/200*TARR[e->tariff%3];
  int unis=0,obras=0,banks=0;
  for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci)for(int k=0;k<CITY_SLOTS;k++){u8 b=w->city[i].bld[k];
   if(b){obras++;if(b==BK_UNIVERSIDADE)unis+=2;if(b==BK_ESCOLA)unis++;if(b==BK_BANCO)banks++;}}
  e->xArmy=(c->troops+c->atkTroops)/60*MILX[e->mil%3]/100;
  e->xNavy=c->ships;
  e->xAdmin=(s32)c->cells/60+c->nCity+(s32)(c->popK/2000);          // maquina do Estado
  e->xCorrupt=c->gold>2000?(c->gold-2000)/100:0;                     // cofre parado: desvio e corrupcao
  e->xEdu=(s32)((u64)c->popK*EDUX[e->edu%3]/1000)/(Y<1870?3:1);
  e->xUpkeep=obras/2;
  e->xDebt=(s32)((s64)e->debt*e->rate/5200);
  e->inc=e->rProd+e->rPop+e->rCity+e->rLand;e->trade=e->rTariff;e->exp=WEco_Expense(w,ci);
  s32 g=c->gold+e->inc+e->trade-e->exp;
  // deficit: toma emprestado ate' o limite; acima dele, calote
  if(g<0){
   s32 lim=WEco_DebtLimit(w,ci);
   if(e->debt-g<=lim){e->debt-=g;g=0;}
   else{                                               // calote
    e->debt/=3;g=0;e->defaults=(u8)(e->defaults<5?e->defaults+1:5);
    c->prestige-=c->prestige/10+5;
    if(c->happy>15)c->happy-=15;
    if(c->troops>c->cap/3)c->troops-=c->troops/8;   // soldo atrasado: deserção
   }
  }else if(e->debt>0){s32 pay=g/4;if(pay>e->debt)pay=e->debt;e->debt-=pay;g-=pay;}
  c->gold=g;
  // juros: base + risco (divida / limite) + calotes passados
  {s32 lim=WEco_DebtLimit(w,ci);int risk=lim?(int)((s64)e->debt*8/lim):8;
   int r=3+risk+e->defaults*2-(banks>4?2:banks/2);e->rate=(u8)(r<1?1:(r>20?20:r));}
  // populacao e alfabetizacao andam 1x por semana
  {int gr=GrowthPermil(w,c,e);
   s64 d=(s64)c->popK*gr/52000;if(!d&&gr>0&&c->popK>500)d=1;
   s64 np=(s64)c->popK+d;c->popK=(u32)(np<10?10:np);
   LitWeek(w,c,e,unis);}
  // nivel de vida: comer pesa mais; o resto depende da era
  int lw=e->sat[GD_GRAOS]*40+e->sat[GD_TEXTEIS]*20+e->sat[GD_LUXO]*15;
  int tw=75;
  if(Y>=1830){lw+=e->sat[GD_FERRAMENTAS]*15;tw+=15;}
  if(Y>=1900){lw+=e->sat[GD_PETROLEO]*10;tw+=10;}
  e->life=(u8)(lw/tw);
 }
 // 7) comida local das cidades: o entorno primeiro, o mercado completa
 for(int i=0;i<w->nCity;i++){
  WCity *ct=&w->city[i];u8 o=ct->owner;
  if(!o||!w->c[o].alive)continue;
  int loc=(int)cg[i];
  ct->food=(u8)Mn(100,loc+(100-loc)*w->eco[o].sat[GD_GRAOS]/100);
 }
 // 8) rotas maritimas (mapa/painel): os 3 bens que o pais mais exporta e
 // quem mais importa cada um entre os paises com porto de outro mercado
 for(u8 ci=1;ci<w->nC;ci++){
  WEco *e=&w->eco[ci];
  for(int k=0;k<3;k++){e->route[k]=0;e->routeG[k]=0;e->routeV[k]=0;}
  if(!w->c[ci].alive||!e->ports)continue;
  u8 top[3]={0,0,0};s32 tv[3]={0,0,0};
  for(int g=0;g<GD_COUNT;g++){
   s32 v=-e->net[g];
   if(v<=0)continue;
   for(int k=0;k<3;k++)if(v>tv[k]){for(int j=2;j>k;j--){tv[j]=tv[j-1];top[j]=top[j-1];}tv[k]=v;top[k]=(u8)g;break;}
  }
  u8 mk=MarketOf(w,ci);int nr=0;
  for(int k=0;k<3&&tv[k]>0;k++){
   u8 g=top[k];u8 best=0;s32 bv=0;
   for(u8 j=1;j<w->nC;j++){
    if(j==ci||!w->c[j].alive||!w->eco[j].ports)continue;
    if(MarketOf(w,j)==mk)continue;
    s32 v=w->eco[j].net[g];
    if(v>bv){bv=v;best=j;}
   }
   if(best){e->route[nr]=best;e->routeG[nr]=g;e->routeV[nr]=(u16)Mn(60000,Mn(bv,tv[k]));nr++;}
  }
 }
}

// IA: politica fiscal 1x por mes (chamada pela diplomacia, wdiplo.c)
void WEco_AiPolicy(World *w,u8 ci){
 WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
 s32 bal=WEco_GoldWeek(w,ci)+e->xCorrupt,rev=WEco_Revenue(w,ci),lim=WEco_DebtLimit(w,ci);
 e->mil=(u8)(c->warN?2:(c->archetype==WA_EXPANSIONISTA?1:(bal<0?0:1)));
 e->tariff=(u8)(c->archetype==WA_MERCADOR||c->m.tradePct>=10?0:(c->archetype==WA_ISOLACIONISTA?2:1));
 if(bal<0||e->debt>lim/2){
  if(e->tax<2&&WDip_RevoltRisk(w,ci)<40)e->tax++;
  else if(e->edu>0)e->edu--;
  else if(e->mil>0&&!c->warN)e->mil=0;
 }else if(bal>rev/4&&e->debt==0){
  if(e->edu<2&&w->date.y>=1830)e->edu++;
  else if(e->tax>0)e->tax--;
 }
 if(WDip_RevoltRisk(w,ci)>50&&e->tax>0)e->tax--;
}
