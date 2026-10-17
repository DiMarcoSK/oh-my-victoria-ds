#include "wrank.h"
// wrank.c — ranking das potencias (ver wrank.h). Inteiro, sem RNG, sem estado.
// Pontuacao geral (calibrada em 1800: Russia, Qing, Gra-Bretanha, Franca,
// Habsburgo, Otomanos, Prussia e Espanha abrem como grandes potencias):
//   prestigio x3 + economia/12 + exercito/300 + navios x2 + alfabetizacao/2%

s32 WRank_Score(const World *w,u8 ci,u8 mode){
 const WCountry *c=&w->c[ci];const WEco *e=&w->eco[ci];
 if(!c->alive)return -1;
 s32 army=c->troops+c->atkTroops;
 switch(mode){
  case RK_ECON:return e->gdp;
  case RK_MIL:return army+c->ships*60;
  case RK_POP:return (s32)(c->popK>0x7FFFFFFF?0x7FFFFFFF:c->popK);
  default:
   // poder de grande potencia: prestigio pesa mais (Victoria 3), a economia
   // e as armas sustentam; gente letrada conta pouco mas sempre
   // economia = PIB + gente produtiva (populacao x alfabetizacao)
   {s32 econ=e->gdp*4+(s32)((u64)c->popK*(20+c->lit/10)/4000);
    return c->prestige*3+econ/12+army/300+c->ships*2+c->lit/20;}
 }
}
int WRank_Sort(const World *w,u8 mode,u8 *out){
 static s32 sc[W_MAXC+1];
 int n=0;
 for(u8 ci=1;ci<w->nC;ci++){
  if(!w->c[ci].alive)continue;
  sc[ci]=WRank_Score(w,ci,mode);
  // insercao (estavel): maior pontuacao primeiro, empate pelo indice
  int j=n++;
  while(j>0&&sc[out[j-1]]<sc[ci]){out[j]=out[j-1];j--;}
  out[j]=ci;
 }
 return n;
}
u8 WRank_Tier(int pos){return pos<8?RT_GRANDE:(pos<20?RT_MAIOR:(pos<50?RT_MENOR:RT_INSIG));}
int WRank_Pos(const World *w,u8 ci,u8 mode){
 if(!ci||ci>=w->nC||!w->c[ci].alive)return -1;
 s32 me=WRank_Score(w,ci,mode);int p=0;
 for(u8 j=1;j<w->nC;j++){
  if(j==ci||!w->c[j].alive)continue;
  s32 s=WRank_Score(w,j,mode);
  if(s>me||(s==me&&j<ci))p++;
 }
 return p;
}
