// test_core.c — testes do nucleo do mundo no PC (DESIGN.md sec 9, SDD sec 15).
// Uso: host/run_tests.sh  (compila com gcc no WSL/Linux e roda).
// Saida: "OK"/"FALHA" por teste, avisos de conteudo, e imagens PPM em
// host/out/ (mapa-mundi em varios anos, mapa aleatorio, folha de bandeiras).
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "calendar.h"
#include "txt.h"
#include "traits.h"
#include "nations.h"
#include "flag.h"
#include "px.h"
#include "world.h"
#include "city.h"
#include "history.h"

static int fails=0, warns=0;
#define CHECK(c,...) do{if(!(c)){printf("  FALHA: ");printf(__VA_ARGS__);printf("\n");fails++;}}while(0)
#define WARN(...) do{printf("  aviso: ");printf(__VA_ARGS__);printf("\n");warns++;}while(0)

static World W1, W2;

// ---------- imagens ----------
static void Rgb(u16 c,u8 *o){o[0]=(u8)((c&31)<<3);o[1]=(u8)(((c>>5)&31)<<3);o[2]=(u8)(((c>>10)&31)<<3);}
static void SaveMap(const World *w,const char *path,int scale){
 FILE *f=fopen(path,"wb");if(!f)return;
 fprintf(f,"P6 %d %d 255\n",w->w*scale,w->h*scale);
 static const u8 TC[WT_COUNT][3]={{20,40,90},{120,150,90},{60,110,60},{140,130,90},{150,140,130},
  {200,180,120},{80,110,90},{40,100,50},{150,160,150},{170,170,110},{230,235,240}};
 for(int y=0;y<w->h;y++)for(int sy=0;sy<scale;sy++)for(int x=0;x<w->w;x++){
  u32 i=World_Idx(w,x,y);u8 o=W_OWNER[i];u8 px[3];
  const u8 *t=TC[W_TERR[i]];
  if(W_TERR[i]==WT_WATER){px[0]=t[0];px[1]=t[1];px[2]=t[2];}
  else if(!o){px[0]=t[0]/2+60;px[1]=t[1]/2+60;px[2]=t[2]/2+60;}
  else{const NationDef *d=&NATIONS[w->c[o].nat];
   // cor do pais (70%) sobre o terreno (30%), como no zoom proximo do DESIGN 4.2
   px[0]=(u8)((d->r*8*7+t[0]*3)/10);px[1]=(u8)((d->g*8*7+t[1]*3)/10);px[2]=(u8)((d->b*8*7+t[2]*3)/10);
   bool edge=false;
   for(int k=0;k<4&&!edge;k++){int nx=x+(k==0)-(k==1),ny=y+(k==2)-(k==3);
    if(nx<0||ny<0||nx>=w->w||ny>=w->h)continue;
    u8 q=W_OWNER[World_Idx(w,nx,ny)];
    if(q!=o&&W_TERR[World_Idx(w,nx,ny)]!=WT_WATER)edge=true;}
   if(edge){px[0]/=2;px[1]/=2;px[2]/=2;}
   if(w->c[o].capX==x&&w->c[o].capY==y){px[0]=255;px[1]=220;px[2]=60;}
  }
  for(int sx=0;sx<scale;sx++)fwrite(px,1,3,f);
 }
 fclose(f);
}
static void SaveFlags(const char *path){
 enum{FW=40,FH=58,PAD=6,COLS=16};
 int rows=(NATION_COUNT+COLS-1)/COLS;
 int W=COLS*(FW+PAD)+PAD,H=rows*(FH+PAD)+PAD;
 static u16 buf[(16*46+6)*((200/16+1)*64+6)];
 if((size_t)(W*H)>sizeof(buf)/sizeof(buf[0])){WARN("folha de bandeiras grande demais");return;}
 for(int i=0;i<W*H;i++)buf[i]=Px_Col(28,26,20); // pergaminho
 for(int n=0;n<NATION_COUNT;n++){
  int x=PAD+(n%COLS)*(FW+PAD),y=PAD+(n/COLS)*(FH+PAD);
  Flag_Banner(buf,W,x,y,FW,FH,&NATIONS[n].flag);
 }
 FILE *f=fopen(path,"wb");if(!f)return;
 fprintf(f,"P6 %d %d 255\n",W,H);
 for(int i=0;i<W*H;i++){u8 o[3];Rgb(buf[i],o);fwrite(o,1,3,f);}
 fclose(f);
}

