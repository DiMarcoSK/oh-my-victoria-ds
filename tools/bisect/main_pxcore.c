// main_pxcore.c — DEGRAU 2 do bisect: usa a PROPRIA engine de video do jogo
// (source/px_core.c -> Px_Init/Px_Col/Px_Clear/Px_Flip) + os tokens de
// include/pal.h (v0.9) + a fonte Serifa (px_font2.c/font_data.c).
// Leitura do resultado:
//   tela de cima NAVY opaca + texto dourado  -> px_core+pal+font OK;
//   qualquer tela PRETA                       -> alpha/transparencia ou draw.
// (px_core.c e o MESMO arquivo do jogo: se aqui aparecer, o caminho de video
// do jogo esta bom e o problema esta no desenho/paginas.)
#include <nds.h>
#include "px.h"
#include "pal.h"
#include "font.h"
int main(void){
 Px_Init();
 u16*d=Px_BackTop();
 Px_Clear(d,C_NAVY_BG);
 Px_Text2(d,6,8,"PX-CORE + PAL.H OK",&F_SERIF8,C_GOLD);
 Px_Text2(d,6,30,"Serifa 12: Imperio Acucar",&F_SERIF12,C_TEXT);
 // acentos em UTF-8 cru (bytes explicitos): A c-cedilha a-til o-til
 Px_Text2(d,6,54,"acentos: A\xC3\x87\xC3\xA3o \xC3\xB5",&F_SERIF8,C_TEXT);
 Px_Text2(d,6,74,"abcdefghijklmnopqrstuvwxyz",&F_SERIF8,C_TEXT_DIM);
 Px_Text2(d,6,88,"0123456789 ABCDEFGHIJKLM",&F_SERIF8,C_TEXT_DIM);
 Px_Text2(d,6,110,"SE ISTO NAO APARECER -> VIDEO",&F_SERIF12,C_CRIMSON);
 Px_FlipTop();
 u16*b=Px_BackBot();
 Px_Clear(b,C_CRIMSON);
 Px_Text2(b,6,8,"TELA BAIXO VERMELHA OPACA",&F_SERIF8,C_WHITE);
 Px_FlipBot();
 while(1)swiWaitForVBlank();
 return 0;
}
