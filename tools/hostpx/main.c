// Smoke host do RENDER (sem libnds). Roda o jogo de verdade no PC e falha se:
//   (a) travar (timeout no run.sh) — pega laco infinito no bake/desenho;
//   (b) sobrar pixel TRANSPARENTE (bit 15 = 0) em qualquer uma das telas — a
//       classe "tela preta" do postmortem (ADR-13).
// Navega como um jogador (teclas injetadas no stub): menus da v0.9 -> partida
// no mapa-mundi (zooms, guerra, modos de mapa, pausa, opcoes, 10 anos na
// velocidade 5) -> volta ao menu -> jogo classico 32x24 (bake do mapa).
// Salva cada passo como PPM (tela de cima sobre a de baixo) em $SHOTS (ou no
// diretorio do binario). Screenshot do host NAO substitui o melonDS (postmortem
// sec 7.3): serve p/ revisar layout e pegar regressao visual grosseira.
// Uso: tools/hostpx/run.sh   (compila com ASan/UBSan e roda)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nds.h"
#include "game.h"
#include "px.h"
#include "settings.h"
#include "menu.h"
#include "wgame.h"
#include "wwar.h"
#include "wsoc.h"
#include "city.h"
#include "wfort.h"
#include "wlaw.h"
#include "wstat.h"
#include "str.h"
#include "pal.h"
#include "city.h"
#include "wdiplo.h"
#include "history.h"
#include "ui.h"
#include "wrank.h"
#include "wecon.h"

static const char *dir=0;   // sem SHOTS nao grava screenshot
static int bad=0;

static long Trans(const u16*b){long n=0;for(int i=0;i<256*192;i++)if(!(b[i]&0x8000))n++;return n;}
static void Frames(GameState*g,int n){
 static u32 f=0;
 for(int k=0;k<n;k++,f++){
  In_Handle(g);
  Game_Frame(g);
  Px_Tick(g);                       // bake do mapa classico: onde morava o hang
  if(g->dirty){Px_Top(g);Px_Bot(g);g->dirty=false;g->dirtyBot=false;Px_TakeTop();}
  else if(g->dirtyBot){Px_Bot(g);g->dirtyBot=false;if(Px_TakeTop())Px_Top(g);}
  else if(Px_TakeTop())Px_Top(g);
  else if((f%15)==0){Px_Top(g);if(!(g->screen==SCR_WORLD||g->screen==SCR_CITY||g->screen==SCR_LEDGER||g->screen==SCR_RANK))Px_Bot(g);}   // = main.c (BotStill)
  WG_Idle(g);                       // = main.c: render-ahead na folga
  Px_VSync();                       // troca de pagina como no DS (ADR-36)
  host_down=0;                      // keysDown so' vale 1 frame
 }
}
static void Px8(FILE*f,u16 c){u8 p[3]={(u8)((c&31)<<3),(u8)(((c>>5)&31)<<3),(u8)(((c>>10)&31)<<3)};fwrite(p,1,3,f);}
static void Shot(GameState*g,const char*name){
 // forca um render completo antes de fotografar
 Px_Top(g);Px_Bot(g);
 long t=Trans(Px_BackTop()),b=Trans(Px_BackBot());
 printf("  %-22s transTop=%ld transBot=%ld\n",name,t,b);
 if(t||b){printf("FALHA: %s com pixel transparente (bit15=0)\n",name);bad=1;}
 if(!dir)return;
 char p[512];snprintf(p,sizeof p,"%s/%s.ppm",dir,name);
 FILE*f=fopen(p,"wb");if(!f)return;
 fprintf(f,"P6 256 388 255\n");
 for(int i=0;i<256*192;i++)Px8(f,Px_BackTop()[i]);
 for(int i=0;i<256*4;i++)Px8(f,0x8000|(4<<10)|(4<<5)|4);
 for(int i=0;i<256*192;i++)Px8(f,Px_BackBot()[i]);
 fclose(f);
}
static void Press(GameState*g,u32 k){host_down=k;host_held=k;Frames(g,1);host_held=0;Frames(g,2);}
static void Touch(GameState*g,int x,int y){host_tx=x;host_ty=y;host_held=KEY_TOUCH;Frames(g,1);host_held=0;Frames(g,1);}
// toca a linha da folha de acoes que tem o item 'it' (DM_*); false = nao ha'
static bool TapAct(GameState*g,u8 it){
 WGame*s=WG_Ptr();
 for(int i=0;i<s->dipN;i++)if(s->dipIt[i]==it){Touch(g,128,WG_ACT_Y+i*WG_ACT_H+6);return true;}
 return false;
}
// resposta da proposta: toque em Aceitar (esq.) ou Recusar (dir.)
static void Answer(GameState*g,bool yes){Touch(g,yes?60:190,PM_BTN_Y+10);}
static void Hold(GameState*g,u32 k,int n){host_down=k;host_held=k;Frames(g,1);for(int i=1;i<n;i++){host_held=k;Frames(g,1);}host_held=0;Frames(g,1);}

