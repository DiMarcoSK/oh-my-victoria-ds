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
#include "wrank.h"
#include "prof.h"
#include "wfort.h"
#include "wlaw.h"
#include "wwar.h"
#include "wsoc.h"
#include "wplan.h"
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
bool WG_CellAt(int sx,int sy,int *cx,int *cy){
 int x,y;
 if(S.zoom==0){x=S.camX+sx*2;y=S.camY+sy*2;}
 else{int s=S.zoom-1;x=S.camX+((sx+S.subX)>>s);y=S.camY+((sy+S.subY)>>s);}
 if(y<0||y>=W.h)return false;
 if(W.wrap)x=WrapX(x);else if(x<0||x>=W.w)return false;
 *cx=x;*cy=y;return true;
}
// ---------------- camera suave (ADR-31) ----------------
// A camera vive em pixels da vista em 1/16 (pX,pY: canto sup-esq) e anda com
// velocidade e aceleracao; a MIRA fica em (rX,rY) da vista, normalmente o
// centro. Quando a camera nao pode andar (mapa todo visivel no eixo, ou
// borda sem volta do mundo), quem anda e' a mira. A celula alvo (curX/curY)
// e' sempre a que esta' sob a mira. camX/subX = celula e resto em pixels,
// derivados p/ o render (px_mapr usa camX*ppc+subX).
enum { VW16=256*16, VH16=WG_MAP_H*16, RCX=128*16, RCY=(WG_MAP_H/2)*16 };
static int P16(void){return S.zoom==0?8:(16<<(S.zoom-1));}     // 1/16 px por celula
static s32 FloorDiv(s32 a,s32 b){return a>=0?a/b:-((-a+b-1)/b);}
static void Derive(void){
 int p=P16();
 s32 wx=W.w*p,wy=W.h*p;
 if(W.wrap&&wx>VW16){S.pX%=wx;if(S.pX<0)S.pX+=wx;}
 s32 cx=FloorDiv(S.pX,p),cy=FloorDiv(S.pY,p);
 S.camX=(s16)(W.wrap?WrapX((int)cx):cx);S.camY=(s16)cy;
 if(S.zoom<=1){S.subX=S.subY=0;}
 else{S.subX=(u8)((S.pX-cx*p)>>4);S.subY=(u8)((S.pY-cy*p)>>4);}
 (void)wy;
 s32 tx=FloorDiv(S.pX+S.rX,p),ty=FloorDiv(S.pY+S.rY,p);
 if(ty<0)ty=0;
 if(ty>=W.h)ty=W.h-1;
 if(W.wrap)tx=WrapX((int)tx);else{if(tx<0)tx=0;if(tx>=W.w)tx=W.w-1;}
 S.curX=(s16)tx;S.curY=(s16)ty;
}
// limites da camera num eixo: [lo,hi]; lo>hi = o mapa cabe inteiro (centrado)
static void Lim(bool xAxis,s32 *lo,s32 *hi){
 int p=P16();s32 size=(xAxis?W.w:W.h)*p,view=xAxis?VW16:VH16;
 if(xAxis&&W.wrap&&size>view){*lo=-0x7FFFFFF;*hi=0x7FFFFFF;return;}
 if(size<=view){*lo=*hi=(size-view)/2;return;}
 *lo=0;*hi=size-view;
}
// anda d (1/16 px) num eixo: a mira volta ao centro primeiro; o resto e' a
// camera; o que a camera nao anda (borda) a mira anda.
static void Axis(bool xAxis,s32 d){
 s32 *cam=xAxis?&S.pX:&S.pY;s16 *r=xAxis?&S.rX:&S.rY;
 s32 c=xAxis?RCX:RCY,rmax=(xAxis?VW16:VH16)-16*6;
 if((*r>c&&d<0)||(*r<c&&d>0)){
  s32 g=c-*r;s32 t=(d<0?(d<g?g:d):(d>g?g:d));
  *r=(s16)(*r+t);d-=t;
 }
 s32 lo,hi;Lim(xAxis,&lo,&hi);
 s32 n=*cam+d;if(n<lo)n=lo;
 if(n>hi)n=hi;
 s32 left=d-(n-*cam);*cam=n;
 s32 nr=*r+left;if(nr<16*6)nr=16*6;
 if(nr>rmax)nr=rmax;
 *r=(s16)nr;
}
// poe a celula (x,y) sob a mira, com a mira o mais perto do centro possivel
static void Center(void){
 int p=P16();
 S.rX=RCX;S.rY=RCY;
 S.pX=(s32)S.curX*p+p/2-RCX;S.pY=(s32)S.curY*p+p/2-RCY;
 s32 lo,hi;
 Lim(true,&lo,&hi);if(S.pX<lo){S.rX=(s16)(S.rX-(lo-S.pX));S.pX=lo;}else if(S.pX>hi){S.rX=(s16)(S.rX+(S.pX-hi));S.pX=hi;}
 Lim(false,&lo,&hi);if(S.pY<lo){S.rY=(s16)(S.rY-(lo-S.pY));S.pY=lo;}else if(S.pY>hi){S.rY=(s16)(S.rY+(S.pY-hi));S.pY=hi;}
 int cx=S.curX,cy=S.curY;
 Derive();
 S.curX=(s16)cx;S.curY=(s16)cy;     // arredondamento nao troca o alvo
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

void WG_Goto(int x,int y){S.curX=(s16)x;S.curY=(s16)y;S.vX=S.vY=0;Center();Hover();}

void WG_New(u8 mode,u8 hist,u8 size,u8 nNat,u8 diff,u32 seed,int nat){
 if(mode==WM_MUNDI)World_GenMundi(&W,seed,hist,diff,nat);
 else if(mode==WM_BLITZ)World_GenBlitz(&W,seed,diff,nat);
 else World_GenRandom(&W,seed,size,nNat,diff,nat);
 u8 *p=(u8*)&S;for(unsigned i=0;i<sizeof S;i++)p[i]=0;
 S.hcity=-1;S.cityIdx=-1;S.lawCat=0xFF;PxM_Invalidate();PxC_SceneReset();PxS_Reset();
 S.me=World_CountryOfNat(&W,nat);
 for(u8 i=1;!S.me&&i<W.nC;i++)if(W.c[i].player)S.me=i;
 if(!S.me){S.me=1;World_SetPlayer(&W,1);} // nacao nao coube no mapa: assume o 1o
 S.pct=50;S.speed=2;S.paused=true;S.mapMode=WMAP_POL;
 S.zoom=(mode==WM_MUNDI)?3:2;
 S.curX=(s16)W.c[S.me].capX;S.curY=(s16)W.c[S.me].capY;
 Center();Hover();
 PxW_Stats();
 S.sel=S.me;S.selLand=false;
 S.msgSeen=W.msgDay;
 Toast(STR_DICA_JOGO,0);S.toastT=400;
 S.centY=W.date.y/100*100;S.centT=330;     // abertura: o seculo em que a partida comeca (ADR-46)
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
  // guerra e' diplomacia (DESIGN 4.5, ADR-45): primeiro a CRISE com uma
  // exigencia; a guerra so' abre quando o prazo acaba sem acordo
  if(War_Find(&W,S.me,t)>=0){Toast(STR_T_CRISE_ESPERA,t);return;}
  S.actKind=AK_META;S.dipTo=t;S.dipSel=0;S.dipN=(u8)WG_DipItems(S.dipIt);S.dipOpen=true;return;
 }
 if(me->atkOn&&me->atkTo!=t)World_Recall(&W,S.me);
 // sem fronteira: desembarque numa praia do alvo (frontwars)
 if(landing){
  s32 cost=0;
  if(!World_Landing(&W,S.me,t,&cost)){Toast(declared?STR_T_GUERRA:STR_T_SEM_FRONTEIRA,t);return;}
  if(!declared){if(World_Attack(&W,S.me,t,S.pct))Toast(STR_T_DESEMBARQUE,t);else Toast(STR_T_SEM_TROPAS,t);return;}
 }
 if(World_Attack(&W,S.me,t,S.pct))Toast(declared?STR_T_GUERRA:STR_T_ATAQUE,t);
 else Toast(declared?STR_T_GUERRA:STR_T_SEM_TROPAS,t);
 // foco: a mira sobre a terra do alvo puxa o avanco p/ la' (ADR-38)
 if(me->atkOn&&me->atkTo==t){
  bool on=t&&W_OWNER[World_Idx(&W,S.curX,S.curY)]==t;
  World_SetFocus(&W,S.me,on?S.curX:-1,on?S.curY:-1);
 }
}
// ---------------- folha de acoes (A sobre a mira / botao bandeira) ----------------
// O alvo e' o que esta' sob a mira: sua cidade, sua terra, terra livre ou um
// pais. A folha so' lista o que da' p/ fazer ali (DESIGN 3.6: nada de botao
// morto); preco vai no rotulo (px_world.c), entao nao ha' 2o toque.
static u8 TargetKind(void){
 if(S.hcity>=0&&W.city[S.hcity].owner==S.me)return AK_CIDADE;
 if(S.selLand)return AK_LIVRE;
 if(S.sel==S.me)return AK_TERRA;
 return AK_PAIS;
}
int WG_DipItems(u8 *o){
 int n=0;u8 t=S.dipTo,me=S.me;
 if(S.dead)return 0;
 switch(S.actKind){
  case AK_OBRA:
   if(S.hcity>=0&&W.city[S.hcity].owner==me)o[n++]=DM_CIDADE;
   if(City_CanFound(&W,me,S.curX,S.curY))o[n++]=DM_FUNDAR;
   if(Fort_CanBuild(&W,me,S.curX,S.curY,FT_FORTE))o[n++]=DM_FORTE;
   if(Fort_CanBuild(&W,me,S.curX,S.curY,FT_TRINCHEIRA))o[n++]=DM_TRINCHEIRA;
   if(Fort_CanBuild(&W,me,S.curX,S.curY,FT_BATERIA))o[n++]=DM_BATERIA;
   break;
  case AK_CIDADE:o[n++]=DM_CIDADE;o[n++]=DM_RECRUTAR;break;
  case AK_TERRA:
   if(City_CanFound(&W,me,S.curX,S.curY))o[n++]=DM_FUNDAR;
   o[n++]=DM_RECRUTAR;break;
  case AK_LIVRE:o[n++]=DM_COLONIZAR;o[n++]=DM_RECRUTAR;break;
  case AK_META:
   if(!t||t>=W.nC||!W.c[t].alive||t==me||War_Find(&W,me,t)>=0||World_AtWar(&W,me,t))return 0;
   o[n++]=DM_META_TERRA;o[n++]=DM_META_TRIBUTO;o[n++]=DM_META_HUMILHAR;break;
  default:{
   if(!t||t>=W.nC||!W.c[t].alive||t==me)return 0;
   bool war=World_AtWar(&W,me,t),al=WDip_Allied(&W,me,t);
   bool sub=W.c[t].overlord==me||W.c[me].overlord==t;
   if(war){o[n++]=DM_ATACAR;o[n++]=DM_PAZ;
    {int k=War_Find(&W,me,t);if(k>=0&&W.cr[k].st==CRS_GUERRA)o[n++]=DM_CONCEDER;}}
   else{
    if(!al&&WDip_Allies(&W,me)<2&&!sub)o[n++]=DM_ALIANCA;
    if(al)o[n++]=DM_ROMPER;
    u8 L=W.c[t].bloc;
    if(L&&L<W.nC&&W.c[L].bloc==L&&W.c[me].bloc!=L&&L!=me)o[n++]=DM_ENTRAR;
    if(!W.c[t].bloc)o[n++]=DM_CONVIDAR;
    // armas e intriga (ADR-37): vender a quem esta' em guerra; acender guerras
    if(W.c[me].armsTo==t)o[n++]=DM_ARMAS_FIM;
    else if(W.c[t].warN)o[n++]=DM_ARMAS;
    if(!sub&&!W.c[t].player)o[n++]=DM_INSTIGAR;
    if(!al&&!sub&&War_Find(&W,me,t)<0)o[n++]=DM_GUERRA;   // exigir = abrir crise (ADR-45)
   }
  }break;
 }
 o[n++]=DM_CANCELAR;
 return n;
}
// (re)monta a folha p/ o alvo atual; a folha segue a mira enquanto o mapa anda
static void ActBuild(void){
 S.actKind=TargetKind();
 S.dipTo=S.actKind==AK_PAIS?S.sel:0;
 S.dipSel=0;S.dipN=(u8)WG_DipItems(S.dipIt);
}
static void DipOpen(void){
 if(S.dead)return;
 ActBuild();
 if(S.dipN<=1){Toast(STR_T_ALVO,0);return;}
 S.dipOpen=true;
}
// botao Construir: cidade e fortificacoes na celula sob a mira
static void BuildOpen(void){
 if(S.dead)return;
 S.actKind=AK_OBRA;S.dipTo=0;S.dipSel=0;S.dipN=(u8)WG_DipItems(S.dipIt);
 if(S.dipN<=1){Toast(STR_T_FORTE_NAO,0);return;}
 S.dipOpen=true;
}
static void FortDo(u8 kind){
 int k=Fort_Build(&W,S.me,S.curX,S.curY,kind);
 if(k<0){Toast(Fort_CanBuild(&W,S.me,S.curX,S.curY,kind)?STR_T_SEM_OURO:STR_T_FORTE_NAO,0);return;}
 Toast(STR_T_FORTE_OK,0);S.toastNum=FORT[kind].days;
}
static void DoRecruit(void);
static void OpenCity(GameState *g,int ci);
static void DipDo(GameState *g,u8 it){
 u8 t=S.dipTo,me=S.me;
 S.dipOpen=false;g->dirty=true;g->dirtyBot=true;
 switch(it){
  case DM_CIDADE:if(S.hcity>=0)OpenCity(g,S.hcity);return;
  case DM_FUNDAR:{
   int i=City_Found(&W,me,S.curX,S.curY);
   if(i<0){Toast(City_CanFound(&W,me,S.curX,S.curY)?STR_T_SEM_OURO:STR_T_NAO_FUNDA,0);return;}
   Toast(STR_T_FUNDOU,0);S.toastCity=(s16)i;S.hcity=(s16)i;
  }return;
  case DM_RECRUTAR:DoRecruit();ActBuild();S.dipOpen=true;return;   // recrutar de novo sem reabrir
  case DM_ATACAR:case DM_COLONIZAR:DoAttack();return;
  case DM_FORTE:FortDo(FT_FORTE);return;
  case DM_TRINCHEIRA:FortDo(FT_TRINCHEIRA);return;
  case DM_BATERIA:FortDo(FT_BATERIA);return;
  default:break;
 }
 if(!t||!W.c[t].alive)return;
 switch(it){
  case DM_PAZ:{
   int k=War_Find(&W,me,t);
   if(k>=0&&W.cr[k].st==CRS_GUERRA){int r=War_PeaceTry(&W,k,me);
    Toast(r==2?STR_T_PAZ_TERMOS:(r==1?STR_T_PAZ_OK:(W.cr[k].talkCd>15?STR_T_PAZ_NAO:STR_T_CONVERSA_CD)),t);}
   else Toast(World_ProposePeace(&W,me,t)?STR_T_PAZ_OK:STR_T_PAZ_NAO,t);
  }break;
  case DM_CONCEDER:{
   int k=War_Find(&W,me,t);
   if(k<0)break;
   s32 c=War_ConcedeCost(&W,k,me);
   if(War_Concede(&W,k,me))Toast(STR_T_CONCEDEU,t);else Toast(c>W.c[me].gold?STR_T_SEM_OURO:STR_T_PAZ_NAO,0);
  }break;
  case DM_META_TERRA:case DM_META_TRIBUTO:case DM_META_HUMILHAR:{
   int k=War_Start(&W,me,t,(u8)(GL_TERRA+(it-DM_META_TERRA)));
   Toast(k>=0?STR_T_CRISE_ABERTA:STR_T_ALVO,t);
  }break;
  case DM_ALIANCA:Toast(WDip_AskAlliance(&W,me,t)?STR_T_ALIANCA_OK:STR_T_ALIANCA_NAO,t);break;
  case DM_ROMPER:WDip_Unally(&W,me,t);W.c[t].opin=(s8)(W.c[t].opin>-80?W.c[t].opin-20:-100);Toast(STR_T_ROMPEU,t);break;
  case DM_ENTRAR:{u8 L=W.c[t].bloc;
   if(W.c[me].bloc&&W.c[me].bloc!=L)WDip_LeaveBloc(&W,me);
   Toast(WDip_AskBloc(&W,me,L)?STR_T_BLOCO_OK:STR_T_BLOCO_NAO,L);}break;
  case DM_CONVIDAR:Toast(WDip_InviteBloc(&W,me,t)?STR_T_BLOCO_OK:STR_T_BLOCO_NAO,t);break;
  case DM_ARMAS:{
   const WEco *e=&W.eco[me];
   if(e->sup[GD_ARMAS]<=e->dem[GD_ARMAS]){Toast(STR_T_ARMAS_NAO,0);break;}
   W.c[me].armsTo=t;Toast(STR_T_ARMAS_OK,t);
  }break;
  case DM_ARMAS_FIM:W.c[me].armsTo=0;Toast(STR_T_ARMAS_FIM,0);break;
  case DM_INSTIGAR:{
   u8 v=0;int r=WDip_Instigate(&W,me,t,&v);
   if(r==1)Toast(STR_T_INSTIGA_OK,t);
   else if(r==0)Toast(STR_T_INSTIGA_NAO,t);
   else if(r==-2)Toast(STR_T_INSTIGA_CD,0);
   else if(r==-3)Toast(STR_T_SEM_OURO,0);
   else Toast(STR_T_ALVO,0);
  }break;
  case DM_GUERRA:
   // exigir (ADR-45): escolhe o objetivo da crise numa 2a folha
   S.actKind=AK_META;S.dipTo=t;S.dipSel=0;S.dipN=(u8)WG_DipItems(S.dipIt);S.dipOpen=S.dipN>1;
   break;
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
  g->dirty=true;g->dirtyBot=true;
 }
 return true;
}
// abre o mapa de lotes da cidade (SCR_CITY)
static void OpenCity(GameState *g,int ci){
 S.cityIdx=(s16)ci;S.citySlot=0;S.cityPick=false;S.cityLot=(u8)City_Plaza(&W,ci);S.cityHold=0;S.cityCamX=0xFFFF;
 Menu_Go(g,SCR_CITY);
}
static void DoRecruit(void){
 if(S.dead)return;
 Toast(World_Recruit(&W,S.me)?STR_T_RECRUTA:STR_T_SEM_OURO,0);
}
static void SetZoom(int z){
 if(z<0||z>=WZ_COUNT||z==S.zoom)return;
 // o alvo sob a mira continua sob a mira (o mapa gira em volta dele)
 S.zoom=(u8)z;S.vX=S.vY=0;
 Center();
}
static void Act(GameState *g,int id){
 switch(id){
  case UI_W_ATK:  DoAttack();break;
  case UI_W_PEACE:DipOpen();break;
  case UI_W_LEDGER:S.ledSel=0;Menu_Go(g,SCR_LEDGER);return;
  case UI_W_RANK:S.rankTop=0;Menu_Go(g,SCR_RANK);return;
  case UI_W_REC:  DoRecruit();break;
  case UI_W_MODE:{static const u16 MS[WMAP_COUNT]={STR_MAPA_POLITICO,STR_MAPA_TERRENO,STR_MAPA_DIPLO,STR_MAPA_COMERCIO,STR_MAPA_IDEO};
   S.mapMode=(u8)((S.mapMode+1)%WMAP_COUNT);
   Toast(MS[S.mapMode],0);g->dirty=true;}break;
  case UI_W_CITY: BuildOpen();break;   // Construir: cidade, forte, trincheira, bateria (ADR-38)
  case UI_W_PCT:  S.pct=(u8)(S.pct>=100?25:S.pct+25);break;
  case UI_W_SPEED:
   if(S.paused&&!S.dead&&!W.ended)S.paused=false;
   else S.speed=(u8)(S.speed>=5?1:S.speed+1);
   g->dirty=true;break;
  default:return;
 }
 S.btnDown=(u8)id;S.btnT=6;g->dirtyBot=true;
}
// SETAS movem o MAPA com inercia: velocidade sobe de 2 a 7 px/quadro
// segurando (1/16 px de precisao, entao o passo e' de 1 px mesmo em Z4) e
// desliza ate' parar ao soltar. Devolve true se algo andou.
static bool MoveCursor(u32 held){
 int dx=((held&KEY_RIGHT)?1:0)-((held&KEY_LEFT)?1:0);
 int dy=((held&KEY_DOWN)?1:0)-((held&KEY_UP)?1:0);
 enum { V0=32, VMAX=112, ACC=3 };
 s16 *v[2]={&S.vX,&S.vY};int dir[2]={dx,dy};
 for(int k=0;k<2;k++){
  s16 *q=v[k];
  if(dir[k]){
   int sp=*q*dir[k];                      // velocidade no sentido pedido
   sp=sp<V0?V0:sp+ACC;if(sp>VMAX)sp=VMAX;
   *q=(s16)(sp*dir[k]);
  }else if(*q){
   *q=(s16)(*q*3/4);                      // atrito: desliza ate' parar
   if(*q>-6&&*q<6)*q=0;
  }
 }
 if(!S.vX&&!S.vY)return false;
 int cx=S.curX,cy=S.curY,ox=S.camX,oy=S.camY,sx=S.subX,sy=S.subY,rx=S.rX>>4,ry=S.rY>>4;
 Axis(true,S.vX);Axis(false,S.vY);
 Derive();
 return cx!=S.curX||cy!=S.curY||ox!=S.camX||oy!=S.camY||sx!=S.subX||sy!=S.subY||rx!=(S.rX>>4)||ry!=(S.rY>>4);
}
// Passo fixo de 1/60 s: o quadro que levou N VBlanks anda N passos (ADR-36).
// Sem isso, quadro pesado = mapa mais lento E aos trancos; com isso a
// velocidade na tela e' sempre a mesma, so' a amostragem cai.
static bool MoveSteps(u32 held){bool mv=false;for(int i=Px_Vb();i>0;i--)mv|=MoveCursor(held);return mv;}
// Mapa andando e a mira cruzando paises: a ficha do painel (tela de baixo
// inteira + DMA de 96KB) sai no maximo ~6x/s e na hora em que o mapa para.
// Antes: redesenho a cada pais cruzado = tranco no meio do pan (ADR-36).
static void BotLazy(void){if(!S.botLazy)S.botLazy=1;}
// Layout Invertida: o mapa esta na tela de toque. Arrastar = pan (pixel a
// pixel); tocar e soltar sem arrastar = mira ali.
static s16 tLastX,tLastY,tAccX,tAccY;static bool tDrag;
static void TouchMap(GameState *g,bool tDown,bool tUp,bool tHeld,int tx,int ty){
 if(tDown){tLastX=(s16)tx;tLastY=(s16)ty;tAccX=tAccY=0;tDrag=false;return;}
 if(tHeld){
  int dx=tx-tLastX,dy=ty-tLastY;tLastX=(s16)tx;tLastY=(s16)ty;
  tAccX=(s16)(tAccX+dx);tAccY=(s16)(tAccY+dy);
  if(!tDrag&&(tAccX*tAccX+tAccY*tAccY)>64)tDrag=true;
  if(!tDrag||(!dx&&!dy))return;
  int cx=S.curX,cy=S.curY;
  Axis(true,-dx*16);Axis(false,-dy*16);Derive();
  if((cx!=S.curX||cy!=S.curY)&&Hover())g->dirtyBot=true;
  Px_WantTop();
  return;
 }
 if(tUp&&!tDrag&&ty>=WG_MAP_Y){
  int cx,cy;
  if(WG_CellAt(tx,ty-WG_MAP_Y,&cx,&cy)){
   S.rX=(s16)(tx*16);S.rY=(s16)((ty-WG_MAP_Y)*16);Derive();
   if(Hover())g->dirtyBot=true;
   Px_WantTop();
  }
 }
}

