#include <stdlib.h>
#include "wecon.h"
#include "hot.h"
#include "city.h"
#include "str.h"
#include "wdiplo.h"
#include "wlaw.h"
// wecon.c — economia do mundo (ver wecon.h; DESIGN 4.8; SDD ADR-26).
// Custo: 1x por semana, O(cidades) + O(paises x bens) + rotas O(portos x 3 x
// paises). Buffers estaticos (.bss, ~27KB), zero malloc, sem float, sem RNG.

//                              graos madeira fibra minerio luxo texteis ferram. armas petroleo aco bens maquinas eletronicos
const u16 GD_BASE[GD_COUNT]={  10,   12,     14,   16,     30,  26,     34,     44,   40,      36, 30,  60,      80};
static const u16 GD_YEAR[GD_COUNT]={0,0,0,0,0,0,0,0,1860,1850,0,1870,1950};
static const u16 GD_STR[GD_COUNT]={STR_GD_GRAOS,STR_GD_MADEIRA,STR_GD_FIBRA,STR_GD_MINERIO,STR_GD_LUXO,STR_GD_TEXTEIS,
 STR_GD_FERRAMENTAS,STR_GD_ARMAS,STR_GD_PETROLEO,STR_GD_ACO,STR_GD_BENS,STR_GD_MAQUINAS,STR_GD_ELETRONICOS};
u16 Gd_Str(u8 g){return GD_STR[g<GD_COUNT?g:0];}
u16 Gd_Year(u8 g){return g<GD_COUNT?GD_YEAR[g]:0;}
// manufaturados: saem da industria (rodam a ind% do insumo que falta)
static inline bool Manuf(int g){return (g>=GD_TEXTEIS&&g<=GD_ARMAS)||g>=GD_ACO;}
// vagas da industria/extracao (milhares de trabalhadores por obra, ADR-37):
// a cidade so' toca as fabricas que a populacao consegue tocar
static const u8 JOBS[BK_COUNT]={[BK_OFICINA]=4,[BK_FABRICA]=10,[BK_ARSENAL]=5,[BK_MINA]=4,[BK_SERRARIA]=2,
 [BK_POCO]=3,[BK_SIDERURGICA]=10,[BK_MANUFATURA]=6,[BK_MAQUINARIA]=8,[BK_ELETRONICA]=8,[BK_ARMAMENTOS]=10};
