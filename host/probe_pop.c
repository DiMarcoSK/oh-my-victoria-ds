// probe_pop.c — calibracao: populacao/alfabetizacao de 1800 e componentes do
// ranking (prestigio, PIB, exercito, frota) das potencias.
#include <stdio.h>
#include "world.h"
#include "nations.h"
static World W;
int main(void){
 World_GenMundi(&W,42,WH_LIGADA,1,-1);
 for(int d=0;d<30;d++)World_Day(&W);
 const char *T[]={"GBR","FRA","RUS","QNG","USA","PRU","HAB","ESP","OTT","JAP","MAR","BRU","JOH","SUL","DAN","RAM","OMA","EIC",0};
 for(int k=0;T[k];k++){
  u8 ci=World_CountryOfNat(&W,Nat_Find(T[k]));if(!ci)continue;
  const WCountry *c=&W.c[ci];const WEco *e=&W.eco[ci];
  printf("%s cells %u popK %u lit %d prest %d gdp %d tropas %d navios %d\n",T[k],c->cells,c->popK,c->lit/10,(int)c->prestige,(int)e->gdp,(int)c->troops,c->ships);
 }
 return 0;
}