// faixa da crise no painel (ADR-45): 3 botoes conforme o estado. Crise:
// chamar aliados / propor acordo / ceder (defensor) ou recuar (atacante).
// Guerra com objetivo: chamar aliados / negociar paz / comprar a paz.
static bool CrisisTap(GameState *g,int tx,int ty){
 if(S.dead||ty<CR_BY||ty>=CR_BY+CR_BH)return false;
 int k=War_Of(&W,S.me);
 if(k<0)return false;
 const WCrisis *c=&W.cr[k];
 if(c->att!=S.me&&c->def!=S.me)return false;   // so' o lider negocia
 int b=(tx-CR_BX0)/CR_BDX;
 if(tx<CR_BX0||b>2||tx>=CR_BX0+b*CR_BDX+CR_BW)return true;
 u8 o=c->att==S.me?c->def:c->att;
 g->dirtyBot=true;
 if(b==0){int n=War_CallAllies(&W,k,S.me);Toast(n?STR_T_ALIADOS_OK:STR_T_ALIADOS_NAO,0);if(n)S.toastNum=n;return true;}
 if(c->st==CRS_CRISE){
  if(b==1){bool cd=c->talkCd>0;Toast(War_Deal(&W,k,S.me)?STR_T_ACORDO_OK:(cd?STR_T_CONVERSA_CD:STR_T_ACORDO_NAO),o);}
  else{bool def=c->def==S.me;War_Yield(&W,k,S.me);Toast(def?STR_T_CEDEU:STR_T_RECUOU,o);}
 }else{
  if(b==1){int r=War_PeaceTry(&W,k,S.me);Toast(r==2?STR_T_PAZ_TERMOS:(r==1?STR_T_PAZ_OK:STR_T_PAZ_NAO),o);}
  else if(War_Concede(&W,k,S.me))Toast(STR_T_CONCEDEU,o);else Toast(STR_T_SEM_OURO,0);
 }
 g->dirty=true;
 return true;
}
void WG_Input(GameState *g,u32 d,u32 h,bool tDown,bool tUp,bool tHeld,int tx,int ty){
 // painel (tela de baixo) e' so' caneta. No layout Invertida o painel fica em
 // cima, sem toque: ai' a folha/proposta aceitam CIMA/BAIXO + A/B.
 bool swap=Set_Ptr()->layout==LAYOUT_SWAP;
 if(PropInput(g,swap?d:0,swap?false:tDown,tx,ty))return;
 if(S.dipOpen){
  if(swap){
   if(d&KEY_UP){S.dipSel=(u8)((S.dipSel+S.dipN-1)%S.dipN);g->dirtyBot=true;}
   if(d&KEY_DOWN){S.dipSel=(u8)((S.dipSel+1)%S.dipN);g->dirtyBot=true;}
   if(d&KEY_A){DipDo(g,S.dipIt[S.dipSel]);return;}
  }else{
   if(tDown&&ty>=WG_ACT_Y&&ty<WG_ACT_Y+S.dipN*WG_ACT_H){
    int i=(ty-WG_ACT_Y)/WG_ACT_H;if(i>=0&&i<S.dipN)DipDo(g,S.dipIt[i]);
    return;
   }
   if(d&KEY_A){S.dipOpen=false;g->dirtyBot=true;d&=~(u32)KEY_A;}   // A de novo fecha
  }
  if(d&KEY_B){S.dipOpen=false;g->dirtyBot=true;d&=~(u32)KEY_B;}
  if(!S.dipOpen)return;
  // a folha segue a mira: o mapa continua andando com as setas (layout Normal)
  if(!swap&&MoveSteps(h)){Px_WantTop();if(Hover()){ActBuild();BotLazy();}}
  if(d&KEY_L){SetZoom(S.zoom-1);g->dirty=true;}
  if(d&KEY_R){SetZoom(S.zoom+1);g->dirty=true;}
  return;
 }
 if(d&KEY_SELECT){
  if(!S.dead&&!W.ended){S.paused=!S.paused;g->dirty=true;}
 }
 if(d&KEY_L){SetZoom(S.zoom-1);Px_WantTop();if(Hover())g->dirtyBot=true;}
 if(d&KEY_R){SetZoom(S.zoom+1);Px_WantTop();if(Hover())g->dirtyBot=true;}
 if(d&KEY_Y)Act(g,UI_W_MODE);
 if(d&KEY_X){Act(g,UI_W_LEDGER);return;}
 if(d&KEY_B){
  if(W.c[S.me].atkOn){World_Recall(&W,S.me);Toast(STR_T_RECOLHIDA,0);g->dirtyBot=true;}
  S.warAsk=0;S.foundAsk=false;
 }
 if(d&KEY_A){
  if(W.ended){World_Continue(&W);S.paused=false;g->dirty=true;}
  else if(S.dead){
   if(S.sel&&W.c[S.sel].alive){
    World_SetPlayer(&W,S.sel);S.me=S.sel;S.dead=false;
    Toast(STR_ASSUMIU,S.me);g->dirty=true;
   }
  }else{DipOpen();g->dirtyBot=true;}   // A = interagir com o que esta' sob a mira
 }
 if(MoveSteps(h)){Px_WantTop();if(Hover())BotLazy();}
 if(swap)TouchMap(g,tDown,tUp,tHeld,tx,ty);
 else if(tDown&&CrisisTap(g,tx,ty))return;
 else if(tDown){int id=Ui_Hit(UI_WORLD,tx,ty);if(id>0)Act(g,id);}
}

