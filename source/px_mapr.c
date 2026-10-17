#include "wgame.h"
#include "px.h"
#include "pal.h"
#include "hot.h"
// px_mapr.c — camada do mapa-mundi: pintura suave + cache incremental
// (DESIGN.md sec 4.3, SDD ADR-24 e ADR-36). Por que assim no DS (67MHz, 4MB):
//
// 1) PINTURA SUAVE: cada pixel interpola as 4 celulas mais proximas
//    (bilinear) depois de um "domain warp" leve por uma tabela de ruido
//    fixa. Costa e fronteira saem curvas e organicas, nao em degraus. Relevo
//    por hillshade do campo W_LIM (luz de noroeste); mar com profundidade
//    (raso -> fundo) e banda de espuma; textura de arvores/areia por hash.
//    Tudo em inteiro, com pontilhado Bayer 4x4 p/ gradiente liso em 15 bits.
//    CUSTO (ADR-36, medido no ARM9 emulado): o pixel roda em ARM de 32 bits
//    na ITCM (hot.h), sem NENHUMA chamada de divisao (o ARM9 nao tem
//    divisor; antes eram ~20 por pixel em Thumb): potencia de 2 = shift;
//    constante = tabela ou smull (ARM); /(3*ppc) = reciproco exato. As 4
//    amostras ficam em registrador: pesos empacotados num u32 (SWAR, 1 byte
//    cada), terra/agua numa mascara de 4 bits. Resultado bit a bit IGUAL ao
//    da pintura antiga (o smoke compara).
// 2) CACHE EM ANEL (ADR-36): LAYER/KEY sao um toro 512x256 indexado pelo
//    pixel do MUNDO (coluna = (ox+sx)&511, linha = (oy+sy)&255). Andar com a
//    camera = somar o deslocamento em ox/oy (O(1)) e pintar so' o que entrou
//    no RETANGULO VALIDO + a antiga borda dele. Antes o cache era
//    deslocado com memmove (132KB/quadro; com passo impar a newlib copiava
//    BYTE A BYTE). Bonus: o pontilhado e a moldura "fora do mapa" ficam
//    presos ao mundo, entao nao "nadam" nem fazem costura no pan.
//    Troca de zoom/modo/paleta = tela toda; conquista = so' os blocos 8x8
//    marcados em World.dirtyBlk (0-2% da tela num dia comum).
// 2b) RENDER-AHEAD (ADR-36): o retangulo valido [VX0,VX1)x[VY0,VY1) (em
//    coordenadas da vista) contem a vista e pode ter margem de ate' MH px de
//    cada lado e MV em cima/baixo. Nos quadros com folga PxM_Ahead pinta a
//    margem a' frente da camera; o pan so' paga pintura quando sai dela.
// 3) OVERLAY: frente piscando e contorno do pais sob o cursor saem do KEY
//    (dono por pixel) a cada desenho, sem repintar o cache. Varredura SWAR:
//    4 pixels por palavra; regiao uniforme (quase tudo) pula em 1 comparacao.
enum { RW=512, RWS=9, RH=256, MH=(RW-256)/2, MV=(RH-WG_MAP_H)/2 };
static u16 LAYER[RW*RH] __attribute__((aligned(32)));
static u8 KEY[RW*RH] __attribute__((aligned(32)));
enum { K_OUT=0xFE, K_SEA=0xFF };
static struct { bool ok; s16 camX,camY; u8 subX,subY,zoom,mode,me,oy; u16 ox; s8 dirX,dirY; u32 sig; } CS;
static int VX0,VX1=256,VY0,VY1=WG_MAP_H;   // retangulo valido do anel (coordenadas da vista)
// ESPELHO EMPACOTADO das grades (so' o render le): 1 palavra por celula =
// terreno | dono<<8 | relevo(W_LIM)<<16 | terra<<24. A amostra bilinear le 1
// palavra em vez de 3 bytes de 3 grades (3 linhas de cache), e os indices
// morrem logo apos a carga (menos spill no laco). Sincronia: mundo novo
// (PxM_Invalidate, chamado por WG_New) = tudo; em jogo o dono so' muda por
// SetOwner (world.c), que marca World.dirtyBlk = reempacota esses blocos.
static u32 PK[W_CELLS];
static bool pkAll=true;
void PxM_Invalidate(void){CS.ok=false;pkAll=true;}
static inline u32 Pack1(u32 i){return (u32)W_TERR[i]|((u32)W_OWNER[i]<<8)|((u32)W_LIM[i]<<16)|((u32)(W_TERR[i]!=WT_WATER)<<24);}
static void PackBlock(const World *w,int b){
 int x0=(b%W_BLK_W)*8,y0=(b/W_BLK_W)*8;
 for(int y=y0;y<y0+8&&y<w->h;y++)for(int x=x0;x<x0+8&&x<w->w;x++){u32 i=(u32)y*w->w+x;PK[i]=Pack1(i);}
}
u16 *PxM_Scratch(void){CS.ok=false;return LAYER;}   // a cena da cidade reusa o anel (256KB; usa 88KB lineares)
u8 PxM_KeyAt(int sx,int sy){return KEY[(((CS.oy+sy)&(RH-1))<<RWS)|((CS.ox+sx)&(RW-1))];}

