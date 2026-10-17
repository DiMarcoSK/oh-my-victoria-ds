#include "events.h"
#include "map.h"
#include "market.h"
#include "economy.h"
#include "rng.h"

// OH MY VICTORIA DS! v0.6 — clima + 8 eventos deterministicos por seed.
// Clima: 7%/dia muda (0 normal, 1 seca, 2 chuva). Seca -grao, chuva +grao.
// Eventos: ~1/25 dias em tile aleatorio + cooldown 20d. Efeitos aplicam
// direto em tile/faccao e deixam marcador laranja + mensagem no painel.

const char *Evt_Name(u8 e){
  switch(e){
    case EVT_HARVEST: return "COLHEITA";
    case EVT_PLAGUE: return "PRAGA";
    case EVT_GOLDSTRIKE: return "OURO";
    case EVT_REVOLT: return "REVOLTA";
    case EVT_ARMSDEAL: return "ARSENAL";
    case EVT_STORM: return "TEMPESTADE";
    case EVT_FESTIVAL: return "FESTA";
    case EVT_COALRUSH: return "CARVAO";
    default: return "-";
  }
}

static void SetMsg(GameState *g,const char *m){
  int i=0; while(m[i]&&i<63){ g->evtMsg[i]=m[i]; i++; } g->evtMsg[i]=0;
}

void Evt_Day(GameState *g){
  // 1) clima
  if(Rng_Chance(g,7)){
    g->weather=(u8)Rng_Range(g,3); // 0..2
  }
  // 2) cooldown
  if(g->evtCd>0) g->evtCd--;
  if(g->evtCd>0) return;
  if(!Rng_Chance(g,4)) return; // ~1/25 dias (4% ao dia apos cooldown)

  int x=Rng_Range(g,MAP_W), y=Rng_Range(g,MAP_H);
  Tile *t=Map_Get(g,x,y);
  if(!t||!tileWalk(t->terrain)) return;
  u8 e=(u8)(1+Rng_Range(g,8)); // 1..8
  g->evtX=(u8)x; g->evtY=(u8)y; g->lastEvt=e;
  g->evtCd=20;
  u8 f=t->owner;
  Faction *F=(f<MAX_FACTIONS)?&g->factions[f]:0;

  switch(e){
    case EVT_HARVEST:
      if(F&&f){ F->stock[GOOD_GRAIN]+=30; if(F->stock[GOOD_GRAIN]>9999) F->stock[GOOD_GRAIN]=9999; Mkt_IntSup(g,f,GOOD_GRAIN,10); }
      if(t->happiness<95) t->happiness+=5;
      SetMsg(g,"COLHEITA +30 GRAO"); break;
    case EVT_PLAGUE: {
      int peas=popGet(t->popA,false), work=popGet(t->popA,true);
      int burg=popGet(t->popB,false), nobl=popGet(t->popB,true);
      if(peas+work+burg+nobl>0){
        // perde 2 pop (peas/work primeiro)
        if(peas>0) t->popA=popSet(t->popA,false,(u8)(peas>2?peas-2:0));
        else if(work>0) t->popA=popSet(t->popA,true,(u8)(work>2?work-2:0));
      }
      if(t->happiness>10) t->happiness-=10;
      if(F&&f&&F->morale>0) F->morale--;
      SetMsg(g,"PRAGA -POP -MORAL"); break; }
    case EVT_GOLDSTRIKE:
      if(F&&f){ F->treasury+=60; if(F->treasury>MAX_RES) F->treasury=MAX_RES; }
      t->site=SITE_GOLDMINE;
      SetMsg(g,"OURO +60 +MINA"); break;
    case EVT_REVOLT: {
      // risco maior se militancia alta / felicidade baixa
      int risk=(F&&f)?(int)F->militancy:0;
      if(t->troops>0&&(risk>30||t->happiness<40)){
        if(t->troops>1) t->troops--;
        if(t->happiness>20) t->happiness-=20;
        if(F&&f&&F->militancy>10) F->militancy-=10; // revolta "descarrega"
        SetMsg(g,"REVOLTA -TROPA");
      } else {
        if(t->happiness<100) t->happiness++;
        SetMsg(g,"PROTESTO CONTIDO");
      }
      break; }
    case EVT_ARMSDEAL:
      if(F&&f){ F->stock[GOOD_ARMS]+=10; if(F->stock[GOOD_ARMS]>9999) F->stock[GOOD_ARMS]=9999; Mkt_IntSup(g,f,GOOD_ARMS,5); }
      SetMsg(g,"ARSENAL +10 ARMA"); break;
    case EVT_STORM:
      if(t->building!=BLD_NONE&&Rng_Chance(g,50)){
        t->building=BLD_NONE;
        SetMsg(g,"TEMPESTADE DEMOLIU");
      } else {
        if(t->happiness>5) t->happiness-=5;
        SetMsg(g,"TEMPESTADE");
      }
      break;
    case EVT_FESTIVAL:
      if(t->happiness<90) t->happiness+=10;
      if(F&&f){
        F->prestige+=5;
        F->treasury-=10; if(F->treasury<0) F->treasury=0;
        if(F->militancy>5) F->militancy-=5;
      }
      SetMsg(g,"FESTA +FELIZ +PREST"); break;
    case EVT_COALRUSH:
      if(F&&f){ F->stock[GOOD_COAL]+=20; if(F->stock[GOOD_COAL]>9999) F->stock[GOOD_COAL]=9999; Mkt_IntSup(g,f,GOOD_COAL,8); }
      if(t->site==SITE_NONE) t->site=SITE_COAL;
      SetMsg(g,"CARVAO +20"); break;
    default: SetMsg(g,"EVENTO"); break;
  }
  g->dirty=true;
}
