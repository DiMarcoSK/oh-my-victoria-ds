#pragma once
#include "world.h"
// wsoc.h — estratos sociais (DESIGN 4.11, SDD ADR-42). Tres camadas por pais:
//  * POBRES (camponeses, operarios): comida, roupa, trabalho; a militancia
//    deles e' o combustivel das revolucoes.
//  * MEDIOS (artesaos, funcionarios, comerciantes, profissionais): crescem
//    com a alfabetizacao e a cidade; pedem escola, bens, ferrovia; satisfeitos
//    produzem mais.
//  * RICOS (aristocracia, latifundio e BURGUESIA): pouca gente, muito peso
//    politico. A burguesia (bourg % dos ricos, sobe com a industria e o seculo)
//    REINVESTE: o fundo privado (WEco.invest) ergue e amplia obras sozinho, e
//    de tempos em tempos oferece FINANCIAR uma obra publica em troca de algo
//    (baixar imposto, baixar juros, afrouxar a lei trabalhista, tarifa ou uma
//    concessao da renda urbana) — despacho PR_BURGUES.
// Cada estrato tem satisfacao (alvo por cesta de bens, leis, juros, ideologia)
// e um DESEJO que muda com a CULTURA (estilo da nacao: europeu, asiatico,
// islamico, africano, nativo) — nada engessado: o desejo sorteado e' o que
// ainda nao esta' atendido. Atender alegra; ignorar irrita. Fecha 1x por
// semana (WEco_Week), sem RNG exceto o golpe dos ricos (World_Rng).
enum { LB_IMPOSTO=0, LB_JUROS, LB_TRABALHO, LB_COMERCIO, LB_CONCESSAO, LB_COUNT };

void Soc_Init(World *w);                 // fatias de 1800 (depois do WEco_PopInit)
void Soc_Prep(World *w);                 // resumo das cidades da semana (antes dos paises)
void Soc_Week(World *w,u8 ci);           // semana do pais (WEco_Week, depois do nivel de vida)
int Soc_HappyFx(const World *w,u8 ci);   // pontos no alvo de felicidade (-16..16)
int Soc_MilitFx(const World *w,u8 ci);   // pontos no alvo de militancia (0..12)
int Soc_InvestPct(const World *w,u8 ci); // % do investimento privado (ricos + burguesia)
int Soc_ProdPct(const World *w,u8 ci);   // % de producao (classe media satisfeita)
bool Soc_WantOk(const World *w,u8 ci,u8 dw);  // o desejo esta' atendido agora?
u8 Soc_Culture(const World *w,u8 ci);    // ST_* (city.h): cultura que escolhe os desejos
u16 Soc_StratStr(u8 s);                  // StrId do estrato
u16 Soc_WantStr(u8 dw);                  // StrId do desejo
u16 Soc_LobbyStr(u8 lb);                 // StrId da exigencia da burguesia
void Soc_LobbyAnswer(World *w,u8 me,const WProp *p,bool yes);
