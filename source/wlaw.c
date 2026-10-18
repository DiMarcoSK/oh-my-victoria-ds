#include "wlaw.h"
#include "wecon.h"
#include "wdiplo.h"
#include "str.h"
#include "wplan.h"
// wlaw.c — leis (ver wlaw.h; SDD ADR-40). Tabela de efeitos: proposta de
// design com fundamento historico; balancear aqui. Unidades dos campos:
//  REV/PROD/INV/TRADE/CAP/REGEN/ARMYX/ATK/DEBT/EXPORT/COL = % (soma sobre 100)
//  HAPPY/MILIT = pontos no alvo de felicidade/militancia
//  GROW = por mil ao ano no crescimento da populacao; LIT = por mil do vao
//   ate' o teto de alfabetizacao fechado por ano
//  TARIFF = aliquota da alfandega (1..4; so' Comercio define)
//  EDU/SOC = gasto por mil habitantes x mil por semana (educacao / social)
//  INFL = inflacao por mil ao ano; RATE = pontos nos juros; STATE = % da
//  receita que o Estado investe em obras (vira despesa e fundo de obras)

#define F(...) {__VA_ARGS__}
static const s8 FX[LW_COUNT][LW_OPTS][LFX_COUNT]={
 [LW_IMPOSTO]={
  F([LFX_REV]=-50,[LFX_INV]=40,[LFX_HAPPY]=5),                       // minimos
  F([LFX_REV]=-25,[LFX_INV]=20,[LFX_HAPPY]=2),                       // baixos
  F(0),                                                              // moderados
  F([LFX_REV]=30,[LFX_INV]=-20,[LFX_HAPPY]=-5,[LFX_MILIT]=2),        // altos
  F([LFX_REV]=60,[LFX_INV]=-50,[LFX_HAPPY]=-12,[LFX_MILIT]=6,[LFX_PROD]=-5)}, // confiscatorios
 [LW_COMERCIO]={
  F([LFX_TRADE]=25,[LFX_TARIFF]=1,[LFX_EXPORT]=10,[LFX_HAPPY]=2,[LFX_PROD]=-2), // livre comercio
  F([LFX_TRADE]=10,[LFX_TARIFF]=2),                                  // tarifa baixa
  F([LFX_TARIFF]=2),                                                 // moderada
  F([LFX_TRADE]=-20,[LFX_TARIFF]=3,[LFX_PROD]=4),                    // protecionismo
  F([LFX_TRADE]=-60,[LFX_TARIFF]=1,[LFX_PROD]=7,[LFX_HAPPY]=-4,[LFX_EXPORT]=-40)}, // autarquia
 [LW_EDUCACAO]={
  F(0),                                                              // nenhuma
  F([LFX_LIT]=4,[LFX_EDU]=1,[LFX_HAPPY]=1),                          // religiosa
  F([LFX_LIT]=7,[LFX_EDU]=2),                                        // basica
  F([LFX_LIT]=13,[LFX_EDU]=4,[LFX_PROD]=2),                          // publica (1850)
  F([LFX_LIT]=20,[LFX_EDU]=6,[LFX_PROD]=4)},                        // universal (1880)
 [LW_EXERCITO]={
  F([LFX_ARMYX]=-40,[LFX_REGEN]=-40,[LFX_CAP]=-25,[LFX_HAPPY]=3),    // voluntario
  F([LFX_ARMYX]=10,[LFX_REGEN]=-15,[LFX_CAP]=-10,[LFX_ATK]=15),      // profissional
  F(0),                                                              // recrutamento seletivo
  F([LFX_ARMYX]=30,[LFX_REGEN]=40,[LFX_CAP]=30,[LFX_HAPPY]=-4,[LFX_PROD]=-3), // conscricao
  F([LFX_ARMYX]=60,[LFX_REGEN]=80,[LFX_CAP]=70,[LFX_HAPPY]=-10,[LFX_PROD]=-12,[LFX_MILIT]=4)}, // mobilizacao total
 [LW_TRABALHO]={
  F([LFX_PROD]=4,[LFX_HAPPY]=-10,[LFX_MILIT]=8,[LFX_GROW]=-1),       // servidao
  F([LFX_PROD]=6,[LFX_HAPPY]=-5,[LFX_INV]=15,[LFX_MILIT]=3),         // sem regulacao
  F([LFX_PROD]=1,[LFX_HAPPY]=2),                         // jornada regulada (1840)
  F([LFX_PROD]=-3,[LFX_HAPPY]=6,[LFX_MILIT]=-5,[LFX_INV]=-10),       // sindicatos (1870)
  F([LFX_PROD]=-5,[LFX_HAPPY]=11,[LFX_MILIT]=-9,[LFX_SOC]=3,[LFX_GROW]=1)}, // bem-estar (1910)
 [LW_MODELO]={
  F([LFX_EXPORT]=25,[LFX_TRADE]=-10,[LFX_REV]=10,[LFX_INV]=-20),     // mercantilismo
  F([LFX_INV]=80,[LFX_REV]=-10,[LFX_HAPPY]=-2),                      // laissez-faire
  F([LFX_INV]=20,[LFX_PROD]=3,[LFX_STATE]=3),                        // economia mista (1870)
  F([LFX_INV]=-30,[LFX_PROD]=5,[LFX_STATE]=8),                       // dirigismo (1900)
  F([LFX_INV]=-100,[LFX_PROD]=-4,[LFX_STATE]=20,[LFX_MILIT]=-4,[LFX_HAPPY]=-3,[LFX_EXPORT]=-20)}, // planificacao (1917)
 [LW_SAUDE]={
  F([LFX_GROW]=-1),                                                  // nenhuma
  F([LFX_HAPPY]=1),                                                  // caridade religiosa
  F([LFX_GROW]=1,[LFX_SOC]=1,[LFX_HAPPY]=2),                         // hospitais publicos (1850)
  F([LFX_GROW]=1,[LFX_SOC]=2,[LFX_HAPPY]=5,[LFX_MILIT]=-3),          // seguro social (1883)
  F([LFX_GROW]=2,[LFX_SOC]=4,[LFX_HAPPY]=8,[LFX_PROD]=3)},           // saude universal (1948)
 [LW_MOEDA]={
  F([LFX_DEBT]=-30,[LFX_RATE]=-1,[LFX_INV]=5),                       // padrao-ouro
  F([LFX_INFL]=4,[LFX_DEBT]=-10),                                    // bimetalismo
  F([LFX_INFL]=20,[LFX_DEBT]=20,[LFX_INV]=10,[LFX_HAPPY]=-1),        // papel-moeda
  F([LFX_INFL]=15,[LFX_DEBT]=40,[LFX_RATE]=-2,[LFX_INV]=15),         // banco central (1913)
  F([LFX_INFL]=60,[LFX_HAPPY]=-2)},                                  // emissao livre (deficit = maquina de imprimir)
 [LW_IMIGRACAO]={
  F([LFX_GROW]=-2,[LFX_HAPPY]=2,[LFX_PROD]=-2),                      // fronteiras fechadas
  F([LFX_GROW]=-1,[LFX_HAPPY]=1),                                    // restrita
  F(0),                                                              // moderada
  F([LFX_GROW]=4,[LFX_PROD]=3,[LFX_HAPPY]=-2,[LFX_LIT]=-2),          // portas abertas
  F([LFX_GROW]=6,[LFX_COL]=30,[LFX_SOC]=1,[LFX_HAPPY]=-1)},          // colonizacao incentivada
};
#undef F
static const u16 YEAR[LW_COUNT][LW_OPTS]={
 [LW_EDUCACAO]={0,0,0,1850,1880},[LW_TRABALHO]={0,0,1840,1870,1910},[LW_MODELO]={0,0,1870,1900,1917},
 [LW_SAUDE]={0,0,1850,1883,1948},[LW_MOEDA]={0,0,0,1913,0}};
