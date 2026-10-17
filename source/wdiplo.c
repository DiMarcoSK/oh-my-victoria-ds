#include "wdiplo.h"
#include "hot.h"
#include "wecon.h"
#include "city.h"
#include "str.h"
#include "calendar.h"
// wdiplo.c — ideologia, diplomacia da IA, propostas, marinha e ambicoes.
// Ver wdiplo.h. Usa o RNG do mundo (w->rng): mesma semente = mesma historia.

static inline int Iabs(int v){return v<0?-v:v;}
static inline s32 Mn(s32 a,s32 b){return a<b?a:b;}
static int CapDist(const World *w,u8 a,u8 b){
 int dx=Iabs((int)w->c[a].capX-(int)w->c[b].capX);
 if(w->wrap&&dx>w->w/2)dx=w->w-dx;
 return dx+Iabs((int)w->c[a].capY-(int)w->c[b].capY);
}
static bool Live(const World *w,u8 c){return c&&c<w->nC&&w->c[c].alive;}
static void Msg2(World *w,u16 titleStr,const char *a,u16 midStr,const char *b){
 char t[128];int n=0;
 const char *p=Str(titleStr);while(*p&&n<60)t[n++]=*p++;
 t[n]=0;
 char body[128];int k=0;
 p=a;while(p&&*p&&k<60)body[k++]=*p++;
 p=Str(midStr);while(*p&&k<110)body[k++]=*p++;
 p=b;while(p&&*p&&k<126)body[k++]=*p++;
 body[k]=0;
 World_Msg(w,t,body);
}
static void Itoa(int v,char *o){char t[12];int n=0;bool neg=v<0;if(neg)v=-v;do{t[n++]=(char)('0'+v%10);v/=10;}while(v&&n<11);
 int k=0;if(neg)o[k++]='-';while(n)o[k++]=t[--n];o[k]=0;}
static const char *NN(const World *w,u8 c){return c?NATIONS[w->c[c].nat].name:"";}
static bool Player(const World *w,u8 c){return Live(w,c)&&w->c[c].player;}

