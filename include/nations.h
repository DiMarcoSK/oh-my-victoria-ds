#pragma once
#include <nds.h>
#include "traits.h"
// nations.h — catalogo de paises e povos (DESIGN.md sec 6). Dados em ROM.
// A ORDEM da tabela NATIONS e o ID gravado no save: so ADICIONE no fim de
// cada bloco se o save ja existir; prefira sempre buscar por TAG (Nat_Find).

typedef enum { CONT_EUROPA=0, CONT_AMERICAS, CONT_AFRICA, CONT_ASIA, CONT_OCEANIA, CONT_COUNT } Continent;
typedef enum { NK_ESTADO=0, NK_SUDITO, NK_POVO, NK_AUTONOMO } NationKind;
// Governos do mundo (DESIGN.md sec 6.2). NG_REPUBLICA = DEMOCRACIA da v0.7.
typedef enum { NG_MONARQUIA=0, NG_REPUBLICA, NG_TEOCRACIA, NG_CONSELHO, NG_COMPANHIA, NG_COUNT } NatGov;
// Cena regional da tela de selecao (SDD sec 14.4, art_region_<r>).
typedef enum {
 SC_EUR_OCID=0, SC_EUR_CENT, SC_EUR_LESTE, SC_MEDITERRANEO, SC_AMN_LESTE,
 SC_AMN_PLANICIES, SC_AMS, SC_CARIBE, SC_AFR_NORTE, SC_AFR_OCID, SC_AFR_LESTE_SUL,
 SC_ORIENTE_MEDIO, SC_ESTEPE, SC_ASIA_SUL, SC_ASIA_LESTE, SC_SUDESTE, SC_OCEANIA,
 SC_ARTICO, SC_COUNT
} SceneRegion;

// Bandeira procedural (DESIGN.md sec 6.2.1): padrao + 3 cores + emblema.
typedef enum {
 FP_LISA=0, FP_TRI_V, FP_TRI_H, FP_BI_H, FP_BI_V, FP_NORDICA, FP_CRUZ,
 FP_SALTIRE, FP_UNION, FP_CANTAO, FP_BORDA, FP_DIAG, FP_LISTRAS, FP_COUNT
} FlagPattern;
typedef enum {
 FE_NONE=0, FE_ESTRELA, FE_CRESCENTE, FE_SOL, FE_DISCO, FE_COROA, FE_AGUIA,
 FE_AGUIA2, FE_LEAO, FE_CRUZ, FE_DRAGAO, FE_ESCUDO, FE_COUNT
} FlagEmblem;
typedef enum {
 FC_WHITE=0, FC_RED, FC_BLUE, FC_NAVY, FC_GREEN, FC_YELLOW, FC_BLACK, FC_ORANGE,
 FC_CRIMSON, FC_SKY, FC_PURPLE, FC_BROWN, FC_DKGREEN, FC_GOLD, FC_COUNT
} FlagColor;
typedef struct { u8 pat, c0, c1, c2, emb, ec; } FlagDef;

typedef struct {
 const char *tag;       // 3 letras
 const char *name;      // PT (UTF-8)
 const char *endo;      // endonimo/nome original, ou NULL
 u8 cont, kind, gov, scene;
 u8 r, g, b;            // cor de mapa (5 bits/canal)
 FlagDef flag;
 u8 banner;             // 1 = estandarte de jogo (nao havia bandeira nacional)
 u8 traits[NAT_TRAITS];
 s8 lat; s16 lon;       // capital (graus inteiros)
 u8 size;               // raio inicial em celulas no mapa-mundi
 u16 yFrom;             // 0 = existe em 1/1/1800; ano = surge por evento
 const char *overlord;  // TAG do suserano se NK_SUDITO
 const char *desc;      // 2 linhas, ~90 caracteres (tela de selecao)
} NationDef;

// Centros extras de grandes imperios/colonias p/ pintar o mapa de 1800.
typedef struct { const char *tag; s8 lat; s16 lon; u8 r; } NatAnchor;

extern const NationDef NATIONS[];
extern const int NATION_COUNT;
extern const NatAnchor NAT_ANCHORS[];
extern const int NAT_ANCHOR_COUNT;

int Nat_Find(const char *tag);           // indice ou -1
const char *Nat_ContName(u8 c);
const char *Nat_GovName(u8 g);
void Flag_PalRGB(u8 c,u8 *r,u8 *g,u8 *b); // cor da paleta de bandeiras (5 bits)
