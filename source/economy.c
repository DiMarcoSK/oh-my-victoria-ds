#include "economy.h"
#include "map.h"
#include "market.h"
#include "rng.h"

// OH MY VICTORIA DS! v0.6 — 23 obras em 5 cadeias (sociedade completa).
// Rural(4) -> Industria(6) -> Urbana(5) -> Estado(5) -> Mar(3).
// Regra de throughput: se falta insumo, produz proporcionalmente menos
// (nunca negativo). Tudo deterministico via g->rng; sem float.

int Eco_Cost(u8 b){
  switch(b){
    case BLD_FARM:      return 30;
    case BLD_ORCHARD:   return 40;
    case BLD_MINE:      return 50;
    case BLD_COALPIT:   return 45;
    case BLD_SAWMILL:   return 40;
    case BLD_TEXTILE:   return 50;
    case BLD_STEEL:     return 70;
    case BLD_TOOLSHOP:  return 65;
    case BLD_WINERY:    return 55;
    case BLD_GLASSWORKS:return 80;
    case BLD_MARKET:    return 60;
    case BLD_BANK:      return 100;
    case BLD_UNIV:      return 90;
    case BLD_THEATER:   return 70;
    case BLD_HOSPITAL:  return 80;
    case BLD_BARRACKS:  return 60;
    case BLD_WALL:      return 50;
    case BLD_FORT:      return 90;
    case BLD_PORT:      return 80;
    case BLD_SHRINE:    return 40;
    case BLD_DOCKYARD:  return 110;
    case BLD_CUSTOMS:   return 90;
    default: return 9999;
  }
}

const char *Eco_BldName(u8 b){
  switch(b){
    case BLD_FARM: return "FAZEND"; case BLD_ORCHARD: return "POMAR ";
    case BLD_MINE: return "MINA  "; case BLD_COALPIT: return "CARVAO";
    case BLD_SAWMILL: return "SERRA "; case BLD_TEXTILE: return "TEXTIL";
    case BLD_STEEL: return "ACO   "; case BLD_TOOLSHOP: return "OFICIN";
    case BLD_WINERY: return "VINHO "; case BLD_GLASSWORKS: return "VIDRO ";
    case BLD_MARKET: return "MERCAD"; case BLD_BANK: return "BANCO ";
    case BLD_UNIV: return "UNIV  "; case BLD_THEATER: return "TEATRO";
    case BLD_HOSPITAL: return "HOSP  "; case BLD_BARRACKS: return "QUARTL";
    case BLD_WALL: return "MURALH"; case BLD_FORT: return "FORTE ";
    case BLD_PORT: return "PORTO "; case BLD_SHRINE: return "TEMPLO";
    case BLD_DOCKYARD: return "ESTAL "; case BLD_CUSTOMS: return "ALFAND";
    default: return "VAZIO ";
  }
}

const char *Eco_BldDesc(u8 b){
  switch(b){
    case BLD_FARM: return "+8 grao (clima)";
    case BLD_ORCHARD: return "+4 grao +1 vinho";
    case BLD_MINE: return "+3 carvao +ouro se ouro";
    case BLD_COALPIT: return "+4 carvao";
    case BLD_SAWMILL: return "+3 movel (floresta+1)";
    case BLD_TEXTILE: return "2 grao -> 3 roupa";
    case BLD_STEEL: return "2 carvao -> 2 ferram";
    case BLD_TOOLSHOP: return "1 ferram+1 carvao -> 2 arma";
    case BLD_WINERY: return "2 grao -> 2 vinho";
    case BLD_GLASSWORKS: return "1 carvao+1 ferram -> 2 luxo";
    case BLD_MARKET: return "+tesouro comercio";
    case BLD_BANK: return "+juros +prestigio";
    case BLD_UNIV: return "+alfabet +prestigio";
    case BLD_THEATER: return "+feliz +prestigio";
    case BLD_HOSPITAL: return "+pop +feliz";
    case BLD_BARRACKS: return "recruta barato";
    case BLD_WALL: return "+3 def -1 ouro/d";
    case BLD_FORT: return "+8 def -1 ouro/d";
    case BLD_PORT: return "exporta +oferta ext";
    case BLD_SHRINE: return "+feliz -militancia";
    case BLD_DOCKYARD: return "+export +prestigio";
    case BLD_CUSTOMS: return "+1/ouro exportado";
    default: return "-";
  }
}