// % das vagas que a cidade preenche (40% da populacao trabalha nelas)
int WEco_CityJobs(const WCity *c,int *want){
 int j=0;for(int s=0;s<CITY_SLOTS;s++)j+=JOBS[c->bld[s]%BK_COUNT];
 if(want)*want=j;
 if(!j)return 100;
 int work=c->pop*2/5;
 return work>=j?100:work*100/j;
}
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
 // ADR-37: moveis, louca e roupa boa (vila em diante); aco nas obras da
 // cidade grande industrial; eletronicos no seculo XX tardio
 if(c->tier>=CT_VILAREJO)dem[GD_BENS]+=pop/90;
 if(c->tier>=CT_CIDADE&&y>=1880)dem[GD_ACO]+=pop/300;
 if(y>=1950)dem[GD_ELETRONICOS]+=pop/400;
 // subsistencia: a lavoura em volta alimenta a vila mesmo sem obra
 prod[GD_GRAOS]+=c->agri/12;
 int ind=y<1880?100:(y<1950?200:300);   // produtividade industrial por era
 int agr=100+(y-1800)*3/5;              // e a do campo
 bool rail=false;int usina=0,viz=0,ind0=0;
 int jp=WEco_CityJobs(c,0);              // falta gente: a industria roda a jp%
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
  // cadeia longa (ADR-37): minerio -> aco -> maquinas/armamentos/eletronicos
  case BK_SIDERURGICA:if(y>=1850){need[GD_MINERIO]+=10;need[GD_MADEIRA]+=2;prod[GD_ACO]+=12*ind/100;}break;
  case BK_MANUFATURA:need[GD_MADEIRA]+=4;need[GD_TEXTEIS]+=4;prod[GD_BENS]+=12*ind/100;break;
  case BK_MAQUINARIA:if(y>=1870){need[GD_ACO]+=6;need[GD_FERRAMENTAS]+=2;prod[GD_MAQUINAS]+=8*ind/100;}break;
  case BK_ELETRONICA:if(y>=1950){need[GD_ACO]+=3;need[GD_PETROLEO]+=4;prod[GD_ELETRONICOS]+=16*ind/100;}break;
  case BK_ARMAMENTOS:if(y>=1850){need[GD_ACO]+=6;need[GD_FERRAMENTAS]+=3;prod[GD_ARMAS]+=28*ind/100;}break;
  default:break;
 }
 if(man>100)for(int g=0;g<GD_COUNT;g++)if(Manuf(g))prod[g]=prod[g]*man/100;
 // maquinas: cada obra industrial grande pede 1 carga/semana depois de 1870
 // (o bonus de produtividade vem da satisfacao nacional, em WEco_Week)
 for(int s=0;s<CITY_SLOTS;s++){u8 k=c->bld[s];if(JOBS[k%BK_COUNT]>=5)ind0++;}
 if(y>=1870)need[GD_MAQUINAS]+=ind0;
 // cidade sem gente p/ as vagas: industria e extracao rodam a jp%
 if(jp<100)for(int g=0;g<GD_COUNT;g++){if(g!=GD_GRAOS&&g!=GD_FIBRA&&g!=GD_LUXO)prod[g]=prod[g]*jp/100;need[g]=need[g]*jp/100;}
 if(rail)for(int g=0;g<GD_COUNT;g++)prod[g]+=prod[g]/5;
}

// ---------------- marinha ----------------
s32 WEco_ShipCost(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];
 u8 g=w->date.y>=1860?GD_MINERIO:GD_MADEIRA;   // vela de madeira -> couracado de ferro
 return WEco_Nom(w,ci,60+(s32)e->price[g]*3+(w->date.y-1800)/4);
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

