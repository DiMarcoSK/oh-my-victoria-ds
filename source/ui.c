#include "ui.h"
// ui.c — tabelas de layout (geografia do toque). px_ui.c desenha a partir
// daqui; input.c despacha aqui. Nada de geometria hardcode fora deste arquivo.
//
// Tela baixo (256x192). Dock fixo no rafe (y=166..188): 4 abas.
// Abas: painel quase fullscreen com X de fechamento no canto.
// Tabelas estaticas quando cabe na fonte; grades (obras/mercado) geradas 1x.

static const Btn T_MAP[]={
 {4,166,59,22,UI_D_OBRAS, "OBRAS",0},
 {67,166,59,22,UI_D_MERC, "MERCADO",0},
 {130,166,59,22,UI_D_DIPLO,"RELAC.",0},
 {193,166,59,22,UI_D_SYS, "SIST.",0},
 {228,2,24,20,UI_D_PAUSE, "P",0},
 {202,2,24,20,UI_D_SPEED, "V",0},
 {0,0,0,0,0,0,0} // fim
};
static const Btn T_CTX[]={ // acoes contextuais do tile selecionado
 {4,140,48,22,UI_A_MARCH,"MARCH",0},
 {56,140,48,22,UI_A_REC, "REC",0},
 {108,140,48,22,UI_A_FORT,"FORTE",0},
 {0,0,0,0,0,0,0}
};
static const Btn T_DIPLO[]={
 {236,4,16,16,UI_CLOSE,"X",0},
 {4,44,60,22,UI_DP_BASE+0,"PAZ",0},
 {70,44,60,22,UI_DP_BASE+1,"UNIAO",0},
 {4,104,60,22,UI_DP_BASE+2,"PAZ",0},
 {70,104,60,22,UI_DP_BASE+3,"UNIAO",0},
 {0,0,0,0,0,0,0}
};
static const Btn T_SYS[]={
 {236,4,16,16,UI_CLOSE,"X",0},
 {4,24,50,22,UI_GV_MON,"MON",0},
 {58,24,50,22,UI_GV_DEM,"DEM",0},
 {112,24,50,22,UI_GV_TEO,"TEO",0},
 {4,54,50,22,UI_SP_IMPT,"IMP",0},
 {58,54,50,22,UI_SP_SP1,"V1",0},
 {112,54,50,22,UI_SP_SP2,"V2",0},
 {166,54,50,22,UI_SP_SP3,"V3",0},
 {4,88,50,22,UI_SP_PAUSE,"PAUS",0},
 {58,88,50,22,UI_SP_SAVE,"SAVE",0},
 {112,88,50,22,UI_SP_LOAD,"LOAD",0},
 {166,88,50,22,UI_SP_NEW,"NOVO",0},
 {0,0,0,0,0,0,0}
};
static const Btn T_MENU[]={
 {36,64,30,24,UI_MN_PREVD,"<",0},
 {190,64,30,24,UI_MN_NEXTD,">",0},
 {88,92,80,24,UI_MN_SEED,"SEED",0},
 {68,124,120,30,UI_MN_START,"COMECAR",0},
 {0,0,0,0,0,0,0}
};

// Painel do mapa-mundi (SCR_WORLD): 5 botoes icone 34x34 (atacar, diplomacia,
// recrutar, cidade, gabinete) + % + velocidade. px_world.c desenha daqui;
// wgame.c despacha daqui. Ordem fixa: os 5 icones primeiro.
static const Btn T_WORLD[]={
 {4,152,34,34,UI_W_ATK,0,0},
 {40,152,34,34,UI_W_PEACE,0,0},
 {76,152,34,34,UI_W_REC,0,0},
 {112,152,34,34,UI_W_CITY,0,0},
 {148,152,34,34,UI_W_LEDGER,0,0},
 {186,152,32,34,UI_W_PCT,0,0},
 {220,152,32,34,UI_W_SPEED,0,0},
 {0,0,24,24,UI_W_RANK,0,0},        // bolinha do ranking (canto sup-esq)
 {0,0,0,0,0,0,0}
};

// Grades geradas 1x (tabelas de 23 obras e 8 bens ficam grandes de escrever)
static Btn gbuild[BLD_COUNT]; // id==BuildingKind (1..22); slot 0 = fim
static Btn gtrade[GOOD_COUNT+1]; // id = UI_TR_SHOP + good*3
static bool gridDone=false;

static void InitGrids(void){
 if(gridDone)return;
 // OBRAS: 5 colunas x 5 linhas, celula 48x25 (>= 20px de toque), gap 2
 for(u8 id=1;id<BLD_COUNT;id++){
  int idx=id-1,c=(idx)%5,r=(idx)/5;
  Btn *b=&gbuild[idx];
  b->x=(u8)(4+c*50);b->y=(u8)(24+r*27);b->w=48;b->h=25;
  b->id=id;b->lab=0;b->dim=0;
 }
 gbuild[BLD_COUNT-1].id=0; // terminador depois do 22
 // TRADE: 8 linhas (h=15, gap 2): a linha inteira e UM alvo
 // id = UI_TR_SHOP + good*3; coluna do toque decide a acao (Int/Ext/Venda)
 for(u8 gd=0;gd<GOOD_COUNT;gd++){
  Btn *b=&gtrade[gd];
  b->x=64;b->y=(u8)(20+gd*17);b->w=172;b->h=15;
  b->id=UI_TR_SHOP+(u8)(gd*3);b->lab=0;b->dim=0;
 }
 gtrade[GOOD_COUNT].id=0;
 gridDone=true;
}
const Btn *Ui_Tbl(u8 page){
 InitGrids();
 switch(page){
  case UI_MAP:   return T_MAP;
  case UI_BUILD: return gbuild;
  case UI_TRADE: return gtrade;
  case UI_DIPLO: return T_DIPLO;
  case UI_SYS:   return T_SYS;
  case UI_MENU:  return T_MENU;
  case UI_WORLD: return T_WORLD;
  default:       return T_MAP;
 }
}
const Btn *Ui_Ctx(void){return T_CTX;}
int Ui_Hit(u8 page,int px,int py){
 const Btn*t=Ui_Tbl(page);
 if(page==UI_MAP){
  const Btn*ctx=Ui_Ctx(); // territorio contextual: so se vir a faixa
  for(const Btn*c=ctx;c->id;c++)
   if(px>=c->x&&px<c->x+c->w&&py>=c->y&&py<c->y+c->h)return c->id;
 }
 for(;t->id;t++)
  if(px>=t->x&&px<t->x+t->w&&py>=t->y&&py<t->y+t->h)return t->id;
 return -1;
}
void Ui_Open(GameState *g,u8 page){g->uiPage=page;g->dirtyBot=true;}
void Ui_Close(GameState *g){g->uiPage=UI_MAP;g->dirtyBot=true;}
