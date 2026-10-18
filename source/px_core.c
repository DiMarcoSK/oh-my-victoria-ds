// px_core.c — framebuffers, ritmo de quadro e primitivas (SDD ADR-36).
// Baseado na API publica libnds/BlocksDS: videoSetMode(MODE_5_2D),
// vramSetBankA/B (BG principal 0x06000000/0x06020000), bgInit(3,BgType_Bmp16,
// BgSize_B16_256x256,...), bgSetMapBase (pagina do bitmap), RGB15(r,g,b).
#include "px.h"
#include "hot.h"
#ifdef ARM9
#include <calico/system/tick.h>
#endif
// TELA DE CIMA (motor principal) = PAGE FLIP NA VRAM: dois bitmaps 256x256
// (banco A em 0x06000000 = base 0, banco B em 0x06020000 = base 8). O jogo
// desenha direto na pagina escondida; Px_FlipTop so' marca, e Px_VSync (logo
// apos o VBlank) troca a base do BG3. Antes: desenho na RAM principal +
// DMA de 96KB no meio do quadro = ~3 ms de CPU parada e TEARING (a copia
// cruzava o feixe). Agora a troca e' 1 escrita de registrador, sem rasgo.
// Toda tela de cima redesenha por inteiro (a pagina escondida tem o quadro
// de 2 trocas atras), entao nada depende do conteudo antigo.
// VRAM nao aceita escrita de 8 bits (o byte some): nada de memcpy/memset da
// newlib nela (caem em strb quando desalinhados). Copia = Px_Copy16.
// TELA DE BAIXO (motor sub, so' o banco C): buffer na RAM (.bss) + DMA, mas
// disparado no INICIO do VBlank (Px_VSync): o DMA (~3 ms) corre 4x mais rapido
// que o feixe, entao termina cada linha antes de ela ser lida = sem rasgo.
static u16 backBot[256*192] __attribute__((aligned(32)));
static int topBg,topBack=1;          // id do BG3 principal; pagina escondida
static bool topPend,botPend;
static u8 vbN=1;
static u32 vsT;                      // tick do ultimo VBlank (folga do quadro)
static inline u16 *TopPage(int p){return p?BG_BMP_RAM(8):BG_BMP_RAM(0);}
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
static void Fill32(u16 *d,int n,u16 c){u32a *p=(u32a*)d;u32 v=c|((u32)c<<16);for(int i=0;i<n/2;i++)p[i]=v;}
void Px_Init(void){
 videoSetMode(MODE_5_2D);
 videoSetModeSub(MODE_5_2D);
 vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
 vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
 vramSetBankC(VRAM_C_SUB_BG);
 topBg=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 // preto sem alpha = backdrop preto (o mesmo visual) ate' o 1o desenho
 Fill32(TopPage(0),256*192,0);Fill32(TopPage(1),256*192,0);
 Fill32(backBot,256*192,0);
 Fill32(BG_BMP_RAM_SUB(0),256*192,0);
 topBack=1;bgSetMapBase(topBg,0);
}
void Px_FlipTop(void){topPend=true;}
void Px_FlipBot(void){botPend=true;}
// chamado logo apos swiWaitForVBlank: troca a pagina de cima, sobe o painel
// e mede quantos VBlanks o quadro levou (o ritmo da camera/simulacao).
static s8 fadeLv;                    // brilho mestre (0 = normal; -16 = preto)
void Px_FadeIn(int lv){if(lv<1)lv=1;if(lv>12)lv=12;if(-lv<fadeLv)fadeLv=(s8)-lv;}
int Px_Fade(void){return fadeLv;}
void Px_VSync(void){
 // transicao de tela: clareia 2 niveis por VBlank (~6 quadros do mais escuro)
 if(fadeLv<0){fadeLv=(s8)(fadeLv+2>0?0:fadeLv+2);
#ifdef ARM9
  setBrightness(3,fadeLv);
#endif
 }
#ifdef OMV_ARMBENCH
 vsT=Px_Ticks();
#endif
 if(topPend){bgSetMapBase(topBg,topBack?8:0);topBack^=1;topPend=false;}
 if(botPend){DC_FlushRange(backBot,sizeof backBot);dmaCopyWords(3,backBot,BG_BMP_RAM_SUB(0),sizeof backBot);botPend=false;}
#ifdef ARM9
 // 1 quadro = 263 linhas x 2130 ciclos de 33,51 MHz = 560190; tick = /64
 static u64 last;u64 t=tickGetCount();vsT=(u32)t;
 u32 d=(u32)(t-last);last=t;
 u32 n=(d+4376)/8753;
 vbN=(u8)(n<1?1:(n>4?4:n));       // teto: depois de um tranco, nao teleporta
#endif
}
// Tela aparece JA' (antes de um trabalho longo que trava o laco, ex.: gerar
// o mundo): espera o VBlank e troca.
void Px_Present(void){swiWaitForVBlank();Px_VSync();}
int Px_Vb(void){return vbN;}
// relogio p/ orcamento de trabalho no quadro (1 tick = 1/523656 s, tick do
// calico). No host fica parado: o orcamento vira so' o teto de passos, e o
// smoke continua deterministico.
#ifdef OMV_ARMBENCH
// tools/armbench: o emulador escreve aqui ciclos estimados / 128 (= ticks do
// calico a 67 MHz, instrucoes x 1,4): orcamento e render-ahead como no DS
volatile u32 bench_cticks;
#endif
__attribute__((noinline)) u32 Px_Ticks(void){
#if defined(ARM9)
 return (u32)tickGetCount();
#elif defined(OMV_ARMBENCH)
 __asm__ volatile("");return bench_cticks;
#else
 return 0;
#endif
}
u32 Px_FrameTicks(void){return Px_Ticks()-vsT;}
static bool topWant;
void Px_WantTop(void){topWant=true;}
bool Px_TakeTop(void){bool t=topWant;topWant=false;return t;}
u16 *Px_BackTop(void){return TopPage(topBack);}
u16 *Px_BackBot(void){return backBot;}
void Px_Clear(u16 *b,u16 c){Fill32(b,256*192,c);}

