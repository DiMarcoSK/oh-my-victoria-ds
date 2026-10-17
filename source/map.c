#include "map.h"
#include "rng.h"
void Map_Gen(GameState *g,u32 seed){
 Rng_Init(g,seed);
 for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
  Tile*t=&g->tiles[y][x];
  t->owner=0;t->troops=0;t->terrain=TER_PLAINS;t->building=0;t->site=0;
  t->happiness=60;t->popA=0x80;t->popB=0x00;
  t->moveCd=0;t->atkCd=0;t->dstX=0xFF;t->dstY=0xFF;
  int r=Rng_Range(g,100);
  if(r<8)t->terrain=TER_FOREST;else if(r<14)t->terrain=TER_HILLS;
  else if(r<19)t->terrain=TER_WATER;else if(r<22)t->terrain=TER_MOUNTAIN;
  else if(r<26)t->terrain=TER_DESERT;else if(r<29)t->terrain=TER_SWAMP;
  else if(r<31)t->terrain=TER_VOLCANO;else if(r<34)t->terrain=TER_RUINS;
  else if(r<36)t->terrain=TER_OASIS;
  if(!tileWalk(t->terrain)){t->popA=0;t->popB=0;}
  int s=Rng_Range(g,100);
  if(tileWalk(t->terrain)){
   if(s<3)t->site=SITE_GOLDMINE;else if(s<6)t->site=SITE_FERTILE;
   else if(s<8)t->site=SITE_IRON;else if(s<9)t->site=SITE_RUINARM;
   else if(s<10)t->site=SITE_CURSED;else if(s<13)t->site=SITE_COAL;
   if(t->terrain==TER_RUINS&&t->site==SITE_NONE)t->site=SITE_RUINARM;
  }
  if(t->site==SITE_CURSED&&t->happiness>40)t->happiness=40;
 }
 int cap[3][2]={{2,2},{MAP_W-3,MAP_H-3},{MAP_W-3,2}};
 for(int i=0;i<3;i++)for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  Tile*t=Map_Get(g,cap[i][0]+dx,cap[i][1]+dy);
  if(t){t->terrain=TER_PLAINS;t->site=SITE_NONE;t->troops=0;}
 }
 // v0.6 sociedade: capital 3x3 propria (9 tiles) p/ caber cadeia inicial.
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  Tile*a=Map_Get(g,2+dx,2+dy);
  if(a){a->owner=1;a->popA=0x22;a->popB=0x00;a->happiness=60;}
  Tile*b=Map_Get(g,MAP_W-3+dx,MAP_H-3+dy);
  if(b){b->owner=2;b->popA=0x22;b->popB=0x00;b->happiness=60;}
  Tile*c=Map_Get(g,MAP_W-3+dx,2+dy);
  if(c){c->owner=3;c->popA=0x22;c->popB=0x00;c->happiness=60;}
 }
 g->tiles[2][2].troops=5;g->tiles[2][2].building=BLD_FARM;
 g->tiles[1][2].building=BLD_FARM;g->tiles[2][1].building=BLD_SAWMILL;
 g->tiles[MAP_H-3][MAP_W-3].troops=5;g->tiles[MAP_H-3][MAP_W-3].building=BLD_FARM;
 g->tiles[MAP_H-4][MAP_W-3].building=BLD_FARM;g->tiles[MAP_H-3][MAP_W-4].building=BLD_SAWMILL;
 g->tiles[2][MAP_W-3].troops=4;g->tiles[2][MAP_W-3].building=BLD_FARM;
 g->tiles[1][MAP_W-3].building=BLD_SAWMILL;g->tiles[2][MAP_W-4].building=BLD_MINE;
 for(int i=0;i<12;i++){int x=4+Rng_Range(g,MAP_W-8),y=4+Rng_Range(g,MAP_H-8);
  Tile*t=Map_Get(g,x,y);
  if(t&&t->owner==0&&tileWalk(t->terrain))t->troops=(u8)(1+Rng_Range(g,3));}
}
void Map_Init(GameState *g){
 for(int i=0;i<MAX_FACTIONS*GOOD_COUNT;i++){}
 for(int i=0;i<MAX_FACTIONS;i++){g->factions[i].id=(u8)i;
  g->factions[i].alive=(i<4);}
 g->factions[1].isPlayer=true;
 Map_Gen(g,DEFAULT_SEED);
 for(int i=0;i<MAX_FACTIONS;i++){Faction*f=&g->factions[i];
  f->treasury=i?START_TREASURY:0;f->taxRate=1;f->morale=60;f->literacy=10;f->militancy=0;
  for(int j=0;j<MAX_FACTIONS;j++){f->rel[j]=50;f->tariff[j]=15;}
  for(int gd=0;gd<GOOD_COUNT;gd++)f->stock[gd]=i?START_STOCK_G:0;}
 g->day=1;g->selX=2;g->selY=2;g->camX=0;g->camY=0;
 g->diff=DIFF_NORMAL;g->speed=2;g->weather=0;g->evtCd=15;g->lastEvt=0;
 g->status[0]=0;g->evtMsg[0]=0;
 g->dirty=true;g->inMenu=true;Map_Recalc(g);
}
Tile *Map_Get(GameState *g,int x,int y){return tileInside(x,y)?&g->tiles[y][x]:0;}
void Map_Recalc(GameState *g){
 for(int i=0;i<MAX_FACTIONS;i++){g->factions[i].tilesOwned=0;
  g->factions[i].troopsTotal=0;g->factions[i].popTotal=0;}
 for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){Tile*t=&g->tiles[y][x];
  if(t->owner<MAX_FACTIONS){g->factions[t->owner].tilesOwned++;
   g->factions[t->owner].troopsTotal+=t->troops;
   g->factions[t->owner].popTotal+=(u8)(popGet(t->popA,true)+popGet(t->popA,false)
    +popGet(t->popB,true)+popGet(t->popB,false));}}}
int Map_Def(u8 t){if(t==TER_FOREST)return 1;if(t==TER_HILLS)return 2;
 if(t==TER_SWAMP)return -1;
 if(t==TER_RUINS)return 1;
 return 0;}