// fim do dia: ficha, morte, faixa de noticia e redesenho
static void DayDone(GameState *g){
 if(Hover())g->dirtyBot=true;
 // virada de seculo (ADR-46): 1/1/1900, 1/1/2000... o ano toma a tela
 if(W.date.m==1&&W.date.d==1&&(W.date.y%100)==0&&W.date.y!=S.centY){S.centY=W.date.y;S.centT=330;}
 if(!W.c[S.me].alive){S.dead=true;S.paused=true;Toast(STR_CAIU,0);S.toastT=600;}
 if(W.msgDay!=S.msgSeen&&W.msg[0]){S.msgSeen=W.msgDay;S.bannerT=200;}
 if(W.ended)S.paused=true;
 // o mapa redesenha todo dia; o painel a cada dia ate' a velocidade 3 e em
 // dias alternados acima (numeros mudam devagar; poupa ~1 tela por quadro)
 Px_WantTop();
 if(S.speed<=3||(W.day&1))g->dirtyBot=true;
}
void WG_Frame(GameState *g){
 S.tick+=(u32)Px_Vb();PxS_Frame();
 // cena da cidade: so' os sprites vivos mudam; ~20 quadros/s (fundo em cache)
 if(g->screen==SCR_CITY&&(S.tick%3)==0&&!g->dirty){PxC_Scene(Px_BackTop(),S.cityIdx);Px_FlipTop();}
 if(S.toastT&&!--S.toastT)g->dirtyBot=true;
 if(S.botLazy&&((!S.vX&&!S.vY)||++S.botLazy>10)){g->dirtyBot=true;S.botLazy=0;}
 if(S.btnT&&!--S.btnT)g->dirtyBot=true;
 if(S.bannerT&&!--S.bannerT)g->dirty=true;
 if(S.centT){int v=Px_Vb();S.centT=(u16)(S.centT>v?S.centT-v:0);if(g->screen==SCR_WORLD)Px_WantTop();}
 // cidade tomada enquanto aberta: volta ao mapa (o painel nao tem mais dono)
 if((g->screen==SCR_CITY)&&(S.cityIdx<0||S.cityIdx>=W.nCity||W.city[S.cityIdx].owner!=S.me)){
  Menu_Go(g,SCR_WORLD);Toast(STR_T_CIDADE_PERDIDA,0);
 }
 // proposta nova da IA: pausa e mostra o despacho (resposta em PropInput).
 // So' no mapa: o despacho e' desenhado no painel do mapa; na cidade ou no
 // Gabinete ele espera a volta (antes o jogo pausava sem mostrar nada).
 if(W.prop.kind&&g->screen==SCR_WORLD){u32 k=W.prop.day*8u+W.prop.kind;
  if(k!=S.propSeen){S.propSeen=k;S.propRun=!S.paused;S.paused=true;g->dirty=true;g->dirtyBot=true;}}
 // rotulos/minimapa: algumas linhas do mundo por quadro (nunca engasga)
 if(g->screen==SCR_WORLD)PxW_StatsStep(S.speed>=5?6:12);
 // dia em andamento: uma fase por quadro (o trabalho do dia se espalha pelos
 // quadros em vez de pesar todo num so': o mapa nao engasga ao virar o dia).
 // Velocidades 4 e 5 (dia mais curto que as 4 fases): antes o dia INTEIRO
 // caia num quadro (~36 ms no ARM9 = 15-20 fps). Agora as fases correm ate'
 // um ORCAMENTO de tempo por quadro (Px_Ticks, ADR-36): no DS o dia leva uns
 // quadros e o mapa fica a 60 fps; no DSi (2x) cabe mais por quadro. No host
 // o relogio fica parado = dia inteiro no quadro, igual a antes.
 static const u8 FPD[6]={0,24,12,6,3,1}; // frames por dia, velocidade 1..5
 static const u16 BUDGET[6]={0,0,0,0,2100,4200};   // ~4 e ~8 ms (1/523656 s)
 bool slow=FPD[S.speed]>WD_PHASES;
 u32 t0=Px_Ticks();
 if(S.phase&&slow){
  World_DayPhase(&W,S.phase);
  if(++S.phase>=WD_PHASES){S.phase=0;DayDone(g);}
  return;
 }
 if(!S.phase){
  if(S.paused||S.dead||W.ended)return;
  S.frameAcc=(u16)(S.frameAcc+Px_Vb());   // relogio do jogo em VBlanks, nao em quadros desenhados
  if(S.frameAcc<FPD[S.speed])return;
  S.frameAcc=0;
  World_DayPhase(&W,0);S.phase=1;
  if(slow)return;
 }
 while(S.phase&&(u32)(Px_Ticks()-t0)<BUDGET[S.speed]){
  World_DayPhase(&W,S.phase);
  if(++S.phase>=WD_PHASES){S.phase=0;DayDone(g);}
 }
}

