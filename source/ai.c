#include "ai.h"
#include "map.h"
#include "economy.h"
#include "combat.h"
#include "diplomacy.h"
#include "market.h"
#include "gov.h"
#include "rng.h"
int AI_Budget(u8 d){if(d==0)return 40;if(d==2)return 120;if(d==3)return 200;return 80;}
static int fX[256],fY[256];
static int FBuild(GameState *g,u8 f){
 int n=0;
 for(int y=0;y<MAP_H&&n<256;y++)for(int x=0;x<MAP_W&&n<256;x++){
  if(g->tiles[y][x].owner!=f)continue;
  for(int d=0;d<4;d++){Tile*t=Map_Get(g,x+(d==0)-(d==1),y+(d==2)-(d==3));
   if(t&&t->owner!=f){fX[n]=x;fY[n]=y;n++;break;}}
 }
 return n;
}
void AI_Day(GameState *g,u8 f){
 if(f==0||f>=MAX_FACTIONS||!g->factions[f].alive)return;
 int bd=AI_Budget(g->diff);
 Faction*F=&g->factions[f];
 int area=(int)(g->day+f)%4;
 if(area==0){
  // eco em cadeia (sociedade completa): cobre gargalos antes de luxo.
  // trade: compra grao/carvao barato, exporta excedente caro.
  if(F->stock[GOOD_GRAIN]<30&&Mkt_IntPrice(g,f,GOOD_GRAIN)<=3)
   Mkt_BuyInt(g,f,GOOD_GRAIN,6);
  if(F->stock[GOOD_COAL]<15&&Mkt_IntPrice(g,f,GOOD_COAL)<=3)
   Mkt_BuyInt(g,f,GOOD_COAL,5);
  if(F->stock[GOOD_FURN]>50)
   Mkt_SellExt(g,f,GOOD_FURN,15);
  if(F->stock[GOOD_ARMS]>40&&Mkt_ExtPrice(g,GOOD_ARMS)>=8)
   Mkt_SellExt(g,f,GOOD_ARMS,6);
  if(F->stock[GOOD_WINE]>40&&Mkt_ExtPrice(g,GOOD_WINE)>=8)
   Mkt_SellExt(g,f,GOOD_WINE,6);
  if(F->stock[GOOD_LUX]>30&&Mkt_ExtPrice(g,GOOD_LUX)>=10)
   Mkt_SellExt(g,f,GOOD_LUX,4);
  for(int y=0;y<MAP_H&&bd>0;y+=3)for(int x=0;x<MAP_W&&bd>0;x+=3){bd--;
   Tile*t=&g->tiles[y][x]; if(t->owner!=f||t->building!=0||!tileWalk(t->terrain))continue;
   if(t->site==SITE_GOLDMINE){Eco_Build(g,x,y,BLD_MINE);return;}
   if(t->site==SITE_COAL){Eco_Build(g,x,y,BLD_COALPIT);return;}
   if(t->site==SITE_IRON){Eco_Build(g,x,y,BLD_STEEL);return;}
   if(t->site==SITE_FERTILE){Eco_Build(g,x,y,BLD_FARM);return;}
   if(F->stock[GOOD_GRAIN]<20){Eco_Build(g,x,y,BLD_FARM);return;}
   if(F->stock[GOOD_COAL]<15){Eco_Build(g,x,y,BLD_COALPIT);return;}
   if(F->stock[GOOD_TOOLS]<10){Eco_Build(g,x,y,BLD_STEEL);return;}
   if(F->stock[GOOD_ARMS]<15){Eco_Build(g,x,y,BLD_TOOLSHOP);return;}
   if(F->stock[GOOD_CLOTH]<15){Eco_Build(g,x,y,BLD_TEXTILE);return;}
   if(F->stock[GOOD_WINE]<10&&F->stock[GOOD_GRAIN]>30){Eco_Build(g,x,y,BLD_WINERY);return;}
   if(F->literacy<40){Eco_Build(g,x,y,BLD_UNIV);return;}
   if(F->stock[GOOD_LUX]<10&&F->literacy>=20){Eco_Build(g,x,y,BLD_GLASSWORKS);return;}
   if(t->terrain==TER_FOREST){Eco_Build(g,x,y,BLD_SAWMILL);return;}
   if(F->treasury>150){Eco_Build(g,x,y,BLD_MARKET);return;}
   Eco_Build(g,x,y,BLD_SAWMILL);return;}
 }else if(area==1){
  int n=FBuild(g,f);
  for(int i=0;i<n&&bd>0;i++){bd--;
   Tile*t=Map_Get(g,fX[i],fY[i]);
   if(t->building==BLD_NONE&&F->treasury>=90){Eco_Build(g,fX[i],fY[i],BLD_FORT);return;}}
  for(int y=0;y<MAP_H&&bd>0;y++)for(int x=0;x<MAP_W&&bd>0;x++){bd--;
   Tile*t=&g->tiles[y][x];
   if(t->owner==f&&t->troops<8){Eco_Recruit(g,x,y);return;}}
 }else if(area==2){
  int th=g->diff==0?4:(g->diff==2?1:2); if(g->diff==3)th=0;
  int tries=g->diff>=2?2:1;
  int n=FBuild(g,f);
  for(int k=0;k<tries;k++){
   int sx=-1,sy=-1,dx=-1,dy=-1,best=-999;
   for(int i=0;i<n&&bd>0;i++){bd--;
    Tile*s=Map_Get(g,fX[i],fY[i]);
    if(s->troops<3||s->moveCd>0)continue;
    for(int d=0;d<4;d++){int nx=fX[i]+(d==0)-(d==1),ny=fY[i]+(d==2)-(d==3);
     Tile*t=Map_Get(g,nx,ny);
     if(!t||t->owner==f||!tileWalk(t->terrain)||!Dip_CanAtk(g,f,t->owner))continue;
     int a=Combat_Pow(g,s,true,f),e=Combat_Pow(g,t,false,t->owner)+2;
     int sc=(a-e)*2;
     if(t->owner==0)sc+=4;
     if(t->site==SITE_GOLDMINE||t->site==SITE_IRON)sc+=4;
     if(t->site==SITE_CURSED)sc-=3;
     if(t->building==BLD_FORT)sc-=6;
     // ousadia divertida: as vezes ataca mesmo perdendo (RNG por dificuldade)
     sc+=Rng_Range(g,g->diff==3?5:3);
     if(sc>best){best=sc;sx=fX[i];sy=fY[i];dx=nx;dy=ny;}
    }
   }
   if(best>=th&&sx>=0)Combat_Order(g,sx,sy,dx,dy);
   else break;
  }
  if(g->diff>=2&&F->tilesOwned<6){
   u8 e=g->factions[2].tilesOwned>=g->factions[3].tilesOwned?2:3;
   if(f!=e)Dip_Peace(g,f,e);
  }
 }else{
  // governo: monarquia em crise de militancia vira teocracia (templo acalma)
  if(F->gov==GOV_MONARCHY&&F->militancy>80) Gov_Switch(g,f,GOV_THEOCRACY);
  // diplomacia: alia com quem tem rel alta (uniao aduaneira)
  for(int o=1;o<MAX_FACTIONS;o++){if(o==f)continue;
   if(F->rel[o]>=70&&g->dip[f][o]==DIP_PEACE)Dip_Ally(g,f,o);}
 }
}
