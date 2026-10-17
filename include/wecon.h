#pragma once
#include "world.h"
// wecon.h — economia do mundo (DESIGN.md sec 4.8, SDD ADR-26).
// 9 bens em cadeia: o campo produz graos/madeira/fibra/minerio/luxo (terreno
// do pais + obras das cidades); oficina/fabrica/arsenal/poco transformam.
// Quatro niveis de mercado, do menor ao maior:
//  1. LOCAL: cada cidade come do proprio entorno (fazenda + lavoura em volta);
//     a sobra/falta vai para o mercado nacional (WCity.food).
//  2. NACIONAL: oferta x demanda de cada bem -> preco do pais.
//  3. BLOCO (mercado comum): paises do mesmo bloco somam oferta e demanda,
//     comerciam sem limite entre si e dividem o preco.
//  4. MUNDIAL: o que sobra/falta no bloco vai ao mundo pelos PORTOS (capacidade
//     maritima; sem porto so' caravana). Preco mundial = falta global / sobra.
// Tudo fecha 1x por semana (dia % 7 == 0), O(cidades + paises x bens), inteiro,
// sem RNG: o replay continua deterministico.
extern const u16 GD_BASE[GD_COUNT];   // preco base de cada bem
u16 Gd_Str(u8 g);                      // StrId do nome do bem
u16 Gd_Year(u8 g);                    // ano em que o bem passa a existir

void WEco_Init(World *w);              // depois da geracao (precos, capacidade)
void WEco_PopInit(World *w);           // populacao e alfabetizacao de 1800 (tabela + regra)
// Balanco fiscal (ADR-32). Receita - despesa da semana; deficit vira divida
// com juros pelo risco; divida acima do limite = calote (juros sobem, o
// exercito deserta, prestigio cai). Politicas: imposto, tarifa, educacao e
// gasto militar (jogador no Gabinete; IA ajusta 1x por mes).
s32 WEco_DebtLimit(const World *w,u8 ci);
s32 WEco_Revenue(const World *w,u8 ci);   // receita total da semana
s32 WEco_Expense(const World *w,u8 ci);   // despesa total da semana
void WEco_AiPolicy(World *w,u8 ci);       // IA: imposto/tarifa/educacao/militar
void WEco_Week(World *w);              // fecha a semana do mundo (World_Day)
// Producao/consumo de uma cidade numa semana (UI e IA). prod = saida,
// need = insumo das obras, dem = consumo da populacao.
void WEco_CityFlows(const World *w,int city,s32 prod[GD_COUNT],s32 need[GD_COUNT],s32 dem[GD_COUNT]);
s32 WEco_GoldWeek(const World *w,u8 ci);   // saldo fiscal da ultima semana (receita - despesa)
int WEco_ArmsPct(const World *w,u8 ci);    // 50..100: regeneracao do exercito pelas armas
int WEco_ToolsPct(const World *w,u8 ci);   // 50..100: ritmo de obra pelas ferramentas

// Marinha (portos com estaleiro): navios protegem rotas, bloqueiam o inimigo
// e garantem desembarques (World_Landing).
s32 WEco_ShipCost(const World *w,u8 ci);
bool WEco_BuildShip(World *w,u8 ci);       // paga e poe na fila do estaleiro
int WEco_ShipDays(const World *w,u8 ci);   // dias por navio (cai com estaleiros)
bool WEco_Blockaded(const World *w,u8 ci); // algum inimigo domina o mar dele
