#include "menu.h"
#include "game.h"
#include "wgame.h"
#include "settings.h"
#include "str.h"
#include "nations.h"
#include "px.h"
// menu.c — maquina de telas da v0.9 (SDD sec 14.2): D-pad/A/B/L/R e caneta.
// Regra de input em menus: CIMA/BAIXO move (com wrap), A confirma, B volta,
// ESQ/DIR muda valor; lista na tela de baixo = tap seleciona e confirma;
// lista na tela de cima = tap em qualquer ponto da tela de baixo vale A.
static Menu M={0,0,MAPK_MUNDI,WH_LIGADA,WS_PEQUENO,1,1,0x0C1A6u,0,-1,0,0,0};
Menu *Menu_Ptr(void){return &M;}
bool Menu_Owns(u8 s){return s!=SCR_GAME;}

void Menu_Go(GameState *g,u8 scr){
 g->screen=scr;M.sel=0;M.confirm=0;g->dirty=true;g->dirtyBot=true;
 // Layout Invertida so' vale na partida do mundo: mapa embaixo, tocavel
 // (ADR-16). Menus ficam sempre como na concept.
 if(scr==SCR_WORLD&&Set_Ptr()->layout==LAYOUT_SWAP)lcdMainOnBottom();
 else lcdMainOnTop();
}

// ---- carrossel de nacoes ----
static bool NatOk(int i){
 const NationDef *d=&NATIONS[i];
 if(d->yFrom)return false;                          // so' quem existe em 1800
 if(M.map!=MAPK_MUNDI&&d->kind==NK_AUTONOMO)return false;
 return !M.cont||d->cont==M.cont-1;
}
int Menu_NatCount(void){int n=0;for(int i=0;i<NATION_COUNT;i++)if(NatOk(i))n++;return n;}
int Menu_NatAt(int k){for(int i=0;i<NATION_COUNT;i++)if(NatOk(i)&&k--==0)return i;return -1;}
int Menu_NatIndex(int nat){int k=0;for(int i=0;i<NATION_COUNT;i++)if(NatOk(i)){if(i==nat)return k;k++;}return -1;}
static void NatFix(void){
 if(M.nat<0)M.nat=(s16)Nat_Find("GBR");
 if(M.nat<0||Menu_NatIndex(M.nat)<0)M.nat=(s16)Menu_NatAt(0);
}

// ---- linhas do Preparar (dependem do mapa escolhido) ----
int Menu_SetupRows(u8 *rows){
 int n=0;rows[n++]=ROW_MAP;
 if(M.map==MAPK_MUNDI)rows[n++]=ROW_HIST;
 if(M.map==MAPK_ALEAT){rows[n++]=ROW_SIZE;rows[n++]=ROW_NNAT;}
 rows[n++]=ROW_DIFF;rows[n++]=ROW_SEED;rows[n++]=ROW_GO;
 return n;
}
static void SetupChange(u8 row,int dir){
 switch(row){
  case ROW_MAP: M.map=(u8)((M.map+MAPK_COUNT+dir)%MAPK_COUNT);break;
  case ROW_HIST:M.hist=(u8)((M.hist+3+dir)%3);break;
  case ROW_SIZE:M.size=(u8)((M.size+3+dir)%3);break;
  case ROW_NNAT:M.nNat=(u8)((M.nNat+3+dir)%3);break;
  case ROW_DIFF:M.diff=(u8)((M.diff+3+dir)%3);break;
  case ROW_SEED:M.seed=dir>0?M.seed+0x9E3779B9u:M.seed-0x9E3779B9u;break;
  default:break;
 }
}

static void StartWorld(GameState *g){
 Menu_DrawLoading(g);                 // gerar o mundo leva ~1-3 s no DS
 u8 mode=M.map==MAPK_MUNDI?WM_MUNDI:(M.map==MAPK_BLITZ?WM_BLITZ:WM_ALEATORIO);
 static const u8 NN[3]={8,16,32};
 WG_New(mode,M.campaign?WH_LIGADA:M.hist,M.size,NN[M.nNat%3],M.diff,M.seed,M.nat);
 Menu_Go(g,SCR_WORLD);
}