// ===================== ideologia =====================
// modificadores somados aos traits (World_SetTraits): ataque, defesa, renda,
// comercio, producao, teto do exercito, felicidade, militancia
static const s8 IDM[ID_COUNT][8]={
 /*TRAD*/{ 0, 5,  0,  0,  0,  0,  5,  0},
 /*LIB */{ 0, 0, 10, 20,  5,  0,  5, -5},
 /*CONS*/{ 0,10,  5,  5,  0,  5,  0, -5},
 /*COM */{ 5,10,-10,-30, 25, 15,-10,  5},
 /*AUT */{20,10,  0,-10, 10, 20,-15, 10},
 /*TEO */{ 0,15, -5,-10,-10,  5, 10,  0},
};
void WDip_IdeoMods(u8 ideo,TraitMods *m){
 if(ideo>=ID_COUNT)return;
 const s8 *d=IDM[ideo];
 m->atkPct+=d[0];m->defPct+=d[1];m->goldPct+=d[2];m->tradePct+=d[3];
 m->prodPct+=d[4];m->capPct+=d[5];m->happy=(s8)(m->happy+d[6]);m->militancy=(s8)(m->militancy+d[7]);
}
u8 WDip_IdeoOfGov(u8 gov){
 switch(gov){case NG_REPUBLICA:return ID_LIBERAL;case NG_TEOCRACIA:return ID_TEOCRATICO;
  case NG_COMPANHIA:return ID_CONSERVADOR;default:return ID_TRADICIONAL;}
}
static bool Islamic(u8 nat){
 const NationDef *d=&NATIONS[nat];
 if(d->flag.emb==FE_CRESCENTE)return true;
 return d->scene==SC_AFR_NORTE||d->scene==SC_ORIENTE_MEDIO||(d->tag[0]=='O'&&d->tag[1]=='T'&&d->tag[2]=='T');
}
static bool Dark(u8 c){return c==FC_NAVY||c==FC_BLACK||c==FC_DKGREEN||c==FC_PURPLE||c==FC_BROWN||c==FC_CRIMSON||c==FC_BLUE||c==FC_RED||c==FC_GREEN;}
// cor de fundo no centro do pano (onde o emblema cai)
static u8 CenterBg(const FlagDef *f){
 switch(f->pat){case FP_TRI_V:case FP_TRI_H:case FP_NORDICA:case FP_CRUZ:return f->c1;
  case FP_BORDA:return f->c1;default:return f->c0;}
}
void WDip_FlagIdeo(const FlagDef *base,u8 from,u8 to,bool islam,FlagDef *o){
 *o=*base;
 if(to==from||to>=ID_COUNT)return;
 u8 *cs[3]={&o->c0,&o->c1,&o->c2};
 switch(to){
 case ID_COMUNISTA:
  // a bandeira de base em vermelho, com a estrela dourada
  if(o->pat==FP_UNION){o->pat=FP_CANTAO;o->c0=FC_RED;o->c1=FC_CRIMSON;o->c2=FC_RED;}
  for(int i=0;i<3;i++){u8 c=*cs[i];
   if(c==FC_NAVY||c==FC_BLUE||c==FC_SKY||c==FC_GREEN||c==FC_DKGREEN||c==FC_PURPLE)*cs[i]=FC_RED;
   else if(c==FC_BLACK||c==FC_BROWN)*cs[i]=FC_CRIMSON;}
  o->emb=FE_ESTRELA;o->ec=FC_YELLOW;
  if(o->pat==FP_LISA)o->pat=FP_CANTAO,o->c1=o->c0;
  if(CenterBg(o)==FC_YELLOW||CenterBg(o)==FC_GOLD)o->ec=FC_RED;
  break;
 case ID_AUTORITARIO:
  // cores frias viram preto; emblema de aguia
  if(o->pat==FP_UNION){o->pat=FP_TRI_H;o->c0=FC_BLACK;o->c1=FC_WHITE;o->c2=FC_RED;}
  for(int i=0;i<3;i++){u8 c=*cs[i];
   if(c==FC_NAVY||c==FC_BLUE||c==FC_SKY)*cs[i]=FC_BLACK;
   else if(c==FC_GREEN)*cs[i]=FC_DKGREEN;}
  o->emb=FE_AGUIA;
  o->ec=Dark(CenterBg(o))?FC_WHITE:FC_BLACK;
  break;
 case ID_LIBERAL:
  if(o->emb==FE_COROA){o->emb=FE_ESCUDO;}
  if(o->pat==FP_LISA&&o->emb==FE_NONE){o->pat=FP_BI_V;o->c1=FC_WHITE;}
  break;
 case ID_TRADICIONAL:
  if(o->emb==FE_NONE||o->emb==FE_ESCUDO||o->emb==FE_ESTRELA){o->emb=FE_COROA;o->ec=Dark(CenterBg(o))?FC_GOLD:FC_CRIMSON;}
  break;
 case ID_TEOCRATICO:
  if(islam){o->c0=FC_GREEN;if(o->pat==FP_UNION)o->pat=FP_LISA;o->emb=FE_CRESCENTE;o->ec=FC_WHITE;}
  else{o->emb=FE_CRUZ;o->ec=Dark(CenterBg(o))?FC_GOLD:FC_CRIMSON;}
  break;
 default:break; // conservador: bandeira de base
 }
}
void WDip_Flag(const World *w,u8 ci,FlagDef *out){
 const WCountry *c=&w->c[ci];
 // bandeira historica do ano/regime (hflags.c); senao a do catalogo
 FlagDef h;int r=HFlag_Find(NATIONS[c->nat].tag,w->date.y,c->ideo,&h);
 if(r==2){*out=h;return;}
 WDip_FlagIdeo(r==1?&h:&NATIONS[c->nat].flag,c->ideo0,c->ideo,Islamic(c->nat),out);
}
static bool Opposed(u8 a,u8 b){
 if(a==b)return false;
 if(a==ID_COMUNISTA||b==ID_COMUNISTA)return true;          // comunismo x todos
 if((a==ID_AUTORITARIO&&b==ID_LIBERAL)||(b==ID_AUTORITARIO&&a==ID_LIBERAL))return true;
 return false;
}
// peso de cada ideologia por era (sorteio da revolucao)
static const u8 EW[ERA_COUNT][ID_COUNT]={
 {30,40,20, 0, 5, 5},{35,35,25, 0, 5, 5},{20,45,25, 5, 5, 3},{15,40,30,10, 5, 3},
 { 5,25,15,25,30, 3},{ 3,35,20,30,15, 5},{ 2,55,25, 8, 8, 6},{ 5,40,20,15,10,10}};