s32 WEco_Revenue(const World *w,u8 ci){const WEco *e=&w->eco[ci];return e->rProd+e->rPop+e->rCity+e->rLand+e->rTariff+e->rExport+e->rArms;}
s32 WEco_Expense(const World *w,u8 ci){const WEco *e=&w->eco[ci];return e->xArmy+e->xNavy+e->xAdmin+e->xEdu+e->xSoc+e->xUpkeep+e->xDebt+e->xCorrupt;}
s32 WEco_Nom(const World *w,u8 ci,s32 v){u32 p=w->eco[ci].cpi?w->eco[ci].cpi:CPI1;return (s32)((s64)v*p/CPI1);}
s32 WEco_GoldWeek(const World *w,u8 ci){return WEco_Revenue(w,ci)-WEco_Expense(w,ci);}
// limite de credito: meio ano de receita (minimo 300); cai a cada calote
s32 WEco_DebtLimit(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];
 s32 r=WEco_Revenue(w,ci);if(r<12)r=12;
 return (s32)((s64)r*26/(1+e->defaults)*Mx(20,100+e->fx.v[LFX_DEBT])/100);
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
 int base=y<1850?5:(y<1900?7:(y<1950?10:(y<1990?17:11)));   // ADR-40: as leis de saude somam ate +3 por mil
 if(y>=1950&&c->lit>850)base-=(c->lit-850)/30;       // pais rico e letrado cresce menos
 base+=(e->life-60)/6;
 if(e->sat[GD_GRAOS]<60)base-=(60-e->sat[GD_GRAOS])/3; // fome
 if(c->warN)base-=4;
 if(c->unrestT)base-=3;
 // leis (ADR-40): saude e trabalho somam direto; imigracao e' soma zero no
 // mundo — so' atrai gente se o pais vive bem (nivel de vida acima de 60),
 // e a porta aberta num pais pobre e' por onde o povo SAI
 {int im=Law_FxOf(LW_IMIGRACAO,e->law[LW_IMIGRACAO],LFX_GROW);
  int pull=((int)e->life-60)*abs(im)/15;if(pull>abs(im))pull=abs(im);
  if(pull<-abs(im))pull=-abs(im);
  base+=e->fx.v[LFX_GROW]-im+(im>0?pull:(im<0?-abs(im)/2:0));}
 return base<-30?-30:(base>35?35:base);
}
// alfabetizacao (decimos de %): teto possivel na era; a escola (gasto em
// educacao + universidades) puxa em direcao ao teto, e a difusao do mundo
// garante um piso que sobe com o tempo. Quase nunca cai.
static int LitCap(int y){return y<1850?350:(y<1900?350+(y-1850)*6:(y<1950?650+(y-1900)*5:(y<2000?900+(y-1950)*9/5:990)));}
static int LitFloor(int y){return y<1850?10:(y<1900?10+(y-1850)*2/5:(y<1950?30+(y-1900)*2:(y<2000?130+(y-1950)*8:530+(y-2000)*8)));}
static void LitWeek(const World *w,WCountry *c,const WEco *e,int unis){
 int y=w->date.y,l=c->lit,cap=LitCap(y),fl=LitFloor(y);
 int r=Mx(0,e->fx.v[LFX_LIT])+unis*3;           // fracao (por mil) do vao ate' o teto fechada por ano (lei de educacao)if(c->m.prestigeYear>=2)r+=5;
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
  e->life=70;e->ind=100;e->hub=-1;e->rate=4;e->reqG=0xFF;e->jobs=100;e->cpi=CPI1;
  Law_Init(w,ci);
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

// ---------------- pedidos do povo (ADR-37) ----------------
// A cada ~8 semanas, um bem de consumo em falta vira PEDIDO: atender (90% da
// demanda) em 12 semanas alegra (reqMood +10) e da' prestigio; ignorar gera
// protesto (reqMood -12, militancia). reqMood volta a 0 devagar e entra no
// alvo de felicidade (wdiplo.c). Sem RNG: so' o calendario e o mercado.
// So' o jogador recebe pedidos (a IA ja' segue o mercado; puni-la sem ela
// poder ler o pedido so' multiplicava revolucoes pelo mundo).
static void Requests(World *w,u8 ci){
 WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
 const int Y=w->date.y;
 if(!c->player){e->reqG=0xFF;e->reqMood=0;return;}
 if(e->reqMood>0)e->reqMood--;else if(e->reqMood<0&&((w->day/7)&1))e->reqMood++;
 if(e->reqG<GD_COUNT){
  if(e->sat[e->reqG]>=90){
   e->reqMood=10;c->prestige+=5;
   if(c->player)World_Msg(w,Str(STR_PEDIDO_OK),Str(Gd_Str(e->reqG)));
   e->reqG=0xFF;return;
  }
  if(e->reqT&&--e->reqT)return;
  e->reqMood=-10;c->militancy=(u8)(c->militancy>94?100:c->militancy+6);
  if(c->player)World_Msg(w,Str(STR_PEDIDO_FALHOU),Str(Gd_Str(e->reqG)));
  e->reqG=0xFF;return;
 }
 if(((w->day/7)+ci)%8)return;
 static const u8 CONS[7]={GD_GRAOS,GD_TEXTEIS,GD_BENS,GD_LUXO,GD_FERRAMENTAS,GD_PETROLEO,GD_ELETRONICOS};
 static const u16 FROM[7]={0,0,0,0,1830,1900,1950};
 int best=-1,bs=60;
 for(int k=0;k<7;k++){u8 g=CONS[k];if(Y<FROM[k]||Y<Gd_Year(g)||!e->dem[g])continue;if(e->sat[g]<bs){bs=e->sat[g];best=g;}}
 if(best<0)return;
 e->reqG=(u8)best;e->reqT=12;
 if(c->player){char t[112];int n=0;const char *a=Str(Gd_Str((u8)best)),*b=Str(STR_PEDIDO_MSG);
  while(*a&&n<40)t[n++]=*a++;
  t[n++]=' ';
  while(*b&&n<110)t[n++]=*b++;
  t[n]=0;
  World_Msg(w,Str(STR_PEDIDO),t);}
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
  // em guerra o exercito queima municao: a demanda por armas triplica e o
  // preco mundial sobe (o pais que exporta armas lucra com as guerras, ADR-37)
  if(c->warN)e->dem[GD_ARMAS]=e->dem[GD_ARMAS]*3+c->atkTroops/200+2;
  e->dem[GD_BENS]=rp/(Y<1900?800:400);
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
  cap=cap*(100+c->m.tradePct)/100*Mx(10,100+e->fx.v[LFX_TRADE])/100;   // leis de comercio (ADR-40)
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
  int bonus=100+c->m.prodPct+c->lit/40+e->fx.v[LFX_PROD];if(c->unrestT)bonus=bonus*70/100;   // letrados produzem mais (ate' +25%)
  if(Y>=1870)bonus+=e->sat[GD_MAQUINAS]/4;   // maquinas da semana passada: ate' +25% (ADR-37)
  e->ind=(u8)ef;e->lack=MLACK[m];
  for(int g=0;g<GD_COUNT;g++){
   if(Manuf(g)){s32 r=RUR[ci][g];e->sup[g]=r+(e->sup[g]-r)*ef/100*bonus/100;MS[m][g]+=e->sup[g];}
   s32 used=NEED[ci][g]*ef/100;
   e->dem[g]+=used;MD[m][g]+=used;
   if(Y<Gd_Year((u8)g)){e->sup[g]=e->dem[g]=0;}
  }
 }
 // 4b) contratos de armas (ADR-37): o vendedor manda a sobra de armas direto
 // p/ quem esta' em guerra; o comprador paga no preco mundial com agio e
 // reforca o exercito. A carga sai do mercado de um e entra no do outro.
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
  c->armsV=0;e->rArms=0;
  u8 t=c->armsTo;
  if(!t)continue;
  if(!c->alive||t>=w->nC||!w->c[t].alive||!w->c[t].warN||World_AtWar(w,ci,t)){c->armsTo=0;continue;}
  WCountry *b=&w->c[t];
  s32 wp=Mx(1,w->wprice[GD_ARMAS]);
  s32 v=Mn(e->sup[GD_ARMAS]-e->dem[GD_ARMAS],8+(b->troops+b->atkTroops)/300);if(v>400)v=400;
  s32 pay=WEco_Nom(w,t,v*wp/50);                    // o comprador paga em ouro de hoje
  if(pay>b->gold){v=b->gold>0?(s32)((s64)v*b->gold/Mx(1,pay)):0;pay=WEco_Nom(w,t,v*wp/50);}
  if(v<=0)continue;
  e->sup[GD_ARMAS]-=v;w->eco[t].sup[GD_ARMAS]+=v;
  MS[MarketOf(w,ci)][GD_ARMAS]-=v;MS[MarketOf(w,t)][GD_ARMAS]+=v;
  b->gold-=pay;e->rArms=WEco_Nom(w,ci,v*wp/50);
  if(b->troops<b->cap*3/2)b->troops+=v*2;
  c->armsV=(u16)v;c->armsTot=(u16)Mn(60000,c->armsTot+v);w->nArms+=(u32)v;
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
  s32 val=0,trade=0;e->used=0;e->expVal=e->impVal=0;
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
   if(q>0)e->expVal+=q*e->price[g];else e->impVal+=aq*e->price[g];
   val+=e->sup[g]*e->price[g];
  }
  // uso dos portos: a parte deste pais no comercio externo do mercado
  {s32 ext=0;for(int g=0;g<GD_COUNT;g++)ext+=EXP[m][g]+IMP[m][g];
   e->used=(u16)Mn(e->cap,MCAP[m]?(s32)((s64)ext*e->cap/MCAP[m]):0);}
  // ---- balanco fiscal (ADR-32) com as leis (ADR-40) e precos nominais ----
  // tudo que e' ouro sai em valor NOMINAL (x cpi): a inflacao sobe receita e
  // custo juntos, mas a divida antiga nao sobe — ela se dissolve.
  const s16 *F=e->fx.v;
  const s64 cpi=e->cpi?e->cpi:CPI1;
  #define NOM(v) ((s32)((s64)(v)*cpi/CPI1))
  s32 rate2=2*(100+F[LFX_REV]);if(rate2<0)rate2=0;        // aliquota base 2 (x100)
  e->gdp=NOM(val/100);
  e->rProd=NOM((s64)val*rate2/100000);
  e->rCity=NOM(c->cityGold*7/2*(100+c->m.goldPct)/100);               // taxas urbanas (City_Day soma por dia)
  e->rLand=NOM(((s32)c->cells/48+1)*7*(100+c->m.goldPct)/100);         // renda da terra
  // imposto per capita: quem tem mais gente (e mais letrada) arrecada mais
  e->rPop=NOM((s64)(c->popK/600)*rate2/100*(20+c->lit/10)/40);
  e->rTariff=NOM(trade/200*(F[LFX_TARIFF]>0?F[LFX_TARIFF]:1));
  // casas exportadoras: o lucro de vender ao exterior e' tributado (o pais
  // pequeno que exporta muito enriquece, ADR-37); mercador ganha mais
  e->rExport=NOM((s64)(e->expVal/500)*rate2/100*Mx(0,100+c->m.tradePct+F[LFX_EXPORT])/100);
  int unis=0,obras=0,banks=0,jWant=0,jFill=0;
  for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci){
   {int j,p=WEco_CityJobs(&w->city[i],&j);jWant+=j;jFill+=j*p/100;}
   for(int k=0;k<CITY_SLOTS;k++){u8 b=w->city[i].bld[k];
   if(b){obras++;if(b==BK_UNIVERSIDADE)unis+=2;if(b==BK_ESCOLA)unis++;if(b==BK_BANCO||b==BK_BOLSA)banks++;}}}
  e->jobs=(u8)(jWant?jFill*100/jWant:100);
  e->xArmy=NOM((c->troops+c->atkTroops)/60*Mx(10,100+F[LFX_ARMYX])/100);
  e->xNavy=NOM(c->ships);
  e->xAdmin=NOM((s32)c->cells/60+c->nCity+(s32)(c->popK/2000));      // maquina do Estado
  {s32 cofre=NOM(2000);e->xCorrupt=c->gold>cofre?(c->gold-cofre)/100:0;} // cofre parado: desvio e corrupcao
  e->xEdu=NOM((s32)((u64)c->popK*Mx(0,F[LFX_EDU])/1000)/(Y<1870?3:1));
  e->xSoc=NOM((s32)((u64)c->popK*Mx(0,F[LFX_SOC])/1000)/(Y<1870?3:1)); // saude e trabalho (leis)
  e->xUpkeep=NOM(obras/2);
  // dirigismo/planificacao: o Estado separa parte da receita p/ erguer obras
  s32 stInv=F[LFX_STATE]>0?(e->rProd+e->rPop+e->rCity+e->rLand+e->rExport)*F[LFX_STATE]/100:0;
  e->xUpkeep+=stInv;
  e->xDebt=(s32)((s64)e->debt*e->rate/5200);
  e->inc=e->rProd+e->rPop+e->rCity+e->rLand+e->rExport+e->rArms;e->trade=e->rTariff;e->exp=WEco_Expense(w,ci);
  s32 g=c->gold+e->inc+e->trade-e->exp;
  e->printed=0;
  // Emissao livre (lei de moeda 4): o deficit e' coberto com dinheiro novo —
  // sem divida, mas a inflacao dispara (abaixo)
  if(g<0&&e->law[LW_MOEDA]==4){e->printed=-g;g=0;}
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
   int r=3+risk+e->defaults*2-(banks>4?2:banks/2)+F[LFX_RATE];e->rate=(u8)(r<1?1:(r>20?20:r));}
  // inflacao (ADR-40): a da lei de moeda + a da maquina de imprimir; o
  // indice de precos anda 1x por semana (por mil ao ano / 52)
  {s32 rv=Mx(12,WEco_Revenue(w,ci));
   int tgt=F[LFX_INFL]+(e->printed>0?(int)Mn(400,(s64)e->printed*600/rv):0);
   if(c->warN&&e->law[LW_MOEDA]>=2)tgt+=8;              // guerra com papel-moeda
   e->infl=(s16)((e->infl*3+tgt)/4);
   if(e->infl>0){u64 nc=(u64)e->cpi+(u64)e->cpi*(u32)e->infl/52000u;e->cpi=(u32)(nc>4000000000u?4000000000u:nc);}}
  // populacao e alfabetizacao andam 1x por semana
  {int gr=GrowthPermil(w,c,e);
   s64 d=(s64)c->popK*gr/52000;if(!d&&gr>0&&c->popK>500)d=1;
   s64 np=(s64)c->popK+d;c->popK=(u32)(np<10?10:np);
   LitWeek(w,c,e,unis);}
  // nivel de vida: comer pesa mais; o resto depende da era
  int lw=e->sat[GD_GRAOS]*40+e->sat[GD_TEXTEIS]*20+e->sat[GD_LUXO]*15+e->sat[GD_BENS]*10;
  int tw=85;
  if(Y>=1830){lw+=e->sat[GD_FERRAMENTAS]*15;tw+=15;}
  if(Y>=1900){lw+=e->sat[GD_PETROLEO]*10;tw+=10;}
  if(Y>=1950){lw+=e->sat[GD_ELETRONICOS]*10;tw+=10;}
  e->life=(u8)(lw/tw);
  Requests(w,ci);
  // investimento privado (ADR-37): parte do PIB vira fundo (imposto baixo
  // atrai mais; banco/bolsa multiplicam); o fundo ergue obras (City_Invest)
  {s32 add=(s32)((s64)e->gdp*Mx(0,4*(100+F[LFX_INV]))/100000)*(2+(banks>6?6:banks))/2;
   if(c->unrestT||c->warN>1)add/=2;
   e->invest+=add+stInv;
   {s32 cap=NOM(30000);if(e->invest>cap)e->invest=cap;}
   if((((w->day/7)+ci)&3)==0)City_Invest(w,ci);}   // 1x a cada 4 semanas por pais (escalonado)
  #undef NOM
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
 // mercador com sobra de armas vende a quem esta' em guerra (o maior exercito
 // que nao e' inimigo dele nem do aliado): o mundo arma os dois lados
 if(!c->armsTo&&e->sup[GD_ARMAS]>e->dem[GD_ARMAS]+10&&(c->archetype==WA_MERCADOR||c->archetype==WA_OPORTUNISTA)){
  u8 best=0;s32 bt=0;
  for(u8 j=1;j<w->nC;j++){const WCountry *b=&w->c[j];
   if(j==ci||!b->alive||!b->warN||World_AtWar(w,ci,j)||b->gold<50)continue;
   if(c->ally[0]&&World_AtWar(w,c->ally[0],j))continue;
   if(b->troops>bt){bt=b->troops;best=j;}}
  c->armsTo=best;
 }
 Law_Ai(w,ci);                // leis: 1 passo por mes rumo ao alvo (wlaw.c, ADR-40)
}