static int Wrap(int v,int n){return (v%n+n)%n;}
void Menu_Input(GameState *g,u32 d,u32 h,bool tDown,bool tUp,bool tHeld,int tx,int ty){
 bool up=d&KEY_UP,dn=d&KEY_DOWN,lf=d&KEY_LEFT,rt=d&KEY_RIGHT,a=d&KEY_A,b=d&KEY_B;
 bool moved=up||dn||lf||rt;
 switch(g->screen){
  case SCR_MAIN:{
   // 0 Novo Jogo, 1 Carregar (sem save do mundo ainda: desabilitado), 2 Opcoes, 3 Creditos
   if(up){M.sel=(u8)Wrap(M.sel-1,4);if(M.sel==1)M.sel=0;}
   if(dn){M.sel=(u8)Wrap(M.sel+1,4);if(M.sel==1)M.sel=2;}
   if(tDown&&tx>=104&&tx<248&&ty>=22){int i=(ty-22)/30;if(i>=0&&i<4&&i!=1&&(ty-22)%30<24){M.sel=(u8)i;a=true;}}
   if(a){
    if(M.sel==0)Menu_Go(g,SCR_MODE);
    else if(M.sel==2){g->scrPrev=SCR_MAIN;Menu_Go(g,SCR_OPTIONS);}
    else if(M.sel==3){M.credY=0;Menu_Go(g,SCR_CREDITS);}
    return;
   }
   break;}
  case SCR_MODE:
   if(up)M.sel=(u8)Wrap(M.sel-1,3);
   if(dn)M.sel=(u8)Wrap(M.sel+1,3);
   if(tDown)a=true;
   if(b){Menu_Go(g,SCR_MAIN);return;}
   if(a){
    if(M.sel==0){M.campaign=1;M.map=MAPK_MUNDI;M.hist=WH_LIGADA;NatFix();Menu_Go(g,SCR_NATION);}
    else if(M.sel==1){M.campaign=0;Menu_Go(g,SCR_SETUP);}
    return; // Multijogador: F7 (em breve)
   }
   break;
  case SCR_SETUP:{
   u8 rows[8];int n=Menu_SetupRows(rows);
   if(M.sel>=n)M.sel=(u8)(n-1);
   if(up)M.sel=(u8)Wrap(M.sel-1,n);
   if(dn)M.sel=(u8)Wrap(M.sel+1,n);
   if(lf)SetupChange(rows[M.sel],-1);
   if(rt)SetupChange(rows[M.sel],1);
   if(tDown)a=true;
   if(b){Menu_Go(g,SCR_MODE);M.sel=1;return;}
   if(a){
    if(M.map==MAPK_CLASSICO){Game_New(g,M.diff,M.seed);lcdMainOnTop();return;}
    NatFix();Menu_Go(g,SCR_NATION);return;
   }
   break;}
  case SCR_NATION:{
   NatFix();
   int n=Menu_NatCount(),k=Menu_NatIndex(M.nat);
   if(lf&&n)M.nat=(s16)Menu_NatAt(Wrap(k-1,n));
   if(rt&&n)M.nat=(s16)Menu_NatAt(Wrap(k+1,n));
   if(d&(KEY_L|KEY_R)){
    M.cont=(u8)Wrap(M.cont+((d&KEY_R)?1:-1),CONT_COUNT+1);
    if(Menu_NatIndex(M.nat)<0)M.nat=(s16)Menu_NatAt(0);
    moved=true;
   }
   if(tDown)a=true;
   if(b){Menu_Go(g,M.campaign?SCR_MODE:SCR_SETUP);return;}
   if(a&&M.nat>=0){StartWorld(g);return;}
   break;}
  case SCR_WORLD:
   if(d&KEY_START){Menu_Go(g,SCR_PAUSE);return;}
   WG_Input(g,d,h,tDown,tUp,tHeld,tx,ty);
   return;
  case SCR_CITY:
   WG_CityInput(g,d,tDown,tx,ty);
   return;
  case SCR_LEDGER:
   WG_LedgerInput(g,d,tDown,tx,ty);
   return;
  case SCR_RANK:
   WG_RankInput(g,d,tDown,tx,ty);
   return;
  case SCR_PAUSE:
   // 0 Continuar, 1 Salvar (F8: em breve), 2 Configuracoes, 3 Sair para o Menu
   if(up){M.sel=(u8)Wrap(M.sel-1,4);M.confirm=0;}
   if(dn){M.sel=(u8)Wrap(M.sel+1,4);M.confirm=0;}
   if(tDown)a=true;
   if(b||(d&KEY_START)){Menu_Go(g,SCR_WORLD);return;}
   if(a){
    if(M.sel==0){Menu_Go(g,SCR_WORLD);return;}
    if(M.sel==2){g->scrPrev=SCR_PAUSE;Menu_Go(g,SCR_OPTIONS);M.sel=0;return;}
    if(M.sel==3){
     if(M.confirm){Game_Init(g);Menu_Go(g,SCR_MAIN);return;}
     M.confirm=1;moved=true;
    }
   }
   break;
  case SCR_OPTIONS:{
   Settings *st=Set_Ptr();
   if(up)M.sel=(u8)Wrap(M.sel-1,4);
   if(dn)M.sel=(u8)Wrap(M.sel+1,4);
   if(lf||rt){
    int dir=rt?1:-1;
    if(M.sel==0){int v=st->music+dir*10;st->music=(u8)(v<0?0:v>100?100:v);}
    else if(M.sel==1){int v=st->sfx+dir*10;st->sfx=(u8)(v<0?0:v>100?100:v);}
    else if(M.sel==2)st->lang=(u8)Wrap(st->lang+dir,LANG_COUNT);
    else st->layout=(u8)Wrap(st->layout+dir,LAYOUT_COUNT);
   }
   if(b||tDown){u8 back=g->scrPrev;Menu_Go(g,back);if(back==SCR_PAUSE)M.sel=2;else if(back==SCR_MAIN)M.sel=2;return;}
   break;}
  case SCR_CREDITS:
   if(a||b||tDown){Menu_Go(g,SCR_MAIN);M.sel=3;return;}
   break;
  default:
   if(a||b){Menu_Go(g,SCR_MAIN);return;}
   break;
 }
 if(moved){g->dirty=true;g->dirtyBot=true;}
}
void Menu_Frame(GameState *g){
 M.tick++;
 if(g->screen==SCR_CREDITS&&(M.tick&1)==0){M.credY++;g->dirty=true;}
 if(g->screen==SCR_NATION&&(M.tick%20)==0)g->dirtyBot=true; // marcador pisca
}
