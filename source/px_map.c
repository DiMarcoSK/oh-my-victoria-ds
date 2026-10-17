// px_map.c — v0.8: CARTOGRAFIA (adeus grade de quadrados).
// Conceito artistico: "tinta sobre papel". O mundo e pintado como um mapa
// politico moderno: terreno CONTINUO (sangra entre tiles, sem xadrez), costa
// com espuma e shelf, relevo com luz NO, montanha com neve/sombra, fronteiras
// de faccao como faixa de tinta organica (jitter), obras como glifos em chip
// de pergaminho. Tudo BAKED no buffer-mundo 512x384 (.bss). A tela baixo e um
// VIEWPORT copiado por row (96KB, pan da caneta fluido). Overlays (cursor,
// bandeira de evento, alvos) sao desenhados por frame sobre o viewport.
// SNAP u32 por tile: so o que mudou e rebaked (budget espalhado por frames).
#include "px.h"
#include "map.h"
#include "ui.h"

static u16 WORLD[PX_WORLD_W*PX_WORLD_H]; // .bss 384KB
static u32 SNAP[MAP_H*MAP_W];
static u32 s_tick=0; // cadencia das animacoes de overlay

// ============ utilidades de cor ============
static inline int Clamp31(int v){return v<0?0:(v>31?31:v);}
static inline u16 Shc(int r,int g,int b,int d){
 return Px_Col((u8)Clamp31(r+d),(u8)Clamp31(g+d),(u8)Clamp31(b+d));
}
static inline u16 Mix(u16 a,u16 b,int num,int den){
 if(num<=0)return a; if(num>=den)return b;
 int ar=a&31,ag=(a>>5)&31,ab=(a>>10)&31;
 int br=b&31,bg=(b>>5)&31,bb=(b>>10)&31;
 return Shc(ar+((br-ar)*num)/den,ag+((bg-ag)*num)/den,ab+((bb-ab)*num)/den,0);
}
static inline int H2u(int x,int y){
 unsigned h=((unsigned)(x*73856093u))^((unsigned)(y*19349663u));
 h^=h>>13;h*=0x9E37u;h^=h>>11;
 return (int)(h&0x7FFFFFFFu);
}
#define H15(x,y) (H2u(x,y)&15)
static inline void WSET1(int x,int y,u16 c){WORLD[(y)*PX_WORLD_W+(x)]=c;}
static inline u16 WGET1(int x,int y){return WORLD[(y)*PX_WORLD_W+(x)];}
// pixel LOCAL ao tile (clip 0..15; nunca vaza para o vizinho)
static inline void Lpx(int bx,int by,int dx,int dy,u16 c){
 if((unsigned)dx<16&&(unsigned)dy<16)WSET1(bx+dx,by+dy,c);
}
static void MiniFill(int bx,int by,int x0,int y0,int x1,int y1,u16 c){
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)Lpx(bx,by,x,y,c);
}
static void LineP(int bx,int by,int x0,int y0,int x1,int y1,u16 c){
 int dx=x1-x0,dy=y1-y0,sx=dx<0?-1:1,sy=dy<0?-1:1;
 dx=sx*dx;dy=sy*dy;int err=dx+dy,x=x0,y=y0;
 while(1){Lpx(bx,by,x,y,c);if(x==x1&&y==y1)break;
  int e2=err*2;if(e2>=dy){err-=dy;x+=sx;}if(e2<=dx){err+=dx;y+=sy;}}
}

