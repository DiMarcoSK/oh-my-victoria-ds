// Benchmark de render no host (sem libnds): mede o caminho do mapa-mundi
// quadro a quadro por secao (prof.h) em cenarios fixos — pan em cada zoom,
// pan diagonal, parado, com frente de guerra. Mesma seed = mesmos numeros
// relativos; o PC e' ~50x mais rapido que o ARM9, entao compare ANTES x
// DEPOIS no mesmo PC, nunca o valor absoluto com o DS (use make PROF=1 la').
// Uso: tools/hostpx/prof.sh
#include <stdio.h>
#include <string.h>
#include "nds.h"
#include "game.h"
#include "px.h"
#include "settings.h"
#include "menu.h"
#include "wgame.h"
#include "world.h"
#include "nations.h"
#include "prof.h"
#include "wdiplo.h"
void Prof_Reset(void);
const u64 *Prof_Total(u32 *frames);
void Prof_Tally(void);
static void Frames(GameState*g,int n){
 static u32 f=0;
 for(int k=0;k<n;k++,f++){
  u32 t=Prof_Now();
  PROF(PF_IN,In_Handle(g));
  PROF(PF_SIM,Game_Frame(g));
  Px_Tick(g);
  if(g->dirty){PROF(PF_TOP,Px_Top(g));PROF(PF_BOT,Px_Bot(g));g->dirty=false;g->dirtyBot=false;Px_TakeTop();}
  else if(g->dirtyBot){PROF(PF_BOT,Px_Bot(g));g->dirtyBot=false;if(Px_TakeTop())PROF(PF_TOP,Px_Top(g));}
  else if(Px_TakeTop())PROF(PF_TOP,Px_Top(g));
  else if((f%15)==0){PROF(PF_TOP,Px_Top(g));if(!(g->screen==SCR_WORLD||g->screen==SCR_CITY||g->screen==SCR_LEDGER||g->screen==SCR_RANK))PROF(PF_BOT,Px_Bot(g));}   // = main.c (BotStill)
  WG_Idle(g);Px_VSync();(void)t;Prof_Busy();Prof_Tally();Prof_EndFrame(1);
  host_down=0;
 }
}
static void Press(GameState*g,u32 k){host_down=k;host_held=k;Frames(g,1);host_held=0;Frames(g,2);}
static const char *const NM[PF_COUNT+1]={"in","sim","mapu","blit","ovl","city","lbl","deco","ahd","top","bot","QUADRO"};
static void Report(const char *name){
 u32 n;const u64 *t=Prof_Total(&n);
 printf("%-16s",name);
 for(int i=0;i<=PF_COUNT;i++){if(i==PF_TOP||i==PF_IN)continue;printf(" %s=%6.1f",NM[i],n?(double)t[i]/n*1000.0/33514.0:0.0);}
 printf("  us/quadro\n");
}
static void NoProp(void){World*w=WG_World();for(int i=0;i<8&&w->prop.kind;i++)WDip_Answer(w,false);}
static void Zoom(GameState*g,int z){for(int i=0;i<10&&WG_Ptr()->zoom!=z;i++){NoProp();Press(g,WG_Ptr()->zoom>z?KEY_L:KEY_R);}}
#ifdef OMV_ARMBENCH
#define NF(n) ((n)/2)     // no ARM9 emulado: cenarios mais curtos (o emulador instrumentado e' lento)
#else
#define NF(n) (n)
#endif
static void Pan(GameState*g,const char *name,u32 keys,int n){
 n=NF(n);
 Frames(g,NF(NF(40)));NoProp();Prof_Reset();
 for(int i=0;i<n;i++){NoProp();host_held=keys;if(!i)host_down=keys;Frames(g,1);}
 host_held=0;Report(name);Frames(g,NF(NF(40)));
}
#ifdef OMV_ARMBENCH
// no ARM emulado o Px_Copy16 usa o asm ldm/stm (o host usa C): confere contra
// a copia ingenua com todos os alinhamentos e tamanhos de 0 a 70
static int CopyCheck(void){
 static u16 A[160] __attribute__((aligned(32))),B[160] __attribute__((aligned(32))),C[160];
 int bad=0;
 for(int so=0;so<4;so++)for(int dof=0;dof<4;dof++)for(int n=0;n<=70;n++){
  for(int i=0;i<160;i++){A[i]=(u16)(i*2654435761u>>7);B[i]=C[i]=0xBEEF;}
  Px_Copy16(B+dof,A+so,n);for(int i=0;i<n;i++)C[dof+i]=A[so+i];
  for(int i=0;i<160;i++)if(B[i]!=C[i])bad++;
 }
 return bad;
}
#endif
int main(void){
 Settings_Init();Px_Init();Prof_Init();
#ifdef OMV_ARMBENCH
 printf("Px_Copy16 (asm ldm/stm): %s\n",CopyCheck()?"FALHA":"ok");
#endif
 GameState *g=Game_Ptr();Game_Init(g);g->seed=DEFAULT_SEED;g->dirty=true;
 Frames(g,40);
 Press(g,KEY_A);Press(g,KEY_DOWN);Press(g,KEY_A);Press(g,KEY_A);
 Menu_Ptr()->nat=(s16)Nat_Find("GBR");
 Press(g,KEY_A);
 if(g->screen!=SCR_WORLD){printf("nao entrou no mundo\n");return 1;}
 WGame *s=WG_Ptr();World *w=WG_World();
 // guerra aberta: frente piscando (o overlay mais caro) em todos os cenarios
 u8 fra=World_CountryOfNat(w,Nat_Find("FRA"));
 World_DeclareWar(w,s->me,fra);
 // e uma frente por terra: a Franca ataca um vizinho com toda a reserva
 for(u8 o=1;o<w->nC;o++)if(o!=fra&&o!=s->me&&w->c[o].alive&&World_Borders(w,fra,o)){World_DeclareWar(w,fra,o);World_Attack(w,fra,o,100);break;}
 s->paused=false;s->speed=2;Frames(g,300);
 printf("frentes: GBR atk=%d FRA atk=%d (dia %u)\n",w->c[s->me].atkOn,w->c[fra].atkOn,w->day);
 WG_Goto(w->c[fra].capX,w->c[fra].capY);
 for(int z=0;z<5;z++){
   Zoom(g,z);
  char nm[32];snprintf(nm,sizeof nm,"pan->  z%d",z);Pan(g,nm,KEY_RIGHT,120);
  snprintf(nm,sizeof nm,"pan diag z%d",z);Pan(g,nm,KEY_LEFT|KEY_DOWN,90);
 }
 Zoom(g,3);
 Pan(g,"parado z3",0,240);
 s->speed=5;Pan(g,"parado z3 vel5",0,240);
 s->paused=true;Pan(g,"pausado z3",0,240);
 return 0;
}
