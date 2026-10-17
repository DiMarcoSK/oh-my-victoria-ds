#pragma once
#include <nds.h>
// OH MY VICTORIA DS! v0.7 — sociedade completa + governos: 8 bens, 23 obras, 3 govs,
// cadeias produtivas, mercados interno+externo+uniao, RTS Vic-like no DS.
#define GAME_TITLE "OH MY VICTORIA DS!"
#define GAME_VERSION "0.7.0"
#define DEFAULT_SEED 0x0CC1A6u
#define MAP_W 32
#define MAP_H 24
#define VIEW_W 16
#define VIEW_H 12
#define MAX_FACTIONS 4
#define PLAYER_ID 1
#define NEUTRAL_ID 0
#define FRAMES_PER_DAY_1 15
#define FRAMES_PER_DAY_2 8
#define FRAMES_PER_DAY_3 4
#define MAX_TROOPS 12
#define MAX_RES 9999
// 8 bens em cadeia: grao > roupa/movel > ferramentas > armas/luxo/vinho; carvao energia.
#define GOOD_GRAIN 0
#define GOOD_CLOTH 1
#define GOOD_FURN 2
#define GOOD_ARMS 3
#define GOOD_LUX 4
#define GOOD_TOOLS 5
#define GOOD_COAL 6
#define GOOD_WINE 7
#define GOOD_COUNT 8
#define POP_PEAS 0
#define POP_WORK 1
#define POP_BURG 2
#define POP_NOBL 3
#define POP_COUNT 4
#define START_TREASURY 400
#define START_STOCK_G 80
#define WIN_PRESTIGE 600
#define DIFF_EASY 0
#define DIFF_NORMAL 1
#define DIFF_HARD 2
#define DIFF_INSANE 3
#define DIFF_COUNT 4
#define EVT_NONE 0
#define EVT_HARVEST 1
#define EVT_PLAGUE 2
#define EVT_GOLDSTRIKE 3
#define EVT_REVOLT 4
#define EVT_ARMSDEAL 5
#define EVT_STORM 6
#define EVT_FESTIVAL 7
#define EVT_COALRUSH 8
