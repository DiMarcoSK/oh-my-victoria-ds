#include <string.h>
#include "wgame.h"
#include "px.h"
#include "pal.h"
// px_mapr.c — camada do mapa-mundi: pintura suave + cache incremental
// (DESIGN.md sec 4.3, SDD ADR-24). Por que assim no DS (67MHz, 4MB):
//
// 1) PINTURA SUAVE: cada pixel interpola as 4 celulas mais proximas
//    (bilinear) depois de um "domain warp" leve por uma tabela de ruido
//    fixa. Costa e fronteira saem curvas e organicas, nao em degraus. Relevo
//    por hillshade do campo W_LIM (luz de noroeste); mar com profundidade
//    (raso -> fundo) e banda de espuma; textura de arvores/areia por hash.
//    Tudo em inteiro, com pontilhado Bayer 4x4 p/ gradiente liso em 15 bits.
// 2) CACHE: pintar um pixel assim custa ~150 ciclos; a tela inteira (44K px)
//    ~0,1 s no ARM9. Entao a pintura fica num cache (LAYER 88KB + KEY 44KB)
//    e so' se repinta o que mudou: troca de zoom/modo/paleta = tela toda;
//    andar com a camera = desloca o cache e pinta so' a faixa nova; conquista
//    = so' os blocos 8x8 marcados em World.dirtyBlk. Num dia comum isso e'
//    0-2% da tela.
// 3) OVERLAY: frente piscando e contorno do pais sob o cursor saem do KEY
//    (dono por pixel) a cada desenho, sem repintar o cache.
static u16 LAYER[WG_MAP_H*256];
static u8 KEY[WG_MAP_H*256];
enum { K_OUT=0xFE, K_SEA=0xFF };
static struct { bool ok; s16 camX,camY; u8 subX,subY,zoom,mode,me; u32 sig; } CS;
void PxM_Invalidate(void){CS.ok=false;}
u16 *PxM_Scratch(void){CS.ok=false;return LAYER;}   // a cena da cidade reusa os 88KB
const u8 *PxM_Key(void){return KEY;}

static inline int Cl8(int v){return v<0?0:(v>255?255:v);}
static const u8 BAYER[4][4]={{0,4,1,5},{6,2,7,3},{1,5,0,4},{7,3,6,2}};
static inline u16 Dith(int r,int g,int b,int x,int y){
 int t=BAYER[y&3][x&3];
 r=Cl8(r+t);g=Cl8(g+t);b=Cl8(b+t);
 return PXC(r>>3,g>>3,b>>3);
}
// ruido de valor suave 64x64 (gerado 1x, hash fixo: nao usa o RNG do jogo)
static s8 NZ[64][64];static bool nzOk;
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
 nzOk=true;
}

// paleta por dono (RGB 8 bits) e por terreno
static u8 CR[W_MAXC+1][3];
static u8 FLG[W_MAXC+1];            // 1 jogador, 4 povo (hachura)
static const u8 TER8[WT_COUNT][3]={
 {0,0,0},{196,184,128},{120,150,92},{176,156,112},{150,140,128},{224,200,136},
 {130,150,112},{90,140,72},{180,184,170},{200,184,118},{232,238,244}};