u8 WDip_RandomIdeo(World *w,u8 ci){
 const WCountry *c=&w->c[ci];
 int wt[ID_COUNT];int era=Cal_Era(w->date.y);
 for(int i=0;i<ID_COUNT;i++)wt[i]=EW[era][i];
 // contagio: a ideologia do lider do bloco e dos aliados pesa mais
 if(Live(w,c->bloc)&&c->bloc!=ci)wt[w->c[c->bloc].ideo]+=25;
 for(int k=0;k<2;k++)if(Live(w,c->ally[k]))wt[w->c[c->ally[k]].ideo]+=15;
 for(int i=0;i<NAT_TRAITS;i++){
  if(c->traits[i]==TR_REVOLUCAO){wt[ID_LIBERAL]+=15;wt[ID_COMUNISTA]+=10;}
  if(c->traits[i]==TR_FE)wt[ID_TEOCRATICO]+=10;
  if(c->traits[i]==TR_ABSOLUTISMO)wt[ID_TRADICIONAL]+=10;
 }
 if(c->kind==NK_POVO||c->kind==NK_AUTONOMO){wt[ID_COMUNISTA]/=3;wt[ID_AUTORITARIO]/=3;wt[ID_TRADICIONAL]+=20;}
 wt[c->ideo]=0;
 int tot=0;for(int i=0;i<ID_COUNT;i++)tot+=wt[i];
 if(tot<=0)return c->ideo;
 int r=World_RngRange(w,tot);
 for(int i=0;i<ID_COUNT;i++){if(r<wt[i])return (u8)i;r-=wt[i];}
 return ID_LIBERAL;
}
void WDip_SetIdeo(World *w,u8 ci,u8 ideo,bool revolution){
 if(!Live(w,ci)||ideo>=ID_COUNT)return;
 WCountry *c=&w->c[ci];
 if(c->ideo==ideo&&!revolution)return;
 c->ideo=ideo;
 switch(ideo){
  case ID_TRADICIONAL:if(c->gov==NG_REPUBLICA)c->gov=NG_MONARQUIA;break;
  case ID_LIBERAL:case ID_COMUNISTA:if(c->gov==NG_MONARQUIA||c->gov==NG_TEOCRACIA)c->gov=NG_REPUBLICA;break;
  case ID_TEOCRATICO:c->gov=NG_TEOCRACIA;break;
  default:break;
 }
 World_SetTraits(w,ci);
 if(revolution){
  c->troops-=c->troops/4;c->gold-=c->gold/3;
  c->happy=55;c->militancy=25;c->unrestT=365;w->nRevol++;
 }
 // o mundo reage: aliancas e blocos de ideologia oposta se desfazem
 for(int k=0;k<2;k++){u8 a=c->ally[k];if(Live(w,a)&&Opposed(ideo,w->c[a].ideo))WDip_Unally(w,ci,a);}
 if(Live(w,c->bloc)&&c->bloc!=ci&&Opposed(ideo,w->c[c->bloc].ideo))c->bloc=0;
 if(c->bloc==ci)for(u8 j=1;j<w->nC;j++)if(j!=ci&&w->c[j].bloc==ci&&Opposed(ideo,w->c[j].ideo))w->c[j].bloc=0;
}
int WDip_RevoltRisk(const World *w,u8 ci){
 const WCountry *c=&w->c[ci];
 int r=(int)c->militancy-(int)c->happy/2+(60-(int)w->eco[ci].life)/2;
 int y=w->date.y;
 if((y>=1845&&y<=1852)||(y>=1910&&y<=1950))r+=8;
 // espirito da epoca: o velho regime envelhece; o partido unico cansa no fim do seculo XX
 if(c->ideo==ID_TRADICIONAL&&y>1848)r+=(y-1848)/8>25?25:(y-1848)/8;
 if((c->ideo==ID_COMUNISTA||c->ideo==ID_AUTORITARIO)&&y>=1975)r+=15+(y-1975)/3;
 if(c->ideo==ID_LIBERAL&&y>=1918&&y<=1939)r+=6;
 if(w->eco[ci].tax==2)r+=5;
 for(int i=0;i<NAT_TRAITS;i++)if(c->traits[i]==TR_REVOLUCAO)r+=8;
 if(c->ideo==ID_COMUNISTA||c->ideo==ID_AUTORITARIO)r-=10;   // repressao
 if(c->warDays>720)r+=c->warDays/180;
 if(c->unrestT)r-=20;
 return r<0?0:(r>100?100:r);
}
// nacao adormecida do catalogo cuja capital esta neste pais: separatismo
bool WDip_Separatism(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(!Live(w,ci)||c->cells<60)return false;
 int start=World_RngRange(w,NATION_COUNT);
 for(int k=0;k<NATION_COUNT;k++){
  int n=(start+k)%NATION_COUNT;
  if(n==c->nat||World_CountryOfNat(w,n))continue;
  const NationDef *d=&NATIONS[n];
  if(d->kind==NK_SUDITO||d->kind==NK_AUTONOMO)continue;
  int x,y;World_NatPos(w,n,&x,&y);
  u32 i=World_Idx(w,x,y);
  if(W_OWNER[i]!=ci||W_TERR[i]==WT_WATER)continue;
  if(c->capX==x&&c->capY==y)continue;
  if(W_CORE[i]==c->nat+1&&d->yFrom==0)continue;   // terra nuclear de quem governa
  u8 nc=World_Spawn(w,n,ci,x,y,d->size?d->size:4);
  if(!nc)continue;
  w->c[nc].ideo=w->c[nc].ideo0=WDip_IdeoOfGov(d->gov);
  w->c[nc].mood=(u8)World_RngRange(w,101);
  World_DeclareWar(w,ci,nc);
  Msg2(w,STR_D_SEPARA,d->name,STR_D_SEPARA2,NN(w,ci));
  w->nRevol++;
  return true;
 }
 return false;
}