// ============ terreno: cor base + textura ============
static u16 TerrainBase(u8 ter,int tx,int ty){
 int s=((H15(tx,ty)>>1)&3)-1; // -1..2: tom por tile (variacoes sutis)
 switch(ter){
  case TER_WATER: case TER_OASIS:    return Shc(15,24,32,s);
  case TER_FOREST: return Shc(12,20,10,s);
  case TER_HILLS:  return Shc(22,21,13,s);
  case TER_MOUNTAIN: return Shc(19,18,18,s);
  case TER_DESERT: return Shc(27,22,12,s);
  case TER_SWAMP:  return Shc(12,16,10,s);
  case TER_VOLCANO:return Shc(15,12,12,s);
  case TER_RUINS:  return Shc(17,16,15,s);
  default:         return Shc(18,25,13,s); // planicie
 }
}
static inline bool IsWater(GameState*g,int x,int y){
 if(x<0||y<0||x>=MAP_W||y>=MAP_H)return false;
 return g->tiles[y][x].terrain==TER_WATER;
}
// TerTex: devolve delta de brilho -4..+4 OU codigo >=8 de cor plena:
// 8=neve, 9=lago, 10=lava, 11=brasa (resolvidos logo depois)
static int TerTex(u8 ter,int tx,int ty,int fx,int fy){
 switch(ter){
  case TER_PLAINS:{
   int n=H15((fx>>2)+(tx<<2),(fy>>2)+(ty<<2))&3;
   int d=(n==0)?1:(n==2)?-1:0;
   if(((fx+fy*2)&7)==0)d+=1;
   return d;
  }
  case TER_FOREST:{
   int d=(((fx*fx+fy*fy)&3)==0)?1:0; // fibra da mata
   for(int i=0;i<3;i++){ // 3 copas deterministas por tile
    int cx=2+((H15(tx*3+i,ty*7+i)&3)*4)+(i&1);
    int cy=2+((H15(tx*5+i,ty*11+i)&3)*4)+((i>>1)&1);
    int ddx=fx-cx,ddy=fy-cy,d2=ddx*ddx+ddy*ddy;
    if(d2<3)d=-4;
    else if(d2<9&&((fx+fy)&3)==0)d=-1;
    else if(d2==9&&((fx+fy)&7)<2)d=2;
   }
   return d;
  }
  case TER_HILLS:{
   int ln=(fy==3||fy==8||fy==12),hn=(fy==4||fy==9||fy==13);
   if(((fx+fy)&3)<2){if(ln)return -3;if(hn)return 2;}
   if(((fx*2+fy)&15)<1)return -1;
   return 0;
  }
  case TER_MOUNTAIN:{
   int px=4+(H15(tx*7,ty*5)&7),py=4+(H15(tx*3,ty*9)&7);
   int ddx=fx-px,ddy=fy-py,d2=ddx*ddx+ddy*ddy;
   if(d2<=2)return 8;                       // neve
   if(d2<=6)return (ddx>0||ddy>0)?-4:2;     // sombra SE / luz NO
   if(((fx+fy)&7)<1)return -3;              // sulco
   return 0;
  }
  case TER_DESERT:{
   int wv=(fx+((fy*13)&15))&15;
   if(wv==3)return -3; if(wv==4)return 2;
   if((fx&3)==0&&(fy&3)==3)return 1;
   return 0;
  }
  case TER_SWAMP:{
   for(int i=0;i<2;i++){
    int cx=4+((H15(tx*11+i,ty*3)&3)*3),cy=5+((H15(tx*2,ty*13+i)&3)*3);
    if((fx-cx)*(fx-cx)+(fy-cy)*(fy-cy)<4)return -3; // lagoa
   }
   if((fx&3)==2&&(fy&3)==2)return 2;
   return 0;
  }
  case TER_VOLCANO:{
   int cx=7+(H15(tx,ty)&1),cy=9+(H15(tx+1,ty)&1);
   if((fx-cx)*(fx-cx)+(fy-cy)*(fy-cy)<5)return 10; // lava
   if(((fx*2+fy)&15)<2)return 11;                  // fenda
   return (fy<5)?1:0;
  }
  case TER_RUINS:{
   int row=(fy+((fx>>2)*3))%7;
   if(row<1)return -3;
   if((fx&3)<1)return -3;
   return (row<3)?2:-1;
  }
  case TER_OASIS:{
   int cx=7+((H15(tx,ty>>1)&3)-1),cy=8+((H15(tx,ty)&3)-1);
   int dd=(fx-cx)*(fx-cx)+(fy-cy)*(fy-cy)*2;
   if(dd<12)return 9; // lago
   if(dd<24)return 1; // margem
   return -1;
  }
  default: break; // agua
 }
 if(((fx+fy*2)&15)<2)return 2;  // onda
 if(((fx-fy)&15)==7)return 1;   // reflexo
 if(((fx*3-fy)&31)<2)return -2; // fundo
 return 0;
}
static u16 TerOverride(int code){
 switch(code){
  case 8:  return Shc(25,25,27,0); // neve
  case 9:  return Shc(13,22,28,0); // lago
  case 10: return Shc(28,8,2,0);   // lava
  case 11: return Shc(20,9,4,0);   // brasa
  default: return 0;
 }
}