static const u16 CAT_STR[LW_COUNT]={STR_LC_IMPOSTO,STR_LC_COMERCIO,STR_LC_EDUCACAO,STR_LC_EXERCITO,
 STR_LC_TRABALHO,STR_LC_MODELO,STR_LC_SAUDE,STR_LC_MOEDA,STR_LC_IMIGRACAO};
u16 Law_CatStr(u8 cat){return CAT_STR[cat%LW_COUNT];}
u16 Law_Str(u8 cat,u8 opt){return (u16)(STR_LAW_0+(cat%LW_COUNT)*LW_OPTS+opt%LW_OPTS);}
u16 Law_Year(u8 cat,u8 opt){return YEAR[cat%LW_COUNT][opt%LW_OPTS];}
s8 Law_FxOf(u8 cat,u8 opt,u8 f){return FX[cat%LW_COUNT][opt%LW_OPTS][f%LFX_COUNT];}
u16 Law_FxStr(u8 f){return (u16)(STR_LFX_0+f%LFX_COUNT);}
bool Law_FxBad(u8 f,int v){
 switch(f){case LFX_MILIT:case LFX_ARMYX:case LFX_EDU:case LFX_SOC:case LFX_INFL:case LFX_RATE:case LFX_STATE:return v>0;
  case LFX_TARIFF:return false;
  default:return v<0;}
}
void Law_Apply(WEco *e){
 for(int f=0;f<LFX_COUNT;f++){int s=0;for(int c=0;c<LW_COUNT;c++)s+=FX[c][e->law[c]%LW_OPTS][f];e->fx.v[f]=(s16)s;}
 Plan_Fix(e);                 // a doutrina (LW_MODELO) define quantas politicas por bem cabem (ADR-47)
}
// preferencia das ideologias (DESIGN 6.5.1): a lei "deles" alegra, a do outro extremo irrita
static const u8 PREF[ID_COUNT][LW_COUNT]={
 [ID_TRADICIONAL]={1,3,1,2,0,0,1,0,1},
 [ID_LIBERAL]    ={1,0,3,1,2,1,2,0,3},
 [ID_CONSERVADOR]={2,3,1,2,1,0,1,0,1},
 [ID_COMUNISTA]  ={3,4,4,3,4,4,4,3,2},
 [ID_AUTORITARIO]={3,3,2,4,1,3,2,3,0},
 [ID_TEOCRATICO] ={2,3,1,2,1,0,1,1,0}};
