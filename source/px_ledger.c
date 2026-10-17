#include "wgame.h"
#include "wecon.h"
#include "wdiplo.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "flag.h"
#include "wlaw.h"
#include "wstat.h"
#include "calendar.h"
#include "wsoc.h"
#include "city.h"
// px_ledger.c — Gabinete (SCR_LEDGER, DESIGN 4.8): 4 abas sobre a economia e
// o mundo. Cima = pagina da aba (pergaminho com tabela); baixo = abas,
// explicacao curta, 1 acao e voltar (geometria LG_* em wgame.h).
// So' le o World; acoes ficam em wgame.c. Render sob demanda.

static inline int Cl(int v){return v<0?0:(v>31?31:v);}
static inline u16 C3(int r,int g,int b){return PXC(Cl(r),Cl(g),Cl(b));}
static void Cat(char *dst,int max,const char *s){int n=0;while(dst[n])n++;while(*s&&n<max-1)dst[n++]=*s++;dst[n]=0;}
static void Num(char *dst,int max,s32 v){char t[16];Px_Itoa((int)v,t);Cat(dst,max,t);}
static void Signed(char *dst,int max,s32 v){if(v>=0)Cat(dst,max,"+");Num(dst,max,v);}
static void Kfmt(char *o,int max,s32 v){
 o[0]=0;bool neg=v<0;if(neg){Cat(o,max,"-");v=-v;}
 if(v<10000)Num(o,max,v);
 else if(v<1000000){Num(o,max,v/1000);Cat(o,max,"k");}
 else{Num(o,max,v/1000000);Cat(o,max,",");char t[2]={(char)('0'+(v%1000000)/100000),0};Cat(o,max,t);Cat(o,max,"M");}
}
static void RText(u16 *d,int xr,int y,const char *s,u16 c){Px_Text2(d,xr-Px_Width2(s,&F_SERIF8),y,s,&F_SERIF8,c);}

// ---- icones 8x8 dos bens (a = cor, b = sombra, c = brilho, d = cabo/madeira) ----
static const u8 GCOL[GD_COUNT][3][3]={
 {{26,21,6},{17,12,3},{30,27,14}},{{18,11,5},{11,6,3},{24,16,8}},{{28,28,26},{18,18,18},{31,31,31}},
 {{15,15,17},{8,8,9},{22,22,24}},{{18,8,20},{10,4,12},{26,16,28}},{{8,12,24},{4,6,14},{16,20,30}},
 {{18,19,21},{10,10,12},{26,27,28}},{{22,6,5},{12,3,3},{28,24,22}},{{5,5,7},{2,2,3},{14,14,17}},
 {{20,22,25},{11,12,14},{28,29,30}},{{26,14,8},{16,8,4},{30,22,14}},{{16,17,19},{8,8,10},{24,25,26}},{{6,20,26},{4,6,8},{16,28,30}}};
static const char *const GICO[GD_COUNT][8]={
 {"..a.a.a.",".a.aca..","..aca.a.","...aa...","...bb...","..a.ba..","...bb...","...bb..."},
 {"........",".cccccc.","ac.aaaab","aa.aaaab",".bbbbbb.","ac.aaaab","aa.aaaab",".bbbbbb."},
 {"..cc.c..",".ccccca.","cccccccc",".cccccc.","..dddd..","...dd...","...dd...","...d...."},
 {"........","...cc...","..caac..",".caaaab.","caaabaab","aabaaabb",".abbbbb.","........"},
 {"..b..b..","...bb...","..aaaa..",".acaaaa.","aacaaaab","aaaaaaab",".abbbbb.","........"},
 {"........",".cccccc.","caaaaaab","cacacacb","caaaaaab","cacacacb",".bbbbbb.","........"},
 {".cccc...","caaaab..",".bbbbd..","....d...","...d....","..d.....",".d......","d......."},
 {"......c.",".....ca.","....ca..","...ca...","d.ca....",".db.....",".dd.....","d..d...."},
 {"...c....","...a....","..aca...","..aaa...",".aacaa..",".aaaab..","..abb...","........"},
 {"........","..cccc..",".caaaab.","cbbbbbb.","..cccc..",".caaaab.","cbbbbbb.","........"},   // aco: lingotes
 {"..cccc..","...ab...","..caab..",".caaaab.",".caaaab.","..aaab..","...bb...","........"},   // bens: vaso/louca
 {"..a..a..",".aaaaaa.","aab..baa",".ab..ba.",".ab..ba.","aab..baa",".aaaaaa.","..a..a.."},   // maquinas: engrenagem
 {"......c.",".....c..","bbbbbbbb","bccccccb","bcaaaacb","bccccccb","bbbbbbbb",".d....d."}};  // eletronicos: radio
u16 PxW_GoodCol(u8 g){g%=GD_COUNT;return C3(GCOL[g][0][0],GCOL[g][0][1],GCOL[g][0][2]);}
void PxW_GoodChip(u16 *d,int x,int y,u8 g){
 g%=GD_COUNT;
 for(int r=0;r<8;r++)for(int c=0;c<8;c++){
  char ch=GICO[g][r][c];u16 col;
  switch(ch){
   case 'a':col=C3(GCOL[g][0][0],GCOL[g][0][1],GCOL[g][0][2]);break;
   case 'b':col=C3(GCOL[g][1][0],GCOL[g][1][1],GCOL[g][1][2]);break;
   case 'c':col=C3(GCOL[g][2][0],GCOL[g][2][1],GCOL[g][2][2]);break;
   case 'd':col=C3(16,10,5);break;
   default:continue;
  }
  Px_PSet(d,x+c,y+r,col);
 }
}

static const u16 TABS[LT_COUNT]={STR_TAB_MERCADO,STR_TAB_COMERCIO,STR_TAB_DIPLO,STR_TAB_POLITICA,STR_TAB_FINANCAS,STR_TAB_LEIS,STR_TAB_GRAF,STR_TAB_SOCIEDADE};
static void SmallFlag(u16 *d,const World *w,u8 ci,int x,int y,int fw,int fh){
 if(!ci||!w->c[ci].alive)return;
 FlagDef f;WDip_Flag(w,ci,&f);
 Px_Rect(d,x-1,y-1,fw+2,fh+2,C_INK);
 Flag_Draw(d,256,x,y,fw,fh,&f);
}
static const char *NN(const World *w,u8 ci){return ci&&w->c[ci].alive?NATIONS[w->c[ci].nat].name:Str(STR_NENHUM);}
static void Bar(u16 *d,int x,int y,int w,int pct,u16 col){
 if(pct<0)pct=0;
 if(pct>100)pct=100;
 Px_Rect(d,x,y,w,5,C_PARCH_SH);Px_Rect(d,x,y,w*pct/100,5,col);Px_Border(d,x-1,y-1,w+2,7,C_INK);
}