// Folga do fim do quadro (ADR-36): se o quadro ainda nao gastou ~11 ms dos
// 16,7 (sobra p/ o DMA do painel no VBlank e p/ variacao), pinta o mapa a'
// frente da camera ate' la'. No host (sem relogio) = 2 passos por quadro.
void WG_Idle(GameState *g){
 if(g->screen!=SCR_WORLD||g->dirty)return;
 enum { LIM=5700 };                       // ticks de 1/523656 s
 u32 used=Px_FrameTicks();
 bool clock=Px_Ticks()!=0;
 if(clock&&used>=LIM)return;
 PROF(PF_AHD,PxM_Ahead(&W,&S,clock?LIM-used:0,clock?64:2));
}

// ---------------- Gabinete (SCR_LEDGER) ----------------
static void LedgerAct(GameState *g,int tx){
 if(S.dead)return;
 WCountry *me=&W.c[S.me];
 switch(S.ledTab){
  case LT_FINANCAS:{   // juros (ADR-43): metade esquerda baixa, direita sobe
   WEco *e=&W.eco[S.me];
   if(tx<LG_ACT_X+FN_HALF){if(e->bank>0)e->bank--;}else if(e->bank<20)e->bank++;
   Toast(STR_T_JUROS,0);S.toastNum=e->bank;
  }break;
  case LT_MERCADO:S.mapMode=WMAP_COM;Menu_Go(g,SCR_WORLD);return;
  case LT_COMERCIO:Toast(WEco_BuildShip(&W,S.me)?STR_T_NAVIO:STR_T_SEM_NAVIO,0);break;
  case LT_DIPLO:
   if(me->bloc)WDip_LeaveBloc(&W,S.me);else WDip_FoundBloc(&W,S.me);
   break;
  case LT_POLITICA:case LT_SOCIEDADE:S.ledTab=LT_LEIS;S.lawCat=0xFF;break;   // as leis moram na aba propria
  default:break;
 }
 g->dirty=true;g->dirtyBot=true;
}
// aba Economia (ADR-47): toque no bem destaca; toque na politica aplica (ou
// mostra o motivo na pagina); "Mudar doutrina" abre a lei Modelo economico
static bool EcoTap(int tx,int ty,u32 *d){
 if(ty>=EC_GY&&ty<EC_GY+2*EC_GH&&tx>=EC_GX&&tx<EC_GX+EC_GN*EC_GW){
  int g=(tx-EC_GX)/EC_GW+EC_GN*((ty-EC_GY)/EC_GH);
  if(g>=GD_COUNT)return false;
  S.goodSel=(u8)g;S.ecoWhy=0;return true;
 }
 if(ty>=EC_PY&&ty<EC_PY+EC_PH&&tx>=EC_PX&&tx<EC_PX+GP_COUNT*EC_PW){
  if(S.dead)return false;
  u8 p=(u8)((tx-EC_PX)/EC_PW),g=S.goodSel%GD_COUNT;
  u16 why=Plan_Why(&W,S.me,g,p);
  S.ecoWhy=why;
  if(!why)Plan_Set(&W,S.me,g,p);
  return true;
 }
 if(ty>=LG_ACT_Y&&ty<LG_ACT_Y+LG_ACT_H&&tx>=LG_ACT_X&&tx<LG_ACT_X+LG_ACT_W){
  WEco *e=&W.eco[S.me];
  S.ledTab=LT_LEIS;S.lawCat=LW_MODELO;S.lawPick=e->lawCat==LW_MODELO?e->lawTo:e->law[LW_MODELO];return true;
 }
 if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)*d|=KEY_B;
 return false;
}
// aba Leis (ADR-40): grade de assuntos -> 5 leis -> Propor/Retirar. So' caneta;
// CIMA/BAIXO trocam a lei destacada como atalho. Devolve true se algo mudou.
static bool LawTap(int tx,int ty,u32 *d){
 WEco *e=&W.eco[S.me];
 if(S.lawCat>=LW_COUNT){
  if(ty>=LW_GY&&ty<LW_GY+3*LW_GH&&tx>=LW_GX&&tx<LW_GX+3*LW_GW){
   int k=(tx-LW_GX)/LW_GW+3*((ty-LW_GY)/LW_GH);if(k>=LW_COUNT)return false;
   S.lawCat=(u8)k;S.lawPick=e->lawCat==k?e->lawTo:e->law[k];return true;
  }
  if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)*d|=KEY_X;     // sai do Gabinete
  return false;
 }
 u8 cat=S.lawCat;
 if(ty>=LW_LY&&ty<LW_LY+LW_OPTS*LW_LH){S.lawPick=(u8)((ty-LW_LY)/LW_LH);return true;}
 if(ty>=LW_BY&&ty<LW_BY+LW_BH){
  if(tx>=4+LW_PW){S.lawCat=0xFF;return true;}             // voltar a' grade
  if(S.dead)return false;
  u8 pk=S.lawPick%LW_OPTS;
  if(e->lawCat==cat&&e->lawTo==pk){Law_Cancel(&W,S.me);Toast(STR_LEI_RETIRADA,0);return true;}
  if(Law_Propose(&W,S.me,cat,pk)){Toast(STR_LEI_PROPOSTA,0);S.toastNum=e->lawDays;return true;}
 }
 return false;
}
void WG_LedgerInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 bool moved=false;
 u8 tab0=S.ledTab;
 if(d&KEY_L){S.ledTab=(u8)((S.ledTab+LT_COUNT-1)%LT_COUNT);S.lawCat=0xFF;moved=true;}
 if(d&KEY_R){S.ledTab=(u8)((S.ledTab+1)%LT_COUNT);S.lawCat=0xFF;moved=true;}
 if(S.ledTab==LT_LEIS&&S.lawCat<LW_COUNT){
  if(d&KEY_UP){S.lawPick=(u8)((S.lawPick+LW_OPTS-1)%LW_OPTS);moved=true;}
  if(d&KEY_DOWN){S.lawPick=(u8)((S.lawPick+1)%LW_OPTS);moved=true;}
  if(d&KEY_B){S.lawCat=0xFF;d&=~(u32)KEY_B;moved=true;}   // B volta a' grade de assuntos
 }
 if(S.ledTab==LT_ECONOMIA){   // setas andam pela grade de bens (7 por linha)
  int g=S.goodSel%GD_COUNT,o=g;
  if(d&KEY_LEFT)g=(g+GD_COUNT-1)%GD_COUNT;
  if(d&KEY_RIGHT)g=(g+1)%GD_COUNT;
  if(d&KEY_UP)g=g>=EC_GN?g-EC_GN:g;
  if(d&KEY_DOWN)g=g+EC_GN<GD_COUNT?g+EC_GN:g;
  if(g!=o){S.goodSel=(u8)g;S.ecoWhy=0;moved=true;}
 }
 // abas e acao na tela de baixo: so' caneta (L/R trocam a aba como atalho)
 bool act=false;
 if(tDown){
  int tb=Lg_TabAt(tx,ty);
  if(tb>=0){S.ledTab=(u8)tb;S.lawCat=0xFF;moved=true;}
  else if(S.ledTab==LT_LEIS){if(LawTap(tx,ty,&d))moved=true;}
  else if(S.ledTab==LT_ECONOMIA){if(EcoTap(tx,ty,&d))moved=true;}
  else if(S.ledTab==LT_GRAF){
   if(ty>=GR_BY&&ty<GR_BY+2*(GR_BH+4)&&tx>=GR_BX&&tx<GR_BX+4*GR_BW){S.statSel=(u8)((tx-GR_BX)/GR_BW+4*((ty-GR_BY)/(GR_BH+4)));moved=true;}
   else if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)d|=KEY_B;
  }
  else if(ty>=LG_ACT_Y&&ty<LG_ACT_Y+LG_ACT_H&&tx>=LG_ACT_X&&tx<LG_ACT_X+LG_ACT_W)act=true;
  else if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)d|=KEY_B;
 }
 if(act){LedgerAct(g,tx);if(g->screen!=SCR_LEDGER)return;}
 if(d&(KEY_B|KEY_START|KEY_X)){Menu_Go(g,SCR_WORLD);return;}
 if(d&KEY_SELECT){S.paused=!S.paused;}
 if(S.ledTab!=tab0){Px_FadeIn(3);S.ecoWhy=0;}   // troca de aba: piscada curta (ADR-48)
 if(moved){g->dirty=true;g->dirtyBot=true;}
}

