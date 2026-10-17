// simhash.c — determinismo entre plataformas (ADR-36): mundo 1800 (seed 42,
// historia ligada) + 365 dias, imprime World_Hash a cada 73 dias. O MESMO
// numero tem que sair no PC (x86) e no ARM9 emulado (Thumb + trechos ARM do
// hot.h). Otimizou a simulacao? sh tools/armbench/simhash.sh
#include <stdio.h>
#include "world.h"
#include "nations.h"
static World W;
int main(void){
 World_GenMundi(&W,42,WH_LIGADA,1,Nat_Find("GBR"));
 printf("gerado: %08x\n",(unsigned)World_Hash(&W));
 for(int d=1;d<=365;d++){World_Day(&W);if(d%73==0)printf("dia %3d: %08x\n",d,(unsigned)World_Hash(&W));}
 return 0;
}