const char *Eco_DiffName(u8 d){
  switch(d){
    case DIFF_EASY: return "FACIL";
    case DIFF_NORMAL: return "NORMAL";
    case DIFF_HARD: return "DIFICIL";
    case DIFF_INSANE: return "INSANO";
    default: return "?";
  }
}

// Multiplicador Q7 p/ bots (usado em Combat_Pow): facil fraco, insano forte.
int Eco_BotQ7(u8 d){
  if(d==DIFF_EASY) return 96;
  if(d==DIFF_HARD) return 128;
  if(d==DIFF_INSANE) return 144;
  return 112; // NORMAL
}

void Eco_Tax(GameState *g,u8 f,u8 r){
  if(f>=MAX_FACTIONS) return;
  if(r>2) r=2;
  g->factions[f].taxRate=r;
  g->dirty=true;
}

bool Eco_Build(GameState *g,int x,int y,u8 b){
  Tile *t=Map_Get(g,x,y);
  if(!t||b==BLD_NONE||b>=BLD_COUNT) return false;
  if(!tileWalk(t->terrain)) return false;
  if(t->building!=BLD_NONE) return false;
  if(t->owner==0||t->owner>=MAX_FACTIONS) return false;
  Faction *F=&g->factions[t->owner];
  if(!F->alive) return false;
  int c=Eco_Cost(b);
  if(F->treasury<c) return false;
  F->treasury-=c;
  t->building=b;
  g->dirty=true;
  return true;
}

bool Eco_FortUp(GameState *g,int x,int y){
  Tile *t=Map_Get(g,x,y);
  if(!t||t->building!=BLD_WALL) return false;
  if(t->owner==0||t->owner>=MAX_FACTIONS) return false;
  Faction *F=&g->factions[t->owner];
  if(F->treasury<40) return false;
  F->treasury-=40;
  t->building=BLD_FORT;
  g->dirty=true;
  return true;
}

bool Eco_Recruit(GameState *g,int x,int y){
  Tile *t=Map_Get(g,x,y);
  if(!t||t->owner==0||t->owner>=MAX_FACTIONS) return false;
  if(t->troops>=MAX_TROOPS) return false;
  if(!tileWalk(t->terrain)) return false;
  Faction *F=&g->factions[t->owner];
  if(!F->alive) return false;
  bool hasBar=false;
  // quartel no proprio tile OU em vizinho do mesmo dono = recruta barato
  if(t->building==BLD_BARRACKS) hasBar=true;
  for(int d=0;d<4&&!hasBar;d++){
    Tile *n=Map_Get(g,x+(d==0)-(d==1),y+(d==2)-(d==3));
    if(n&&n->owner==t->owner&&n->building==BLD_BARRACKS) hasBar=true;
  }
  int costTreas=hasBar?2:5, costGrain=hasBar?2:4, costArms=hasBar?1:2;
  if(F->treasury<costTreas) return false;
  if(F->stock[GOOD_GRAIN]<costGrain) return false;
  if(F->stock[GOOD_ARMS]<costArms) return false;
  F->treasury-=costTreas;
  F->stock[GOOD_GRAIN]-=costGrain;
  F->stock[GOOD_ARMS]-=costArms;
  t->troops+=2; if(t->troops>MAX_TROOPS) t->troops=MAX_TROOPS;
  if(F->morale<100) F->morale++;
  Mkt_IntDem(g,t->owner,GOOD_GRAIN,costGrain);
  Mkt_IntDem(g,t->owner,GOOD_ARMS,costArms);
  g->dirty=true;
  return true;
}