// ---------------- paginas (tela de cima) ----------------
static void PgMercado(u16 *d,const World *w,u8 me){
 const WEco *e=&w->eco[me];
 RText(d,118,28,Str(STR_OFERTA),C_INK);RText(d,160,28,Str(STR_DEMANDA),C_INK);
 RText(d,198,28,Str(STR_PRECO),C_INK);RText(d,240,28,Str(STR_MUNDO),C_INK);
 Px_Rect(d,10,39,236,1,C_PARCH_SH);
 for(int g=0;g<GD_COUNT;g++){
  int y=42+g*9;                // 13 bens (ADR-37): linhas de 9 px
  bool on=w->date.y>=Gd_Year((u8)g);
  PxW_GoodChip(d,12,y,(u8)g);
  Px_Text2(d,24,y,Str(Gd_Str((u8)g)),&F_SERIF8,on?C_INK:C_GOLD_DIM);
  if(!on){RText(d,118,y,"—",C_GOLD_DIM);continue;}
  char t[16];
  t[0]=0;Num(t,16,e->sup[g]);RText(d,118,y,t,C_INK);
  t[0]=0;Num(t,16,e->dem[g]);RText(d,160,y,t,C_INK);
  // preco: vermelho = falta (caro), verde = sobra (barato)
  u16 pc=e->price[g]*10>GD_BASE[g]*13?C_EVENT:(e->price[g]*10<GD_BASE[g]*8?C3(4,15,6):C_INK);
  t[0]=0;Num(t,16,e->price[g]);RText(d,198,y,t,pc);
  if(e->sat[g]<100){Px_Rect(d,168,y+2,2,4,C_EVENT);}
  t[0]=0;Num(t,16,w->wprice[g]);RText(d,240,y,t,C_GOLD_DIM);
 }
 Px_Rect(d,10,160,236,1,C_PARCH_SH);
 char t[112]={0};
 Cat(t,112,Str(STR_NIVEL_VIDA));Cat(t,112," ");Num(t,112,e->life);Cat(t,112,"% · ");
 Cat(t,112,Str(STR_INDUSTRIA));Cat(t,112," ");Num(t,112,e->ind);Cat(t,112,"%");
 if(e->ind<100){Cat(t,112," (");Cat(t,112,Str(STR_FALTA_INSUMO));Cat(t,112,Str(Gd_Str(e->lack)));Cat(t,112,")");}
 else{Cat(t,112," · ");Cat(t,112,Str(STR_EMPREGO));Cat(t,112," ");Num(t,112,e->jobs);Cat(t,112,"%");}
 Px_Text2(d,12,163,t,&F_SERIF8,e->life<50?C_EVENT:C_INK);
 t[0]=0;
 u8 b=w->c[me].bloc;
 if(e->reqG<GD_COUNT){Cat(t,112,Str(STR_PEDIDO_LED));Cat(t,112,Str(Gd_Str(e->reqG)));Cat(t,112," · ");Num(t,112,e->reqT);Cat(t,112,Str(STR_SEMANAS));}
 else if(b&&w->c[b].alive){Cat(t,112,Str(STR_BLOCO));Cat(t,112,": ");Cat(t,112,NN(w,b));Cat(t,112," · ");Num(t,112,WDip_BlocSize(w,b));Cat(t,112,Str(STR_MEMBROS));}
 else Cat(t,112,Str(STR_MERCADO_NAC));
 Px_Text2(d,12,175,t,&F_SERIF8,e->reqG<GD_COUNT?C_EVENT:C_GOLD_DIM);
}
static void PgComercio(u16 *d,const World *w,u8 me){
 const WEco *e=&w->eco[me];const WCountry *c=&w->c[me];
 Px_Text2(d,12,28,Str(STR_FINANCAS),&F_SERIF8,C_GOLD_DIM);
 char t[64];
 const u16 LB[4]={STR_RECEITA,STR_ALFANDEGA,STR_DESPESA,STR_SALDO};
 s32 V[4]={e->inc,e->trade,-e->exp,WEco_GoldWeek(w,me)};
 for(int i=0;i<4;i++){
  int x=i<2?12:130,y=40+(i&1)*12;
  Px_Text2(d,x,y,Str(LB[i]),&F_SERIF8,C_INK);
  t[0]=0;Signed(t,64,V[i]);Cat(t,64,Str(STR_POR_SEM));
  RText(d,x+108,y,t,V[i]<0?C_EVENT:C3(4,14,6));
 }
 Px_Rect(d,10,66,236,1,C_PARCH_SH);
 t[0]=0;Cat(t,64,Str(STR_PORTOS));Cat(t,64," ");Num(t,64,e->ports);Cat(t,64," · ");Cat(t,64,Str(STR_CAPACIDADE));Cat(t,64," ");
 Num(t,64,e->used);Cat(t,64,"/");Num(t,64,e->cap);
 Px_Text2(d,12,70,t,&F_SERIF8,C_INK);
 Bar(d,12,83,232,e->cap?e->used*100/e->cap:0,C_GOLD);
 if(WEco_Blockaded(w,me))Px_Text2(d,12,92,Str(STR_BLOQUEADO),&F_SERIF8,C_EVENT);
 else{t[0]=0;char q[16];Cat(t,64,Str(STR_EXPORTA));Cat(t,64," ");Kfmt(q,16,e->expVal/100);Cat(t,64,q);Cat(t,64," · ");
  Cat(t,64,Str(STR_IMPORTA));Cat(t,64," ");Kfmt(q,16,e->impVal/100);Cat(t,64,q);
  Px_Text2(d,12,92,t,&F_SERIF8,e->expVal>=e->impVal?C3(4,14,6):C_INK);}
 Px_Text2(d,12,104,Str(STR_ROTAS),&F_SERIF8,C_GOLD_DIM);
 int n=0;
 for(int k=0;k<3;k++){
  u8 p=e->route[k];if(!p)break;
  int y=116+k*12;
  PxW_GoodChip(d,12,y,e->routeG[k]);
  t[0]=0;Cat(t,64,Str(Gd_Str(e->routeG[k])));Cat(t,64,Str(STR_PARA));
  Px_Text2(d,24,y,t,&F_SERIF8,C_INK);
  int x=24+Px_Width2(t,&F_SERIF8)+2;
  SmallFlag(d,w,p,x,y,12,8);
  Px_Text2(d,x+16,y,NN(w,p),&F_SERIF8,C_INK);
  t[0]=0;Num(t,64,e->routeV[k]);RText(d,242,y,t,C_GOLD_DIM);
  n++;
 }
 if(!n)Px_WrapN(d,12,116,232,Str(STR_SEM_ROTAS),&F_SERIF8,C_GOLD_DIM,2,false);
 Px_Rect(d,10,153,236,1,C_PARCH_SH);
 Ui_IconId(d,12,158,ICO_SHIP,false);
 t[0]=0;Cat(t,64,Str(STR_FROTA));Cat(t,64,": ");Num(t,64,c->ships);Cat(t,64," ");Cat(t,64,Str(STR_NAVIOS));
 if(c->shipQ){Cat(t,64," · ");Cat(t,64,Str(STR_NAVIO_FILA));Num(t,64,c->shipQ);Cat(t,64,Str(STR_DIAS));}
 Px_Text2(d,32,162,t,&F_SERIF8,C_INK);
 if(c->armsTo&&w->c[c->armsTo].alive){
  // contrato de armas (ADR-37): quem recebe e quanto rendeu na semana
  t[0]=0;Cat(t,64,Str(STR_CONTRATO));Px_Text2(d,12,174,t,&F_SERIF8,C_GOLD_DIM);
  int x=14+Px_Width2(t,&F_SERIF8);SmallFlag(d,w,c->armsTo,x,174,12,8);
  t[0]=0;Num(t,64,c->armsV);Cat(t,64," · +");Num(t,64,e->rArms);Cat(t,64,Str(STR_POR_SEM));
  Px_Text2(d,x+16,174,t,&F_SERIF8,C_INK);
 }else if(!e->yards)Px_Text2(d,32,174,Str(STR_SEM_ESTALEIRO),&F_SERIF8,C_GOLD_DIM);
}
static void Row(u16 *d,const World *w,int y,u16 lab,u8 ci,const char *extra){
 Px_Text2(d,12,y,Str(lab),&F_SERIF8,C_GOLD_DIM);
 int x=86;
 if(ci&&w->c[ci].alive){SmallFlag(d,w,ci,x,y,12,8);x+=16;}
 char t[64]={0};Cat(t,64,NN(w,ci));if(extra)Cat(t,64,extra);
 Px_Text2(d,x,y,t,&F_SERIF8,C_INK);
}
static void PgDiplo(u16 *d,const World *w,u8 me){
 const WCountry *c=&w->c[me];
 char t[64];
 t[0]=0;Cat(t,64,Str(STR_IDEOLOGIA));Cat(t,64,": ");Cat(t,64,Str((u16)(STR_ID_0+c->ideo)));
 Px_Text2(d,12,28,t,&F_SERIF8,C_INK);
 // aliados vivos em ordem (um slot pode vagar antes do outro: ally[0]=0, ally[1]=X)
 u8 al[2];int na=0;
 for(int k=0;k<2;k++)if(c->ally[k]&&w->c[c->ally[k]].alive)al[na++]=c->ally[k];
 Row(d,w,42,STR_ALIADOS,na?al[0]:0,0);
 if(na>1)Row(d,w,54,STR_ALIADOS,al[1],0);
 Row(d,w,68,STR_RIVAL,c->rival,0);
 // mercado comum: lider + membros em bandeirinhas
 u8 b=c->bloc;
 Row(d,w,82,STR_BLOCO,b,0);
 if(b&&w->c[b].alive){
  int x=12,y=96;
  for(u8 j=1;j<w->nC;j++)if(w->c[j].alive&&w->c[j].bloc==b){SmallFlag(d,w,j,x,y,14,9);x+=18;if(x>230){x=12;y+=13;}}
 }
 Px_Rect(d,10,122,236,1,C_PARCH_SH);
 Px_Text2(d,12,126,Str(STR_EM_GUERRA_COM),&F_SERIF8,C_GOLD_DIM);
 t[0]=0;int k=0;
 for(u8 j=1;j<w->nC&&k<4;j++)if(World_AtWar(w,me,j)){if(k)Cat(t,64,", ");Cat(t,64,NN(w,j));k++;}
 if(!k)Cat(t,64,Str(STR_NENHUMA));
 Px_WrapN(d,96,126,148,t,&F_SERIF8,k?C_EVENT:C_INK,2,false);
 // quem mais gosta de voce (opiniao)
 Px_Text2(d,12,152,Str(STR_GOSTAM),&F_SERIF8,C_GOLD_DIM);
 u8 best[3]={0,0,0};
 for(u8 j=1;j<w->nC;j++){if(!w->c[j].alive||j==me)continue;
  for(int q=0;q<3;q++)if(!best[q]||w->c[j].opin>w->c[best[q]].opin){for(int z=2;z>q;z--)best[z]=best[z-1];best[q]=j;break;}}
 int x=12;
 for(int q=0;q<3;q++){u8 j=best[q];if(!j)break;
  SmallFlag(d,w,j,x,166,12,8);
  t[0]=0;Signed(t,64,w->c[j].opin);Px_Text2(d,x+16,166,t,&F_SERIF8,C_INK);x+=78;}
}
static void PgPolitica(u16 *d,const World *w,u8 me){
 const WCountry *c=&w->c[me];
 FlagDef f;WDip_Flag(w,me,&f);
 Px_Rect(d,11,27,38,26,C_GOLD);Flag_Draw(d,256,12,28,36,24,&f);
 Px_Text2(d,56,28,Str((u16)(STR_ID_0+c->ideo)),&F_SERIF12,C_INK);
 char t[64];t[0]=0;Cat(t,64,Nat_GovName(c->gov));Cat(t,64," · ");
 Cat(t,64,Str(Law_Str(LW_IMPOSTO,w->eco[me].law[LW_IMPOSTO])));
 Px_Text2(d,56,44,t,&F_SERIF8,C_GOLD_DIM);
 const u16 LB[3]={STR_FELICIDADE,STR_MILITANCIA,STR_RISCO};
 int V[3]={c->happy,c->militancy,WDip_RevoltRisk(w,me)};
 for(int i=0;i<3;i++){
  int y=60+i*12;
  Px_Text2(d,12,y,Str(LB[i]),&F_SERIF8,C_INK);
  Bar(d,120,y+2,96,V[i],i==0?C3(6,18,8):C_EVENT);
  t[0]=0;Num(t,64,V[i]);Cat(t,64,"%");RText(d,244,y,t,C_INK);
 }
 Px_Rect(d,10,98,236,1,C_PARCH_SH);
 t[0]=0;Cat(t,64,Str(STR_AMBICOES));Cat(t,64," · ");Num(t,64,w->objDone);Cat(t,64,Str(STR_CUMPRIDAS));
 Px_Text2(d,12,102,t,&F_SERIF8,C_GOLD_DIM);
 for(int k=0;k<3;k++){
  u8 ob=w->obj[k];
  int y=116+k*22;
  if(ob>=OB_COUNT)continue;
  Px_Text2(d,12,y,Str(Obj_Str(ob)),&F_SERIF8,C_INK);
  s32 r=Obj_Reward(ob);
  t[0]=0;Cat(t,64,"+");Num(t,64,r>0?r:-r);Cat(t,64," ");Cat(t,64,Str(r>0?STR_OURO:STR_PRESTIGIO));
  RText(d,244,y,t,C_GOLD_DIM);
  Bar(d,12,y+12,232,Obj_Pct(w,me,ob),C_GOLD);
 }
}
// Financas (ADR-32): receita e despesa da semana linha a linha, divida com
// limite e juros, PIB, gente, inflacao e as leis que pesam no caixa (ADR-40).
// por mil -> "1,5" (com sinal so' se negativo) e indice de precos -> "1,35"
static void Permil(char *t,int max,int v){if(v<0){Cat(t,max,"-");v=-v;}Num(t,max,v/10);Cat(t,max,",");char q[2]={(char)('0'+v%10),0};Cat(t,max,q);}
static void Mult(char *t,int max,u32 cpi){u32 c=(cpi+5000)/10000;Num(t,max,(s32)(c/100));Cat(t,max,",");char q[3]={(char)('0'+(c%100)/10),(char)('0'+c%10),0};Cat(t,max,q);}
static void Line(u16 *d,int x,int y,u16 lab,s32 v,bool neg){
 Px_Text2(d,x,y,Str(lab),&F_SERIF8,C_INK);
 char t[24]={0};if(v){Cat(t,24,neg?"-":"+");}Num(t,24,v<0?-v:v);
 RText(d,x+112,y,t,v==0?C_GOLD_DIM:(neg?C_EVENT:C3(4,14,6)));
}
static void PgFinancas(u16 *d,const World *w,u8 me){
 const WEco *e=&w->eco[me];const WCountry *c=&w->c[me];
 Px_Text2(d,12,26,Str(STR_RECEITA),&F_SERIF8,C_GOLD_DIM);
 Px_Text2(d,132,26,Str(STR_DESPESA),&F_SERIF8,C_GOLD_DIM);
 const u16 RL[8]={STR_RC_PROD,STR_RC_POP,STR_RC_CID,STR_RC_TERRA,STR_ALFANDEGA,STR_RC_EXPORT,STR_RC_ARMAS,STR_RC_TESOURO};
 const s32 RV[8]={e->rProd,e->rPop,e->rCity,e->rLand,e->rTariff,e->rExport,e->rArms,e->rBank};
 for(int i=0;i<8;i++)Line(d,10,36+i*9,RL[i],RV[i],false);
 // despesa: as 8 fixas; credito, concessao e mobilizacao (ADR-42/43/45) so'
 // aparecem quando existem — o que nao cabe soma em "Outros"
 const u16 XL[11]={STR_DP_EXERC,STR_DP_FROTA,STR_DP_ADM,STR_DP_EDU,STR_DP_SOC,STR_DP_OBRAS,STR_DP_JUROS,STR_DP_CORR,STR_DP_CREDITO,STR_DP_CONC,STR_DP_MOB};
 const s32 XV[11]={e->xArmy,e->xNavy,e->xAdmin,e->xEdu,e->xSoc,e->xUpkeep,e->xDebt,e->xCorrupt,e->xBank,e->xConc,e->mobX};
 {u8 li[11];int n=0;
  for(int i=0;i<11;i++)if(i<8||XV[i])li[n++]=(u8)i;
  if(n>8){int m=0;for(int j=0;j<n;j++)if(XV[li[j]]||li[j]>=8)li[m++]=li[j];n=m;}   // zeradas saem primeiro
  s32 rest=0;
  for(int j=0;j<n;j++){if(n>8&&j>=7){rest+=XV[li[j]];continue;}Line(d,130,36+j*9,XL[li[j]],XV[li[j]],true);}
  if(n>8)Line(d,130,36+7*9,STR_DP_OUTROS,rest,true);}
 Px_Rect(d,10,110,236,1,C_PARCH_SH);
 s32 bal=WEco_GoldWeek(w,me);
 char t[112]={0};Cat(t,112,Str(STR_SALDO));Cat(t,112,": ");if(bal>=0)Cat(t,112,"+");Num(t,112,bal);Cat(t,112,Str(STR_POR_SEM));
 Px_Text2(d,12,113,t,&F_SERIF12,bal<0?C_EVENT:C3(4,14,6));
 // divida: barra do quanto do limite ja' foi usado
 s32 lim=WEco_DebtLimit(w,me);
 t[0]=0;Cat(t,112,Str(STR_DIVIDA));Cat(t,112,": ");{char q[24];Kfmt(q,24,e->debt);Cat(t,112,q);}
 Cat(t,112," / ");{char q[24];Kfmt(q,24,lim);Cat(t,112,q);}Cat(t,112," ");Cat(t,112,Str(STR_LIMITE));
 Cat(t,112," · ");Num(t,112,e->rate);Cat(t,112,Str(STR_JUROS_AA));
 if(e->defaults){Cat(t,112," · ");Num(t,112,e->defaults);Cat(t,112,Str(STR_CALOTES));}
 Px_Text2(d,12,129,t,&F_SERIF8,e->debt>lim/2?C_EVENT:C_INK);
 Bar(d,12,141,232,lim?(int)((s64)e->debt*100/lim):0,e->debt>lim/2?C_EVENT:C_GOLD);
 // PIB, gente, inflacao e as leis que mais pesam no caixa
 t[0]=0;Cat(t,112,Str(STR_PIB));Cat(t,112," ");{char q[24];Kfmt(q,24,e->gdp);Cat(t,112,q);}Cat(t,112,Str(STR_POR_SEM));
 Cat(t,112," · ");Cat(t,112,Str(STR_ALFAB));Cat(t,112," ");Num(t,112,c->lit/10);Cat(t,112,"%");
 Cat(t,112," · ");Cat(t,112,Str(STR_INVEST));Cat(t,112," ");{char q[24];Kfmt(q,24,e->invest);Cat(t,112,q);}
 Px_Text2(d,12,151,t,&F_SERIF8,C_INK);
 t[0]=0;Cat(t,112,Str(STR_LFX_0+LFX_INFL));Cat(t,112," ");Permil(t,112,e->infl);Cat(t,112,Str(STR_POR_ANO));
 Cat(t,112," · ");Cat(t,112,Str(STR_PRECOS));Cat(t,112," x");Mult(t,112,e->cpi);
 Px_Text2(d,12,163,t,&F_SERIF8,e->infl>50?C_EVENT:C_GOLD_DIM);
 // juros basicos (ADR-43) e o que eles fazem agora
 t[0]=0;Cat(t,112,Str(STR_JUROS_BAS));Cat(t,112," ");Num(t,112,e->bank);Cat(t,112,Str(STR_JUROS_AA));
 Cat(t,112," · ");Cat(t,112,Str(STR_JX_PROD));Cat(t,112," ");Signed(t,112,(6-(int)e->bank)*3/2);Cat(t,112,"%");
 Cat(t,112," · ");Cat(t,112,Str(STR_JX_INV));Cat(t,112," ");{int m=130-6*(int)e->bank;if(m<10)m=10;Num(t,112,m);}Cat(t,112,"%");
 Px_Text2(d,12,175,t,&F_SERIF8,e->bank>=10?C_EVENT:(e->bank<=2?C_GOLD:C_INK));
}
// ---- Sociedade (ADR-42): pobres, medios e ricos + a burguesia ----
static void PgSociedade(u16 *d,const World *w,u8 me){
 const WSoc *so=&w->soc[me];const WEco *e=&w->eco[me];
 char t[112];
 static const u16 CU[ST_COUNT]={STR_CU_EURO,STR_CU_ASIA,STR_CU_ISLAM,STR_CU_AFRICA,STR_CU_NATIVE};
 t[0]=0;Cat(t,112,Str(STR_CULTURA));Cat(t,112,": ");Cat(t,112,Str(CU[Soc_Culture(w,me)%ST_COUNT]));
 Px_Text2(d,12,25,t,&F_SERIF8,C_GOLD_DIM);
 static const u16 SC[SS_COUNT]={PXC(14,9,5),PXC(6,12,20),PXC(22,16,3)};
 for(int k=0;k<SS_COUNT;k++){
  int y=37+k*34;
  Px_Rect(d,12,y+1,6,6,SC[k]);
  Px_Text2(d,22,y,Str(Soc_StratStr((u8)k)),&F_SERIF12,C_INK);
  // fatia da populacao e renda relativa
  t[0]=0;Num(t,112,so->share[k]/10);Cat(t,112,",");{char q[2]={(char)('0'+so->share[k]%10),0};Cat(t,112,q);}
  Cat(t,112,Str(STR_SS_POP));Cat(t,112," · ");Cat(t,112,Str(STR_SS_RENDA));Cat(t,112," x");
  {int v=so->inc[k];Num(t,112,v/100);Cat(t,112,",");char q[3]={(char)('0'+(v%100)/10),(char)('0'+v%10),0};Cat(t,112,q);}
  RText(d,244,y+2,t,C_GOLD_DIM);
  // satisfacao
  Px_Text2(d,22,y+14,Str(STR_SATISFACAO),&F_SERIF8,C_INK);
  Bar(d,104,y+16,104,so->sat[k],so->sat[k]<30?C_EVENT:(so->sat[k]>=60?PXC(6,18,8):C_GOLD));
  t[0]=0;Num(t,112,so->sat[k]);Cat(t,112,"%");RText(d,244,y+14,t,C_INK);
  // desejo em aberto (e quantas semanas faltam)
  t[0]=0;
  if(so->want[k]){Cat(t,112,Str(STR_SS_QUER));Cat(t,112,Str(Soc_WantStr(so->want[k])));Cat(t,112," (");Num(t,112,so->wantT[k]);Cat(t,112,Str(STR_SEM));Cat(t,112,")");}
  else if(so->mood[k]>0)Cat(t,112,Str(STR_SS_CONTENTE));
  else if(so->mood[k]<0)Cat(t,112,Str(STR_SS_MAGOADO));
  else Cat(t,112,Str(STR_SS_CALMO));
  Px_Text2(d,22,y+24,t,&F_SERIF8,so->want[k]?C_CRIMSON:C_GOLD_DIM);
 }
 Px_Rect(d,10,140,236,1,C_PARCH_SH);
 // burguesia: quanto dos ricos investe, o fundo privado e o que ja' bancou
 t[0]=0;Cat(t,112,Str(STR_BURGUESIA));Cat(t,112," ");Num(t,112,so->bourg);Cat(t,112,Str(STR_SS_DOS_RICOS));
 Px_Text2(d,12,144,t,&F_SERIF8,C_INK);
 t[0]=0;Cat(t,112,Str(STR_INVEST));Cat(t,112," ");{char q[24];Kfmt(q,24,e->invest);Cat(t,112,q);}
 Cat(t,112," · ");Cat(t,112,Str(STR_SS_RITMO));Cat(t,112," ");Num(t,112,Soc_InvestPct(w,me));Cat(t,112,"%");
 Px_Text2(d,12,156,t,&F_SERIF8,C_INK);
 t[0]=0;Cat(t,112,Str(STR_SS_BANCOU));{char q[24];Kfmt(q,24,so->lobbyTot);Cat(t,112,q);}
 if(e->concW){Cat(t,112," · ");Cat(t,112,Str(STR_SS_CONC));Num(t,112,e->concPct);Cat(t,112,"% · ");Num(t,112,e->concW);Cat(t,112,Str(STR_SEM));}
 Px_Text2(d,12,168,t,&F_SERIF8,C_GOLD_DIM);
}
// ---- Leis (ADR-40) ----
// valor de um efeito com unidade: % / pontos / por mil -> "x,y%/ano"
static void FxVal(char *t,int max,u8 f,int v){
 if(v>0)Cat(t,max,"+");
 if(v<0){Cat(t,max,"-");v=-v;}
 switch(f){
  case LFX_GROW:case LFX_INFL:{Num(t,max,v/10);Cat(t,max,",");char q[2]={(char)('0'+v%10),0};Cat(t,max,q);Cat(t,max,Str(STR_POR_ANO));}break;
  case LFX_HAPPY:case LFX_MILIT:case LFX_LIT:case LFX_EDU:case LFX_SOC:case LFX_TARIFF:Num(t,max,v);break;
  case LFX_RATE:Num(t,max,v);Cat(t,max," p.p.");break;
  default:Num(t,max,v);Cat(t,max,"%");break;
 }
}
// lista os efeitos de uma lei em 2 colunas (verde = bom, vermelho = ruim)
static int FxList(u16 *d,u8 cat,u8 opt,int y0){
 int n=0;
 for(u8 f=0;f<LFX_COUNT;f++){
  int v=Law_FxOf(cat,opt,f);if(!v)continue;
  int x=n&1?130:12,y=y0+(n>>1)*11;
  char t[48]={0};Cat(t,48,Str(Law_FxStr(f)));
  Px_Text2(d,x,y,t,&F_SERIF8,C_INK);
  t[0]=0;FxVal(t,48,f,v);
  RText(d,x+114,y,t,f==LFX_TARIFF?C_INK:(Law_FxBad(f,v)?C_EVENT:C3(4,14,6)));
  n++;
 }
 if(!n){Px_Text2(d,12,y0,Str(STR_LEI_NEUTRA),&F_SERIF8,C_GOLD_DIM);return y0+11;}
 return y0+((n+1)>>1)*11;
}
static void PgLeis(u16 *d,const World *w,const WGame *s,u8 me){
 const WEco *e=&w->eco[me];const WCountry *c=&w->c[me];
 char t[112];
 if(s->lawCat>=LW_COUNT){
  // visao geral: assunto, lei em vigor e se o governo gosta dela
  Px_Text2(d,12,25,Str(STR_LEI_ASSUNTOS),&F_SERIF8,C_GOLD_DIM);
  for(int k=0;k<LW_COUNT;k++){
   int y=36+k*12;u8 L=e->law[k],p=Law_Pref(c->ideo,(u8)k);
   Px_Text2(d,12,y,Str(Law_CatStr((u8)k)),&F_SERIF8,C_GOLD_DIM);
   Px_Text2(d,104,y,Str(Law_Str((u8)k,L)),&F_SERIF8,C_INK);
   if(p<LW_OPTS){int dd=L>p?L-p:p-L;u16 col=dd==0?C3(4,16,6):(dd>=3?C_EVENT:C_GOLD_DIM);Px_Rect(d,240,y+2,5,5,col);}
   if(e->lawCat==k){Px_Rect(d,96,y+1,5,7,C_GOLD);}
  }
  Px_Rect(d,10,145,236,1,C_PARCH_SH);
  t[0]=0;Cat(t,112,Str(STR_APROVACAO));Cat(t,112," ");Signed(t,112,Law_Approval(w,me));
  Cat(t,112," · ");Cat(t,112,Str(STR_LFX_0+LFX_INFL));Cat(t,112," ");Permil(t,112,e->infl);Cat(t,112,Str(STR_POR_ANO));
  Px_Text2(d,12,149,t,&F_SERIF8,C_INK);
  t[0]=0;
  if(e->lawCat<LW_COUNT){Cat(t,112,Str(STR_LEI_EM_DEBATE));Cat(t,112,Str(Law_Str(e->lawCat,e->lawTo)));
   Cat(t,112,Str(STR_LEI_DEBATE_DIAS));Num(t,112,e->lawDays);Cat(t,112,Str(STR_DIAS));}
  else{Cat(t,112,Str(STR_PRECOS));Cat(t,112," x");Mult(t,112,e->cpi);Cat(t,112," · ");Cat(t,112,Str(STR_PRESTIGIO));Cat(t,112," ");Num(t,112,c->prestige);}
  Px_WrapN(d,12,161,232,t,&F_SERIF8,e->lawCat<LW_COUNT?C_GOLD:C_GOLD_DIM,2,false);
  return;
 }
 // assunto aberto: as 5 leis em linha + a ficha da destacada
 u8 cat=s->lawCat,pk=s->lawPick%LW_OPTS;
 Px_Text2(d,12,24,Str(Law_CatStr(cat)),&F_SERIF12,C_INK);
 for(int i=0;i<LW_OPTS;i++){
  int x=12+i*47,y=42;bool cur=e->law[cat]==i,sel=pk==i;
  Px_Rect(d,x,y,44,6,cur?C_GOLD:(sel?C_INK:C_PARCH_SH));
  if(w->date.y<Law_Year(cat,(u8)i))for(int k=0;k<44;k+=3)Px_PSet(d,x+k,y+3,C_EVENT);
 }
 Px_Text2(d,12,52,Str(Law_Str(cat,pk)),&F_SERIF12,C_INK);
 int y=68;
 t[0]=0;
 if(e->law[cat]==pk)Cat(t,112,Str(STR_LEI_VIGOR));
 else if(w->date.y<Law_Year(cat,pk)){Cat(t,112,Str(STR_DESDE));Num(t,112,Law_Year(cat,pk));}
 else{Cat(t,112,Str(STR_LEI_CUSTO));Num(t,112,Law_Days(e,cat,pk));Cat(t,112,Str(STR_DIAS));Cat(t,112," · ");
  Num(t,112,Law_Prestige(e,cat,pk));Cat(t,112," ");Cat(t,112,Str(STR_PRESTIGIO));}
 if(Law_Pref(c->ideo,cat)==pk){Cat(t,112," · ");Cat(t,112,Str(STR_LEI_PREF));}
 Px_Text2(d,12,y,t,&F_SERIF8,e->law[cat]==pk?C_GOLD_DIM:C_GOLD_DIM);
 y=FxList(d,cat,pk,y+14);
 if(e->law[cat]!=pk){
  Px_Rect(d,10,y+3,236,1,C_PARCH_SH);
  t[0]=0;Cat(t,112,Str(STR_LEI_VIGOR));Cat(t,112,": ");Cat(t,112,Str(Law_Str(cat,e->law[cat])));
  Px_Text2(d,12,y+7,t,&F_SERIF8,C_GOLD_DIM);
  FxList(d,cat,e->law[cat],y+19);
 }
 if(e->lawCat==cat){t[0]=0;Cat(t,112,Str(STR_LEI_EM_DEBATE));Cat(t,112,Str(Law_Str(cat,e->lawTo)));
  Cat(t,112,Str(STR_LEI_DEBATE_DIAS));Num(t,112,e->lawDays);Cat(t,112,Str(STR_DIAS));
  Px_Text2(d,12,175,t,&F_SERIF8,C_GOLD);}
}
// ---- Graficos (ADR-41) ----
static void FmtSer(char *o,int max,u8 sr,s32 v){
 o[0]=0;
 if(sr==SR_ALFAB){Num(o,max,v/10);Cat(o,max,"%");return;}
 if(sr==SR_VIDA){Num(o,max,v);Cat(o,max,"%");return;}
 if(sr==SR_POP){   // milhares -> k / M / bi
  if(v<1000){Num(o,max,v);Cat(o,max,"k");}
  else if(v<1000000){Num(o,max,v/1000);if(v<100000){Cat(o,max,",");char q[2]={(char)('0'+(v%1000)/100),0};Cat(o,max,q);}Cat(o,max,"M");}
  else{Num(o,max,v/1000000);Cat(o,max,",");char q[2]={(char)('0'+(v%1000000)/100000),0};Cat(o,max,q);Cat(o,max,"bi");}
  return;}
 Kfmt(o,max,v);
}
static void GLine(u16 *d,int x0,int y0,int x1,int y1,u16 c,bool thick){
 int dx=x1-x0,dy=y1-y0,n=dx<0?-dx:dx;int ady=dy<0?-dy:dy;if(ady>n)n=ady;if(!n)n=1;
 for(int k=0;k<=n;k++){int x=x0+dx*k/n,y=y0+dy*k/n;Px_PSet(d,x,y,c);if(thick)Px_PSet(d,x,y-1,c);}
}
static void PgGraf(u16 *d,const World *w,const WGame *s,u8 me){
 const WStats *st=&w->st;u8 sr=s->statSel%SR_COUNT;
 char t[96];
 Px_Text2(d,12,24,Str(Stat_Str(sr)),&F_SERIF12,C_INK);
 // valor de hoje e variacao em ~10 anos
 s32 now=Stat_Val(w,me,sr);
 t[0]=0;FmtSer(t,96,sr,now);RText(d,244,26,t,C_INK);
 int n=st->n;
 if(n<3){Px_WrapN(d,12,80,232,Str(STR_GR_SEM),&F_SERIF8,C_GOLD_DIM,2,false);return;}
 int back=(int)(3650/(st->step?st->step:14));if(back>=n)back=n-1;if(back<1)back=1;
 s32 a=st->v[sr][0][n-1-back],b=st->v[sr][0][n-1];
 t[0]=0;
 if(a>0){s32 pct=(s32)((s64)(b-a)*100/a);Signed(t,96,pct);Cat(t,96,"% ");Cat(t,96,Str(STR_GR_10ANOS));
  RText(d,244,39,t,pct<0?(sr==SR_DIVIDA?C3(4,14,6):C_EVENT):(sr==SR_DIVIDA?C_EVENT:C3(4,14,6)));}
 // escala: de min(0, menor) ate' o maior das duas linhas
 s32 lo=0,hi=1;
 for(int l=0;l<2;l++)for(int i=0;i<n;i++){s32 v=st->v[sr][l][i];if(v>hi)hi=v;if(v<lo)lo=v;}
 hi+=(hi-lo)/10+1;
 enum { PX=40, PY=52, PW=204, PH=112 };
 Px_Rect(d,PX,PY,PW,PH,C3(29,27,22));
 for(int k=0;k<=4;k++){int y=PY+PH*k/4;for(int x=PX;x<PX+PW;x+=2)Px_PSet(d,x,y,C_PARCH_SH);
  t[0]=0;FmtSer(t,96,sr,hi-(s32)((s64)(hi-lo)*k/4));RText(d,PX-3,y-3,t,C_GOLD_DIM);}
 // linha do lider (cinza) e a sua (ouro, grossa, com area pintada)
 int px[ST_N],py[ST_N];
 for(int l=1;l>=0;l--){
  for(int i=0;i<n;i++){px[i]=PX+i*(PW-1)/(n-1);py[i]=PY+PH-1-(int)((s64)(st->v[sr][l][i]-lo)*(PH-1)/(hi-lo));}
  if(l==0)for(int i=0;i<n-1;i++)for(int x=px[i];x<px[i+1];x++){int y=py[i]+(py[i+1]-py[i])*(x-px[i])/(px[i+1]-px[i]>0?px[i+1]-px[i]:1);
   for(int yy=y+1;yy<PY+PH;yy++)if(((x+yy)&1)==0)Px_PSet(d,x,yy,C3(28,23,12));}
  for(int i=0;i<n-1;i++)GLine(d,px[i],py[i],px[i+1],py[i+1],l?C3(12,13,16):C3(26,18,2),l==0);
 }
 Px_Border(d,PX-1,PY-1,PW+2,PH+2,C_INK);
 // anos no eixo x (comeco, meio, fim)
 for(int k=0;k<3;k++){int i=k==0?0:(k==1?(n-1)/2:n-1);Date dt=Cal_Date(Stat_DayOf(w,i));t[0]=0;Num(t,96,dt.y);
  int x=PX+i*(PW-1)/(n-1)-Px_Width2(t,&F_SERIF8)/2;if(x<PX)x=PX;if(x>PX+PW-24)x=PX+PW-24;Px_Text2(d,x,PY+PH+3,t,&F_SERIF8,C_GOLD_DIM);}
 // legenda
 Px_Rect(d,12,40,10,3,C3(26,18,2));Px_Text2(d,26,37,Str(STR_GR_VOCE),&F_SERIF8,C_INK);
 if(st->top&&st->top<w->nC){Px_Rect(d,66,40,10,3,C3(12,13,16));SmallFlag(d,w,st->top,80,37,12,8);
  Px_Text2(d,96,37,NN(w,st->top),&F_SERIF8,C_INK);}
}
void PxL_Top(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();u8 me=s->me;
 Ui_Backdrop(d);
 Px_Rect(d,0,0,256,20,C_NAVY);Px_Rect(d,0,19,256,1,C_GOLD);
 SmallFlag(d,w,me,6,5,16,10);
 char t[64]={0};Cat(t,64,Str(STR_GABINETE));Cat(t,64," · ");Cat(t,64,Str(TABS[s->ledTab%LT_COUNT]));
 Px_Text2(d,28,4,t,&F_SERIF12,C_TEXT);
 char g[24];Kfmt(g,24,w->c[me].gold);
 s32 wk=WEco_GoldWeek(w,me);
 char q[24]={0};Signed(q,24,wk);
 int wq=Px_Width2(q,&F_SERIF8);
 Px_Text2(d,250-wq,6,q,&F_SERIF8,wk<0?C_EVENT:C_GOLD);
 Ui_IconId(d,250-wq-Px_Width2(g,&F_SERIF8)-20,4,ICO_COIN_S,false);
 Px_Text2(d,250-wq-Px_Width2(g,&F_SERIF8)-4,6,g,&F_SERIF8,C_TEXT);
 Ui_Parch(d,4,22,248,166);
 switch(s->ledTab){
  case LT_MERCADO:PgMercado(d,w,me);break;
  case LT_COMERCIO:PgComercio(d,w,me);break;
  case LT_DIPLO:PgDiplo(d,w,me);break;
  case LT_FINANCAS:PgFinancas(d,w,me);break;
  case LT_LEIS:PgLeis(d,w,s,me);break;
  case LT_GRAF:PgGraf(d,w,s,me);break;
  case LT_SOCIEDADE:PgSociedade(d,w,me);break;
  default:PgPolitica(d,w,me);break;
 }
}

