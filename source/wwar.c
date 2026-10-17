#include "wwar.h"
#include "wdiplo.h"
#include "wecon.h"
#include "str.h"
#include "hot.h"
// wwar.c — crises e guerras com objetivo (ver wwar.h; SDD ADR-45).
// Custo: War_Day e' O(CR_MAX) + 4 sorteios de terceiros por crise a cada 3
// dias. MOB[] (quem esta' mobilizado) e' recalculado todo dia a partir do
// World: nao precisa ir ao save e nao quebra o replay.

static u8 MOB[W_MAXC+1];
static inline int Iabs(int v){return v<0?-v:v;}
static inline s32 Mn(s32 a,s32 b){return a<b?a:b;}
static inline s32 Mx(s32 a,s32 b){return a>b?a:b;}
static bool Live(const World *w,u8 c){return c&&c<w->nC&&w->c[c].alive;}
static const char *NN(const World *w,u8 c){return c?NATIONS[w->c[c].nat].name:"";}
static u8 Leader(const WCrisis *k,int side){return side?k->def:k->att;}
static bool Big(const World *w,u8 c){return Live(w,c)&&(w->c[c].player||w->c[c].cells>=300);}
static bool Seen(const World *w,const WCrisis *k){
 if(Big(w,k->att)||Big(w,k->def))return true;
 for(int s=0;s<2;s++)for(int i=0;i<CR_SIDE;i++)if(Live(w,k->side[s][i])&&w->c[k->side[s][i]].player)return true;
 return false;
}
// "<a><mid><b><end>" na faixa de noticias
static void Msg(World *w,u16 title,const char *a,u16 mid,const char *b,u16 end){
 char t[128];int n=0;const char *p=a;
 while(p&&*p&&n<50)t[n++]=*p++;
 p=Str(mid);while(*p&&n<80)t[n++]=*p++;
 p=b;while(p&&*p&&n<110)t[n++]=*p++;
 if(end){p=Str(end);while(*p&&n<126)t[n++]=*p++;}
 t[n]=0;
 World_Msg(w,Str(title),t);
}
static int CapDist(const World *w,u8 a,u8 b){
 int dx=Iabs((int)w->c[a].capX-(int)w->c[b].capX);
 if(w->wrap&&dx>w->w/2)dx=w->w-dx;
 return dx+Iabs((int)w->c[a].capY-(int)w->c[b].capY);
}

u16 War_GoalStr(u8 g){static const u16 S[GL_COUNT]={STR_GL_TERRA,STR_GL_TRIBUTO,STR_GL_HUMILHAR};return S[g<GL_COUNT?g:0];}
void War_Init(World *w){
 for(int k=0;k<CR_MAX;k++)w->cr[k].st=CRS_NONE;
 for(int i=0;i<=W_MAXC;i++)MOB[i]=0;
 w->nCrises=w->nConcede=w->nBackdown=w->nDeals=0;
}
int War_Find(const World *w,u8 a,u8 b){
 for(int k=0;k<CR_MAX;k++){const WCrisis *c=&w->cr[k];
  if(c->st&&((c->att==a&&c->def==b)||(c->att==b&&c->def==a)))return k;}
 return -1;
}
int War_Side(const World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||!w->cr[k].st||!ci)return -1;
 const WCrisis *c=&w->cr[k];
 if(c->att==ci)return 0;
 if(c->def==ci)return 1;
 for(int s=0;s<2;s++)for(int i=0;i<CR_SIDE;i++)if(c->side[s][i]==ci)return s;
 return -1;
}
int War_Of(const World *w,u8 ci){
 int best=-1,bd=0x7FFF;
 for(int k=0;k<CR_MAX;k++){
  const WCrisis *c=&w->cr[k];
  if(!c->st||War_Side(w,k,ci)<0)continue;
  // lider antes de apoio; crise (com prazo) antes de guerra
  int d=(c->st==CRS_CRISE?c->days:1000+c->days)+((c->att==ci||c->def==ci)?0:5000);
  if(d<bd){bd=d;best=k;}
 }
 return best;
}
bool War_Managed(const World *w,u8 a,u8 b){
 for(int k=0;k<CR_MAX;k++){
  if(w->cr[k].st!=CRS_GUERRA)continue;
  int sa=War_Side(w,k,a),sb=War_Side(w,k,b);
  if(sa>=0&&sb>=0&&sa!=sb)return true;
 }
 return false;
}
bool War_Mobilized(const World *w,u8 ci){(void)w;return ci<=W_MAXC&&MOB[ci];}
s32 War_Strength(const World *w,int k,int side){
 const WCrisis *c=&w->cr[k];
 s32 s=0;
 u8 L=Leader(c,side);
 if(Live(w,L))s+=w->c[L].troops+w->c[L].atkTroops+w->c[L].ships*20;
 for(int i=0;i<CR_SIDE;i++){u8 x=c->side[side][i];if(Live(w,x))s+=(w->c[x].troops+w->c[x].atkTroops)*2/3+w->c[x].ships*12;}
 return s;
}
int War_Score(const World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||!w->cr[k].st)return 0;
 int sd=War_Side(w,k,ci);
 return sd==1?-w->cr[k].score:w->cr[k].score;
}
s32 War_DemandGold(const World *w,int k){
 const WCrisis *c=&w->cr[k];
 return WEco_Nom(w,c->def,c->demand);
}

