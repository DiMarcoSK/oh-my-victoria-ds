#pragma once
#include "types.h"
#include "world.h"
#include "hot.h"
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
enum { LT_MERCADO=0, LT_COMERCIO, LT_DIPLO, LT_POLITICA, LT_FINANCAS, LT_LEIS, LT_GRAF, LT_SOCIEDADE, LT_COUNT };    // abas do Gabinete
// folha de acoes (A sobre a mira, ou botao bandeira): o que da' p/ fazer com o
// alvo sob a mira — pais, sua terra, sua cidade ou terra livre. So' caneta.
enum { DM_PAZ=0, DM_ALIANCA, DM_ROMPER, DM_ENTRAR, DM_CONVIDAR, DM_GUERRA, DM_CANCELAR,
       DM_CIDADE, DM_FUNDAR, DM_RECRUTAR, DM_ATACAR, DM_COLONIZAR,
       DM_FORTE, DM_TRINCHEIRA, DM_BATERIA, DM_ARMAS, DM_ARMAS_FIM, DM_INSTIGAR,
       DM_META_TERRA, DM_META_TRIBUTO, DM_META_HUMILHAR, DM_CONCEDER, DM_COUNT };   // ADR-45: objetivo da crise; comprar a paz
// AK_OBRA: folha do botao Construir (cidade + fortificacoes na celula da mira; ADR-38)
// AK_META: objetivo da crise contra o pais dipTo (DM_GUERRA abre; ADR-45)
enum { AK_PAIS=0, AK_TERRA, AK_CIDADE, AK_LIVRE, AK_OBRA, AK_META };  // tipo do alvo da folha
enum { WG_ACT_Y=30, WG_ACT_H=22 };                        // linhas tocaveis da folha
enum { WZ_COUNT=6 };      // zoom 0..5: (1<<z)/2 px por celula (Z0 = mundo inteiro; Z5 = 16 px, tropas lutando)
enum { WG_MAP_Y=20, WG_MAP_H=172 };                    // viewport do mapa
typedef struct {
 s16 camX, camY;      // celula no canto sup-esq do viewport (derivada de pX/pY)
 u8 subX, subY;       // pixels dentro da celula (Z2+): rolagem pixel a pixel
 s32 pX, pY;          // camera em 1/16 de pixel da vista (ADR-31)
 s16 rX, rY;          // mira na vista, 1/16 px (centro, salvo nas bordas)
 s16 vX, vY;          // velocidade da camera (1/16 px por quadro)
 u8 phase;            // fase do dia em andamento (0 = nenhuma; World_DayPhase)
 s16 curX, curY;      // celula sob a mira
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
 u8 citySlot;         // (legado) slot destacado
 u8 cityLot;          // lote sob a mira no mapa da cidade (0..CG_N-1)
 u8 cityHold;         // quadros segurando a seta no mapa da cidade (repeticao)
 bool cityPick;       // escolhendo a obra do slot
 u8 pickK;            // obra em escolha (BK_*)
 bool foundAsk;       // fundar pede o 2o toque (mostra o preco)
 s32 toastNum;        // numero no aviso (-1 = nenhum)
 s16 toastCity;       // cidade citada no aviso (-1 = nenhuma)
 bool dipOpen;        // folha de acoes aberta (alvo: dipTo / actKind)
 u8 dipTo, dipSel, dipN, dipIt[DM_COUNT];
 u8 actKind;          // AK_*: o que esta' sob a mira quando a folha abriu
 u8 ledTab, ledSel;   // Gabinete: aba e acao destacada
 u8 lawCat, lawPick;  // aba Leis: assunto aberto (0xFF = grade de assuntos) e lei destacada
 u8 statSel;          // aba Graficos: serie (SR_*)
 u8 rankTab;          // ranking: criterio (RK_*)
 s16 rankTop;         // ranking: primeira linha visivel
 u32 propSeen;        // dia da ultima proposta que ja pausou o jogo
 bool propRun;        // o jogo corria quando a proposta chegou (volta a correr)
 u8 botLazy;          // ficha do painel pendente durante o pan (quadros desde o pedido; ADR-36)
 u16 centT;           // transicao de seculo na tela do mapa (VBlanks restantes; ADR-46)
 u16 centY;           // ano da transicao (1800, 1900, 2000...)
 u16 cityCamX, cityCamY; // camera do mapa da cidade (px; 20x12 lotes > tela, ADR-44)
} WGame;
// Gabinete (tela de baixo): geometria compartilhada por px_ledger.c e wgame.c
// abas em 2 linhas (ADR-40): 4 em cima (64 px), 3 embaixo (85 px)
enum { LG_TAB_Y=2, LG_TAB_RH=16, LG_TAB_H=33, LG_TAB_N0=4, LG_ACT_X=16, LG_ACT_Y=120, LG_ACT_W=224, LG_ACT_H=28, LG_BACK_Y=156 };
static inline void Lg_TabRect(int i,int *x,int *y,int *w){
 if(i<LG_TAB_N0){*w=256/LG_TAB_N0;*x=i**w;*y=LG_TAB_Y;}
 else{*w=256/(LT_COUNT-LG_TAB_N0);*x=(i-LG_TAB_N0)**w;*y=LG_TAB_Y+LG_TAB_RH+1;}
}
static inline int Lg_TabAt(int tx,int ty){
 if(ty<LG_TAB_Y||ty>=LG_TAB_Y+LG_TAB_H)return -1;
 if(ty<LG_TAB_Y+LG_TAB_RH+1)return tx*LG_TAB_N0/256;
 int i=LG_TAB_N0+tx*(LT_COUNT-LG_TAB_N0)/256;return i<LT_COUNT?i:LT_COUNT-1;
}
// aba Leis (painel): grade 3x3 de assuntos / 5 linhas de leis + Propor e Voltar
enum { LW_GX=4, LW_GY=38, LW_GW=82, LW_GH=36, LW_LY=38, LW_LH=22, LW_BY=152, LW_BH=34, LW_PW=164 };
// aba Graficos (painel): 8 series em 2 linhas de 4
enum { GR_BX=4, GR_BY=40, GR_BW=62, GR_BH=32 };
// ranking (SCR_RANK): abas de criterio, paginas, minha nacao (voltar = LG_BACK_Y)
enum { RK_TAB_Y=2, RK_TAB_H=26, RK_BTN_Y=124, RK_BTN_H=26, RK_BTN_W=56, RK_UP_X=8, RK_DN_X=70, RK_ME_X=132, RK_ME_W=116 };
// cidade (SCR_CITY, painel): minimapa de lotes, botao do lote, catalogo
enum { CY_MINI_X=6, CY_MINI_Y=46, CY_CELL=6, CY_BTN_Y=120, CY_BTN_H=22, CY_PICK_Y=118,
       CY_ARW_W=36, CY_OK_X=56, CY_OK_Y=162, CY_OK_W=96, CY_OK_H=24, CY_NO_X=158, CY_NO_W=90 };
