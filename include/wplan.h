#pragma once
#include "world.h"
// wplan.h — controle da economia (DESIGN 4.13, SDD ADR-47).
// DOUTRINA = a lei LW_MODELO lida como grau de controle do Estado (0..4):
//   0 laissez-faire (libertario) .. 1 mercantilismo .. 2 economia mista ..
//   3 dirigismo .. 4 planificacao (economia de comando).
// Quanto mais controle, mais POLITICAS POR BEM o governo pode ter:
//  * SUBSIDIO (controle >= 1): +25% de producao do bem; o Estado paga 1/4
//    do valor produzido.
//  * PRECO TABELADO (>= 2): o preco fica no de 1800 — o povo gosta e a
//    inflacao esfria, mas a oferta cai 15% (mercado negro, produtor some).
//  * META DO PLANO (>= 2): o bem pega insumo primeiro e produz mais
//    (+10/+15/+25%); o investimento ergue as obras dele antes das outras.
// Vagas de politica por doutrina: 0, 1, 2, 4, 6.
// O livre mercado tem investimento privado forte, Estado barato e CICLOS
// fortes (alta com credito barato -> bolha -> panico -> retomada); o comando
// amortece o ciclo e zera o desemprego, mas acumula ESTAGNACAO (ate' -15% de
// producao) e expulsa a burguesia. Juro alto na alta = pouso suave.
// Tudo 1x por semana dentro do WEco_Week, inteiro, sem RNG (hash da semana).
enum { CTL_COUNT=5 };
int Plan_Ctl(const WEco *e);                 // 0 (livre) .. 4 (comando)
int Plan_Slots(const WEco *e);               // vagas de politica da doutrina
int Plan_Used(const WEco *e);                // politicas em uso
// 0 = pode; senao StrId do motivo (bem nao existe, doutrina, sem vaga)
u16 Plan_Why(const World *w,u8 ci,u8 g,u8 pol);
bool Plan_Set(World *w,u8 ci,u8 g,u8 pol);   // aplica (GP_LIVRE sempre pode)
void Plan_Fix(WEco *e);                      // doutrina mudou: corta o que nao cabe
u16 Plan_PolStr(u8 pol);                     // StrId: estado ("subsidiado")
u16 Plan_BtnStr(u8 pol);                     // StrId: botao ("Subsidiar")
u16 Plan_PolHelp(u8 pol);                    // StrId: o que faz
u16 Plan_CtlStr(int ctl);                    // StrId: resumo da doutrina
u8 Plan_LawOfCtl(int ctl);                   // lei LW_MODELO de um grau de controle
// efeitos (wecon.c)
int Plan_GoodPct(const WEco *e,u8 g);        // % da producao do bem (100 = neutro)
int Plan_MetaPri(const WEco *e);             // % do insumo que falta que a meta recupera
int Plan_ProdPct(const WEco *e);             // pontos de producao do ciclo e da estagnacao
int Plan_InvPct(const WEco *e);              // % do investimento privado pelo ciclo
int Plan_InflPermil(const WEco *e);          // pontos de inflacao (ciclo e precos tabelados)
int Plan_AdminPct(const WEco *e);            // % do custo da maquina do Estado
s32 Plan_SubsCost(const WEco *e);            // ouro de 1800 dos subsidios da semana
s32 Plan_StatePct(const WEco *e);            // % do PIB que as estatais rendem ao Estado
int Plan_HappyFx(const WEco *e,u8 strat);    // pontos de satisfacao do estrato (desemprego, ciclo, tabela)
int Plan_Score(const WEco *e,u8 g);          // bonus do investimento p/ obra que produz g
void Plan_Week(World *w,u8 ci);              // ciclo, desemprego, estagnacao (fim da semana do pais)
void Plan_Shock(World *w,u8 ci,int pts);     // choque externo (crise mundial, evento)
void Plan_Ai(World *w,u8 ci);                // IA: escolhe as politicas 1x por mes
int Plan_Pct(const WEco *e);                 // ciclo em pontos (-100..100)