// ---- abertura ----
static s32 Demand(const World *w,u8 goal,u8 def){
 const WCountry *d=&w->c[def];
 switch(goal){
  case GL_TERRA:return Mx(3,Mn(80,(s32)d->cells/10+3));
  case GL_TRIBUTO:return Mn(3000,100+(s32)d->cells/2+(s32)(d->popK/500));
  default:return 10+Mx(0,d->prestige)/5;
 }
}
static bool InSide(const WCrisis *c,u8 x){
 if(c->att==x||c->def==x)return true;
 for(int s=0;s<2;s++)for(int i=0;i<CR_SIDE;i++)if(c->side[s][i]==x)return true;
 return false;
}
static bool AddSide(WCrisis *c,int side,u8 x){
 if(InSide(c,x))return false;
 for(int i=0;i<CR_SIDE;i++)if(!c->side[side][i]){c->side[side][i]=x;return true;}
 return false;
}
// quanto x quer entrar do lado `side` (IA); <0 = nunca
static int JoinScore(const World *w,const WCrisis *c,u8 x,int side){
 const WCountry *X=&w->c[x];
 u8 L=Leader(c,side),E=Leader(c,!side);
 if(!Live(w,L)||!Live(w,E)||X->player)return -999;
 if(WDip_Allied(w,x,E)||X->overlord==E||World_AtWar(w,x,L))return -999;
 int s=0;
 if(WDip_Allied(w,x,L))s+=70;
 if(X->overlord==L)s+=100;
 if(X->rival==E)s+=45;
 if(WDip_Opposed(X->ideo,w->c[E].ideo))s+=20;
 if(X->ideo==w->c[L].ideo)s+=10;
 if(X->bloc&&X->bloc==w->c[L].bloc)s+=15;
 if(w->c[E].player)s-=X->opin/3;else if(w->c[L].player)s+=X->opin/3;
 s-=CapDist(w,x,L)/6;
 s+=((int)X->mood-50)/3;
 if(X->archetype==WA_ISOLACIONISTA)s-=40;
 if(X->archetype==WA_DIPLOMATA)s+=10;
 if(X->warN)s-=25;                                     // ja' tem guerra p/ cuidar
 if(w->c[E].troops>X->troops*4&&!WDip_Allied(w,x,L))s-=20; // medo do lider inimigo
 return s;
}
int War_CallAllies(World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||!w->cr[k].st)return 0;
 WCrisis *c=&w->cr[k];
 int side=War_Side(w,k,ci);
 if(side<0||Leader(c,side)!=ci)return 0;
 u8 fl=side?CRF_CALL1:CRF_CALL0;
 if(c->flags&fl)return 0;
 c->flags|=fl;
 int n=0;
 // aliados e suditos primeiro; depois quem tem motivo (amostra do mundo)
 for(int pass=0;pass<2;pass++)for(u8 x=1;x<w->nC;x++){
  if(!Live(w,x)||InSide(c,x))continue;
  bool close=WDip_Allied(w,x,ci)||w->c[x].overlord==ci;
  if((pass==0)!=close)continue;
  int s=JoinScore(w,c,x,side);
  if(s<50)continue;
  if(!close&&World_RngRange(w,100)>=s-30)continue;
  if(!AddSide(c,side,x))return n;
  n++;
  if(Seen(w,c))Msg(w,STR_CR_TITULO,NN(w,x),STR_CR_LADO,NN(w,ci),0);
 }
 return n;
}
bool War_JoinSide(World *w,int k,u8 ci,int side){
 if(k<0||k>=CR_MAX||!w->cr[k].st||side<0||side>1)return false;
 WCrisis *c=&w->cr[k];
 if(!AddSide(c,side,ci))return false;
 // entrou numa guerra ja' aberta: declara aos lideres e apoios do outro lado
 if(c->st==CRS_GUERRA){
  u8 E=Leader(c,!side);World_DeclareWar(w,ci,E);
  for(int i=0;i<CR_SIDE;i++)if(Live(w,c->side[!side][i]))World_DeclareWar(w,ci,c->side[!side][i]);
 }
 return true;
}
int War_Start(World *w,u8 att,u8 def,u8 goal){
 if(!Live(w,att)||!Live(w,def)||att==def||goal>=GL_COUNT)return -1;
 if(World_AtWar(w,att,def)||War_Find(w,att,def)>=0)return -1;
 if(w->c[def].overlord==att||w->c[att].overlord==def)return -1;
 int k=-1;for(int i=0;i<CR_MAX;i++)if(!w->cr[i].st){k=i;break;}
 if(k<0)return -1;
 WCrisis *c=&w->cr[k];
 u8 *z=(u8*)c;for(unsigned i=0;i<sizeof *c;i++)z[i]=0;
 c->st=CRS_CRISE;c->att=att;c->def=def;c->goal=goal;c->days=CR_DAYS;
 c->demand=Demand(w,goal,def);c->talkCd=7;
 w->nCrises++;
 WDip_Unally(w,att,def);
 {WCountry *D=&w->c[def];int o=D->opin-20;if(w->c[att].player)D->opin=(s8)(o<-100?-100:o);}
 w->c[att].prestige-=2;                                  // exigir custa reputacao
 w->c[def].alert=10;
 // aliados do jogador: pedido de apoio (despacho); IA chama os seus ja'
 for(int s=0;s<2;s++){
  u8 L=Leader(c,s);
  if(!w->c[L].player)War_CallAllies(w,k,L);
  for(int a=0;a<2;a++){u8 x=w->c[L].ally[a];
   if(Live(w,x)&&w->c[x].player&&!InSide(c,x))WDip_Propose(w,PR_LADO,L,(u8)k,s,0);}
 }
 if(w->c[def].player)WDip_Propose(w,PR_CRISE,att,(u8)k,0,0);
 if(Seen(w,c)){
  char t[96];int n=0;const char *p=NN(w,att);while(*p&&n<40)t[n++]=*p++;
  p=Str(STR_CR_EXIGE);while(*p&&n<60)t[n++]=*p++;
  p=Str(War_GoalStr(goal));while(*p&&n<80)t[n++]=*p++;t[n]=0;
  Msg(w,STR_CR_TITULO,t,STR_CR_DE,NN(w,def),STR_CR_PRAZO);
 }
 return k;
}

