#include "prof.h"
#include "px.h"
// prof.c — medidor de quadro (ver prof.h, SDD ADR-36).
// Media movel exponencial (1/16 por quadro) em ticks<<4: estavel p/ ler a olho
// e sem divisao no caminho quente (so' no desenho da caixa).
static u32 EMA[PF_COUNT+1],ACC[PF_COUNT+1],drops;
#ifdef OMV_PROF
#if defined(OMV_ARMBENCH)
// tools/armbench: o emulador escreve aqui o relogio estimado (instrucoes/2 =
// ticks de 33,5 MHz se cada instrucao custasse 1 ciclo a 67 MHz)
volatile u32 bench_ticks;
__attribute__((noinline)) u32 Prof_Now(void){__asm__ volatile("");return bench_ticks;}
#elif defined(ARM9)
u32 Prof_Now(void){return cpuGetTiming();}
#else
#include <time.h>
// host: 1 tick = 1/33514 ms, como no DS (so' a escala; o PC e' ~50x mais rapido)
u32 Prof_Now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
 return (u32)((u64)t.tv_sec*33514000u+(u64)t.tv_nsec*33514u/1000000u);}
#endif
void Prof_Add(int id,u32 dt){ACC[id]+=dt;}
#endif
void Prof_Init(void){
#if defined(OMV_PROF)&&defined(ARM9)
 cpuStartTiming(0);
#endif
}
#ifdef OMV_PROF
static u32 frameT0;
#endif
void Prof_Busy(void){
#ifdef OMV_PROF
 ACC[PF_COUNT]=Prof_Now()-frameT0;
#endif
}
void Prof_EndFrame(int vb){
#ifdef OMV_PROF
 frameT0=Prof_Now();
#endif
 if(vb>1)drops++;
 for(int i=0;i<=PF_COUNT;i++){EMA[i]=EMA[i]-(EMA[i]>>4)+ACC[i];ACC[i]=0;}
}
const u32 *Prof_Us(void){
 static u32 us[PF_COUNT+1];
 // EMA guarda 16x a media em ticks; 1 tick = 1000/33514 us
 for(int i=0;i<=PF_COUNT;i++)us[i]=(u32)(((u64)EMA[i]*1000u/33514u)>>4);
 return us;
}
u32 Prof_Drops(void){return drops;}
void Prof_Draw(u16 *d){
#if defined(OMV_PROF)&&!defined(OMV_ARMBENCH)   // no armbench a caixa so' distorceria a conta
 static const char *const NM[PF_COUNT+1]={"IN","SIM","MAPU","BLIT","OVL","CITY","LBL","DECO","AHD","TOP","BOT","QUAD"};
 const u32 *us=Prof_Us();
 int x0=2,y0=192-(PF_COUNT+2)*6-2;
 Px_Rect(d,x0,y0,58,(PF_COUNT+2)*6+1,Px_Col(2,3,6));
 char t[24];
 for(int i=0;i<=PF_COUNT+1;i++){
  int n=0;const char *s=i<=PF_COUNT?NM[i]:"PERD";
  while(s[n]){t[n]=s[n];n++;}
  t[n++]=' ';
  if(i<=PF_COUNT){
   // ms com 1 casa: 16,7 = um quadro inteiro do DS
   u32 v=(us[i]+50)/100;char q[12];Px_Itoa((int)(v/10),q);
   for(int k=0;q[k];k++)t[n++]=q[k];
   t[n++]='.';t[n++]=(char)('0'+v%10);
  }else{char q[12];Px_Itoa((int)drops,q);for(int k=0;q[k];k++)t[n++]=q[k];}
  t[n]=0;
  u16 c=Px_Col(31,31,31);
  if(i==PF_COUNT)c=us[i]>16700?Px_Col(31,8,6):Px_Col(10,31,12);
  Px_TextSc(d,x0+2,y0+1+i*6,t,c,1);
 }
#else
 (void)d;
#endif
}
// totais acumulados (benchmark do host, tools/hostpx/prof.c)
#ifdef OMV_PROF
static u64 TOT[PF_COUNT+1];static u32 totN;
void Prof_Reset(void){for(int i=0;i<=PF_COUNT;i++)TOT[i]=0;totN=0;}
const u64 *Prof_Total(u32 *frames){*frames=totN;return TOT;}
void Prof_Tally(void){for(int i=0;i<=PF_COUNT;i++)TOT[i]+=ACC[i];totN++;}
#endif
