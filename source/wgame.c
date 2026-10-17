#include "wgame.h"
#include "settings.h"
#include "str.h"
#include "ui.h"
#include "city.h"
#include "menu.h"
#include "px.h"
#include "wecon.h"
#include "wdiplo.h"
#include "history.h"
// wgame.c — partida no mundo celular (SCR_WORLD). Ver wgame.h.
// O World e as grades W_* sao estaticos (.bss); nada aqui conta nos 14KB do
// GameState. Toda regra de jogo esta em world.c/history.c; este arquivo so'
// traduz botoes em ordens (World_Attack, World_Recruit...) e move a camera.
static World W;
static WGame S;
WGame *WG_Ptr(void){return &S;}
World *WG_World(void){return &W;}

int WG_ViewW(void){return S.zoom==0?512:(256>>(S.zoom-1));}
int WG_ViewH(void){return S.zoom==0?WG_MAP_H*2:(WG_MAP_H>>(S.zoom-1));}
static int WrapX(int x){if(W.wrap){x%=W.w;if(x<0)x+=W.w;}return x;}
// distancia com sinal de a ate b no eixo x (curta, se o mundo da' a volta)
static int RelX(int a,int b){
 int d=b-a;
 if(W.wrap){d%=W.w;if(d<-W.w/2)d+=W.w;else if(d>=W.w/2)d-=W.w;}
 return d;
}
bool WG_CellAt(int sx,int sy,int *cx,int *cy){
 int x,y;
 if(S.zoom==0){x=S.camX+sx*2;y=S.camY+sy*2;}
 else{int s=S.zoom-1;x=S.camX+(sx>>s);y=S.camY+(sy>>s);}
 if(y<0||y>=W.h)return false;
 if(W.wrap)x=WrapX(x);else if(x<0||x>=W.w)return false;
 *cx=x;*cy=y;return true;
}
// camera segue o cursor com margem de 1/5 da vista; Z0 mostra o mundo todo
static void Clamp(s16 *cam,int cur,int view,int size,bool wrap){
 if(wrap){
  if(view>=size){*cam=0;return;}           // mundo inteiro: Atlantico no centro
  // o cursor normalmente esta dentro da vista (rel 0..view-1); fora dela,
  // a menor distancia com sinal diz para que lado andar
  int m=view/5,rel=WrapX(cur-*cam);
  if(rel>=view)rel=RelX(*cam,cur);
  if(rel<m)*cam=(s16)WrapX(cur-m);
  else if(rel>view-1-m)*cam=(s16)WrapX(cur-(view-1-m));
  return;
 }
 if(size<=view){*cam=(s16)((size-view)/2);return;}
 int m=view/5;
 if(cur-*cam<m)*cam=(s16)(cur-m);
 if(cur-*cam>view-1-m)*cam=(s16)(cur-(view-1-m));
 if(*cam<0)*cam=0;
 if(*cam>size-view)*cam=(s16)(size-view);
}
static void Follow(void){
 Clamp(&S.camX,S.curX,WG_ViewW(),W.w,W.wrap);
 Clamp(&S.camY,S.curY,WG_ViewH(),W.h,false);
}
static void Center(void){
 S.camX=(s16)WrapX(S.curX-WG_ViewW()/2);
 S.camY=(s16)(S.curY-WG_ViewH()/2);
 Follow();
}
static void Toast(u16 id,u8 arg){S.toastId=id;S.toastArg=arg;S.toastT=180;S.toastNum=-1;S.toastCity=-1;}
// pais sob o cursor vira a ficha do painel (agua mantem a ultima ficha)
static bool Hover(void){
 u32 i=World_Idx(&W,S.curX,S.curY);
 // cidade sob o cursor (raio cresce no zoom afastado)
 int hc=City_Near(&W,S.curX,S.curY,S.zoom>=3?1:(S.zoom==2?2:3));
 bool ch=hc!=S.hcity;
 if(ch){S.hcity=(s16)hc;S.foundAsk=false;}
 if(W_TERR[i]==WT_WATER)return ch;
 u8 o=W_OWNER[i];bool land=(o==0);
 if(o==S.sel&&land==S.selLand)return ch;
 S.sel=o;S.selLand=land;S.warAsk=0;S.foundAsk=false;
 return true;
}