// ---- cobrar o objetivo ----
// celula do derrotado mais perto do vencedor (fronteira primeiro): centro da cessao
static bool CedeCenter(const World *w,u8 to,u8 from,int *ox,int *oy){
 const WCountry *A=&w->c[to],*B=&w->c[from];
 int best=-1,bd=1<<30;
 for(int y=B->by0;y<=B->by1&&y<w->h;y++)for(int x=B->bx0;x<=B->bx1&&x<w->w;x++){
  u32 i=World_Idx(w,x,y);
  if(W_OWNER[i]!=from||W_TERR[i]==WT_WATER)continue;
  if(B->capX==x&&B->capY==y)continue;
  int dx=Iabs(x-(int)A->capX);if(w->wrap&&dx>w->w/2)dx=w->w-dx;
  int d=dx+Iabs(y-(int)A->capY);
  // colada no vencedor vale como se estivesse muito mais perto
  bool adj=false;
  for(int k=0;k<4&&!adj;k++){int nx=x+(k==0)-(k==1),ny=y+(k==2)-(k==3);
   if(nx<0||ny<0||nx>=w->w||ny>=w->h)continue;
   if(W_OWNER[World_Idx(w,nx,ny)]==to)adj=true;}
  if(adj)d/=4;
  if(d<bd){bd=d;best=(int)i;}
 }
 if(best<0)return false;
 *ox=best%w->w;*oy=best/w->w;return true;
}
static void Cede(World *w,u8 to,u8 from,s32 cells){
 int x,y;
 if(cells<=0||!CedeCenter(w,to,from,&x,&y))return;
 s32 n=0;
 for(int r=1;r<=14&&n<cells&&Live(w,from);r++)n+=World_Transfer(w,to,from,x,y,r);
}
static void Pay(World *w,u8 from,u8 to,s32 v1800){
 WCountry *F=&w->c[from],*T=&w->c[to];
 s32 v=WEco_Nom(w,from,v1800);
 s32 cash=Mn(Mx(0,F->gold),v);
 F->gold-=cash;w->eco[from].debt+=v-cash;      // o que falta vira divida
 T->gold+=WEco_Nom(w,to,v1800);
}
// o lado `win` recebe `pct`% do objetivo do lider vencedor (atacante) ou a
// indenizacao (defensor vence: o atacante paga e perde prestigio)
static void Enforce(World *w,WCrisis *c,int win,int pct){
 u8 A=c->att,D=c->def;
 if(!Live(w,A)||!Live(w,D))return;
 if(win==0){
  switch(c->goal){
   case GL_TERRA:{s32 took=c->st==CRS_GUERRA&&c->c0[1]>w->c[D].cells?(s32)(c->c0[1]-w->c[D].cells):0;
    Cede(w,A,D,c->demand*pct/100-took);}break;
   case GL_TRIBUTO:Pay(w,D,A,c->demand*pct/100);break;
   default:{s32 p=c->demand*pct/100;w->c[D].prestige-=p;w->c[A].prestige+=p;}break;
  }
  w->c[A].prestige+=3;w->c[D].prestige-=3;
 }else{
  Pay(w,A,D,50+(s32)w->c[A].cells/4);
  s32 p=10+Mx(0,w->c[A].prestige)/10;w->c[A].prestige-=p;w->c[D].prestige+=p/2;
 }
}
// fecha a entrada k; se era guerra, todos os pares dos dois lados fazem paz
static void Close(World *w,int k){
 WCrisis c=w->cr[k];
 w->cr[k].st=CRS_NONE;
 if(c.st!=CRS_GUERRA)return;
 u8 L[2][CR_SIDE+1];
 for(int s=0;s<2;s++){L[s][0]=Leader(&c,s);for(int i=0;i<CR_SIDE;i++)L[s][i+1]=c.side[s][i];}
 for(int i=0;i<=CR_SIDE;i++)for(int j=0;j<=CR_SIDE;j++){
  u8 a=L[0][i],b=L[1][j];
  if(Live(w,a)&&Live(w,b)&&World_AtWar(w,a,b)&&War_Find(w,a,b)<0)World_Peace(w,a,b);
 }
}
void War_Yield(World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||!w->cr[k].st)return;
 WCrisis *c=&w->cr[k];
 if(ci==c->def){
  Enforce(w,c,0,100);w->nConcede++;
  if(Seen(w,c))Msg(w,STR_CR_FIM,NN(w,c->def),STR_CR_CEDEU,NN(w,c->att),0);
 }else if(ci==c->att){
  s32 p=8+c->demand/(c->goal==GL_TRIBUTO?60:(c->goal==GL_TERRA?4:2));
  w->c[c->att].prestige-=p;w->c[c->def].prestige+=p/2;w->nBackdown++;
  if(Seen(w,c))Msg(w,STR_CR_FIM,NN(w,c->att),STR_CR_RECUOU,NN(w,c->def),0);
 }else return;
 Close(w,k);
}
// forca do lado de ci / forca do outro lado (x100)
static int Ratio(const World *w,int k,int side){
 s32 a=War_Strength(w,k,side),b=War_Strength(w,k,!side);
 return (int)Mn(1000,(s64)a*100/Mx(1,b));
}
bool War_Deal(World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||w->cr[k].st!=CRS_CRISE)return false;
 WCrisis *c=&w->cr[k];
 int side=War_Side(w,k,ci);
 if(side<0||Leader(c,side)!=ci||c->talkCd)return false;
 c->talkCd=15;
 u8 o=Leader(c,!side);
 int r=Ratio(w,k,side);                 // nossa forca vs a deles
 bool ok;
 if(w->c[o].player)ok=false;            // o jogador responde pelo despacho (PR_CRISE)
 else if(side==0)ok=r>=90||World_RngRange(w,100)<20;   // defensor aceita meio termo de quem e' forte
 else ok=r>=80||World_RngRange(w,100)<25;              // atacante aceita metade de quem resiste
 if(!ok){if(w->c[o].player)WDip_Propose(w,PR_CRISE,ci,(u8)k,0,1);return false;}
 War_Settle(w,k,50);
 return true;
}
void War_Settle(World *w,int k,int pct){
 if(k<0||k>=CR_MAX||!w->cr[k].st)return;
 WCrisis *c=&w->cr[k];
 Enforce(w,c,0,pct);w->nDeals++;
 if(Seen(w,c))Msg(w,STR_CR_FIM,NN(w,c->att),STR_CR_ACORDO,NN(w,c->def),STR_CR_ACORDO2);
 Close(w,k);
}