// ---------------- abas + acao (tela de baixo) ----------------
static void LBtn(u16 *d,int x,int y,int bw,int bh,bool hot,bool on){
 Px_Rect(d,x,y,bw,bh,hot?C_CRIMSON:(on?C_NAVY:C_NAVY_BG));
 Px_Border(d,x,y,bw,bh,on?C_GOLD:C_GOLD_DIM);
 if(hot)for(int k=1;k<bw-1;k++)Px_PSet(d,x+k,y+1,C_CRIMSON_HI);
}
static void LBack(u16 *d,int x,int y,int bw,int bh){
 Px_Rect(d,x,y,bw,bh,C_NAVY);Px_Border(d,x,y,bw,bh,C_GOLD_DIM);
 if(bw<120){Ui_TextC(d,x+bw/2,y+bh/2-5,Str(STR_LEI_ASSUNTOS),0,C_TEXT);return;}   // botao estreito da aba Leis: volta aos assuntos
 Ui_TextC(d,x+bw/2,y+bh/2-5,Str(STR_VOLTAR),0,C_TEXT);
 Px_Text2(d,x+bw-14,y+bh/2-5,"B",&F_SERIF8,C_GOLD_DIM);
}
static void BotLeis(u16 *d,const World *w,const WGame *s,u8 me){
 const WEco *e=&w->eco[me];
 if(s->lawCat>=LW_COUNT){
  for(int k=0;k<LW_COUNT;k++){
   int x=LW_GX+(k%3)*LW_GW,y=LW_GY+(k/3)*LW_GH;
   LBtn(d,x,y,LW_GW-4,LW_GH-3,false,true);
   if(e->lawCat==k)Px_Border(d,x+1,y+1,LW_GW-6,LW_GH-5,C_GOLD_HI);
   Px_WrapN(d,x+4,y+3,LW_GW-12,Str(Law_CatStr((u8)k)),&F_SERIF8,C_TEXT_DIM,1,false);   // 1 linha: nome longo corta na palavra
   Px_WrapN(d,x+4,y+13,LW_GW-12,Str(Law_Str((u8)k,e->law[k])),&F_SERIF8,C_TEXT,2,false);
  }
  LBack(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H);
  return;
 }
 u8 cat=s->lawCat;
 for(int i=0;i<LW_OPTS;i++){
  int y=LW_LY+i*LW_LH;bool cur=e->law[cat]==i,sel=s->lawPick==i,lock=w->date.y<Law_Year(cat,(u8)i);
  LBtn(d,4,y,248,LW_LH-3,sel,!lock);
  Px_Text2(d,12,y+5,Str(Law_Str(cat,(u8)i)),&F_SERIF8,lock?C_TEXT_DIM:C_TEXT);
  char t[24]={0};
  if(cur)Cat(t,24,Str(STR_LEI_VIGOR));
  else if(e->lawCat==cat&&e->lawTo==i){Num(t,24,e->lawDays);Cat(t,24,Str(STR_DIAS));}
  else if(lock)Num(t,24,Law_Year(cat,(u8)i));
  Px_Text2(d,244-Px_Width2(t,&F_SERIF8),y+5,t,&F_SERIF8,cur?C_GOLD:C_TEXT_DIM);
 }
 u8 pk=s->lawPick%LW_OPTS;
 bool undo=e->lawCat==cat&&e->lawTo==pk;
 u16 why=Law_Why(w,me,cat,pk);
 bool ok=undo||!why;
 LBtn(d,4,LW_BY,LW_PW,LW_BH,ok,ok);
 Ui_TextC(d,4+LW_PW/2,LW_BY+6,Str(undo?STR_LEI_RETIRAR:(why?why:STR_LEI_PROPOR)),0,ok?C_TEXT:C_TEXT_DIM);
 if(!why&&!undo){char t[40]={0};Num(t,40,Law_Days(e,cat,pk));Cat(t,40,Str(STR_DIAS));Cat(t,40," · ");Num(t,40,Law_Prestige(e,cat,pk));
  Cat(t,40," ");Cat(t,40,Str(STR_PRESTIGIO));Ui_TextC(d,4+LW_PW/2,LW_BY+19,t,0,C_TEXT_DIM);}
 LBack(d,4+LW_PW+4,LW_BY,248-LW_PW-4,LW_BH);
}
static void BotGraf(u16 *d,const WGame *s){
 for(int k=0;k<SR_COUNT;k++){
  int x=GR_BX+(k%4)*GR_BW,y=GR_BY+(k/4)*(GR_BH+4);
  LBtn(d,x,y,GR_BW-4,GR_BH,s->statSel==k,true);
  Px_WrapN(d,x+4,y+5,GR_BW-10,Str(Stat_Str((u8)k)),&F_SERIF8,C_TEXT,2,true);
 }
 Ui_Navy(d,4,112,248,38);
 Px_WrapN(d,12,117,232,Str(STR_H_GRAF),&F_SERIF8,C_TEXT,3,false);
 LBack(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H);
}
void PxL_Bot(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();u8 me=s->me;
 Ui_Backdrop(d);
 for(int i=0;i<LT_COUNT;i++){
  int x,y,tw;Lg_TabRect(i,&x,&y,&tw);x++;bool sel=i==s->ledTab;
  Px_Rect(d,x,y,tw-2,LG_TAB_RH,sel?C_CRIMSON:C_NAVY);
  Px_Border(d,x,y,tw-2,LG_TAB_RH,sel?C_GOLD:C_GOLD_DIM);
  if(sel)for(int k=1;k<tw-3;k++)Px_PSet(d,x+k,y+1,C_CRIMSON_HI);
  Ui_TextC(d,x+tw/2-1,y+4,Str(TABS[i]),0,sel?C_TEXT:C_TEXT_DIM);
 }
 if(s->ledTab==LT_LEIS){BotLeis(d,w,s,me);return;}
 if(s->ledTab==LT_GRAF){BotGraf(d,s);return;}
 static const u16 HS[LT_COUNT]={STR_H_MERCADO,STR_H_COMERCIO,STR_H_DIPLO,STR_H_POLITICA,STR_H_FINANCAS,STR_H_LEIS,STR_H_GRAF,STR_H_SOCIEDADE};
 Ui_Navy(d,4,38,248,76);
 Px_WrapN(d,12,44,232,Str(HS[s->ledTab%LT_COUNT]),&F_SERIF8,C_TEXT,6,false);
 // acao da aba
 char t[64]={0};bool en=true;u8 ico=ICO_BOOK;
 const WCountry *c=&w->c[me];const WEco *e=&w->eco[me];
 switch(s->ledTab){
  case LT_MERCADO:Cat(t,64,Str(STR_LA_MAPA));ico=ICO_GLOBE;break;
  case LT_COMERCIO:Cat(t,64,Str(STR_LANCAR_NAVIO));Cat(t,64," · ");Num(t,64,WEco_ShipCost(w,me));
   en=e->yards&&!c->shipQ&&c->gold>=WEco_ShipCost(w,me);ico=ICO_SHIP;break;
  case LT_DIPLO:Cat(t,64,Str(c->bloc?STR_SAIR_BLOCO:STR_CRIAR_BLOCO));ico=ICO_FLAG;break;
  case LT_FINANCAS:{   // juros (ADR-43): 2 metades, baixar e subir
   for(int h=0;h<2;h++){
    int x=LG_ACT_X+h*FN_HALF;bool on=h?e->bank<20:e->bank>0;
    Px_Rect(d,x,LG_ACT_Y,FN_HALF-2,LG_ACT_H,on?(h?C_CRIMSON:C_NAVY_HI):C_NAVY);
    Px_Border(d,x,LG_ACT_Y,FN_HALF-2,LG_ACT_H,C_GOLD);
    char q[40]={0};Cat(q,40,Str(h?STR_JUROS_MAIS:STR_JUROS_MENOS));
    Ui_TextC(d,x+FN_HALF/2-1,LG_ACT_Y+4,q,0,on?C_TEXT:C_TEXT_DIM);
    q[0]=0;Num(q,40,e->bank+(h?1:-1)<0?0:e->bank+(h?1:-1));Cat(q,40,Str(STR_JUROS_AA));
    Ui_TextC(d,x+FN_HALF/2-1,LG_ACT_Y+15,q,0,C_TEXT_DIM);
   }
   LBack(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H);
  }return;
  default:Cat(t,64,Str(STR_IR_LEIS));ico=ICO_BOOK;break;     // Politica e Sociedade levam as leis
 }
 Px_Rect(d,LG_ACT_X,LG_ACT_Y,LG_ACT_W,LG_ACT_H,en?C_CRIMSON:C_NAVY);
 Px_Border(d,LG_ACT_X,LG_ACT_Y,LG_ACT_W,LG_ACT_H,C_GOLD);
 Ui_IconId(d,LG_ACT_X+8,LG_ACT_Y+6,ico,!en);
 Px_Text2(d,LG_ACT_X+34,LG_ACT_Y+9,t,&F_SERIF8,en?C_TEXT:C_TEXT_DIM);
 LBack(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H);
}
