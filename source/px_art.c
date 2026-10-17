#include "px.h"
#include "pal.h"
#include "px_art.h"
// px_art.c — decodificador RLE + placeholder de arte (SDD sec 14.6).
// O stream e' linear (pares count,indice somando w*h, linha a linha); a paleta
// ja' vem com BIT(15) do gerador, entao aqui so' copiamos. Zero malloc.

// Placeholder: retangulo C_NAVY_HI + borda C_GOLD + nome em 3x5. Mantem o
// layout de pe' enquanto o PNG nao chega (a F4 troca sem mexer nas telas).
static void PH(u16 *dst,const Art *a,int x,int y){
 Px_Rect(dst,x,y,a->w,a->h,C_NAVY_HI);
 Px_Border(dst,x,y,a->w,a->h,C_GOLD);
 Px_TextSc(dst,x+2,y+2,a->name?a->name:"?",C_TEXT_DIM,1);
}
// Blit generico: desenha so' a janela (sx,sy,sw,sh) da arte em (x,y); mirror
// espelha na horizontal (dentro da janela). Recorte de tela em Px_PSet.
static void Win(u16 *dst,const Art *a,int x,int y,int sx,int sy,int sw,int sh,int mirror){
 if(!a)return;
 if(!a->data){PH(dst,a,x,y);return;}
 const u8 *p=a->data;
 int n=(int)a->w*a->h,i=0;
 while(i<n){
  int cnt=p[0],v=p[1];p+=2;
  for(int k=0;k<cnt&&i<n;k++,i++){
   if(!v)continue;
   int col=i%a->w,row=i/a->w;
   if(col<sx||col>=sx+sw||row<sy||row>=sy+sh)continue;
   int dc=mirror?(sx+sw-1-col):col;
   Px_PSet(dst,x+(dc-sx),y+(row-sy),a->pal[v]);
  }
 }
}
void Art_Blit(u16 *dst,const Art *a,int x,int y){
 if(!a)return;
 Win(dst,a,x,y,0,0,a->w,a->h,0);
}
void Art_BlitH(u16 *dst,const Art *a,int x,int y){
 if(!a)return;
 Win(dst,a,x,y,0,0,a->w,a->h,1);
}
void Art_BlitClip(u16 *dst,const Art *a,int x,int y,int sx,int sy,int sw,int sh){
 if(!a||sw<=0||sh<=0)return;
 Win(dst,a,x,y,sx,sy,sw,sh,0);
}