static const u8 BAYER[4][4]={{0,4,1,5},{6,2,7,3},{1,5,0,4},{7,3,6,2}};
// ruido de valor suave 64x64 (gerado 1x, hash fixo: nao usa o RNG do jogo)
static s8 NZ[64][64];static bool nzOk;
static s8 ELS[256];                 // (el-80)/12: relevo alto mais claro (exato, sem divisao)
static void NoiseInit(void){
 static s16 t[64][64];
 for(int y=0;y<64;y++)for(int x=0;x<64;x++){u32 h=(u32)x*374761393u+(u32)y*668265263u;h=(h^(h>>13))*1274126177u;t[y][x]=(s16)((h>>24)&255)-128;}
 for(int pass=0;pass<3;pass++){
  static s16 u[64][64];
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){
   int s=0;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)s+=t[(y+dy)&63][(x+dx)&63];
   u[y][x]=(s16)(s/9);
  }
  for(int y=0;y<64;y++)for(int x=0;x<64;x++)t[y][x]=u[y][x];
 }
 int mx=1;for(int y=0;y<64;y++)for(int x=0;x<64;x++){int a=t[y][x]<0?-t[y][x]:t[y][x];if(a>mx)mx=a;}
 for(int y=0;y<64;y++)for(int x=0;x<64;x++)NZ[y][x]=(s8)(t[y][x]*32/mx);
 for(int e=0;e<256;e++)ELS[e]=(s8)((e-80)/12);
 nzOk=true;
}

// paleta por dono e por terreno, ja' multiplicada p/ a mistura 78/22:
// cor = (CR78[dono]+TER22[terreno])/100 (constante: smull no ARM)
static u16 CR78[W_MAXC+1][3];
static u8 FLG[W_MAXC+1];            // 1 jogador, 4 povo (hachura)
static const u8 TER8[WT_COUNT][3]={
 {0,0,0},{196,184,128},{120,150,92},{176,156,112},{150,140,128},{224,200,136},
 {130,150,112},{90,140,72},{180,184,170},{200,184,118},{232,238,244}};
static u16 TER22[WT_COUNT][3];
static void Palette(const World *w,const WGame *s){
 for(int t=0;t<WT_COUNT;t++)for(int k=0;k<3;k++)TER22[t][k]=(u16)(TER8[t][k]*22);
 for(u8 ci=1;ci<w->nC;ci++){
  const WCountry *c=&w->c[ci];int r,g,b;
  if(s->mapMode==WMAP_DIP){
   const WCountry *me=&w->c[s->me];
   if(ci==s->me){r=9;g=25;b=13;}
   else if(World_AtWar(w,s->me,ci)){r=27;g=8;b=6;}
   else if(me->ally[0]==ci||me->ally[1]==ci){r=6;g=20;b=26;}          // aliado: azul-petroleo
   else if(c->overlord==s->me){r=16;g=27;b=17;}
   else if(me->overlord==ci){r=8;g=17;b=28;}
   else if(me->bloc&&c->bloc==me->bloc){r=14;g=24;b=26;}               // mesmo mercado comum
   else if(me->rival==ci||c->rival==s->me){r=28;g=17;b=6;}           // rival: laranja
   else{int k=(ci*7)%3;r=21+k;g=20+k;b=16+k;}
  }else if(s->mapMode==WMAP_IDEO){
   static const u8 IC[ID_COUNT][3]={{24,19,8},{9,14,27},{8,19,22},{27,5,4},{9,9,11},{12,22,6}};
   const u8 *q=IC[c->ideo%ID_COUNT];r=q[0];g=q[1];b=q[2];
  }else{const NationDef *d=&NATIONS[c->nat];r=d->r;g=d->g;b=d->b;
   if(s->mapMode==WMAP_COM){int gr=(r+g+b)/3;r=(r+gr*2)/3+3;g=(g+gr*2)/3+3;b=(b+gr*2)/3+2;   // pastel: as rotas aparecem
    if(r>31)r=31;
    if(g>31)g=31;
    if(b>31)b=31;}
  }
  CR78[ci][0]=(u16)(r*8*78);CR78[ci][1]=(u16)(g*8*78);CR78[ci][2]=(u16)(b*8*78);
  FLG[ci]=(u8)((ci==s->me?1:0)|((c->kind==NK_POVO||c->kind==NK_AUTONOMO)?4:0));
 }
}
static u32 Sig(const World *w,const WGame *s){
 u32 h=2166136261u^s->mapMode^((u32)s->me<<8);
 for(u8 ci=1;ci<w->nC;ci++){
  h^=w->c[ci].nat|((u32)w->c[ci].alive<<8);h*=16777619u;
  if(s->mapMode==WMAP_DIP){h^=(u32)World_AtWar(w,s->me,ci)|((u32)w->c[ci].overlord<<1)|((u32)w->c[ci].bloc<<9)|((u32)w->c[ci].rival<<17);h*=16777619u;
   h^=(u32)w->c[ci].ally[0]|((u32)w->c[ci].ally[1]<<8);h*=16777619u;}
  if(s->mapMode==WMAP_IDEO){h^=w->c[ci].ideo;h*=16777619u;}
 }
 return h;
}

// ---- contexto do quadro (PxM_Update): 1 registrador-base no laco quente ----
typedef struct { int sh,W,H,mode,rc3,wrap; } PxCtx;
static PxCtx X;
static int gOx,gOy;                 // origem do anel (= CS.ox/oy)
static s8 gWarp[65];                // NZ*ppc/80 por zoom (exato), indice NZ+32
static int warpSh=-1;
static void ZoomTables(int sh){
 X.sh=sh;
 if(warpSh==sh)return;
 int ppc=1<<sh;
 for(int n=-32;n<=32;n++)gWarp[n+32]=(s8)(ppc>=2?n*ppc/80:0);
 // ceil(65536/(3*ppc)): (v*rc3)>>16 == v/(3*ppc) EXATO p/ |v| <= 510*ppc
 static const u16 RC3[4]={21846,10923,5462,2731};
 X.rc3=RC3[sh&3];warpSh=sh;
}
// byte k = 0xFF se a amostra k e' terra (mascara de 4 bits -> 4 bytes)
static const u32 LMASK[16]={0,0xFF,0xFF00,0xFFFF,0xFF0000,0xFF00FF,0xFFFF00,0xFFFFFF,
 0xFF000000,0xFF0000FF,0xFF00FF00,0xFF00FFFF,0xFFFF0000,0xFFFF00FF,0xFFFFFF00,0xFFFFFFFF};