static void Palette(const World *w,const WGame *s){
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
  CR[ci][0]=(u8)(r*8);CR[ci][1]=(u8)(g*8);CR[ci][2]=(u8)(b*8);
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

// ---- pintura de 1 pixel do mundo (gx,gy em pixels do zoom atual) ----
static int gSh,gPpc,gW,gH,gMode;static bool gWrap;
static inline u32 Cell(int x,int y){
 if(y<0)y=0;
 if(y>=gH)y=gH-1;
 if(x<0||x>=gW){if(gWrap){x%=gW;if(x<0)x+=gW;}else x=x<0?0:gW-1;}
 return (u32)y*gW+x;
}
static u8 Pix(int gx,int gy,u16 *out,int sx,int sy){
 int ppc=gPpc,sh=gSh;
 // domain warp (Z2+): a fronteira serpenteia em vez de seguir a grade
 int ox=0,oy=0;
 if(ppc>=2){
  int q=sh-1;
  ox=NZ[(gy>>q)&63][(gx>>q)&63]*ppc/80;
  oy=NZ[((gx>>q)+29)&63][((gy>>q)+17)&63]*ppc/80;
 }
 int px=gx+ox-(ppc>>1),py=gy+oy-(ppc>>1);
 int cx=px>>sh,cy=py>>sh,fx=px&(ppc-1),fy=py&(ppc-1);
 u32 i0=Cell(cx,cy),i1=Cell(cx+1,cy),i2=Cell(cx,cy+1),i3=Cell(cx+1,cy+1);
 const u32 I[4]={i0,i1,i2,i3};
 const int wgt[4]={(ppc-fx)*(ppc-fy),fx*(ppc-fy),(ppc-fx)*fy,fx*fy};
 int tot=ppc*ppc,landW=0,best=-1,bw=-1;
 u8 own[4];int ow[4]={0,0,0,0},no=0;
 int E[4];
 for(int k=0;k<4;k++){
  bool land=W_TERR[I[k]]!=WT_WATER;
  E[k]=land?W_LIM[I[k]]:0;
  if(!land)continue;
  landW+=wgt[k];
  u8 o=W_OWNER[I[k]];int j=0;
  while(j<no&&own[j]!=o)j++;
  if(j==no){own[no]=o;no++;}
  ow[j]+=wgt[k];
  if(wgt[k]>bw){bw=wgt[k];best=k;}
 }
 if(ppc==1){landW=W_TERR[i0]!=WT_WATER?tot:0;best=landW?0:-1;}
 int r,g,b;u8 key;
 if(landW*2>tot||(ppc==1&&landW)){
  // dono: maior peso entre as amostras de terra
  int bj=0;for(int j=1;j<no;j++)if(ow[j]>ow[bj])bj=j;
  key=ppc==1?W_OWNER[i0]:own[bj];
  u8 t=W_TERR[I[best<0?0:best]];
  int tr=TER8[t][0],tg=TER8[t][1],tb=TER8[t][2];
  if(key&&gMode!=WMAP_TER){
   // cor do pais (78%) sobre o terreno (22%): papel colorido com relevo
   r=(CR[key][0]*78+tr*22)/100;g=(CR[key][1]*78+tg*22)/100;b=(CR[key][2]*78+tb*22)/100;
  }else{r=tr;g=tg;b=tb;}
  // hillshade: derivada do relevo, luz de noroeste
  int dEx=(E[1]-E[0])*(ppc-fy)+(E[3]-E[2])*fy, dEy=(E[2]-E[0])*(ppc-fx)+(E[3]-E[1])*fx;
  int sh2=(dEx+dEy)/(ppc*3);
  if(sh2>36)sh2=36;
  if(sh2<-36)sh2=-36;
  int el=(E[0]*wgt[0]+E[1]*wgt[1]+E[2]*wgt[2]+E[3]*wgt[3])/(tot?tot:1);
  sh2+=(el-80)/12;                       // terra alta um pouco mais clara
  r+=sh2;g+=sh2;b+=sh2;
  // praia: faixa clara onde a terra e' quase mar
  if(ppc>=2&&landW*100<tot*60){r=(r*2+232)/3;g=(g*2+214)/3;b=(b*2+160)/3;}
  // textura: copa de arvore / areia / neve por hash do pixel do mundo
  u32 h=(u32)gx*2246822519u^(u32)gy*3266489917u;h^=h>>15;
  if(ppc>=4){
   if((t==WT_FOREST||t==WT_JUNGLE)&&(h&7)<3){r-=22;g-=18;b-=20;}
   else if(t==WT_DESERT&&(h&15)==0){r+=14;g+=12;b+=8;}
   else if(t==WT_MOUNTAIN&&(h&15)<2){r+=24;g+=24;b+=26;}
  }
  if(key&&(FLG[key]&4)&&((gx+gy)&3)==0){r-=18;g-=18;b-=18;}   // povo autonomo
 }else{
  key=K_SEA;
  // profundidade interpolada (terra conta como 0): raso turquesa -> fundo
  int dsum=0;
  for(int k=0;k<4;k++)if(W_TERR[I[k]]==WT_WATER)dsum+=wgt[k]*W_LIM[I[k]];
  int d=ppc==1?W_LIM[i0]:dsum/(tot?tot:1);
  int t=d*256/10;if(t>256)t=256;
  r=(112*(256-t)+30*t)>>8;g=(186*(256-t)+78*t)>>8;b=(198*(256-t)+142*t)>>8;
  // espuma: perto da linha da costa
  if(ppc>=2&&landW*100>tot*30){r=(r+220)/2;g=(g+236)/2;b=(b+232)/2;}
  else if(ppc<2&&d<=1){r+=40;g+=36;b+=24;}
  u32 h=(u32)gx*2654435761u^(u32)gy*40503u;h^=h>>13;
  if(ppc>=4&&(h&63)==0){r+=24;g+=24;b+=20;}              // brilho de onda
 }
 *out=Dith(r,g,b,sx,sy);
 return key;
}
// Z0: 1 px = 2x2 celulas; media das 4 cores (antisserrilhado), dono = maioria
static u8 PixZ0(int cx,int cy,u16 *out,int sx,int sy){
 int r=0,g=0,b=0;u8 k0=K_SEA,keys[4];
 for(int k=0;k<4;k++){
  u16 c;keys[k]=Pix(cx+(k&1),cy+(k>>1),&c,sx,sy);
  r+=(c&31)<<3;g+=((c>>5)&31)<<3;b+=((c>>10)&31)<<3;
 }
 for(int k=0;k<4;k++)if(keys[k]!=K_SEA){int n=0;for(int j=0;j<4;j++)n+=keys[j]==keys[k];if(n>=2||k0==K_SEA)k0=keys[k];}
 *out=Dith(r/4,g/4,b/4,sx,sy);
 return k0;
}

// pinta o retangulo de tela [x0,x1)x[y0,y1): KEY+cor em R+2, borda em R+1
static void Render(const World *w,const WGame *s,int x0,int y0,int x1,int y1){
 int ax0=x0-2,ay0=y0-2,ax1=x1+2,ay1=y1+2;
 if(ax0<0)ax0=0;
 if(ay0<0)ay0=0;
 if(ax1>256)ax1=256;
 if(ay1>WG_MAP_H)ay1=WG_MAP_H;
 int bx0=x0-1,by0=y0-1,bx1=x1+1,by1=y1+1;
 if(bx0<0)bx0=0;
 if(by0<0)by0=0;
 if(bx1>256)bx1=256;
 if(by1>WG_MAP_H)by1=WG_MAP_H;
 bool z0=s->zoom==0;
 for(int sy=ay0;sy<ay1;sy++){
  int cyz=s->camY+sy*2;
  int gy=z0?0:s->camY*gPpc+s->subY+sy;
  bool outY=z0?(cyz<0||cyz>=w->h):(gy<0||gy>=w->h*gPpc);
  bool inB=sy>=by0&&sy<by1;
  for(int sx=ax0;sx<ax1;sx++){
   u32 k=(u32)sy*256+sx;u16 c;u8 key;
   if(outY){key=K_OUT;c=((sx+sy)&7)==0?C_NAVY:C_NAVY_BG;}
   else if(z0&&!w->wrap&&(s->camX+sx*2<0||s->camX+sx*2>=w->w)){key=K_OUT;c=((sx+sy)&7)==0?C_NAVY:C_NAVY_BG;}
   else if(z0)key=PixZ0(s->camX+sx*2,cyz,&c,sx,sy);
   else{
    int gx=s->camX*gPpc+s->subX+sx;
    if(!w->wrap&&(gx<0||gx>=w->w*gPpc)){key=K_OUT;c=((sx+sy)&7)==0?C_NAVY:C_NAVY_BG;}
    else key=Pix(gx,gy,&c,sx,sy);
   }
   KEY[k]=key;
   if(inB&&sx>=bx0&&sx<bx1)LAYER[k]=c;
  }
 }
 // fronteiras: pixel com vizinho de outro dono escurece (2px no total, o
 // brilho do Victoria 3); seu reino em ouro; costa com fio de tinta
 for(int sy=by0;sy<by1;sy++)for(int sx=bx0;sx<bx1;sx++){
  u32 k=(u32)sy*256+sx;u8 m=KEY[k];
  if(m==K_OUT||m==K_SEA)continue;
  u8 n[4]={sx>0?KEY[k-1]:m,sx<255?KEY[k+1]:m,sy>0?KEY[k-256]:m,sy<WG_MAP_H-1?KEY[k+256]:m};
  bool coast=false,border=false;
  for(int j=0;j<4;j++){if(n[j]==K_SEA)coast=true;else if(n[j]!=m&&n[j]!=K_OUT)border=true;}
  if(!coast&&!border)continue;
  u16 c=LAYER[k];int r=(c&31)<<3,g=((c>>5)&31)<<3,b=((c>>10)&31)<<3;
  if(border&&m&&(FLG[m]&1)&&s->mapMode!=WMAP_TER)LAYER[k]=C_GOLD;
  else if(border){LAYER[k]=Dith(r*9/16,g*9/16,b*9/16,sx,sy);}
  else LAYER[k]=Dith(r*3/4,g*3/4,b*3/4,sx,sy);
 }
}
static int RelX(const World *w,int a,int b){
 int d=b-a;
 if(w->wrap){d%=w->w;if(d<-w->w/2)d+=w->w;else if(d>=w->w/2)d-=w->w;}
 return d;
}
static void Shift(int dx,int dy){
 // conteudo anda (-dx,-dy) pixels; o que entra fica sujo p/ o Render.
 // memmove (ldm/stm da newlib) e' ~4x o laco por pixel no ARM9.
 if(dy){
  int n=WG_MAP_H-(dy>0?dy:-dy);
  if(n<=0)return;
  if(dy>0){memmove(LAYER,LAYER+dy*256,(size_t)n*256*2);memmove(KEY,KEY+dy*256,(size_t)n*256);}
  else{memmove(LAYER-dy*256,LAYER,(size_t)n*256*2);memmove(KEY-dy*256,KEY,(size_t)n*256);}
 }
 if(dx){
  int n=256-(dx>0?dx:-dx);
  if(n<=0)return;
  for(int y=0;y<WG_MAP_H;y++){
   u16 *L=&LAYER[y*256];u8 *K=&KEY[y*256];
   if(dx>0){memmove(L,L+dx,(size_t)n*2);memmove(K,K+dx,(size_t)n);}
   else{memmove(L-dx,L,(size_t)n*2);memmove(K-dx,K,(size_t)n);}
  }
 }
}
void PxM_Update(World *w,const WGame *s){
 if(!nzOk)NoiseInit();
 // o LAYER e' emprestado pela cena da cidade (PxM_Scratch): se o mapa o
 // repinta, o fundo da cena em cache deixou de existir. Sem isso, voltar a'
 // mesma cidade mostrava o mapa com os pedestres da cena por cima.
 PxC_SceneReset();
 gSh=s->zoom?s->zoom-1:0;gPpc=1<<gSh;gW=w->w;gH=w->h;gWrap=w->wrap;gMode=s->mapMode;
 u32 sig=Sig(w,s);
 Palette(w,s);
 bool full=!CS.ok||CS.zoom!=s->zoom||CS.mode!=s->mapMode||CS.me!=s->me||CS.sig!=sig;
 int ppcPan=s->zoom?gPpc:1;
 int dx=RelX(w,CS.camX,s->camX)*ppcPan,dy=(s->camY-CS.camY)*ppcPan;
 if(s->zoom==0){dx/=2;dy/=2;}
 else{dx+=s->subX-CS.subX;dy+=s->subY-CS.subY;}     // rolagem pixel a pixel (ADR-31)
 if(!full&&(dx>=200||dx<=-200||dy>=140||dy<=-140))full=true;
 if(full){
  Render(w,s,0,0,256,WG_MAP_H);
 }else{
  if(dx||dy){
   Shift(dx,dy);
   if(dx>0)Render(w,s,256-dx-2,0,256,WG_MAP_H);
   if(dx<0)Render(w,s,0,0,-dx+2,WG_MAP_H);
   if(dy>0)Render(w,s,0,WG_MAP_H-dy-2,256,WG_MAP_H);
   if(dy<0)Render(w,s,0,0,256,-dy+2);
  }
  // blocos 8x8 que mudaram de dono (conquistas do dia)
  int ppc=gPpc,m=ppc+4,n=0;
  for(int b=0;b<W_BLK_W*W_BLK_H;b++){
   if(!((w->dirtyBlk[b>>3]>>(b&7))&1))continue;
   if(++n>64){Render(w,s,0,0,256,WG_MAP_H);break;}
   int cx=(b%W_BLK_W)*8,cy=(b/W_BLK_W)*8;
   int rx=cx-s->camX;
   if(w->wrap){rx%=w->w;if(rx<0)rx+=w->w;if(rx>w->w-16)rx-=w->w;}
   int x0,y0,x1,y1;
   if(s->zoom==0){x0=rx/2-2;y0=(cy-s->camY)/2-2;x1=x0+4+4;y1=y0+4+4;}
   else{x0=rx*ppc-s->subX-m;y0=(cy-s->camY)*ppc-s->subY-m;x1=(rx+8)*ppc-s->subX+m;y1=(cy+8-s->camY)*ppc-s->subY+m;}
   if(x1<=0||y1<=0||x0>=256||y0>=WG_MAP_H)continue;
   Render(w,s,x0<0?0:x0,y0<0?0:y0,x1>256?256:x1,y1>WG_MAP_H?WG_MAP_H:y1);
  }
 }
 for(u32 i=0;i<sizeof w->dirtyBlk;i++)w->dirtyBlk[i]=0;
 CS.ok=true;CS.camX=s->camX;CS.camY=s->camY;CS.subX=s->subX;CS.subY=s->subY;CS.zoom=s->zoom;CS.mode=s->mapMode;CS.me=s->me;CS.sig=sig;
}
void PxM_Blit(u16 *d){memcpy(d+WG_MAP_Y*256,LAYER,sizeof LAYER);}
// frente de guerra (fogo piscando) e contorno do pais sob o cursor
void PxM_Overlay(u16 *d,const World *w,const WGame *s,int phase){
 bool fronts=false;
 for(u8 ci=1;ci<w->nC&&!fronts;ci++)if(w->c[ci].alive&&w->c[ci].atkOn)fronts=true;
 u8 sel=(s->sel&&!s->selLand&&s->sel!=s->me)?s->sel:0;
 if(!fronts&&!sel)return;
 const u16 FA=PXC(31,27,8),FB=PXC(30,10,3);
 u16 *o=d+WG_MAP_Y*256;
 for(int sy=0;sy<WG_MAP_H;sy++)for(int sx=0;sx<255;sx++){
  u32 k=(u32)sy*256+sx;u8 a=KEY[k];
  if(a>=K_OUT)continue;
  for(int j=0;j<2;j++){
   if(j==1&&sy==WG_MAP_H-1)break;
   u32 k2=j?k+256:k+1;u8 b=KEY[k2];
   if(b==a||b>=K_OUT)continue;
   const WCountry *C=w->c;
   if(fronts&&((C[a].atkOn&&C[a].atkTo==b)||(b&&C[b].atkOn&&C[b].atkTo==a)||(!a&&C[b].atkOn&&C[b].atkTo==0)||(!b&&C[a].atkOn&&C[a].atkTo==0))){
    o[k]=phase?FA:FB;o[k2]=phase?FB:FA;
   }else if(sel&&phase&&(a==sel||b==sel)){o[a==sel?k:k2]=C_TEXT;}
  }
 }
}