// faixa da crise/guerra com objetivo no lugar do despacho (painel do mapa, ADR-45)
enum { CR_Y=106, CR_BY=131, CR_BH=16, CR_BX0=8, CR_BW=78, CR_BDX=80 };
// aba Financas: botoes de juros (metades da linha de acao LG_ACT_*)
enum { FN_HALF=LG_ACT_W/2 };
// modal de proposta (painel do mapa): botoes Aceitar / Recusar
enum { PM_BTN_Y=150, PM_BTN_H=36 };

WGame *WG_Ptr(void);
World *WG_World(void);
// Gera o mundo e posiciona camera/cursor na capital do jogador. diff 0..2.
void WG_New(u8 mode,u8 hist,u8 size,u8 nNat,u8 diff,u32 seed,int nat);
void WG_Frame(GameState *g);   // dias conforme velocidade (parado na pausa)
void WG_Idle(GameState *g);    // fim do quadro: usa a folga p/ pintar o mapa a' frente (ADR-36)
void WG_Goto(int x,int y);     // poe a celula sob a mira (camera centrada nela)
void WG_Input(GameState *g,u32 down,u32 held,bool tDown,bool tUp,bool tHeld,int tx,int ty);
void WG_CityInput(GameState *g,u32 down,bool tDown,int tx,int ty); // SCR_CITY
void WG_LedgerInput(GameState *g,u32 down,bool tDown,int tx,int ty); // SCR_LEDGER
void WG_RankInput(GameState *g,u32 down,bool tDown,int tx,int ty);   // SCR_RANK
void PxR_Top(u16 *d);          // ranking (px_rank.c): lista
void PxR_Bot(u16 *d);          // ranking: criterio, paginas, sua posicao
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
void PxW_Stats(void);          // centros/caixas p/ rotulos + minimapa (volta inteira)
void PxW_StatsStep(int rows);  // idem, incremental: algumas linhas por quadro
// camada do mapa com cache incremental (px_mapr.c, ADR-24)
void PxM_Update(World *w,const WGame *s);
HOT_CALL void PxM_Blit(u16 *d);   // cache (anel) -> tela, so' acessos 16/32 bits (ITCM)
HOT_CALL void PxM_Overlay(u16 *d,const World *w,const WGame *s,int phase);
void PxM_Invalidate(void);
// render-ahead: pinta a margem a' frente da camera (budget em ticks, 0 = so' o
// teto de passos). So' vale com a camera do ultimo PxM_Update.
void PxM_Ahead(const World *w,const WGame *s,u32 budget,int maxSteps);
u16 *PxM_Scratch(void);       // LAYER emprestado (cena da cidade); invalida o cache
u8 PxM_KeyAt(int sx,int sy);  // dono do pixel (sx,sy) do viewport (0xFE fora, 0xFF mar)
// px_city.c: arte das cidades (procedural)
void PxC_Building(u16 *d,int x,int yb,int w,u8 k,u8 style,u16 year,u32 seed,int tick,u8 natFlag);
void PxC_MapCities(u16 *d,const World *w,const WGame *s);
void PxC_Scene(u16 *d,int city);         // tela de cima do SCR_CITY
void PxC_Panel(u16 *d,int city,int lot,bool picking,u8 pk);
u8 PxC_LotDen(const World *w,int city,int lot);   // 0 nada, 1-3 casas, 4 campo, 5 mata
void PxC_SceneReset(void);
// px_sea.c: rotas pelo mar (grade grossa + BFS + cache; ADR-33)
void PxS_Reset(void);
void PxS_Frame(void);                    // orcamento de buscas do quadro
int PxS_Node(const World *w,int x,int y); // no' de mar perto da celula (-1 = nenhum)
int PxS_Path(const World *w,int cityA,int cityB,const u16 **pts); // n pontos; 0 sem mar; -1 depois
void PxS_NodeCell(int n,int *x,int *y);  // centro do no' em celulas
// Mapa da Terra em sepia (selecao de nacao / preparar partida).
void PxW_Earth(u16 *d,int x,int y,int w,int h,int markNat,int pulse);
