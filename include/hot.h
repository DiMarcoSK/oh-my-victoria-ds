#pragma once
// hot.h — codigo quente do render (SDD ADR-36).
// O jogo compila em Thumb (codigo menor, a RAM principal tem barramento de
// 16 bits). Mas Thumb-1 nao tem multiplicacao longa: ate' x/100 vira chamada
// de __aeabi_idiv (~40-120 ciclos). Os lacos por pixel (pintura do mapa,
// fronteiras, copia p/ VRAM) vao em ARM de 32 bits (smull, barrel shifter,
// execucao condicional) e moram na ITCM: 32KB sem espera e fora do cache de
// instrucoes de 8KB, que o resto do quadro (UI, IA) disputa.
//   HOT_BEGIN/HOT_END: trecho do arquivo compilado em ARM.
//   HOT_ITCM: funcao na ITCM. long_call porque a ITCM fica a 16MB da RAM
//   principal no devkitPro oficial (0x01000100) — fora do alcance do BL Thumb.
// No host (tools/hostpx) tudo vira C comum.
#if defined(ARM9)||defined(OMV_ARMBENCH)   // ARMBENCH: tools/armbench (ARM9 emulado)
#define HOT_BEGIN _Pragma("GCC push_options") _Pragma("GCC target(\"arm\")")
#define HOT_END   _Pragma("GCC pop_options")
#define HOT_ITCM  __attribute__((section(".itcm"),long_call,noinline))
#else
#define HOT_BEGIN
#define HOT_END
#define HOT_ITCM
#endif
// palavra de 32 bits que pode apelidar buffers u16/u8 (SWAR, copia larga)
// sem violar strict aliasing no -O2.
typedef unsigned int __attribute__((may_alias)) u32a;
// funcao na ITCM chamada SO' de dentro da ITCM (BL curto, sem long_call)
#if defined(ARM9)||defined(OMV_ARMBENCH)
#define HOT_ITCM_IN __attribute__((section(".itcm"),noinline))
#define HOT_CALL    __attribute__((long_call))   // prototipo de funcao HOT_ITCM
#else
#define HOT_ITCM_IN
#define HOT_CALL
#endif
// funcao da SIMULACAO em ARM (na RAM principal, nao na ITCM): mesma semantica
// C, mas x/constante vira smull exato em vez de chamar __aeabi_idiv (Thumb-1
// nao tem multiplicacao longa). Resultado identico = replay intacto.
#if defined(ARM9)||defined(OMV_ARMBENCH)
#define HOT_ARM __attribute__((target("arm")))
#else
#define HOT_ARM
#endif
