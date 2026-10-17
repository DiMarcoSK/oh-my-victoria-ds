#include "combat.h"
#include "diplomacy.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "rng.h"
static int MarchDays(Tile *d){
 int d0=2;
 if(d->terrain==TER_HILLS)d0+=1;
 if(d->terrain==TER_DESERT||d->terrain==TER_SWAMP)d0+=1;
 if(d->building==BLD_FORT)d0+=2;
 return d0;
}
int Combat_Pow(GameState *g,Tile *t,bool atk,u8 fac){
 int p=t->troops*2+Map_Def(t->terrain);
 if(!atk){
  if(t->building==BLD_WALL)p+=3;
  if(t->building==BLD_FORT)p+=8;
  if(t->site==SITE_CURSED)p-=1;
 }else{
  if(t->terrain==TER_HILLS)p-=1;
  if(t->terrain==TER_DESERT)p-=1;
  if(t->site==SITE_CURSED)p-=1;
 }
 Faction*F=&g->factions[fac];
 if(F->morale>=70)p+=1; // tropa bem paga e alimentada luta melhor
 if(F->gov==GOV_MONARCHY&&atk)p+=1; // hosts reais atacam melhor
 if(F->gov==GOV_DEMOCRACY&&atk)p-=1; // milicia cidada ataca pior
 if(F->gov==GOV_THEOCRACY&&!atk)p+=1; // fe defende o lar
 if(!F->isPlayer)p=(p*Eco_BotQ7(g->diff))>>7;
 if(F->stock[GOOD_ARMS]>20)p+=2;
 // caos divertido: +-1 por RNG (dado de batalha)
 p+=(Rng_Range(g,3)-1);
 if(p<0)p=0;
 return p;
}
bool Combat_Order(GameState *g,int sx,int sy,int dx,int dy){
 Tile*s=Map_Get(g,sx,sy),*d=Map_Get(g,dx,dy);
 if(!s||!d)return false;
 int ax=sx>dx?sx-dx:dx-sx, ay=sy>dy?sy-dy:dy-sy;
 if(ax+ay!=1)return false;
 if(!tileWalk(d->terrain)||d->owner==s->owner||s->troops<2)return false;
 if(d->owner!=0&&!Dip_CanAtk(g,s->owner,d->owner))return false;
 if(s->moveCd>0||s->atkCd>0)return false;
 s->dstX=(u8)dx;s->dstY=(u8)dy;
 s->moveCd=(u8)MarchDays(d);
 g->dirty=true;return true;
}
void Combat_Day(GameState *g){
 for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
  Tile*s=&g->tiles[y][x];
  if(s->moveCd==0||s->dstX==0xFF)continue;
  if(s->owner>=MAX_FACTIONS||s->troops<2){s->moveCd=0;s->dstX=0xFF;continue;}
  s->moveCd--;
  if(s->moveCd>0)continue;
  int dx=s->dstX, dy=s->dstY; s->dstX=0xFF;
  Tile*d=Map_Get(g,dx,dy);
  if(!d||!tileWalk(d->terrain)||d->owner==s->owner)continue;
  if(d->owner!=0&&!Dip_CanAtk(g,s->owner,d->owner))continue;
  if(d->owner==0){
   if(s->troops>d->troops){d->owner=s->owner;
    int r=s->troops-d->troops-1; d->troops=r<1?1:(r>12?12:(u8)r); s->troops=1;
    d->happiness=55;
    if(d->site==SITE_RUINARM)g->factions[s->owner].stock[GOOD_ARMS]+=4;
    g->dirty=true;}
   continue;
  }
  int a=Combat_Pow(g,s,true,s->owner), f=Combat_Pow(g,d,false,d->owner)+2;
  if(a>f){d->owner=s->owner;int r=s->troops-d->troops-1;
   d->troops=r<1?1:(r>12?12:(u8)r);s->troops=1;
   d->happiness=d->happiness>20?d->happiness-20:0;g->dirty=true;}
  else{if(d->troops>1)d->troops--;s->troops=1;
   if(d->happiness<100)d->happiness++;
    g->dirty=true;}
 }
 Map_Recalc(g);
}