void WG_New(u8 mode,u8 hist,u8 size,u8 nNat,u8 diff,u32 seed,int nat){
 if(mode==WM_MUNDI)World_GenMundi(&W,seed,hist,diff,nat);
 else if(mode==WM_BLITZ)World_GenBlitz(&W,seed,diff,nat);
 else World_GenRandom(&W,seed,size,nNat,diff,nat);
 u8 *p=(u8*)&S;for(unsigned i=0;i<sizeof S;i++)p[i]=0;
 S.hcity=-1;S.cityIdx=-1;PxM_Invalidate();PxC_SceneReset();
 S.me=World_CountryOfNat(&W,nat);
 for(u8 i=1;!S.me&&i<W.nC;i++)if(W.c[i].player)S.me=i;
 if(!S.me){S.me=1;World_SetPlayer(&W,1);} // nacao nao coube no mapa: assume o 1o
 S.pct=50;S.speed=2;S.paused=true;S.mapMode=WMAP_POL;
 S.zoom=(mode==WM_MUNDI)?3:2;
 S.curX=(s16)W.c[S.me].capX;S.curY=(s16)W.c[S.me].capY;
 Center();Hover();
 S.sel=S.me;S.selLand=false;
 S.msgSeen=W.msgDay;S.stale=true;
 Toast(STR_DICA_JOGO,0);S.toastT=400;
}

