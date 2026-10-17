#include "wsoc.h"
#include "wecon.h"
#include "wlaw.h"
#include "wdiplo.h"
#include "city.h"
#include "str.h"
// wsoc.c — estratos sociais e burguesia (ver wsoc.h; SDD ADR-42).
// Custo: Soc_Prep e' 1 passada nas cidades por semana; Soc_Week e' O(1) por pais.

static inline s32 Mn(s32 a,s32 b){return a<b?a:b;}
static inline s32 Mx(s32 a,s32 b){return a>b?a:b;}
static inline int Cl(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static u32 Hs(u32 a,u32 b){u32 h=a*2654435761u^b*40503u^0x7F4A7C15u;h^=h>>15;h*=0x2C1B3C6Du;h^=h>>12;return h;}

// resumo das cidades de cada pais (1 passada por semana, antes dos paises)
enum { SC_TEMPLO=0, SC_ESCOLA, SC_HOSP, SC_TREM, SC_BANCO, SC_IND, SC_CID, SC_N };
static u16 CNT[W_MAXC+1][SC_N];
static u32 URB[W_MAXC+1];          // populacao urbana (milhares)
void Soc_Prep(World *w){
 for(int c=0;c<=W_MAXC;c++){URB[c]=0;for(int k=0;k<SC_N;k++)CNT[c][k]=0;}
 for(int i=0;i<w->nCity;i++){
  const WCity *c=&w->city[i];u8 o=c->owner;
  if(!o)continue;
  URB[o]+=c->pop;CNT[o][SC_CID]++;
  for(int s=0;s<CITY_SLOTS;s++){
   u8 lv=c->blv[s]?c->blv[s]:1;
   switch(c->bld[s]){
    case BK_TEMPLO:CNT[o][SC_TEMPLO]++;break;
    case BK_ESCOLA:case BK_UNIVERSIDADE:CNT[o][SC_ESCOLA]++;break;
    case BK_HOSPITAL:CNT[o][SC_HOSP]++;break;
    case BK_FERROVIA:CNT[o][SC_TREM]++;break;
    case BK_BANCO:case BK_BOLSA:CNT[o][SC_BANCO]+=lv;break;
    case BK_FABRICA:case BK_SIDERURGICA:case BK_MAQUINARIA:case BK_ELETRONICA:case BK_ARMAMENTOS:case BK_MANUFATURA:case BK_USINA:
     CNT[o][SC_IND]+=lv;break;
    default:break;
   }
  }
 }
}

// a cultura e' a mesma regra do estilo das cidades (city.c): cena da nacao no catalogo
u8 Soc_Culture(const World *w,u8 ci){return City_StyleOf(w,ci);}
// desejos possiveis por cultura x estrato (proposta de design, DESIGN 4.11):
// o europeu pobre quer pao e imposto baixo; o asiatico, arroz, templo e ordem;
// o islamico, mesquita e madraca; o africano e o nativo, terra e paz.
static const u8 WANTS[ST_COUNT][SS_COUNT][4]={
 /*EURO  */{{DW_COMIDA,DW_ROUPA,DW_IMPOSTO,DW_SAUDE},{DW_ESCOLA,DW_BENS,DW_FERROVIA,DW_ELETRO},{DW_JUROS_ALTOS,DW_IMPOSTO,DW_GLORIA,DW_LUXO}},
 /*ASIA  */{{DW_COMIDA,DW_TEMPLO,DW_ROUPA,DW_PAZ},{DW_ESCOLA,DW_LUXO,DW_BENS,DW_FERROVIA},{DW_LUXO,DW_PAZ,DW_IMPOSTO,DW_JUROS_ALTOS}},
 /*ISLAM */{{DW_COMIDA,DW_TEMPLO,DW_ROUPA,DW_PAZ},{DW_TEMPLO,DW_ESCOLA,DW_BENS,DW_SAUDE},{DW_LUXO,DW_GLORIA,DW_IMPOSTO,DW_TEMPLO}},
 /*AFRICA*/{{DW_COMIDA,DW_TERRA,DW_PAZ,DW_ROUPA},{DW_BENS,DW_ESCOLA,DW_SAUDE,DW_LUXO},{DW_LUXO,DW_GLORIA,DW_JUROS_ALTOS,DW_IMPOSTO}},
 /*NATIVE*/{{DW_TERRA,DW_PAZ,DW_COMIDA,DW_TEMPLO},{DW_TERRA,DW_BENS,DW_ESCOLA,DW_PAZ},{DW_GLORIA,DW_LUXO,DW_PAZ,DW_TERRA}}};
static const u16 WSTR[DW_COUNT]={STR_VAZIO,STR_DW_COMIDA,STR_DW_ROUPA,STR_DW_BENS,STR_DW_LUXO,STR_DW_TEMPLO,STR_DW_ESCOLA,
 STR_DW_SAUDE,STR_DW_FERROVIA,STR_DW_IMPOSTO,STR_DW_JUROS_BAIXOS,STR_DW_JUROS_ALTOS,STR_DW_PAZ,STR_DW_GLORIA,STR_DW_TERRA,STR_DW_ELETRO};
u16 Soc_WantStr(u8 dw){return WSTR[dw<DW_COUNT?dw:0];}
u16 Soc_StratStr(u8 s){static const u16 S[SS_COUNT]={STR_SS_POBRE,STR_SS_MEDIA,STR_SS_RICA};return S[s%SS_COUNT];}
u16 Soc_LobbyStr(u8 lb){static const u16 S[LB_COUNT]={STR_LB_IMPOSTO,STR_LB_JUROS,STR_LB_TRABALHO,STR_LB_COMERCIO,STR_LB_CONCESSAO};return S[lb%LB_COUNT];}

static bool Avail(const World *w,u8 dw){
 int y=w->date.y;
 if(dw==DW_ELETRO)return y>=1950;
 if(dw==DW_FERROVIA)return y>=1830;
 if(dw==DW_SAUDE)return y>=1850;
 return true;
}
bool Soc_WantOk(const World *w,u8 ci,u8 dw){
 const WEco *e=&w->eco[ci];const WCountry *c=&w->c[ci];const WSoc *s=&w->soc[ci];
 int nc=Mx(1,CNT[ci][SC_CID]);
 switch(dw){
  case DW_COMIDA:return e->sat[GD_GRAOS]>=95;
  case DW_ROUPA:return e->sat[GD_TEXTEIS]>=90;
  case DW_BENS:return e->sat[GD_BENS]>=90;
  case DW_LUXO:return e->sat[GD_LUXO]>=90;
  case DW_ELETRO:return e->sat[GD_ELETRONICOS]>=90;
  case DW_TEMPLO:return CNT[ci][SC_TEMPLO]*2>=nc;
  case DW_ESCOLA:return CNT[ci][SC_ESCOLA]*2>=nc||e->law[LW_EDUCACAO]>=3;
  case DW_SAUDE:return CNT[ci][SC_HOSP]*3>=nc||e->law[LW_SAUDE]>=3;
  case DW_FERROVIA:return CNT[ci][SC_TREM]*3>=nc&&CNT[ci][SC_TREM];
  case DW_IMPOSTO:return e->law[LW_IMPOSTO]<=1;
  case DW_JUROS_BAIXOS:return e->bank<=3;
  case DW_JUROS_ALTOS:return e->bank>=8;
  case DW_PAZ:return !c->warN;
  case DW_GLORIA:return c->prestige>=s->pres0+15;
  case DW_TERRA:return e->law[LW_TRABALHO]>=2&&e->law[LW_IMIGRACAO]<=2;
  default:return false;
 }
}
// fatias-alvo (por mil): os ricos crescem com bancos e industria; os medios
// com a alfabetizacao e a cidade; o resto e' pobre
static void Target(const World *w,u8 ci,int *tr,int *tm,int *tb){
 const WCountry *c=&w->c[ci];
 int ind=CNT[ci][SC_IND],bank=CNT[ci][SC_BANCO];
 int urb=c->popK?(int)Mn(1000,(s64)URB[ci]*1000/c->popK):0;
 *tr=Cl(14+ind/3+bank,10,60);
 *tm=Cl(50+c->lit/4+urb/4+ind/2,30,560);
 int y=w->date.y,b=12+(y-1800)/3+ind*2;
 const WEco *e=&w->eco[ci];
 if(e->law[LW_MODELO]==1)b+=10;
 if(e->law[LW_MODELO]==4||c->ideo==ID_COMUNISTA)b=4;
 *tb=Cl(b,2,92);
}
void Soc_Init(World *w){
 Soc_Prep(w);
 for(u8 ci=0;ci<=W_MAXC;ci++){
  WSoc *s=&w->soc[ci];u8 *z=(u8*)s;for(unsigned k=0;k<sizeof *s;k++)z[k]=0;
  if(ci>=w->nC||!w->c[ci].alive)continue;
  int tr,tm,tb;Target(w,ci,&tr,&tm,&tb);
  s->share[SS_RICA]=(u16)tr;s->share[SS_MEDIA]=(u16)tm;s->share[SS_POBRE]=(u16)(1000-tr-tm);
  s->bourg=(u8)tb;
  for(int k=0;k<SS_COUNT;k++){s->sat[k]=55;s->inc[k]=100;}
  s->lobbyCd=(u8)(16+ci%20);
 }
 w->nCoups=w->nLobby=0;
}
// satisfacao-alvo de cada estrato: cesta de bens + leis + juros + ideologia
static int Base(const World *w,u8 ci,int k){
 const WEco *e=&w->eco[ci];const WCountry *c=&w->c[ci];const WSoc *s=&w->soc[ci];
 const u8 *L=e->law;int y=w->date.y,b=e->bank,v;
 switch(k){
 case SS_POBRE:
  v=10+(e->sat[GD_GRAOS]*40+e->sat[GD_TEXTEIS]*15+e->sat[GD_BENS]*5)/100;
  {static const s8 TX[LW_OPTS]={8,4,0,-8,-16};v+=TX[L[LW_IMPOSTO]%LW_OPTS];}
  {static const s8 TB[LW_OPTS]={-16,-8,0,6,10};v+=TB[L[LW_TRABALHO]%LW_OPTS];}
  v+=L[LW_SAUDE]*2;
  if(c->warN)v-=L[LW_EXERCITO]>=3?8:4;              // recrutamento cai nos pobres
  v+=(5-b)/2;                                         // credito barato = emprego
  if(c->ideo==ID_COMUNISTA)v+=6;
  if(e->infl>60)v-=6;
  break;
 case SS_MEDIA:
  v=12+(e->sat[GD_BENS]*20+e->sat[GD_LUXO]*10+e->sat[GD_TEXTEIS]*10)/100;
  if(y>=1830)v+=e->sat[GD_FERRAMENTAS]/12;
  if(y>=1950)v+=e->sat[GD_ELETRONICOS]/10;
  v+=L[LW_EDUCACAO]*3;
  {static const s8 TX[LW_OPTS]={6,4,0,-6,-12};v+=TX[L[LW_IMPOSTO]%LW_OPTS];}
  if(c->ideo==ID_LIBERAL)v+=6;
  if(c->ideo==ID_AUTORITARIO||c->ideo==ID_COMUNISTA)v-=6;
  if(e->infl>40)v-=8;                                 // salario fixo: a inflacao corroi
  v+=c->lit/100;
  break;
 default:
  v=24+e->sat[GD_LUXO]/5;
  {static const s8 TX[LW_OPTS]={14,7,0,-10,-26};v+=TX[L[LW_IMPOSTO]%LW_OPTS];}
  {static const s8 TM[LW_OPTS]={2,10,2,-8,-30};v+=TM[L[LW_MODELO]%LW_OPTS];}
  {static const s8 TB[LW_OPTS]={4,6,0,-6,-10};v+=TB[L[LW_TRABALHO]%LW_OPTS];}
  // juros: o rentista (aristocracia) gosta de juro alto; o burgues, de credito barato
  v+=(b-5)*(100-2*(int)s->bourg)/100;
  if(c->ideo==ID_COMUNISTA)v-=30;
  if(c->ideo==ID_TRADICIONAL||c->ideo==ID_CONSERVADOR)v+=6;
  if(e->debt>WEco_DebtLimit(w,ci)/2)v-=6;             // credor do Estado com medo do calote
  break;
 }
 v+=s->mood[k];
 if(s->want[k])v-=3;                                  // desejo em aberto incomoda
 return Cl(v,0,100);
}
static void Note(World *w,u8 ci,u16 title,int k,u8 dw){
 if(!w->c[ci].player)return;
 char t[96];int n=0;const char *p=Str(Soc_StratStr((u8)k));
 while(*p&&n<30)t[n++]=*p++;
 t[n++]=':';t[n++]=' ';p=Str(Soc_WantStr(dw));
 while(*p&&n<94)t[n++]=*p++;
 t[n]=0;World_Msg(w,Str(title),t);
}
static void Wants(World *w,u8 ci){
 WSoc *s=&w->soc[ci];u8 cu=Soc_Culture(w,ci);
 u32 wk=w->day/7;
 for(int k=0;k<SS_COUNT;k++){
  if(s->mood[k]>0)s->mood[k]--;else if(s->mood[k]<0&&(wk&1))s->mood[k]++;
  if(s->want[k]){
   if(Soc_WantOk(w,ci,s->want[k])){
    s->mood[k]=12;w->c[ci].prestige+=2;Note(w,ci,STR_SS_ATENDIDO,k,s->want[k]);s->want[k]=0;
   }else if(!s->wantT[k]||!--s->wantT[k]){
    s->mood[k]=-10;Note(w,ci,STR_SS_IGNORADO,k,s->want[k]);s->want[k]=0;
    if(k==SS_POBRE){int m=w->c[ci].militancy+4;w->c[ci].militancy=(u8)(m>100?100:m);}
   }
   continue;
  }
  if((wk+ci*3u+(u32)k*4u)%10u)continue;
  // sorteio fixo (hash): o primeiro desejo da cultura que ainda falta
  u32 h=Hs(ci*31u+(u32)k,wk);
  for(int t=0;t<4;t++){
   u8 dw=WANTS[cu][k][(h+t)&3];
   if(k==SS_RICA&&dw==DW_JUROS_ALTOS&&s->bourg>50)dw=DW_JUROS_BAIXOS;   // a burguesia quer credito
   if(k==SS_RICA&&dw==DW_LUXO&&s->bourg>60&&((h>>8)&1))dw=DW_FERROVIA;
   if(!Avail(w,dw)||Soc_WantOk(w,ci,dw))continue;
   s->want[k]=dw;s->wantT[k]=16;
   if(dw==DW_GLORIA)s->pres0=w->c[ci].prestige;
   Note(w,ci,STR_SS_DESEJA,k,dw);
   break;
  }
 }
}
// ---- a burguesia oferece financiar uma obra publica ----
static const u8 PUB[]={BK_FERROVIA,BK_ESCOLA,BK_HOSPITAL,BK_UNIVERSIDADE,BK_PORTO,BK_TEATRO,BK_PARQUE};
static u8 LobbyDemand(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];
 u32 h=Hs(ci,w->day/7);
 u8 c[LB_COUNT];int n=0;
 if(e->law[LW_IMPOSTO]>=2)c[n++]=LB_IMPOSTO;
 if(e->bank>=4)c[n++]=LB_JUROS;
 if(e->law[LW_TRABALHO]>=3)c[n++]=LB_TRABALHO;
 if(w->date.y<1900?e->law[LW_COMERCIO]<3:e->law[LW_COMERCIO]>1)c[n++]=LB_COMERCIO;
 c[n++]=LB_CONCESSAO;
 return c[h%(u32)n];
}
static void Lobby(World *w,u8 ci){
 WSoc *s=&w->soc[ci];WEco *e=&w->eco[ci];WCountry *C=&w->c[ci];
 if(s->lobbyCd){s->lobbyCd--;return;}
 s->lobbyCd=(u8)(26+Hs(ci,w->day)%27u);
 if(s->bourg<20||w->prop.kind)return;
 // a maior cidade que aceita uma obra publica da lista
 int bi=-1;u8 bk=0;u16 bp=0;
 for(int i=0;i<w->nCity;i++){
  const WCity *c=&w->city[i];
  if(c->owner!=ci||c->bq||c->pop<=bp)continue;
  for(unsigned j=0;j<sizeof PUB;j++){u8 k=PUB[(j+w->day/7)%sizeof PUB];if(City_CanBuild(w,i,k)){bi=i;bk=k;bp=c->pop;break;}}
 }
 if(bi<0)return;
 s32 cost=City_Cost(w,bi,bk);
 if(e->invest<cost/2)return;
 u8 lb=LobbyDemand(w,ci);
 if(C->player){WDip_Propose(w,PR_BURGUES,ci,bk,cost,(u16)(bi|(lb<<10)));return;}
 // IA: aceita se a ideologia nao e' contra o capital e a exigencia nao doi demais
 if(C->ideo==ID_COMUNISTA||Hs(ci,w->day)%100u>=70u)return;
 WProp p={PR_BURGUES,ci,bk,(u16)(bi|(lb<<10)),cost,w->day};
 Soc_LobbyAnswer(w,ci,&p,true);
}
static void Law1(World *w,u8 ci,u8 cat,int to){
 WEco *e=&w->eco[ci];
 if(to<0||to>=LW_OPTS||e->law[cat]==to)return;
 if(e->lawCat==cat)Law_Cancel(w,ci);
 e->law[cat]=(u8)to;Law_Apply(e);
}
void Soc_LobbyAnswer(World *w,u8 me,const WProp *p,bool yes){
 WSoc *s=&w->soc[me];WEco *e=&w->eco[me];
 int city=p->idx&1023;u8 lb=(u8)(p->idx>>10),k=p->about;
 if(!yes){
  s->mood[SS_RICA]=(s8)Mx(-20,s->mood[SS_RICA]-6);
  e->invest-=e->invest/10;                            // o capital procura outro pais
  return;
 }
 if(city>=w->nCity||w->city[city].owner!=me||!City_Grant(w,city,k))return;
 s32 cost=p->gold;
 e->invest=Mx(0,e->invest-cost/2);
 s->lobbyTot+=cost;w->nLobby++;
 s->mood[SS_RICA]=(s8)Mn(20,s->mood[SS_RICA]+8);
 switch(lb){
  case LB_IMPOSTO:Law1(w,me,LW_IMPOSTO,e->law[LW_IMPOSTO]-1);break;
  case LB_JUROS:e->bank=(u8)(e->bank>2?e->bank-2:0);break;
  case LB_TRABALHO:Law1(w,me,LW_TRABALHO,e->law[LW_TRABALHO]-1);
   s->mood[SS_POBRE]=(s8)Mx(-20,s->mood[SS_POBRE]-6);break;
  case LB_COMERCIO:Law1(w,me,LW_COMERCIO,w->date.y<1900?3:1);break;
  default:e->concPct=15;e->concW=104;break;           // 15% da renda urbana por 2 anos
 }
}
void Soc_Week(World *w,u8 ci){
 WSoc *s=&w->soc[ci];WCountry *c=&w->c[ci];WEco *e=&w->eco[ci];
 int tr,tm,tb;Target(w,ci,&tr,&tm,&tb);
 // as fatias andam ~5% do caminho por semana (uma geracao muda o pais)
 int r=s->share[SS_RICA],m=s->share[SS_MEDIA];
 r+=(tr-r)/20+(tr>r?1:(tr<r?-1:0));m+=(tm-m)/20+(tm>m?1:(tm<m?-1:0));
 r=Cl(r,5,80);m=Cl(m,20,700);
 s->share[SS_RICA]=(u16)r;s->share[SS_MEDIA]=(u16)m;s->share[SS_POBRE]=(u16)(1000-r-m);
 {int b=s->bourg;b+=(tb-b)/20+(tb>b?1:(tb<b?-1:0));s->bourg=(u8)Cl(b,0,95);}
 // renda relativa (x100 da media): ricos ficam com ~35% do PIB, medios ~35%,
 // pobres com o resto (lei trabalhista e saude puxam p/ os pobres)
 {int pp=26+e->law[LW_TRABALHO]*3+e->law[LW_SAUDE]*2,rp=38-e->law[LW_TRABALHO]*2-e->law[LW_IMPOSTO];
  if(c->ideo==ID_COMUNISTA){pp+=15;rp=8;}
  int mp=100-pp-rp;
  const int P[SS_COUNT]={pp,mp,rp};
  for(int k=0;k<SS_COUNT;k++)s->inc[k]=(u16)Mn(60000,P[k]*1000/Mx(1,s->share[k]));}
 Wants(w,ci);
 for(int k=0;k<SS_COUNT;k++){int t=Base(w,ci,k),v=s->sat[k];v+=(t-v)/4+(t>v?1:(t<v?-1:0));s->sat[k]=(u8)Cl(v,0,100);}
 // golpe: ricos furiosos e o povo nao protege o governo
 if(s->sat[SS_RICA]<12&&c->ideo!=ID_AUTORITARIO&&!c->unrestT&&s->sat[SS_POBRE]<55&&World_RngRange(w,150)==0){
  WDip_SetIdeo(w,ci,ID_AUTORITARIO,true);s->mood[SS_RICA]=15;w->nCoups++;
  if(c->player||c->cells>=150){
   char t[96];int n=0;const char *p=NATIONS[c->nat].name;while(*p&&n<40)t[n++]=*p++;
   p=Str(STR_SS_GOLPE2);while(*p&&n<94)t[n++]=*p++;t[n]=0;World_Msg(w,Str(STR_SS_GOLPE),t);}
 }
 // concessao em curso: a parte da renda urbana sai do caixa
 if(e->concW){e->concW--;if(!e->concW)e->concPct=0;}
 Lobby(w,ci);
}
// felicidade do pais: cada estrato pesa pelo tamanho x poder (rico pesa 12x, medio 3x)
int Soc_HappyFx(const World *w,u8 ci){
 const WSoc *s=&w->soc[ci];
 s32 wt=0,v=0;static const u8 PW[SS_COUNT]={1,3,12};
 for(int k=0;k<SS_COUNT;k++){s32 q=(s32)s->share[k]*PW[k];wt+=q;v+=q*((int)s->sat[k]-50);}
 return wt?Cl((int)(v/wt)/3,-16,16):0;
}
int Soc_MilitFx(const World *w,u8 ci){int p=w->soc[ci].sat[SS_POBRE];return p<45?(45-p)/4+(p<20?3:0):0;}
int Soc_InvestPct(const World *w,u8 ci){
 const WSoc *s=&w->soc[ci];
 return Cl((60+s->sat[SS_RICA])*(30+s->bourg)/80,10,220);
}
int Soc_ProdPct(const World *w,u8 ci){return Cl(((int)w->soc[ci].sat[SS_MEDIA]-50)/5,-8,8);}
