#include "market.h"
#include "diplomacy.h"

// OH MY VICTORIA DS! v0.6 — mercados interno (por faccao) + externo (unico).
// Sem float, sem malloc. Precos sao ouro inteiro (nao Q8 fracionado; o
// comentario antigo "Q8" era aspiracao — na pratica usamos int para caber
// no painel de 2 digitos e manter determinismo). Sup/Dem acumulam no dia e
// Mkt_CloseDay ajusta: p += (dem-sup)*k>>4, clamp base/2..base*3.

static const s16 BASE[GOOD_COUNT] = {
  2,  // GRAIN grao — base alimentar
  4,  // CLOTH roupa
  6,  // FURN  movel
  10, // ARMS  arma
  12, // LUX   luxo
  6,  // TOOLS ferramentas (intermediario industrial)
  3,  // COAL  carvao (energia)
  8,  // WINE  vinho (luxo rural)
};

// Sensibilidade oferta/demanda (k). Grao reage rapido, armas/luxo devagar.
static int KOf(u8 good){
  switch(good){
    case GOOD_GRAIN: return 12;
    case GOOD_COAL:  return 8;
    case GOOD_CLOTH: return 6;
    case GOOD_FURN:  return 6;
    case GOOD_TOOLS: return 6;
    case GOOD_WINE:  return 5;
    case GOOD_ARMS:  return 3;
    case GOOD_LUX:   return 3;
    default: return 6;
  }
}

void Mkt_Init(GameState *g){
  for(int f=0;f<MAX_FACTIONS;f++){
    for(int gd=0;gd<GOOD_COUNT;gd++){
      g->factions[f].priceInt[gd]=BASE[gd];
      g->factions[f].supInt[gd]=0;
      g->factions[f].demInt[gd]=0;
      if(f) g->factions[f].stock[gd]=START_STOCK_G;
      else g->factions[f].stock[gd]=0;
    }
  }
  for(int gd=0;gd<GOOD_COUNT;gd++){
    g->priceExt[gd]=BASE[gd];
    g->supExt[gd]=0;
    g->demExt[gd]=0;
  }
}

int Mkt_Base(u8 good){ return good<GOOD_COUNT?BASE[good]:2; }
int Mkt_IntPrice(GameState *g,u8 f,u8 good){
  if(f>=MAX_FACTIONS||good>=GOOD_COUNT) return 2;
  int p=g->factions[f].priceInt[good];
  return p<1?1:p;
}
int Mkt_ExtPrice(GameState *g,u8 good){
  if(good>=GOOD_COUNT) return 2;
  int p=g->priceExt[good];
  return p<1?1:p;
}
void Mkt_IntSup(GameState *g,u8 f,u8 good,int q){
  if(f>=MAX_FACTIONS||good>=GOOD_COUNT) return;
  g->factions[f].supInt[good]+=q;
}
void Mkt_IntDem(GameState *g,u8 f,u8 good,int q){
  if(f>=MAX_FACTIONS||good>=GOOD_COUNT) return;
  g->factions[f].demInt[good]+=q;
}
void Mkt_ExtSup(GameState *g,u8 good,int q){
  if(good>=GOOD_COUNT) return;
  g->supExt[good]+=q;
}
void Mkt_ExtDem(GameState *g,u8 good,int q){
  if(good>=GOOD_COUNT) return;
  g->demExt[good]+=q;
}

static int ClampPrice(int p,int base){
  int lo=base/2; if(lo<1) lo=1;
  int hi=base*3; if(hi<lo+1) hi=lo+1;
  if(p<lo) p=lo;
  if(p>hi) p=hi;
  return p;
}

// Tarifa: 15% base, 0 se tem ALLY (uniao aduaneira) com alguem.
static int TariffPct(GameState *g,u8 me){
  if(me>=MAX_FACTIONS) return 15;
  for(int o=1;o<MAX_FACTIONS;o++){
    if(o==me) continue;
    if(g->dip[me][o]==DIP_ALLY) return 0;
  }
  return 15;
}

