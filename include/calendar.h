#pragma once
#include <nds.h>
// calendar.h — calendario do mapa-mundi (DESIGN.md sec 5 e 7.6).
// Dia 0 = 1 jan 1800. Fim da historia = 31 dez 2025; depois, Modo Eterno
// (o calendario segue sem limite). Gregoriano proleptico, so inteiros.
#define CAL_START_YEAR 1800
#define CAL_END_YEAR 2025

typedef struct { u16 y; u8 m, d; } Date; // m 1..12, d 1..31

typedef enum {
 ERA_NAPOLEONICA=0, // 1800-1814
 ERA_RESTAURACAO,   // 1815-1847
 ERA_POVOS,         // 1848-1870 (Primavera dos Povos)
 ERA_IMPERIALISMO,  // 1871-1913
 ERA_GUERRAS,       // 1914-1944 (Guerras Mundiais)
 ERA_GUERRA_FRIA,   // 1945-1990
 ERA_GLOBALIZACAO,  // 1991-2025
 ERA_ETERNA,        // 2026+ (Modo Eterno)
 ERA_COUNT
} Era;

bool Cal_Leap(int y);
int Cal_MonthDays(int y,int m);
u32 Cal_DaysFrom(int y,int m,int d);  // indice do dia desde 1/1/1800
Date Cal_Date(u32 day);               // inverso (laco por ano: use fora do loop quente)
void Cal_Next(Date *d);               // avanca 1 dia (barato: use no Game_Day)
u32 Cal_EndDay(void);                 // indice de 31/12/2025
Era Cal_Era(int year);
const char *Cal_EraName(Era e);       // "Napoleônica"... (UTF-8)
const char *Cal_MonthShort(int m);    // "jan".."dez"
// "1 jan 1800" (UTF-8 minusculo; Txt_Fold converte p/ a fonte 3x5)
void Cal_Format(const Date *d,char *out,int max);
