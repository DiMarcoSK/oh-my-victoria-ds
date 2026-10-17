#include "rng.h"
void Rng_Init(GameState *g,u32 seed){g->rng=seed?seed:1;}
u32 Rng_Next(GameState *g){
 u32 x=g->rng; if(x==0)x=1;
 x^=x<<13;x^=x>>17;x^=x<<5; g->rng=x; return x;
}
int Rng_Range(GameState *g,int n){
 if(n<=0)return 0;
 return (int)(Rng_Next(g)%(u32)n);
}
int Rng_Chance(GameState *g,int pct){
 if(pct<=0)return 0;
 if(pct>=100)return 1;
 return (int)(Rng_Next(g)%100)<pct;
}