// ---------------- acoes (botoes do painel e atalhos) ----------------
static void DoAttack(void){
 if(S.dead)return;
 WCountry *me=&W.c[S.me];
 u8 t=S.sel;
 if(!S.selLand&&(!t||t==S.me)){Toast(STR_T_ALVO,0);return;}
 if(t&&(W.c[t].overlord==S.me||me->overlord==t)){Toast(STR_T_SUDITO,0);return;}
 bool landing=!World_Borders(&W,S.me,t);
 bool declared=false;
 if(t&&!World_AtWar(&W,S.me,t)){
  // guerra e' diplomacia (DESIGN 4.5): pede o 2o toque e custa prestigio
  if(S.warAsk!=t){S.warAsk=t;Toast(STR_T_CONFIRMA_GUERRA,t);return;}
  World_DeclareWar(&W,S.me,t);me->prestige-=5;S.warAsk=0;declared=true;
 }
 if(me->atkOn&&me->atkTo!=t)World_Recall(&W,S.me);
 // sem fronteira: desembarque numa praia do alvo (frontwars)
 if(landing){
  s32 cost=0;
  if(!World_Landing(&W,S.me,t,&cost)){Toast(declared?STR_T_GUERRA:STR_T_SEM_FRONTEIRA,t);return;}
  S.stale=true;
  if(!declared){if(World_Attack(&W,S.me,t,S.pct))Toast(STR_T_DESEMBARQUE,t);else Toast(STR_T_SEM_TROPAS,t);return;}
 }
 if(World_Attack(&W,S.me,t,S.pct))Toast(declared?STR_T_GUERRA:STR_T_ATAQUE,t);
 else Toast(declared?STR_T_GUERRA:STR_T_SEM_TROPAS,t);
}
static void DoPeace(void){
 if(S.dead||!S.sel||!World_AtWar(&W,S.me,S.sel)){Toast(STR_T_ALVO,0);return;}
 Toast(World_ProposePeace(&W,S.me,S.sel)?STR_T_PAZ_OK:STR_T_PAZ_NAO,S.sel);
}
// ---------------- diplomacia (botao bandeira): menu sobre o pais selecionado ----------------
int WG_DipItems(u8 *o){
 int n=0;u8 t=S.dipTo,me=S.me;
 if(!t||t>=W.nC||!W.c[t].alive||t==me)return 0;
 bool war=World_AtWar(&W,me,t),al=WDip_Allied(&W,me,t);
 if(war)o[n++]=DM_PAZ;
 else{
  if(!al&&WDip_Allies(&W,me)<2&&W.c[t].overlord!=me&&W.c[me].overlord!=t)o[n++]=DM_ALIANCA;
  if(al)o[n++]=DM_ROMPER;
  u8 L=W.c[t].bloc;
  if(L&&L<W.nC&&W.c[L].bloc==L&&W.c[me].bloc!=L&&L!=me)o[n++]=DM_ENTRAR;
  if(!W.c[t].bloc)o[n++]=DM_CONVIDAR;
  if(!al&&W.c[t].overlord!=me&&W.c[me].overlord!=t)o[n++]=DM_GUERRA;
 }
 o[n++]=DM_CANCELAR;
 return n;
}
static void DipOpen(void){
 if(S.dead)return;
 u8 t=S.selLand?0:S.sel;
 if(!t||t==S.me||!W.c[t].alive){Toast(STR_T_ALVO,0);return;}
 S.dipTo=t;S.dipSel=0;S.dipN=(u8)WG_DipItems(S.dipIt);S.dipOpen=true;
}
static void DipDo(GameState *g,u8 it){
 u8 t=S.dipTo,me=S.me;
 S.dipOpen=false;g->dirty=true;g->dirtyBot=true;
 if(!t||!W.c[t].alive)return;
 switch(it){
  case DM_PAZ:Toast(World_ProposePeace(&W,me,t)?STR_T_PAZ_OK:STR_T_PAZ_NAO,t);break;
  case DM_ALIANCA:Toast(WDip_AskAlliance(&W,me,t)?STR_T_ALIANCA_OK:STR_T_ALIANCA_NAO,t);break;
  case DM_ROMPER:WDip_Unally(&W,me,t);W.c[t].opin=(s8)(W.c[t].opin>-80?W.c[t].opin-20:-100);Toast(STR_T_ROMPEU,t);break;
  case DM_ENTRAR:{u8 L=W.c[t].bloc;
   if(W.c[me].bloc&&W.c[me].bloc!=L)WDip_LeaveBloc(&W,me);
   Toast(WDip_AskBloc(&W,me,L)?STR_T_BLOCO_OK:STR_T_BLOCO_NAO,L);}break;
  case DM_CONVIDAR:Toast(WDip_InviteBloc(&W,me,t)?STR_T_BLOCO_OK:STR_T_BLOCO_NAO,t);break;
  case DM_GUERRA:World_DeclareWarCall(&W,me,t);W.c[me].prestige-=5;Toast(STR_T_GUERRA,t);S.stale=true;break;
  default:break;
 }
}
// proposta pendente (IA -> jogador): o jogo pausa ate a resposta
static bool PropInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 if(!W.prop.kind||S.dead)return false;
 int ans=-1;
 if(d&KEY_A)ans=1;
 else if(d&KEY_B)ans=0;
 else if(tDown&&ty>=PM_BTN_Y&&ty<PM_BTN_Y+PM_BTN_H)ans=tx<128?1:0;
 if(ans>=0){
  WDip_Answer(&W,ans==1);
  if(S.propRun&&!W.prop.kind)S.paused=false;
  S.stale=true;g->dirty=true;g->dirtyBot=true;
 }
 return true;
}
// Botao "Cidade": abre a sua cidade sob o cursor; senao funda uma ali
// (2o toque confirma, mostrando o preco: DESIGN 4.7, preco claro)
static void OpenCity(GameState *g,int ci){
 S.cityIdx=(s16)ci;S.citySlot=0;S.cityPick=false;
 Menu_Go(g,SCR_CITY);
}
static void DoCity(GameState *g){
 if(S.dead)return;
 if(S.hcity>=0&&W.city[S.hcity].owner==S.me){OpenCity(g,S.hcity);return;}
 if(!City_CanFound(&W,S.me,S.curX,S.curY)){Toast(STR_T_NAO_FUNDA,0);return;}
 s32 cost=City_FoundCost(&W,S.me);
 if(!S.foundAsk){S.foundAsk=true;Toast(STR_T_FUNDAR_CONF,0);S.toastNum=cost;S.toastT=300;return;}
 S.foundAsk=false;
 int i=City_Found(&W,S.me,S.curX,S.curY);
 if(i<0){Toast(STR_T_SEM_OURO,0);return;}
 Toast(STR_T_FUNDOU,0);S.toastCity=(s16)i;S.hcity=(s16)i;S.stale=true;
 g->dirty=true;
}
static void DoRecruit(void){
 if(S.dead)return;
 Toast(World_Recruit(&W,S.me)?STR_T_RECRUTA:STR_T_SEM_OURO,0);
}
static void SetZoom(int z){
 if(z<0||z>=WZ_COUNT||z==S.zoom)return;
 // mantem o cursor no mesmo ponto da tela
 int rx=WrapX(S.curX-S.camX);if(rx<0||rx>=WG_ViewW())rx=WG_ViewW()/2;
 int fx=rx*1024/WG_ViewW(), fy=(S.curY-S.camY)*1024/WG_ViewH();
 S.zoom=(u8)z;
 S.camX=(s16)WrapX(S.curX-fx*WG_ViewW()/1024);
 S.camY=(s16)(S.curY-fy*WG_ViewH()/1024);
 Follow();
}
static void Act(GameState *g,int id){
 switch(id){
  case UI_W_ATK:  DoAttack();break;
  case UI_W_PEACE:DipOpen();break;
  case UI_W_LEDGER:S.ledSel=0;Menu_Go(g,SCR_LEDGER);return;
  case UI_W_REC:  DoRecruit();break;
  case UI_W_MODE:{static const u16 MS[WMAP_COUNT]={STR_MAPA_POLITICO,STR_MAPA_TERRENO,STR_MAPA_DIPLO,STR_MAPA_COMERCIO,STR_MAPA_IDEO};
   S.mapMode=(u8)((S.mapMode+1)%WMAP_COUNT);
   Toast(MS[S.mapMode],0);g->dirty=true;}break;
  case UI_W_CITY: DoCity(g);if(g->screen==SCR_CITY)return;break;
  case UI_W_PCT:  S.pct=(u8)(S.pct>=100?25:S.pct+25);break;
  case UI_W_SPEED:
   if(S.paused&&!S.dead&&!W.ended)S.paused=false;
   else S.speed=(u8)(S.speed>=5?1:S.speed+1);
   g->dirty=true;break;
  default:return;
 }
 S.btnDown=(u8)id;S.btnT=6;g->dirtyBot=true;
}
// cursor: passo por zoom (celulas por passo, frames por passo); acelera
// depois de 24 frames segurando
static const u8 STEP_C[WZ_COUNT]={4,2,1,1,1}, STEP_F[WZ_COUNT]={1,1,1,2,4};
static bool MoveCursor(u32 held){
 int dx=((held&KEY_RIGHT)?1:0)-((held&KEY_LEFT)?1:0);
 int dy=((held&KEY_DOWN)?1:0)-((held&KEY_UP)?1:0);
 if(!dx&&!dy){S.holdF=0;return false;}
 bool fast=S.holdF>24;
 int f=STEP_F[S.zoom];if(fast&&f>1)f>>=1;
 int c=STEP_C[S.zoom]*(fast?2:1);
 bool go=(S.holdF%f)==0;
 S.holdF++;
 if(!go)return false;
 int nx=S.curX+dx*c,ny=S.curY+dy*c;
 if(W.wrap)nx=WrapX(nx);else{if(nx<0)nx=0;if(nx>=W.w)nx=W.w-1;}
 if(ny<0)ny=0;
 if(ny>=W.h)ny=W.h-1;
 S.curX=(s16)nx;S.curY=(s16)ny;
 Follow();
 return true;
}
// Layout Invertida: o mapa esta na tela de toque. Arrastar = pan; tocar e
// soltar sem arrastar = cursor ali.
static s16 tLastX,tLastY,tAccX,tAccY;static bool tDrag;
static void TouchMap(GameState *g,bool tDown,bool tUp,bool tHeld,int tx,int ty){
 if(tDown){tLastX=(s16)tx;tLastY=(s16)ty;tAccX=tAccY=0;tDrag=false;return;}
 if(tHeld){
  int dx=tx-tLastX,dy=ty-tLastY;tLastX=(s16)tx;tLastY=(s16)ty;
  tAccX=(s16)(tAccX+dx);tAccY=(s16)(tAccY+dy);
  if(!tDrag&&(tAccX*tAccX+tAccY*tAccY)>64)tDrag=true;
  if(!tDrag)return;
  int ppc=S.zoom==0?1:(1<<(S.zoom-1));   // Z0: 1 px = 2 celulas
  int mx=0,my=0;
  while(tAccX>=ppc){tAccX=(s16)(tAccX-ppc);mx--;}
  while(tAccX<=-ppc){tAccX=(s16)(tAccX+ppc);mx++;}
  while(tAccY>=ppc){tAccY=(s16)(tAccY-ppc);my--;}
  while(tAccY<=-ppc){tAccY=(s16)(tAccY+ppc);my++;}
  if(S.zoom==0){mx*=2;my*=2;}
  if(mx||my){
   S.curX=(s16)WrapX(S.curX+mx);S.curY=(s16)(S.curY+my);
   if(S.curY<0)S.curY=0;
   if(S.curY>=W.h)S.curY=(s16)(W.h-1);
   if(!W.wrap){if(S.curX<0)S.curX=0;if(S.curX>=W.w)S.curX=(s16)(W.w-1);}
   S.camX=(s16)WrapX(S.camX+mx);S.camY=(s16)(S.camY+my);Follow();
   if(Hover())g->dirtyBot=true;
   g->dirty=true;
  }
  return;
 }
 if(tUp&&!tDrag&&ty>=WG_MAP_Y){
  int cx,cy;
  if(WG_CellAt(tx,ty-WG_MAP_Y,&cx,&cy)){S.curX=(s16)cx;S.curY=(s16)cy;Hover();g->dirty=true;}
 }
}