// ===================== aliancas e blocos =====================
bool WDip_Allied(const World *w,u8 a,u8 b){
 if(!Live(w,a)||!Live(w,b))return false;
 return w->c[a].ally[0]==b||w->c[a].ally[1]==b;
}
int WDip_Allies(const World *w,u8 a){int n=0;for(int k=0;k<2;k++)if(Live(w,w->c[a].ally[k]))n++;return n;}
static bool AddAlly(WCountry *c,u8 b){
 for(int k=0;k<2;k++)if(c->ally[k]==b)return true;
 for(int k=0;k<2;k++)if(!c->ally[k]){c->ally[k]=b;return true;}
 return false;
}
void WDip_Ally(World *w,u8 a,u8 b){
 if(!Live(w,a)||!Live(w,b)||a==b||World_AtWar(w,a,b))return;
 if(WDip_Allies(w,a)>=2||WDip_Allies(w,b)>=2)return;
 AddAlly(&w->c[a],b);AddAlly(&w->c[b],a);
 if(w->c[a].rival==b)w->c[a].rival=0;
 if(w->c[b].rival==a)w->c[b].rival=0;
 w->nAlliances++;
}
void WDip_Unally(World *w,u8 a,u8 b){
 for(int k=0;k<2;k++){if(w->c[a].ally[k]==b)w->c[a].ally[k]=0;if(w->c[b].ally[k]==a)w->c[b].ally[k]=0;}
}
int WDip_BlocSize(const World *w,u8 leader){
 if(!Live(w,leader)||w->c[leader].bloc!=leader)return 0;
 int n=0;for(u8 j=1;j<w->nC;j++)if(w->c[j].alive&&w->c[j].bloc==leader)n++;
 return n;
}
bool WDip_FoundBloc(World *w,u8 ci){
 if(!Live(w,ci)||w->c[ci].bloc)return false;
 w->c[ci].bloc=ci;return true;
}
void WDip_LeaveBloc(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(c->bloc==ci){for(u8 j=1;j<w->nC;j++)if(w->c[j].bloc==ci)w->c[j].bloc=0;}
 c->bloc=0;
}
static bool BlocOk(const World *w,u8 ci,u8 leader){
 if(!Live(w,leader)||w->c[leader].bloc!=leader||leader==ci)return false;
 if(WDip_BlocSize(w,leader)>=12||World_AtWar(w,ci,leader))return false;
 if(!w->c[ci].player&&!w->c[leader].player&&CapDist(w,ci,leader)>160)return false; // mercado comum e' regional
 return !Opposed(w->c[ci].ideo,w->c[leader].ideo);
}
// pontuacao de uma alianca vista por a (IA)
static int AllyScore(const World *w,u8 a,u8 b){
 const WCountry *A=&w->c[a],*B=&w->c[b];
 if(World_AtWar(w,a,b)||A->rival==b||B->rival==a||WDip_Allies(w,b)>=2||WDip_Allied(w,a,b))return -999;
 if(A->overlord==b||B->overlord==a)return -999;
 int s=0;
 if(A->ideo==B->ideo)s+=30;
 if(Opposed(A->ideo,B->ideo))s-=50;
 if(A->rival&&B->rival==A->rival)s+=45;      // o inimigo do meu inimigo
 if(A->bloc&&A->bloc==B->bloc)s+=15;
 s+=Mn((s32)B->cells/40,30);
 s+=((int)A->mood+(int)B->mood)/10-10;
 if(A->archetype==WA_DIPLOMATA)s+=15;
 if(A->archetype==WA_ISOLACIONISTA)s-=40;
 s-=CapDist(w,a,b)/5;
 if(B->player)s+=B->opin/2;
 return s;
}
bool WDip_AskAlliance(World *w,u8 me,u8 to){
 if(!Live(w,me)||!Live(w,to)||me==to)return false;
 int s=AllyScore(w,to,me);
 bool ok=s>=35;
 if(ok)WDip_Ally(w,me,to);
 else if(w->c[to].opin>-90)w->c[to].opin-=5;
 return ok;
}
bool WDip_AskBloc(World *w,u8 me,u8 leader){
 if(!BlocOk(w,me,leader)||w->c[me].bloc)return false;
 const WCountry *L=&w->c[leader];
 int s=40+L->opin/2-CapDist(w,me,leader)/6+(L->ideo==w->c[me].ideo?15:0);
 if(L->archetype==WA_MERCADOR)s+=15;
 if(L->archetype==WA_ISOLACIONISTA)s-=40;
 if(s<30)return false;
 w->c[me].bloc=leader;return true;
}
bool WDip_InviteBloc(World *w,u8 me,u8 to){
 if(!Live(w,to)||w->c[to].bloc||to==me)return false;
 if(!w->c[me].bloc)WDip_FoundBloc(w,me);
 if(!BlocOk(w,to,me))return false;
 const WCountry *T=&w->c[to];
 int s=30+T->opin/2-CapDist(w,me,to)/6+(T->ideo==w->c[me].ideo?15:0)+Mn((s32)w->c[me].cells/30,25)-Mn((s32)T->cells/30,25);
 if(T->archetype==WA_MERCADOR)s+=10;
 if(T->archetype==WA_ISOLACIONISTA)s-=40;
 if(s<35)return false;
 w->c[to].bloc=me;return true;
}

