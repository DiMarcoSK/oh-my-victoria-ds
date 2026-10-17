#include "wstat.h"
#include "wecon.h"
#include "wrank.h"
#include "str.h"
// wstat.c — series p/ os graficos + marcos (ver wstat.h; SDD ADR-41).
// Custo: 1 amostra a cada `step` dias (14 no comeco) = 16 numeros + 1
// ordenacao do ranking; marcos 1x por mes (1 ranking). Nada usa RNG.

static s32 Real(const World *w,u8 ci,s32 v){
 u32 p=w->eco[ci].cpi?w->eco[ci].cpi:CPI1;
 return (s32)((s64)v*CPI1/p);
}
s32 Stat_Val(const World *w,u8 ci,u8 sr){
 if(!ci||ci>=w->nC)return 0;
 const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
 switch(sr){
  case SR_PIB:return Real(w,ci,e->gdp);
  case SR_POP:return (s32)(c->popK>0x7FFFFFFF?0x7FFFFFFF:c->popK);
  case SR_CAIXA:return Real(w,ci,c->gold);
  case SR_EXERC:return c->troops+(c->atkOn?c->atkTroops:0);
  case SR_DIVIDA:return Real(w,ci,e->debt);
  case SR_ALFAB:return c->lit;
  case SR_VIDA:return e->life;
  default:return c->prestige;
 }
}
u16 Stat_Str(u8 sr){return (u16)(STR_SR_0+sr%SR_COUNT);}
u32 Stat_DayOf(const World *w,int i){return w->st.day0+(u32)i*w->st.step;}
static u8 PlayerC(const World *w){for(u8 i=1;i<w->nC;i++)if(w->c[i].player&&w->c[i].alive)return i;return 0;}
static void Itoa(int v,char *o){char t[12];int n=0;bool neg=v<0;if(neg)v=-v;do{t[n++]=(char)('0'+v%10);v/=10;}while(v&&n<11);
 int k=0;if(neg)o[k++]='-';while(n)o[k++]=t[--n];o[k]=0;}
static void Cat(char *d,int max,const char *s){int n=0;while(d[n])n++;while(*s&&n<max-1)d[n++]=*s++;d[n]=0;}

// marcos do jogador (1x por mes): melhor posicao no ranking, entrar/sair das
// 8 grandes, PIB real e populacao dobrando. Recompensa pequena e visivel: o
// "mais um mes" do Victoria.
static void Milestones(World *w,u8 me){
 WCountry *c=&w->c[me];
 int pos=WRank_Pos(w,me,RK_GERAL);
 if(pos>=0){
  int p1=pos+1;char t[96];
  if(!w->rankBest){w->rankBest=(u8)(p1>255?255:p1);w->rankGreat=p1<=8;}
  else{
   if(p1<=8&&!w->rankGreat){w->rankGreat=1;c->prestige+=15;World_Msg(w,Str(STR_MS_GRANDE),Str(STR_MS_GRANDE_TXT));}
   else if(p1>8&&w->rankGreat){w->rankGreat=0;t[0]=0;Cat(t,96,Str(STR_MS_PARA));char n[8];Itoa(p1,n);Cat(t,96,n);Cat(t,96,Str(STR_MS_LUGAR));World_Msg(w,Str(STR_MS_CAIU),t);}
   else if(p1<w->rankBest){t[0]=0;Cat(t,96,Str(STR_MS_PARA));char n[8];Itoa(p1,n);Cat(t,96,n);Cat(t,96,Str(STR_MS_LUGAR));World_Msg(w,Str(STR_MS_SUBIU),t);c->prestige+=2;}
   if(p1<w->rankBest)w->rankBest=(u8)p1;
  }
 }
 s32 g=Stat_Val(w,me,SR_PIB);
 if(w->gdpMark<=0)w->gdpMark=g>0?g:1;
 else if(g>=w->gdpMark*2){w->gdpMark=g;c->prestige+=10;World_Msg(w,Str(STR_MS_PIB),Str(STR_MS_PIB_TXT));}
 if(!w->popMark)w->popMark=c->popK?c->popK:1;
 else if(c->popK>=w->popMark*2){w->popMark=c->popK;c->prestige+=5;World_Msg(w,Str(STR_MS_POP),Str(STR_MS_POP_TXT));}
}
void Stat_Day(World *w){
 WStats *s=&w->st;
 u8 me=PlayerC(w);
 if(me&&(w->day%30)==11)Milestones(w,me);
 if(!s->step){s->step=14;s->day0=w->day;s->acc=(u16)(s->step-1);}
 if(++s->acc<s->step)return;
 s->acc=0;
 if(!me)return;
 if(s->n>=ST_N){
  // cheio: media dos pares, passo dobra (a serie continua cobrindo tudo)
  for(int k=0;k<SR_COUNT;k++)for(int l=0;l<2;l++)for(int i=0;i<ST_N/2;i++){
   s64 a=s->v[k][l][2*i],b=s->v[k][l][2*i+1];s->v[k][l][i]=(s32)((a+b)/2);}
  s->n=ST_N/2;s->step=(u16)(s->step*2);
 }
 static u8 ord[W_MAXC];
 int nr=WRank_Sort(w,RK_GERAL,ord);
 u8 top=nr>0?ord[0]:me;
 if(top==me&&nr>1)top=ord[1];        // voce lidera: compara com o 2o
 s->top=top;
 for(int k=0;k<SR_COUNT;k++){s->v[k][0][s->n]=Stat_Val(w,me,(u8)k);s->v[k][1][s->n]=Stat_Val(w,top,(u8)k);}
 s->n++;
}