static void AddStock(Faction *F,u8 good,int q){
  if(q<=0) return;
  F->stock[good]+=q;
  if(F->stock[good]>9999) F->stock[good]=9999;
}

// Producao de UM tile. Retorna prestigio gerado (0/1).
static int Eco_Tile(GameState *g,u8 f,Tile *t){
  Faction *F=&g->factions[f];
  int pres=0;
  int jit=Rng_Range(g,3); // 0..2 p/ variar colheita (deterministico por seed)
  switch(t->building){
    case BLD_FARM: {
      int q=8+(jit-1);
      if(g->weather==1) q-=2;       // seca
      else if(g->weather==2) q+=2;  // chuva
      if(t->site==SITE_FERTILE) q+=2;
      if(t->terrain==TER_DESERT) q-=2;
      if(t->site==SITE_CURSED) q-=2;
      if(q<0) q=0;
      AddStock(F,GOOD_GRAIN,q); Mkt_IntSup(g,f,GOOD_GRAIN,q);
      break; }
    case BLD_ORCHARD:
      AddStock(F,GOOD_GRAIN,4); Mkt_IntSup(g,f,GOOD_GRAIN,4);
      AddStock(F,GOOD_WINE,1);  Mkt_IntSup(g,f,GOOD_WINE,1);
      break;
    case BLD_MINE: {
      int q=3;
      if(t->terrain==TER_HILLS) q+=1;
      if(t->site==SITE_COAL) q+=2;
      if(t->site==SITE_IRON) q+=1;
      AddStock(F,GOOD_COAL,q); Mkt_IntSup(g,f,GOOD_COAL,q);
      if(t->site==SITE_GOLDMINE){ F->treasury+=3; if(F->treasury>MAX_RES) F->treasury=MAX_RES; }
      break; }
    case BLD_COALPIT: {
      int q=4;
      if(t->terrain==TER_FOREST) q+=1;
      if(t->site==SITE_COAL) q+=2;
      AddStock(F,GOOD_COAL,q); Mkt_IntSup(g,f,GOOD_COAL,q);
      break; }
    case BLD_SAWMILL: {
      int q=3+(t->terrain==TER_FOREST?1:0);
      // ferramental na faccao acelera serraria (+1 se tem ferramentas)
      if(F->stock[GOOD_TOOLS]>10) q+=1;
      AddStock(F,GOOD_FURN,q); Mkt_IntSup(g,f,GOOD_FURN,q);
      break; }
    case BLD_TEXTILE: {
      // 2 grao -> 3 roupa; sem grao produz 1 (throughput parcial)
      if(F->stock[GOOD_GRAIN]>=2){ F->stock[GOOD_GRAIN]-=2; AddStock(F,GOOD_CLOTH,3); Mkt_IntSup(g,f,GOOD_CLOTH,3); Mkt_IntDem(g,f,GOOD_GRAIN,2); }
      else if(F->stock[GOOD_GRAIN]>0){ F->stock[GOOD_GRAIN]--; AddStock(F,GOOD_CLOTH,1); Mkt_IntSup(g,f,GOOD_CLOTH,1); Mkt_IntDem(g,f,GOOD_GRAIN,1); }
      break; }
    case BLD_STEEL: {
      // 2 carvao -> 2 ferramentas; bonus ferro/literacia
      int out=2;
      if(t->site==SITE_IRON) out+=1;
      if(F->literacy>50) out+=1;
      if(F->stock[GOOD_COAL]>=2){ F->stock[GOOD_COAL]-=2; AddStock(F,GOOD_TOOLS,out); Mkt_IntSup(g,f,GOOD_TOOLS,out); Mkt_IntDem(g,f,GOOD_COAL,2); }
      else if(F->stock[GOOD_COAL]>0){ F->stock[GOOD_COAL]--; AddStock(F,GOOD_TOOLS,1); Mkt_IntSup(g,f,GOOD_TOOLS,1); Mkt_IntDem(g,f,GOOD_COAL,1); }
      break; }
    case BLD_TOOLSHOP: {
      // 1 ferram + 1 carvao -> 2 armas
      if(F->stock[GOOD_TOOLS]>=1&&F->stock[GOOD_COAL]>=1){
        F->stock[GOOD_TOOLS]--; F->stock[GOOD_COAL]--;
        AddStock(F,GOOD_ARMS,2); Mkt_IntSup(g,f,GOOD_ARMS,2);
        Mkt_IntDem(g,f,GOOD_TOOLS,1); Mkt_IntDem(g,f,GOOD_COAL,1);
      } else if(F->stock[GOOD_TOOLS]>=1){
        F->stock[GOOD_TOOLS]--; AddStock(F,GOOD_ARMS,1);
        Mkt_IntSup(g,f,GOOD_ARMS,1); Mkt_IntDem(g,f,GOOD_TOOLS,1);
      }
      break; }
    case BLD_WINERY: {
      if(F->stock[GOOD_GRAIN]>=2){ F->stock[GOOD_GRAIN]-=2; AddStock(F,GOOD_WINE,2); Mkt_IntSup(g,f,GOOD_WINE,2); Mkt_IntDem(g,f,GOOD_GRAIN,2); }
      else if(F->stock[GOOD_GRAIN]>0){ F->stock[GOOD_GRAIN]--; AddStock(F,GOOD_WINE,1); Mkt_IntSup(g,f,GOOD_WINE,1); Mkt_IntDem(g,f,GOOD_GRAIN,1); }
      break; }
    case BLD_GLASSWORKS: {
      // luxo avancado: precisa carvao+ferramenta e alguma literacia
      if(F->literacy>=20&&F->stock[GOOD_COAL]>=1&&F->stock[GOOD_TOOLS]>=1){
        F->stock[GOOD_COAL]--; F->stock[GOOD_TOOLS]--;
        AddStock(F,GOOD_LUX,2); Mkt_IntSup(g,f,GOOD_LUX,2);
        Mkt_IntDem(g,f,GOOD_COAL,1); Mkt_IntDem(g,f,GOOD_TOOLS,1);
        pres=1;
      }
      break; }
    case BLD_MARKET:
      F->treasury+=(F->gov==GOV_DEMOCRACY)?3:2;
      if(F->treasury>MAX_RES) F->treasury=MAX_RES;
      Mkt_IntDem(g,f,GOOD_CLOTH,1); Mkt_IntDem(g,f,GOOD_FURN,1);
      break;
    case BLD_BANK:
      // teocracia proibe usura: +1 fixo (dizimo vai p/ templo, nao banco)
      F->treasury+=(F->gov==GOV_THEOCRACY)?1:((F->treasury/100)+1);
      if(F->treasury>MAX_RES) F->treasury=MAX_RES;
      if((g->day%5)==0) pres=1;
      break;
    case BLD_UNIV:
      if(F->gov==GOV_DEMOCRACY){ // escola publica: alfabetiza 2/dia
        if(F->literacy<100) F->literacy++;
        if(F->literacy<100) F->literacy++;
      } else if(F->gov!=GOV_MONARCHY||(g->day&1)==0){ // monarquia: censura, 1/2 dias
        if(F->literacy<100) F->literacy++;
      }
      if((g->day%5)==0&&F->gov!=GOV_THEOCRACY) pres=1; // teocracia: saber laico nao da prestigio
      Mkt_IntDem(g,f,GOOD_CLOTH,1);
      break;
    case BLD_THEATER:
      if(t->happiness<100) t->happiness++;
      if((g->day%7)==0) pres=1;
      Mkt_IntDem(g,f,GOOD_WINE,1); Mkt_IntDem(g,f,GOOD_LUX,1);
      break;
    case BLD_HOSPITAL:
      if(t->happiness<100) t->happiness++;
      Mkt_IntDem(g,f,GOOD_TOOLS,1);
      break;
    case BLD_BARRACKS:
      if(F->morale<100) F->morale++;
      Mkt_IntDem(g,f,GOOD_GRAIN,1); Mkt_IntDem(g,f,GOOD_ARMS,1);
      break;
    case BLD_WALL:
      F->treasury-=1;
      break;
    case BLD_FORT:
      F->treasury-=1;
      if((g->day%10)==0) pres=1;
      break;
    case BLD_PORT:
      // porto gera oferta externa (mundo compra) + pequena renda
      Mkt_ExtSup(g,GOOD_GRAIN,2); Mkt_ExtSup(g,GOOD_CLOTH,1);
      F->treasury+=1; if(F->treasury>MAX_RES) F->treasury=MAX_RES;
      break;
    case BLD_SHRINE:
      if(t->happiness<100) t->happiness++;
      if(F->militancy>0) F->militancy--;
      if(F->gov==GOV_THEOCRACY){ // fe oficial: templo rende e acalma
        if(t->happiness<100) t->happiness++;
        if(F->militancy>0) F->militancy--;
        F->treasury+=1; if(F->treasury>MAX_RES) F->treasury=MAX_RES;
        if((g->day%3)==0&&F->morale<100) F->morale++;
      }
      break;
    case BLD_DOCKYARD:
      if(F->stock[GOOD_FURN]>=1){
        F->stock[GOOD_FURN]--;
        F->treasury+=4; if(F->treasury>MAX_RES) F->treasury=MAX_RES;
        Mkt_ExtSup(g,GOOD_FURN,2);
        Mkt_IntDem(g,f,GOOD_FURN,1);
        if((g->day%7)==0) pres=1;
      }
      break;
    case BLD_CUSTOMS:
      F->treasury+=1; if(F->treasury>MAX_RES) F->treasury=MAX_RES;
      break;
    default: break;
  }
  return pres;
}