// ===================== propostas ao jogador =====================
bool WDip_Propose(World *w,u8 kind,u8 from,u8 about,s32 gold,u16 idx){
 if(w->prop.kind)return false;
 w->prop.kind=kind;w->prop.from=from;w->prop.about=about;w->prop.gold=gold;w->prop.idx=idx;w->prop.day=w->day;
 return true;
}
void Rnd_Decide(World *w,u8 ci,int idx,bool optA);   // history.c
static u8 PlayerC(const World *w){for(u8 i=1;i<w->nC;i++)if(w->c[i].player&&w->c[i].alive)return i;return 0;}
void WDip_Answer(World *w,bool yes){
 WProp p=w->prop;w->prop.kind=PR_NONE;
 u8 me=PlayerC(w);if(!me)return;
 WCountry *F=Live(w,p.from)?&w->c[p.from]:0;
 switch(p.kind){
 case PR_ALIANCA:
  if(!F)break;
  if(yes){WDip_Ally(w,me,p.from);F->opin=(s8)Mn(100,F->opin+20);}
  else F->opin=(s8)(F->opin>-80?F->opin-10:F->opin);
  break;
 case PR_BLOCO:
  if(!F)break;
  if(yes){
   if(p.about==0){if(BlocOk(w,me,p.from)){WDip_LeaveBloc(w,me);w->c[me].bloc=p.from;}}
   else{if(!w->c[me].bloc)WDip_FoundBloc(w,me);if(BlocOk(w,p.from,me))F->bloc=me;}
   F->opin=(s8)Mn(100,F->opin+15);
  }else F->opin=(s8)(F->opin>-80?F->opin-5:F->opin);
  break;
 case PR_AJUDA:
  if(!F)break;
  if(yes&&Live(w,p.about)){World_DeclareWar(w,me,p.about);F->opin=(s8)Mn(100,F->opin+25);}
  else{F->opin=(s8)(F->opin>-75?F->opin-25:-100);if(World_RngRange(w,100)<40)WDip_Unally(w,me,p.from);}
  break;
 case PR_TRIBUTO:
  if(!F)break;
  if(yes&&w->c[me].gold>=p.gold){w->c[me].gold-=p.gold;F->gold+=p.gold;F->opin=(s8)Mn(100,F->opin+10);w->c[me].prestige-=5;}
  else{World_DeclareWarCall(w,p.from,me);F->warTarget=me;F->warGain=0;F->warGoal=(u16)(w->c[me].cells/4+4);}
  break;
 case PR_PAZ:
  if(F&&yes)World_Peace(w,me,p.from);
  break;
 case PR_DECISAO:
  Rnd_Decide(w,me,p.idx,yes);
  break;
 default:break;
 }
}

// ===================== guerra: chamada as armas =====================
void WDip_OnWar(World *w,u8 a,u8 b){
 for(int side=0;side<2;side++){
  u8 att=side?b:a,def=side?a:b;   // side 0: aliados do defensor; 1: do agressor
  const WCountry *D=&w->c[def];
  for(int k=0;k<2;k++){
   u8 x=D->ally[k];
   if(!Live(w,x)||World_AtWar(w,x,att)||WDip_Allied(w,x,att)||x==att)continue;
   if(w->c[x].player){if(side==0)WDip_Propose(w,PR_AJUDA,def,att,0,0);continue;}
   int ch=side==0?60:25;
   ch+=((int)w->c[x].mood-50)/2;
   if(World_RngRange(w,100)<ch){
    World_DeclareWar(w,x,att);
    if(Player(w,att)||Player(w,def))Msg2(w,STR_D_ENTRA,NN(w,x),STR_D_AO_LADO,NN(w,def));
   }
  }
 }
}

// ===================== ambicoes do jogador =====================
static const u16 OBS[OB_COUNT]={STR_OB_FUNDAR,STR_OB_PORTO,STR_OB_NAVIOS,STR_OB_FABRICA,STR_OB_METROPOLE,
 STR_OB_ALIADO,STR_OB_BLOCO,STR_OB_VIDA,STR_OB_COMERCIO,STR_OB_CIDADES,STR_OB_EXERCITO,STR_OB_MINA,
 STR_OB_FERROVIA,STR_OB_PRESTIGIO};
