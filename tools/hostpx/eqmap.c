// eqmap.c — prova de equivalencia do pintor do mapa (tools/hostpx/eqmap.sh):
// px_mapr.c de uma versao do git (simbolos renomeados O_*) x o atual, mesmo
// mundo/camera. Origem alinhada em 8 px p/ o pontilhado (preso ao anel desde
// o ADR-36) coincidir. Compara a repintura completa (5 modos x 5 zooms x 3
// regioes) e o overlay (frente + contorno), este tambem com origem
// desalinhada (costura do anel). Refatorou o pixel por desempenho? Prove
// que nada mudou: sh tools/hostpx/eqmap.sh <ref>   (padrao: HEAD).
#include <stdio.h>
#include "wgame.h"
#include "world.h"
#include "nations.h"
void O_Update(World*,const WGame*);void O_Blit(u16*);void O_Inv(void);void O_Overlay(u16*,const World*,const WGame*,int);
void PxC_SceneReset(void){}
void Px_Copy16(u16*d,const u16*s,int n){while(n--)*d++=*s++;}
u32 Px_Ticks(void){return 0;}
static World W;static WGame S;static u16 A[256*192],B[256*192],P[256*192];
int main(void){
 World_GenMundi(&W,42,WH_LIGADA,1,Nat_Find("GBR"));
 S.me=World_CountryOfNat(&W,Nat_Find("GBR"));
 int bad=0,cases=0;long drawn=0;
 const char *spot[3]={"FRA","CHN","BRA"};
 for(int m=0;m<WMAP_COUNT;m++)for(int z=0;z<5;z++)for(int k=0;k<3;k++){
  u8 c=World_CountryOfNat(&W,Nat_Find(spot[k]));if(!c)continue;
  S.mapMode=(u8)m;S.zoom=(u8)z;S.subX=S.subY=0;
  int p=z?1<<(z-1):1;
  int cx=W.c[c].capX-(z?128/p:256),cy=W.c[c].capY-(z?86/p:172);
  if(z){int q=8/p>0?8/p:1;cx=cx/q*q;cy=cy/q*q;}else{cx&=~15;cy&=~15;}
  S.camX=(s16)cx;S.camY=(s16)cy;
  O_Inv();O_Update(&W,&S);O_Blit(A);
  PxM_Invalidate();PxM_Update(&W,&S);PxM_Blit(B);
  long d=0;for(int i=20*256;i<192*256;i++)if(A[i]!=B[i])d++;
  cases++;
  if(d){bad++;printf("modo %d z%d %s: %ld px diferentes\n",m,z,spot[k],d);}
  // overlay: contorno do selecionado + frente de guerra (c ataca um vizinho)
  u8 nb=0;for(u8 o=1;o<W.nC&&!nb;o++)if(o!=c&&W.c[o].alive&&World_Borders(&W,c,o))nb=o;
  for(int ph=0;ph<2;ph++){
   S.sel=c;S.selLand=0;W.c[c].atkOn=1;W.c[c].atkTo=nb;
   O_Blit(A);O_Overlay(A,&W,&S,ph);O_Blit(P);PxM_Blit(B);PxM_Overlay(B,&W,&S,ph);
   W.c[c].atkOn=0;
   long e=0;for(int y=20;y<192;y++)for(int x=0;x<255;x++){int i=y*256+x;if(A[i]!=B[i])e++;if(A[i]!=P[i])drawn++;}
   cases++;
   if(e){bad++;printf("overlay modo %d z%d %s fase %d: %ld px diferentes\n",m,z,spot[k],ph,e);}
  }
 }
 // overlay com origem DESALINHADA (costura do anel no meio de uma palavra):
 // so' o overlay, sobre buffer zerado (nao depende do pontilhado)
 for(int z=1;z<5;z++)for(int k=0;k<3;k++)for(int off=1;off<8;off+=2){
  u8 c=World_CountryOfNat(&W,Nat_Find(spot[k]));if(!c)continue;
  S.mapMode=0;S.zoom=(u8)z;int p=1<<(z-1);
  S.camX=(s16)(W.c[c].capX-128/p+off);S.camY=(s16)(W.c[c].capY-86/p+off);
  S.subX=(u8)(off%p);S.subY=(u8)((off*3)%p);
  O_Inv();O_Update(&W,&S);PxM_Invalidate();PxM_Update(&W,&S);
  u8 nb=0;for(u8 o=1;o<W.nC&&!nb;o++)if(o!=c&&W.c[o].alive&&World_Borders(&W,c,o))nb=o;
  S.sel=c;W.c[c].atkOn=1;W.c[c].atkTo=nb;
  for(int ph=0;ph<2;ph++){
   for(int i=0;i<256*192;i++)A[i]=B[i]=0;
   O_Overlay(A,&W,&S,ph);PxM_Overlay(B,&W,&S,ph);
   long e=0;for(int y=20;y<192;y++)for(int x=0;x<255;x++){int i=y*256+x;if(A[i]!=B[i])e++;if(A[i])drawn++;}
   cases++;if(e){bad++;printf("overlay desalinhado z%d %s off %d fase %d: %ld px\n",z,spot[k],off,ph,e);}
  }
  W.c[c].atkOn=0;
 }
 printf("%d casos, overlay desenhou %ld px no total\n",cases,drawn);
 printf(bad?"DIVERGIU em %d casos\n":"IDENTICO (%d divergencias)\n",bad);
 return bad!=0||!drawn;
}
