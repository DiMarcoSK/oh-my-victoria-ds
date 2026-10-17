#pragma once
#include "types.h"
// prof.h — medidor de quadro (SDD ADR-36). Conta quanto cada secao do quadro
// gasta em ticks do barramento (33,514 MHz no DS = TIMER2+3 em cascata via
// cpuStartTiming; no host, relogio monotonico convertido p/ a mesma escala).
// Build normal: PROF() vira so' o comando (custo zero). `make PROF=1` liga
// OMV_PROF: o canto da tela de cima mostra ms por secao + quadros perdidos,
// p/ medir no melonDS/DS real sem adivinhar (o host so' da' a ordem relativa).
enum {
 PF_IN=0,   // In_Handle (entrada + camera)
 PF_SIM,    // Game_Frame (dia em fases, IA, estatisticas)
 PF_MAPU,   // PxM_Update: pintura das faixas novas do cache do mapa
 PF_BLIT,   // copia do cache p/ a tela
 PF_OVL,    // frente de guerra + contorno do selecionado
 PF_CITY,   // arte das cidades sobre o mapa (PxC_MapCities)
 PF_LBL,    // rotulos dos paises/cidades + capitais
 PF_DECO,   // resto: rotas, frotas, mira, minimapa, barra, faixa
 PF_AHD,    // render-ahead do mapa na folga do quadro (WG_Idle)
 PF_TOP,    // Px_Top inteiro (inclui MAPU..DECO no mapa)
 PF_BOT,    // Px_Bot (painel)
 PF_COUNT
};
#ifdef OMV_PROF
u32 Prof_Now(void);
void Prof_Add(int id,u32 dt);
#define PROF(id,...) do{u32 _pt=Prof_Now();__VA_ARGS__;Prof_Add((id),Prof_Now()-_pt);}while(0)
#else
#define PROF(id,...) do{__VA_ARGS__;}while(0)
#endif
void Prof_Init(void);
// Prof_Busy: antes do VBlank (fecha o tempo OCUPADO do quadro, sem a espera).
// Prof_EndFrame: depois da troca; vb = VBlanks que o quadro levou. Media movel.
void Prof_Busy(void);
void Prof_EndFrame(int vb);
// media movel em microssegundos por secao; [PF_COUNT] = quadro inteiro
const u32 *Prof_Us(void);
u32 Prof_Drops(void);              // quadros que passaram de 1 vblank
void Prof_Draw(u16 *d);            // caixa no canto da tela (so' com OMV_PROF)