int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 if(getenv("SHOTS"))dir=getenv("SHOTS");
 Settings_Init();
 Px_Init();
 GameState *g=Game_Ptr();
 Game_Init(g);
 g->seed=DEFAULT_SEED;g->dirty=true;

 printf("[menus v0.9]\n");
 Frames(g,40);                       Shot(g,"01_menu");
 Press(g,KEY_A);                     Shot(g,"02_modo");
 Press(g,KEY_DOWN);                  Shot(g,"03_modo_ia");
 Press(g,KEY_A);                     Shot(g,"04_preparar");
 Press(g,KEY_A);                     Shot(g,"05_nacao");
 Press(g,KEY_R);Press(g,KEY_RIGHT);  Shot(g,"06_nacao_europa");
 Press(g,KEY_L);Menu_Ptr()->nat=(s16)Nat_Find("GBR");

 printf("[mapa-mundi]\n");
 clock_t t0=clock();
 Press(g,KEY_A);
 printf("  gerou o mundo em %.0f ms (PC, ASan)\n",(double)(clock()-t0)*1000.0/CLOCKS_PER_SEC);
 if(g->screen!=SCR_WORLD){printf("FALHA: nao entrou no SCR_WORLD\n");return 1;}
 // abertura (ADR-46): o seculo toma a tela por ~5 s e some sozinho
 if(!WG_Ptr()->centT){printf("FALHA: a partida nao abriu com a transicao de seculo\n");bad=1;}
 Frames(g,30);Shot(g,"07a_seculo");
 Frames(g,340);
 if(WG_Ptr()->centT){printf("FALHA: a transicao de seculo nao terminou\n");bad=1;}
 Shot(g,"07_mundo_z3");
 Press(g,KEY_SELECT);Frames(g,600);  Shot(g,"08_mundo_50dias");
 Press(g,KEY_SELECT);
 Press(g,KEY_L);                     Shot(g,"09_z2");
 Press(g,KEY_L);Press(g,KEY_L);      Shot(g,"10_z0");
 Press(g,KEY_R);Press(g,KEY_R);Press(g,KEY_R);Press(g,KEY_R);Shot(g,"12_z4");
 Press(g,KEY_L);                     Shot(g,"11_z3");
 // pan suave: segurar a seta tem que andar o mapa pixel a pixel e parar
 {WGame*s=WG_Ptr();s32 p0=s->pX;Hold(g,KEY_RIGHT,20);s32 p1=s->pX;Frames(g,30);
  printf("  pan: %d px em 20 quadros, deslizou %d px ao soltar, v=%d\n",(int)((p1-p0)>>4),(int)((s->pX-p1)>>4),s->vX);
  if(p1-p0<20*16*2){printf("FALHA: segurar a seta nao andou o mapa\n");bad=1;}
  if(s->vX){printf("FALHA: o mapa nao parou depois de soltar\n");bad=1;}}
 // cache em anel (ADR-36): o mapa montado por faixas durante o pan tem que
 // ser IGUAL ao repintado do zero (o pontilhado e' preso ao mundo). So' o
 // aro de 1px da vista pode diferir (la' o vizinho de fora e' desconhecido).
 {static u16 A[256*192],B[256*192];WGame*s=WG_Ptr();
  static const u32 DIRS[4]={KEY_RIGHT|KEY_DOWN,KEY_LEFT,KEY_UP|KEY_LEFT,KEY_DOWN};
  for(int z=1;z<6;z++){
   while(s->zoom<z)Press(g,KEY_R);
   while(s->zoom>z)Press(g,KEY_L);
   long bad1=0;
   for(int k=0;k<4;k++){
    Hold(g,DIRS[k],9+k*5);Frames(g,6);
    PxM_Update(WG_World(),s);PxM_Blit(A);PxM_Overlay(A,WG_World(),s,1);
    PxM_Invalidate();PxM_Update(WG_World(),s);PxM_Blit(B);PxM_Overlay(B,WG_World(),s,1);
    for(int y=WG_MAP_Y+1;y<191;y++)for(int x=1;x<255;x++)if(A[y*256+x]!=B[y*256+x])bad1++;
   }
   printf("  anel z%d: %ld px diferentes entre pan incremental e repintura\n",z,bad1);
   if(bad1){printf("FALHA: cache em anel divergiu da repintura (z%d)\n",z);bad=1;}
  }
  while(s->zoom>3)Press(g,KEY_L);}
 {World*w=WG_World();u8 fra=World_CountryOfNat(w,Nat_Find("FRA"));WG_Goto(w->c[fra].capX+3,w->c[fra].capY+4);}
 Hold(g,KEY_DOWN,3);
 Shot(g,"13_cursor_franca");
 Press(g,KEY_A);                     Shot(g,"14_folha_acoes");
 if(!WG_Ptr()->dipOpen){printf("FALHA: A sobre a mira nao abriu a folha de acoes\n");bad=1;}
 u32 c0=WG_World()->c[WG_Ptr()->me].cells,c1=c0;
 World_Recruit(WG_World(),WG_Ptr()->me);World_Recruit(WG_World(),WG_Ptr()->me); // recruta com o ouro inicial
 WG_Ptr()->pct=100;
 // ADR-45: exigir abre a folha de objetivo; o objetivo abre a CRISE (40 dias);
 // so' no fim do prazo, sem acordo, vira guerra
 {World*w=WG_World();u8 me=WG_Ptr()->me,fra=World_CountryOfNat(w,Nat_Find("FRA"));
  if(TapAct(g,DM_GUERRA)){
   if(!WG_Ptr()->dipOpen||WG_Ptr()->actKind!=AK_META){printf("FALHA: Exigir nao abriu a folha de objetivo\n");bad=1;}
   Shot(g,"14b_objetivo");
   if(!TapAct(g,DM_META_TERRA)){printf("FALHA: folha de objetivo sem Territorio\n");bad=1;}
   if(WG_Ptr()->dipOpen){printf("FALHA: a folha nao fechou depois da acao\n");bad=1;}
   u8 tg=WG_Ptr()->dipTo;int k=War_Find(w,me,tg);    // alvo = o pais sob a mira (a GBR ja' esta' em guerra com a Franca em 1800)
   printf("  exigir territorio de %s\n",tg?NATIONS[w->c[tg].nat].tag:"-");
   if(k<0||w->cr[k].st!=CRS_CRISE){printf("FALHA: exigir nao abriu a crise\n");bad=1;}
   else{
    // a Franca e' forte: o jogador fica com o mesmo tamanho da coalizao dela p/ a crise nao acabar em recuo
    w->c[me].troops+=War_Strength(w,k,1);
    if(w->prop.kind)Answer(g,false);
    Frames(g,4);Shot(g,"15a_crise");
    int n=War_CallAllies(w,k,me);printf("  crise aberta: prazo %d dias, exige %d celulas, aliados chamados %d\n",w->cr[k].days,(int)w->cr[k].demand,n);
    bool p=WG_Ptr()->paused;WG_Ptr()->paused=false;
    for(int i=0;i<4000&&w->cr[k].st==CRS_CRISE;i++){if(w->prop.kind)Answer(g,false);WG_Ptr()->paused=false;Frames(g,1);}
    WG_Ptr()->paused=p;
    printf("  fim da crise: %s\n",World_AtWar(w,me,tg)?"guerra":(w->cr[k].st?"?":"resolvida sem guerra"));
   }
  }
  if(fra&&World_AtWar(w,me,fra)){if(w->c[me].atkOn)World_Recall(w,me);WG_Goto(w->c[fra].capX+3,w->c[fra].capY+4);WG_Ptr()->pct=100;Press(g,KEY_A);
   if(!TapAct(g,DM_ATACAR)){printf("FALHA: em guerra, a folha nao tem Atacar\n");bad=1;}
   if(WG_Ptr()->dipOpen){printf("FALHA: a folha nao fechou depois do ataque\n");bad=1;}
   Shot(g,"15e_guerra_placar");}
  else{World_DeclareWar(w,me,fra);w->c[me].troops+=3000;World_Attack(w,me,fra,100);}   // a crise acabou em acordo: guerra direta p/ o resto do roteiro
 }
 Press(g,KEY_SELECT);
 // resposta da IA: quantos dias ate' o alvo abrir frente de volta (contra-ataque)
 {World*w=WG_World();u8 me=WG_Ptr()->me,fra=World_CountryOfNat(w,Nat_Find("FRA"));u32 d0=w->day;int resp=-1;
  for(int i=0;i<240;i++){Frames(g,1);u32 c=w->c[me].cells;if(c>c1)c1=c;
   if(resp<0&&fra&&w->c[fra].atkOn&&w->c[fra].atkTo==me)resp=(int)(w->day-d0);}
  if(resp>=0)printf("  IA respondeu ao ataque em %d dia(s)\n",resp);
  else printf("  IA nao contra-atacou em %u dias (reserva baixa ou sem fronteira)\n",w->day-d0);}
 Shot(g,"15_guerra");
 WG_Ptr()->bannerT=0;Frames(g,120);Shot(g,"15b_frente");
 WG_Ptr()->bannerT=0;Press(g,KEY_R);Shot(g,"15c_z4_cidades");
 // Z5 (ADR-38): soldados lutando na frente; mira numa celula da frente
 {World*w=WG_World();u8 me=WG_Ptr()->me;int fx=-1,fy=-1;(void)me;
  // qualquer frente terrestre aberta no mundo (a do jogador pode ter fechado:
  // o avanco agora e' gradual e a frente sangra)
  bool anyF=false;for(u8 j=1;j<w->nC;j++)if(w->c[j].alive&&w->c[j].atkOn&&w->c[j].atkTo)anyF=true;
  if(!anyF){for(u8 j=1;j<w->nC;j++)if(j!=me&&w->c[j].alive&&World_Borders(w,me,j)){
    World_DeclareWar(w,me,j);w->c[me].troops+=3000;World_Attack(w,me,j,100);break;}
   bool p=WG_Ptr()->paused;WG_Ptr()->paused=false;Frames(g,30);WG_Ptr()->paused=p;}
  u8 fa=0;for(int y=0;y<w->h&&fx<0;y++)for(int x=1;x<w->w-1;x++){u32 i=World_Idx(w,x,y);u8 o=W_OWNER[i];
   if(!o||!w->c[o].atkOn||!w->c[o].atkTo)continue;
   if(W_OWNER[i+1]==w->c[o].atkTo||W_OWNER[i-1]==w->c[o].atkTo){fx=x;fy=y;fa=o;break;}}
  printf("  frente na tela: %s\n",fx>=0?NATIONS[w->c[fa].nat].tag:"nenhuma frente no mundo");
  if(fx>=0)WG_Goto(fx,fy);
  if(w->prop.kind)Answer(g,false);          // despacho pendente come as teclas
  WG_Ptr()->bannerT=0;Press(g,KEY_R);Frames(g,8);Shot(g,"15d_z5_batalha");
  if(WG_Ptr()->zoom!=5){printf("FALHA: R nao chegou ao Z5\n");bad=1;}
  Press(g,KEY_L);}
 Press(g,KEY_L);

 printf("[cidades]\n");
 {
  World *w=WG_World();WGame *s=WG_Ptr();
  int na=0;for(int i=0;i<w->nCity;i++)if(w->city[i].tier==CT_METROPOLE)na++;
  printf("  %u cidades no mundo, %d metropoles\n",w->nCity,na);
  int cap=City_Near(w,w->c[s->me].capX,w->c[s->me].capY,3);
  if(cap<0){printf("FALHA: capital do jogador sem cidade\n");bad=1;}
  else{
   if(w->prop.kind)Answer(g,false);         // despacho da IA pendente (trava o mapa): recusa antes
   WG_Goto(w->city[cap].x,w->city[cap].y);
   w->c[s->me].gold+=5000;
   Press(g,KEY_A);
   if(!s->dipOpen||s->actKind!=AK_CIDADE){printf("FALHA: A na capital nao abriu a folha da cidade\n");bad=1;}
   TapAct(g,DM_CIDADE);
   if(g->screen!=SCR_CITY){printf("FALHA: folha nao abriu a cidade\n");bad=1;}
   Shot(g,"22_cidade");
   // bug dos pedestres: sair e voltar a' mesma cidade tem que repintar o fundo
   {static u16 a[256*172];Px_Top(g);for(int i=0;i<256*172;i++)a[i]=Px_BackTop()[20*256+i];
    Press(g,KEY_B);Frames(g,4);
    if(g->screen!=SCR_WORLD){printf("FALHA: B nao saiu da cidade\n");bad=1;}
    Shot(g,"22b_saiu_da_cidade");
    Press(g,KEY_A);TapAct(g,DM_CIDADE);Px_Top(g);
    long diff=0;for(int i=0;i<256*172;i++){u16 p=Px_BackTop()[20*256+i];if(p!=a[i])diff++;}
    printf("  reabrir a cidade: %ld px diferentes (so' pedestres/fumaca)\n",diff);
    if(diff>256*172/8){printf("FALHA: cena da cidade reaberta mostra o mapa (cache velho)\n");bad=1;}}
   // mapa da cidade: lote livre pelo minimapa -> Construir aqui -> catalogo
   {int lot=-1;for(int i=0;i<CG_N;i++)if(City_LotBase(w,cap,i)==LK_TERRA&&City_LotSlot(&w->city[cap],i)<0){lot=i;break;}
    if(lot<0){printf("FALHA: capital sem lote livre\n");bad=1;lot=0;}
    Touch(g,CY_MINI_X+(lot%CG_W)*CY_CELL+3,CY_MINI_Y+(lot/CG_W)*CY_CELL+3);
    if(s->cityLot!=lot){printf("FALHA: toque no minimapa nao escolheu o lote\n");bad=1;}
    Hold(g,KEY_RIGHT,1);Hold(g,KEY_LEFT,1);
    if(s->cityLot!=lot){printf("FALHA: setas nao andaram lote a lote (%d)\n",s->cityLot);bad=1;}
    Touch(g,128,CY_BTN_Y+8);}
   if(!s->cityPick){printf("FALHA: Construir aqui nao abriu o catalogo\n");bad=1;}
   Shot(g,"23_escolher_obra");
   for(int k=0;k<BK_COUNT&&!City_CanBuildAt(w,cap,s->cityLot,s->pickK);k++)Touch(g,250,CY_PICK_Y+18);
   u8 want=s->pickK;Touch(g,CY_OK_X+10,CY_OK_Y+8);
   printf("  obra pedida: %d, em construcao: %d\n",want,w->city[cap].bq);
   if(!w->city[cap].bq){printf("FALHA: obra nao entrou em construcao\n");bad=1;}
   Frames(g,30);Shot(g,"24_cidade_andaime");
   s->paused=false;s->speed=5;for(int i=0;i<400&&w->city[cap].bq;i++)Frames(g,1);
   s->paused=true;
   bool built=false;for(int k=0;k<CITY_SLOTS;k++)if(w->city[cap].bld[k]==want)built=true;
   if(!built){printf("FALHA: obra nao ficou pronta\n");bad=1;}
   Frames(g,8);Shot(g,"25_cidade_pronta");
   // ampliar (ADR-44): a obra recem-feita sobe de nivel pelo botao do lote
   {WCity *c=&w->city[cap];int sl=-1;for(int k=0;k<CITY_SLOTS;k++)if(c->bld[k]==want)sl=k;
    if(sl>=0&&c->bpos[sl]<CG_N){
     s->cityLot=c->bpos[sl];w->c[s->me].gold+=5000;int lv=City_Level(c,sl);u16 why=City_UpWhy(w,cap,sl);
     Touch(g,128,CY_BTN_Y+8);Shot(g,"25b_ampliando");
     if(why)printf("  ampliar: indisponivel (%s)\n",Str(why));
     else{
      if(!c->bq||!c->bqUp){printf("FALHA: botao Ampliar nao pos a obra na fila\n");bad=1;}
      s->paused=false;for(int i=0;i<400&&c->bq;i++)Frames(g,1);s->paused=true;
      printf("  ampliar: nivel %d -> %d\n",lv,City_Level(c,sl));
      if(City_Level(c,sl)!=lv+1){printf("FALHA: ampliacao nao subiu o nivel\n");bad=1;}
     }}
    // mapa 20x12 (ADR-44): mira no canto oposto rola a camera
    s->cityLot=(u8)(CG_N-1);for(int i=0;i<12;i++)Frames(g,1);
    printf("  camera da cidade: %d,%d\n",s->cityCamX,s->cityCamY);
    if(!s->cityCamX||!s->cityCamY){printf("FALHA: a camera da cidade nao rolou ate' o canto\n");bad=1;}
    Shot(g,"25c_cidade_rolada");}
   Press(g,KEY_B);
  }
  // fundar: celula propria longe de cidades
  // a ilha do jogador ja' esta cheia de cidades: testa com um pais grande
  u8 big=World_CountryOfNat(w,Nat_Find("RUS"));
  if(big){World_SetPlayer(w,big);s->me=big;w->c[big].gold+=5000;}
  int fx=-1,fy=-1;
  for(int y=0;y<w->h&&fx<0;y++)for(int x=0;x<w->w;x++)if(City_CanFound(w,s->me,x,y)){fx=x;fy=y;break;}
  if(fx>=0){
   u16 n0=w->nCity;
   WG_Goto(fx,fy);
   Press(g,KEY_A);                                                    // folha: "Fundar cidade aqui — preco"
   if(s->actKind!=AK_TERRA){printf("FALHA: mira na terra propria nao deu a folha de territorio (%d)\n",s->actKind);bad=1;}
   Shot(g,"26_fundar_preco");
   bool ok=TapAct(g,DM_FUNDAR);
   printf("  fundar pela folha (%s): %u -> %u cidades\n",ok?"item presente":"SEM item",n0,w->nCity);
   if(w->nCity!=n0+1){printf("FALHA: nao fundou a cidade\n");bad=1;}
   Shot(g,"27_fundada");
  }else printf("  (sem celula livre p/ fundar)\n");
  // estilos arquitetonicos por cultura (so' render: cena de cidades alheias)
  static const char *const CUL[]={"Pequim","Edo","Cairo","Kumasi","Chota"};
  for(unsigned k=0;k<sizeof CUL/sizeof CUL[0];k++){
   int ci=-1;for(int i=0;i<w->nCity;i++){char nm[40];City_Name(w,i,nm,40);if(!strcmp(nm,CUL[k]))ci=i;}
   if(ci<0){printf("  (sem %s)\n",CUL[k]);continue;}
   s->cityIdx=(s16)ci;s->citySlot=0;s->cityPick=false;g->screen=SCR_CITY;
   char nm[48];snprintf(nm,sizeof nm,"28_estilo_%u",k);Shot(g,nm);
  }
  g->screen=SCR_WORLD;
 }
 printf("  jogador: %u -> %u celulas no auge da frente (desembarque)\n",c0,c1);
 if(c1<=c0){printf("FALHA: o ataque do jogador nao tomou nenhuma celula\n");bad=1;}
 Press(g,KEY_Y);                     Shot(g,"16_terreno");
 Press(g,KEY_Y);                     Shot(g,"17_diplomatico");
 Press(g,KEY_Y);                     Shot(g,"17b_comercial");
 Press(g,KEY_Y);                     Shot(g,"17c_ideologico");
 Press(g,KEY_Y);
 if(WG_Ptr()->mapMode!=WMAP_POL){printf("FALHA: Y nao voltou ao mapa politico (%d)\n",WG_Ptr()->mapMode);bad=1;}
 printf("[construir: fortificacoes (ADR-38)]\n");
 {
  World *w=WG_World();WGame *s=WG_Ptr();u8 me=s->me;
  if(w->prop.kind)Answer(g,false);
  w->c[me].gold+=3000;
  // celula da propria terra longe de cidade/forte
  int bx=-1,by=-1;const WCountry *c=&w->c[me];
  for(int y=c->by0;y<=c->by1&&bx<0;y++)for(int x=c->bx0;x<=c->bx1;x++)
   if(Fort_CanBuild(w,me,x,y,FT_FORTE)&&City_Near(w,x,y,2)<0){bx=x;by=y;break;}
  if(bx<0){printf("FALHA: nenhuma celula fortificavel no pais do jogador\n");bad=1;}
  else{
   WG_Goto(bx,by);
   const Btn *b=Ui_Tbl(UI_WORLD);int k=0;while(b[k].id!=UI_W_CITY)k++;
   Touch(g,b[k].x+b[k].w/2,b[k].y+b[k].h/2);
   Shot(g,"40_folha_construir");
   if(!s->dipOpen||s->actKind!=AK_OBRA){printf("FALHA: Construir nao abriu a folha de obras\n");bad=1;}
   int n0=w->nFort;
   if(!TapAct(g,DM_FORTE)){printf("FALHA: folha de obras sem Forte\n");bad=1;}
   if(w->nFort!=n0+1){printf("FALHA: forte nao foi erguido\n");bad=1;}
   else{
    int fk=w->nFort-1;
    for(int i=0;i<3000&&w->fort[fk].days;i++){if(w->prop.kind)Answer(g,false);s->paused=false;Frames(g,1);}
    s->paused=true;
    printf("  forte %s; bonus de defesa na celula vizinha: %d%%\n",w->fort[fk].days?"NAO pronto":"pronto",
     Fort_DefPct(w,World_Idx(w,bx+1,by),w->fort[fk].owner));
    if(w->fort[fk].days){printf("FALHA: forte nao terminou\n");bad=1;}
    if(w->prop.kind)Answer(g,false);
    WG_Goto(bx,by);for(int i=0;i<6&&s->zoom<4;i++)Press(g,KEY_R);
    s->bannerT=0;Shot(g,"41_forte_z4");
    printf("  forte em (%d,%d) dono %d tipo %d; mira (%d,%d) zoom %d\n",w->fort[fk].x,w->fort[fk].y,w->fort[fk].owner,w->fort[fk].kind,s->curX,s->curY,s->zoom);
    {const u16 *t=Px_BackTop();int px=s->rX>>4,py=WG_MAP_Y+(s->rY>>4),n=0;
     for(int y=py-12;y<=py+12;y++)for(int x=px-12;x<=px+12;x++)if(x>=0&&x<256&&y>=0&&y<192&&t[y*256+x]==PXC(20,19,16))n++;
     printf("  pixels de pedra do forte perto da mira (%d,%d): %d\n",px,py,n);}
    Press(g,KEY_R);Shot(g,"41b_forte_z5");
    for(int i=0;i<6&&s->zoom>3;i++)Press(g,KEY_L);
   }
  }
  printf("  fortes no mundo: %u, armas vendidas %u, guerras instigadas %u, obras do investimento privado %u\n",w->nFort,w->nArms,w->nInstig,w->nInvest);
 }
 printf("[diplomacia + gabinete]\n");
 {
  World *w=WG_World();WGame *s=WG_Ptr();
  u8 fra=World_CountryOfNat(w,Nat_Find("FRA"));
  if(!fra||!w->c[fra].alive)for(u8 i=1;i<w->nC&&!fra;i++)if(w->c[i].alive&&i!=s->me)fra=i;
  if(w->prop.kind)Answer(g,false);
  s->dipOpen=false;
  WG_Goto(w->c[fra].capX,w->c[fra].capY);
  s->sel=fra;s->selLand=false;s->hcity=-1;
  Touch(g,57,169);   // botao bandeira
  if(!s->dipOpen){printf("FALHA: bandeira nao abriu a folha de acoes\n");bad=1;}
  Shot(g,"34_diplomacia_menu");
  Press(g,KEY_B);
  if(s->dipOpen){printf("FALHA: B nao fechou a folha de acoes\n");bad=1;}
  // proposta da IA: o jogo pausa e mostra o despacho; A aceita
  w->prop.kind=PR_NONE;
  WDip_Propose(w,PR_ALIANCA,fra,0,0,0);s->paused=false;
  Frames(g,4);Shot(g,"35_proposta");
  if(!s->paused){printf("FALHA: proposta nao pausou o jogo\n");bad=1;}
  Press(g,KEY_A);
  if(!w->prop.kind){printf("FALHA: A respondeu a proposta (o painel e' so' caneta)\n");bad=1;}
  {u32 pd=w->prop.day;Answer(g,true);
   // depois de aceitar o jogo volta a correr: outra proposta pode chegar no mesmo quadro
   if(w->prop.kind==PR_ALIANCA&&w->prop.from==fra&&w->prop.day==pd){printf("FALHA: toque em Aceitar nao respondeu a proposta\n");bad=1;}}
  printf("  alianca aceita: %s\n",WDip_Allied(w,s->me,fra)?"sim":"nao (pais cheio de aliados)");
  // decisao de evento (2 opcoes)
  WDip_Propose(w,PR_DECISAO,s->me,0,0,(u16)(Rnd_Count()-3));
  Frames(g,4);Shot(g,"36_decisao");Answer(g,false);
  // Gabinete: 4 abas (pausado; despacho que chegou no meio e' recusado antes)
  s->paused=true;for(int k=0;k<4&&w->prop.kind;k++)Answer(g,false);
  Press(g,KEY_X);
  if(g->screen!=SCR_LEDGER){printf("FALHA: X nao abriu o Gabinete\n");bad=1;}
  Shot(g,"30_gabinete_mercado");
  Press(g,KEY_R);Shot(g,"31_gabinete_comercio");
  Press(g,KEY_R);Shot(g,"32_gabinete_diplo");
  Press(g,KEY_R);Shot(g,"33_gabinete_politica");
  Press(g,KEY_R);Shot(g,"37_gabinete_financas");
  // juros (ADR-43): metade esquerda baixa, direita sobe
  {u8 b0=w->eco[s->me].bank;Touch(g,LG_ACT_X+FN_HALF+20,LG_ACT_Y+10);
   if(w->eco[s->me].bank!=b0+1){printf("FALHA: botao Subir juros nao subiu (%d -> %d)\n",b0,w->eco[s->me].bank);bad=1;}
   Touch(g,LG_ACT_X+20,LG_ACT_Y+10);Touch(g,LG_ACT_X+20,LG_ACT_Y+10);
   if(w->eco[s->me].bank!=(b0?b0-1:0)){printf("FALHA: botao Baixar juros nao baixou\n");bad=1;}
   printf("  juros: %d%% -> %d%% a.a.\n",b0,w->eco[s->me].bank);}
  Shot(g,"37b_financas_juros");
  // Sociedade (ADR-42): 3 estratos + burguesia
  {int x,y,tw;Lg_TabRect(LT_SOCIEDADE,&x,&y,&tw);Touch(g,x+tw/2,y+8);}
  if(s->ledTab!=LT_SOCIEDADE){printf("FALHA: toque na aba Sociedade nao abriu\n");bad=1;}
  {const WSoc *so=&w->soc[s->me];
   printf("  sociedade: pobres %d medios %d ricos %d por mil, satisf %d/%d/%d, burguesia %d%%\n",so->share[0],so->share[1],so->share[2],so->sat[0],so->sat[1],so->sat[2],so->bourg);
   if(so->share[0]+so->share[1]+so->share[2]!=1000){printf("FALHA: estratos nao somam 1000\n");bad=1;}}
  Shot(g,"46_sociedade");
  // Leis (ADR-40): grade de assuntos -> Educacao -> outra lei -> Propor = debate -> aprovada
  {int x,y,tw;Lg_TabRect(LT_LEIS,&x,&y,&tw);Touch(g,x+tw/2,y+8);}
  if(s->ledTab!=LT_LEIS){printf("FALHA: toque na aba Leis nao abriu\n");bad=1;}
  Shot(g,"42_leis_grade");
  {WEco *e=&w->eco[s->me];u8 cat=LW_EDUCACAO,before=e->law[cat];
   Touch(g,LW_GX+(cat%3)*LW_GW+20,LW_GY+(cat/3)*LW_GH+10);
   if(s->lawCat!=cat){printf("FALHA: toque no assunto nao abriu as leis dele\n");bad=1;}
   u8 to=(u8)(before>0?before-1:before+1);
   Touch(g,128,LW_LY+to*LW_LH+6);
   Shot(g,"43_leis_educacao");
   w->c[s->me].prestige+=50;
   Touch(g,4+LW_PW/2,LW_BY+10);
   if(e->lawCat!=cat||e->lawTo!=to){printf("FALHA: Propor nao abriu o debate (motivo %d)\n",Law_Why(w,s->me,cat,to));bad=1;}
   else{
    int dd=e->lawDays;
    Press(g,KEY_B);Press(g,KEY_B);                       // volta a' grade e ao mapa
    for(int i=0;i<4000&&e->lawCat<LW_COUNT;i++){if(w->prop.kind)Answer(g,false);s->paused=false;Frames(g,1);}
    s->paused=true;
    printf("  lei: %s -> %s em %d dias (%s); aprovacao ideologica %+d, inflacao %d por mil\n",Str(Law_Str(cat,before)),Str(Law_Str(cat,to)),dd,
     e->law[cat]==to?"aprovada":"NAO aprovada",Law_Approval(w,s->me),e->infl);
    if(e->law[cat]!=to){printf("FALHA: debate terminou sem aprovar a lei\n");bad=1;}
    Press(g,KEY_X);
   }
  }
  // Graficos (ADR-41): 8 series, troca por toque
  {int x,y,tw;Lg_TabRect(LT_GRAF,&x,&y,&tw);Touch(g,x+tw/2,y+8);}
  if(s->ledTab!=LT_GRAF){printf("FALHA: toque na aba Graficos nao abriu\n");bad=1;}
  Shot(g,"44_graficos_pib");
  Touch(g,GR_BX+1*GR_BW+10,GR_BY+10);
  if(s->statSel!=SR_POP){printf("FALHA: toque na serie nao trocou o grafico\n");bad=1;}
  Shot(g,"45_graficos_pop");
  printf("  graficos: %d pontos, passo %d dias, comparado com %s\n",w->st.n,w->st.step,w->st.top?NATIONS[w->c[w->st.top].nat].tag:"-");
  if(w->st.n<2){printf("FALHA: series vazias\n");bad=1;}
  printf("  financas: receita %d despesa %d divida %d alf %d%% pop %uk\n",(int)WEco_Revenue(w,s->me),(int)WEco_Expense(w,s->me),(int)w->eco[s->me].debt,w->c[s->me].lit/10,w->c[s->me].popK);
  Press(g,KEY_B);
  if(g->screen!=SCR_WORLD){printf("FALHA: B do Gabinete nao voltou ao mapa\n");bad=1;}
  // ranking: bolinha no canto do painel
  Touch(g,12,12);
  if(g->screen!=SCR_RANK){printf("FALHA: a bolinha nao abriu o ranking\n");bad=1;}
  Shot(g,"38_ranking");
  {static u8 o[W_MAXC+1];int n=WRank_Sort(w,RK_GERAL,o);printf("  ranking: %d paises; top 5:",n);
   for(int k=0;k<5&&k<n;k++)printf(" %s(%d)",NATIONS[w->c[o[k]].nat].tag,(int)WRank_Score(w,o[k],RK_GERAL));
   printf(" | voce %d\n",WRank_Pos(w,s->me,RK_GERAL)+1);}
  Touch(g,RK_DN_X+10,RK_BTN_Y+10);Press(g,KEY_R);Shot(g,"39_ranking_economia");
  Touch(g,RK_ME_X+10,RK_BTN_Y+10);Shot(g,"39b_ranking_minha");
  Press(g,KEY_B);
  if(g->screen!=SCR_WORLD){printf("FALHA: B do ranking nao voltou ao mapa\n");bad=1;}
 }
 Press(g,KEY_START);                 Shot(g,"18_pausa");
 Press(g,KEY_DOWN);Press(g,KEY_DOWN);Press(g,KEY_A);Shot(g,"19_opcoes");
 Press(g,KEY_B);Press(g,KEY_B);
 if(g->screen!=SCR_WORLD){printf("FALHA: B da pausa nao voltou ao mundo\n");bad=1;}
 // 10 anos na velocidade 5 (1 dia/frame)
 for(int i=0;i<3;i++)Press(g,KEY_SELECT|0),host_down=0;
 WG_Ptr()->speed=5;WG_Ptr()->paused=false;
 Press(g,KEY_L);
 t0=clock();u32 d0=WG_World()->day;
 int props=0;
 for(int i=0;i<3650;i++){
  // despachos e decisoes da IA pausam o jogo: responde alternando A/B
  if(WG_World()->prop.kind){if(!props)Shot(g,"29_despacho_ia");Answer(g,!(props&1));props++;WG_Ptr()->paused=false;}
  Frames(g,1);
 }
 printf("  %d despachos/decisoes respondidos\n",props);
 u32 dd=WG_World()->day-d0;
 printf("  %u dias em %.0f ms (PC, ASan, com render)\n",dd,(double)(clock()-t0)*1000.0/CLOCKS_PER_SEC);
 Shot(g,"20_mundo_10anos");
 WG_Ptr()->mapMode=WMAP_COM;g->dirty=true;Frames(g,30);Shot(g,"20b_rotas_10anos");WG_Ptr()->mapMode=WMAP_POL;

 printf("[classico 32x24]\n");
 Press(g,KEY_START);Press(g,KEY_UP);Press(g,KEY_A);Press(g,KEY_A);
 if(g->screen!=SCR_MAIN){printf("FALHA: Sair para o Menu nao voltou (%d)\n",g->screen);bad=1;}
 Game_New(g,DIFF_NORMAL,DEFAULT_SEED);  // entra no jogo: bake total + sim
 Frames(g,200);
 Shot(g,"21_classico");
 printf("  classico: day=%u\n",g->day);

 if(!bad)printf("OK: nenhum pixel transparente, sem hang\n");
 return bad;
}