// ouro (>0) ou prestigio (<0)
static const s16 OBR[OB_COUNT]={150,120,-10,-15,-20,150,150,-10,250,-20,120,100,200,300};
u16 Obj_Str(u8 ob){return ob<OB_COUNT?OBS[ob]:STR_VAZIO;}
s32 Obj_Reward(u8 ob){return ob<OB_COUNT?OBR[ob]:0;}
static int CountBld(const World *w,u8 ci,u8 k){
 int n=0;
 for(int i=0;i<w->nCity;i++){const WCity *c=&w->city[i];if(c->owner!=ci)continue;
  for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==k)n++;}
 return n;
}
static int Pct(s32 v,s32 goal){if(goal<=0)return 100;s32 p=v*100/goal;return p<0?0:(p>100?100:(int)p);}
int Obj_Pct(const World *w,u8 ci,u8 ob){
 const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
 switch(ob){
 case OB_FUNDAR:{int n=0;for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci&&(w->city[i].flags&CF_FOUNDED))n++;return Pct(n,1);}
 case OB_PORTO:return Pct(e->ports,1);
 case OB_NAVIOS:return Pct(c->ships,3);
 case OB_FABRICA:return Pct(CountBld(w,ci,BK_FABRICA),1);
 case OB_METROPOLE:{int n=0;for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci&&w->city[i].tier==CT_METROPOLE)n++;return Pct(n,1);}
 case OB_ALIADO:return Pct(WDip_Allies(w,ci),1);
 case OB_BLOCO:return c->bloc&&WDip_BlocSize(w,c->bloc)>=2?100:0;
 case OB_VIDA:return Pct(e->life,80);
 case OB_COMERCIO:return Pct(e->trade,40);
 case OB_CIDADES:return Pct(c->nCity,5);
 case OB_EXERCITO:return Pct(c->troops,c->cap);
 case OB_MINA:return Pct(CountBld(w,ci,BK_MINA),1);
 case OB_FERROVIA:return Pct(CountBld(w,ci,BK_FERROVIA),2);
 case OB_PRESTIGIO:return Pct(c->prestige,150);
 default:return 0;
 }
}
static bool ObjUsed(const World *w,u8 ob){
 if((w->objMask>>ob)&1)return true;
 for(int k=0;k<3;k++)if(w->obj[k]==ob)return true;
 return false;
}
static u8 NextObj(World *w){
 // sorteio sem RNG do mundo (o jogador nao mexe na historia por olhar a lista)
 u32 h=w->seed*2654435761u+w->day*40503u;
 for(int t=0;t<OB_COUNT;t++){u8 ob=(u8)((h+t*3u)%OB_COUNT);if(!ObjUsed(w,ob))return ob;} // passo 3 (coprimo de 14) visita todas
 return 0xFF;
}
static void Objectives(World *w){
 u8 me=PlayerC(w);if(!me)return;
 for(int k=0;k<3;k++){
  u8 ob=w->obj[k];
  if(ob>=OB_COUNT){w->obj[k]=NextObj(w);continue;}
  if(Obj_Pct(w,me,ob)<100)continue;
  s32 r=Obj_Reward(ob);
  if(r>0)w->c[me].gold+=r;else w->c[me].prestige-=r;
  w->objMask|=(u16)(1u<<ob);w->objDone++;
  World_Msg(w,Str(STR_OB_FEITA),Str(Obj_Str(ob)));
  w->obj[k]=NextObj(w);
 }
}

