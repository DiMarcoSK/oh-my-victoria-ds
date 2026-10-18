#include "wplan.h"
#include "wecon.h"
#include "str.h"
// wplan.c — controle da economia (ver wplan.h; SDD ADR-47). Tabelas abaixo
// sao proposta de design: balancear aqui. Custo: O(bens) por pais por semana;
// a crise que contagia o mundo e' O(paises), so' no dia do panico.

static inline s32 Mn(s32 a,s32 b){return a<b?a:b;}
static inline s32 Mx(s32 a,s32 b){return a>b?a:b;}
static inline int Cl(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static u32 Hs(u32 a,u32 b){u32 h=a*2246822519u^b*3266489917u^0x5BD1E995u;h^=h>>13;h*=0x85EBCA6Bu;h^=h>>16;return h;}

// lei LW_MODELO -> grau de controle: mercantilismo ja' dirige (1), o
// laissez-faire e' o extremo livre (0). A troca e' simetrica (0<->1).
static const u8 CTL[LW_OPTS]={1,0,2,3,4};
static const u8 SLOTS[CTL_COUNT]={0,1,2,4,6};
static const u8 META_PCT[CTL_COUNT]={0,0,10,15,25};    // producao extra da meta
static const u8 META_PRI[CTL_COUNT]={0,0,40,60,80};    // % do insumo que falta que a meta recupera
static const u8 AMP[CTL_COUNT]={150,120,100,55,15};    // amplitude do ciclo (%)
static const u8 ADM[CTL_COUNT]={80,100,100,112,125};   // custo da maquina do Estado (%)
static const u8 SOE[CTL_COUNT]={0,0,1,3,6};            // lucro das estatais (% do PIB)

int Plan_Ctl(const WEco *e){return CTL[e->law[LW_MODELO]%LW_OPTS];}
u8 Plan_LawOfCtl(int ctl){return CTL[Cl(ctl,0,CTL_COUNT-1)];}
int Plan_Slots(const WEco *e){return SLOTS[Plan_Ctl(e)];}
int Plan_Used(const WEco *e){int n=0;for(int g=0;g<GD_COUNT;g++)if(e->gpol[g])n++;return n;}
int Plan_Pct(const WEco *e){return e->cyc/64;}
static int Count(const WEco *e,u8 pol){int n=0;for(int g=0;g<GD_COUNT;g++)if(e->gpol[g]==pol)n++;return n;}

u16 Plan_Why(const World *w,u8 ci,u8 g,u8 pol){
 const WEco *e=&w->eco[ci];
 if(g>=GD_COUNT||pol>=GP_COUNT)return STR_VAZIO;
 if(pol==GP_LIVRE||e->gpol[g]==pol)return 0;
 if(w->date.y<Gd_Year(g))return STR_GP_NAO_EXISTE;
 int c=Plan_Ctl(e);
 if(c==0)return STR_GP_NAO_LIVRE;                 // laissez-faire: o Estado nao mexe
 if(pol>=GP_TETO&&c<2)return STR_GP_NAO_DOUT;
 if(Plan_Used(e)-(e->gpol[g]?1:0)>=Plan_Slots(e))return STR_GP_SEM_VAGA;
 return 0;
}
bool Plan_Set(World *w,u8 ci,u8 g,u8 pol){
 if(Plan_Why(w,ci,g,pol))return false;
 WEco *e=&w->eco[ci];
 if(pol&&e->gpol[g]!=pol)w->nPolicy++;
 e->gpol[g]=pol;
 return true;
}
void Plan_Fix(WEco *e){
 int c=Plan_Ctl(e),s=Plan_Slots(e),n=0;
 for(int g=0;g<GD_COUNT;g++){
  u8 p=e->gpol[g];
  if(p>=GP_COUNT||(p&&c==0)||(p>=GP_TETO&&c<2))p=GP_LIVRE;
  if(p&&++n>s)p=GP_LIVRE;                          // sem vaga: corta as ultimas
  e->gpol[g]=p;
 }
}
u16 Plan_PolStr(u8 pol){static const u16 S[GP_COUNT]={STR_GPS_0,STR_GPS_1,STR_GPS_2,STR_GPS_3};return S[pol%GP_COUNT];}
u16 Plan_BtnStr(u8 pol){static const u16 S[GP_COUNT]={STR_GPB_0,STR_GPB_1,STR_GPB_2,STR_GPB_3};return S[pol%GP_COUNT];}
u16 Plan_PolHelp(u8 pol){static const u16 S[GP_COUNT]={STR_GPD_0,STR_GPD_1,STR_GPD_2,STR_GPD_3};return S[pol%GP_COUNT];}
u16 Plan_CtlStr(int ctl){return (u16)(STR_CTL_0+Cl(ctl,0,CTL_COUNT-1));}

// ---------------- efeitos (lidos por wecon.c e wsoc.c) ----------------
int Plan_GoodPct(const WEco *e,u8 g){
 switch(e->gpol[g%GD_COUNT]){
  case GP_SUBSIDIO:return 125;
  case GP_TETO:return 85;                          // produtor some, mercado negro
  case GP_META:return 100+META_PCT[Plan_Ctl(e)];
  default:return 100;
 }
}
int Plan_MetaPri(const WEco *e){return META_PRI[Plan_Ctl(e)];}
// estagnacao: cada ~16 meses de planificacao custa 1% (teto 15%)
static int Stag(const WEco *e){return Mn(15,e->planW/70);}
int Plan_ProdPct(const WEco *e){return e->cyc/512-Stag(e);}
int Plan_InvPct(const WEco *e){return Mx(30,100+e->cyc/128);}
int Plan_InflPermil(const WEco *e){return e->cyc/256-6*Count(e,GP_TETO);}
int Plan_AdminPct(const WEco *e){return ADM[Plan_Ctl(e)];}
s32 Plan_StatePct(const WEco *e){return SOE[Plan_Ctl(e)];}
// o Estado paga 1/4 do valor produzido do bem subsidiado (mesma unidade do PIB)
s32 Plan_SubsCost(const WEco *e){
 s32 v=0;
 for(int g=0;g<GD_COUNT;g++)if(e->gpol[g]==GP_SUBSIDIO)v+=(s32)((s64)Mx(0,e->sup[g])*e->price[g]/400);
 return v;
}
int Plan_HappyFx(const WEco *e,u8 k){
 int u=e->unemp,c=Plan_Pct(e),v;
 const u8 *P=e->gpol;
 switch(k){
  case 0:   // pobres: desemprego doi; pao e roupa tabelados alegram
   v=-u/2+(P[GD_GRAOS]==GP_TETO?5:0)+(P[GD_TEXTEIS]==GP_TETO?4:0);break;
  case 1:   // medios: emprego, confianca e bens tabelados
   v=-u/4+c/10+(P[GD_BENS]==GP_TETO?3:0)+(P[GD_ELETRONICOS]==GP_TETO?3:0)+(P[GD_PETROLEO]==GP_TETO?2:0);break;
  default:  // ricos: vivem da alta; tabela de preco e' confisco
   v=c/6-2*Count(e,GP_TETO);break;
 }
 return Cl(v,-25,15);
}
int Plan_Score(const WEco *e,u8 g){
 if(g>=GD_COUNT)return 0;
 switch(e->gpol[g]){case GP_META:return 60;case GP_SUBSIDIO:return 25;case GP_TETO:return -30;default:return 0;}
}

// ---------------- ciclo economico ----------------
static void Note(World *w,u8 ci,u16 title,u16 body){
 const WCountry *c=&w->c[ci];
 char t[112];int n=0;const char *p=NATIONS[c->nat].name;
 while(*p&&n<40)t[n++]=*p++;
 p=Str(body);while(*p&&n<110)t[n++]=*p++;
 t[n]=0;World_Msg(w,Str(title),t);
}
static s32 WorldGdp(const World *w){s32 t=0;for(u8 j=1;j<w->nC;j++)if(w->c[j].alive)t+=Mx(0,w->eco[j].gdp);return t;}
void Plan_Shock(World *w,u8 ci,int pts){
 WEco *e=&w->eco[ci];
 int d=pts*AMP[Plan_Ctl(e)]/100;
 if(d<=0)return;
 int fl=Mn(e->cyc/64,0)-d;
 if(e->cycPh==CY_CRISE&&e->cycP<fl)fl=e->cycP;
 if(e->cycPh==CY_ALTA)e->crashes++;
 e->cycPh=CY_CRISE;e->cycP=(s8)Cl(fl,-100,-2);
}
static void Crash(World *w,u8 ci){
 WEco *e=&w->eco[ci];WCountry *c=&w->c[ci];
 u32 h=Hs(ci*7u+1u,w->day/7);
 int fl=-(e->cycP*(60+(int)(h%30u))/100);
 e->cycPh=CY_CRISE;e->cycP=(s8)Cl(fl,-100,-2);
 e->crashes++;w->nCrash++;
 e->invest-=e->invest/3;                           // o capital evapora no panico
 s32 tot=WorldGdp(w);
 bool big=tot>0&&e->gdp*10>tot;
 if(c->player||big)Note(w,ci,STR_CRASH_T,STR_CRASH_MSG);
 if(!big)return;
 // potencia em panico: quem comercia com o mundo pega a gripe
 for(u8 j=1;j<w->nC;j++){
  WEco *o=&w->eco[j];
  if(j==ci||!w->c[j].alive||o->cycPh!=CY_ALTA||o->cyc<=0||o->gdp<=0)continue;
  s32 open=(o->expVal+o->impVal)/Mx(1,o->gdp);     // ~% do PIB (expVal vem x100)
  if(open<10)continue;
  Plan_Shock(w,j,(int)Mn(40,10+open/4));
  if(w->c[j].player)Note(w,j,STR_CRASH_T,STR_CRASH_CONTAGIO);
 }
}
void Plan_Week(World *w,u8 ci){
 WEco *e=&w->eco[ci];WCountry *c=&w->c[ci];
 int C=Plan_Ctl(e),amp=AMP[C],b=e->bank,x=e->cyc;
 u32 h=Hs(ci,w->day/7);
 switch(e->cycPh){
  case CY_ALTA:{
   if(e->cycP<=0)e->cycP=(s8)Cl(amp*(30+(int)(h%40u))/100,3,95);   // pico escondido desta alta
   // credito barato infla a bolha; juro alto esfria (pouso suave, sem panico)
   int r=amp*(10+3*(5-b)+(C==0?4:0)-(e->infl>100?3:0))/100;
   if(r>0)x+=r;else if(x>0)x=Mx(0,x+r*2);
   if(x>=e->cycP*64){e->cyc=(s16)x;Crash(w,ci);x=e->cyc;}
  }break;
  case CY_CRISE:
   x-=Mx(32,3*64*amp/100);
   if(x<=e->cycP*64){x=e->cycP*64;e->cycPh=CY_RETOMADA;}
   break;
  default:{   // retomada: juro baixo e Estado que gasta aceleram
   int r=Mx(3,7+3*(5-b)+3*C);
   x+=r;
   if(x>=0){x=0;e->cycPh=CY_ALTA;e->cycP=0;if(c->player)Note(w,ci,STR_RETOMADA_T,STR_RETOMADA_MSG);}
  }break;
 }
 e->cyc=(s16)Cl(x,-6400,6400);
 // desemprego: estrutural da doutrina + o da crise; a alta quase zera; o
 // plano emprega todo mundo (o custo dele e' a estagnacao)
 {int p=e->cyc/64;
  int u=4+(C==0?2:0)-(C>=3?2:0)+(p<0?-p/3:-p/15)-(e->jobs<90?2:0)-(c->warN?2:0);
  e->unemp=(u8)(C==4?0:Cl(u,0,45));}
 if(C==4){if(e->planW<2000)e->planW++;}
 else if(e->planW)e->planW=(u16)(e->planW-e->planW/26-1);
 if(c->player){w->cycH[w->cycHN%52]=(s8)Cl(e->cyc/64,-100,100);w->cycHN=(u8)((w->cycHN+1)%52);}
}

// ---------------- IA: politicas 1x por mes ----------------
void Plan_Ai(World *w,u8 ci){
 WEco *e=&w->eco[ci];const WCountry *c=&w->c[ci];
 int C=Plan_Ctl(e),S=SLOTS[C],y=w->date.y;
 u8 nw[GD_COUNT];for(int g=0;g<GD_COUNT;g++)nw[g]=GP_LIVRE;
 int n=0;
 bool broke=c->gold<0&&e->debt>WEco_DebtLimit(w,ci)/2;
 #define WANT(g,p) do{if(n<S&&!nw[g]&&y>=Gd_Year(g)&&((p)!=GP_SUBSIDIO||!broke)&&((p)<GP_TETO||C>=2)){nw[g]=(u8)(p);n++;}}while(0)
 if(S){
  if(c->warN)WANT(GD_ARMAS,C>=2?GP_META:GP_SUBSIDIO);
  if(e->ind<90&&e->lack<GD_COUNT)WANT(e->lack,C>=2?GP_META:GP_SUBSIDIO);
  if(e->sat[GD_GRAOS]<85)WANT(GD_GRAOS,GP_SUBSIDIO);
  if(C==1)WANT(GD_TEXTEIS,GP_SUBSIDIO);            // mercantilismo: manufatura p/ exportar
  if(C>=3){WANT(GD_ACO,GP_META);WANT(GD_MAQUINAS,GP_META);WANT(GD_ELETRONICOS,GP_META);}
  if(C>=2&&e->infl>70&&e->sat[GD_GRAOS]>=95)WANT(GD_GRAOS,GP_TETO);
  if(C==4){if(e->sat[GD_TEXTEIS]>=90)WANT(GD_TEXTEIS,GP_TETO);if(e->sat[GD_BENS]>=90)WANT(GD_BENS,GP_TETO);}
 }
 #undef WANT
 for(int g=0;g<GD_COUNT;g++){if(nw[g]&&nw[g]!=e->gpol[g])w->nPolicy++;e->gpol[g]=nw[g];}
}