void WG_Input(GameState *g,u32 d,u32 h,bool tDown,bool tUp,bool tHeld,int tx,int ty){
 // modal de proposta tem prioridade (no layout invertido o toque e' do mapa)
 if(PropInput(g,d,Set_Ptr()->layout==LAYOUT_SWAP?false:tDown,tx,ty))return;
 if(S.dipOpen){
  if(d&KEY_UP){S.dipSel=(u8)((S.dipSel+S.dipN-1)%S.dipN);g->dirtyBot=true;}
  if(d&KEY_DOWN){S.dipSel=(u8)((S.dipSel+1)%S.dipN);g->dirtyBot=true;}
  if(d&KEY_B){S.dipOpen=false;g->dirtyBot=true;}
  if(d&KEY_A)DipDo(g,S.dipIt[S.dipSel]);
  else if(tDown&&Set_Ptr()->layout!=LAYOUT_SWAP&&ty>=28&&ty<28+S.dipN*22){
   int i=(ty-28)/22;if(i>=0&&i<S.dipN)DipDo(g,S.dipIt[i]);
  }
  return;
 }
 if(d&KEY_SELECT){
  if(!S.dead&&!W.ended){S.paused=!S.paused;g->dirty=true;}
 }
 if(d&KEY_L){SetZoom(S.zoom-1);g->dirty=true;}
 if(d&KEY_R){SetZoom(S.zoom+1);g->dirty=true;}
 if(d&KEY_Y)Act(g,UI_W_MODE);
 if(d&KEY_X){Act(g,UI_W_LEDGER);return;}
 if(d&KEY_B){
  if(W.c[S.me].atkOn){World_Recall(&W,S.me);Toast(STR_T_RECOLHIDA,0);g->dirtyBot=true;}
  S.warAsk=0;
 }
 if(d&KEY_A){
  if(W.ended){World_Continue(&W);S.paused=false;g->dirty=true;}
  else if(S.dead){
   if(S.sel&&W.c[S.sel].alive){
    World_SetPlayer(&W,S.sel);S.me=S.sel;S.dead=false;
    Toast(STR_ASSUMIU,S.me);g->dirty=true;
   }
  }else if(S.hcity>=0&&W.city[S.hcity].owner==S.me)OpenCity(g,S.hcity);
  else if(!S.selLand&&S.sel==S.me)Act(g,UI_W_CITY);  // na sua terra: fundar
  else Act(g,UI_W_ATK);
 }
 if(MoveCursor(h)){Hover();g->dirtyBot=true;g->dirty=true;}
 if(Set_Ptr()->layout==LAYOUT_SWAP)TouchMap(g,tDown,tUp,tHeld,tx,ty);
 else if(tDown){int id=Ui_Hit(UI_WORLD,tx,ty);if(id>0)Act(g,id);}
}