void Mkt_CloseDay(GameState *g){
  // 1) interno por faccao
  for(int f=1;f<MAX_FACTIONS;f++){
    if(!g->factions[f].alive) continue;
    for(int gd=0;gd<GOOD_COUNT;gd++){
      int p=g->factions[f].priceInt[gd];
      int d=g->factions[f].demInt[gd]-g->factions[f].supInt[gd];
      p+=(d*KOf((u8)gd))>>4;
      g->factions[f].priceInt[gd]=(s16)ClampPrice(p,BASE[gd]);
      g->factions[f].supInt[gd]=0;
      g->factions[f].demInt[gd]=0;
    }
  }
  // 2) externo unico
  for(int gd=0;gd<GOOD_COUNT;gd++){
    int p=g->priceExt[gd];
    int d=g->demExt[gd]-g->supExt[gd];
    p+=(d*KOf((u8)gd))>>4;
    g->priceExt[gd]=(s16)ClampPrice(p,BASE[gd]);
    g->supExt[gd]=0;
    g->demExt[gd]=0;
  }
  // 3) uniao aduaneira (ALLY): precos internos dos aliados convergem p/ media.
  for(int a=1;a<MAX_FACTIONS;a++)for(int b=a+1;b<MAX_FACTIONS;b++){
    if(g->dip[a][b]!=DIP_ALLY) continue;
    if(!g->factions[a].alive||!g->factions[b].alive) continue;
    for(int gd=0;gd<GOOD_COUNT;gd++){
      int m=(g->factions[a].priceInt[gd]+g->factions[b].priceInt[gd]+1)/2;
      g->factions[a].priceInt[gd]=(s16)m;
      g->factions[b].priceInt[gd]=(s16)m;
    }
  }
}

bool Mkt_BuyInt(GameState *g,u8 me,u8 good,int qty){
  if(me>=MAX_FACTIONS||good>=GOOD_COUNT||qty<=0) return false;
  Faction *F=&g->factions[me];
  if(!F->alive) return false;
  int p=Mkt_IntPrice(g,me,good);
  int cost=p*qty;
  if(F->treasury<cost) return false;
  if(F->stock[good]>9000) return false;
  F->treasury-=cost;
  F->stock[good]+=qty;
  Mkt_IntDem(g,me,good,qty);
  return true;
}

bool Mkt_BuyExt(GameState *g,u8 me,u8 good,int qty){
  if(me>=MAX_FACTIONS||good>=GOOD_COUNT||qty<=0) return false;
  Faction *F=&g->factions[me];
  if(!F->alive) return false;
  int p=Mkt_ExtPrice(g,good);
  int tp=TariffPct(g,me);
  int cost=(p*qty*(100+tp))/100;
  if(F->treasury<cost) return false;
  if(F->stock[good]>9000) return false;
  F->treasury-=cost;
  F->stock[good]+=qty;
  Mkt_ExtDem(g,good,qty);
  return true;
}

bool Mkt_SellExt(GameState *g,u8 me,u8 good,int qty){
  if(me>=MAX_FACTIONS||good>=GOOD_COUNT||qty<=0) return false;
  Faction *F=&g->factions[me];
  if(!F->alive) return false;
  if(F->stock[good]<qty) return false;
  // CUSTOMS no imperio: +1 ouro por unidade exportada (bônus, ver economy.c).
  int bonus=0;
  // conta alfandegas sem varrer mapa todo dia? varre: 768 tiles e' barato (single-pass).
  for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
    Tile *t=&g->tiles[y][x];
    if(t->owner==me&&t->building==BLD_CUSTOMS){ bonus=qty; break; }
  }
  int p=Mkt_ExtPrice(g,good);
  F->stock[good]-=qty;
  F->treasury+=p*qty+bonus;
  if(F->treasury>MAX_RES) F->treasury=MAX_RES;
  Mkt_ExtSup(g,good,qty);
  return true;
}

const char *Mkt_Name(u8 good){
  switch(good){
    case GOOD_GRAIN: return "grao";
    case GOOD_CLOTH: return "roupa";
    case GOOD_FURN:  return "movel";
    case GOOD_ARMS:  return "arma";
    case GOOD_LUX:   return "luxo";
    case GOOD_TOOLS: return "ferram";
    case GOOD_COAL:  return "carvao";
    case GOOD_WINE:  return "vinho";
    default: return "?";
  }
}
const char *Mkt_Short(u8 good){
  switch(good){
    case GOOD_GRAIN: return "GR";
    case GOOD_CLOTH: return "RO";
    case GOOD_FURN:  return "MO";
    case GOOD_ARMS:  return "AR";
    case GOOD_LUX:   return "LX";
    case GOOD_TOOLS: return "FE";
    case GOOD_COAL:  return "CA";
    case GOOD_WINE:  return "VI";
    default: return "??";
  }
}
