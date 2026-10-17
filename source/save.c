#include "save.h"
static GameState slot;static bool ok=false;
bool Save_W(GameState *g){if(!g)return false;
 for(unsigned i=0;i<sizeof(slot);i++)((u8*)&slot)[i]=((u8*)g)[i];
 ok=true;
 const char *m="SAVE OK";int k=0;while(m[k]){g->status[k]=m[k];k++;}g->status[k]=0;
 return true;}
bool Save_R(GameState *g){if(!g||!ok)return false;
 for(unsigned i=0;i<sizeof(slot);i++)((u8*)g)[i]=((u8*)&slot)[i];
 g->dirty=true;return true;}
bool Save_Ok(void){return ok;}
void Save_Clr(void){ok=false;for(unsigned i=0;i<sizeof(slot);i++)((u8*)&slot)[i]=0;}