// ---------------- ranking (SCR_RANK) ----------------
void WG_RankInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 bool moved=false;
 int n=0;for(u8 ci=1;ci<W.nC;ci++)if(W.c[ci].alive)n++;
 int page=11;
 if(d&KEY_L){S.rankTab=(u8)((S.rankTab+RK_COUNT-1)%RK_COUNT);S.rankTop=0;moved=true;}
 if(d&KEY_R){S.rankTab=(u8)((S.rankTab+1)%RK_COUNT);S.rankTop=0;moved=true;}
 if(d&KEY_UP){S.rankTop=(s16)(S.rankTop-1);moved=true;}
 if(d&KEY_DOWN){S.rankTop=(s16)(S.rankTop+1);moved=true;}
 if(tDown){
  if(ty>=RK_TAB_Y&&ty<RK_TAB_Y+RK_TAB_H){S.rankTab=(u8)(tx*RK_COUNT/256);S.rankTop=0;moved=true;}
  else if(ty>=RK_BTN_Y&&ty<RK_BTN_Y+RK_BTN_H){
   if(tx>=RK_UP_X&&tx<RK_UP_X+RK_BTN_W){S.rankTop=(s16)(S.rankTop-page);moved=true;}
   else if(tx>=RK_DN_X&&tx<RK_DN_X+RK_BTN_W){S.rankTop=(s16)(S.rankTop+page);moved=true;}
   else if(tx>=RK_ME_X&&tx<RK_ME_X+RK_ME_W){int p=WRank_Pos(&W,S.me,S.rankTab);S.rankTop=(s16)(p-4);moved=true;}
  }
  else if(ty>=LG_BACK_Y&&ty<LG_BACK_Y+LG_ACT_H)d|=KEY_B;
 }
 if(S.rankTop>n-8)S.rankTop=(s16)(n-8);
 if(S.rankTop<0)S.rankTop=0;
 if(d&(KEY_B|KEY_START)){Menu_Go(g,SCR_WORLD);return;}
 if(d&KEY_SELECT){S.paused=!S.paused;}
 if(moved){g->dirty=true;g->dirtyBot=true;}
}

