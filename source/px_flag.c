#include "flag.h"
#include "px.h"
// px_flag.c — padroes geometricos + emblemas 8x8 (1bpp) escalados.
// Proporcoes calculadas por linha; o laco de pixel so compara inteiros.

static const u8 EMB[FE_COUNT][8]={
 {0,0,0,0,0,0,0,0},
 {0x18,0x18,0xFF,0x7E,0x3C,0x3C,0x66,0x42}, // estrela
 {0x38,0x60,0xC2,0xC7,0xC2,0x60,0x38,0x00}, // crescente + estrela
 {0x24,0xBD,0x7E,0xFF,0xFF,0x7E,0xBD,0x24}, // sol
 {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C}, // disco
 {0x18,0x99,0xDB,0xFF,0xFF,0xFF,0x00,0xFF}, // coroa
 {0x18,0x99,0xFF,0x7E,0x3C,0x3C,0x5A,0x81}, // aguia
 {0x42,0x66,0xBD,0xFF,0x7E,0x3C,0x5A,0x81}, // aguia de duas cabecas
 {0x70,0xF8,0xFF,0x7F,0x3E,0x3E,0x66,0x66}, // leao
 {0x18,0x18,0xFF,0xFF,0x18,0x18,0x18,0x18}, // cruz
 {0xE0,0xB8,0x1C,0x0E,0x77,0xE3,0x81,0x00}, // dragao
 {0xFF,0xFF,0xFF,0xFF,0x7E,0x7E,0x3C,0x18}, // escudo
};

static u16 FC(u8 c){u8 r,g,b;Flag_PalRGB(c,&r,&g,&b);return Px_Col(r,g,b);}
static inline int Iabs(int v){return v<0?-v:v;}

static void Emblem(u16 *buf,int stride,int x,int y,int w,int h,u8 e,u16 col){
 if(e==FE_NONE||e>=FE_COUNT)return;
 int m=w<h?w:h;
 int s=(m*5/8)/8; if(s<1)s=1;
 int sz=8*s;
 if(sz>w||sz>h)return;
 int ox=x+(w-sz)/2, oy=y+(h-sz)/2;
 for(int j=0;j<8;j++){
  u8 row=EMB[e][j];
  for(int i=0;i<8;i++){
   if(!(row&(0x80>>i)))continue;
   for(int dy=0;dy<s;dy++){
    u16 *p=buf+(oy+j*s+dy)*stride+ox+i*s;
    for(int dx=0;dx<s;dx++)p[dx]=col;
   }
  }
 }
}

// vcut>0: as ultimas vcut linhas tem o centro aberto (ponta em V do
// estandarte). Pixel aberto NAO e escrito: o fundo de quem chamou aparece
// (escrever 0 seria transparente no Bmp16 e mostraria o backdrop, ADR-13).
static void Draw(u16 *buf,int stride,int x,int y,int w,int h,const FlagDef *f,int vcut){
 if(w<3||h<3)return;
 u16 c0=FC(f->c0),c1=FC(f->c1),c2=FC(f->c2);
 u16 nv=FC(FC_NAVY),wh=FC(FC_WHITE),rd=FC(FC_RED);
 int t=h/6; if(t<1)t=1;              // espessura de cruzes
 int big=w>h?w:h;
 int stripes=h>=13?13:(h>=7?7:5);
 int cantW=w*2/5, cantH=(h*7)/13;
 for(int j=0;j<h;j++){
  u16 *row=buf+(y+j)*stride+x;
  int band3=(j*3)/h, band2=(j*2)/h, stripe=(j*stripes)/h;
  int oa=w, ob=w;                    // faixa aberta [oa,ob) desta linha
  if(vcut>0&&j>=h-vcut){
   int k=j-(h-vcut)+1;
   int open=(k*w)/(vcut+1);
   oa=(w-open)/2; ob=oa+open;
  }
  for(int i=0;i<w;i++){
   if(i>=oa&&i<ob)continue;
   u16 c=c0;
   switch(f->pat){
   case FP_TRI_V:{int b=(i*3)/w;c=b==0?c0:(b==1?c1:c2);}break;
   case FP_TRI_H:c=band3==0?c0:(band3==1?c1:c2);break;
   case FP_BI_H:c=band2==0?c0:c1;break;
   case FP_BI_V:c=(i*2)/w==0?c0:c1;break;
   case FP_NORDICA:if(Iabs(i-w*5/16)<=t/2||Iabs(j-h/2)<=t/2)c=c1;break;
   case FP_CRUZ:if(Iabs(i-w/2)<=t/2||Iabs(j-h/2)<=t/2)c=c1;break;
   case FP_SALTIRE:{int d1=Iabs(i*h-j*w),d2=Iabs((w-1-i)*h-j*w);
    if(d1<=t*big/2||d2<=t*big/2)c=c1;}break;
   case FP_UNION:{
    int d1=Iabs(i*h-j*w),d2=Iabs((w-1-i)*h-j*w);
    c=nv;
    if(d1<=t*big||d2<=t*big)c=wh;
    if(d1<=t*big/3||d2<=t*big/3)c=rd;
    if(Iabs(i-w/2)<=t||Iabs(j-h/2)<=t)c=wh;
    if(Iabs(i-w/2)<=t/2||Iabs(j-h/2)<=t/2)c=rd;
   }break;
   case FP_CANTAO:if(i<w/2&&j<h/2)c=c1;break;
   case FP_BORDA:c=(i<t||j<t||i>=w-t||j>=h-t)?c0:c1;break;
   case FP_DIAG:c=(i*h+j*w<w*h)?c0:c1;break;
   case FP_LISTRAS:c=(stripe&1)?c1:c0;
    if(i<cantW&&j<cantH)c=c2;
    break;
   default:break;
   }
   row[i]=c;
  }
 }
 u16 ec=FC(f->ec);
 if(f->pat==FP_CANTAO)Emblem(buf,stride,x,y,w/2,h/2,f->emb,ec);
 else if(f->pat==FP_LISTRAS)Emblem(buf,stride,x,y,cantW,cantH,f->emb,ec);
 else Emblem(buf,stride,x,y,w,h-vcut,f->emb,ec);
}

void Flag_Draw(u16 *buf,int stride,int x,int y,int w,int h,const FlagDef *f){
 Draw(buf,stride,x,y,w,h,f,0);
}

void Flag_Banner(u16 *buf,int stride,int x,int y,int w,int h,const FlagDef *f){
 // haste dourada de 2px acima do pano (tokens C_GOLD/C_GOLD_DIM, SDD 14.1)
 u16 gold=Px_Col(25,20,9), dark=Px_Col(15,12,6);
 for(int i=-2;i<w+2;i++){buf[y*stride+x+i]=gold;buf[(y+1)*stride+x+i]=dark;}
 int vh=h/6; if(vh<2)vh=2;
 Draw(buf,stride,x,y+2,w,h-2,f,vh);
}