// ---- guerra ----
static void OpenWar(World *w,int k){
 WCrisis *c=&w->cr[k];
 u8 L[2][CR_SIDE+1];
 for(int s=0;s<2;s++){L[s][0]=Leader(c,s);for(int i=0;i<CR_SIDE;i++)L[s][i+1]=c->side[s][i];}
 for(int i=0;i<=CR_SIDE;i++)for(int j=0;j<=CR_SIDE;j++){
  u8 a=L[0][i],b=L[1][j];
  if(Live(w,a)&&Live(w,b))World_DeclareWar(w,a,b);
 }
 c->st=CRS_GUERRA;c->days=0;c->score=0;c->talkCd=30;
 c->c0[0]=w->c[c->att].cells;c->c0[1]=w->c[c->def].cells;
 for(int s=0;s<2;s++)for(int i=0;i<=CR_SIDE;i++){u8 x=L[s][i];if(Live(w,x)){w->c[x].alert=20;w->c[x].warGoal=0;w->c[x].warTarget=Leader(c,!s);}}
 if(Seen(w,c))Msg(w,STR_CR_GUERRA,NN(w,c->att),STR_CR_X,NN(w,c->def),STR_CR_GUERRA2);
}
static void Score(World *w,WCrisis *c){
 const WCountry *A=&w->c[c->att],*D=&w->c[c->def];
 s32 la=c->c0[0]?((s32)c->c0[0]-(s32)A->cells)*100/(s32)c->c0[0]:0;   // % que o atacante perdeu
 s32 ld=c->c0[1]?((s32)c->c0[1]-(s32)D->cells)*100/(s32)c->c0[1]:0;   // % que o defensor perdeu
 s32 sa=0,sd=0;
 for(int s=0;s<2;s++){s32 v=War_Strength(w,(int)(c-w->cr),s);if(s)sd=v;else sa=v;}
 s32 tr=(sa+sd)?(sa-sd)*20/(sa+sd):0;
 s32 v=(ld-la)*2+tr;
 if(v>100)v=100;
 if(v<-100)v=-100;
 c->score=(s8)v;
}
static int Need(const WCrisis *c){return c->goal==GL_TERRA?30:20;}
static void EndWar(World *w,int k,int win){       // win: 0 atacante, 1 defensor, -1 paz branca
 WCrisis *c=&w->cr[k];
 if(win>=0)Enforce(w,c,win,100);
 if(Seen(w,c)){
  if(win<0)Msg(w,STR_CR_PAZ,NN(w,c->att),STR_CR_BRANCA,NN(w,c->def),0);
  else Msg(w,STR_CR_PAZ,NN(w,win?c->def:c->att),STR_CR_VENCEU,NN(w,win?c->att:c->def),0);
 }
 Close(w,k);
}
int War_PeaceTry(World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||w->cr[k].st!=CRS_GUERRA)return 0;
 WCrisis *c=&w->cr[k];
 int side=War_Side(w,k,ci);
 if(side<0||Leader(c,side)!=ci||c->talkCd)return 0;
 c->talkCd=20;
 Score(w,c);
 int sc=War_Score(w,k,ci);
 u8 o=Leader(c,!side);
 int tired=w->c[o].warDays/30;           // meses de guerra do outro lado
 // vencendo: o outro aceita os termos; empate cansado: paz branca
 if(sc>=Need(c)||(sc>=Need(c)/2&&tired>=12)){EndWar(w,k,side);return 2;}
 if(sc>-10&&(tired>=12||World_RngRange(w,100)<15+tired*3)){EndWar(w,k,-1);return 1;}
 return 0;
}
// preco da paz comprada (ouro de 1800): sobe com o placar contra e o tamanho do outro
static s32 ConcedeBase(const World *w,int k,u8 ci){
 const WCrisis *c=&w->cr[k];
 int sc=War_Score(w,k,ci);
 u8 o=c->att==ci?c->def:c->att;
 return 80+Mx(0,-sc)*12+(s32)w->c[o].cells/8+(c->goal==GL_TRIBUTO?c->demand/2:0);
}
s32 War_ConcedeCost(const World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||w->cr[k].st!=CRS_GUERRA)return 0;
 return WEco_Nom(w,ci,ConcedeBase(w,k,ci));
}
bool War_Concede(World *w,int k,u8 ci){
 if(k<0||k>=CR_MAX||w->cr[k].st!=CRS_GUERRA)return false;
 WCrisis *c=&w->cr[k];
 int side=War_Side(w,k,ci);
 if(side<0||Leader(c,side)!=ci)return false;
 s32 cost=War_ConcedeCost(w,k,ci);
 if(w->c[ci].gold<cost)return false;
 u8 o=Leader(c,!side);
 w->c[ci].gold-=cost;w->c[o].gold+=WEco_Nom(w,o,ConcedeBase(w,k,ci));
 w->c[ci].prestige-=5;w->c[o].prestige+=5;
 if(Seen(w,c))Msg(w,STR_CR_PAZ,NN(w,ci),STR_CR_COMPROU,NN(w,o),0);
 Close(w,k);
 return true;
}
void War_PeaceAnswer(World *w,u8 me,u8 from,s32 gold,bool yes){
 int k=War_Find(w,me,from);
 if(!yes)return;
 if(gold>0){s32 v=Mn(gold,Mx(0,w->c[me].gold));w->c[me].gold-=v;w->eco[me].debt+=gold-v;w->c[from].gold+=gold;}
 else if(gold<0){w->c[me].gold-=gold;w->c[from].gold+=gold;}
 if(k>=0&&w->cr[k].st==CRS_GUERRA)Close(w,k);
 else World_Peace(w,me,from);
}
void War_OnPeace(World *w,u8 a,u8 b){
 for(int k=0;k<CR_MAX;k++){
  WCrisis *c=&w->cr[k];
  if(c->st!=CRS_GUERRA)continue;
  if((c->att==a&&c->def==b)||(c->att==b&&c->def==a)){Close(w,k);continue;}
  // paz em separado de um apoio: sai da lista
  int sa=War_Side(w,k,a),sb=War_Side(w,k,b);
  if(sa<0||sb<0||sa==sb)continue;
  u8 x=(a==c->att||a==c->def)?b:a;
  for(int s=0;s<2;s++)for(int i=0;i<CR_SIDE;i++)if(c->side[s][i]==x&&!World_AtWar(w,x,Leader(c,!s)))c->side[s][i]=0;
 }
}
void War_OnDeath(World *w,u8 ci){
 for(int k=0;k<CR_MAX;k++){
  WCrisis *c=&w->cr[k];
  if(!c->st)continue;
  if(c->att==ci||c->def==ci){c->st=CRS_NONE;continue;}
  for(int s=0;s<2;s++)for(int i=0;i<CR_SIDE;i++)if(c->side[s][i]==ci)c->side[s][i]=0;
 }
 if(ci<=W_MAXC)MOB[ci]=0;
}

