// prof_day.c — onde o World_Day gasta. Compile com -DPROF_DAY (world.c mede
// cada subsistema). 10 anos de mapa-mundi com historia, jogador = GBR.
#include <stdio.h>
#include <time.h>
#include "world.h"
#include "nations.h"
extern double PROF_T[8];
static World W;
int main(void){
 World_GenMundi(&W,42,WH_LIGADA,1,Nat_Find("GBR"));
 struct timespec a,b;clock_gettime(CLOCK_MONOTONIC,&a);
 int N=365*10;
 for(int d=0;d<N;d++)World_Day(&W);
 clock_gettime(CLOCK_MONOTONIC,&b);
 double tot=(b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6;
 static const char *const NM[7]={"City_Day","WEco_Week","WDip_Day","AI","Fronts","Hist_Day","Rnd_Day"};
 printf("total %.3f ms/dia\n",tot/N);
 for(int k=0;k<7;k++)printf("  %-10s %.4f ms/dia\n",NM[k],PROF_T[k]/N);
 return 0;
}