HOT_BEGIN
static inline int Cl8(int v){return v<0?0:(v>255?255:v);}
// pontilhado preso ao ANEL (= mundo): (x,y) e' a coluna/linha do cache
static inline u16 Dith(int r,int g,int b,int x,int y){
 int t=BAYER[y&3][x&3];
 r=Cl8(r+t);g=Cl8(g+t);b=Cl8(b+t);
 return PXC(r>>3,g>>3,b>>3);
}
// ---- pintura de 1 pixel do mundo (gx,gy em pixels do zoom atual) ----
// Devolve dono<<16 | cor num registrador (sem ponteiro de saida na pilha).
HOT_ITCM_IN static u32 Pix(int gx,int gy,int bx,int by){
 const PxCtx *x=&X;
 const int sh=x->sh,ppc=1<<sh,msk=ppc-1;
 int px=gx-(ppc>>1),py=gy-(ppc>>1);
 // domain warp (Z2+): a fronteira serpenteia em vez de seguir a grade
 if(sh){
  int q=sh-1;
  px+=gWarp[NZ[(gy>>q)&63][(gx>>q)&63]+32];
  py+=gWarp[NZ[((gx>>q)+29)&63][((gy>>q)+17)&63]+32];
 }
 const int fx=px&msk,fy=py&msk;
 const int cx=px>>sh,cy=py>>sh,W=x->W,H=x->H;
 // as 4 celulas, 1 palavra cada (PK). Miolo do mapa: 4 cargas seguidas;
 // borda de cima/baixo prende, volta do mundo enrola (caminho raro).
 u32 c0,c1,c2,c3;
 if((unsigned)cx<(unsigned)(W-1)&&(unsigned)cy<(unsigned)(H-1)){
  const u32 *p=PK+(u32)cy*W+cx;c0=p[0];c1=p[1];c2=p[W];c3=p[W+1];
 }else{
  int y0=cy,y1=cy+1;
  if(y0<0)y0=0;else if(y0>=H)y0=H-1;
  if(y1<0)y1=0;else if(y1>=H)y1=H-1;
  int x0=cx,x1=cx+1;
  if((unsigned)x0>=(unsigned)(W-1)){
   if(x->wrap){while(x0<0)x0+=W;while(x0>=W)x0-=W;x1=x0+1;if(x1>=W)x1-=W;}
   else{x0=x0<0?0:(x0>=W?W-1:x0);x1=x1<0?0:(x1>=W?W-1:x1);}
  }
  c0=PK[(u32)y0*W+x0];c1=PK[(u32)y0*W+x1];c2=PK[(u32)y1*W+x0];c3=PK[(u32)y1*W+x1];
 }
 const u32 lm=(c0>>24)|((c1>>24)<<1)|((c2>>24)<<2)|((c3>>24)<<3);
 // pesos bilineares (<=64 cada, somam ppc^2): 1 byte por amostra
 const int ifx=ppc-fx,ify=ppc-fy;
 const u32 wp=(u32)(ifx*ify)|((u32)(fx*ify)<<8)|((u32)(ifx*fy)<<16)|((u32)(fx*fy)<<24);
 const u32 wl=wp&LMASK[lm];                         // so' os pesos de terra
 const int landW=(int)((wl*0x01010101u)>>24);       // soma dos 4 bytes (sem vai-um: <=64)
 const int tot=1<<(sh*2);
 int r,g,b;u32 key;
 if(landW*2>tot){
  // dono: maior peso somado entre as amostras de terra (empate: o 1o a aparecer)
  const u32 o0=(c0>>8)&255,o1=(c1>>8)&255,o2=(c2>>8)&255,o3=(c3>>8)&255;
  if(lm==15&&o0==o1&&o0==o2&&o0==o3)key=o0;        // miolo de pais: sem votacao
  else{
   const int w0=wl&255,w1=(wl>>8)&255,w2=(wl>>16)&255,w3=wl>>24;
   int bs=-1;key=0;
   if(lm&1){bs=w0+(o1==o0?w1:0)+(o2==o0?w2:0)+(o3==o0?w3:0);key=o0;}
   if(lm&2){int s=w1+(o0==o1?w0:0)+(o2==o1?w2:0)+(o3==o1?w3:0);if(s>bs){bs=s;key=o1;}}
   if(lm&4){int s=w2+(o0==o2?w0:0)+(o1==o2?w1:0)+(o3==o2?w3:0);if(s>bs){bs=s;key=o2;}}
   if(lm&8){int s=w3+(o0==o3?w0:0)+(o1==o3?w1:0)+(o2==o3?w2:0);if(s>bs)key=o3;}
  }
  // terreno: a amostra de terra de maior peso (empate: a 1a)
  u32 cb=c0;int bw=-1;
  if(lm&1)bw=wl&255;
  if((lm&2)&&(int)((wl>>8)&255)>bw){bw=(wl>>8)&255;cb=c1;}
  if((lm&4)&&(int)((wl>>16)&255)>bw){bw=(wl>>16)&255;cb=c2;}
  if((lm&8)&&(int)(wl>>24)>bw)cb=c3;
  const int t=(int)(cb&255);
  if(key&&x->mode!=WMAP_TER){
   // cor do pais (78%) sobre o terreno (22%): papel colorido com relevo
   r=(int)((u32)(CR78[key][0]+TER22[t][0])/100u);g=(int)((u32)(CR78[key][1]+TER22[t][1])/100u);b=(int)((u32)(CR78[key][2]+TER22[t][2])/100u);
  }else{r=TER8[t][0];g=TER8[t][1];b=TER8[t][2];}
  // hillshade: derivada do relevo (mar = 0), luz de noroeste
  const int e0=(lm&1)?(int)((c0>>16)&255):0,e1=(lm&2)?(int)((c1>>16)&255):0,e2=(lm&4)?(int)((c2>>16)&255):0,e3=(lm&8)?(int)((c3>>16)&255):0;
  const int v=(e1-e0)*ify+(e3-e2)*fy+(e2-e0)*ifx+(e3-e1)*fx;
  int sh2=v>=0?(int)(((u32)v*(u32)x->rc3)>>16):-(int)(((u32)(-v)*(u32)x->rc3)>>16);   // v/(3*ppc), truncado
  if(sh2>36)sh2=36;
  if(sh2<-36)sh2=-36;
  sh2+=ELS[(e0*(int)(wp&255)+e1*(int)((wp>>8)&255)+e2*(int)((wp>>16)&255)+e3*(int)(wp>>24))>>(sh*2)];   // terra alta mais clara
  r+=sh2;g+=sh2;b+=sh2;
  // praia: faixa clara onde a terra e' quase mar (landW < 60%); x/3 = x*21846>>16 (exato aqui)
  if(sh&&landW*5<tot*3){r=((r*2+232)*21846)>>16;g=((g*2+214)*21846)>>16;b=((b*2+160)*21846)>>16;}
  // textura: copa de arvore / areia / neve por hash do pixel do mundo
  if(sh>=2){
   u32 h=(u32)gx*2246822519u^(u32)gy*3266489917u;h^=h>>15;
   if((t==WT_FOREST||t==WT_JUNGLE)&&(h&7)<3){r-=22;g-=18;b-=20;}
   else if(t==WT_DESERT&&(h&15)==0){r+=14;g+=12;b+=8;}
   else if(t==WT_MOUNTAIN&&(h&15)<2){r+=24;g+=24;b+=26;}
  }
  if(key&&(FLG[key]&4)&&((gx+gy)&3)==0){r-=18;g-=18;b-=18;}   // povo autonomo
 }else{
  key=K_SEA;
  // profundidade interpolada (terra conta como 0): raso turquesa -> fundo
  const u32 ws=wp&~LMASK[lm];
  const int d=((int)((c0>>16)&255)*(int)(ws&255)+(int)((c1>>16)&255)*(int)((ws>>8)&255)+(int)((c2>>16)&255)*(int)((ws>>16)&255)+(int)((c3>>16)&255)*(int)(ws>>24))>>(sh*2);
  int t=(d*6554)>>8;if(t>256)t=256;      // d*256/10 (exato p/ d<=31)
  r=(112*(256-t)+30*t)>>8;g=(186*(256-t)+78*t)>>8;b=(198*(256-t)+142*t)>>8;
  // espuma: perto da linha da costa (landW > 30%)
  if(sh&&landW*10>tot*3){r=(r+220)>>1;g=(g+236)>>1;b=(b+232)>>1;}
  else if(!sh&&d<=1){r+=40;g+=36;b+=24;}
  if(sh>=2){u32 h=(u32)gx*2654435761u^(u32)gy*40503u;h^=h>>13;
   if((h&63)==0){r+=24;g+=24;b+=20;}}             // brilho de onda
 }
 return (key<<16)|Dith(r,g,b,bx,by);
}
// Z0: 1 px = 2x2 celulas; media das 4 cores (antisserrilhado), dono = maioria
HOT_ITCM_IN static u32 PixZ0(int cx,int cy,int bx,int by){
 int r=0,g=0,b=0;u8 k0=K_SEA,keys[4];
 for(int k=0;k<4;k++){
  u32 v=Pix(cx+(k&1),cy+(k>>1),bx,by);u16 c=(u16)v;keys[k]=(u8)(v>>16);
  r+=(c&31)<<3;g+=((c>>5)&31)<<3;b+=((c>>10)&31)<<3;
 }
 for(int k=0;k<4;k++)if(keys[k]!=K_SEA){int n=0;for(int j=0;j<4;j++)n+=keys[j]==keys[k];if(n>=2||k0==K_SEA)k0=keys[k];}
 return ((u32)k0<<16)|Dith(r>>2,g>>2,b>>2,bx,by);
}
static inline u32 Out(int bx,int by){return ((u32)K_OUT<<16)|(((bx+by)&7)==0?C_NAVY:C_NAVY_BG);}
// passo 1: KEY+cor base no retangulo [x0,x1)x[y0,y1) (coordenadas da VISTA:
// pode sair dela, nas margens do render-ahead); a cor so' e' gravada dentro
// de [cx0,cx1)x[cy0,cy1) (fora disso o KEY e' so' vizinhanca)
HOT_ITCM static void Paint(const World *w,const WGame *s,int x0,int y0,int x1,int y1,int cx0,int cy0,int cx1,int cy1){
 const bool z0=s->zoom==0,wrap=w->wrap;
 const int ppc=1<<X.sh,gx0=s->camX*ppc+s->subX,gxMax=w->w*ppc;
 for(int sy=y0;sy<y1;sy++){
  const int by=(gOy+sy)&(RH-1);
  u16 *L=LAYER+(by<<RWS);u8 *K=KEY+(by<<RWS);
  const int cyz=s->camY+sy*2,gy=s->camY*ppc+s->subY+sy;
  const bool outY=z0?(cyz<0||cyz>=w->h):(gy<0||gy>=w->h*ppc);
  const bool inB=sy>=cy0&&sy<cy1;
  const int c0=inB?cx0:x1,c1=inB?cx1:x0;           // colunas que gravam cor
  int bx=(gOx+x0)&(RW-1);
  for(int sx=x0;sx<x1;sx++,bx=(bx+1)&(RW-1)){
   u32 v;
   if(outY)v=Out(bx,by);
   else if(z0){int cxz=s->camX+sx*2;v=(!wrap&&(cxz<0||cxz>=w->w))?Out(bx,by):PixZ0(cxz,cyz,bx,by);}
   else{int gx=gx0+sx;v=(!wrap&&(gx<0||gx>=gxMax))?Out(bx,by):Pix(gx,gy,bx,by);}
   K[bx]=(u8)(v>>16);
   if(sx>=c0&&sx<c1)L[bx]=(u16)v;
  }
 }
}
// passo 2: fronteiras. Pixel com vizinho de outro dono escurece (2px no
// total, o brilho do Victoria 3); seu reino em ouro; costa com fio de tinta.
// Vizinho FORA do retangulo valido conta como "mesmo dono" (quem estende o
// retangulo repinta essa coluna/linha). Retangulo = vista: igual a antes.
HOT_ITCM static void Edge(const WGame *s,int x0,int y0,int x1,int y1){
 const bool ter=s->mapMode==WMAP_TER;
 const int vx0=VX0,vx1=VX1;
 for(int sy=y0;sy<y1;sy++){
  const int by=(gOy+sy)&(RH-1);
  u16 *L=LAYER+(by<<RWS);const u8 *K=KEY+(by<<RWS);
  const u8 *KU=KEY+((sy>VY0?((by-1)&(RH-1)):by)<<RWS),*KD=KEY+((sy<VY1-1?((by+1)&(RH-1)):by)<<RWS);
  int bx=(gOx+x0)&(RW-1);
  for(int sx=x0;sx<x1;sx++,bx=(bx+1)&(RW-1)){
   const u8 m=K[bx];
   if(m>=K_OUT)continue;
   const u8 n0=sx>vx0?K[(bx-1)&(RW-1)]:m,n1=sx<vx1-1?K[(bx+1)&(RW-1)]:m,n2=KU[bx],n3=KD[bx];
   if(n0==m&&n1==m&&n2==m&&n3==m)continue;          // miolo: o caso comum
   bool coast=n0==K_SEA||n1==K_SEA||n2==K_SEA||n3==K_SEA;
   bool border=(n0!=m&&n0<K_OUT)||(n1!=m&&n1<K_OUT)||(n2!=m&&n2<K_OUT)||(n3!=m&&n3<K_OUT);
   if(!coast&&!border)continue;
   u16 c=L[bx];int r=(c&31)<<3,g=((c>>5)&31)<<3,b=((c>>10)&31)<<3;
   if(border&&m&&(FLG[m]&1)&&!ter)L[bx]=C_GOLD;
   else if(border)L[bx]=Dith((r*9)>>4,(g*9)>>4,(b*9)>>4,bx,by);
   else L[bx]=Dith((r*3)>>2,(g*3)>>2,(b*3)>>2,bx,by);
  }
 }
}
// INDICE DE ARESTAS do anel (ADR-36): bit w de EB[linha] = a palavra w (4 px)
// tem par vizinho (direita/baixo NO ANEL) entre dois donos de terra
// diferentes. O overlay so' visita essas palavras (~5-10% da vista) em vez
// das 11 mil a cada quadro. Vizinho no anel inclui o da tela, entao bit 0 =
// garantidamente nenhuma aresta na tela (bit 1 sobrando so' custa a visita).
static u32 EB[RH][RW/128];
static inline u32 WordEdge(const u8 *K,const u8 *KD,int bw){
 u32 v=*(const u32a*)(K+bw),nb=K[(bw+4)&(RW-1)];
 if(v==((v>>8)|(nb<<24))&&v==*(const u32a*)(KD+bw))return 0;   // SWAR: uniforme
 for(int i=0;i<4;i++){
  u8 a=K[bw+i];if(a>=K_OUT)continue;
  u8 r=K[(bw+i+1)&(RW-1)],d=KD[bw+i];
  if((r!=a&&r<K_OUT)||(d!=a&&d<K_OUT))return 1;
 }
 return 0;
}
// recalcula EB p/ o retangulo (da vista) cujo KEY mudou: + 1 coluna a
// esquerda e 1 linha acima (o vizinho direito/de baixo delas mudou)
HOT_ITCM static void EBRect(int x0,int y0,int x1,int y1){
 if(x0>=x1||y0>=y1)return;
 enum { NW=RW/4 };
 int wa,nw;
 if(x1-x0>=RW-4){wa=0;nw=NW;}
 else{wa=((gOx+x0-1)&(RW-1))>>2;int wb=((gOx+x1-1)&(RW-1))>>2;nw=((wb-wa)&(NW-1))+1;}
 for(int sy=y0-1;sy<y1;sy++){
  int by=(gOy+sy)&(RH-1);
  const u8 *K=KEY+(by<<RWS),*KD=KEY+(((by+1)&(RH-1))<<RWS);
  u32 *e=EB[by];
  for(int k=0,wi=wa;k<nw;k++,wi=(wi+1)&(NW-1)){
   u32 bit=1u<<(wi&31);
   if(WordEdge(K,KD,wi<<2))e[wi>>5]|=bit;else e[wi>>5]&=~bit;
  }
 }
}
HOT_END

