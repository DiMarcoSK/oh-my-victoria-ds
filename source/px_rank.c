#include "wgame.h"
#include "wrank.h"
#include "wdiplo.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "flag.h"
// px_rank.c — ranking das potencias (SCR_RANK, estilo Victoria 3). Cima: a
// lista (posicao, faixa, bandeira, nome, valor do criterio); faixas separadas
// por um rotulo (Grandes potencias, Potencias maiores...). Baixo: criterio,
// paginas, "minha nacao", ficha da sua posicao e voltar (geometria RK_* em
// wgame.h; o toque em wgame.c usa a mesma). Render sob demanda.

static void Cat(char *dst,int max,const char *s){int n=0;while(dst[n])n++;while(*s&&n<max-1)dst[n++]=*s++;dst[n]=0;}
static void Num(char *dst,int max,s32 v){char t[16];Px_Itoa((int)v,t);Cat(dst,max,t);}
static void Kfmt(char *o,int max,s32 v){
 o[0]=0;if(v<0)v=0;
 if(v<10000)Num(o,max,v);
 else if(v<1000000){Num(o,max,v/1000);Cat(o,max,"k");}
 else{Num(o,max,v/1000000);Cat(o,max,",");char t[2]={(char)('0'+(v%1000000)/100000),0};Cat(o,max,t);Cat(o,max,"M");}
}
static void PopFmt(char *o,int max,s32 k){
 o[0]=0;
 if(k<1000){Num(o,max,k);Cat(o,max,"k");}
 else if(k<1000000){Num(o,max,k/1000);Cat(o,max,",");char t[2]={(char)('0'+(k%1000)/100),0};Cat(o,max,t);Cat(o,max,"M");}
 else{Num(o,max,k/1000000);Cat(o,max,",");char t[2]={(char)('0'+(k%1000000)/100000),0};Cat(o,max,t);Cat(o,max,"bi");}
}
static const u16 RT_STR[RT_COUNT]={STR_RT_GRANDE,STR_RT_MAIOR,STR_RT_MENOR,STR_RT_INSIG};
static const u16 RK_STR[RK_COUNT]={STR_RK_GERAL,STR_RK_ECON,STR_RK_MIL,STR_RK_POP};
// cor da faixa: ouro, prata, bronze, cinza
static u16 TierCol(u8 t){
 switch(t){case RT_GRANDE:return C_GOLD;case RT_MAIOR:return PXC(22,23,25);case RT_MENOR:return PXC(21,13,7);default:return C_TEXT_DIM;}
}
static void ValueStr(const World *w,u8 ci,u8 mode,char *o){
 s32 v=WRank_Score(w,ci,mode);
 if(mode==RK_POP)PopFmt(o,24,v);else Kfmt(o,24,v);
}
static u8 ORD[W_MAXC+1];
void PxR_Top(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();
 Ui_Backdrop(d);
 Px_Rect(d,0,0,256,20,C_NAVY);Px_Rect(d,0,19,256,1,C_GOLD);
 Ui_IconId(d,5,4,ICO_RANK,false);
 char t[64]={0};Cat(t,64,Str(STR_RANKING));Cat(t,64," · ");Cat(t,64,Str(RK_STR[s->rankTab%RK_COUNT]));
 Px_Text2(d,22,4,t,&F_SERIF12,C_TEXT);
 int n=WRank_Sort(w,s->rankTab,ORD);
 Ui_Parch(d,2,22,252,168);
 int y=26,last=-1;
 for(int i=s->rankTop;i<n&&y<180;i++){
  u8 ci=ORD[i];u8 tier=WRank_Tier(i);
  if(tier!=last){
   // rotulo da faixa (Victoria 3): linha + nome em versalete
   if(y>170)break;
   Px_Rect(d,8,y+4,240,1,TierCol(tier));
   const char *tn=Str(RT_STR[tier]);int tw=Px_Width2(tn,&F_SERIF8);
   Px_Rect(d,124-tw/2,y,tw+8,9,C_PARCH);
   Px_Text2(d,128-tw/2,y,tn,&F_SERIF8,tier==RT_INSIG?C_GOLD_DIM:C_INK);
   y+=11;last=tier;
   if(y>170)break;
  }
  bool me=ci==s->me;
  if(me){Px_Rect(d,5,y-2,246,12,C_CRIMSON);Px_Border(d,5,y-2,246,12,C_GOLD);}
  char p[8];Px_Itoa(i+1,p);
  Px_Text2(d,24-Px_Width2(p,&F_SERIF8),y,p,&F_SERIF8,me?C_TEXT:C_INK);
  Px_Rect(d,28,y,3,7,TierCol(tier));
  FlagDef f;WDip_Flag(w,ci,&f);
  Px_Rect(d,34,y-1,16,10,C_INK);Flag_Draw(d,256,35,y,14,8,&f);
  // nome cortado p/ caber antes do valor
  char nm[40]={0};const char *src=NATIONS[w->c[ci].nat].name;
  int k=0;while(src[k]&&k<38){nm[k]=src[k];k++;}nm[k]=0;
  while(k>0&&Px_Width2(nm,&F_SERIF8)>150){k--;while(k>0&&(nm[k]&0xC0)==0x80)k--;nm[k]=0;}
  Px_Text2(d,54,y,nm,&F_SERIF8,me?C_TEXT:C_INK);
  char v[24];ValueStr(w,ci,s->rankTab,v);
  Px_Text2(d,248-Px_Width2(v,&F_SERIF8),y,v,&F_SERIF8,me?C_TEXT:C_INK);
  y+=12;
 }
 // barra de rolagem
 if(n>0){int h=150*12/(n<12?12:n);if(h<6)h=6;int py=26+(150-h)*s->rankTop/(n>1?n:1);
  Px_Rect(d,251,26,2,150,C_PARCH_SH);Px_Rect(d,251,py,2,h,C_GOLD_DIM);}
}
void PxR_Bot(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();
 Ui_Backdrop(d);
 // criterios (abas)
 for(int i=0;i<RK_COUNT;i++){
  int x=i*64+2;bool sel=i==s->rankTab;
  Px_Rect(d,x,RK_TAB_Y,60,RK_TAB_H,sel?C_CRIMSON:C_NAVY);
  Px_Border(d,x,RK_TAB_Y,60,RK_TAB_H,sel?C_GOLD:C_GOLD_DIM);
  if(sel)for(int k=1;k<59;k++)Px_PSet(d,x+k,RK_TAB_Y+1,C_CRIMSON_HI);
  Ui_TextC(d,x+30,RK_TAB_Y+9,Str(RK_STR[i]),0,sel?C_TEXT:C_TEXT_DIM);
 }
 // sua ficha: posicao, faixa, pontos e quanto falta para subir
 Ui_Navy(d,4,34,248,84);
 u8 me=s->me;int pos=WRank_Pos(w,me,s->rankTab);
 FlagDef f;WDip_Flag(w,me,&f);
 Px_Rect(d,10,40,38,26,C_GOLD);Flag_Draw(d,256,11,41,36,24,&f);
 char t[96]={0};
 if(pos>=0){
  Num(t,96,pos+1);Cat(t,96,"º · ");Cat(t,96,Str(RT_STR[WRank_Tier(pos)]));
  Px_Text2(d,56,41,t,&F_SERIF12,TierCol(WRank_Tier(pos)));
  t[0]=0;Cat(t,96,Str(STR_RK_VALOR));char v[24];ValueStr(w,me,s->rankTab,v);Cat(t,96,v);
  Px_Text2(d,56,57,t,&F_SERIF8,C_TEXT);
  if(pos>0){
   int n=WRank_Sort(w,s->rankTab,ORD);(void)n;
   u8 up=ORD[pos-1];s32 gap=WRank_Score(w,up,s->rankTab)-WRank_Score(w,me,s->rankTab)+1;
   t[0]=0;Cat(t,96,Str(STR_RK_FALTAM));
   if(s->rankTab==RK_POP){char q[24];PopFmt(q,24,gap);Cat(t,96,q);}else{char q[24];Kfmt(q,24,gap);Cat(t,96,q);}
   Cat(t,96,Str(STR_RK_PARA));Cat(t,96,NATIONS[w->c[up].nat].name);
   Px_WrapN(d,12,74,232,t,&F_SERIF8,C_TEXT_DIM,2,false);
  }else Px_WrapN(d,12,74,232,Str(STR_RK_TOPO),&F_SERIF8,C_GOLD,2,false);
 }
 Px_WrapN(d,12,98,232,Str(STR_RK_DICA),&F_SERIF8,C_TEXT_DIM,2,false);
 // paginas + minha nacao
 static const u8 BX[3]={RK_UP_X,RK_DN_X,RK_ME_X},BW[3]={RK_BTN_W,RK_BTN_W,RK_ME_W};
 for(int i=0;i<3;i++){
  Px_Rect(d,BX[i],RK_BTN_Y,BW[i],RK_BTN_H,C_NAVY);Px_Border(d,BX[i],RK_BTN_Y,BW[i],RK_BTN_H,C_GOLD);
 }
 int cx=RK_UP_X+RK_BTN_W/2,cy=RK_BTN_Y+RK_BTN_H/2;
 for(int k=0;k<6;k++)for(int j=-k;j<=k;j++){Px_PSet(d,cx+j,cy-3+k,C_TEXT);}
 cx=RK_DN_X+RK_BTN_W/2;
 for(int k=0;k<6;k++)for(int j=-k;j<=k;j++){Px_PSet(d,cx+j,cy+3-k,C_TEXT);}
 Ui_TextC(d,RK_ME_X+RK_ME_W/2,RK_BTN_Y+9,Str(STR_RK_MINHA),0,C_TEXT);
 Px_Rect(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H,C_NAVY);
 Px_Border(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H,C_GOLD_DIM);
 Ui_TextC(d,128,LG_BACK_Y+9,Str(STR_VOLTAR),0,C_TEXT);
 Px_Text2(d,LG_ACT_X+LG_ACT_W-16,LG_BACK_Y+9,"B",&F_SERIF8,C_GOLD_DIM);
}
