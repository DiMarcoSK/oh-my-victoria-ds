#include "nations.h"
#include "world.h"
// hflags.c — bandeiras historicas e por regime (ADR-34, DESIGN 6.5.1).
// Uma linha = (pais, anos [y0,y1), ideologia ou QQ, bandeira). Regra de busca
// (HFlag_Find): primeiro a linha da IDEOLOGIA atual dentro dos anos (bandeira
// propria do regime: Bourbon, Weimar, URSS...); senao a linha QQ do periodo
// (base historica, a ideologia ainda a transforma); senao o catalogo.
// Desenho com o mesmo vocabulario do catalogo (padrao + 3 cores + emblema).
// Fonte: vexilologia historica (Flags of the World); simplificado ao pixel.
#define FL(p,a,b,c,e,x) {FP_##p,FC_##a,FC_##b,FC_##c,FE_##e,FC_##x}
enum { QQ=255 };
typedef struct { const char *tag; u16 y0,y1; u8 ideo; FlagDef f; } HFlag;
static const HFlag HF[]={
 // Franca: Restauracao Bourbon (branca com lirios), Segundo Imperio (aguia), Vichy
 {"FRA",0,9999,ID_TRADICIONAL,FL(LISA,WHITE,WHITE,WHITE,COROA,GOLD)},
 {"FRA",1804,1815,ID_AUTORITARIO,FL(TRI_V,BLUE,WHITE,RED,AGUIA,GOLD)},
 {"FRA",1852,1871,ID_AUTORITARIO,FL(TRI_V,BLUE,WHITE,RED,AGUIA,GOLD)},
 {"FRA",1940,1945,ID_AUTORITARIO,FL(TRI_V,BLUE,WHITE,RED,ESCUDO,GOLD)},
 // Espanha: Segunda Republica (roxo), franquismo (aguia), democracia (escudo)
 {"ESP",1873,1978,ID_LIBERAL,FL(TRI_H,RED,YELLOW,PURPLE,NONE,WHITE)},
 {"ESP",1936,1981,ID_AUTORITARIO,FL(TRI_H,RED,YELLOW,RED,AGUIA,BLACK)},
 {"ESP",1981,9999,QQ,FL(TRI_H,RED,YELLOW,RED,ESCUDO,RED)},
 // Portugal: monarquia constitucional azul e branca, Republica verde e vermelha
 {"POR",1830,1911,QQ,FL(BI_V,BLUE,WHITE,WHITE,COROA,GOLD)},
 {"POR",1911,9999,QQ,FL(BI_V,DKGREEN,RED,RED,GLOBO,GOLD)},
 {"POR",0,9999,ID_TRADICIONAL,FL(BI_V,BLUE,WHITE,WHITE,COROA,GOLD)},
 // Italia e Sardenha: cruz de Saboia; Republica de 1946 sem escudo
 {"SAR",1848,1862,QQ,FL(TRI_V,GREEN,WHITE,RED,ESCUDO,RED)},
 {"ITA",1946,9999,QQ,FL(TRI_V,GREEN,WHITE,RED,NONE,WHITE)},
 {"ITA",0,9999,ID_LIBERAL,FL(TRI_V,GREEN,WHITE,RED,NONE,WHITE)},
 // Alemanha: Imperio (catalogo), Weimar, Reich, Republica Federal, RDA
 {"GER",1919,1933,QQ,FL(TRI_H,BLACK,RED,GOLD,NONE,WHITE)},
 {"GER",1933,1946,ID_AUTORITARIO,FL(TRI_H,BLACK,WHITE,RED,AGUIA,BLACK)},
 {"GER",1949,9999,QQ,FL(TRI_H,BLACK,RED,GOLD,NONE,WHITE)},
 {"GER",0,9999,ID_COMUNISTA,FL(TRI_H,BLACK,RED,GOLD,FOICE,GOLD)},
 {"GER",0,9999,ID_LIBERAL,FL(TRI_H,BLACK,RED,GOLD,NONE,WHITE)},
 // Austria: Estado corporativo de 1934
 {"AUT",1934,1938,ID_AUTORITARIO,FL(TRI_H,RED,WHITE,RED,AGUIA,BLACK)},
 // Russia: tricolor imperial negra-ouro-branca (1858-1883); URSS com foice e martelo
 {"RUS",1858,1883,QQ,FL(TRI_H,BLACK,YELLOW,WHITE,NONE,WHITE)},
 {"RUS",0,9999,ID_COMUNISTA,FL(LISA,RED,RED,RED,FOICE,GOLD)},
 {"URS",0,9999,QQ,FL(LISA,RED,RED,RED,FOICE,GOLD)},
 {"URS",0,9999,ID_LIBERAL,FL(TRI_H,WHITE,BLUE,RED,NONE,WHITE)},
 // China: Republica (catalogo), Republica Popular (5 estrelas)
 {"CHN",0,9999,ID_COMUNISTA,FL(LISA,RED,RED,RED,ESTRELAS,YELLOW)},
 {"QNG",0,9999,ID_COMUNISTA,FL(LISA,RED,RED,RED,ESTRELAS,YELLOW)},
 {"PRC",0,9999,QQ,FL(LISA,RED,RED,RED,ESTRELAS,YELLOW)},
 // Japao militarista: sol nascente
 {"JAP",1889,1946,ID_AUTORITARIO,FL(LISA,WHITE,WHITE,WHITE,SOL,RED)},
 // Persia/Ira: leao e sol no tricolor (1907), Republica Islamica (1980)
 {"QAJ",1907,1980,QQ,FL(TRI_H,GREEN,WHITE,RED,LEAO,GOLD)},
 {"QAJ",1979,9999,ID_TEOCRATICO,FL(TRI_H,GREEN,WHITE,RED,CRESCENTE,RED)},
 // Egito: reino (verde e crescente), republica (aguia de Saladino)
 {"EGY",1922,1953,QQ,FL(LISA,GREEN,GREEN,GREEN,CRESCENTE,WHITE)},
 {"EGY",1953,9999,QQ,FL(TRI_H,RED,WHITE,BLACK,AGUIA,GOLD)},
 // Grecia: listras com cruz no cantao (1978)
 {"GRE",1978,9999,QQ,FL(LISTRAS,BLUE,WHITE,BLUE,CRUZ,WHITE)},
 // Siam/Tailandia: elefante branco, depois o Trairanga (1917)
 {"SIA",1855,1917,QQ,FL(LISA,RED,RED,RED,LEAO,WHITE)},
 {"SIA",1917,9999,QQ,FL(LISTRAS5,RED,WHITE,NAVY,NONE,WHITE)},
 // Etiopia: leao de Juda (1897-1974), estrela (1996)
 {"ETH",1897,1975,QQ,FL(TRI_H,GREEN,YELLOW,RED,LEAO,RED)},
 {"ETH",1996,9999,QQ,FL(TRI_H,GREEN,YELLOW,RED,ESTRELA,BLUE)},
 // Marrocos: selo de Salomao verde (1915)
 {"MOR",1915,9999,QQ,FL(LISA,RED,RED,RED,ESTRELA,DKGREEN)},
 // Canada: Red Ensign ate' 1965, depois a folha de bordo
 {"CAN",1868,1965,QQ,FL(ENSIGN,RED,RED,RED,ESCUDO,GOLD)},
 {"CAN",1965,9999,QQ,FL(TRI_V,RED,WHITE,RED,FOLHA,RED)},
 // Raj Britanico (Estrela da India) e Nova Gales do Sul (Blue Ensign)
 {"RAJ",0,9999,QQ,FL(ENSIGN,NAVY,NAVY,NAVY,ESTRELA,GOLD)},
 {"NSW",0,9999,QQ,FL(ENSIGN,NAVY,NAVY,NAVY,ESTRELA,WHITE)},
 // Israel: duas faixas azuis e a estrela de Davi
 {"ISR",0,9999,QQ,FL(LISTRAS5,WHITE,BLUE,WHITE,ESTRELA6,BLUE)},
 // Cuba (1902), Filipinas (1898), Tchecoslovaquia (1920)
 {"CUB",1902,9999,QQ,FL(TRIANG,NAVY,WHITE,RED,ESTRELA,WHITE)},
 {"PHI",1898,9999,QQ,FL(TRIANG2,BLUE,RED,WHITE,SOL,GOLD)},
 {"CZE",0,9999,QQ,FL(TRIANG2,WHITE,RED,BLUE,NONE,WHITE)},
 // Brasil: Imperio (1822, coroa no losango), Republica (1889, globo azul)
 {"BRA",1822,1889,QQ,FL(LOSANGO,GREEN,YELLOW,GREEN,COROA,GOLD)},
 {"BRA",1889,9999,QQ,FL(LOSANGO,GREEN,YELLOW,BLUE,GLOBO,WHITE)},
 {"BRA",1889,9999,ID_TRADICIONAL,FL(LOSANGO,GREEN,YELLOW,GREEN,COROA,GOLD)},
 // Vietna: estrela amarela no vermelho
 {"VIE",1945,9999,ID_COMUNISTA,FL(LISA,RED,RED,RED,ESTRELA,YELLOW)},
 {"VIE",1945,9999,QQ,FL(LISA,RED,RED,RED,ESTRELA,YELLOW)},
 // Afeganistao: reino tricolor (1928-1973)
 {"AFG",1928,1974,QQ,FL(TRI_V,BLACK,RED,GREEN,ESCUDO,WHITE)},
 // Hungria/Polonia/Romenia/Iugoslavia comunistas: brasao socialista
 {"HUN",0,9999,ID_COMUNISTA,FL(TRI_H,RED,WHITE,GREEN,ESCUDO,RED)},
 {"ROM",0,9999,ID_COMUNISTA,FL(TRI_V,BLUE,YELLOW,RED,ESCUDO,DKGREEN)},
 {"YUG",0,9999,ID_COMUNISTA,FL(TRI_H,BLUE,WHITE,RED,ESTRELA,RED)},
 // Estados Unidos: os Confederados
 {"CSA",0,9999,QQ,FL(SALTIRE,RED,NAVY,NAVY,NONE,WHITE)},
};
int HFlag_Count(void){return (int)(sizeof HF/sizeof HF[0]);}
static bool Tag(const char *a,const char *b){return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2];}
int HFlag_Find(const char *tag,u16 year,u8 ideo,FlagDef *out){
 int base=-1;
 for(int i=0;i<HFlag_Count();i++){
  const HFlag *h=&HF[i];
  if(!Tag(h->tag,tag)||year<h->y0||year>=h->y1)continue;
  if(h->ideo==ideo){*out=h->f;return 2;}
  if(h->ideo==QQ&&base<0)base=i;
 }
 if(base>=0){*out=HF[base].f;return 1;}
 return 0;
}