// retangulo autossuficiente (conquista, tela toda): KEY com 2px de margem,
// cor+fronteira com 1px (a fronteira do vizinho tambem muda); tudo recortado
// no retangulo valido
static void Render(const World *w,const WGame *s,int x0,int y0,int x1,int y1){
 int ax0=x0-2,ay0=y0-2,ax1=x1+2,ay1=y1+2;
 if(ax0<VX0)ax0=VX0;
 if(ay0<VY0)ay0=VY0;
 if(ax1>VX1)ax1=VX1;
 if(ay1>VY1)ay1=VY1;
 int bx0=x0-1,by0=y0-1,bx1=x1+1,by1=y1+1;
 if(bx0<VX0)bx0=VX0;
 if(by0<VY0)by0=VY0;
 if(bx1>VX1)bx1=VX1;
 if(by1>VY1)by1=VY1;
 if(ax0>=ax1||ay0>=ay1)return;
 Paint(w,s,ax0,ay0,ax1,ay1,bx0,by0,bx1,by1);
 Edge(s,bx0,by0,bx1,by1);
 EBRect(ax0,ay0,ax1,ay1);
}
// estende o retangulo valido ate' 'to' num lado: pinta o que entra + a antiga
// borda daquele lado (pintada sem o vizinho de fora). Se passar da largura
// (ou altura) do anel, o lado oposto encolhe ANTES de pintar (as colunas que
// saem sao as mesmas posicoes do anel que entram).
enum { E_LEFT=0, E_RIGHT, E_UP, E_DOWN };
static void Extend(const World *w,const WGame *s,int side,int to){
 int x0,x1,y0,y1;
 switch(side){
  case E_LEFT: if(to>=VX0)return;x0=to;x1=VX0+1;VX0=to;if(VX1-VX0>RW)VX1=VX0+RW;if(x1>VX1)x1=VX1;y0=VY0;y1=VY1;break;
  case E_RIGHT:if(to<=VX1)return;x0=VX1-1;x1=to;VX1=to;if(VX1-VX0>RW)VX0=VX1-RW;if(x0<VX0)x0=VX0;y0=VY0;y1=VY1;break;
  case E_UP:   if(to>=VY0)return;y0=to;y1=VY0+1;VY0=to;if(VY1-VY0>RH)VY1=VY0+RH;if(y1>VY1)y1=VY1;x0=VX0;x1=VX1;break;
  default:     if(to<=VY1)return;y0=VY1-1;y1=to;VY1=to;if(VY1-VY0>RH)VY0=VY1-RH;if(y0<VY0)y0=VY0;x0=VX0;x1=VX1;break;
 }
 Paint(w,s,x0,y0,x1,y1,x0,y0,x1,y1);
 Edge(s,x0,y0,x1,y1);
 EBRect(x0,y0,x1,y1);
}
static int RelX(const World *w,int a,int b){
 int d=b-a;
 // entradas em [0,w): |d|<w e o % (divisao em software no Thumb) e' inutil
 if(w->wrap){if(d<=-w->w||d>=w->w)d%=w->w;if(d<-w->w/2)d+=w->w;else if(d>=w->w/2)d-=w->w;}
 return d;
}
void PxM_Update(World *w,const WGame *s){
 if(!nzOk)NoiseInit();
 // o LAYER e' emprestado pela cena da cidade (PxM_Scratch): se o mapa o
 // repinta, o fundo da cena em cache deixou de existir. Sem isso, voltar a'
 // mesma cidade mostrava o mapa com os pedestres da cena por cima.
 PxC_SceneReset();
 int sh=s->zoom?s->zoom-1:0,ppc=1<<sh;
 ZoomTables(sh);X.W=w->w;X.H=w->h;X.mode=s->mapMode;X.wrap=w->wrap;
 u32 sig=Sig(w,s);
 Palette(w,s);
 // espelho PK em dia ANTES de pintar (faixas do pan tambem leem dele)
 if(pkAll){u32 n=(u32)w->w*w->h;for(u32 i=0;i<n;i++)PK[i]=Pack1(i);pkAll=false;}
 for(int b=0;b<W_BLK_W*W_BLK_H;b++){
  if(!w->dirtyBlk[b>>3]){b|=7;continue;}
  if((w->dirtyBlk[b>>3]>>(b&7))&1)PackBlock(w,b);
 }
 bool full=!CS.ok||CS.zoom!=s->zoom||CS.mode!=s->mapMode||CS.me!=s->me||CS.sig!=sig;
 int ppcPan=s->zoom?ppc:1;
 int dx=RelX(w,CS.camX,s->camX)*ppcPan,dy=(s->camY-CS.camY)*ppcPan;
 if(s->zoom==0){dx/=2;dy/=2;}
 else{dx+=s->subX-CS.subX;dy+=s->subY-CS.subY;}     // rolagem pixel a pixel (ADR-31)
 if(!full&&(dx>=200||dx<=-200||dy>=140||dy<=-140))full=true;
 if(full){
  // origem do anel = pixel do mundo no canto da vista (pontilhado preso ao
  // mundo); retangulo valido = a vista (as margens vem do PxM_Ahead)
  int X0=s->zoom?s->camX*ppc+s->subX:s->camX>>1,Y0=s->zoom?s->camY*ppc+s->subY:s->camY>>1;
  CS.ox=(u16)(X0&(RW-1));CS.oy=(u8)Y0;gOx=CS.ox;gOy=CS.oy;
  VX0=0;VX1=256;VY0=0;VY1=WG_MAP_H;
  Render(w,s,0,0,256,WG_MAP_H);
 }else{
  if(dx||dy){
   // anda o anel (O(1)); o retangulo valido anda junto (coordenadas da vista).
   // So' pinta se a vista saiu dele: colunas primeiro (com as linhas validas
   // reduzidas as da vista: a faixa custa a altura dela), depois linhas.
   CS.ox=(u16)((CS.ox+dx)&(RW-1));CS.oy=(u8)(CS.oy+dy);gOx=CS.ox;gOy=CS.oy;
   VX0-=dx;VX1-=dx;VY0-=dy;VY1-=dy;
   if(dx)CS.dirX=(s8)(dx>0?1:-1);
   if(dy)CS.dirY=(s8)(dy>0?1:-1);
   if(VX0>0||VX1<256){
    if(VY0<0)VY0=0;
    if(VY1>WG_MAP_H)VY1=WG_MAP_H;
    if(VX0>0)Extend(w,s,E_LEFT,0);
    if(VX1<256)Extend(w,s,E_RIGHT,256);
   }
   if(VY0>0||VY1<WG_MAP_H){
    if(VX0<0)VX0=0;
    if(VX1>256)VX1=256;
    if(VY0>0)Extend(w,s,E_UP,0);
    if(VY1<WG_MAP_H)Extend(w,s,E_DOWN,WG_MAP_H);
   }
  }
  gOx=CS.ox;gOy=CS.oy;
  // blocos 8x8 que mudaram de dono (conquistas do dia), na vista E nas
  // margens validas (com volta do mundo, nas 3 copias que podem estar perto)
  int m=ppc+4,n=0;
  for(int b=0;b<W_BLK_W*W_BLK_H;b++){
   if(!w->dirtyBlk[b>>3]){b|=7;continue;}            // 8 blocos limpos de uma vez
   if(!((w->dirtyBlk[b>>3]>>(b&7))&1))continue;
   if(++n>64){VX0=0;VX1=256;VY0=0;VY1=WG_MAP_H;Render(w,s,0,0,256,WG_MAP_H);break;}
   int cx=(b%W_BLK_W)*8,cy=(b/W_BLK_W)*8;
   int rx0=cx-s->camX;
   if(w->wrap){rx0%=w->w;if(rx0<0)rx0+=w->w;if(rx0>w->w-16)rx0-=w->w;}
   for(int c=w->wrap?-1:0;c<=(w->wrap?1:0);c++){
    int rx=rx0+c*w->w;
    int x0,y0,x1,y1;
    if(s->zoom==0){x0=rx/2-2;y0=(cy-s->camY)/2-2;x1=x0+4+4;y1=y0+4+4;}
    else{x0=rx*ppc-s->subX-m;y0=(cy-s->camY)*ppc-s->subY-m;x1=(rx+8)*ppc-s->subX+m;y1=(cy+8-s->camY)*ppc-s->subY+m;}
    if(x1<=VX0||y1<=VY0||x0>=VX1||y0>=VY1)continue;
    Render(w,s,x0,y0,x1,y1);
   }
  }
 }
 for(u32 i=0;i<sizeof w->dirtyBlk;i++)w->dirtyBlk[i]=0;
 CS.ok=true;CS.camX=s->camX;CS.camY=s->camY;CS.subX=s->subX;CS.subY=s->subY;CS.zoom=s->zoom;CS.mode=s->mapMode;CS.me=s->me;CS.sig=sig;
}
// RENDER-AHEAD (ADR-36): num quadro com folga, pinta o mapa a' frente da
// camera — primeiro o lado para onde ela andou por ultimo, depois o outro,
// depois em cima/baixo — ate' MH/MV de margem. Com relogio (DS/DSi), o passo
// e' do tamanho que CABE na folga: tpp16 = ticks por 16 pixels medidos nos
// passos anteriores (media movel; ~3,5 ticks/px no DS, metade no DSi), entao
// nunca se pinta um passo que estoura o quadro. budget 0 = sem relogio (host):
// passos fixos de ~2 mil pixels ate' o teto. Camera diferente do ultimo
// PxM_Update = espera o proximo desenho.
static u32 tpp16=56;
void PxM_Ahead(const World *w,const WGame *s,u32 budget,int maxSteps){
 if(!CS.ok||CS.camX!=s->camX||CS.camY!=s->camY||CS.subX!=s->subX||CS.subY!=s->subY||
    CS.zoom!=s->zoom||CS.mode!=s->mapMode||CS.me!=s->me)return;
 gOx=CS.ox;gOy=CS.oy;
 u32 t0=Px_Ticks();
 for(int k=0;k<maxSteps;k++){
  int ml=-VX0,mr=VX1-256,mu=-VY0,md=VY1-WG_MAP_H,side=-1;
  if(CS.dirX>=0){if(mr<MH)side=E_RIGHT;else if(ml<MH)side=E_LEFT;}
  else{if(ml<MH)side=E_LEFT;else if(mr<MH)side=E_RIGHT;}
  if(side<0){
   if(CS.dirY>=0){if(md<MV)side=E_DOWN;else if(mu<MV)side=E_UP;}
   else{if(mu<MV)side=E_UP;else if(md<MV)side=E_DOWN;}
  }
  if(side<0)return;                                  // margens cheias
  bool hz=side<=E_RIGHT;
  int len=hz?VY1-VY0:VX1-VX0;                        // pixels por coluna/linha
  int px=2048;                                       // passo sem relogio
  if(budget){
   u32 el=(u32)(Px_Ticks()-t0);
   if(el>=budget)break;
   px=(int)((budget-el)*16/tpp16);
   if(px>4096)px=4096;
  }
  int n=px/len-1;                                    // -1: a antiga borda tambem e' repintada
  int room=hz?(side==E_RIGHT?MH-mr:MH-ml):(side==E_DOWN?MV-md:MV-mu);
  if(n>room)n=room;
  if(n<(hz?2:1))break;                               // nao cabe nem um passo util
  u32 ts=Px_Ticks();
  switch(side){
   case E_RIGHT:Extend(w,s,E_RIGHT,VX1+n);break;
   case E_LEFT: Extend(w,s,E_LEFT,VX0-n);break;
   case E_DOWN: Extend(w,s,E_DOWN,VY1+n);break;
   default:     Extend(w,s,E_UP,VY0-n);break;
  }
  if(budget){u32 c=(u32)((u64)(Px_Ticks()-ts)*16/(u32)((n+1)*len));if(c<8)c=8;tpp16=(tpp16*3+c)/4;}
 }
}