// ===================== IA mensal =====================
static void Rival(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(Live(w,c->rival)&&!WDip_Allied(w,ci,c->rival)&&World_RngRange(w,12))return;
 u8 best=0;s32 bs=0;
 for(int s=0;s<24;s++){
  u8 j=(u8)(1+World_RngRange(w,w->nC-1));
  if(!Live(w,j)||j==ci||WDip_Allied(w,ci,j)||w->c[j].overlord==ci||c->overlord==j)continue;
  int d=CapDist(w,ci,j);if(d>140)continue;
  s32 sc=w->c[j].troops/(10+d)+(Opposed(c->ideo,w->c[j].ideo)?40:0);
  if(sc>bs){bs=sc;best=j;}
 }
 c->rival=best;
}
static void SeekAlly(World *w,u8 ci){
 if(WDip_Allies(w,ci)>=2||World_RngRange(w,100)>=25)return;
 u8 best=0;int bs=-999;
 for(int s=0;s<32;s++){
  u8 j=(u8)(1+World_RngRange(w,w->nC-1));
  if(!Live(w,j)||j==ci)continue;
  int sc=AllyScore(w,ci,j);
  if(sc>bs){bs=sc;best=j;}
 }
 if(!best||bs<35)return;
 if(w->c[best].player){if(w->c[best].opin>-20)WDip_Propose(w,PR_ALIANCA,ci,0,0,0);return;}
 if(AllyScore(w,best,ci)>=30)WDip_Ally(w,ci,best);
}
static void KeepAllies(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 for(int k=0;k<2;k++){
  u8 a=c->ally[k];
  if(!a)continue;
  if(!Live(w,a)){c->ally[k]=0;continue;}
  if(World_AtWar(w,ci,a)||Opposed(c->ideo,w->c[a].ideo)||World_RngRange(w,300)==0)WDip_Unally(w,ci,a);
 }
}
static void Blocs(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(c->bloc&&(!Live(w,c->bloc)||w->c[c->bloc].bloc!=c->bloc)){c->bloc=0;}
 if(c->bloc&&c->bloc!=ci&&(World_AtWar(w,ci,c->bloc)||Opposed(c->ideo,w->c[c->bloc].ideo))){c->bloc=0;}
 if(c->bloc)return;
 if(c->archetype==WA_ISOLACIONISTA||c->overlord)return;
 // grande economia mercantil funda um mercado comum
 bool merc=c->archetype==WA_MERCADOR||c->m.tradePct>=10||c->cells>=500;
 if(merc&&c->cells>=150&&World_RngRange(w,100)<8){
  int leaders=0;for(u8 j=1;j<w->nC;j++)if(w->c[j].alive&&w->c[j].bloc==j)leaders++;
  if(leaders<10){c->bloc=ci;return;}
 }
 if(World_RngRange(w,100)>=20)return;
 u8 best=0;int bs=-999;
 for(u8 j=1;j<w->nC;j++){
  if(!BlocOk(w,ci,j))continue;
  const WCountry *L=&w->c[j];
  int s=Mn((s32)L->cells/30,30)-CapDist(w,ci,j)/5+(L->ideo==c->ideo?20:0)+(WDip_Allied(w,ci,j)?30:0);
  if(L->player)s+=L->opin/3;
  if(s>bs){bs=s;best=j;}
 }
 if(!best||bs<10)return;
 if(w->c[best].player){WDip_Propose(w,PR_BLOCO,ci,1,0,0);return;}
 c->bloc=best;
}
// tributo: um vizinho forte e hostil exige ouro do jogador
static void Bully(World *w,u8 ci,u8 me){
 WCountry *c=&w->c[ci];const WCountry *P=&w->c[me];
 if(c->rival!=me||World_AtWar(w,ci,me)||WDip_Allied(w,ci,me))return;
 if(c->troops<P->troops*2||c->mood<55||World_RngRange(w,100)>=6)return;
 if(c->archetype!=WA_EXPANSIONISTA&&c->archetype!=WA_OPORTUNISTA)return;
 s32 g=P->gold/4+50;
 WDip_Propose(w,PR_TRIBUTO,ci,0,g,0);
}
static void AiNavyArmy(World *w,u8 ci){
 WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
 // frota: mercador/senhor dos mares ou rival armado no mar
 if(e->yards&&!c->shipQ){
  bool want=c->archetype==WA_MERCADOR||c->m.tradePct>=10||(Live(w,c->rival)&&w->c[c->rival].ships>c->ships);
  s32 cost=WEco_ShipCost(w,ci);
  if(want&&c->ships<3+e->ports*4+e->yards*8&&c->gold>cost*3)WEco_BuildShip(w,ci);
 }
 // recrutar quando a guerra aperta e ha ouro
 if(c->warN&&c->troops<c->cap&&c->gold>World_RecruitCost(w,ci)*2)World_Recruit(w,ci);
 // politica fiscal (imposto, tarifa, educacao, gasto militar): wecon.c
 WEco_AiPolicy(w,ci);
}
// batalhas navais do mes entre paises em guerra
static void Naval(World *w){
 u8 me=PlayerC(w);
 for(u8 i=1;i<w->nC;i++){
  WCountry *A=&w->c[i];
  if(!A->alive||A->ships<=0||!A->warN)continue;
  for(u8 j=(u8)(i+1);j<w->nC;j++){
   WCountry *B=&w->c[j];
   if(!B->alive||B->ships<=0||!World_AtWar(w,i,j))continue;
   if(World_RngRange(w,3))continue;   // nem todo mes as frotas se acham
   s32 sa=A->ships,sb=B->ships;
   s32 la=Mn(sa,sb*sb/(3*(sa+sb))+1),lb=Mn(sb,sa*sa/(3*(sa+sb))+1);
   A->ships-=(s16)la;B->ships-=(s16)lb;w->nNaval++;
   if(la<lb)A->prestige+=3;else if(lb<la)B->prestige+=3;
   if(i==me||j==me){
    u8 foe=i==me?j:i;s32 lost=i==me?la:lb,sunk=i==me?lb:la;
    char t[96];int n=0;const char *p=Str(STR_D_NAVAL);while(*p&&n<40)t[n++]=*p++;
    p=NN(w,foe);while(*p&&n<70)t[n++]=*p++;t[n]=0;
    char b[96];int k=0;p=Str(STR_D_PERDEMOS);while(*p&&k<40)b[k++]=*p++;
    char q[12];Itoa((int)lost,q);p=q;while(*p&&k<50)b[k++]=*p++;
    p=Str(STR_D_AFUNDAMOS);while(*p&&k<80)b[k++]=*p++;
    Itoa((int)sunk,q);p=q;while(*p&&k<90)b[k++]=*p++;b[k]=0;
    World_Msg(w,t,b);
   }
  }
 }
}
static void Revolutions(World *w,u8 ci){
 WCountry *c=&w->c[ci];
 if(c->kind==NK_SUDITO&&c->overlord)return;
 int r=WDip_RevoltRisk(w,ci);
 if(r<=45||World_RngRange(w,1000)>=r-45)return;
 // 1 em 3: o descontentamento vira separatismo, nao revolucao
 if(World_RngRange(w,3)==0&&WDip_Separatism(w,ci))return;
 u8 to=WDip_RandomIdeo(w,ci);
 if(to==c->ideo)return;
 WDip_SetIdeo(w,ci,to,true);
 if(c->player||c->cells>=150)Msg2(w,STR_D_REVOL,NN(w,ci),STR_D_AGORA,Str((u16)(STR_ID_0+to)));
 // intervencao: o rival de ideologia oposta aproveita o caos
 u8 rv=c->rival;
 if(Live(w,rv)&&!w->c[rv].player&&Opposed(w->c[rv].ideo,to)&&w->c[rv].troops>c->troops&&World_RngRange(w,100)<30)
  World_DeclareWarCall(w,rv,ci);
}

