// Implementacao no-op das funcoes libnds usadas pelo smoke host (tools/hostpx).
#include "nds.h"

u16 host_vram_main[256*256*2];
u16 host_bmp_sub[256*256];

void videoSetMode(int m){(void)m;}
void videoSetModeSub(int m){(void)m;}
void vramSetBankA(int v){(void)v;}
void vramSetBankB(int v){(void)v;}
void vramSetBankC(int v){(void)v;}
void bgSetMapBase(int id,unsigned b){(void)id;(void)b;}
void dmaCopyWords(int ch,const void*s,void*d,u32 n){(void)ch;memcpy(d,s,n);}
int  bgInit(int a,int b,int c,int d,int e){(void)a;(void)b;(void)c;(void)d;(void)e;return 0;}
int  bgInitSub(int a,int b,int c,int d,int e){(void)a;(void)b;(void)c;(void)d;(void)e;return 0;}
void dmaCopy(const void*s,void*d,u32 n){memcpy(d,s,n);}
void swiWaitForVBlank(void){}
void scanKeys(void){}
u32 host_down=0, host_held=0; int host_tx=0, host_ty=0;
u32  keysDown(void){return host_down;}
u32  keysHeld(void){return host_held;}
void touchRead(touchPosition*t){if(t){t->px=host_tx;t->py=host_ty;}}
void lcdSwap(void){}
void lcdMainOnTop(void){}
void lcdMainOnBottom(void){}
