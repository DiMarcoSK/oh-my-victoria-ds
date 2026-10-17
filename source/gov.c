#include "gov.h"

// OH MY VICTORIA DS! v0.7 — governos. Troca barata de implementar (2 bytes
// na Faction), efeito grande no jogo: cada governo muda imposto, felicidade,
// guerra e prestigio. Sem float, sem RNG aqui (deterministico).

void Gov_Init(GameState *g){
 for(int i=0;i<MAX_FACTIONS;i++){
  g->factions[i].gov=GOV_MONARCHY;
  g->factions[i].govCd=0;
 }
 // sabor: o Sul comeca teocratico (joga diferente desde o dia 1).
 g->factions[3].gov=GOV_THEOCRACY;
}

bool Gov_Switch(GameState *g,u8 f,u8 gov){
 if(f==0||f>=MAX_FACTIONS||gov>=GOV_COUNT) return false;
 Faction *F=&g->factions[f];
 if(!F->alive||F->gov==gov||F->govCd>0) return false;
 if(F->treasury<GOV_COST) return false;
 F->treasury-=GOV_COST;
 F->gov=gov;
 F->govCd=GOV_COOLDOWN;
 g->dirty=true;
 return true;
}

bool Gov_Cycle(GameState *g,u8 f){
 if(f>=MAX_FACTIONS) return false;
 return Gov_Switch(g,f,(u8)((g->factions[f].gov+1)%GOV_COUNT));
}

const char *Gov_Name(u8 gov){
 switch(gov){
  case GOV_MONARCHY: return "MONARQUIA";
  case GOV_DEMOCRACY: return "DEMOCRACIA";
  case GOV_THEOCRACY: return "TEOCRACIA";
  default: return "?";
 }
}

const char *Gov_Short(u8 gov){
 switch(gov){
  case GOV_MONARCHY: return "MON";
  case GOV_DEMOCRACY: return "DEM";
  case GOV_THEOCRACY: return "TEO";
  default: return "???";
 }
}
