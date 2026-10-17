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
#include "ui.h"

static const char *dir=0;   // sem SHOTS nao grava screenshot
static int bad=0;

static long Trans(const u16*b){long n=0;for(int i=0;i<256*192;i++)if(!(b[i]&0x8000))n++;return n;}
static void Frames(GameState*g,int n){
 static u32 f=0;
 for(int k=0;k<n;k++,f++){
  In_Handle(g);
  Game_Frame(g);
  Px_Tick(g);                       // bake do mapa classico: onde morava o hang
  if(g->dirty){Px_Top(g);Px_Bot(g);g->dirty=false;g->dirtyBot=false;}
  else if(g->dirtyBot){Px_Bot(g);g->dirtyBot=false;}
  else if((f%15)==0){Px_Top(g);Px_Bot(g);}
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
 Hold(g,KEY_DOWN,7);Hold(g,KEY_RIGHT,7);
 Shot(g,"13_cursor_franca");
 Press(g,KEY_A);                     Shot(g,"14_confirma_guerra");
 u32 c0=WG_World()->c[WG_Ptr()->me].cells,c1=c0;
 Press(g,KEY_X);Press(g,KEY_X);            // recruta com o ouro inicial
 WG_Ptr()->pct=100;Press(g,KEY_A);Press(g,KEY_SELECT);
 for(int i=0;i<240;i++){Frames(g,1);u32 c=WG_World()->c[WG_Ptr()->me].cells;if(c>c1)c1=c;}
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
   s->curX=(s16)w->city[cap].x;s->curY=(s16)w->city[cap].y;Hold(g,KEY_LEFT,1);Hold(g,KEY_RIGHT,1);
   w->c[s->me].gold+=5000;
   Press(g,KEY_A);
   if(g->screen!=SCR_CITY){printf("FALHA: A na capital nao abriu a cidade\n");bad=1;}
   Shot(g,"22_cidade");
   // escolhe um slot vazio e constroi
   for(int k=0;k<8;k++){if(!w->city[cap].bld[s->citySlot]&&s->citySlot<City_Slots(w->city[cap].tier))break;Press(g,KEY_RIGHT);if(k==3)Press(g,KEY_DOWN);}
   Press(g,KEY_A);Shot(g,"23_escolher_obra");
   for(int k=0;k<11&&!City_CanBuild(w,cap,s->pickK);k++)Press(g,KEY_RIGHT);
   u8 want=s->pickK;Press(g,KEY_A);
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
   s->curX=(s16)fx;s->curY=(s16)fy;Hold(g,KEY_LEFT,1);Hold(g,KEY_RIGHT,1);
   if(!City_CanFound(w,s->me,s->curX,s->curY)){s->curX=(s16)fx;s->curY=(s16)fy;}
   int id=Ui_Hit(UI_WORLD,130,160);
   host_tx=130;host_ty=160;host_held=KEY_TOUCH;Frames(g,1);host_held=0;Frames(g,1);  // 1o toque: preco
   Shot(g,"26_fundar_preco");
   host_held=KEY_TOUCH;Frames(g,1);host_held=0;Frames(g,1);                            // 2o: funda
   printf("  fundar (botao %d): %u -> %u cidades\n",id,n0,w->nCity);
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
 Press(g,KEY_Y);                     Shot(g,"16_diplomatico");
 Press(g,KEY_Y);                     Shot(g,"17_terreno");
 Press(g,KEY_Y);
 Press(g,KEY_START);                 Shot(g,"18_pausa");
 Press(g,KEY_DOWN);Press(g,KEY_DOWN);Press(g,KEY_A);Shot(g,"19_opcoes");
 Press(g,KEY_B);Press(g,KEY_B);
 if(g->screen!=SCR_WORLD){printf("FALHA: B da pausa nao voltou ao mundo\n");bad=1;}
 // 10 anos na velocidade 5 (1 dia/frame)
 for(int i=0;i<3;i++)Press(g,KEY_SELECT|0),host_down=0;
 WG_Ptr()->speed=5;WG_Ptr()->paused=false;
 Press(g,KEY_L);
 t0=clock();u32 d0=WG_World()->day;
 for(int i=0;i<3650&&!WG_Ptr()->paused;i++)Frames(g,1);
 u32 dd=WG_World()->day-d0;
 printf("  %u dias em %.0f ms (PC, ASan, com render)\n",dd,(double)(clock()-t0)*1000.0/CLOCKS_PER_SEC);
 Shot(g,"20_mundo_10anos");

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
