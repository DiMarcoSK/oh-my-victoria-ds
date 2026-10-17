// px_core.c — framebuffer duplo MODE_5_2D, paleta e primitivas.
// Baseado na API publica libnds/BlocksDS: videoSetMode(MODE_5_2D),
// vramSetBankA(VRAM_A_MAIN_BG 0x06000000), bgInit(3,BgType_Bmp16,
// BgSize_B16_256x256,0,0), BG_BMP_RAM(0)/BG_BMP_RAM_SUB(0), RGB15(r,g,b).
#include "px.h"
// OTIMIZACAO DS: buffers estaticos em .bss (2x 256x192x2 = 200KB), ZERO malloc.
// malloc puxaria sbrk/syscalls e fragmentaria o heap de 4MB; estatico e'
// deterministico e some do caminho do link (sem -lnosys necessario p/ isso).
static u16 backTop[256*192], backBot[256*192];
u16 *Px_TopBuf(void){return BG_BMP_RAM(0);}
u16 *Px_BotBuf(void){return BG_BMP_RAM_SUB(0);}
// BIT(15)=alpha opaco: em Bmp16 (aBBBBBGGGGGRRRRR) pixel sem 'a' e'
// TRANSPARENTE (vira backdrop). RGB15() puro nunca seta o bit -> tela preta.
// Todo pixel do jogo passa por aqui, entao OR uma vez resolve tudo.
u16 Px_Col(u8 r,u8 g,u8 b){return (u16)(RGB15(r,g,b)|BIT(15));}
void Px_PSet(u16 *b,int x,int y,u16 c){
 if(x<0||x>255||y<0||y>191)return;
 b[y*256+x]=c;
}
void Px_Rect(u16 *b,int x,int y,int w,int h,u16 c){
 if(x<0){w+=x;x=0;}
 if(y<0){h+=y;y=0;}
 if(x+w>256)w=256-x;
 if(y+h>192)h=192-y;
 for(int j=0;j<h;j++){u16 *row=b+(y+j)*256+x;
  for(int i=0;i<w;i++)row[i]=c;}
}
void Px_Border(u16 *b,int x,int y,int w,int h,u16 c){
 for(int i=0;i<w;i++){Px_PSet(b,x+i,y,c);Px_PSet(b,x+i,y+h-1,c);}
 for(int j=0;j<h;j++){Px_PSet(b,x,y+j,c);Px_PSet(b,x+w-1,y+j,c);}
}
void Px_Init(void){
 videoSetMode(MODE_5_2D);
 videoSetModeSub(MODE_5_2D);
 vramSetBankA(VRAM_A_MAIN_BG);
 vramSetBankC(VRAM_C_SUB_BG);
 bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 u16 black=RGB15(0,0,0);
 for(int i=0;i<256*192;i++)backTop[i]=black, backBot[i]=black;
 dmaCopy(backTop,BG_BMP_RAM(0),256*192*2);
 dmaCopy(backBot,BG_BMP_RAM_SUB(0),256*192*2);
}
// O DMA le a RAM principal direto: o que a CPU escreveu ainda pode estar so'
// no cache de dados (write-back) -> a tela mostraria pixels velhos (restos de
// sprite). Esvazia o cache do buffer antes de copiar.
void Px_FlipTop(void){DC_FlushRange(backTop,256*192*2);dmaCopy(backTop,BG_BMP_RAM(0),256*192*2);}
void Px_FlipBot(void){DC_FlushRange(backBot,256*192*2);dmaCopy(backBot,BG_BMP_RAM_SUB(0),256*192*2);}
// pedido de redesenho so' da tela de cima (mapa andando: o painel nao muda)
static bool topWant;
void Px_WantTop(void){topWant=true;}
bool Px_TakeTop(void){bool t=topWant;topWant=false;return t;}
u16 *Px_BackTop(void){return backTop;}
u16 *Px_BackBot(void){return backBot;}
void Px_Clear(u16 *b,u16 c){for(int i=0;i<256*192;i++)b[i]=c;}