void Eco_Day(GameState *g,u8 f){
  if(f==0||f>=MAX_FACTIONS) return;
  Faction *F=&g->factions[f];
  if(!F->alive) return;
  int pres=0;
  for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
    Tile *t=&g->tiles[y][x];
    if(t->owner!=f) continue;
    // manutencao militar diaria das tropas no campo
    pres+=Eco_Tile(g,f,t);
  }
  // imposto: renda por pop, custo em felicidade/militancia
  // taxRate 0/1/2 -> 0 / +1 por 4 pops / +1 por 2 pops
  if(F->govCd>0) F->govCd--; // cooldown de troca de governo
  int perPop=f==1?(int)F->taxRate:(g->diff==DIFF_HARD||g->diff==DIFF_INSANE?2:1);
  int income=0;
  if(perPop==1) income=F->popTotal/4;
  else if(perPop>=2) income=F->popTotal/2;
  if(F->gov==GOV_MONARCHY) income=(income*3)/2; // coroa tributa forte
  else if(F->gov==GOV_DEMOCRACY) income=(income*3)/4; // democracia tributa leve
  F->treasury+=income;
  if(F->treasury>MAX_RES) F->treasury=MAX_RES;
  if(F->treasury<0){
    F->treasury=0;
    if(F->morale>0) F->morale--;
    if(F->militancy<100) F->militancy++;
  }
  // salarios das tropas: 1 ouro por 8 tropas/dia (arredonda p/ baixo)
  int wages=F->troopsTotal/8;
  if(wages>0){
    F->treasury-=wages;
    if(F->treasury<0){ F->treasury=0; if(F->morale>0) F->morale--; }
  }
  // valvula: sem fome aguda, militancia cede 1 a cada 5 dias (paz social).
  if((g->day%5)==0&&F->militancy>0&&F->stock[GOOD_GRAIN]>0) F->militancy--;
  if(F->gov==GOV_MONARCHY&&(g->day%5)==0) pres+=1; // pompa real: prestigio da coroa
  F->prestige+=pres;
  if(F->prestige>9999) F->prestige=9999;
}
