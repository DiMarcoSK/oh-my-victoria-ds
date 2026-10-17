// stub_px.c — Px_Col do host (mesma regra do DS: BIT 15 = opaco, ADR-13).
#include "px.h"
u16 Px_Col(u8 r,u8 g,u8 b){return (u16)(RGB15(r,g,b)|BIT(15));}