// ---------------- tela da cidade (SCR_CITY): mapa de lotes ----------------
// Setas movem a mira de lote em lote (repete segurando); A num lote livre
// abre o catalogo. Painel so' caneta: minimapa escolhe o lote, "Construir
// aqui", setas do catalogo, Construir/Cancelar. B fecha o catalogo / volta.
static u8 FirstPickAt(int ci,int lot){
 for(u8 k=1;k<BK_COUNT;k++)if(City_CanBuildAt(&W,ci,lot,k))return k;
 for(u8 k=1;k<BK_COUNT;k++)if(City_CanBuild(&W,ci,k))return k;
 return BK_FAZENDA;
}
static void OpenPick(int ci){S.cityPick=true;S.pickK=FirstPickAt(ci,S.cityLot);}
// acao do lote (A ou o botao): estaleiro lanca navio; obra pronta amplia
// (ADR-44); lote livre abre o catalogo
static void LotAct(int ci){
 WCity *c=&W.city[ci];
 int sl=City_LotSlot(c,S.cityLot);
 if(sl>=0&&c->bld[sl]==BK_ESTALEIRO){Toast(WEco_BuildShip(&W,S.me)?STR_T_NAVIO:STR_T_SEM_NAVIO,0);return;}
 if(sl>=0){
  u16 why=City_UpWhy(&W,ci,sl);
  if(why){Toast(why,0);return;}
  if(City_Upgrade(&W,ci,sl)){Toast(STR_T_AMPLIA,0);S.toastNum=c->bqDays;}else Toast(STR_T_SEM_OURO,0);
  return;
 }
 if(City_LotBase(&W,ci,S.cityLot)==LK_TERRA&&!(c->bq&&c->bqLot==S.cityLot))OpenPick(ci);
}
static void CityGoLot(int ci){
 (void)ci;S.cityPick=false;
}
void WG_CityInput(GameState *g,u32 d,bool tDown,int tx,int ty){
 int ci=S.cityIdx;
 if(ci<0||ci>=W.nCity||W.city[ci].owner!=S.me){Menu_Go(g,SCR_WORLD);return;}
 bool moved=false,top=false;
 // setas: lote a lote; segurando, repete a cada 5 quadros depois de 14
 u32 h=keysHeld();
 int dx=((h&KEY_RIGHT)?1:0)-((h&KEY_LEFT)?1:0),dy=((h&KEY_DOWN)?1:0)-((h&KEY_UP)?1:0);
 if(dx||dy){
  bool go=(d&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))||(S.cityHold>14&&(S.cityHold%5)==0);
  if(S.cityHold<255)S.cityHold++;
  if(go&&!S.cityPick){
   int x=S.cityLot%CG_W+dx,y=S.cityLot/CG_W+dy;
   if(x<0)x=0;
   if(x>=CG_W)x=CG_W-1;
   if(y<0)y=0;
   if(y>=CG_H)y=CG_H-1;
   S.cityLot=(u8)(y*CG_W+x);moved=true;top=true;
  }
 }else S.cityHold=0;
 if(d&KEY_A&&!S.cityPick){LotAct(ci);moved=true;top=true;}
 if(tDown){
  if(S.cityPick&&ty>=CY_PICK_Y){
   if(ty<CY_OK_Y&&tx<CY_ARW_W)d|=KEY_LEFT;
   else if(ty<CY_OK_Y&&tx>=256-CY_ARW_W)d|=KEY_RIGHT;
   else if(ty>=CY_OK_Y&&tx>=CY_OK_X&&tx<CY_OK_X+CY_OK_W){
    u8 k=S.pickK;
    if(City_BuildAt(&W,ci,S.cityLot,k)){Toast(STR_T_OBRA_INI,0);S.cityPick=false;top=true;}
    else if(City_CanBuildAt(&W,ci,S.cityLot,k))Toast(STR_T_SEM_OURO,0);
   }
   else if(ty>=CY_OK_Y&&tx>=CY_NO_X)S.cityPick=false;
   if(d&(KEY_LEFT|KEY_RIGHT)){
    int dir=(d&KEY_RIGHT)?1:-1;
    S.pickK=(u8)(1+((S.pickK-1+dir+(BK_COUNT-1))%(BK_COUNT-1)));
   }
   moved=true;
  }else if(tx>=CY_MINI_X&&tx<CY_MINI_X+CG_W*CY_CELL&&ty>=CY_MINI_Y&&ty<CY_MINI_Y+CG_H*CY_CELL){
   int l=((ty-CY_MINI_Y)/CY_CELL)*CG_W+(tx-CY_MINI_X)/CY_CELL;
   S.cityLot=(u8)l;CityGoLot(ci);moved=true;top=true;
  }else if(!S.cityPick&&ty>=CY_BTN_Y&&ty<CY_BTN_Y+CY_BTN_H){LotAct(ci);moved=true;top=true;}
 }
 if(d&KEY_B){
  if(S.cityPick){S.cityPick=false;moved=true;}
  else{Menu_Go(g,SCR_WORLD);return;}
 }
 if(!S.cityPick&&(d&(KEY_L|KEY_R))){
  // proxima/anterior cidade sua
  int dir=(d&KEY_R)?1:-1;
  for(int k=1;k<=W.nCity;k++){
   int j=((ci+dir*k)%W.nCity+W.nCity)%W.nCity;
   if(W.city[j].owner==S.me){S.cityIdx=(s16)j;S.cityCamX=0xFFFF;S.cityLot=(u8)City_Plaza(&W,j);S.curX=(s16)W.city[j].x;S.curY=(s16)W.city[j].y;Center();Hover();break;}
  }
  g->dirty=true;
 }
 if(d&KEY_START){Menu_Go(g,SCR_WORLD);return;}
 if(d&KEY_SELECT){S.paused=!S.paused;}
 if(moved)g->dirtyBot=true;
 if(top)Px_WantTop();
}