// ============ dono: tinta de faccao ============
static u16 OwnerPaintF(u8 o){
 switch(o){case 1:return Shc(9,27,14,0);case 2:return Shc(27,10,7,0);
  case 3:return Shc(28,24,4,0);default:return Shc(10,10,10,0);}
}
static u16 OwnerDeepF(u8 o){
 switch(o){case 1:return Shc(4,14,7,0);case 2:return Shc(12,4,3,0);
  case 3:return Shc(13,11,2,0);default:return Shc(5,5,6,0);}
}

// ============ borda por pixel (costa/blend) ============
// dist = 0 colado na fronteira ate 3. Regras:
//  terra->agua: linha de tinta da costa + rampa sombreada;
//  agua->terra: espuma alternada (1px) + shelf (2px);
//  mesmo tipo: blend simetrico ramp 4-dist (continuo, sem ping-pong).
static u16 EdgePaint(GameState*g,int tx,int ty,int nx,int ny,bool selfWater,u16 own,int dist,int fx,int fy){
 (void)tx;(void)ty;(void)fx;(void)fy;
 if(nx<0||ny<0||nx>=MAP_W||ny>=MAP_H)return own; // fora: sem costa/blend
 bool nWater=g->tiles[ny][nx].terrain==TER_WATER;
 if(nWater&&!selfWater){
  if(dist==0)return Shc(8,8,9,0); // tinta da costa
  if(dist==1)return Shc(own&31,(own>>5)&31,(own>>10)&31,-1);
  return own;
 }
 if(!nWater&&selfWater){
  if(dist==0)return (((fx+fy+H15(nx,ny))&1)?Shc(28,35,41,0):Shc(24,32,39,0));
  if(dist==1)return Shc(20,31,39,0); // shelf
  if(dist==2)return Shc(19,30,38,0);
  return own;
 }
 u16 nb=TerrainBase(g->tiles[ny][nx].terrain,nx,ny);
 return Mix(own,nb,4-dist,8);
}

// ============ fronteira de faccao: faixa com jitter ============
static void BandH(u16 deep,u16 paint,int bx,int by,int dir,int sd){
 for(int k=0;k<16;k++){
  int j=H15(sd,k>>2)&3,j2=H15(sd+1,k>>2)&3;
  WSET1(bx+k,by+dir*(2+j),deep);
  WSET1(bx+k,by+dir*(3+j2),paint);
  if(((k+j)&3)==0)WSET1(bx+k,by+dir*(4+j),deep); // gotas de tinta
 }
}
static void BandV(u16 deep,u16 paint,int bx,int by,int dir,int sd){
 for(int k=0;k<16;k++){
  int j=H15(sd+7,k>>2)&3,j2=H15(sd+3,k>>2)&3;
  WSET1(bx+dir*(2+j),by+k,deep);
  WSET1(bx+dir*(3+j2),by+k,paint);
  if(((k+j)&3)==0)WSET1(bx+dir*(4+j),by+k,deep);
 }
}

