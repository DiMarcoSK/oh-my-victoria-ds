#pragma once
#include "types.h"
#include "world.h"
// wgame.h — partida no mundo celular (mapa-mundi 1800 / aleatorio / blitz),
// tela SCR_WORLD (v0.9 + M1). O mundo (World + grades W_*) vive em .bss,
// fora do GameState (trava de 14KB, DESIGN 9.1); aqui mora so' o estado de
// interface: camera, zoom, cursor, selecao, velocidade, avisos.
//
// Tela do MAPA (motor principal, cima no layout Normal): barra de recursos
// + mapa politico estilo Victoria 3 (px_world.c). As SETAS movem o mapa e a
// MIRA fica no centro da tela; A abre a folha de acoes do que esta' sob ela
// (DESIGN 3.6). Tela do PAINEL (baixo, so' caneta): ficha do alvo + despacho
// + botoes rapidos + % e velocidade.
// Layout Invertida (Settings) poe o mapa embaixo, tocavel (lcdMainOnBottom).
enum { WMAP_POL=0, WMAP_TER, WMAP_DIP, WMAP_COM, WMAP_IDEO, WMAP_COUNT };   // modos de mapa (Y)
enum { LT_MERCADO=0, LT_COMERCIO, LT_DIPLO, LT_POLITICA, LT_COUNT };    // abas do Gabinete
// folha de acoes (A sobre a mira, ou botao bandeira): o que da' p/ fazer com o
// alvo sob a mira — pais, sua terra, sua cidade ou terra livre. So' caneta.
enum { DM_PAZ=0, DM_ALIANCA, DM_ROMPER, DM_ENTRAR, DM_CONVIDAR, DM_GUERRA, DM_CANCELAR,
       DM_CIDADE, DM_FUNDAR, DM_RECRUTAR, DM_ATACAR, DM_COLONIZAR, DM_COUNT };
enum { AK_PAIS=0, AK_TERRA, AK_CIDADE, AK_LIVRE };        // tipo do alvo da folha
enum { WG_ACT_Y=30, WG_ACT_H=22 };                        // linhas tocaveis da folha
enum { WZ_COUNT=5 };      // zoom 0..4: (1<<z)/2 px por celula (Z0 = mundo inteiro)
enum { WG_MAP_Y=20, WG_MAP_H=172 };                    // viewport do mapa
typedef struct {
 s16 camX, camY;      // celula no canto sup-esq do viewport
 s16 curX, curY;      // cursor (celula)
 u8 zoom, mapMode;
 u8 me;               // pais do jogador (indice em World.c)
 u8 sel;              // pais sob o cursor (0 = terra livre ou nada)
 bool selLand;        // cursor sobre terra sem dono
 u8 warAsk;           // pais a quem a proxima confirmacao declara guerra
 u8 pct;              // % da reserva por ordem (25/50/75/100)
 u8 speed;            // 1..5
 bool paused, dead;
 u16 frameAcc, holdF; // frames ate o dia; frames segurando o D-pad
 u8 btnDown, btnT;    // botao do painel afundado (feedback 6 frames)
 u16 toastT;          // aviso de acao no despacho
 u16 toastId;         // StrId do aviso
 u8 toastArg;         // pais citado no aviso (0 = nenhum)
 u16 bannerT;         // faixa de evento historico sobre o mapa
 u32 msgSeen;         // ultimo w->msgDay exibido
 u32 tick;            // frames (animacao: frente pisca, selecao pulsa)
 bool stale;          // estatisticas de rotulos/minimapa precisam recalcular
 s16 hcity;           // cidade sob o cursor (-1 = nenhuma)
 s16 cityIdx;         // cidade aberta no SCR_CITY
 u8 citySlot;         // slot destacado na cidade
 bool cityPick;       // escolhendo a obra do slot
 u8 pickK;            // obra em escolha (BK_*)
 bool foundAsk;       // fundar pede o 2o toque (mostra o preco)
 s32 toastNum;        // numero no aviso (-1 = nenhum)
 s16 toastCity;       // cidade citada no aviso (-1 = nenhuma)
 bool dipOpen;        // folha de acoes aberta (alvo: dipTo / actKind)
 u8 dipTo, dipSel, dipN, dipIt[DM_COUNT];
 u8 actKind;          // AK_*: o que esta' sob a mira quando a folha abriu
 u8 ledTab, ledSel;   // Gabinete: aba e acao destacada
 u32 propSeen;        // dia da ultima proposta que ja pausou o jogo
 bool propRun;        // o jogo corria quando a proposta chegou (volta a correr)
} WGame;
// Gabinete (tela de baixo): geometria compartilhada por px_ledger.c e wgame.c
enum { LG_TAB_Y=2, LG_TAB_H=28, LG_ACT_X=16, LG_ACT_Y=120, LG_ACT_W=224, LG_ACT_H=28, LG_BACK_Y=156 };
// modal de proposta (painel do mapa): botoes Aceitar / Recusar
enum { PM_BTN_Y=150, PM_BTN_H=36 };

