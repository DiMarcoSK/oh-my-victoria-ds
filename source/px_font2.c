#include "px.h"
#include "font.h"
// px_font2.c — renderizador da Serifa 8/12 (SDD sec 14.3). Os glifos vem de
// source/gen/font_data.c. Compartilha Px_PSet (recorte de tela) do px_core.
// Zero malloc; sem state global.

// Decodifica 1 codepoint UTF-8 e avanca *s. Entrada invalida -> 1 byte cru
// (nunca trava; a fonte resolve o que faltar com '?').
static int U8(const char **s){
 const u8 *p=(const u8*)*s; int c=*p; int cp;
 if(c<0x80){cp=c;*s+=1;return cp;}
 if((c&0xE0)==0xC0&&p[1]){cp=((c&0x1F)<<6)|(p[1]&0x3F);*s+=2;return cp;}
 if((c&0xF0)==0xE0&&p[1]&&p[2]){cp=((c&0x0F)<<12)|((p[1]&0x3F)<<6)|(p[2]&0x3F);*s+=3;return cp;}
 if((c&0xF8)==0xF0&&p[1]&&p[2]&&p[3]){cp=((c&0x07)<<18)|((p[1]&0x3F)<<12)|((p[2]&0x3F)<<6)|(p[3]&0x3F);*s+=4;return cp;}
 *s+=1;return c;
}
// Busca binaria por codepoint (tabela ordenada por cp). Zero malloc.
const Glyph *F_Glyph(const Font *f,int cp){
 int lo=0,hi=(int)f->n-1;
 while(lo<=hi){
  int mid=(lo+hi)>>1; int c=f->gl[mid].cp;
  if(c==cp)return &f->gl[mid];
  if(c<cp)lo=mid+1; else hi=mid-1;
 }
 return 0;
}
// cobertura q (1..3) de col sobre o pixel ja' na tela: mistura em terços,
// so' inteiro (Serifa 12 antisserrilhada). Fora da tela = nada.
static void Mix(u16 *dst,int x,int y,u16 col,int q){
 if(x<0||x>255||y<0||y>191)return;
 if(q>=3){dst[y*256+x]=col;return;}
 u16 b=dst[y*256+x];
 int r=((b&31)*(3-q)+(col&31)*q)/3,g=(((b>>5)&31)*(3-q)+((col>>5)&31)*q)/3,bl=(((b>>10)&31)*(3-q)+((col>>10)&31)*q)/3;
 dst[y*256+x]=(u16)(BIT(15)|(bl<<10)|(g<<5)|r);
}
// nivel 0..3 do pixel (c,r) do glifo (1bpp vira 0/3)
static inline int Lv(const Font *f,const u8 *bits,int stride,int c,int r){
 const u8 *row=bits+r*stride;
 if(f->bpp==2)return (row[c>>2]>>(6-2*(c&3)))&3;
 return (row[c>>3]&(0x80>>(c&7)))?3:0;
}
static inline int Stride(const Font *f,const Glyph *g){return (g->w*f->bpp+7)>>3;}
static void Draw(u16 *dst,int penx,int y,const Font *f,const Glyph *g,u16 col){
 if(!g||!g->w||!g->h)return;
 const u8 *bits=f->bits+g->off; int stride=Stride(f,g);
 int x0=penx+g->left, y0=y+f->caph-g->top; // y = topo da caixa alta -> baseline
 for(int r=0;r<g->h;r++)for(int c=0;c<g->w;c++){
  int q=Lv(f,bits,stride,c,r);
  if(q)Mix(dst,x0+c,y0+r,col,q);
 }
}
// Caixa alta p/ nomes no mapa (Victoria): ASCII, Latin-1 e Latin Extended-A.
// So' troca se a fonte tiver o glifo maiusculo; senao fica a minuscula (o
// acento nunca some — DESIGN 3.4).
static int Up(const Font *f,int cp){
 int u=cp;
 if(cp>='a'&&cp<='z')u=cp-32;
 else if(cp>=0xE0&&cp<=0xFE&&cp!=0xF7)u=cp-32;
 else if(cp>=0x100&&cp<=0x17F&&(cp&1))u=cp-1;
 return (u!=cp&&F_Glyph(f,u))?u:cp;
}
// Glifo ampliado (sc>1: cada pixel vira sc x sc). Usado no logo e titulos.
static void DrawSc(u16 *dst,int penx,int y,const Font *f,const Glyph *g,u16 col,int sc){
 if(!g||!g->w||!g->h)return;
 if(sc==1){Draw(dst,penx,y,f,g,col);return;}
 const u8 *bits=f->bits+g->off; int stride=Stride(f,g);
 int x0=penx+g->left*sc, y0=y+(f->caph-g->top)*sc;
 for(int r=0;r<g->h;r++)for(int c=0;c<g->w;c++){
  int q=Lv(f,bits,stride,c,r);
  if(q)for(int j=0;j<sc;j++)for(int i=0;i<sc;i++)Mix(dst,x0+c*sc+i,y0+r*sc+j,col,q);
 }
}
void Px_Text2Ex(u16 *dst,int x,int y,const char *s,const Font *f,u16 col,int sp,int sc,int flags){
 if(sc<1)sc=1;
 int cx=x;
 while(*s){
  int cp=U8(&s);
  if(cp=='\n'){cx=x;y+=f->lineh*sc;continue;}
  if(flags&TX_UPPER)cp=Up(f,cp);
  const Glyph *g=F_Glyph(f,cp);
  if(!g&&cp!=' ')g=F_Glyph(f,'?');
  if(g){DrawSc(dst,cx,y,f,g,col,sc);cx+=(g->adv+sp)*sc;}
  else if(cp==' ')cx+=(f->caph/2+1+sp)*sc;
 }
}
int Px_Width2Ex(const char *s,const Font *f,int sp,int sc,int flags){
 if(sc<1)sc=1;
 int w=0,n=0;
 while(*s){
  int cp=U8(&s);
  if(cp=='\n')break;
  if(flags&TX_UPPER)cp=Up(f,cp);
  const Glyph *g=F_Glyph(f,cp);
  if(g)w+=g->adv; else if(cp==' ')w+=f->caph/2+1; else continue;
  n++;
 }
 return (w+(n>1?(n-1)*sp:0))*sc;
}
// Texto com contorno de 1 px (sc px se ampliado) nas 8 direcoes: nomes no mapa
// e logo. Desenha o contorno primeiro e o miolo por cima.
void Px_Text2Out(u16 *dst,int x,int y,const char *s,const Font *f,u16 col,u16 out,int sp,int sc,int flags){
 if(sc<1)sc=1;
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)
  if(dx||dy)Px_Text2Ex(dst,x+dx*sc,y+dy*sc,s,f,out,sp,sc,flags);
 Px_Text2Ex(dst,x,y,s,f,col,sp,sc,flags);
}
void Px_Text2(u16 *dst,int x,int y,const char *s,const Font *f,u16 col){
 int cx=x;
 while(*s){
  int cp=U8(&s);
  if(cp=='\n'){cx=x;y+=f->lineh;continue;}
  const Glyph *g=F_Glyph(f,cp);
  if(!g&&cp!=' ')g=F_Glyph(f,'?');
  if(g){Draw(dst,cx,y,f,g,col);cx+=g->adv;}
  else if(cp==' ')cx+=f->caph/2+1;
 }
}
int Px_Width2(const char *s,const Font *f){
 int w=0;
 while(*s){
  int cp=U8(&s);
  if(cp=='\n')break;
  const Glyph *g=F_Glyph(f,cp);
  if(g)w+=g->adv; else if(cp==' ')w+=f->caph/2+1;
 }
 return w;
}
// Quebra por palavra com teto de linhas; center = cada linha centrada em
// x+lim/2. Se o texto nao couber, a ultima linha termina em "…". Devolve o
// numero de linhas desenhadas. Linha montada num buffer local (zero malloc).
int Px_WrapN(u16 *dst,int x,int y,int lim,const char *s,const Font *f,u16 col,int maxLines,bool center){
 char line[192];int ln=0,lines=0;const char *p=s;
 while(*p&&lines<maxLines){
  // proxima palavra
  const char *w0=p;while(*p&&*p!=' '&&*p!='\n')p++;
  int wl=(int)(p-w0);
  char cand[192];int cn=0;
  for(int i=0;i<ln&&cn<190;i++)cand[cn++]=line[i];
  if(ln&&cn<190)cand[cn++]=' ';
  for(int i=0;i<wl&&cn<190;i++)cand[cn++]=w0[i];
  cand[cn]=0;
  bool brk=(*p=='\n');
  if(ln&&Px_Width2(cand,f)>lim){
   // a palavra nao cabe: fecha a linha atual e recomeca com ela
   line[ln]=0;
   bool last=(lines==maxLines-1);
   if(last){
    // corta e poe reticencias
    while(ln>0&&Px_Width2(line,f)+Px_Width2("…",f)>lim)line[--ln]=0;
    int k=ln;const char *e="…";while(*e&&k<190)line[k++]=*e++;line[k]=0;
   }
   int lx=center?x+(lim-Px_Width2(line,f))/2:x;
   Px_Text2(dst,lx,y+lines*f->lineh,line,f,col);lines++;
   if(last)return lines;
   ln=0;for(int i=0;i<wl&&ln<190;i++)line[ln++]=w0[i];
  }else{for(int i=0;i<cn;i++)line[i]=cand[i];ln=cn;}
  while(*p==' ')p++;
  if(brk||!*p){
   line[ln]=0;
   if(ln){int lx=center?x+(lim-Px_Width2(line,f))/2:x;Px_Text2(dst,lx,y+lines*f->lineh,line,f,col);lines++;}
   ln=0;if(*p=='\n')p++;
  }
 }
 return lines;
}
int Px_Wrap(u16 *dst,int x,int y,int lim,const char *s,const Font *f,u16 col){
 int cx=x,cy=y; const char *p=s;
 while(*p){
  if(*p=='\n'){cx=x;cy+=f->lineh;p++;continue;}
  if(*p==' '){cx+=f->caph/2+1;p++;continue;}
  char wbuf[80]; int wi=0;
  while(*p&&*p!=' '&&*p!='\n'){if(wi<79)wbuf[wi++]=*p;p++;}
  wbuf[wi]=0;
  int ww=Px_Width2(wbuf,f);
  if(cx>x&&cx+ww>x+lim){cx=x;cy+=f->lineh;}
  Px_Text2(dst,cx,cy,wbuf,f,col);
  cx+=ww;
 }
 return cy+f->lineh-y;
}
