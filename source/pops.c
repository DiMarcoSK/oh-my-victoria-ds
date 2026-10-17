#include "pops.h"
#include "market.h"
#include "rng.h"

// OH MY VICTORIA DS! v0.6 — 4 POPs com demandas distintas (Vic-like lite).
// peas=camponeses, work=operarios, burg=burgueses, nobl=nobreza.
// Cada tile guarda 4 nibbles 0..15 (popA=peas|work, popB=burg|nobl).
// Felicidade = % da demanda atendida +- humor RNG +- clima/imposto.
// Crescimento/promocao exige grao>=80%; fome perde pop e sobe militancia.

int Pop_Need(u8 pop,u8 good){
  switch(pop){
    case POP_PEAS:
      return good==GOOD_GRAIN?2:0;
    case POP_WORK:
      if(good==GOOD_GRAIN) return 2;
      if(good==GOOD_CLOTH) return 1;
      if(good==GOOD_COAL)  return 1;
      return 0;
    case POP_BURG:
      if(good==GOOD_GRAIN) return 1;
      if(good==GOOD_FURN)  return 1;
      if(good==GOOD_WINE)  return 1;
      if(good==GOOD_LUX)   return 1;
      if(good==GOOD_TOOLS) return 1;
      return 0;
    case POP_NOBL:
      if(good==GOOD_ARMS) return 1;
      if(good==GOOD_LUX)  return 2;
      if(good==GOOD_WINE) return 1;
      return 0;
    default: return 0;
  }
}

int Pop_Count(Tile *t,u8 pop){
  if(!t) return 0;
  if(pop==POP_PEAS) return popGet(t->popA,false);
  if(pop==POP_WORK) return popGet(t->popA,true);
  if(pop==POP_BURG) return popGet(t->popB,false);
  if(pop==POP_NOBL) return popGet(t->popB,true);
  return 0;
}

void Pop_Add(Tile *t,u8 pop,int dq){
  if(!t) return;
  if(pop==POP_PEAS){ int v=Pop_Count(t,pop)+dq; t->popA=popSet(t->popA,false,(u8)v); }
  else if(pop==POP_WORK){ int v=Pop_Count(t,pop)+dq; t->popA=popSet(t->popA,true,(u8)v); }
  else if(pop==POP_BURG){ int v=Pop_Count(t,pop)+dq; t->popB=popSet(t->popB,false,(u8)v); }
  else if(pop==POP_NOBL){ int v=Pop_Count(t,pop)+dq; t->popB=popSet(t->popB,true,(u8)v); }
}

