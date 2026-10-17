// Stub minimo de <nds.h> para rodar source/*.c no PC (x86) sob ASan/UBSan.
// NAO e' a libnds: so' os tipos/macros/funcoes que o jogo usa de fato.
// Existe para o smoke de render (tools/hostpx) pegar hang e pixel transparente
// sem precisar de melonDS/DS real. Nao entra no build do .nds.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

#ifndef BIT
#define BIT(n) (1 << (n))
#endif
#define RGB15(r,g,b) ((u16)(((r)&31) | ((((g)&31))<<5) | ((((b)&31))<<10)))

#define MODE_5_2D          5
#define BgType_Bmp16       2
#define BgSize_B16_256x256 0
#define VRAM_A_MAIN_BG     1
#define VRAM_A_MAIN_BG_0x06000000 1
#define VRAM_B_MAIN_BG_0x06020000 2
#define VRAM_C_SUB_BG      3

#define KEY_A      (1u<<0)
#define KEY_B      (1u<<1)
#define KEY_SELECT (1u<<2)
#define KEY_START  (1u<<3)
#define KEY_RIGHT  (1u<<4)
#define KEY_LEFT   (1u<<5)
#define KEY_UP     (1u<<6)
#define KEY_DOWN   (1u<<7)
#define KEY_R      (1u<<8)
#define KEY_L      (1u<<9)
#define KEY_X      (1u<<10)
#define KEY_Y      (1u<<11)
#define KEY_TOUCH  (1u<<12)

typedef struct { s32 px, py; u16 x, y; } touchPosition;

// VRAM principal: 2 paginas 256x256 (bancos A+B, base em blocos de 16KB)
extern u16 host_vram_main[256*256*2];
extern u16 host_bmp_sub[256*256];
#define BG_BMP_RAM(n)     (host_vram_main+(n)*8192)
#define BG_BMP_RAM_SUB(n) (host_bmp_sub+(n)*8192)

void videoSetMode(int m);
void videoSetModeSub(int m);
void vramSetBankA(int v);
void vramSetBankB(int v);
void vramSetBankC(int v);
void bgSetMapBase(int id,unsigned base);
void dmaCopyWords(int ch,const void*src,void*dst,u32 size);
static inline bool isDSiMode(void){return false;}
static inline bool setCpuClock(bool s){(void)s;return false;}
int  bgInit(int bg,int type,int size,int mapbase,int tilebase);
int  bgInitSub(int bg,int type,int size,int mapbase,int tilebase);
void dmaCopy(const void*src,void*dst,u32 size);
static inline void DC_FlushRange(const void*p,u32 n){(void)p;(void)n;}
void swiWaitForVBlank(void);
void scanKeys(void);
u32  keysDown(void);
u32  keysHeld(void);
void touchRead(touchPosition*t);
void lcdSwap(void);
void lcdMainOnTop(void);
void lcdMainOnBottom(void);
// injecao de entrada do smoke (tools/hostpx/main.c)
extern u32 host_down, host_held; extern int host_tx, host_ty;
