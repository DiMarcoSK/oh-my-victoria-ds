#include "city.h"
#include "hot.h"
#include "earth.h"
#include "str.h"
#include "wecon.h"
// city.c — cidades (DESIGN.md sec 4.7). Ver city.h.
// Custo: City_Day e' O(cidades) (<=512) por dia; buscas por celula passam
// antes pelo bitmap W_CITYB (12KB) e so' entao varrem a lista. Nenhuma
// chamada ao RNG do mundo: escolhas da IA e posicoes usam hash da semente.
u8 W_CITYB[W_CELLS/8];
static inline void SetB(u32 i){W_CITYB[i>>3]|=(u8)(1u<<(i&7));}
static inline bool GetB(u32 i){return (W_CITYB[i>>3]>>(i&7))&1;}
static u32 Hs(u32 a,u32 b){u32 h=a*2654435761u^b*40503u^0x9E3779B9u;h^=h>>15;h*=0x2C1B3C6Du;h^=h>>12;return h;}

// nameStr cost days minTier minYear gold food cap prest def need flags desc
const BldDef BLD[BK_COUNT]={
 [BK_NONE]       ={0,0,0,0,0,0,0,0,0,0,0,0,0},
 [BK_FAZENDA]    ={STR_BK_FAZENDA,     60, 30,0,   0,0,3, 0,0, 0,0,   BF_MULTI,STR_BD_FAZENDA},
 [BK_MERCADO]    ={STR_BK_MERCADO,    120, 45,0,   0,2,1, 0,0, 0,0,          0,STR_BD_MERCADO},
 [BK_OFICINA]    ={STR_BK_OFICINA,    150, 60,1,   0,3,0, 0,0, 0,0,   BF_MULTI,STR_BD_OFICINA},
 [BK_QUARTEL]    ={STR_BK_QUARTEL,    140, 40,0,   0,0,0,30,0,10,0,   BF_MULTI,STR_BD_QUARTEL},
 [BK_PORTO]      ={STR_BK_PORTO,      160, 60,0,   0,3,1, 0,0, 0,0,   BF_COAST,STR_BD_PORTO},
 [BK_TEMPLO]     ={STR_BK_TEMPLO,     110, 50,0,   0,0,1, 0,2, 0,0,          0,STR_BD_TEMPLO},
 [BK_MURALHA]    ={STR_BK_MURALHA,    180, 80,1,   0,0,0,10,0,60,0,          0,STR_BD_MURALHA},
 [BK_UNIVERSIDADE]={STR_BK_UNIVERSIDADE,300,120,2,  0,1,0, 0,5, 0,0,          0,STR_BD_UNIVERSIDADE},
 [BK_FABRICA]    ={STR_BK_FABRICA,    400,120,2,1830,8,0, 0,1, 0,BK_OFICINA, BF_MULTI,STR_BD_FABRICA},
 [BK_FERROVIA]   ={STR_BK_FERROVIA,   350,100,1,1830,4,2, 0,1, 0,0,          0,STR_BD_FERROVIA},
 [BK_PALACIO]    ={STR_BK_PALACIO,    500,150,2,   0,2,0,20,8,20,0, BF_CAPITAL,STR_BD_PALACIO},
 [BK_SERRARIA]   ={STR_BK_SERRARIA,    90, 35,0,   0,0,0, 0,0, 0,0,   BF_WOOD|BF_MULTI,STR_BD_SERRARIA},
 [BK_MINA]       ={STR_BK_MINA,       160, 60,0,   0,1,0, 0,0, 0,0,    BF_ORE|BF_MULTI,STR_BD_MINA},
 [BK_ARSENAL]    ={STR_BK_ARSENAL,    260, 80,1,   0,0,0,15,0, 5,BK_OFICINA,BF_MULTI,STR_BD_ARSENAL},
 [BK_ESTALEIRO]  ={STR_BK_ESTALEIRO,  240, 90,1,   0,1,0, 0,1, 0,BK_PORTO, BF_COAST,STR_BD_ESTALEIRO},
 [BK_POCO]       ={STR_BK_POCO,       300, 90,0,1860,2,0, 0,0, 0,0,    BF_OIL|BF_MULTI,STR_BD_POCO},
 [BK_ESCOLA]     ={STR_BK_ESCOLA,      90, 40,0,   0,0,0, 0,1, 0,0,          0,STR_BD_ESCOLA},
 [BK_HOSPITAL]   ={STR_BK_HOSPITAL,   220, 70,1,1850,0,2, 0,1, 0,0,          0,STR_BD_HOSPITAL},
 [BK_BANCO]      ={STR_BK_BANCO,      260, 70,1,   0,4,0, 0,1, 0,BK_MERCADO, 0,STR_BD_BANCO},
 [BK_TEATRO]     ={STR_BK_TEATRO,     200, 60,2,   0,1,0, 0,3, 0,0,          0,STR_BD_TEATRO},
 [BK_PARQUE]     ={STR_BK_PARQUE,      80, 30,1,1830,0,1, 0,1, 0,0,          0,STR_BD_PARQUE},
 [BK_USINA]      ={STR_BK_USINA,      380,100,2,1890,3,0, 0,0, 0,0,          0,STR_BD_USINA},
 // ADR-37: cadeia longa e financas
 [BK_SIDERURGICA]={STR_BK_SIDERURGICA,420,120,1,1850,2,0, 0,1, 0,0,          BF_MULTI,STR_BD_SIDERURGICA},
 [BK_MANUFATURA] ={STR_BK_MANUFATURA, 220, 70,1,   0,3,0, 0,0, 0,0,          BF_MULTI,STR_BD_MANUFATURA},
 [BK_MAQUINARIA] ={STR_BK_MAQUINARIA, 480,130,2,1870,3,0, 0,1, 0,0,          BF_MULTI,STR_BD_MAQUINARIA},
 [BK_ELETRONICA] ={STR_BK_ELETRONICA, 600,150,2,1950,5,0, 0,2, 0,0,          BF_MULTI,STR_BD_ELETRONICA},
 [BK_ARMAMENTOS] ={STR_BK_ARMAMENTOS, 450,120,1,1850,1,0,25,0, 5,BK_ARSENAL, BF_MULTI,STR_BD_ARMAMENTOS},
 [BK_BOLSA]      ={STR_BK_BOLSA,      520,120,2,   0,6,0, 0,3, 0,BK_BANCO,   0,STR_BD_BOLSA},
};
int City_MaxOf(u8 k){return k<BK_COUNT&&(BLD[k].flags&BF_MULTI)?3:1;}