// ---- IA dos lideres ----
static void AiCrisis(World *w,int k){
 WCrisis *c=&w->cr[k];
 bool pa=w->c[c->att].player,pd=w->c[c->def].player;
 int r=Ratio(w,k,0);                     // forca do atacante / defensor (x100)
 int mood=w->c[c->def].mood;
 // defensor IA: muito mais fraco cede; mais fraco tenta acordo
 if(!pd){
  if(r>=300&&World_RngRange(w,100)<40+(100-mood)/4){War_Yield(w,k,c->def);return;}
  if(r>=160&&!c->talkCd&&World_RngRange(w,100)<50){if(War_Deal(w,k,c->def))return;}
 }
 // atacante IA: lado mais fraco recua; empate procura acordo
 if(!pa&&c->st){
  if(r<75&&World_RngRange(w,100)<60){War_Yield(w,k,c->att);return;}
  if(r<115&&!c->talkCd&&World_RngRange(w,100)<45){if(War_Deal(w,k,c->att))return;}
 }
 // o jogador e' o alvo e a IA e' fraca demais p/ manter a exigencia: oferece acordo
 if(pd&&!pa&&c->st&&r<130&&!c->talkCd&&World_RngRange(w,100)<40){c->talkCd=20;WDip_Propose(w,PR_CRISE,c->att,(u8)k,0,1);}
}
static void Sway(World *w,int k){
 WCrisis *c=&w->cr[k];
 for(int t=0;t<4;t++){
  u8 x=(u8)(1+World_RngRange(w,w->nC-1));
  if(!Live(w,x)||InSide(c,x)||w->c[x].player)continue;
  int s0=JoinScore(w,c,x,0),s1=JoinScore(w,c,x,1);
  int side=s1>s0?1:0,s=side?s1:s0;
  if(s<60||World_RngRange(w,100)>=s-30)continue;
  if(AddSide(c,side,x)&&Seen(w,c))Msg(w,STR_CR_TITULO,NN(w,x),STR_CR_LADO,NN(w,Leader(c,side)),0);
 }
}
static void AiWar(World *w,int k){
 WCrisis *c=&w->cr[k];
 bool pa=w->c[c->att].player,pd=w->c[c->def].player;
 int sc=c->score,need=Need(c);
 if(!pa&&!pd){
  if(sc>=need){EndWar(w,k,0);return;}
  if(sc<=-need){EndWar(w,k,1);return;}
  if(c->days>540&&Iabs(sc)<need&&World_RngRange(w,100)<30){EndWar(w,k,-1);return;}
  if(c->days>1080){EndWar(w,k,-1);return;}
  return;
 }
 // o jogador e' um dos lideres: a IA propoe a paz pelo despacho
 u8 me=pa?c->att:c->def,ai=pa?c->def:c->att;
 int my=pa?sc:-sc;                          // placar do jogador
 if(w->prop.kind)return;
 if(my<=-need){                              // IA vencendo: cobra p/ parar
  s32 g=WEco_Nom(w,me,(c->goal==GL_TRIBUTO&&pa==false?c->demand:80+(s32)w->c[ai].cells/6));
  WDip_Propose(w,PR_PAZ,ai,(u8)k,g,0);
 }else if(my>=need||(c->days>360&&World_RngRange(w,100)<25)){
  s32 g=my>=need?-WEco_Nom(w,me,40+(s32)w->c[me].cells/10):0;   // IA perdendo: oferece indenizacao
  WDip_Propose(w,PR_PAZ,ai,(u8)k,g,0);
 }
}
HOT_ARM void War_Day(World *w){
 for(int i=0;i<=W_MAXC;i++)MOB[i]=0;
 for(int k=0;k<CR_MAX;k++){
  WCrisis *c=&w->cr[k];
  if(!c->st)continue;
  if(!Live(w,c->att)||!Live(w,c->def)){c->st=CRS_NONE;continue;}
  for(int s=0;s<2;s++){MOB[Leader(c,s)]=1;for(int i=0;i<CR_SIDE;i++){u8 x=c->side[s][i];if(x&&!Live(w,x))c->side[s][i]=0;else if(x)MOB[x]=1;}}
  if(c->talkCd)c->talkCd--;
  if(c->st==CRS_CRISE){
   if((w->day+k)%3==0)Sway(w,k);
   if(c->days==CR_DAYS/2||c->days==CR_DAYS/4||c->days==3){AiCrisis(w,k);if(!c->st)continue;}
   if(c->days)c->days--;
   if(!c->days)OpenWar(w,k);
  }else{
   if(c->days<60000)c->days++;
   if(((w->day+k)&7)==0)Score(w,c);
   if(((w->day+(u32)k)%30)==0&&!c->talkCd)AiWar(w,k);
   // os lideres ja' nao estao em guerra (paz por outro caminho): fecha
   if(c->st&&!World_AtWar(w,c->att,c->def))c->st=CRS_NONE;
  }
 }
}