// ============ obra ============
static u16 CPaper(void){return Shc(26,24,18,0);}
static u16 CFace(void){return Shc(31,29,23,0);}
static u16 CChipEdge(void){return Shc(8,8,9,0);}
static void BldGlyph(u8 bld,int bx,int by){
 u16 paper=CPaper(),ink=CChipEdge(),face=CFace();
 if(bld==BLD_WALL||bld==BLD_FORT)paper=Shc(22,21,19,0);
 MiniFill(bx,by,3,3,12,12,paper);
 for(int k=3;k<=12;k++){Lpx(bx,by,k,3,ink);Lpx(bx,by,12,k,ink);Lpx(bx,by,3,k,ink);Lpx(bx,by,k,12,ink);}
 MiniFill(bx,by,4,7,11,10,face);
 switch(bld){
  case BLD_FARM:
   LineP(bx,by,4,8,8,4,ink);LineP(bx,by,8,4,11,8,ink);
   Lpx(bx,by,7,9,ink);Lpx(bx,by,8,9,ink);
   break;
  case BLD_ORCHARD:
   LineP(bx,by,4,8,7,5,ink);LineP(bx,by,7,5,10,8,ink);
   Lpx(bx,by,4,5,Shc(8,22,10,0));Lpx(bx,by,3,6,Shc(8,22,10,0));Lpx(bx,by,4,6,Shc(8,22,10,0));
   Lpx(bx,by,7,9,ink);
   break;
  case BLD_MINE: case BLD_COALPIT:
   LineP(bx,by,4,10,11,4,ink);LineP(bx,by,7,8,10,10,ink);
   if(bld==BLD_COALPIT){Lpx(bx,by,5,9,Shc(4,4,5,0));Lpx(bx,by,6,9,Shc(4,4,5,0));Lpx(bx,by,5,10,Shc(4,4,5,0));}
   else{Lpx(bx,by,6,5,Shc(4,4,5,0));Lpx(bx,by,7,5,Shc(4,4,5,0));}
   break;
  case BLD_SAWMILL:
   LineP(bx,by,4,8,7,5,ink);LineP(bx,by,7,5,10,8,ink);
   Lpx(bx,by,11,7,ink);Lpx(bx,by,11,9,ink);Lpx(bx,by,10,8,ink);
   break;
  case BLD_TEXTILE:
   LineP(bx,by,5,4,5,11,ink);LineP(bx,by,10,4,10,11,ink);
   LineP(bx,by,5,6,10,5,ink);LineP(bx,by,5,9,10,10,ink);
   break;
  case BLD_STEEL:
   MiniFill(bx,by,5,4,7,10,ink);
   MiniFill(bx,by,9,8,11,10,Shc(28,12,3,0));
   break;
  case BLD_TOOLSHOP:
   LineP(bx,by,4,10,9,5,Shc(14,11,7,0));
   MiniFill(bx,by,9,4,12,6,ink);
   break;
  case BLD_WINERY:
   LineP(bx,by,6,4,6,11,ink);LineP(bx,by,10,4,10,11,ink);
   LineP(bx,by,5,6,11,6,ink);LineP(bx,by,5,9,11,9,ink);
   break;
  case BLD_GLASSWORKS:
   LineP(bx,by,4,10,4,6,ink);LineP(bx,by,4,6,7,4,ink);LineP(bx,by,7,4,11,6,ink);
   MiniFill(bx,by,6,7,9,9,Shc(14,23,26,0));
   break;
  case BLD_MARKET:
   for(int k=4;k<=11;k++){
    u16 aw=(k&1)?CChipEdge():Shc(30,28,10,0);
    Lpx(bx,by,k,4,aw);Lpx(bx,by,k,5,aw);
   }
   LineP(bx,by,4,7,11,7,ink);
   break;
  case BLD_BANK:
   LineP(bx,by,4,6,8,4,ink);LineP(bx,by,8,4,11,6,ink);
   LineP(bx,by,5,8,5,10,ink);LineP(bx,by,8,8,8,10,ink);LineP(bx,by,11,8,11,10,ink);
   MiniFill(bx,by,4,10,11,10,ink);
   break;
  case BLD_UNIV:
   MiniFill(bx,by,5,4,9,4,ink);
   LineP(bx,by,5,5,5,10,ink);LineP(bx,by,9,5,9,10,ink);
   MiniFill(bx,by,6,6,8,9,face);
   break;
  case BLD_THEATER:
   LineP(bx,by,4,9,7,4,ink);LineP(bx,by,4,9,7,7,ink);
   LineP(bx,by,9,4,11,9,ink);LineP(bx,by,9,7,11,9,ink);
   break;
  case BLD_HOSPITAL:
   MiniFill(bx,by,7,5,9,11,CFace());
   MiniFill(bx,by,5,7,11,9,CFace());
   break;
  case BLD_BARRACKS:
   LineP(bx,by,4,10,8,5,ink);LineP(bx,by,8,5,11,10,ink);
   MiniFill(bx,by,7,8,9,9,ink);
   break;
  case BLD_WALL:
   for(int k=4;k<=11;k+=2){Lpx(bx,by,k,3,ink);Lpx(bx,by,k+1,3,ink);}
   MiniFill(bx,by,4,7,11,11,Shc(19,18,17,0));
   for(int k=5;k<=11;k+=3){MiniFill(bx,by,k,8,k,10,Shc(14,13,12,0));}
   break;
  case BLD_FORT:
   for(int k=4;k<=11;k+=2){Lpx(bx,by,k,3,ink);Lpx(bx,by,k+1,3,ink);}
   MiniFill(bx,by,6,4,9,10,Shc(20,19,18,0));
   LineP(bx,by,9,4,11,3,Shc(28,8,8,0));
   break;
  case BLD_PORT: case BLD_DOCKYARD:
   MiniFill(bx,by,4,9,11,11,Shc(10,13,25,0));
   LineP(bx,by,7,4,7,8,ink);LineP(bx,by,9,4,7,6,ink);
   if(bld==BLD_PORT){Lpx(bx,by,8,5,Shc(4,4,5,0));Lpx(bx,by,8,6,Shc(4,4,5,0));}
   else{MiniFill(bx,by,10,6,11,7,Shc(13,13,14,0));}
   break;
  case BLD_SHRINE:
   LineP(bx,by,8,4,6,10,ink);LineP(bx,by,8,4,10,10,ink);
   MiniFill(bx,by,5,11,11,11,ink);
   break;
  case BLD_CUSTOMS:
   LineP(bx,by,8,4,8,10,ink);MiniFill(bx,by,7,10,9,11,ink);
   LineP(bx,by,4,6,11,6,ink);
   Lpx(bx,by,5,7,ink);Lpx(bx,by,11,7,ink);
   break;
  default: break;
 }
}
static void SitePip(u8 site,int bx,int by){
 u16 c;
 switch(site){
  case SITE_GOLDMINE:c=Shc(30,26,8,0);break;
  case SITE_IRON:    c=Shc(24,10,8,0);break;
  case SITE_FERTILE: c=Shc(8,24,10,0);break;
  case SITE_RUINARM: c=Shc(20,19,18,0);break;
  case SITE_CURSED:  c=Shc(18,4,18,0);break;
  case SITE_COAL:    c=Shc(6,6,7,0);break;
  default:return;
 }
 Lpx(bx,by,2,3,c);Lpx(bx,by,3,2,c);Lpx(bx,by,3,3,c);Lpx(bx,by,3,4,c);Lpx(bx,by,4,3,c);
}
// escudo das tropas: fill da faccao + pips brancos + seta gold (marcha)
static void ArmyShield(u8 owner,int troops,int dirx,int diry,int bx,int by,bool onBld){
 u16 ink=CChipEdge(),pips=Shc(31,29,24,0),flg=Shc(31,24,8,0);
 u16 fill=OwnerPaintF(owner); if(!(fill&0x8000))fill=Shc(14,13,14,0);
 if(troops<=0)return;
 int x0,y0,x1,y1;
 if(onBld){x0=8;y0=1;x1=12;y1=5;} // canto sup dir (tile com obra)
 else{x0=5;y0=4;x1=12;y1=11;}    // centro (tile sem obra)
 MiniFill(bx,by,x0+1,y0+1,x1-1,y1-1,fill);
 for(int k=x0;k<=x1;k++){Lpx(bx,by,k,y0,ink);Lpx(bx,by,k,y1,ink);}
 for(int k=y0;k<=y1;k++){Lpx(bx,by,x0,k,ink);Lpx(bx,by,x1,k,ink);}
 int n=troops; if(n>12)n=12;
 for(int i=0;i<n;i++){
  int px=x0+1+((i&1)*2), py=y0+1+(i>>1);
  if(py>y1-1)py=y1-1;
  Lpx(bx,by,px,py,pips);
 }
 if(dirx||diry){
  int cx=(x0+x1)/2, cy=(y0+y1)/2;
  if(dirx){
   LineP(bx,by,cx-1,cy,cx+1,cy,flg);
   Lpx(bx,by,cx+dirx*2,cy,flg);
   Lpx(bx,by,cx+dirx,cy+diry?0:1,flg);
  }else{
   LineP(bx,by,cx,cy-1,cx,cy+1,flg);
   Lpx(bx,by,cx,cy+diry*2,flg);
  }
 }
}