WGame *WG_Ptr(void);
World *WG_World(void);
// Gera o mundo e posiciona camera/cursor na capital do jogador. diff 0..2.
void WG_New(u8 mode,u8 hist,u8 size,u8 nNat,u8 diff,u32 seed,int nat);
void WG_Frame(GameState *g);   // dias conforme velocidade (parado na pausa)
void WG_Input(GameState *g,u32 down,u32 held,bool tDown,bool tUp,bool tHeld,int tx,int ty);
void WG_CityInput(GameState *g,u32 down,bool tDown,int tx,int ty); // SCR_CITY
void WG_LedgerInput(GameState *g,u32 down,bool tDown,int tx,int ty); // SCR_LEDGER
int WG_DipItems(u8 *out);      // itens validos da folha de acoes (alvo atual)
void PxL_Top(u16 *d);          // Gabinete (px_ledger.c): pagina da aba
void PxL_Bot(u16 *d);          // Gabinete: abas + acoes
void PxW_GoodChip(u16 *d,int x,int y,u8 g); // icone 8x8 do bem (px_ledger.c)
u16 PxW_GoodCol(u8 g);
// Celula visivel no pixel (sx,sy) do viewport; false = fora do mapa.
bool WG_CellAt(int sx,int sy,int *cx,int *cy);
int WG_ViewW(void);            // largura do viewport em celulas
int WG_ViewH(void);

// px_world.c
void PxW_Map(u16 *d);          // barra de recursos + mapa (tela do mapa)
void PxW_Panel(u16 *d);        // ficha + despacho + acoes (tela do painel)
void PxW_Stats(void);          // centros/caixas p/ rotulos + minimapa (1x/dia)
// camada do mapa com cache incremental (px_mapr.c, ADR-24)
void PxM_Update(World *w,const WGame *s);
void PxM_Blit(u16 *d);
void PxM_Overlay(u16 *d,const World *w,const WGame *s,int phase);
void PxM_Invalidate(void);
u16 *PxM_Scratch(void);       // LAYER emprestado (cena da cidade); invalida o cache
const u8 *PxM_Key(void);      // dono por pixel do viewport (0xFE fora, 0xFF mar)
// px_city.c: arte das cidades (procedural)
void PxC_Building(u16 *d,int x,int yb,int w,u8 k,u8 style,u16 year,u32 seed,int tick,u8 natFlag);
void PxC_MapCities(u16 *d,const World *w,const WGame *s);
void PxC_Scene(u16 *d,int city);         // tela de cima do SCR_CITY
void PxC_Panel(u16 *d,int city,int slot,bool picking,u8 pk);
void PxC_SceneReset(void);
// Mapa da Terra em sepia (selecao de nacao / preparar partida).
void PxW_Earth(u16 *d,int x,int y,int w,int h,int markNat,int pulse);
