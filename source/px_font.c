// px_font.c — fonte 3x5 propria (A-Z 0-9 :;.,-+*/!@()<>% ), 4x6 com espaco.
// Cada glifo = 5 linhas x 3 bits. Desenhada a mao p/ caber em <2KB.
#include "px.h"
static const u8 G_A[]={0x2,0x5,0x7,0x5,0x5};
static const u8 G_B[]={0x6,0x5,0x6,0x5,0x6};
static const u8 G_C[]={0x3,0x4,0x4,0x4,0x3};
static const u8 G_D[]={0x6,0x5,0x5,0x5,0x6};
static const u8 G_E[]={0x7,0x4,0x6,0x4,0x7};
static const u8 G_F[]={0x7,0x4,0x6,0x4,0x4};
static const u8 G_G[]={0x3,0x4,0x5,0x5,0x3};
static const u8 G_H[]={0x5,0x5,0x7,0x5,0x5};
static const u8 G_I[]={0x7,0x2,0x2,0x2,0x7};
static const u8 G_J[]={0x1,0x1,0x1,0x5,0x2};
static const u8 G_K[]={0x5,0x5,0x6,0x5,0x5};
static const u8 G_L[]={0x4,0x4,0x4,0x4,0x7};
static const u8 G_M[]={0x5,0x7,0x7,0x5,0x5};
static const u8 G_N[]={0x6,0x5,0x5,0x5,0x5};
static const u8 G_O[]={0x2,0x5,0x5,0x5,0x2};
static const u8 G_P[]={0x6,0x5,0x6,0x4,0x4};
static const u8 G_Q[]={0x2,0x5,0x5,0x6,0x3};
static const u8 G_R[]={0x6,0x5,0x6,0x5,0x5};
static const u8 G_S[]={0x3,0x4,0x2,0x1,0x6};
static const u8 G_T[]={0x7,0x2,0x2,0x2,0x2};
static const u8 G_U[]={0x5,0x5,0x5,0x5,0x7};
static const u8 G_V[]={0x5,0x5,0x5,0x5,0x2};
static const u8 G_W[]={0x5,0x5,0x7,0x7,0x5};
static const u8 G_X[]={0x5,0x5,0x2,0x5,0x5};
static const u8 G_Y[]={0x5,0x5,0x2,0x2,0x2};
static const u8 G_Z[]={0x7,0x1,0x2,0x4,0x7};
static const u8 G_0[]={0x7,0x5,0x5,0x5,0x7};
static const u8 G_1[]={0x2,0x6,0x2,0x2,0x7};
static const u8 G_2[]={0x6,0x1,0x2,0x4,0x7};
static const u8 G_3[]={0x6,0x1,0x2,0x1,0x6};
static const u8 G_4[]={0x5,0x5,0x7,0x1,0x1};
static const u8 G_5[]={0x7,0x4,0x6,0x1,0x6};
static const u8 G_6[]={0x3,0x4,0x6,0x5,0x2};
static const u8 G_7[]={0x7,0x1,0x2,0x2,0x2};
static const u8 G_8[]={0x2,0x5,0x2,0x5,0x2};
static const u8 G_9[]={0x2,0x5,0x3,0x1,0x6};
static const u8 G_DOT[]={0x0,0x0,0x0,0x0,0x2};
static const u8 G_COL[]={0x0,0x2,0x0,0x2,0x0};
static const u8 G_PLUS[]={0x0,0x2,0x7,0x2,0x0};
static const u8 G_MIN[]={0x0,0x0,0x7,0x0,0x0};
static const u8 G_EXC[]={0x2,0x2,0x2,0x0,0x2};
static const u8 G_AT[]={0x2,0x5,0x7,0x4,0x3};
static const u8 G_SLA[]={0x1,0x1,0x2,0x4,0x4};
static const u8 G_GT[]={0x4,0x2,0x1,0x2,0x4};
static const u8 G_PCT[]={0x5,0x1,0x2,0x4,0x5};
static const u8 G_SPC[]={0x0,0x0,0x0,0x0,0x0};
static const u8 *Glyph(char c){
 if(c>='a'&&c<='z')c-=32;
 switch(c){
  case 'A':return G_A;case 'B':return G_B;case 'C':return G_C;
  case 'D':return G_D;case 'E':return G_E;case 'F':return G_F;
  case 'G':return G_G;case 'H':return G_H;case 'I':return G_I;
  case 'J':return G_J;case 'K':return G_K;case 'L':return G_L;
  case 'M':return G_M;case 'N':return G_N;case 'O':return G_O;
  case 'P':return G_P;case 'Q':return G_Q;case 'R':return G_R;
  case 'S':return G_S;case 'T':return G_T;case 'U':return G_U;
  case 'V':return G_V;case 'W':return G_W;case 'X':return G_X;
  case 'Y':return G_Y;case 'Z':return G_Z;
  case '0':return G_0;case '1':return G_1;case '2':return G_2;
  case '3':return G_3;case '4':return G_4;case '5':return G_5;
  case '6':return G_6;case '7':return G_7;case '8':return G_8;
  case '9':return G_9;
  case '.':case ',':return G_DOT;case ':':return G_COL;
  case '+':return G_PLUS;case '-':return G_MIN;
  case '!':return G_EXC;case '@':return G_AT;
  case '/':return G_SLA;case '>':return G_GT;case '%':return G_PCT;
  default:return G_SPC;
 }
}
int Px_Width(const char *s){int n=0;while(*s++){n+=4;}return n?n-1:0;}
static void GlyphSc(u16 *dst,int gx,int gy,int sc,const u8 *gl,u16 col){
 for(int r=0;r<5;r++)for(int c2=0;c2<3;c2++)
  if(gl[r]&(1<<(2-c2)))
   for(int r2=0;r2<sc;r2++)for(int c3=0;c3<sc;c3++)
    Px_PSet(dst,gx+c2*sc+c3,gy+r*sc+r2,col);
}
const u8 *Px_Glyph(char c){return Glyph(c);}
static void GlyphTo(u16 *dst,int x,int y,const char c,u16 col){
 const u8 *gl=Glyph(c);
 for(int r=0;r<5;r++)for(int c2=0;c2<3;c2++)
  if(gl[r]&(1<<(2-c2)))Px_PSet(dst,x+c2,y+r,col);
}
void Px_Text(int x,int y,const char *s,u16 col){
 u16 *dst=Px_BackBot();
 int cx=x;
 while(*s){GlyphTo(dst,cx,y,*s,col);cx+=4;s++;}
}
void Px_TextTop(int x,int y,const char *s,u16 col){
 u16 *dst=Px_BackTop();
 int cx=x;
 while(*s){GlyphTo(dst,cx,y,*s,col);cx+=4;s++;}
}
void Px_TextSc(u16 *dst,int x,int y,const char *s,u16 col,int sc){
 int cx=x;
 while(*s){GlyphSc(dst,cx,y,sc,Glyph(*s),col);cx+=4*sc;s++;}
}
void Px_Bar(int x,int y,int w,int pct,u16 col){
 if(pct<0)pct=0;
 if(pct>100)pct=100;
 u16 *dst=Px_BackBot();
 Px_Rect(dst,x,y,w,5,Px_Col(4,4,4));
 Px_Rect(dst,x,y,(w*pct)/100,5,col);
}
void Px_BarT(int x,int y,int w,int h,int pct,u16 col){
 if(pct<0)pct=0;
 if(pct>100)pct=100;
 u16 *dst=Px_BackTop();
 Px_Rect(dst,x,y,w,h,Px_Col(4,4,4));
 Px_Rect(dst,x,y,(w*pct)/100,h,col);
}
void Px_Itoa(int v,char *out){
 char tmp[12];int k=0,n=0;
 if(v<0){*out++='-';if(v== -2147483647-1)v=2147483647;v=-v;}
 if(v==0)tmp[k++]='0';
 while(v>0&&k<11){tmp[k++]=(char)('0'+(v%10));v/=10;}
 while(k>0)out[n++]=tmp[--k];
 out[n]=0;
}