// ============ assinatura / bake ============
static u32 TileKey(GameState*g,int tx,int ty){
 Tile*t=&g->tiles[ty][tx];
 u32 dst=0;
 if(t->dstX!=0xFF){
  int dx=(int)t->dstX-tx,dy=(int)t->dstY-ty;
  dst=(1u<<19)|((dx>0?1u:dx<0?2u:0)<<20)|((dy>0?1u:dy<0?2u:0)<<22);
 }
 return (u32)t->owner|(u32)t->terrain<<3|(u32)t->building<<7
  |(u32)t->site<<12|(u32)t->troops<<15|dst;
}
static void BakeTile(GameState*g,int tx,int ty){
 Tile*t=&g->tiles[ty][tx];
 int bx=tx*16,by=ty*16;
 bool selfWater=(t->terrain==TER_WATER);
 u16 base=TerrainBase(t->terrain,tx,ty);
 for(int fy=0;fy<16;fy++)for(int fx=0;fx<16;fx++){
  int d=TerTex(t->terrain,tx,ty,fx,fy);
  u16 own;
  if(d>4)own=TerOverride(d);
  else own=Shc((int)(base&31),(int)((base>>5)&31),(int)((base>>10)&31),d);
  u16 eo=own;
  if(fx<4)  eo=EdgePaint(g,tx,ty,tx-1,ty,selfWater,eo,fx,fx,fy);
  if(fx>=12)eo=EdgePaint(g,tx,ty,tx+1,ty,selfWater,eo,15-fx,fx,fy);
  if(fy<4)  eo=EdgePaint(g,tx,ty,tx,ty-1,selfWater,eo,fy,fx,fy);
  if(fy>=12)eo=EdgePaint(g,tx,ty,tx,ty+1,selfWater,eo,15-fy,fx,fy);
  WSET1(bx+fx,by+fy,eo);
 }
 u8 o=t->owner;
 if(o){ // fronteiras antes dos glifos (o chip "corta" a faixa: ok)
  u16 paint=OwnerPaintF(o),deep=OwnerDeepF(o);
  if(ty>0&&g->tiles[ty-1][tx].owner!=o)BandH(deep,paint,bx,by,1,(tx*13+ty*7)&63);
  if(ty+1<MAP_H&&g->tiles[ty+1][tx].owner!=o)BandH(deep,paint,bx,by,-1,(tx*11+ty*5)&63);
  if(tx>0&&g->tiles[ty][tx-1].owner!=o)BandV(deep,paint,bx,by,1,(tx*17+ty*3)&63);
  if(tx+1<MAP_W&&g->tiles[ty][tx+1].owner!=o)BandV(deep,paint,bx,by,-1,(tx*7+ty*19)&63);
 }
 SitePip(t->site,bx,by);
 if(t->building)BldGlyph(t->building,bx,by);
 if(t->troops){
  int dx=0,dy=0;
  if(t->dstX!=0xFF){
   int ax=(int)t->dstX-tx,ay=(int)t->dstY-ty;
   dx=ax>0?1:(ax<0?-1:0); dy=ay>0?1:(ay<0?-1:0);
  }
  ArmyShield(t->owner,t->troops,dx,dy,bx,by,t->building!=0);
 }
}
// passa: bake tiles mudados (visiveis primeiro), devolve budget restante
static int BakePass(GameState*g,int budget,bool visOnly){
 int remaining=budget;
 for(int ty=0;ty<MAP_H;ty++)for(int tx=0;tx<MAP_W;tx++){
  if(remaining<=0)return 0;
  if(visOnly){
   int vx2=tx-(int)g->camX,vy2=ty-(int)g->camY;
   if(vx2<0||vy2<0||vx2>=VIEW_W||vy2>=VIEW_H)continue;
  }
  u32 k=TileKey(g,tx,ty);
  if(SNAP[ty*MAP_W+tx]==k)continue;
  BakeTile(g,tx,ty);
  SNAP[ty*MAP_W+tx]=k;
  remaining--;
 }
 return remaining;
}
void Px_Tick(GameState*g){
 int budget=g->dirty?200:40;
 int rem=BakePass(g,budget,true);
 if(rem>0)BakePass(g,rem*2,false);
}

