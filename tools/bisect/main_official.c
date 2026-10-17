// main_official.c — DEGRAU 1 do bisect do postmortem (docs/POSTMORTEM-TELA-BRANCA.md
// sec 6.1/7.4): caminho INDEPENDENTE do jogo. videoSetMode + bgInit Bmp16 cru.
// Se aparecer texto embaixo e a tela de cima VERMELHA, boot+toolchain+libnds OK.
// Se a de cima ficar PRETA com o texto visivel -> problema de alpha/transparencia.
// Se nada aparecer -> boot/toolchain.
#include <nds.h>
#include <stdio.h>
int main(void){
 videoSetMode(MODE_5_2D);
 videoSetModeSub(MODE_0_2D);
 vramSetBankA(VRAM_A_MAIN_BG);
 consoleDemoInit();
 bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 // vermelho OPACO: RGB15(31,0,0)|BIT(15). (Se ficar preto, o OR foi perdido.)
 for(int i=0;i<256*192;i++)BG_GFX[i]=(u16)(RGB15(31,0,0)|BIT(15));
 iprintf("BISECT DEGRAU 1 (oficial/libnds)\n");
 iprintf("Se você LE isto: boot+libnds OK.\n");
 iprintf("Tela de CIMA deve estar VERMELHA opaca.\n");
 while(1)swiWaitForVBlank();
 return 0;
}