// Cidades de 1800 (populacao aproximada em milhares). Grafia em PT quando
// existe exonimo consagrado; senao, o nome local (DESIGN 3.4).
typedef struct { const char *name; s8 lat; s16 lon; u16 pop; } HistCity;
static const HistCity HC[]={
 // Europa
 {"Londres",51,0,1000},{"Paris",49,2,550},{"Edimburgo",56,-3,80},{"Dublin",53,-6,180},
 {"Liverpool",53,-3,80},{"Manchester",53,-2,90},{"Lisboa",39,-9,180},{"Porto",41,-9,40},
 {"Madri",40,-4,170},{"Barcelona",41,2,115},{"Sevilha",37,-6,90},{"Valência",39,0,80},
 {"Bordéus",45,-1,90},{"Lyon",46,5,110},{"Marselha",43,5,110},{"Amsterdã",52,5,220},
 {"Antuérpia",51,4,60},{"Bruxelas",51,4,70},{"Hamburgo",54,10,130},{"Berlim",52,13,170},
 {"Viena",48,16,230},{"Praga",50,14,75},{"Peste",47,19,50},{"Munique",48,12,40},
 {"Dresden",51,14,60},{"Copenhague",56,12,100},{"Estocolmo",59,18,75},{"Christiania",60,11,10},
 {"São Petersburgo",60,30,220},{"Moscou",56,38,250},{"Kiev",50,30,20},{"Varsóvia",52,21,65},
 {"Riga",57,24,30},{"Roma",42,12,150},{"Nápoles",41,14,430},{"Milão",45,9,135},
 {"Veneza",45,12,140},{"Florença",44,11,80},{"Turim",45,8,70},{"Palermo",38,13,140},
 {"Constantinopla",41,29,570},{"Atenas",38,24,10},{"Bucareste",44,26,60},{"Belgrado",45,20,25},
 {"Genebra",46,6,25},{"Valeta",36,14,30},{"Sarajevo",44,18,50},{"Königsberg",55,20,55},
 {"Vilnius",55,25,25},{"Kazan",56,49,30},{"Astracã",46,48,30},{"Tbilisi",42,45,20},
 // Oriente Medio e Norte da Africa
 {"Cairo",30,31,260},{"Alexandria",31,30,15},{"Argel",37,3,50},{"Túnis",37,10,90},
 {"Trípoli",33,13,25},{"Fez",34,-5,90},{"Marrakech",32,-8,50},{"Bagdá",33,44,80},
 {"Damasco",34,36,90},{"Alepo",36,37,110},{"Jerusalém",32,35,9},{"Meca",21,40,40},
 {"Medina",24,40,15},{"Diriyah",25,47,10},{"Mascate",24,59,15},{"Teerã",36,51,50},
 {"Isfahan",33,52,100},{"Tabriz",38,46,50},{"Herat",34,62,40},{"Cabul",35,69,60},
 {"Bukhara",40,64,70},{"Samarcanda",40,67,15},{"Khiva",41,60,10},{"Esmirna",38,27,100},
 {"Sana",15,44,30},{"Mossul",36,43,40},
 // Asia do Sul
 {"Délhi",29,77,150},{"Agra",27,78,60},{"Lucknow",27,81,250},{"Calcutá",23,88,400},
 {"Bombaim",19,73,140},{"Madras",13,80,125},{"Hyderabad",17,78,200},{"Pune",19,74,100},
 {"Seringapatam",12,77,40},{"Lahore",32,74,80},{"Amritsar",32,75,40},{"Daca",24,90,100},
 {"Catmandu",28,85,30},{"Colombo",7,80,30},{"Kandy",7,81,15},{"Benares",25,83,180},
 {"Surat",21,73,150},{"Jaipur",27,76,100},
 // Asia do Leste
 {"Pequim",40,116,1100},{"Cantão",23,113,800},{"Nanquim",32,119,500},{"Hangzhou",30,120,380},
 {"Suzhou",31,121,240},{"Xi'an",34,109,220},{"Chengdu",31,104,180},{"Wuhan",31,114,200},
 {"Lhasa",30,91,20},{"Kashgar",39,76,40},{"Urga",48,107,7},{"Seul",38,127,190},
 {"Edo",36,140,1000},{"Quioto",35,136,380},{"Osaka",35,135,380},{"Nagasaki",33,130,60},
 {"Shuri",26,128,30},{"Mukden",42,123,90},{"Kunming",25,103,60},
 // Sudeste Asiatico e ilhas
 {"Bangkok",14,100,50},{"Amarapura",22,96,170},{"Hué",16,108,100},{"Hanói",21,106,80},
 {"Saigon",11,107,60},{"Phnom Penh",12,105,20},{"Batávia",-6,107,50},{"Surakarta",-8,111,100},
 {"Manila",15,121,80},{"Malaca",2,102,10},{"Brunei",5,115,15},{"Makassar",-5,119,20},
 {"Luang Prabang",20,102,15},{"Aceh",6,95,20},
 // Africa
 {"Kumasi",7,-2,40},{"Abomé",7,2,25},{"Oyo",8,4,70},{"Benin",6,6,40},{"Kano",12,8,50},
 {"Tombuctu",17,-3,10},{"Segu",13,-6,30},{"Gondar",13,37,50},{"Harar",9,42,30},
 {"Zanzibar",-6,39,20},{"Mombaça",-4,40,10},{"Mogadíscio",2,45,10},{"Sennar",14,34,20},
 {"Luanda",-9,13,20},{"Mbanza Kongo",-6,14,20},{"Cidade do Cabo",-34,18,20},
 {"Moçambique",-15,41,6},{"Antananarivo",-19,47,30},{"Saint-Louis",16,-16,10},
 {"Freetown",8,-13,5},{"Elmina",5,-1,10},{"Lagos",6,3,20},{"Kukawa",13,13,30},
 {"Ankober",10,40,10},{"Kilwa",-9,39,5},{"Bunyoro",2,31,10},{"Nyanza",-2,30,10},
 // Americas
 {"Filadélfia",40,-75,70},{"Nova York",41,-74,60},{"Boston",42,-71,25},{"Baltimore",39,-77,26},
 {"Charleston",33,-80,19},{"Washington",39,-77,3},{"Nova Orleans",30,-90,8},{"Quebec",47,-71,10},
 {"Montreal",46,-74,9},{"Halifax",45,-64,9},{"Cidade do México",19,-99,130},{"Puebla",19,-98,60},
 {"Guadalajara",21,-103,35},{"Guanajuato",21,-101,55},{"Havana",23,-82,50},{"Cap-Français",20,-72,20},
 {"São Domingos",18,-70,10},{"Kingston",18,-77,30},{"Guatemala",15,-91,25},{"Caracas",10,-67,40},
 {"Bogotá",5,-74,25},{"Quito",0,-79,25},{"Lima",-12,-77,55},{"Cusco",-14,-72,30},
 {"Potosí",-20,-66,25},{"La Paz",-17,-68,30},{"Santiago",-33,-71,30},{"Buenos Aires",-35,-58,40},
 {"Montevidéu",-35,-56,15},{"Assunção",-25,-58,10},{"Rio de Janeiro",-23,-43,45},
 {"Salvador",-13,-38,50},{"Recife",-8,-35,25},{"São Paulo",-24,-47,20},{"Vila Rica",-20,-44,20},
 {"Belém",-1,-48,12},{"São Luís",-3,-44,20},{"Santa Fé",36,-106,3},{"Monterrey",26,-100,10},
 {"Chota",35,-84,2},{"Onondaga",43,-76,2},{"Paha Sápa",44,-103,1},
 // Oceania e Pacifico
 {"Kailua",20,-156,5},{"Papeete",-18,-150,3},{"Nukuʻalofa",-21,-175,3},{"Sydney",-34,151,5},
 {"Kororāreka",-35,174,2},
};
enum { HC_N=sizeof HC/sizeof HC[0] };

// nomes gerados (vilas e cidades fundadas): silaba inicial + final no
// estilo da cultura de quem fundou (256 por estilo). Codificado no u16
// `name`: bits 0-3 inicio, 4-7 fim, 13-15 estilo. Nao e' historico: soa da
// regiao sem fingir ser um lugar real.
static const char *const SY0[ST_COUNT][16]={
 {"Al","Bel","Cor","Dor","Fal","Gran","Hal","Lan","Mar","Nor","Ros","San","Ter","Val","Wes","Ald"},
 {"Shan","Kyo","Hai","Lin","Nan","Pur","Sri","Ban","Chi","Tai","Har","Jai","Kan","Mal","Ra","Su"},
 {"Al","Bah","Dar","Has","Kar","Mar","Nab","Qas","Sam","Tab","Zar","Kir","Mos","Shir","Bas","Yaz"},
 {"Ka","Bu","Ma","Ny","Ki","Lo","Go","Ta","Sa","Mbo","Za","Do","Ou","Ba","Ko","Ga"},
 {"Ta","Ko","Wa","Shi","Na","Che","Mo","Oki","Ha","Ya","Pa","Ma","Ki","To","Ne","Ah"}};