HOT_BEGIN
// copia n pixels de 16 bits so' com acessos de 16/32 bits (seguro na VRAM).
// Destino alinhado em 4 apos no maximo 1 pixel; origem alinhada = 8 palavras
// por volta (ldm/stm); desalinhada = junta 2 meias-palavras por palavra.
HOT_ITCM void Px_Copy16(u16 *d,const u16 *s,int n){
 if(n<=0)return;
 if((u32)(uintptr_t)d&2){*d++=*s++;n--;}
 u32a *D=(u32a*)d;
 if(!((u32)(uintptr_t)s&2)){
  const u32a *S=(const u32a*)s;
#if defined(ARM9)||defined(OMV_ARMBENCH)
  // 8 palavras por ldm/stm: 2 instrucoes por 16 pixels e acesso em RAJADA
  // sequencial no barramento (o GCC gerava 16 ldr/str soltos)
  for(;n>=16;n-=16)
   __asm__ volatile("ldmia %0!,{r3-r10}\n\tstmia %1!,{r3-r10}":"+r"(S),"+r"(D)::"r3","r4","r5","r6","r7","r8","r9","r10","memory");
#else
  for(;n>=16;n-=16,D+=8,S+=8){
   u32 a=S[0],b=S[1],c=S[2],e=S[3],f=S[4],g=S[5],h=S[6],i=S[7];
   D[0]=a;D[1]=b;D[2]=c;D[3]=e;D[4]=f;D[5]=g;D[6]=h;D[7]=i;
  }
#endif
  for(;n>=2;n-=2)*D++=*S++;
  if(n)*(u16*)D=*(const u16*)S;
 }else{
  for(;n>=2;n-=2,s+=2)*D++=(u32)s[0]|((u32)s[1]<<16);
  if(n)*(u16*)D=*s;
 }
}
HOT_END