void WG_Frame(GameState *g){
 S.tick++;
 // cena da cidade: so' os sprites vivos mudam; ~20 quadros/s (fundo em cache)
 if(g->screen==SCR_CITY&&(S.tick%3)==0&&!g->dirty){PxC_Scene(Px_BackTop(),S.cityIdx);Px_FlipTop();}
 if(S.toastT&&!--S.toastT)g->dirtyBot=true;
 if(S.btnT&&!--S.btnT)g->dirtyBot=true;
 if(S.bannerT&&!--S.bannerT)g->dirty=true;
 // proposta nova da IA: pausa e mostra o despacho (resposta em PropInput)
 if(W.prop.kind){u32 k=W.prop.day*8u+W.prop.kind;
  if(k!=S.propSeen){S.propSeen=k;S.propRun=!S.paused;S.paused=true;g->dirty=true;g->dirtyBot=true;}}
 if(S.paused||S.dead||W.ended)return;
 static const u8 FPD[6]={0,24,12,6,3,1}; // frames por dia, velocidade 1..5
 if(++S.frameAcc<FPD[S.speed])return;
 S.frameAcc=0;
 World_Day(&W);
 S.stale=true;
 Hover();
 if(!W.c[S.me].alive){S.dead=true;S.paused=true;Toast(STR_CAIU,0);S.toastT=600;}
 if(W.msgDay!=S.msgSeen&&W.msg[0]){S.msgSeen=W.msgDay;S.bannerT=200;}
 if(W.ended)S.paused=true;
 // velocidade 5: o mapa redesenha 1 dia sim, 1 nao (o painel sempre)
 if(S.speed<5||(W.day&1))g->dirty=true;else g->dirtyBot=true;
}

