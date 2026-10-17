#include "diplomacy.h"
#include "map.h"
void Dip_Init(GameState *g){
 for(int a=0;a<MAX_FACTIONS;a++)for(int b=0;b<MAX_FACTIONS;b++)
  g->dip[a][b]=(a==b)?DIP_PEACE:DIP_WAR;
}
u8 Dip_Get(GameState *g,u8 a,u8 b){
 if(a>=MAX_FACTIONS||b>=MAX_FACTIONS)return DIP_WAR;
 if(a==0||b==0)return DIP_WAR;
 return g->dip[a][b];
}
void Dip_Set(GameState *g,u8 a,u8 b,u8 s){
 if(a>=MAX_FACTIONS||b>=MAX_FACTIONS)return;
 g->dip[a][b]=g->dip[b][a]=s; g->dirty=true;
}
bool Dip_CanAtk(GameState *g,u8 a,u8 d){return d==0?true:Dip_Get(g,a,d)==DIP_WAR;}
void Dip_Day(GameState *g){
 if((g->day&1)==0)return;
 for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){Tile*t=&g->tiles[y][x];
  if(t->owner==0)continue;
  for(int d=0;d<4;d++){Tile*n=Map_Get(g,x+(d==0)-(d==1),y+(d==2)-(d==3));
   if(!n||n->owner==0||n->owner==t->owner)continue;
   u8 a=t->owner,b=n->owner;
   if(g->dip[a][b]!=DIP_WAR){if(g->factions[a].rel[b]<100)g->factions[a].rel[b]++;}
   else if(g->factions[a].rel[b]>0)g->factions[a].rel[b]--;
  }
 }
}
bool Dip_Peace(GameState *g,u8 me,u8 to){
 if(me>=MAX_FACTIONS||to>=MAX_FACTIONS||me==to)return false;
 int rel=g->factions[me].rel[to];
 int cost=40-((rel*24)/100); if(cost<12)cost=12;
 if(g->factions[me].treasury<cost)return false;
 g->factions[me].treasury-=cost;Dip_Set(g,me,to,DIP_PEACE);
 if(g->factions[me].rel[to]<100)g->factions[me].rel[to]+=5;
 return true;
}
bool Dip_Ally(GameState *g,u8 me,u8 to){
 // alianca = uniao aduaneira: exige rel>=70 + presente 100 ouro
 if(me>=MAX_FACTIONS||to>=MAX_FACTIONS||me==to)return false;
 if(g->factions[me].rel[to]<70)return false;
 if(g->factions[me].treasury<100)return false;
 g->factions[me].treasury-=100;g->factions[to].treasury+=100;
 Dip_Set(g,me,to,DIP_ALLY);
 return true;
}
const char *Dip_Name(u8 s){if(s==DIP_WAR)return "guerra";if(s==DIP_PEACE)return "paz";
 if(s==DIP_ALLY)return "UNIAO";
 return "?";}
