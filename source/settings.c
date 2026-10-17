#include "settings.h"
// settings.c — estado das Configuracoes (SDD sec 14.8). Estatico e fora do
// GameState (nao conta no _Static_assert de 14KB) e aplicado 1x no boot.
static Settings S;
Settings *Set_Ptr(void){return &S;}
void Settings_Init(void){
 S.music=80;S.sfx=70;S.lang=LANG_PT;S.layout=LAYOUT_NORMAL;
 // F6 (MaxMod) liga o som; F3 liga a troca de tela (lcdSwap, ADR-16). Ate
 // entao os valores existem e sao configuraveis, mas nao tem efeito: nada
 // quebra (comportamento igual ao da v0.8).
}