// ============ minimapa (px_hud usa) ============
u16 Px_MiniCol(GameState*g,u8 tx,u8 ty){
 Tile*t=&g->tiles[ty][tx];
 if(t->owner)return OwnerPaintF(t->owner);
 switch(t->terrain){
  case TER_WATER:   return Shc(13,22,30,0);
  case TER_FOREST:  return Shc(14,23,12,0);
  case TER_HILLS:   return Shc(22,21,15,0);
  case TER_MOUNTAIN:return Shc(20,19,20,0);
  case TER_DESERT:  return Shc(27,23,15,0);
  case TER_SWAMP:   return Shc(13,17,12,0);
  case TER_VOLCANO: return Shc(22,10,8,0);
  case TER_RUINS:   return Shc(18,17,16,0);
  case TER_OASIS:   return Shc(15,24,30,0);
  default:          return Shc(19,26,15,0);
 }
}

// ============ viewport bake->tela + overlays ao vivo ============
static void ViewportOut(GameState*g,u16*d){
 int cx0=(int)(g->camX)*16, cy0=(int)(g->camY)*16;
 for(int vy=0;vy<VIEW_H*16;vy++){
  u16*s=WORLD+((cy0+vy)*PX_WORLD_W)+cx0;
  for(int i=0;i<VIEW_W*16;i++)*d++=*s++;
 }
}
// cursor: tile clareado + brackets gold pulsando
static void CursorPaint(u16*d,GameState*g,int ph){
 int sx=(g->selX-(int)g->camX)*16, sy=(g->selY-(int)g->camY)*16;
 if(sx<0||sy<0||sx>=VIEW_W*16||sy>=VIEW_H*16)return;
 for(int j=0;j<16;j++)for(int i=0;i<16;i++){
  u16 c=d[(sy+j)*256+(sx+i)];
  int r=c&31,g2=(c>>5)&31,b=(c>>10)&31;
  d[(sy+j)*256+(sx+i)]=Px_Col((u8)Clamp31(r+2),(u8)Clamp31(g2+2),(u8)Clamp31(b+2));
 }
 u16 gold=ph?Shc(31,28,16,0):Shc(18,16,9,0);
 // brackets 4 cantos (L de 3px, 2 de espessura: leitura firme em 16px)
 for(int m=0;m<3;m++){
  Lbrk(d,sx,sy,1,1,gold);Lbrk(d,sx+15,sy,-1,1,gold);
  Lbrk(d,sx,sy+15,1,-1,gold);Lbrk(d,sx+15,sy+15,-1,-1,gold);
 }
}
// canto "L" 3x3 no viewport (d, base x,y, sentido)
static void Lbrk(u16*d,int x,int y,int sx,int sy,u16 c){
 for(int k=0;k<3;k++){
  int hx=x+sx*k, hy=y;
  int vy=0;
  (void)vy;
  if(hx>=0&&hx<256&&hy>=0&&hy<192)d[hy*256+hx]=c;
 }
 Lbrk2 elsewhere...
}
