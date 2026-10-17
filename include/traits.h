#pragma once
#include <nds.h>
// traits.h — sabor mecanico de cada pais (DESIGN.md sec 6.4).
// Cada trait mexe em alavancas que o motor ja tem; nada de sistema novo
// por trait. IDs sao gravados no save e na tabela de paises: so ADICIONE
// antes de TR_COUNT, nunca reordene.
typedef enum {
 TR_NONE=0,
 TR_INDUSTRIAL,   // Berco Industrial (+2)
 TR_MARES,        // Senhora dos Mares (+2)
 TR_ENTREPOSTO,   // Entreposto Comercial (+1)
 TR_COLONIAL,     // Imperio Colonial (+1)
 TR_CELEIRO,      // Celeiro (+1)
 TR_MINERAL,      // Riqueza Mineral (+1)
 TR_EXERCITO,     // Exercito Profissional (+2)
 TR_LEVEE,        // Levee en Masse (+2)
 TR_DISCIPLINA,   // Disciplina Regimental (+1)
 TR_INVERNO,      // Inverno Implacavel (+2)
 TR_CAVALEIROS,   // Cavaleiros das Planicies (+2)
 TR_TERRA,        // Conhecimento da Terra (+2)
 TR_MONTANHAS,    // Montanhas-Fortaleza (+1)
 TR_GUERRILHA,    // Guerra de Guerrilha (+1)
 TR_CONFEDERACAO, // Confederacao (+1)
 TR_MOVIMENTO,    // Povo em Movimento (+1)
 TR_COESA,        // Comunidade Coesa (+1)
 TR_FE,           // Fe Unificadora (+1)
 TR_CULTURA,      // Centro Cultural (+1)
 TR_BUROCRACIA,   // Burocracia Antiga (+1)
 TR_REFORMADOR,   // Reformador (+1)
 TR_REVOLUCAO,    // Fervor Revolucionario (+1)
 TR_JOVEM,        // Nacao Jovem (+1)
 TR_FRONTEIRA,    // Fronteira Aberta (+1)
 TR_CORSARIOS,    // Corsarios (+1)
 TR_ISOLAMENTO,   // Isolamento (-1)
 TR_ABSOLUTISMO,  // Absolutismo (-1)
 TR_MULTIETNICO,  // Imperio Multietnico (-1)
 TR_CRISE,        // Gigante em Crise (-1)
 TR_GLORIA,       // Gloria Antiga (0)
 TR_COUNT
} TraitId;

#define NAT_TRAITS 4 // traits por pais

// Soma dos efeitos dos traits de um pais. Percentuais somam (+25 = +25%).
typedef struct {
 s16 atkPct, defPct;      // forca de ataque/defesa em qualquer celula
 s16 defRoughPct;         // defesa extra em floresta/pantano/colina/deserto/selva
 s16 defMountPct;         // defesa extra em colina/montanha
 s16 plainsSpeedPct;      // avanco extra da frente em planicie/estepe/deserto
 s16 regenPct, capPct;    // regeneracao e teto do exercito
 s16 goldPct, tradePct;   // renda e comercio externo
 s16 prodPct;             // producao industrial
 s16 recruitCostPct;      // custo de recrutar (negativo = mais barato)
 s16 colonizePct;         // custo menor p/ celulas sem dono
 s16 literacyPct, reformPct;
 s16 supplyPct;           // penalidade de distancia da capital (negativo = menor)
 s8 happy, militancy;     // pontos 0..100 somados ao alvo
 s8 prestigeYear;         // prestigio extra por ano
 s8 winterAttrition;      // % das tropas inimigas perdidas/dia no inverno
 s8 roughAttrition;       // % das tropas inimigas perdidas/dia em terreno dificil
 s8 revertPct;            // chance/dia (x0,1%) de celula ocupada voltar
 bool capitalNotFatal;    // perder a capital nao derruba o pais
 bool confed, isolated;
} TraitMods;

s8 Tr_Weight(u8 t);
const char *Tr_Name(u8 t);   // UTF-8
const char *Tr_Desc(u8 t);   // 1 linha, ~60 caracteres (tooltip)
void Tr_Mods(const u8 traits[NAT_TRAITS],TraitMods *out);
int Tr_Budget(const u8 traits[NAT_TRAITS]); // soma dos pesos