// ---------- testes ----------
static void TestTxt(void){
 printf("[txt]\n");
 char o[64];
 struct{const char *in,*out;}T[]={{"Ngāpuhi","NGAPUHI"},{"Očhéthi Šakówiŋ","OCHETHI SAKOWIN"},
  {"Đại Việt","DAI VIET"},{"Hawaiʻi","HAWAI'I"},{"Nʉmʉnʉʉ","NUMUNUU"},{"Ọ̀yọ́","OYO"},
  {"Configurações","CONFIGURACOES"},{"Yolŋu","YOLNU"},{"Mẽbêngôkre","MEBENGOKRE"}};
 for(unsigned i=0;i<sizeof(T)/sizeof(T[0]);i++){
  Txt_Fold(T[i].in,o,64);
  CHECK(!strcmp(o,T[i].out),"Txt_Fold(%s) = %s, esperado %s",T[i].in,o,T[i].out);
 }
 CHECK(Txt_Len("Ngāpuhi")==7,"Txt_Len");
 // todo texto dos dados deve dobrar sem '?'
 for(int n=0;n<NATION_COUNT;n++){
  const char *s[3]={NATIONS[n].name,NATIONS[n].endo,NATIONS[n].desc};
  for(int k=0;k<3;k++){if(!s[k])continue;Txt_Fold(s[k],o,64);
   CHECK(!strchr(o,'?'),"glifo sem dobra em %s: %s",NATIONS[n].tag,s[k]);}
 }
}
static void TestCalendar(void){
 printf("[calendario]\n");
 CHECK(Cal_DaysFrom(1800,1,1)==0,"dia 0");
 CHECK(Cal_DaysFrom(1900,3,1)-Cal_DaysFrom(1900,2,28)==1,"1900 nao e bissexto");
 CHECK(Cal_DaysFrom(2000,3,1)-Cal_DaysFrom(2000,2,28)==2,"2000 e bissexto");
 Date d={1800,1,1};
 u32 end=Cal_EndDay();
 for(u32 i=0;i<=end+400;i++){
  Date e=Cal_Date(i);
  if(e.y!=d.y||e.m!=d.m||e.d!=d.d){CHECK(0,"Cal_Date(%u) != Cal_Next",i);break;}
  if(Cal_DaysFrom(e.y,e.m,e.d)!=i){CHECK(0,"ida e volta %u",i);break;}
  Cal_Next(&d);
 }
 Date e=Cal_Date(end);
 CHECK(e.y==2025&&e.m==12&&e.d==31,"fim = 31/12/2025 (%d/%d/%d)",e.d,e.m,e.y);
 CHECK(Cal_Era(1814)==ERA_NAPOLEONICA&&Cal_Era(1815)==ERA_RESTAURACAO,"era 1815");
 CHECK(Cal_Era(1945)==ERA_GUERRA_FRIA&&Cal_Era(2025)==ERA_GLOBALIZACAO&&Cal_Era(2026)==ERA_ETERNA,"eras modernas");
 char b[32];Cal_Format(&e,b,32);
 CHECK(!strcmp(b,"31 dez 2025"),"Cal_Format = %s",b);
 printf("  %u dias de 1/1/1800 a 31/12/2025\n",end+1);
}
static void TestNations(void){
 printf("[paises + traits]\n");
 CHECK(NATION_COUNT<255,"NATION_COUNT %d cabe em u8",NATION_COUNT);
 int start=0,cont[CONT_COUNT]={0},kinds[4]={0};
 for(int i=0;i<NATION_COUNT;i++){
  const NationDef *d=&NATIONS[i];
  CHECK(strlen(d->tag)==3,"tag %s",d->tag);
  CHECK(Nat_Find(d->tag)==i,"tag duplicada %s",d->tag);
  if(d->overlord)CHECK(Nat_Find(d->overlord)>=0,"suserano %s de %s",d->overlord,d->tag);
  if(d->overlord)CHECK(d->kind==NK_SUDITO,"%s tem suserano mas nao e sudito",d->tag);
  int b=Tr_Budget(d->traits);
  bool gloria=false,giant=false;
  for(int k=0;k<NAT_TRAITS;k++){CHECK(d->traits[k]<TR_COUNT,"trait invalido em %s",d->tag);
   if(d->traits[k]==TR_GLORIA)gloria=true;
   if(d->traits[k]==TR_MULTIETNICO)giant=true;
   for(int q=k+1;q<NAT_TRAITS;q++)CHECK(!d->traits[k]||d->traits[k]!=d->traits[q],"trait repetido em %s",d->tag);}
  if(!gloria)CHECK(b>=(giant?0:1)&&b<=5,"orcamento de traits de %s = %d (DESIGN 6.4)",d->tag,b);
  CHECK(d->lat>=-90&&d->lat<=90&&d->lon>=-180&&d->lon<=180,"capital de %s",d->tag);
  if(Txt_Len(d->desc)>95)WARN("descricao de %s tem %d caracteres (alvo ~90)",d->tag,Txt_Len(d->desc));
  if(!d->yFrom){start++;cont[d->cont]++;kinds[d->kind]++;}
 }
 for(int a=0;a<NAT_ANCHOR_COUNT;a++)CHECK(Nat_Find(NAT_ANCHORS[a].tag)>=0,"ancora %s",NAT_ANCHORS[a].tag);
 printf("  %d no catalogo, %d em 1800 (estados %d, suditos %d, povos %d, autonomos %d)\n",
  NATION_COUNT,start,kinds[0],kinds[1],kinds[2],kinds[3]);
 for(int c=0;c<CONT_COUNT;c++){
  int pct=cont[c]*100/start;
  printf("  %-9s %3d (%d%%)\n",Nat_ContName((u8)c),cont[c],pct);
  if(pct<15)WARN("%s abaixo de 15%% das entidades de 1800 (DESIGN 6.1)",Nat_ContName((u8)c));
 }
 if(start<150)WARN("%d entidades em 1800; meta do DESIGN 6.1 e >=150",start);
 if(NATION_COUNT<300)WARN("catalogo com %d; meta do DESIGN 6.1 e >=300",NATION_COUNT);
 for(int t=1;t<TR_COUNT;t++){int n=0;for(int i=0;i<NATION_COUNT;i++)for(int k=0;k<NAT_TRAITS;k++)if(NATIONS[i].traits[k]==t)n++;
  if(n<2)WARN("trait '%s' usado por %d pais(es) (DESIGN 8.3 pede >=2)",Tr_Name((u8)t),n);}
}
static void TestHistoryData(void){
 printf("[eventos]\n");
 static const char *LOCS[]={"LOU","SIN","OKL","HKG","CAL","NMX","ALS","TWN","ATA","TAR"};
 u32 prev=0;
 for(int i=0;i<Hist_Count();i++){
  const HistEvent *e=Hist_Get(i);
  u32 a=Cal_DaysFrom(e->y,e->m,e->d);
  CHECK(a>=prev,"eventos fora de ordem em %s",e->id);prev=a;
  CHECK(e->y>=1800&&e->y<=2025,"ano de %s",e->id);
  CHECK(Txt_Len(e->title)<=32,"titulo longo em %s",e->id);
  if(Txt_Len(e->body)>160)WARN("corpo de %s com %d caracteres (DESIGN 3.4: <=160)",e->id,Txt_Len(e->body));
  for(int j=0;j<i;j++)CHECK(strcmp(Hist_Get(j)->id,e->id),"id repetido %s",e->id);
  const HOp *ops[2]={e->cond,e->eff};int nops[2]={3,6};
  for(int s=0;s<2;s++)for(int k=0;k<nops[s];k++){
   const HOp *o=&ops[s][k];
   if(!o->op)continue;
   bool fired=(s==0&&o->op==HC_FIRED);
   if(o->a&&!fired)CHECK(Nat_Find(o->a)>=0,"tag %s em %s",o->a,e->id);
   if(o->b)CHECK(Nat_Find(o->b)>=0,"tag %s em %s",o->b,e->id);
   if(o->c){bool ok=Nat_Find(o->c)>=0;for(unsigned q=0;q<sizeof(LOCS)/sizeof(LOCS[0]);q++)if(!strcmp(o->c,LOCS[q]))ok=true;
    CHECK(ok,"local %s em %s",o->c,e->id);}
  }
 }
 printf("  %d eventos historicos (1801-2022), %d eventos aleatorios\n",Hist_Count(),Rnd_Count());
}
static u32 SumCells(const World *w){u32 s=0;for(u8 i=1;i<w->nC;i++)if(w->c[i].alive)s+=w->c[i].cells;return s;}
static u32 CountOwned(const World *w){u32 s=0,n=(u32)w->w*w->h;for(u32 i=0;i<n;i++)if(W_OWNER[i])s++;return s;}
static void Invariants(const World *w,const char *ctx){
 CHECK(SumCells(w)==CountOwned(w),"%s: soma de celulas %u != grade %u",ctx,SumCells(w),CountOwned(w));
 u32 n=(u32)w->w*w->h;
 for(u32 i=0;i<n;i++){u8 o=W_OWNER[i];
  if(o&&(!w->c[o].alive||W_TERR[i]==WT_WATER)){CHECK(0,"%s: celula %u de pais morto/agua",ctx,i);break;}}
 for(u8 i=1;i<w->nC;i++){const WCountry *c=&w->c[i];
  if(!c->alive)continue;
  CHECK(c->troops>=0&&c->gold>=0,"%s: %s com tropas/ouro negativos",ctx,NATIONS[c->nat].tag);
  CHECK(c->cells>0,"%s: %s vivo sem celulas",ctx,NATIONS[c->nat].tag);
 }
}
static void TestRandom(void){
 printf("[mapa aleatorio + frentes]\n");
 World_GenRandom(&W1,1234,WS_PEQUENO,12,1,-1);
 Invariants(&W1,"gerado");
 u32 alive0=0;for(u8 i=1;i<W1.nC;i++)if(W1.c[i].alive)alive0++;
 CHECK(alive0==12,"12 paises vivos (%u)",alive0);
 SaveMap(&W1,"host/out/aleatorio_inicio.ppm",3);
 clock_t t0=clock();
 for(int d=0;d<3650;d++)World_Day(&W1);
 double ms=(double)(clock()-t0)*1000.0/CLOCKS_PER_SEC/3650.0;
 Invariants(&W1,"10 anos");
 u32 alive=0;for(u8 i=1;i<W1.nC;i++)if(W1.c[i].alive)alive++;
 printf("  10 anos: %u conquistas de celula, %u guerras, %u eventos, %u/%u vivos, %.3f ms/dia (PC)\n",
  W1.nConquests,W1.nWars,W1.nEvents,alive,alive0,ms);
 CHECK(W1.nConquests>500,"frentes andaram (%u celulas)",W1.nConquests);
 CHECK(W1.nEvents>50,"eventos aleatorios dispararam (%u)",W1.nEvents);
 SaveMap(&W1,"host/out/aleatorio_10anos.ppm",3);
 // determinismo: mesma seed = mesma historia
 World_GenRandom(&W2,1234,WS_PEQUENO,12,1,-1);
 for(int d=0;d<3650;d++)World_Day(&W2);
 u32 h2=World_Hash(&W2);
 World_GenRandom(&W1,1234,WS_PEQUENO,12,1,-1);
 for(int d=0;d<3650;d++)World_Day(&W1);
 CHECK(World_Hash(&W1)==h2,"replay deterministico");
 World_GenRandom(&W2,999,WS_PEQUENO,12,1,-1);
 for(int d=0;d<3650;d++)World_Day(&W2);
 CHECK(World_Hash(&W2)!=h2,"seeds diferentes divergem");
 // jogador: ataque manual com 50%
 World_GenBlitz(&W1,77,1,Nat_Find("JAP"));
 u8 me=World_CountryOfNat(&W1,Nat_Find("JAP"));
 CHECK(me&&W1.c[me].player,"jogador escolhido no Blitz");
 u32 before=W1.c[me].cells;
 CHECK(World_Attack(&W1,me,0,50),"ataque a terra sem dono");
 for(int d=0;d<20;d++)World_Day(&W1);
 CHECK(W1.c[me].cells>before,"jogador expandiu (%u -> %u)",before,W1.c[me].cells);
 Invariants(&W1,"blitz");
}
static void Snap(const World *w,int year){
 char p[64];sprintf(p,"host/out/mundi_%d.ppm",year);SaveMap(w,p,2);
}
static void TestMundi(void){
 printf("[mapa-mundi 1800 -> 2025]\n");
 int bra=Nat_Find("BRA");
 World_GenMundi(&W1,42,WH_LIGADA,1,-1);
 Invariants(&W1,"1800");
 u32 alive0=0,land=0,neutral=0,n=(u32)W1.w*W1.h;
 for(u8 i=1;i<W1.nC;i++)if(W1.c[i].alive)alive0++;
 for(u32 i=0;i<n;i++)if(W_TERR[i]!=WT_WATER){land++;if(!W_OWNER[i])neutral++;}
 printf("  1800: %u entidades no mapa, %u celulas de terra, %u sem dono (%u%%)\n",alive0,land,neutral,neutral*100/land);
 int start=0;for(int i=0;i<NATION_COUNT;i++)if(!NATIONS[i].yFrom)start++;
 CHECK(alive0>=(u32)start*9/10,"quase todas as nacoes de 1800 no mapa (%u de %d)",alive0,start);
 if(neutral*100>land*3)WARN("%u%% da terra sem dono em 1800 (DESIGN 4.6: nenhuma)",neutral*100/land);
 Snap(&W1,1800);
 {int t[CT_COUNT]={0};for(int i=0;i<W1.nCity;i++)t[W1.city[i].tier]++;
  printf("  1800: %u cidades (%d povoados, %d vilarejos, %d cidades, %d metropoles)\n",W1.nCity,t[0],t[1],t[2],t[3]);
  CHECK(W1.nCity>=150,"pelo menos 150 cidades em 1800 (%u)",W1.nCity);
  int lon=-1;for(int i=0;i<W1.nCity;i++){char nm[40];City_Name(&W1,i,nm,40);if(!strcmp(nm,"Londres"))lon=i;}
  CHECK(lon>=0&&W1.city[lon].tier==CT_METROPOLE,"Londres e metropole em 1800");}
 CHECK(World_AtWar(&W1,World_CountryOfNat(&W1,Nat_Find("FRA")),World_CountryOfNat(&W1,Nat_Find("GBR"))),"FRA x GBR em guerra em 1800");
 clock_t t0=clock();
 int snaps[]={1815,1850,1871,1900,1950,2000};int si=0;
 u32 days=0;
 // nascimento e morte das potencias (diagnostico de eventos historicos que falham)
 static const char *watch[]={"USA","MEX","GBR","FRA","PRU","RUS","AUT","OTT","BRA","QNG","JAP","ESP","POR","ARG"};
 enum{NW=sizeof watch/sizeof watch[0]};u8 wst[NW]={0}; // 0 nunca viu, 1 vivo, 2 morto
 while(!W1.ended){
  World_Day(&W1);days++;
  for(int k=0;k<NW;k++){int nt=Nat_Find(watch[k]);if(nt<0||wst[k]==2)continue;
   bool on=World_CountryOfNat(&W1,nt)!=0;
   if(on&&!wst[k]){wst[k]=1;if(days>1)printf("  nasce: %s em %d/%d/%d\n",watch[k],W1.date.d,W1.date.m,W1.date.y);}
   else if(!on&&wst[k]==1){wst[k]=2;
    // quem ficou com o nucleo: o herdeiro (evento) ou o conquistador
    u32 cnt[W_MAXC+1]={0},nn=(u32)W1.w*W1.h;u8 top=0;
    for(u32 i=0;i<nn;i++)if(W_CORE[i]==nt+1&&W_OWNER[i]){cnt[W_OWNER[i]]++;if(cnt[W_OWNER[i]]>cnt[top])top=W_OWNER[i];}
    printf("  morte: %s em %d/%d/%d -> nucleo com %s (%u) | msg: %s\n",watch[k],W1.date.d,W1.date.m,W1.date.y,
     top?NATIONS[W1.c[top].nat].tag:"-",cnt[top],W1.msg);}}
  if(si<6&&W1.date.y==snaps[si]&&W1.date.m==1&&W1.date.d==1){Snap(&W1,snaps[si]);si++;
   if(snaps[si-1]==1850)Invariants(&W1,"1850");}
 }
 double ms=(double)(clock()-t0)*1000.0/CLOCKS_PER_SEC/days;
 CHECK(W1.date.y==2025&&W1.date.m==12&&W1.date.d==31,"terminou em 31/12/2025 (%d/%d/%d)",W1.date.d,W1.date.m,W1.date.y);
 Invariants(&W1,"2025");
 Snap(&W1,2025);
 int fired=0,failed=0,pend=0;
 for(int i=0;i<Hist_Count();i++){u8 s=Hist_State(&W1,i);if(s==1)fired++;else if(s==2)failed++;else pend++;}
 u32 alive=0;for(u8 i=1;i<W1.nC;i++)if(W1.c[i].alive)alive++;
 printf("  2025: %u dias, %.3f ms/dia (PC); historicos %d disparados, %d falharam, %d pendentes\n",days,ms,fired,failed,pend);
 printf("  %u paises vivos, %u guerras, %u conquistas de celula, %u eventos\n",alive,W1.nWars,W1.nConquests,W1.nEvents);
 {int t[CT_COUNT]={0},b=0;for(int i=0;i<W1.nCity;i++){t[W1.city[i].tier]++;for(int k=0;k<CITY_SLOTS;k++)if(W1.city[i].bld[k])b++;}
  printf("  2025: %u cidades (%d/%d/%d/%d por nivel), %d obras\n",W1.nCity,t[0],t[1],t[2],t[3],b);
  CHECK(t[CT_METROPOLE]>40,"cidades cresceram ate 2025 (%d metropoles)",t[CT_METROPOLE]);}
 printf("  ultima mensagem: %s\n",W1.msg);
 CHECK(fired>=Hist_Count()/3,"pelo menos 1/3 dos eventos historicos aconteceu (%d)",fired);
 // quem falhou (historia divergiu) — informativo
 printf("  divergencias:");int shown=0;
 for(int i=0;i<Hist_Count()&&shown<25;i++)if(Hist_State(&W1,i)==2){printf(" %s",Hist_Get(i)->id);shown++;}
 printf("%s\n",shown==25?" ...":"");
 // ranking
 printf("  top 8 em 2025:");
 bool picked[W_MAXC+1]={false};
 for(int k=0;k<8;k++){s32 best=-1;int bi=0;
  for(u8 i=1;i<W1.nC;i++){if(!W1.c[i].alive||picked[i])continue;s32 s=World_Score(&W1,i);
   if(s>best){best=s;bi=i;}}
  if(!bi)break;
  picked[bi]=true;printf(" %s(%d)",NATIONS[W1.c[bi].nat].tag,(int)best);}
 printf("\n");
 // Modo Eterno
 World_Day(&W1);
 CHECK(W1.date.y==2025,"parado em 2025 ate o jogador decidir");
 World_Continue(&W1);
 for(int d=0;d<400;d++)World_Day(&W1);
 CHECK(W1.date.y==2027&&!W1.ended&&Cal_Era(W1.date.y)==ERA_ETERNA,"Modo Eterno segue o calendario");
 (void)bra;
 // determinismo do mapa-mundi (1 ano)
 // (as grades sao globais: um World por vez; o hash e tirado antes de gerar outro)
 World_GenMundi(&W1,42,WH_LIGADA,1,-1);for(int d=0;d<365;d++)World_Day(&W1);
 u32 h1=World_Hash(&W1);
 World_GenMundi(&W1,42,WH_LIGADA,1,-1);for(int d=0;d<365;d++)World_Day(&W1);
 CHECK(World_Hash(&W1)==h1,"replay deterministico do mapa-mundi");
}

int main(void){
 printf("sizeof(World)=%u bytes, grades=%u bytes\n",(unsigned)sizeof(World),(unsigned)(sizeof(W_OWNER)+sizeof(W_TERR)));
 TestTxt();
 TestCalendar();
 TestNations();
 TestHistoryData();
 SaveFlags("host/out/bandeiras.ppm");
 TestRandom();
 TestMundi();
 printf("\n%s: %d falha(s), %d aviso(s)\n",fails?"FALHA":"OK",fails,warns);
 return fails?1:0;
}