// um pais morreu: some das aliancas, rivalidades e do bloco dos outros
void WDip_OnDeath(World *w,u8 ci){
 for(u8 j=1;j<w->nC;j++){
  WCountry *c=&w->c[j];
  for(int k=0;k<2;k++)if(c->ally[k]==ci)c->ally[k]=0;
  if(c->rival==ci)c->rival=0;
  if(c->bloc==ci&&j!=ci)c->bloc=0;
 }
 WCountry *d=&w->c[ci];d->bloc=0;d->ally[0]=d->ally[1]=0;d->rival=0;d->ships=0;d->shipQ=0;
 if(w->prop.kind&&w->prop.from==ci)w->prop.kind=PR_NONE;
}
void WDip_Init(World *w){
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  c->ideo=c->ideo0=WDip_IdeoOfGov(c->gov);
  c->mood=(u8)(((u32)ci*2654435761u^w->seed)%101u);
  c->opin=0;c->ships=0;c->shipQ=0;c->unrestT=0;
  c->ally[0]=c->ally[1]=0;c->rival=0;c->bloc=0;
  // marinhas de 1800: grandes potencias navais comecam com frota
  int s=0;
  for(int i=0;i<NAT_TRAITS;i++){
   if(c->traits[i]==TR_MARES)s+=20;
   if(c->traits[i]==TR_COLONIAL)s+=6;
   if(c->traits[i]==TR_CORSARIOS||c->traits[i]==TR_ENTREPOSTO)s+=4;
  }
  if(c->coastal)s+=(int)(c->cells/120);
  c->ships=(s16)s;
  World_SetTraits(w,ci);
 }
 w->prop.kind=PR_NONE;
 for(int k=0;k<3;k++)w->obj[k]=0xFF;
 w->objMask=0;w->objDone=0;w->nRevol=w->nAlliances=w->nNaval=0;
}
HOT_ARM void WDip_Day(World *w){
 u8 me=PlayerC(w);
 // proposta sem resposta em 30 dias = recusa
 if(w->prop.kind&&w->day>w->prop.day+30){
  if(me)WDip_Answer(w,false);else w->prop.kind=PR_NONE;
 }
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive)continue;
  if(c->unrestT)c->unrestT--;
  if(c->shipQ&&!--c->shipQ)c->ships++;
  // felicidade busca o nivel de vida (+ traits/ideologia); militancia sobe na miseria
  if(((ci+w->day)&7)==0){
   int tgt=30+w->eco[ci].life/2+c->m.happy-(w->eco[ci].tax==2?8:(w->eco[ci].tax==0?-5:0));
   if(c->warDays>360)tgt-=5;
   // teatros e parques alegram (ate +8); deficit cronico e divida alta irritam
   {int fun=0;for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci)for(int k=0;k<CITY_SLOTS;k++){u8 b=w->city[i].bld[k];if(b==BK_TEATRO)fun+=2;else if(b==BK_PARQUE)fun++;}
    tgt+=fun>8?8:fun;
    if(w->eco[ci].debt>WEco_DebtLimit(w,ci)*2/3)tgt-=5;}
   if(c->happy<tgt)c->happy++;else if(c->happy>tgt)c->happy--;
   int mt=20+c->m.militancy+(60-(int)c->happy)/2;
   if(mt<0)mt=0;
   if(c->militancy<mt)c->militancy++;else if(c->militancy>mt)c->militancy--;
  }
  if(((ci+w->day)%30)!=0)continue;
  // mensal
  if(((ci*13u+w->day)%3650u)<30u)c->mood=(u8)World_RngRange(w,101);
  if(c->player){
   continue;
  }
  if(me){
   int d=0;
   if(WDip_Allied(w,ci,me)||(c->bloc&&c->bloc==w->c[me].bloc))d+=2;
   if(c->ideo==w->c[me].ideo)d+=1;
   if(Opposed(c->ideo,w->c[me].ideo))d-=1;
   if(World_AtWar(w,ci,me))d-=3;
   if(c->rival==me)d-=1;
   if(!d)d=c->opin>0?-1:(c->opin<0?1:0);
   int o=c->opin+d;c->opin=(s8)(o<-100?-100:(o>100?100:o));
  }
  Rival(w,ci);
  KeepAllies(w,ci);
  SeekAlly(w,ci);
  Blocs(w,ci);
  AiNavyArmy(w,ci);
  Revolutions(w,ci);
  if(me)Bully(w,ci,me);
  // paz oferecida ao jogador por quem esta perdendo
  if(me&&World_AtWar(w,ci,me)&&c->warDays>365&&c->troops<w->c[me].troops/2&&World_RngRange(w,100)<15)
   WDip_Propose(w,PR_PAZ,ci,0,0,0);
 }
 if(me){
  WCountry *P=&w->c[me];
  if(((me+w->day)%30)==0)Revolutions(w,me);
  if((w->day%7)==3)Objectives(w);
  (void)P;
 }
 if((w->day%30)==15)Naval(w);
}
