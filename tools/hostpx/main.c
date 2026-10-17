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
  for(int z=1;z<5;z++){
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
 if(!TapAct(g,DM_GUERRA)&&!TapAct(g,DM_ATACAR)){printf("FALHA: folha sem guerra/ataque sobre a Franca\n");bad=1;}
 if(WG_Ptr()->dipOpen){printf("FALHA: a folha nao fechou depois da acao\n");bad=1;}
 Press(g,KEY_SELECT);
 // resposta da IA: quantos dias ate' o alvo abrir frente de volta (contra-ataque)
 {World*w=WG_World();u8 me=WG_Ptr()->me,fra=World_CountryOfNat(w,Nat_Find("FRA"));u32 d0=w->day;int resp=-1;
  for(int i=0;i<240;i++){Frames(g,1);u32 c=w->c[me].cells;if(c>c1)c1=c;
   if(resp<0&&fra&&w->c[fra].atkOn&&w->c[fra].atkTo==me)resp=(int)(w->day-d0);}
  if(resp>=0)printf("  IA respondeu ao ataque em %d dia(s)\n",resp);
  else printf("  IA nao contra-atacou em %u dias (reserva baixa ou sem fronteira)\n",w->day-d0);}
 Shot(g,"15_guerra");
 WG_Ptr()->bannerT=0;Frames(g,120);Shot(g,"15b_frente");
 WG_Ptr()->bannerT=0;Press(g,KEY_R);Shot(g,"15c_z4_cidades");Press(g,KEY_L);

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
 printf("[diplomacia + gabinete]\n");
 {
  World *w=WG_World();WGame *s=WG_Ptr();
  u8 fra=World_CountryOfNat(w,Nat_Find("FRA"));
  if(!fra||!w->c[fra].alive)for(u8 i=1;i<w->nC&&!fra;i++)if(w->c[i].alive&&i!=s->me)fra=i;
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
  u8 tax0=w->eco[s->me].tax;Touch(g,LG_ACT_X+20,LG_ACT_Y+10);
  if(w->eco[s->me].tax==tax0){printf("FALHA: toque na acao nao mudou o imposto\n");bad=1;}
  // Financas: balanco fiscal + 4 politicas por toque
  Press(g,KEY_R);Shot(g,"37_gabinete_financas");
  u8 edu0=w->eco[s->me].edu;Touch(g,LG_POL_X0+20,LG_POL_Y+LG_POL_H+4+10);
  if(w->eco[s->me].edu==edu0){printf("FALHA: toque em Educacao nao mudou a politica\n");bad=1;}
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