HOT_BEGIN
// anel -> tela: cada linha sai em ate' 2 pedacos (da origem ox ate' o fim do
// anel, e o comeco); Px_Copy16 = so' acessos de 16/32 bits (a pagina e' VRAM)
HOT_ITCM void PxM_Blit(u16 *d){
 u16 *o=d+WG_MAP_Y*256;const int ox=CS.ox,n1=RW-ox<256?RW-ox:256;
 for(int sy=0;sy<WG_MAP_H;sy++,o+=256){
  const u16 *src=LAYER+(((CS.oy+sy)&(RH-1))<<RWS);
  Px_Copy16(o,src+ox,n1);
  if(n1<256)Px_Copy16(o+n1,src,256-n1);
 }
}
// frente de guerra (fogo piscando) e contorno do pais sob o cursor.
// ATK[c] = alvo da frente de c (0xFF = nenhuma): par (a,b) e' frente se um
// ataca o outro (0 = terra livre: colonizacao). Mesma regra de antes, sem
// buscar w->c[] por par.
typedef struct { const u8 *ATK; u8 sel; bool fronts,phase; u16 F0,F1; } OvCtx;
// os 4 pixels da palavra bw do anel, que comecam na coluna sx0 da tela:
// pares direita/baixo NA TELA (fora da vista: pula)
static inline void OvWord(const OvCtx *c,u16 *row,const u8 *K,const u8 *KD,bool last,int bw,int sx0){
 for(int i=0;i<4;i++){
  int sx=sx0+i;if(sx<0||sx>255)continue;
  int bx=bw+i;u8 a=K[bx];
  if(a>=K_OUT)continue;
  for(int j=0;j<2;j++){
   u8 b;u16 *p=row+sx,*q;
   if(j==0){if(sx==255)continue;b=K[(bx+1)&(RW-1)];q=p+1;}
   else{if(last)break;b=KD[bx];q=p+256;}
   if(b==a||b>=K_OUT)continue;
   if(c->fronts&&(c->ATK[a]==b||c->ATK[b]==a)){*p=c->F0;*q=c->F1;}
   else if(c->sel&&c->phase&&(a==c->sel||b==c->sel)){if(a==c->sel)*p=C_TEXT;else *q=C_TEXT;}
  }
 }
}
// 64 bits da mascara ciclica de 128 a partir do bit p (palavras da vista)
static inline u64 Win64(const u32 *E,int p){
 u64 r=0;
 for(int k=0;k<2;k++){int q=(p+32*k)&127,wi=q>>5,sh=q&31;
  u32 v=sh?((E[wi]>>sh)|(E[(wi+1)&3]<<(32-sh))):E[wi];
  r|=(u64)v<<(32*k);}
 return r;
}
HOT_ITCM void PxM_Overlay(u16 *d,const World *w,const WGame *s,int phase){
 static u8 ATK[W_MAXC+1];
 OvCtx c;c.ATK=ATK;c.fronts=false;
 ATK[0]=0xFF;
 for(u8 ci=1;ci<w->nC;ci++){
  const WCountry *k=&w->c[ci];
  ATK[ci]=k->atkOn?k->atkTo:0xFF;
  if(k->alive&&k->atkOn)c.fronts=true;
 }
 c.sel=(s->sel&&!s->selLand&&s->sel!=s->me)?s->sel:0;
 if(!c.fronts&&!c.sel)return;
 const u16 FA=PXC(31,27,8),FB=PXC(30,10,3);
 c.phase=phase!=0;c.F0=phase?FA:FB;c.F1=phase?FB:FA;
 u16 *o=d+WG_MAP_Y*256;
 // palavras da vista em ORDEM DE TELA: a 1a contem a coluna 0 (comeca em
 // -sub), +64 seguidas; com sub>0 a 65a fecha as colunas 252-255. Onde dois
 // pares pintam o mesmo pixel vence o ultimo, como na varredura completa de
 // antes (resultado bit a bit igual).
 const int w0=CS.ox>>2,sub=CS.ox&3,wx=(w0+64)&127;
 for(int sy=0;sy<WG_MAP_H;sy++){
  int by=(CS.oy+sy)&(RH-1);
  const u32 *E=EB[by];
  if(!(E[0]|E[1]|E[2]|E[3]))continue;
  const u8 *K=KEY+(by<<RWS),*KD=KEY+(((by+1)&(RH-1))<<RWS);
  bool last=sy==WG_MAP_H-1;
  u16 *row=o+sy*256;
  for(u64 m=Win64(E,w0);m;m&=m-1){int k=__builtin_ctzll(m);OvWord(&c,row,K,KD,last,((w0+k)&127)<<2,k*4-sub);}
  if(sub&&((E[wx>>5]>>(wx&31))&1))OvWord(&c,row,K,KD,last,wx<<2,256-sub);
 }
}
HOT_END