static const char *const SY1[ST_COUNT][16]={
 {"ford","burg","ville","mont","berg","stad","ia","heim","by","ona","ez","ac","ham","dorf","polis","ov"},
 {"pur","abad","gar","shan","jing","zhou","ma","kot","nagar","an","wan","yang","ra","mu","ka","ri"},
 {"abad","an","dar","iyya","kand","ra","stan","ur","ira","ah","ul","ein","sar","ab","ar","is"},
 {"mba","ngo","la","di","bu","ra","ko","se","no","ga","wa","ri","mi","nde","so","ta"},
 {"ka","wee","nee","ta","po","sha","we","ho","ma","ni","to","ke","hu","ya","la","wa"}};
static u8 StyleOf(const World *w,u8 owner);
static u16 GenName(const World *w,u8 owner,u32 h){return (u16)(((u32)StyleOf(w,owner)<<13)|(h&0xFF));}

int City_Slots(u8 tier){static const u8 S[CT_COUNT]={3,6,10,16,16};return tier<CT_COUNT?S[tier]:3;}
int City_NextPop(u8 tier){static const u16 N[CT_COUNT]={10,50,400,5000,0};return tier<CT_COUNT?N[tier]:0;}
// nivel das obras (ADR-44): 1..tier+1; a megalopole amplia ate' 5
int City_Level(const WCity *c,int s){return c->bld[s]?(c->blv[s]?c->blv[s]:1):0;}
int City_MaxLevel(u8 tier){return tier<CT_COUNT?tier+1:1;}
static bool Has(const WCity *c,u8 k){for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==k)return true;return false;}
static int Count(const WCity *c,u8 k){int n=0;for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]==k)n++;return n;}
int City_Count(const WCity *c,u8 k){return Count(c,k);}
HOT_ARM int City_PopCap(const WCity *c){
 static const u16 B[CT_COUNT]={14,70,460,3600,12000};
 int m=100;
 for(int s=0;s<CITY_SLOTS;s++){int l=c->blv[s]?c->blv[s]:1;   // uma passada (roda todo dia p/ cada cidade)
  switch(c->bld[s]){case BK_FAZENDA:m+=20*l;break;case BK_PORTO:m+=10*l;break;case BK_FERROVIA:m+=15*l;break;
   case BK_USINA:m+=10*l;break;case BK_HOSPITAL:m+=8*l;break;default:break;}}
 s32 v=(s32)B[c->tier%CT_COUNT]*m/100;
 return v>60000?60000:(int)v;
}
// megalopole (ADR-44): 5 milhoes de habitantes + bolsa ou usina (finanças e energia)
static bool TierReq(const WCity *c,u8 to){
 if(c->pop<City_NextPop((u8)(to-1)))return false;
 if(to==CT_CIDADE)return Has(c,BK_MERCADO);
 if(to==CT_METROPOLE)return Has(c,BK_FABRICA)||Has(c,BK_UNIVERSIDADE);
 if(to==CT_MEGALOPOLE)return Has(c,BK_BOLSA)||Has(c,BK_USINA);
 return true;
}
static int Dist(const World *w,int ax,int ay,int bx,int by){
 int dx=ax-bx;if(dx<0)dx=-dx;
 if(w->wrap&&dx>w->w/2)dx=w->w-dx;
 int dy=ay-by;if(dy<0)dy=-dy;
 return dx>dy?dx:dy;
}
int City_At(const World *w,int x,int y){
 if(x<0||y<0||x>=w->w||y>=w->h||!GetB(World_Idx(w,x,y)))return -1;
 for(int i=0;i<w->nCity;i++)if(w->city[i].x==x&&w->city[i].y==y)return i;
 return -1;
}
int City_Near(const World *w,int x,int y,int r){
 int best=-1,bd=r+1;
 for(int i=0;i<w->nCity;i++){int d=Dist(w,x,y,w->city[i].x,w->city[i].y);if(d<bd){bd=d;best=i;}}
 return best;
}
bool City_IsCapital(const World *w,int i){
 const WCity *c=&w->city[i];
 if(!c->owner)return false;
 const WCountry *C=&w->c[c->owner];
 return Dist(w,c->x,c->y,C->capX,C->capY)<=2;
}
static bool Coastal(const World *w,int x,int y){
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  int nx=x+dx,ny=y+dy;
  if(ny<0||ny>=w->h)continue;
  if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
  if(W_TERR[World_Idx(w,nx,ny)]==WT_WATER)return true;
 }
 return false;
}
// latitude aproximada da linha y (mapa-mundi: projecao da Terra; aleatorio: faixa)
static int LatOf(const World *w,int y){
 if(w->mode==WM_MUNDI)return EARTH_LAT_TOP-(y*EARTH_LAT_SPAN)/w->h;
 return 90-(y*180)/(w->h?w->h:1);
}
// potencial do entorno (raio 3, 49 celulas): lavoura, mata, minerio, clima e
// petroleo. Calculado 1x ao criar a cidade; a economia so' le (wecon.c).
static void Survey(const World *w,WCity *c){
 int ag=0,wd=0,ore=0,oil=0,jun=0;
 for(int dy=-3;dy<=3;dy++)for(int dx=-3;dx<=3;dx++){
  int nx=c->x+dx,ny=c->y+dy;
  if(ny<0||ny>=w->h)continue;
  if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
  switch(W_TERR[World_Idx(w,nx,ny)]){
   case WT_PLAINS:ag+=3;break;
   case WT_STEPPE:ag+=2;oil++;break;
   case WT_FOREST:wd+=3;ag+=1;break;
   case WT_JUNGLE:wd+=2;ag+=1;jun++;break;
   case WT_HILLS:ore+=2;ag+=1;break;
   case WT_MOUNTAIN:ore+=4;break;
   case WT_DESERT:oil+=2;break;
   case WT_SWAMP:oil+=1;wd+=1;break;
   case WT_TUNDRA:wd+=1;oil+=1;break;
   default:break;
  }
 }
 c->agri=(u8)(ag>255?255:ag);c->wood=(u8)(wd>255?255:wd);c->ore=(u8)(ore>255?255:ore);
 int lat=LatOf(w,c->y);
 c->res=0;
 if((lat<=23&&lat>=-23)||jun>=4)c->res|=RS_TROPICAL;
 // petroleo: deserto/estepe/pantano e um sorteio fixo pela posicao (nem todo deserto tem)
 if(oil>=4&&(Hs((u32)c->x*977u+c->y,w->seed^0x0117u)%3)!=0)c->res|=RS_OLEO;
 c->food=100;
}
// ---------------- mapa da cidade: tracado ----------------
// lado do mar: conta agua em volta (raio 2) por lado; 0 = sem mar
enum { SD_NONE=0, SD_E, SD_W, SD_N, SD_S };
static int SeaSide(const World *w,const WCity *c){
 if(!(c->flags&CF_COASTAL))return SD_NONE;
 int n[5]={0,0,0,0,0};
 for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
  int x=c->x+dx,y=c->y+dy;if(y<0||y>=w->h)continue;
  if(x<0||x>=w->w){if(!w->wrap)continue;x=(x+w->w)%w->w;}
  if(W_TERR[World_Idx(w,x,y)]!=WT_WATER)continue;
  if(dx>0)n[SD_E]++;
  if(dx<0)n[SD_W]++;
  if(dy<0)n[SD_N]++;
  if(dy>0)n[SD_S]++;
 }
 int b=SD_E;for(int k=SD_E;k<=SD_S;k++)if(n[k]>n[b])b=k;
 return n[b]?b:SD_E;
}
static u8 LAY[CG_N];static int layCity=-1;static u32 layKey=0;
static void Layout(const World *w,int ci){
 const WCity *c=&w->city[ci];
 u32 key=((u32)c->x<<16)^c->y^((u32)c->flags<<28)^w->seed;
 if(layCity==ci&&layKey==key)return;
 layCity=ci;layKey=key;
 u32 h=Hs(c->x*7919u+c->y,w->seed^0xC17Au);
 for(int i=0;i<CG_N;i++)LAY[i]=LK_TERRA;
 // mar num dos lados (costa irregular) ou rio atravessando (1 em 3)
 int sd=SeaSide(w,c);
 for(int y=0;y<CG_H;y++)for(int x=0;x<CG_W;x++){
  u32 q=Hs(h,(u32)(sd==SD_N||sd==SD_S?x:y));int dep=3+(int)(q%2);
  bool wat=(sd==SD_E&&x>=CG_W-dep)||(sd==SD_W&&x<dep)||(sd==SD_N&&y<dep-1)||(sd==SD_S&&y>=CG_H-dep+1);
  if(wat)LAY[y*CG_W+x]=LK_AGUA;
 }
 if(sd==SD_NONE&&(h%3)==0){
  int rx=3+(int)((h>>4)%10);
  for(int y=0;y<CG_H;y++){LAY[y*CG_W+rx]=LK_AGUA;u32 q=Hs(h,(u32)y+50);if((q&3)==0&&rx<CG_W-2)rx++;else if((q&3)==1&&rx>1)rx--;}
 }
 // avenidas (cruz) + ruas secundarias; ponte onde cruza o rio
 int r0=CG_H/2-1+(int)((h>>8)%2),c0=CG_W/2-2+(int)((h>>10)%4);
 for(int x=0;x<CG_W;x++){int i=r0*CG_W+x;if(LAY[i]!=LK_AGUA||sd==SD_NONE)LAY[i]=LK_RUA;}
 for(int y=0;y<CG_H;y++){int i=y*CG_W+c0;if(LAY[i]!=LK_AGUA||sd==SD_NONE)LAY[i]=LK_RUA;}
 static const s8 SR[2]={-3,3},SC[2]={-4,4};
 for(int k=0;k<2;k++){
  int r=r0+SR[k];if(r>=0&&r<CG_H)for(int x=c0-6;x<=c0+6;x++)if(x>=0&&x<CG_W&&LAY[r*CG_W+x]==LK_TERRA)LAY[r*CG_W+x]=LK_RUA;
  int cc=c0+SC[k];if(cc>=0&&cc<CG_W)for(int y=r0-3;y<=r0+3;y++)if(y>=0&&y<CG_H&&LAY[y*CG_W+cc]==LK_TERRA)LAY[y*CG_W+cc]=LK_RUA;
  // anel de fora (ADR-44): ruas p/ onde a cidade grande se espalha
  int r2=r0+SR[k]*2-(k?1:-1);if(r2>=0&&r2<CG_H)for(int x=c0-9;x<=c0+9;x++)if(x>=0&&x<CG_W&&LAY[r2*CG_W+x]==LK_TERRA&&((x+r2)%5))LAY[r2*CG_W+x]=LK_RUA;
  int c2=c0+SC[k]*2;if(c2>=0&&c2<CG_W)for(int y=r0-5;y<=r0+5;y++)if(y>=0&&y<CG_H&&LAY[y*CG_W+c2]==LK_TERRA&&((y+c2)%4))LAY[y*CG_W+c2]=LK_RUA;
 }
 int pl=(r0-1)*CG_W+c0+1;if(LAY[pl]==LK_TERRA)LAY[pl]=LK_PRACA;
}
u8 City_LotBase(const World *w,int ci,int lot){
 if(lot<0||lot>=CG_N)return LK_AGUA;
 Layout(w,ci);return LAY[lot];
}
bool City_LotCoast(const World *w,int ci,int lot){
 if(City_LotBase(w,ci,lot)!=LK_TERRA)return false;
 int x=lot%CG_W,y=lot/CG_W;
 static const s8 D4[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
 for(int k=0;k<4;k++){int nx=x+D4[k][0],ny=y+D4[k][1];
  if(nx<0||ny<0||nx>=CG_W||ny>=CG_H)continue;
  if(LAY[ny*CG_W+nx]==LK_AGUA)return true;}
 return false;
}
int City_Plaza(const World *w,int ci){
 Layout(w,ci);
 for(int i=0;i<CG_N;i++)if(LAY[i]==LK_PRACA)return i;
 return (CG_H/2-2)*CG_W+CG_W/2-1;
}
// limite urbano (ADR-44): o povoado ocupa o miolo; a cidade se espalha pelo
// mapa conforme o nivel. Campo e extracao (fazenda, mina, serraria, poco)
// podem ir alem do limite: sao os arredores.
int City_Radius(u8 tier){static const u8 R[CT_COUNT]={3,5,7,9,99};return tier<CT_COUNT?R[tier]:3;}
bool City_LotInside(const World *w,int ci,int lot){
 int pl=City_Plaza(w,ci),dx=lot%CG_W-pl%CG_W,dy=lot/CG_W-pl/CG_W;
 if(dx<0)dx=-dx;
 if(dy<0)dy=-dy;
 return (dx>dy?dx:dy)<=City_Radius(w->city[ci].tier);
}
static bool Rural(u8 k){return k==BK_FAZENDA||k==BK_MINA||k==BK_SERRARIA||k==BK_POCO;}
int City_LotSlot(const WCity *c,int lot){
 for(int s=0;s<CITY_SLOTS;s++)if(c->bld[s]&&c->bpos[s]==lot)return s;
 return -1;
}
static bool LotFree(const World *w,int ci,int lot){
 const WCity *c=&w->city[ci];
 if(City_LotBase(w,ci,lot)!=LK_TERRA||City_LotSlot(c,lot)>=0)return false;
 if(c->bq&&c->bqLot==lot)return false;
 return true;
}
// lote livre mais perto da praca (porto/estaleiro: na beira d'agua); hash
// desempata p/ a cidade nao virar um tabuleiro
int City_AutoLot(const World *w,int ci,u8 k){
 int pl=City_Plaza(w,ci),px=pl%CG_W,py=pl/CG_W;
 bool coast=k<BK_COUNT&&(BLD[k].flags&BF_COAST);
 int best=-1,bd=1<<30;
 for(int i=0;i<CG_N;i++){
  if(!LotFree(w,ci,i))continue;
  if(coast&&!City_LotCoast(w,ci,i))continue;
  if(!Rural(k)&&!City_LotInside(w,ci,i))continue;
  int dx=i%CG_W-px,dy=i/CG_W-py;
  int d=(dx*dx+dy*dy)*8+(int)(Hs((u32)ci*131u+(u32)i,k)%7);
  // campo e extracao: nos arredores, logo alem do limite urbano (nem no
  // centro, nem na borda do mapa)
  if(Rural(k)){int ax=dx<0?-dx:dx,ay=dy<0?-dy:dy,ch=ax>ay?ax:ay,r=City_Radius(w->city[ci].tier)+1;if(r>7)r=7;
   d=(ch>r?ch-r:r-ch)*64+(int)(Hs((u32)ci*131u+(u32)i,k)%7);}
  if(d<bd){bd=d;best=i;}
 }
 return best;
}
bool City_CanBuildAt(const World *w,int ci,int lot,u8 k){
 if(!City_CanBuild(w,ci,k)||!LotFree(w,ci,lot))return false;
 if(!Rural(k)&&!City_LotInside(w,ci,lot))return false;
 if((BLD[k].flags&BF_COAST)&&!City_LotCoast(w,ci,lot))return false;
 return true;
}
static bool Has(const WCity *c,u8 k);
// industria (oficina, fabrica, arsenal, usina) colada (8 vizinhos) numa
// ferrovia ou porto produz +25%: o lugar da obra importa
int City_IndustryBonus(const WCity *c,int slot){
 u8 k=c->bld[slot];
 if(k!=BK_OFICINA&&k!=BK_FABRICA&&k!=BK_ARSENAL&&k!=BK_USINA&&k!=BK_SIDERURGICA&&k!=BK_MANUFATURA&&
    k!=BK_MAQUINARIA&&k!=BK_ELETRONICA&&k!=BK_ARMAMENTOS)return 0;
 int lx=c->bpos[slot]%CG_W,ly=c->bpos[slot]/CG_W;
 if(c->bpos[slot]>=CG_N)return 0;
 for(int s=0;s<CITY_SLOTS;s++){
  u8 b=c->bld[s];if(b!=BK_FERROVIA&&b!=BK_PORTO)continue;
  if(c->bpos[s]>=CG_N)continue;
  int dx=c->bpos[s]%CG_W-lx,dy=c->bpos[s]/CG_W-ly;
  if(dx>=-1&&dx<=1&&dy>=-1&&dy<=1)return 25;
 }
 return 0;
}
static int Add(World *w,int x,int y,u16 pop,u8 flags,u16 name){
 if(w->nCity>=W_MAXCITY)return -1;
 u32 i=World_Idx(w,x,y);
 WCity *c=&w->city[w->nCity];
 u8 *p=(u8*)c;for(unsigned k=0;k<sizeof *c;k++)p[k]=0;
 c->x=(u16)x;c->y=(u16)y;c->owner=W_OWNER[i];c->pop=pop;c->name=name;
 for(int s=0;s<CITY_SLOTS;s++){c->bpos[s]=0xFF;}
 c->bqLot=0xFF;
 c->flags=(u8)(flags|(Coastal(w,x,y)?CF_COASTAL:0));
 Survey(w,c);
 SetB(i);
 return w->nCity++;
}
// nivel e obras iniciais coerentes com o tamanho (cidades de 1800 ja tem
// mercado, templo...; uma metropole historica ja tem universidade). Toda
// cidade comeca com slots livres: a escolha do que erguer e' do jogador.
static void InitCity(World *w,int i){
 WCity *c=&w->city[i];
 c->tier=c->pop>=400?CT_METROPOLE:c->pop>=50?CT_CIDADE:c->pop>=10?CT_VILAREJO:CT_POVOADO;
 static const u8 L[CT_COUNT][6]={
  {BK_FAZENDA,0},
  {BK_FAZENDA,BK_TEMPLO,0},
  {BK_FAZENDA,BK_MERCADO,BK_TEMPLO,0},
  {BK_FAZENDA,BK_MERCADO,BK_TEMPLO,BK_UNIVERSIDADE,0},
  {BK_FAZENDA,BK_MERCADO,BK_TEMPLO,BK_UNIVERSIDADE,0}};  // sempre sobram slots p/ o jogador
 int s=0;
 for(int k=0;k<6&&L[c->tier][k];k++)c->bld[s++]=L[c->tier][k];
 if(c->tier>=CT_CIDADE&&(c->flags&CF_COASTAL)&&s<City_Slots(c->tier))c->bld[s++]=BK_PORTO;
 // potencias navais de 1800 ja tem estaleiro nos grandes portos
 if(c->tier>=CT_CIDADE&&(c->flags&CF_COASTAL)&&c->owner&&s<City_Slots(c->tier)-2){
  const WCountry *O=&w->c[c->owner];bool sea=false;
  for(int t=0;t<NAT_TRAITS;t++)if(O->traits[t]==TR_MARES||O->traits[t]==TR_COLONIAL||O->traits[t]==TR_ENTREPOSTO||O->traits[t]==TR_CORSARIOS)sea=true;
  if(sea)c->bld[s++]=BK_ESTALEIRO;
 }
 // 1800: as grandes cidades ja tem oficinas; minas e serrarias onde ha recurso
 if(c->tier>=CT_CIDADE&&s<City_Slots(c->tier)-2)c->bld[s++]=BK_OFICINA;
 if(c->tier>=CT_VILAREJO&&c->ore>=24&&s<City_Slots(c->tier)-2)c->bld[s++]=BK_MINA;
 else if(c->tier>=CT_VILAREJO&&c->wood>=30&&s<City_Slots(c->tier)-2)c->bld[s++]=BK_SERRARIA;
 if(c->tier>=CT_METROPOLE&&City_IsCapital(w,i)&&s<City_Slots(c->tier)-1)c->bld[s++]=BK_PALACIO;
 if(c->tier>=CT_CIDADE&&s<City_Slots(c->tier)-2)c->bld[s++]=BK_ESCOLA;
 // cada obra inicial ganha um lote no mapa da cidade
 for(int k=0;k<CITY_SLOTS;k++)if(c->bld[k]){int l=City_AutoLot(w,i,c->bld[k]);c->bpos[k]=(u8)(l<0?0xFF:l);}
 // as grandes cidades de 1800 ja' tem a fazenda e o mercado ampliados
 for(int k=0;k<CITY_SLOTS;k++){u8 b=c->bld[k];c->blv[k]=(u8)((b&&c->tier>=CT_CIDADE&&(b==BK_FAZENDA||b==BK_MERCADO))?2:(b?1:0));}
}
// terra mais proxima (ate r) de (x,y)
static bool NearLand(const World *w,int *x,int *y,int r){
 for(int d=0;d<=r;d++)for(int dy=-d;dy<=d;dy++)for(int dx=-d;dx<=d;dx++){
  int nx=*x+dx,ny=*y+dy;
  if(ny<0||ny>=w->h)continue;
  if(nx<0||nx>=w->w){if(!w->wrap)continue;nx=(nx+w->w)%w->w;}
  if(W_TERR[World_Idx(w,nx,ny)]!=WT_WATER){*x=nx;*y=ny;return true;}
 }
 return false;
}
void City_Gen(World *w){
 w->nCity=0;
 for(u32 i=0;i<sizeof W_CITYB;i++)W_CITYB[i]=0;
 if(w->mode==WM_MUNDI){
  for(int k=0;k<HC_N;k++){
   int x=Earth_X(HC[k].lon),y=Earth_Y(HC[k].lat);
   if(!NearLand(w,&x,&y,2)||City_Near(w,x,y,1)>=0)continue;
   Add(w,x,y,HC[k].pop,CF_HIST,(u16)k);
  }
 }
 // capital de cada pais sem cidade: recebe uma (nome gerado pela nacao)
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *C=&w->c[ci];
  if(!C->alive)continue;
  int n=City_Near(w,C->capX,C->capY,2);
  if(n>=0&&w->city[n].owner==ci)continue;
  if(n>=0)continue;  // ha uma cidade de outro dono colada: nao duplica
  Add(w,C->capX,C->capY,(u16)(8+C->cells/25),0,GenName(w,ci,Hs(ci,w->seed)));
 }
 // vilas: ~1 a cada 140 celulas (ate 6), longe das outras
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *C=&w->c[ci];
  if(!C->alive)continue;
  int want=(int)(C->cells/140);if(want>6)want=6;
  int have=0;for(int i=0;i<w->nCity;i++)if(w->city[i].owner==ci)have++;
  int bw=C->bx1-C->bx0+1,bh=C->by1-C->by0+1;
  for(int t=0;t<80&&have<want;t++){
   u32 h=Hs(ci*977u+t,w->seed);
   int x=C->bx0+(int)(h%(u32)bw),y=C->by0+(int)((h>>12)%(u32)bh);
   if(x>=w->w||y>=w->h)continue;
   u32 i=World_Idx(w,x,y);
   if(W_OWNER[i]!=ci||W_TERR[i]==WT_WATER||City_Near(w,x,y,7)>=0)continue;
   if(Add(w,x,y,(u16)(3+(h>>24)%8),0,GenName(w,ci,h>>8))>=0)have++;
  }
 }
 for(int i=0;i<w->nCity;i++)InitCity(w,i);
}
void City_OnOwner(World *w,u32 cell,u8 to){
 if(!GetB(cell))return;
 int x=(int)(cell%w->w),y=(int)(cell/w->w);
 int i=City_At(w,x,y);if(i<0)return;
 WCity *c=&w->city[i];
 c->owner=to;c->pop=(u16)(c->pop-c->pop/8);   // saque e fuga na conquista
 c->bq=0;c->bqDays=0;
}
s32 City_FoundCost(const World *w,u8 ci){return WEco_Nom(w,ci,200+120*(s32)w->c[ci].nCity);}
bool City_CanFound(const World *w,u8 ci,int x,int y){
 if(x<0||y<0||x>=w->w||y>=w->h||w->nCity>=W_MAXCITY)return false;
 u32 i=World_Idx(w,x,y);
 return W_OWNER[i]==ci&&W_TERR[i]!=WT_WATER&&W_TERR[i]!=WT_ICE&&City_Near(w,x,y,5)<0;
}
int City_Found(World *w,u8 ci,int x,int y){
 s32 cost=City_FoundCost(w,ci);
 if(!City_CanFound(w,ci,x,y)||w->c[ci].gold<cost)return -1;
 w->c[ci].gold-=cost;
 int i=Add(w,x,y,3,CF_FOUNDED,GenName(w,ci,Hs((u32)x*131u+(u32)y,w->day)));
 if(i>=0){w->city[i].bld[0]=BK_FAZENDA;{int l=City_AutoLot(w,i,BK_FAZENDA);w->city[i].bpos[0]=(u8)(l<0?0xFF:l);}w->c[ci].nCity++;}
 return i;
}
bool City_CanBuild(const World *w,int i,u8 k){
 if(k==BK_NONE||k>=BK_COUNT)return false;
 const WCity *c=&w->city[i];const BldDef *b=&BLD[k];
 if(c->bq||Count(c,k)>=City_MaxOf(k))return false;
 int used=0;for(int s=0;s<City_Slots(c->tier);s++)if(c->bld[s])used++;
 if(used>=City_Slots(c->tier))return false;
 if(c->tier<b->minTier||w->date.y<b->minYear)return false;
 if(b->need&&!Has(c,b->need))return false;
 if((b->flags&BF_COAST)&&!(c->flags&CF_COASTAL))return false;
 if((b->flags&BF_CAPITAL)&&!City_IsCapital(w,i))return false;
 if((b->flags&BF_WOOD)&&c->wood<6)return false;
 if((b->flags&BF_ORE)&&c->ore<6)return false;
 if((b->flags&BF_OIL)&&!(c->res&RS_OLEO))return false;
 return true;
}
// preco da obra: madeira e ferramentas caras no mercado encarecem (75%..150%)
s32 City_Cost(const World *w,int i,u8 k){
 const WCity *c=&w->city[i];
 const WEco *e=&w->eco[c->owner];
 s32 p=e->price[GD_MADEIRA]+e->price[GD_FERRAMENTAS],b=GD_BASE[GD_MADEIRA]+GD_BASE[GD_FERRAMENTAS];
 s32 m=100+(p-b)*50/b;
 if(m<75)m=75;
 if(m>150)m=150;
 return WEco_Nom(w,c->owner,(s32)BLD[k].cost*m/100);
}
// enfileira a obra k (slot livre + lote automatico); quem paga e' o chamador
static bool Queue(World *w,int i,int slot,u8 k){
 WCity *c=&w->city[i];
 if(slot<0||slot>=City_Slots(c->tier)||c->bld[slot]){
  slot=-1;for(int s=0;s<City_Slots(c->tier);s++)if(!c->bld[s]){slot=s;break;}
  if(slot<0)return false;
 }
 int lot=City_AutoLot(w,i,k);
 if(lot<0&&(BLD[k].flags&BF_COAST))return false;
 c->bq=k;c->bqDays=BLD[k].days;c->bqSlot=(u8)slot;c->bqLot=(u8)(lot<0?0xFF:lot);c->bqUp=0;
 return true;
}
bool City_Build(World *w,int i,int slot,u8 k){
 WCity *c=&w->city[i];
 if(!c->owner||!City_CanBuild(w,i,k)||w->c[c->owner].gold<City_Cost(w,i,k))return false;
 s32 cost=City_Cost(w,i,k);
 if(!Queue(w,i,slot,k))return false;
 w->c[c->owner].gold-=cost;
 return true;
}
bool City_BuildAt(World *w,int i,int lot,u8 k){
 WCity *c=&w->city[i];
 if(!c->owner||!City_CanBuildAt(w,i,lot,k)||w->c[c->owner].gold<City_Cost(w,i,k))return false;
 int slot=-1;for(int s=0;s<City_Slots(c->tier);s++)if(!c->bld[s]){slot=s;break;}
 if(slot<0)return false;
 w->c[c->owner].gold-=City_Cost(w,i,k);
 c->bq=k;c->bqDays=BLD[k].days;c->bqSlot=(u8)slot;c->bqLot=(u8)lot;c->bqUp=0;
 return true;
}
// obra doada (a burguesia paga, ADR-42): entra na fila sem cobrar do Estado
bool City_Grant(World *w,int i,u8 k){
 if(i<0||i>=w->nCity||!City_CanBuild(w,i,k))return false;
 return Queue(w,i,-1,k);
}
// ---- ampliar (ADR-44): a obra do slot sobe 1 nivel ----
// custo: o preco da obra x (nivel+1)/2; prazo: 2/3 da obra nova. Uma obra por
// vez na cidade (a ampliacao ocupa a fila como uma obra nova).
s32 City_UpCost(const World *w,int i,int s){
 const WCity *c=&w->city[i];
 return City_Cost(w,i,c->bld[s])*(City_Level(c,s)+1)/2;
}
u16 City_UpWhy(const World *w,int i,int s){
 const WCity *c=&w->city[i];
 if(s<0||s>=CITY_SLOTS||!c->bld[s])return STR_VAZIO;
 if(c->bq)return STR_JA_EM_OBRA;
 if(City_Level(c,s)>=City_MaxLevel(c->tier))return City_Level(c,s)>=City_MaxLevel(CT_COUNT-1)?STR_NIVEL_MAX:STR_REQ_NIVEL_CID;
 if(w->date.y<BLD[c->bld[s]].minYear)return STR_REQ_ANO;
 return 0;
}
static void QueueUp(World *w,int i,int s){
 WCity *c=&w->city[i];
 c->bq=c->bld[s];c->bqDays=(u16)(BLD[c->bld[s]].days*2/3+1);c->bqSlot=(u8)s;c->bqLot=c->bpos[s];c->bqUp=1;
}
bool City_Upgrade(World *w,int i,int s){
 WCity *c=&w->city[i];
 if(!c->owner||City_UpWhy(w,i,s))return false;
 s32 cost=City_UpCost(w,i,s);
 if(w->c[c->owner].gold<cost)return false;
 w->c[c->owner].gold-=cost;QueueUp(w,i,s);
 return true;
}
void City_Name(const World *w,int i,char *out,int max){
 const WCity *c=&w->city[i];
 const char *a,*b=0;
 if(c->flags&CF_HIST)a=HC[c->name%HC_N].name;
 else{int st=(c->name>>13)%ST_COUNT;a=SY0[st][c->name&15];b=SY1[st][(c->name>>4)&15];}
 int n=0;
 while(*a&&n<max-1)out[n++]=*a++;
 if(b)while(*b&&n<max-1)out[n++]=*b++;
 out[n]=0;
 (void)w;
}
u8 City_Style(const World *w,int i){return StyleOf(w,w->city[i].owner);}
u8 City_StyleOf(const World *w,u8 owner){return StyleOf(w,owner);}
static u8 StyleOf(const World *w,u8 owner){
 if(!owner)return ST_EURO;
 const NationDef *d=&NATIONS[w->c[owner].nat];
 bool povo=(d->kind==NK_POVO||d->kind==NK_AUTONOMO);
 switch(d->scene){
  case SC_ASIA_LESTE:case SC_SUDESTE:case SC_ASIA_SUL:return ST_ASIA;
  case SC_AFR_NORTE:case SC_ORIENTE_MEDIO:case SC_ESTEPE:return ST_ISLAM;
  case SC_AFR_OCID:case SC_AFR_LESTE_SUL:return ST_AFRICA;
  case SC_AMN_PLANICIES:case SC_ARTICO:return ST_NATIVE;
  case SC_MEDITERRANEO:return d->tag[0]=='O'&&d->tag[1]=='T'?ST_ISLAM:ST_EURO;
  default:return povo&&(d->cont==CONT_AMERICAS||d->cont==CONT_OCEANIA)?ST_NATIVE:ST_EURO;
 }
}