// ---------------- Gabinete (SCR_LEDGER) ----------------
static void LedgerAct(GameState *g){
 WCountry *me=&W.c[S.me];WEco *e=&W.eco[S.me];
 switch(S.ledTab){
  case LT_MERCADO:S.mapMode=WMAP_COM;Menu_Go(g,SCR_WORLD);return;
  case LT_COMERCIO:Toast(WEco_BuildShip(&W,S.me)?STR_T_NAVIO:STR_T_SEM_NAVIO,0);break;
  case LT_DIPLO:
   if(me->bloc)WDip_LeaveBloc(&W,S.me);else WDip_FoundBloc(&W,S.me);
   break;
  case LT_POLITICA:e->tax=(u8)((e->tax+1)%3);Toast(STR_T_IMPOSTO,0);break;
  default:break;
 }
 g->dirty=true;g->dirtyBot=true;
}
void WG_LedgerInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 bool moved=false;
 if(d&KEY_L){S.ledTab=(u8)((S.ledTab+LT_COUNT-1)%LT_COUNT);moved=true;}
 if(d&KEY_R){S.ledTab=(u8)((S.ledTab+1)%LT_COUNT);moved=true;}
 if(d&(KEY_LEFT)){S.ledTab=(u8)((S.ledTab+LT_COUNT-1)%LT_COUNT);moved=true;}
 if(d&(KEY_RIGHT)){S.ledTab=(u8)((S.ledTab+1)%LT_COUNT);moved=true;}
 if(tDown){
  if(ty>=LG_TAB_Y&&ty<LG_TAB_Y+LG_TAB_H){S.ledTab=(u8)(tx*LT_COUNT/256);moved=true;}
  else if(ty>=LG_ACT_Y&&ty<LG_ACT_Y+LG_ACT_H&&tx>=LG_ACT_X&&tx<LG_ACT_X+LG_ACT_W)d|=KEY_A;
  else if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)d|=KEY_B;
 }
 if(d&KEY_A){LedgerAct(g);if(g->screen!=SCR_LEDGER)return;}
 if(d&(KEY_B|KEY_START|KEY_X)){Menu_Go(g,SCR_WORLD);return;}
 if(d&KEY_SELECT){S.paused=!S.paused;}
 if(moved){g->dirty=true;g->dirtyBot=true;}
}

