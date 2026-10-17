#pragma once
#include <nds.h>
// earth.h — mascara terra/agua do mapa-mundi (Natural Earth, dominio publico).
// Dados em source/earth_mask.c, gerado por tools/gen_earth.py.
#define EARTH_W 512
#define EARTH_H 192
#define EARTH_LAT_TOP 80   // linha 0 = 80N
#define EARTH_LAT_SPAN 135 // ate 55S

extern const u16 EARTH_ROW[EARTH_H];
extern const u8 EARTH_RLE[];
extern const u32 EARTH_RLE_LEN;

// Decodifica a linha y em out[EARTH_W] (1 = terra, 0 = agua).
static inline void Earth_Row(int y,u8 *out){
 const u8 *p=EARTH_RLE+EARTH_ROW[y];
 int x=0; u8 v=0;
 while(x<EARTH_W){
  int n=*p++;
  for(int i=0;i<n&&x<EARTH_W;i++)out[x++]=v;
  v^=1;
 }
}
// Graus inteiros -> celula (sem float).
static inline int Earth_X(int lon){int x=((lon+180)*EARTH_W)/360;return x<0?0:(x>=EARTH_W?EARTH_W-1:x);}
static inline int Earth_Y(int lat){int y=((EARTH_LAT_TOP-lat)*EARTH_H)/EARTH_LAT_SPAN;return y<0?0:(y>=EARTH_H?EARTH_H-1:y);}
