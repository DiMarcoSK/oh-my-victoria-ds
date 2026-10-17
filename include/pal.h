#pragma once
#include <nds.h>
// pal.h — tokens de cor da UI (v0.9, SDD sec 14.1). Fabrica UNICA de cor de
// COMPILACAO: PXC(r,g,b) e' o gemeo do Px_Col (source/px_core.c) e, como ele,
// seta BIT(15) (alpha opaco). Em framebuffer Bmp16 (aBBBBBGGGGGRRRRR) um pixel
// sem o bit 15 e' TRANSPARENTE (mostra o backdrop) -> "tela preta com o jogo
// rodando invisivel". Por isso e' proibido RGB15() cru em pixel de tela
// (ADR-13). 5 bits por canal, mesmo clamp do hardware.
//
// Regra: nenhuma cor de UI literal solta no codigo. Ou e' um token daqui, ou
// e' um PXC(...)/Px_Col(...) pontual e justificado (arte do mapa, px_map.c).
#define PXC(r,g,b) ((u16)(BIT(15)|(((u16)(b)&31)<<10)|(((u16)(g)&31)<<5)|((u16)(r)&31)))

// --- tokens canonicos (SDD sec 14.1) ---
#define C_NAVY_BG    PXC(2,3,4)     // fundo de tela, moldura externa
#define C_NAVY       PXC(3,4,7)     // paineis e botoes escuros, barra do HUD
#define C_NAVY_HI    PXC(5,7,10)    // botao pressionado, placeholder de arte
#define C_PARCH      PXC(27,23,18)  // pergaminho (conteudo; papel da concept 217,186,144)
#define C_PARCH_SH   PXC(22,18,13)  // manchas/gravuras do pergaminho
#define C_INK        PXC(5,4,3)     // texto e tracos sobre pergaminho
#define C_CRIMSON    PXC(19,3,3)    // item selecionado
#define C_CRIMSON_HI PXC(24,6,5)    // brilho de 1px no topo da selecao
#define C_GOLD       PXC(25,20,9)   // bordas, ornamentos, icones
#define C_GOLD_DIM   PXC(15,12,6)   // separadores, borda interna, icone off
#define C_TEXT       PXC(29,28,26)  // texto sobre navy
#define C_TEXT_DIM   PXC(17,17,19)  // item desabilitado

// --- tokens de PAPEL da v0.8 (nao estao na tabela 14.1; a F2 os revisa contra
//     a concept). Sao papeis, nao cores soltas: o HUD/menu da v0.8 ja os usa. ---
#define C_PLAYER     PXC(9,25,13)   // territorio/estado do jogador (verde)
#define C_RIVAL_A    PXC(27,10,7)   // rival Norte (vermelho)
#define C_RIVAL_B    PXC(28,24,4)   // rival Sul (mostarda)
#define C_WARM       PXC(24,18,8)   // grao / destaque quente
#define C_WARM_DIM   PXC(20,15,7)   // sombra do grao
#define C_GOLD_HI    PXC(31,27,14)  // brilho de moeda/icone
#define C_WHITE      PXC(31,31,31)  // moldura do viewport do minimapa
#define C_EVENT      PXC(28,12,10)  // mensagem de evento (alerta)
#define C_SEA_WAVE   PXC(8,13,24)   // ornamento de onda (menu)
#define C_BRUSH      PXC(27,9,7)    // pincelada vermelha do logo