// ---------------- tela da cidade (SCR_CITY) ----------------
static u8 FirstPick(int ci){
 for(u8 k=1;k<BK_COUNT;k++)if(City_CanBuild(&W,ci,k))return k;
 return BK_FAZENDA;
}
void WG_CityInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 int ci=S.cityIdx;
 if(ci<0||ci>=W.nCity||W.city[ci].owner!=S.me){Menu_Go(g,SCR_WORLD);return;}
 WCity *c=&W.city[ci];
 bool moved=false;
 if(tDown&&ty>=66&&ty<146&&tx>=4&&tx<252){
  int s=((ty-66)/40)*4+(tx-4)/62;
  if(s==S.citySlot&&!S.cityPick)d|=KEY_A;else{S.citySlot=(u8)s;S.cityPick=false;moved=true;}
 }else if(tDown&&S.cityPick&&ty>=148){
  if(tx<40)d|=KEY_LEFT;else if(tx>216)d|=KEY_RIGHT;else d|=KEY_A;
 }
 if(S.cityPick){
  if(d&(KEY_LEFT|KEY_RIGHT)){
   int dir=(d&KEY_RIGHT)?1:-1;
   S.pickK=(u8)(1+((S.pickK-1+dir+(BK_COUNT-1))%(BK_COUNT-1)));moved=true;
  }
  if(d&KEY_A){
   u8 k=S.pickK;
   if(City_Build(&W,ci,S.citySlot,k)){Toast(STR_T_OBRA_INI,0);S.cityPick=false;}
   else if(City_CanBuild(&W,ci,k))Toast(STR_T_SEM_OURO,0);
   moved=true;
  }
  if(d&KEY_B){S.cityPick=false;moved=true;}
 }else{
  if(d&KEY_LEFT){S.citySlot=(u8)((S.citySlot+3)%4+(S.citySlot/4)*4);moved=true;}
  if(d&KEY_RIGHT){S.citySlot=(u8)((S.citySlot+1)%4+(S.citySlot/4)*4);moved=true;}
  if(d&(KEY_UP|KEY_DOWN)){S.citySlot=(u8)((S.citySlot+4)%8);moved=true;}
  if(d&KEY_A){
   if(S.citySlot<City_Slots(c->tier)&&!c->bld[S.citySlot]&&!c->bq){S.cityPick=true;S.pickK=FirstPick(ci);moved=true;}
   else if(c->bld[S.citySlot]==BK_ESTALEIRO){Toast(WEco_BuildShip(&W,S.me)?STR_T_NAVIO:STR_T_SEM_NAVIO,0);moved=true;}
  }
  if(d&(KEY_L|KEY_R)){
   // proxima/anterior cidade sua
   int dir=(d&KEY_R)?1:-1;
   for(int k=1;k<=W.nCity;k++){
    int j=((ci+dir*k)%W.nCity+W.nCity)%W.nCity;
    if(W.city[j].owner==S.me){S.cityIdx=(s16)j;S.citySlot=0;S.curX=(s16)W.city[j].x;S.curY=(s16)W.city[j].y;Center();Hover();break;}
   }
   g->dirty=true;
  }
  if(d&KEY_B){Menu_Go(g,SCR_WORLD);return;}
 }
 if(d&KEY_START){Menu_Go(g,SCR_WORLD);return;}
 if(d&KEY_SELECT){S.paused=!S.paused;}
 if(moved)g->dirtyBot=true;
}
