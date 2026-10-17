#include "wgame.h"
#include "wecon.h"
#include "wdiplo.h"
#include "px.h"
#include "pal.h"
#include "font.h"
#include "ui.h"
#include "str.h"
#include "flag.h"
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
 {{18,19,21},{10,10,12},{26,27,28}},{{22,6,5},{12,3,3},{28,24,22}},{{5,5,7},{2,2,3},{14,14,17}}};
static const char *const GICO[GD_COUNT][8]={
 {"..a.a.a.",".a.aca..","..aca.a.","...aa...","...bb...","..a.ba..","...bb...","...bb..."},
 {"........",".cccccc.","ac.aaaab","aa.aaaab",".bbbbbb.","ac.aaaab","aa.aaaab",".bbbbbb."},
 {"..cc.c..",".ccccca.","cccccccc",".cccccc.","..dddd..","...dd...","...dd...","...d...."},
 {"........","...cc...","..caac..",".caaaab.","caaabaab","aabaaabb",".abbbbb.","........"},
 {"..b..b..","...bb...","..aaaa..",".acaaaa.","aacaaaab","aaaaaaab",".abbbbb.","........"},
 {"........",".cccccc.","caaaaaab","cacacacb","caaaaaab","cacacacb",".bbbbbb.","........"},
 {".cccc...","caaaab..",".bbbbd..","....d...","...d....","..d.....",".d......","d......."},
 {"......c.",".....ca.","....ca..","...ca...","d.ca....",".db.....",".dd.....","d..d...."},
 {"...c....","...a....","..aca...","..aaa...",".aacaa..",".aaaab..","..abb...","........"}};
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

static const u16 TABS[LT_COUNT]={STR_TAB_MERCADO,STR_TAB_COMERCIO,STR_TAB_DIPLO,STR_TAB_POLITICA};
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
  int y=43+g*12;
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
 Px_Rect(d,10,152,236,1,C_PARCH_SH);
 char t[96]={0};
 Cat(t,96,Str(STR_NIVEL_VIDA));Cat(t,96," ");Num(t,96,e->life);Cat(t,96,"% · ");
 Cat(t,96,Str(STR_INDUSTRIA));Cat(t,96," ");Num(t,96,e->ind);Cat(t,96,"%");
 if(e->ind<100){Cat(t,96," (");Cat(t,96,Str(STR_FALTA_INSUMO));Cat(t,96,Str(Gd_Str(e->lack)));Cat(t,96,")");}
 Px_Text2(d,12,156,t,&F_SERIF8,e->life<50?C_EVENT:C_INK);
 t[0]=0;
 u8 b=w->c[me].bloc;
 if(b&&w->c[b].alive){Cat(t,96,Str(STR_BLOCO));Cat(t,96,": ");Cat(t,96,NN(w,b));Cat(t,96," · ");Num(t,96,WDip_BlocSize(w,b));Cat(t,96,Str(STR_MEMBROS));}
 else Cat(t,96,Str(STR_MERCADO_NAC));
 Px_Text2(d,12,169,t,&F_SERIF8,C_GOLD_DIM);
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
 if(!e->yards)Px_Text2(d,32,174,Str(STR_SEM_ESTALEIRO),&F_SERIF8,C_GOLD_DIM);
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
 char t[64];t[0]=0;Cat(t,64,Nat_GovName(c->gov));Cat(t,64," · ");Cat(t,64,Str(STR_IMPOSTO));Cat(t,64,": ");
 static const u16 TX[3]={STR_BAIXO,STR_MEDIO,STR_ALTO};
 Cat(t,64,Str(TX[w->eco[me].tax%3]));
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
  default:PgPolitica(d,w,me);break;
 }
}

// ---------------- abas + acao (tela de baixo) ----------------
void PxL_Bot(u16 *d){
 const World *w=WG_World();const WGame *s=WG_Ptr();u8 me=s->me;
 Ui_Backdrop(d);
 for(int i=0;i<LT_COUNT;i++){
  int x=i*64+2;bool sel=i==s->ledTab;
  Px_Rect(d,x,LG_TAB_Y,60,LG_TAB_H,sel?C_CRIMSON:C_NAVY);
  Px_Border(d,x,LG_TAB_Y,60,LG_TAB_H,sel?C_GOLD:C_GOLD_DIM);
  if(sel)for(int k=1;k<59;k++)Px_PSet(d,x+k,LG_TAB_Y+1,C_CRIMSON_HI);
  Ui_TextC(d,x+30,LG_TAB_Y+10,Str(TABS[i]),0,sel?C_TEXT:C_TEXT_DIM);
 }
 Ui_Navy(d,4,36,248,78);
 static const u16 HS[LT_COUNT]={STR_H_MERCADO,STR_H_COMERCIO,STR_H_DIPLO,STR_H_POLITICA};
 Px_WrapN(d,12,44,232,Str(HS[s->ledTab%LT_COUNT]),&F_SERIF8,C_TEXT,6,false);
 // acao da aba
 char t[64]={0};bool en=true;u8 ico=ICO_BOOK;
 const WCountry *c=&w->c[me];const WEco *e=&w->eco[me];
 switch(s->ledTab){
  case LT_MERCADO:Cat(t,64,Str(STR_LA_MAPA));ico=ICO_GLOBE;break;
  case LT_COMERCIO:Cat(t,64,Str(STR_LANCAR_NAVIO));Cat(t,64," · ");Num(t,64,WEco_ShipCost(w,me));
   en=e->yards&&!c->shipQ&&c->gold>=WEco_ShipCost(w,me);ico=ICO_SHIP;break;
  case LT_DIPLO:Cat(t,64,Str(c->bloc?STR_SAIR_BLOCO:STR_CRIAR_BLOCO));ico=ICO_FLAG;break;
  default:{static const u16 TX[3]={STR_BAIXO,STR_MEDIO,STR_ALTO};
   Cat(t,64,Str(STR_IMPOSTO));Cat(t,64,": ");Cat(t,64,Str(TX[e->tax%3]));ico=ICO_COIN_S;}break;
 }
 Px_Rect(d,LG_ACT_X,LG_ACT_Y,LG_ACT_W,LG_ACT_H,en?C_CRIMSON:C_NAVY);
 Px_Border(d,LG_ACT_X,LG_ACT_Y,LG_ACT_W,LG_ACT_H,C_GOLD);
 Ui_IconId(d,LG_ACT_X+8,LG_ACT_Y+6,ico,!en);
 Px_Text2(d,LG_ACT_X+34,LG_ACT_Y+9,t,&F_SERIF8,en?C_TEXT:C_TEXT_DIM);
 Px_Rect(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H,C_NAVY);
 Px_Border(d,LG_ACT_X,LG_BACK_Y,LG_ACT_W,LG_ACT_H,C_GOLD_DIM);
 Ui_TextC(d,128,LG_BACK_Y+9,Str(STR_VOLTAR),0,C_TEXT);
 Px_Text2(d,LG_ACT_X+LG_ACT_W-16,LG_BACK_Y+9,"B",&F_SERIF8,C_GOLD_DIM);
}