// ---- IA: constroi por prioridade e funda quando tem ouro e terra vazia ----
// ordem pensada no caminho de nivel: mercado (-> cidade) e universidade
// (-> metropole) antes de quartel/muralha, senao a cidade lota os slots
// A IA pesa o que o mercado pede: bem em falta (preco alto) puxa a obra que
// o produz; o caminho de nivel (mercado -> cidade, universidade/fabrica ->
// metropole) e a guerra (quartel, muralha, arsenal) somam pontos.
static int Want(const WEco *e,u8 g){return e->price[g]*100/GD_BASE[g]+(100-e->sat[g]);}
static int AiScore(const World *w,int i,u8 k){
 const WCity *c=&w->city[i];const WCountry *C=&w->c[c->owner];const WEco *e=&w->eco[c->owner];
 int s=0;
 switch(k){
  case BK_SIDERURGICA:s=40+Want(e,GD_ACO)*2/3;break;
  case BK_MANUFATURA:s=40+Want(e,GD_BENS)/2;break;
  case BK_MAQUINARIA:s=40+Want(e,GD_MAQUINAS)/2;break;
  case BK_ELETRONICA:s=60+Want(e,GD_ELETRONICOS)/2;break;
  case BK_ARMAMENTOS:s=(C->warN?100:20)+Want(e,GD_ARMAS)/2+(C->archetype==WA_MERCADOR||C->armsTo?40:0);break;
  case BK_BOLSA:s=60+(e->invest<200?20:0);break;
  case BK_FAZENDA:s=60+(100-c->food)*2+Want(e,GD_GRAOS)/2;break;
  case BK_MERCADO:s=c->tier>=CT_VILAREJO?150:70;break;
  case BK_TEMPLO:s=40+(C->happy<50?60:0);break;
  case BK_UNIVERSIDADE:s=c->tier>=CT_CIDADE?120:0;break;
  case BK_OFICINA:s=50+Want(e,GD_FERRAMENTAS)/2;break;
  case BK_FABRICA:s=80+Want(e,GD_TEXTEIS)/2+Want(e,GD_FERRAMENTAS)/2;break;
  case BK_SERRARIA:s=Want(e,GD_MADEIRA)+c->wood;break;
  case BK_MINA:s=Want(e,GD_MINERIO)+c->ore;break;
  case BK_ARSENAL:s=(C->warN?120:30)+Want(e,GD_ARMAS)/2;break;
  case BK_POCO:s=60+Want(e,GD_PETROLEO);break;
  case BK_PORTO:s=90+(C->archetype==WA_MERCADOR?60:0);break;
  case BK_ESTALEIRO:s=(C->archetype==WA_MERCADOR||C->m.tradePct>=10)?110:30;break;
  case BK_FERROVIA:s=90;break;
  case BK_QUARTEL:s=C->warN?110:40;break;
  case BK_MURALHA:s=C->warN?80:20;break;
  case BK_PALACIO:s=60;break;
  case BK_ESCOLA:s=55+(w->eco[c->owner].law[LW_EDUCACAO]>=2?25:0);break;
  case BK_HOSPITAL:s=50+(c->food<80?20:0);break;
  case BK_BANCO:s=45+(w->eco[c->owner].debt>0?50:0);break;
  case BK_TEATRO:s=35+(C->happy<55?40:0);break;
  case BK_PARQUE:s=30+(C->happy<55?30:0);break;
  case BK_USINA:s=70+Want(e,GD_TEXTEIS)/4;break;
  default:break;
 }
 // a 2a/3a copia so' vale se o mercado ainda pede (e se ha' gente p/ as vagas)
 int n=Count(c,k);
 if(n){s=s*2/(2+n*2);if(WEco_CityJobs(c,0)<90)s/=2;}
 return s;
}
// ---- investimento privado (ADR-37) ----
// O fundo do pais (WEco.invest, enchido pelo PIB em wecon.c) ergue a obra que
// o mercado mais pede, na cidade que mais combina. Obra de Estado (quartel,
// muralha, palacio, templo, escola...) fica com o governo. Nas cidades do
// jogador o capital so' entra se sobrarem 2 slots (1 fica p/ a escolha dele).
static bool Private(u8 k){
 switch(k){case BK_QUARTEL:case BK_MURALHA:case BK_PALACIO:case BK_TEMPLO:case BK_ESCOLA:case BK_UNIVERSIDADE:
  case BK_HOSPITAL:case BK_PARQUE:case BK_ESTALEIRO:return false;default:return true;}
}
static int UpScore(const World *w,int i,int s);
void City_Invest(World *w,u8 ci){
 WEco *e=&w->eco[ci];WCountry *C=&w->c[ci];
 if(!C->alive||e->invest<150)return;
 int bi=-1,bk=0,bs=69,bu=-1;
 for(int i=0;i<w->nCity;i++){
  const WCity *c=&w->city[i];
  if(c->owner!=ci||c->bq)continue;
  int fr=0;for(int s=0;s<City_Slots(c->tier);s++)if(!c->bld[s])fr++;
  // a burguesia amplia as proprias fabricas (ADR-42/44): sem slot livre, cresce no lugar
  for(int s=0;s<CITY_SLOTS;s++){
   if(!Private(c->bld[s]))continue;
   int sc=UpScore(w,i,s);if(sc<0)continue;
   sc+=(int)(Hs((u32)i*23u+(u32)s,w->day>>6)%15);
   if(sc>bs&&City_UpCost(w,i,s)<=e->invest){bs=sc;bi=i;bu=s;bk=c->bld[s];}
  }
  if(fr<(C->player?2:1))continue;
  for(u8 k=1;k<BK_COUNT;k++){
   if(!Private(k)||!City_CanBuild(w,i,k))continue;
   int sc=AiScore(w,i,k)+(int)(Hs((u32)i*17u+k,w->day>>6)%15);
   if(sc>bs&&City_Cost(w,i,k)<=e->invest){bs=sc;bi=i;bk=k;bu=-1;}
  }
 }
 if(bi<0)return;
 s32 cost;
 if(bu>=0){cost=City_UpCost(w,bi,bu);QueueUp(w,bi,bu);}
 else{cost=City_Cost(w,bi,(u8)bk);if(!Queue(w,bi,-1,(u8)bk))return;}
 e->invest-=cost;w->nInvest++;
 if(C->player){char nm[40];City_Name(w,bi,nm,40);char t[96];int n=0;const char *a=Str(BLD[bk].nameStr);
  while(*a&&n<50)t[n++]=*a++;
  t[n++]=' ';t[n++]='-';t[n++]=' ';a=nm;
  while(*a&&n<94)t[n++]=*a++;
  t[n]=0;
  World_Msg(w,Str(STR_INVEST_MSG),t);}
}
// ampliar vale o que a obra vale (o mercado ainda pede) menos um tanto por
// nivel: a IA amplia o que da' lucro e constroi o que falta
static int UpScore(const World *w,int i,int s){
 const WCity *c=&w->city[i];
 if(City_UpWhy(w,i,s))return -1;
 // so' amplia com gente sobrando p/ as vagas novas (40% da cidade trabalha)
 int want;WEco_CityJobs(c,&want);
 if(c->pop*2/5<want+WEco_BldJobs(c->bld[s]))return -1;
 int lv=City_Level(c,s),sc=AiScore(w,i,c->bld[s]);
 if(sc<70)return -1;                                   // o mercado precisa pedir de verdade
 return sc*2/(2+lv*2);
}
static void AiBuild(World *w,int i){
 WCity *c=&w->city[i];WCountry *C=&w->c[c->owner];
 int best=-1,bs=0,up=-1;
 for(u8 k=1;k<BK_COUNT;k++){
  if(!City_CanBuild(w,i,k))continue;
  int sc=AiScore(w,i,k)+(int)(Hs((u32)i*31u+k,w->day>>5)%20);   // pitada de acaso (hash, nao RNG)
  if(sc>bs){bs=sc;best=k;}
 }
 for(int s=0;s<CITY_SLOTS;s++){int sc=UpScore(w,i,s);if(sc>bs){bs=sc;up=s;best=-1;}}
 if(up>=0){if(C->gold>=City_UpCost(w,i,up)*2)City_Upgrade(w,i,up);return;}
 if(best<0||C->gold<City_Cost(w,i,(u8)best)*2)return;     // guarda reserva p/ a guerra
 City_Build(w,i,-1,(u8)best);
}
static void AiFound(World *w,u8 ci){
 WCountry *C=&w->c[ci];
 s32 cost=City_FoundCost(w,ci);
 if(C->gold<cost*2||C->cells/(u32)(C->nCity+1)<90)return;
 int bw=C->bx1-C->bx0+1,bh=C->by1-C->by0+1;
 for(int t=0;t<40;t++){
  u32 h=Hs(ci*7919u+t,w->day);
  int x=C->bx0+(int)(h%(u32)bw),y=C->by0+(int)((h>>12)%(u32)bh);
  if(x<w->w&&y<w->h&&City_CanFound(w,ci,x,y)){City_Found(w,ci,x,y);return;}
 }
}
// Urbanizacao (ADR-44): a soma das cidades nao passa de uma fatia da
// populacao do pais que sobe com a era (1800 ~8%, 1900 ~25%, 2000 ~50%) e
// com a alfabetizacao. Sem isso a megalopole crescia sem gente no pais e a
// demanda urbana engolia o mercado. URBP = soma de ontem (1 passada/dia).
static u32 URBP[W_MAXC+1],URBC[W_MAXC+1];
static u32 UrbCap(const World *w,const WCountry *C){
 int y=w->date.y,p=7+(y-1800)/5+C->lit/60;
 if(p>80)p=80;
 return (u32)((u64)C->popK*(u32)p/100u)+20u;
}
HOT_ARM void City_Day(World *w){
 for(u8 ci=1;ci<w->nC;ci++){w->c[ci].cityGold=0;w->c[ci].cityCap=0;w->c[ci].nCity=0;}
 bool newYear=(w->date.m==1&&w->date.d==1);
 for(int ci=0;ci<=W_MAXC;ci++)URBP[ci]=0;
 for(int i=0;i<w->nCity;i++)URBP[w->city[i].owner]+=w->city[i].pop;
 for(u8 ci=1;ci<w->nC;ci++)URBC[ci]=w->c[ci].alive?UrbCap(w,&w->c[ci]):0;
 for(int i=0;i<w->nCity;i++){
  WCity *c=&w->city[i];
  u8 o=c->owner;
  if(!o||!w->c[o].alive)continue;
  WCountry *C=&w->c[o];
  if(c->bq){
   // falta de ferramentas atrasa a obra (1 dia de obra a cada ~2 dias)
   if(c->bqDays&&(int)((w->day*37u+(u32)i)%100u)<WEco_ToolsPct(w,o))c->bqDays--;
   if(!c->bqDays){
    int s=c->bqSlot;
    if(c->bqUp){
     // ampliacao: a obra do slot sobe 1 nivel (se ainda for a mesma)
     if(s<CITY_SLOTS&&c->bld[s]==c->bq){c->blv[s]=(u8)(City_Level(c,s)+1);w->nUpgrades++;}
    }else{
     if(s>=CITY_SLOTS||c->bld[s]){s=-1;for(int k=0;k<CITY_SLOTS;k++)if(!c->bld[k]){s=k;break;}}
     if(s>=0){c->bld[s]=c->bq;c->bpos[s]=c->bqLot;c->blv[s]=1;}
    }
    if(C->player){char nm[40];City_Name(w,i,nm,40);World_Msg(w,Str(c->bqUp?STR_OBRA_AMPLIADA:STR_OBRA_PRONTA),nm);}
    c->bq=0;c->bqUp=0;
   }
  }
  int gold=c->tier,food=0,cap=c->pop/8,prest=0;
  for(int s=0;s<CITY_SLOTS;s++){u8 k=c->bld[s];if(!k)continue;int lv=c->blv[s]?c->blv[s]:1;
   gold+=BLD[k].gold*lv;food+=BLD[k].food*lv;cap+=BLD[k].cap*lv;prest+=BLD[k].prestige*(lv+1)/2;}
  C->cityGold+=gold;C->cityCap+=cap;C->nCity++;   // ouro entra na semana (WEco_Week)
  if(newYear)C->prestige+=prest;
  int pc=City_PopCap(c);
  if(c->food<50&&c->pop>3&&((w->day+(u32)i)%60u)==0)c->pop=(u16)(c->pop-(c->pop/100+1)); // fome: a cidade encolhe
  bool full=URBP[o]>=URBC[o];
  // cidade demais p/ o pais: a maior perde gente devagar (exodo p/ o campo)
  if(full&&c->pop>50&&URBP[o]>URBC[o]+URBC[o]/10&&((w->day+(u32)i)%90u)==0)c->pop=(u16)(c->pop-c->pop/100);
  if(c->pop<pc&&!full){
   // crescimento realista: ~1,5%/ano sem obras, ate ~4%/ano com fazenda,
   // porto e ferrovia; desacelera perto do teto (logistica) e depende da
   // comida (mercado local + nacional, wecon.c) e do nivel de vida.
   int g=4+food*3+c->tier*2+(C->happy>60?2:0);
   g=g*c->food/100*(50+w->eco[o].life/2)/100;
   if(c->pop*4>pc*3)g/=2;
   c->grow=(u16)(c->grow+g);
   if(c->grow>=1000){c->grow-=1000;int add=c->pop/200;if(add<1)add=1;c->pop=(u16)(c->pop+add);if(c->pop>pc)c->pop=(u16)pc;}
  }
  if(c->tier<CT_COUNT-1&&TierReq(c,(u8)(c->tier+1))){
   c->tier++;
   if(C->player){char nm[40];City_Name(w,i,nm,40);World_Msg(w,Str(STR_CIDADE_CRESCE),nm);}
  }
  if(!C->player&&!c->bq&&((i+(int)w->day)&31)==0)AiBuild(w,i);
 }
 for(u8 ci=1;ci<w->nC;ci++)
  if(w->c[ci].alive&&!w->c[ci].player&&((ci*37u+w->day)%730u)==0)AiFound(w,ci);
}
