#pragma once
#include "types.h"
// ui.h — v0.8 TOUCH-FIRST. Fonte unica de verdade da geografia da interface:
// tabelas de botoes (x,y,w,h,id,label) por pagina, escritas em source/ui.c.
// px_ui.c desenha A PARTIR das tabelas (nada hardcode), input.c despacha taps
// via Ui_Hit(). Regra: target minimo 20px (dedo/caneta DS).
//
// Paginas (tela baixo, sobre o mapa): mapa puro + dock sempre visivel;
// abre aba = troca pagina; fecha com X ou tap fora da aba.
typedef enum {
 UI_MAP=0,    // mapa tocavel (selecionar/pan) + dock
 UI_BUILD=1,  // aba OBRAS (grade 23)
 UI_TRADE=2,  // aba MERCADO (8 bens x compra int/ext+venda)
 UI_DIPLO=3,  // aba RELACOES (Norte/Sul: paz/uniao)
 UI_SYS=4,    // aba SISTEMA (gov, imposto, vel, save/load)
 UI_MENU=5,   // menu inicial (caneta): dificuldade/seed/comecar
 UI_WORLD=6   // painel do mapa-mundi (SCR_WORLD, wgame.c)
} UiPage;
// IDs de botao (u8). Esquema:
// 10..29 dock/chips do mapa; 30..39 acoes contextuais do tile;
// 100..123 trade: 100+good*3+(0=cial,interna 1=externa 2=venda);
// 130..135 diplo: 130+(f-2)*2+(0=paz 1=uniao) p/ f=2,3
// 140..149 sistema; 150..159 menu.
enum {
 UI_NONE=0,
 UI_D_OBRAS=10, UI_D_MERC=11, UI_D_DIPLO=12, UI_D_SYS=13,
 UI_D_PAUSE=14, UI_D_SPEED=15,
 UI_A_MARCH=30, UI_A_REC=31, UI_A_FORT=32,
 UI_TR_SHOP=100, // +good*3: col 0=COMP INT 1=COMP EXT 2=VEND
 UI_DP_BASE=130, // +(f-2)*2: col 0=PAZ 1=UNIAO, f=2..3
 UI_SP_PAUSE=140, UI_SP_SP1=141, UI_SP_SP2=142, UI_SP_SP3=143,
 UI_SP_IMPT=144, // imposto: 0..2 sequencial (tap cicla)
 UI_SP_SAVE=145, UI_SP_LOAD=146, UI_SP_NEW=147,
 UI_GV_MON=148, UI_GV_DEM=149, UI_GV_TEO=150,
 UI_W_ATK=160, UI_W_PEACE=161, UI_W_REC=162, UI_W_MODE=163, UI_W_PCT=164, UI_W_SPEED=165, UI_W_CITY=166,
 UI_MN_PREVD=152, UI_MN_NEXTD=153, UI_MN_SEED=154, UI_MN_START=155,
 UI_CLOSE=0xFF
};
typedef struct { u8 x,y,w,h,id; const char *lab; u8 dim; } Btn;
// API (source/ui.c):
const Btn *Ui_Tbl(u8 page);   // tabela da pagina ( termina em id==0)
int Ui_Hit(u8 page,int px,int py); // id do botao sob o toque, -1 se nada
void Ui_Open(GameState *g,u8 page);
void Ui_Close(GameState *g);
// desenhadores (source/px_ui.c): dock/abas sobre o mapa vivo + menu caneta
void Ui_DrawBot(GameState *g,u16 *d);
void Ui_DrawMenu(GameState *g,u16 *d);
const Btn *Ui_Ctx(void); // botoes contextuais do tile (faixa acima do dock)

// ---- v0.9 componentes da concept (SDD sec 14.1), em source/px_ui.c ----
enum { ICO_NONE=0, ICO_CROWN, ICO_SWORD, ICO_SWORDS, ICO_PEOPLE, ICO_FLAG,
 ICO_SHIELD, ICO_GLOBE, ICO_COIN_S, ICO_SWORDS_S, ICO_CROWN_S, ICO_CAL_S, ICO_LAND_S, ICO_HOUSE };
void Ui_Parch(u16*d,int x,int y,int w,int h);   // pergaminho + borda dupla + cantos
void Ui_Navy(u16*d,int x,int y,int w,int h);    // painel navy + borda ouro
void Ui_Backdrop(u16*d);                        // fundo navy com grao
void Ui_TextC(u16*d,int cx,int y,const char*s,int big,u16 col); // Serifa centrada
void Ui_Div(u16*d,int cx,int y,u16 col);        // divisor 48x5
void Ui_Crown(u16*d,int x,int y);               // coroa 16x12
void Ui_Icon(u16*d,int x,int y,const char*const*rows,int h,bool dim);
void Ui_IconId(u16*d,int x,int y,u8 id,bool dim);
void Ui_Item(u16*d,int x,int y,int w,const char*s,bool sel,bool dim,bool pulse,bool star);
void Ui_BigBtn(u16*d,int x,int y,const char*s,u8 icon,bool sel,bool dim,bool pulse);
void Ui_IconBtn(u16*d,int x,int y,u8 icon,bool pressed,bool dim);
void Ui_ValueRow(u16*d,int x,int y,int w,const char*lab,const char*val,bool sel);
void Ui_Slider(u16*d,int x,int y,int v);
void Ui_Arrows(u16*d,int xl,int xr,int y,u16 c); // ◂ em xl, ▸ em xr (4x7)
void Ui_Bar(u16*d,int x,int y,int w,int pct,u16 col);
