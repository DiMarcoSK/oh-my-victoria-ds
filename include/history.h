#pragma once
#include "world.h"
// history.h — historia viva (DESIGN.md sec 7) + eventos aleatorios (sec 7.5).
// Evento historico = ancora (data) + janela (anos) + condicoes + efeitos.
// Dispara no primeiro dia, a partir da ancora, em que as condicoes valem;
// se a janela acaba sem isso, falha (o mundo divergiu). Determinístico.

typedef enum {
 HC_END=0,
 HC_EXISTS,      // a vivo
 HC_NOT_EXISTS,  // a nao existe
 HC_SUBJECT,     // a e suserano... a e sudito de b
 HC_STRONGER,    // tropas(a)*100 >= tropas(b)*v
 HC_AT_WAR,      // a em guerra com b
 HC_FIRED,       // evento de id a ja disparou
} HCondOp;

typedef enum {
 HE_END=0,
 HE_PRESTIGE,    // a += v
 HE_GOLD,        // a += v (absoluto)
 HE_TROOPS,      // a += v% das tropas
 HE_HAPPY,       // a felicidade += v
 HE_MILIT,       // a militancia += v
 HE_INDEP,       // a deixa de ser sudito
 HE_SUBJECT,     // a vira sudito de b
 HE_ANNEX,       // a absorve b inteiro (terras seguem sendo nucleo de b)
 HE_UNIFY,       // a absorve b e as terras viram nucleo de a (unificacoes)
 HE_BECOME,      // o pais da nacao a vira a nacao b (mesmo territorio)
 HE_SPAWN,       // nacao a surge na sua capital, raio v, tomando de b (NULL = qualquer)
 HE_CEDE,        // celulas de b perto do local c (raio v) vao para a
 HE_RELOCATE,    // a perde tudo para b e recomeca perto do local c (raio v)
 HE_WAR,         // a declara guerra a b
 HE_PEACE,       // a e b fazem paz
 HE_PEACEALL,    // fim de todas as guerras
 HE_GOV,         // governo de a = v (NatGov)
 HE_ADDTRAIT,    // a ganha trait v (se houver espaco)
 HE_DELTRAIT,    // a perde trait v
 HE_WORLD_GOLD,  // todo pais: ouro += v% do ouro
 HE_WORLD_HAPPY, // todo pais: felicidade += v
 HE_DECOLONIZE,  // todo sudito do continente v fica independente
 HE_POVOS_PRESTIGE, // todo pais do tipo POVO: prestigio += v
 HE_IDEO,        // a muda de ideologia: v = ID_* (transicao) | v+16 = revolucao | -1 = sorteio
 HE_SHIPS,       // a ganha/perde v navios
 HE_SEPARA,      // a sofre um movimento separatista (wdiplo.c)
 HE_WORLD_MILIT, // todo pais: militancia += v
} HEffOp;

// a/b/c: TAG de nacao (ou de local, p/ c); NULL = "o proprio" nos aleatorios.
typedef struct { u8 op; const char *a, *b, *c; s16 v; } HOp;
typedef struct {
 u16 y; u8 m, d, win;       // ancora e janela (anos)
 const char *id, *title, *body;
 HOp cond[3];
 HOp eff[6];
} HistEvent;

void Hist_Init(World *w);
void Hist_Day(World *w);
void Rnd_Day(World *w);
int Hist_Count(void);
const HistEvent *Hist_Get(int i);
u8 Hist_State(const World *w,int i);  // 0 pendente, 1 disparou, 2 falhou
int Rnd_Count(void);
int Rnd_DecisionCount(void);
// decisao do jogador (PR_DECISAO): opcao A ou B do evento idx
void Rnd_Decide(World *w,u8 ci,int idx,bool optA);
const char *Rnd_Title(int idx);
const char *Rnd_Body(int idx);
const char *Rnd_Opt(int idx,bool optA);