u8 Law_Pref(u8 ideo,u8 cat){return ideo<ID_COUNT&&cat<LW_COUNT?PREF[ideo][cat]:0xFF;}
int Law_Approval(const World *w,u8 ci){
 const WEco *e=&w->eco[ci];u8 id=w->c[ci].ideo;int s=0;
 for(int c=0;c<LW_COUNT;c++){u8 p=Law_Pref(id,(u8)c);if(p>=LW_OPTS)continue;
  int d=e->law[c]-p;if(d<0)d=-d;s+=d==0?1:(d>=3?-1:0);}
 return s>6?6:(s<-6?-6:s);
}
int Law_Days(const WEco *e,u8 cat,u8 opt){int d=opt-e->law[cat];if(d<0)d=-d;return LW_DEBATE0+LW_DEBATE_STEP*d;}
int Law_Prestige(const WEco *e,u8 cat,u8 opt){int d=opt-e->law[cat];if(d<0)d=-d;return LW_PREST_STEP*d;}
u16 Law_Why(const World *w,u8 ci,u8 cat,u8 opt){
 const WEco *e=&w->eco[ci];
 if(cat>=LW_COUNT||opt>=LW_OPTS)return STR_VAZIO;
 if(e->law[cat]==opt)return STR_LEI_VIGOR;
 if(w->date.y<Law_Year(cat,opt))return STR_REQ_ANO;
 if(e->lawCat<LW_COUNT)return STR_LEI_DEBATE_OUTRA;
 if(w->c[ci].prestige<Law_Prestige(e,cat,opt))return STR_LEI_SEM_PREST;
 return 0;
}
static void Set(World *w,u8 ci,u8 cat,u8 opt){WEco *e=&w->eco[ci];e->law[cat]=opt;Law_Apply(e);}
bool Law_Propose(World *w,u8 ci,u8 cat,u8 opt){
 if(Law_Why(w,ci,cat,opt))return false;
 WEco *e=&w->eco[ci];
 if(!w->c[ci].player){Set(w,ci,cat,opt);return true;}
 w->c[ci].prestige-=Law_Prestige(e,cat,opt);
 e->lawCat=cat;e->lawTo=opt;e->lawDays=(u16)Law_Days(e,cat,opt);
 return true;
}
void Law_Cancel(World *w,u8 ci){w->eco[ci].lawCat=0xFF;w->eco[ci].lawDays=0;}
void Law_Day(World *w){
 for(u8 ci=1;ci<w->nC;ci++){
  WEco *e=&w->eco[ci];
  if(e->lawCat>=LW_COUNT)continue;
  if(!w->c[ci].alive){Law_Cancel(w,ci);continue;}
  if(e->lawDays&&--e->lawDays)continue;
  u8 cat=e->lawCat,opt=e->lawTo;
  e->lawCat=0xFF;
  Set(w,ci,cat,opt);
  if(w->c[ci].player)World_Msg(w,Str(STR_LEI_APROVADA),Str(Law_Str(cat,opt)));
 }
}
// ---- IA: alvo por assunto (arquetipo, ideologia, era, caixa) ----
static u8 Target(const World *w,u8 ci,u8 cat){
 const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
 int y=w->date.y;u8 id=c->ideo,cur=e->law[cat];
 s32 bal=WEco_GoldWeek(w,ci)+e->xCorrupt,rev=WEco_Revenue(w,ci),lim=WEco_DebtLimit(w,ci);
 bool broke=bal<0||e->debt>lim/2,rich=bal>rev/4&&e->debt==0;
 const NationDef *nd=&NATIONS[c->nat];
 switch(cat){
  case LW_IMPOSTO:{
   int t=cur<1?1:(cur>3?3:cur);
   if(broke&&WDip_RevoltRisk(w,ci)<40)t++;
   else if(rich)t--;
   if(WDip_RevoltRisk(w,ci)>50)t--;
   if(e->debt>lim*4/5&&t<3)t=3;
   return (u8)(t<0?0:(t>4?4:t));}
  case LW_COMERCIO:
   if(c->archetype==WA_MERCADOR||c->m.tradePct>=10)return 0;
   if(c->archetype==WA_ISOLACIONISTA)return c->m.isolated?4:3;
   if(id==ID_LIBERAL)return 1;
   if(id==ID_COMUNISTA)return 4;
   return y<1860?3:2;
  case LW_EDUCACAO:{
   int t=y<1830?1:(y<1850?2:(y<1880?3:4));
   if(broke&&t>1)t--;
   if(id==ID_TEOCRATICO&&t>1)t=1;
   return (u8)t;}
  case LW_EXERCITO:
   if(c->warN)return c->warDays>540&&c->archetype==WA_EXPANSIONISTA?4:3;
   if(id==ID_AUTORITARIO)return 3;
   if(c->archetype==WA_EXPANSIONISTA)return 2;
   if(c->archetype==WA_MERCADOR||c->archetype==WA_DIPLOMATA)return 1;
   return 2;
  case LW_TRABALHO:
   if(id==ID_COMUNISTA)return y>=1910?4:3;
   if(y>=1950&&e->life>65)return 4;
   if(y>=1910&&(id==ID_LIBERAL||id==ID_CONSERVADOR))return 3;
   if(y>=1870)return 2;
   if(y>=1840)return id==ID_TRADICIONAL?1:2;
   return id==ID_TRADICIONAL?0:1;
  case LW_MODELO:
   if(id==ID_COMUNISTA&&y>=1917)return 4;
   if(id==ID_AUTORITARIO&&y>=1900)return 3;
   if(id==ID_LIBERAL)return 1;
   if(id==ID_TRADICIONAL||id==ID_TEOCRATICO)return y<1870?0:2;
   return y<1870?0:2;
  case LW_SAUDE:{
   int t=y<1850?1:(y<1883?2:(y<1948?3:4));
   if(broke&&t>1)t--;
   return (u8)t;}
  case LW_MOEDA:
   if(c->archetype==WA_REVOLUCIONARIO&&e->debt>lim*4/5)return 4;
   if(y<1870)return 1;
   if(y<1914)return 0;
   if(y<1933)return 3;
   return 3;
  default:
   if(c->archetype==WA_ISOLACIONISTA)return 0;
   if((nd->cont==CONT_AMERICAS||nd->cont==CONT_OCEANIA)&&nd->kind==NK_ESTADO)return y<1920?4:3;
   return y<1900?2:(id==ID_LIBERAL?3:2);
 }
}
void Law_Ai(World *w,u8 ci){
 WEco *e=&w->eco[ci];bool ch=false;
 for(u8 cat=0;cat<LW_COUNT;cat++){
  u8 t=Target(w,ci,cat),cur=e->law[cat];
  if(t==cur)continue;
  u8 nx=(u8)(t>cur?cur+1:cur-1);
  while(nx!=cur&&w->date.y<Law_Year(cat,nx))nx=(u8)(nx>cur?nx-1:nx+1);   // pula o que ainda nao existe
  if(nx!=cur){e->law[cat]=nx;ch=true;}
 }
 if(ch)Law_Apply(e);
}
void Law_Init(World *w,u8 ci){
 WEco *e=&w->eco[ci];
 static const u8 D[LW_COUNT]={2,2,1,2,1,0,1,1,2};
 for(int c=0;c<LW_COUNT;c++)e->law[c]=D[c];
 e->lawCat=0xFF;e->lawDays=0;
 if(ci&&ci<w->nC&&w->c[ci].alive)
  for(u8 cat=0;cat<LW_COUNT;cat++){
   if(cat==LW_IMPOSTO)continue;     // o caixa ainda nao fechou semana nenhuma
   u8 t=Target(w,ci,cat);
   if(w->date.y>=Law_Year(cat,t))e->law[cat]=t;
  }
 Law_Apply(e);
}