void Pop_Day(GameState *g){
  for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
    Tile *t=&g->tiles[y][x];
    if(!tileWalk(t->terrain)) continue;
    if(t->owner>=MAX_FACTIONS) continue;
    u8 f=t->owner;
    bool owned=f!=0;
    Faction *F=owned?&g->factions[f]:0;

    int peas=Pop_Count(t,POP_PEAS), work=Pop_Count(t,POP_WORK);
    int burg=Pop_Count(t,POP_BURG), nobl=Pop_Count(t,POP_NOBL);
    int total=peas+work+burg+nobl;
    if(total<=0) continue;

    // demanda do tile (unidades/dia) por bem
    int dem[GOOD_COUNT]={0};
    for(int p=0;p<POP_COUNT;p++){
      int n=Pop_Count(t,(u8)p);
      if(n<=0) continue;
      for(int gd=0;gd<GOOD_COUNT;gd++)
        dem[gd]+=n*Pop_Need((u8)p,(u8)gd);
    }
    // registra demanda no mercado interno (move preco) — escala /4 p/ nao explodir
    if(owned){
      for(int gd=0;gd<GOOD_COUNT;gd++)
        if(dem[gd]>0) Mkt_IntDem(g,f,(u8)gd,dem[gd]/4);
    }

    // atendimento: fracao do estoque da faccao sobre a demanda (0..100%).
    // grainHave/Need capturados ANTES de consumir (fome e por grao).
    int grainHave=0,grainNeed=0;
    if(owned&&F){ grainHave=F->stock[GOOD_GRAIN]; grainNeed=dem[GOOD_GRAIN]/4; if(dem[GOOD_GRAIN]>0&&grainNeed<1) grainNeed=1; }
    // sat ponderada: grao vale 3x (subsistencia sustenta; luxo da bonus).
    int sat=100;
    if(owned&&F){
      int sumDem=0,sumMet=0;
      for(int gd=0;gd<GOOD_COUNT;gd++){
        if(dem[gd]<=0) continue;
        int w=(gd==GOOD_GRAIN)?3:1;
        sumDem+=dem[gd]*w;
        int have=F->stock[gd]; if(have<0) have=0;
        // consome de verdade (limitado a dem/4 p/ nao zerar estoque num dia)
        int take=dem[gd]/4; if(take<1&&dem[gd]>0) take=1;
        if(have>=take){ F->stock[gd]-=take; sumMet+=dem[gd]*w; }
        else if(have>0){ F->stock[gd]=0; sumMet+=(dem[gd]*have*w)/take; }
      }
      if(sumDem>0) sat=(sumMet*100)/sumDem;
      if(sat>100) sat=100;
      if(sat<0) sat=0;
    } else {
      // neutro: vive do terreno (60% base)
      sat=60;
    }

    // felicidade nova = satisfacao +- humor RNG +- clima/imposto/local
    int mood=Rng_Range(g,13)-6; // -6..+6
    int h=sat+mood;
    if(g->weather==1) h-=5;      // seca deprime
    else if(g->weather==2) h+=3; // chuva anima campo
    if(t->site==SITE_CURSED) h-=10;
    if(t->site==SITE_FERTILE||t->site==SITE_GOLDMINE) h+=3;
    if(t->building==BLD_THEATER||t->building==BLD_SHRINE) h+=5;
    if(t->building==BLD_HOSPITAL) h+=3;
    if(owned&&F){
      if(F->taxRate>=2) h-=8;
      else if(F->taxRate==1) h-=3;
      if(F->gov==GOV_DEMOCRACY) h+=6; // voz do povo: felicidade civica
      if(F->gov==GOV_MONARCHY&&F->taxRate>=2) h-=4; // coroa pesada: -12 no max
      if(F->militancy>60) h-=5;
    }
    // suaviza: move 1/4 em direcao ao alvo (evita serrote)
    int cur=t->happiness;
    int nxt=cur+((h-cur)/4);
    if(nxt<0) nxt=0;
    if(nxt>100) nxt=100;
    t->happiness=(u8)nxt;

    // crescimento / fome / promocao (so em tiles habitados de faccao)
    // fome e por GRAO (estoque pre-consumo vs necessidade), nao por sat global.
    if(owned&&F){
      int grainSat=100;
      if(grainNeed>0) grainSat=(grainHave*100)/(grainNeed*4+1);
      if(grainSat>100) grainSat=100;
      if(grainSat<0) grainSat=0;
      if(grainSat>=80){
        // cresce + promove se alfabetizado e feliz
        if((g->day%3)==0){
          if(peas<15&&(Rng_Chance(g,30))) Pop_Add(t,POP_PEAS,1);
          else if(work<15&&(Rng_Chance(g,20))) Pop_Add(t,POP_WORK,1);
        }
        if(t->happiness>65&&F->literacy>30){
          if(peas>4&&work<15&&Rng_Chance(g,10)){ Pop_Add(t,POP_PEAS,-1); Pop_Add(t,POP_WORK,1); }
          else if(work>4&&burg<15&&Rng_Chance(g,8)){ Pop_Add(t,POP_WORK,-1); Pop_Add(t,POP_BURG,1); }
          else if(burg>3&&nobl<15&&Rng_Chance(g,5)){ Pop_Add(t,POP_BURG,-1); Pop_Add(t,POP_NOBL,1); }
        }
        if(t->building==BLD_HOSPITAL&&Rng_Chance(g,15)){
          if(peas<15) Pop_Add(t,POP_PEAS,1);
        }
      } else if(grainSat<40){
        // fome: perde pop, moral cai, militancia sobe (risco de revolta em events.c)
        if(Rng_Chance(g,25)){
          if(peas>0) Pop_Add(t,POP_PEAS,-1);
          else if(work>0) Pop_Add(t,POP_WORK,-1);
        }
        if(F->morale>0&&Rng_Chance(g,20)) F->morale--;
        if(F->militancy<100) F->militancy++;
      }
      // militancia cai devagar se povo feliz
      if(t->happiness>70&&F->militancy>0&&Rng_Chance(g,20)) F->militancy--;
    }
  }
}
