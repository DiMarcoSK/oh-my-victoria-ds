#include <stddef.h>
#include "nations.h"
// nations.c — catalogo semente (DESIGN.md sec 6.7). Traits e textos sao
// proposta de design (balancear). Capitais em graus inteiros (aprox.).
// Bandeiras: periodo de 1800 quando havia; banner=1 = estandarte de jogo.
// Ao adicionar: siga o checklist do DESIGN.md sec 8.1 (e 8.2 p/ povos).

#define FL(p,a,b,c,e,x) {FP_##p,FC_##a,FC_##b,FC_##c,FE_##e,FC_##x}
#define N_EST NK_ESTADO
#define N_SUD NK_SUDITO
#define N_POV NK_POVO
#define N_AUT NK_AUTONOMO
#define G_MON NG_MONARQUIA
#define G_REP NG_REPUBLICA
#define G_TEO NG_TEOCRACIA
#define G_CON NG_CONSELHO
#define G_CIA NG_COMPANHIA
#define K_EU CONT_EUROPA
#define K_AM CONT_AMERICAS
#define K_AF CONT_AFRICA
#define K_AS CONT_ASIA
#define K_OC CONT_OCEANIA

const NationDef NATIONS[]={
// ===== EUROPA OCIDENTAL =====
{"GBR","Grã-Bretanha",NULL,K_EU,N_EST,G_MON,SC_EUR_OCID,26,4,4,FL(UNION,NAVY,WHITE,RED,NONE,WHITE),0,{TR_INDUSTRIAL,TR_MARES,TR_COLONIAL,0},51,0,5,0,NULL,"Grandes recursos, comércio forte e uma marinha incomparável."},
{"IRL","Irlanda",NULL,K_EU,N_SUD,G_MON,SC_EUR_OCID,6,20,8,FL(LISA,GREEN,GOLD,GOLD,COROA,GOLD),0,{TR_COESA,TR_FE,0,0},53,-6,4,0,"GBR","Ilha verde em união com a coroa britânica; o Ato de União de 1801 a absorve."},
{"FRA","França",NULL,K_EU,N_EST,G_REP,SC_EUR_OCID,6,10,26,FL(TRI_V,BLUE,WHITE,RED,NONE,WHITE),0,{TR_LEVEE,TR_REVOLUCAO,TR_CULTURA,0},49,2,9,0,NULL,"A Revolução em armas: exércitos de massa e a cultura mais brilhante da Europa."},
{"ESP","Espanha",NULL,K_EU,N_EST,G_MON,SC_EUR_OCID,28,22,4,FL(TRI_H,RED,YELLOW,RED,NONE,RED),0,{TR_COLONIAL,TR_ABSOLUTISMO,TR_GUERRILHA,0},40,-4,7,0,NULL,"Velho império de ultramar; quando invadida, cada aldeia vira trincheira."},
{"POR","Portugal",NULL,K_EU,N_EST,G_MON,SC_EUR_OCID,4,16,8,FL(LISA,WHITE,WHITE,WHITE,COROA,GOLD),0,{TR_COLONIAL,TR_ENTREPOSTO,0,0},39,-9,4,0,NULL,"Pequeno no mapa, imenso no mar: rotas e colônias em quatro continentes."},
{"NED","República Batava",NULL,K_EU,N_EST,G_REP,SC_EUR_OCID,30,16,4,FL(TRI_H,RED,WHITE,BLUE,NONE,WHITE),0,{TR_ENTREPOSTO,TR_COLONIAL,0,0},52,5,3,0,NULL,"Mercadores, bancos e as Índias Orientais. Pouca terra, muito ouro."},
{"HEL","República Helvética","Helvetia",K_EU,N_EST,G_CON,SC_EUR_CENT,22,6,10,FL(TRI_H,GREEN,RED,YELLOW,NONE,WHITE),0,{TR_MONTANHAS,TR_CONFEDERACAO,0,0},47,7,3,0,NULL,"Cantões unidos entre os Alpes: difíceis de conquistar, fáceis de respeitar."},
// ===== EUROPA CENTRAL E DO NORTE =====
{"PRU","Prússia","Preußen",K_EU,N_EST,G_MON,SC_EUR_CENT,8,10,14,FL(LISA,WHITE,WHITE,WHITE,AGUIA,BLACK),0,{TR_EXERCITO,TR_REFORMADOR,0,0},52,13,6,0,NULL,"Um exército com um Estado. Disciplina, reformas e o sonho de unir a Alemanha."},
{"HAB","Monarquia de Habsburgo","Österreich",K_EU,N_EST,G_MON,SC_EUR_CENT,28,28,25,FL(LISA,GOLD,GOLD,GOLD,AGUIA2,BLACK),0,{TR_MULTIETNICO,TR_CULTURA,TR_CELEIRO,0},48,16,7,0,NULL,"Um império de muitos povos unido pela coroa, com Viena no coração."},
{"BAV","Baviera","Bayern",K_EU,N_EST,G_MON,SC_EUR_CENT,12,20,30,FL(BI_H,WHITE,SKY,SKY,NONE,WHITE),0,{TR_CONFEDERACAO,TR_CELEIRO,0,0},48,12,3,0,NULL,"O maior dos Estados do sul alemão, entre a França e a Áustria."},
{"SAX","Saxônia","Sachsen",K_EU,N_EST,G_MON,SC_EUR_CENT,14,24,10,FL(BI_H,WHITE,GREEN,GREEN,NONE,WHITE),0,{TR_CONFEDERACAO,TR_INDUSTRIAL,0,0},51,14,3,0,NULL,"Minas, porcelana e as primeiras fábricas da Alemanha."},
{"WUR","Württemberg",NULL,K_EU,N_EST,G_MON,SC_EUR_CENT,24,14,8,FL(BI_H,BLACK,RED,RED,NONE,WHITE),0,{TR_CONFEDERACAO,TR_COESA,0,0},49,9,2,0,NULL,"Ducado da Suábia que Napoleão fará reino."},
{"HAN","Hanôver","Hannover",K_EU,N_EST,G_MON,SC_EUR_CENT,24,24,10,FL(LISA,RED,RED,RED,LEAO,WHITE),0,{TR_CONFEDERACAO,TR_ENTREPOSTO,0,0},52,10,3,0,NULL,"Terra natal da dinastia britânica, em união pessoal com Londres."},
{"SWE","Suécia","Sverige",K_EU,N_EST,G_MON,SC_EUR_CENT,8,16,28,FL(NORDICA,BLUE,YELLOW,YELLOW,NONE,WHITE),0,{TR_EXERCITO,TR_INVERNO,0,0},59,18,8,0,NULL,"Antiga potência do Báltico, ainda senhora da Finlândia."},
{"DAN","Dinamarca-Noruega","Danmark-Norge",K_EU,N_EST,G_MON,SC_EUR_CENT,24,8,12,FL(NORDICA,RED,WHITE,WHITE,NONE,WHITE),0,{TR_MARES,TR_ENTREPOSTO,0,0},56,13,5,0,NULL,"Reinos gêmeos do mar do Norte, da Groenlândia ao Báltico."},
// ===== EUROPA ORIENTAL E MEDITERRANEO =====
{"RUS","Rússia","Rossiya",K_EU,N_EST,G_MON,SC_EUR_LESTE,4,14,6,FL(TRI_H,WHITE,BLUE,RED,NONE,WHITE),0,{TR_INVERNO,TR_ABSOLUTISMO,TR_FRONTEIRA,TR_MULTIETNICO},60,30,9,0,NULL,"Do Báltico ao Pacífico. Quem a invade descobre o inverno."},
{"GEO","Kartli-Kakheti","Sakartvelo",K_EU,N_EST,G_MON,SC_EUR_LESTE,26,24,24,FL(CRUZ,WHITE,RED,RED,NONE,WHITE),0,{TR_MONTANHAS,TR_FE,0,0},42,45,3,0,NULL,"Reino cristão do Cáucaso, entre a Pérsia e a Rússia."},
{"OTT","Império Otomano","Devlet-i Aliyye",K_EU,N_EST,G_MON,SC_MEDITERRANEO,16,20,10,FL(LISA,RED,RED,RED,CRESCENTE,WHITE),0,{TR_MULTIETNICO,TR_CRISE,TR_FE,TR_REFORMADOR},41,29,8,0,NULL,"Senhor dos Estreitos e de três continentes, em busca de reforma."},
{"SAR","Reino da Sardenha","Piemonte",K_EU,N_EST,G_MON,SC_MEDITERRANEO,20,22,26,FL(CRUZ,RED,WHITE,WHITE,NONE,WHITE),0,{TR_REFORMADOR,TR_EXERCITO,0,0},45,8,3,0,NULL,"Piemonte e Sardenha: o motor do Risorgimento italiano."},
{"NAP","Reino de Nápoles","Napoli",K_EU,N_EST,G_MON,SC_MEDITERRANEO,14,26,22,FL(LISA,WHITE,WHITE,WHITE,COROA,GOLD),0,{TR_ABSOLUTISMO,TR_CELEIRO,TR_CULTURA,0},41,14,4,0,NULL,"O sul da Itália, rico em trigo e em arte, governado com mão firme."},
{"PAP","Estados Papais","Status Pontificius",K_EU,N_EST,G_TEO,SC_MEDITERRANEO,30,30,18,FL(BI_V,YELLOW,WHITE,WHITE,NONE,WHITE),0,{TR_FE,TR_CULTURA,TR_GLORIA,0},42,12,3,0,NULL,"Roma e o poder temporal do papa: prestígio enorme, exército pequeno."},
{"MNE","Montenegro","Crna Gora",K_EU,N_EST,G_TEO,SC_MEDITERRANEO,12,8,6,FL(CRUZ,RED,WHITE,WHITE,NONE,WHITE),0,{TR_MONTANHAS,TR_GUERRILHA,0,0},42,19,2,0,NULL,"Príncipes-bispos nas montanhas negras; nunca de fato dominados."},
{"ALG","Regência de Argel","Al-Jaza'ir",K_AF,N_SUD,G_MON,SC_AFR_NORTE,20,16,8,FL(TRI_H,RED,YELLOW,GREEN,NONE,WHITE),0,{TR_CORSARIOS,0,0,0},37,3,6,0,"OTT","Porto corsário do Mediterrâneo, vassalo nominal de Istambul."},
{"TUN","Regência de Túnis","Tunis",K_AF,N_SUD,G_MON,SC_AFR_NORTE,24,20,14,FL(LISA,RED,RED,RED,CRESCENTE,WHITE),0,{TR_CORSARIOS,0,0,0},37,10,3,0,"OTT","Cidade de comércio e corsários na antiga Cartago."},
{"TRP","Regência de Trípoli","Tarabulus",K_AF,N_SUD,G_MON,SC_AFR_NORTE,18,22,12,FL(LISA,GREEN,GREEN,GREEN,CRESCENTE,WHITE),0,{TR_CORSARIOS,0,0,0},33,13,6,0,"OTT","Os paxás Karamanli cobram tributo dos navios que passam."},
{"EGY","Egito","Misr",K_AF,N_SUD,G_MON,SC_AFR_NORTE,26,24,14,FL(LISA,RED,RED,RED,CRESCENTE,WHITE),0,{TR_REFORMADOR,TR_CELEIRO,0,0},30,31,7,0,"OTT","O Nilo alimenta um reformador ambicioso dentro do Império Otomano."},
// ===== AMERICAS - ESTADOS E COLONIAS =====
{"USA","Estados Unidos",NULL,K_AM,N_EST,G_REP,SC_AMN_LESTE,6,8,20,FL(LISTRAS,RED,WHITE,NAVY,ESTRELA,WHITE),0,{TR_JOVEM,TR_FRONTEIRA,TR_REVOLUCAO,0},39,-77,8,0,NULL,"República jovem e faminta de terra, com o oeste inteiro à frente."},
{"HAI","Saint-Domingue","Ayiti",K_AM,N_SUD,G_REP,SC_CARIBE,28,10,14,FL(BI_V,BLUE,RED,RED,NONE,WHITE),0,{TR_GUERRILHA,TR_TERRA,TR_REVOLUCAO,0},19,-72,3,0,"FRA","Uma revolução de escravizados que vai fundar um país livre."},
{"CUB","Cuba",NULL,K_AM,N_SUD,G_MON,SC_CARIBE,26,20,8,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_CELEIRO,0,0,0},23,-82,3,0,"ESP","A pérola do Caribe espanhol: açúcar, tabaco e portos."},
{"NSP","Nova Espanha",NULL,K_AM,N_SUD,G_MON,SC_AMS,26,22,10,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_MINERAL,0,0,0},19,-99,10,0,"ESP","Vice-reino da prata, do Texas à Guatemala."},
{"NGR","Nova Granada",NULL,K_AM,N_SUD,G_MON,SC_AMS,28,24,12,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_MINERAL,0,0,0},5,-74,9,0,"ESP","Vice-reino dos Andes do norte, de Caracas a Quito."},
{"VPE","Vice-reino do Peru",NULL,K_AM,N_SUD,G_MON,SC_AMS,24,20,6,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_MINERAL,0,0,0},-12,-77,9,0,"ESP","Lima, a cidade dos reis, sobre a herança dos Incas."},
{"RDP","Río de la Plata",NULL,K_AM,N_SUD,G_MON,SC_AMS,22,26,28,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_CELEIRO,0,0,0},-35,-58,9,0,"ESP","Vice-reino dos pampas e da prata de Potosí."},
{"BRA","Brasil",NULL,K_AM,N_SUD,G_MON,SC_AMS,8,22,8,FL(LISA,WHITE,WHITE,WHITE,SOL,GOLD),0,{TR_MINERAL,TR_CELEIRO,0,0},-23,-43,10,0,"POR","Ouro de Minas, açúcar e um território continental."},
{"BNA","América do Norte Britânica",NULL,K_AM,N_SUD,G_MON,SC_AMN_LESTE,22,6,6,FL(CANTAO,RED,NAVY,WHITE,CRUZ,WHITE),0,{TR_FRONTEIRA,0,0,0},47,-71,8,0,"GBR","Quebec, as Grandes Planícies do norte e o comércio de peles."},
{"RAM","América Russa",NULL,K_AM,N_SUD,G_CIA,SC_ARTICO,8,18,10,FL(TRI_H,WHITE,BLUE,RED,AGUIA,BLACK),0,{TR_ENTREPOSTO,0,0,0},57,-135,4,0,"RUS","Postos da Companhia Russo-Americana no Alasca."},
// ===== AMERICAS - POVOS =====
{"HDS","Haudenosaunee","Haudenosaunee",K_AM,N_POV,G_CON,SC_AMN_LESTE,16,10,22,FL(LISA,PURPLE,WHITE,WHITE,NONE,WHITE),1,{TR_CONFEDERACAO,TR_TERRA,0,0},43,-76,3,0,NULL,"A Confederação das Seis Nações e sua Grande Lei da Paz."},
{"TSA","Tsalagi","Tsalagi",K_AM,N_POV,G_CON,SC_AMN_LESTE,20,14,6,FL(BORDA,WHITE,ORANGE,ORANGE,ESTRELA,WHITE),1,{TR_REFORMADOR,TR_COESA,TR_TERRA,0},35,-84,3,0,NULL,"Os Cherokee: escrita própria, constituição e luta pela terra."},
{"MUS","Muscogee","Mvskoke",K_AM,N_POV,G_CON,SC_AMN_LESTE,24,18,10,FL(BI_H,RED,WHITE,WHITE,NONE,WHITE),1,{TR_CONFEDERACAO,TR_TERRA,0,0},32,-85,4,0,NULL,"Os Creek: cidades confederadas do sudeste."},
{"SEM","Seminole",NULL,K_AM,N_POV,G_CON,SC_AMN_LESTE,10,20,14,FL(TRI_H,WHITE,YELLOW,RED,NONE,WHITE),1,{TR_TERRA,TR_GUERRILHA,0,0},29,-82,3,0,NULL,"Os pântanos da Flórida como fortaleza. Nunca se renderam."},
{"SHA","Shawnee",NULL,K_AM,N_POV,G_CON,SC_AMN_LESTE,18,12,10,FL(DIAG,RED,BLACK,BLACK,NONE,WHITE),1,{TR_CONFEDERACAO,TR_TERRA,0,0},39,-83,3,0,NULL,"O povo de Tecumseh, que sonhou unir todos os povos do oeste."},
{"ANI","Anishinaabe","Anishinaabeg",K_AM,N_POV,G_CON,SC_AMN_LESTE,10,14,22,FL(LISA,BLUE,WHITE,WHITE,SOL,WHITE),1,{TR_CONFEDERACAO,TR_ENTREPOSTO,0,0},46,-85,5,0,NULL,"O Conselho dos Três Fogos, senhor dos Grandes Lagos."},
{"CRE","Nēhiyawak","Nēhiyawak",K_AM,N_POV,G_CON,SC_AMN_PLANICIES,14,20,24,FL(BI_H,SKY,GREEN,GREEN,NONE,WHITE),1,{TR_TERRA,TR_ENTREPOSTO,0,0},54,-98,7,0,NULL,"Os Cree: florestas do norte, rios e rotas de peles."},
{"DEN","Dene",NULL,K_AM,N_POV,G_CON,SC_ARTICO,18,22,24,FL(LISA,SKY,SKY,SKY,ESTRELA,WHITE),1,{TR_TERRA,TR_MOVIMENTO,0,0},62,-115,7,0,NULL,"Povos da taiga e dos grandes lagos do noroeste."},
{"LAK","Očhéthi Šakówiŋ","Očhéthi Šakówiŋ",K_AM,N_POV,G_CON,SC_AMN_PLANICIES,20,8,6,FL(LISA,RED,RED,RED,SOL,WHITE),1,{TR_CAVALEIROS,TR_MOVIMENTO,0,0},44,-102,6,0,NULL,"Lakota, Dakota e Nakota: os Sete Fogos do Conselho das Planícies."},
{"COM","Numunuu","Nʉmʉnʉʉ",K_AM,N_POV,G_CON,SC_AMN_PLANICIES,26,16,10,FL(BI_H,BLUE,ORANGE,ORANGE,NONE,WHITE),1,{TR_CAVALEIROS,TR_MOVIMENTO,0,0},34,-100,6,0,NULL,"Os Comanche: senhores da Comanchería e do cavalo."},
{"NII","Niitsitapi",NULL,K_AM,N_POV,G_CON,SC_AMN_PLANICIES,12,12,16,FL(BI_H,BLACK,YELLOW,YELLOW,NONE,WHITE),1,{TR_CAVALEIROS,TR_CONFEDERACAO,0,0},49,-112,5,0,NULL,"A Confederação Blackfoot das planícies do norte."},
{"SHO","Newe","Newe",K_AM,N_POV,G_CON,SC_AMN_PLANICIES,22,20,12,FL(TRI_H,GREEN,WHITE,BLUE,NONE,WHITE),1,{TR_TERRA,TR_MOVIMENTO,0,0},42,-113,6,0,NULL,"Os Shoshone, entre as Rochosas e a Grande Bacia."},
{"DIN","Diné",NULL,K_AM,N_POV,G_CON,SC_AMN_PLANICIES,24,12,8,FL(LISA,ORANGE,ORANGE,ORANGE,SOL,YELLOW),1,{TR_TERRA,TR_COESA,0,0},36,-109,4,0,NULL,"Os Navajo, entre as quatro montanhas sagradas do deserto."},
{"NDE","Ndé",NULL,K_AM,N_POV,G_CON,SC_AMN_PLANICIES,22,18,12,FL(TRI_V,BROWN,YELLOW,BROWN,NONE,WHITE),1,{TR_GUERRILHA,TR_TERRA,0,0},33,-108,4,0,NULL,"Os Apache: os cânions e as serras são aliados."},
{"INU","Inuit",NULL,K_AM,N_AUT,G_CON,SC_ARTICO,24,28,30,FL(BI_H,WHITE,BLUE,BLUE,DISCO,RED),1,{TR_TERRA,TR_COESA,0,0},65,-90,9,0,NULL,"Os povos do Ártico, onde nenhum exército sobrevive sem eles."},
{"HAW","Havaí","Hawaiʻi",K_OC,N_EST,G_MON,SC_OCEANIA,20,10,24,FL(LISTRAS,WHITE,RED,BLUE,CRUZ,WHITE),0,{TR_MARES,TR_REFORMADOR,0,0},20,-156,2,0,NULL,"Kamehameha une as ilhas. Navegadores que cruzaram o Pacífico."},
{"MAP","Mapuche","Mapuche",K_AM,N_POV,G_CON,SC_AMS,8,14,26,FL(TRI_H,BLUE,WHITE,GREEN,NONE,WHITE),1,{TR_TERRA,TR_GUERRILHA,0,0},-38,-72,4,0,NULL,"Wallmapu: a fronteira que a Espanha nunca conseguiu cruzar."},
{"AON","Aónikenk",NULL,K_AM,N_POV,G_CON,SC_AMS,20,22,18,FL(BI_H,SKY,WHITE,WHITE,NONE,WHITE),1,{TR_MOVIMENTO,TR_CAVALEIROS,0,0},-46,-69,6,0,NULL,"Os Tehuelche: senhores dos ventos da Patagônia."},
{"WAY","Wayuu",NULL,K_AM,N_POV,G_CON,SC_CARIBE,26,12,18,FL(BORDA,RED,YELLOW,YELLOW,NONE,WHITE),1,{TR_TERRA,TR_ENTREPOSTO,0,0},12,-72,2,0,NULL,"La Guajira: comerciantes do deserto que a Espanha não submeteu."},
{"QOM","Qom e Wichí",NULL,K_AM,N_POV,G_CON,SC_AMS,18,16,8,FL(TRI_H,GREEN,BROWN,GREEN,NONE,WHITE),1,{TR_TERRA,TR_MOVIMENTO,0,0},-24,-61,4,0,NULL,"Povos do Gran Chaco, terra que nenhum vice-rei controlou."},
{"MDK","Munduruku",NULL,K_AM,N_AUT,G_CON,SC_AMS,12,22,10,FL(LISA,DKGREEN,DKGREEN,DKGREEN,SOL,YELLOW),1,{TR_TERRA,TR_COESA,0,0},-6,-57,6,0,NULL,"Povo guerreiro do Tapajós, no coração da Amazônia."},
{"KAY","Mẽbêngôkre","Mẽbêngôkre",K_AM,N_AUT,G_CON,SC_AMS,16,24,8,FL(BI_H,BLACK,RED,RED,NONE,WHITE),1,{TR_TERRA,TR_COESA,0,0},-8,-52,6,0,NULL,"Os Kayapó, entre o Xingu e o Araguaia."},
{"YAN","Yanomami",NULL,K_AM,N_AUT,G_CON,SC_AMS,10,18,6,FL(LISA,GREEN,GREEN,GREEN,DISCO,RED),1,{TR_TERRA,TR_COESA,0,0},2,-64,5,0,NULL,"Povo das serras entre o Orinoco e o rio Negro."},
{"ASK","Asháninka",NULL,K_AM,N_AUT,G_CON,SC_AMS,18,10,16,FL(TRI_V,RED,BROWN,RED,NONE,WHITE),1,{TR_TERRA,TR_GUERRILHA,0,0},-11,-74,4,0,NULL,"A selva central do Peru, livre desde a revolta de Juan Santos Atahualpa."},
// ===== AFRICA =====
{"ASH","Império Asante","Asante",K_AF,N_EST,G_MON,SC_AFR_OCID,28,24,4,FL(TRI_H,GOLD,BLACK,GREEN,NONE,WHITE),1,{TR_MINERAL,TR_EXERCITO,0,0},7,-2,4,0,NULL,"Ouro, estradas e um exército temido, unidos pelo Banco Dourado."},
{"DAH","Daomé","Danxome",K_AF,N_EST,G_MON,SC_AFR_OCID,20,6,10,FL(LISA,WHITE,WHITE,WHITE,ESCUDO,RED),1,{TR_DISCIPLINA,TR_EXERCITO,0,0},7,2,2,0,NULL,"Reino militar de Abomé, com o regimento feminino das Agojie."},
{"OYO","Império de Oió","Ọ̀yọ́",K_AF,N_EST,G_MON,SC_AFR_OCID,14,18,6,FL(TRI_V,GREEN,WHITE,GREEN,NONE,WHITE),1,{TR_CAVALEIROS,TR_CRISE,0,0},9,4,5,0,NULL,"Cavalaria iorubá de um império que começa a rachar."},
{"HSA","Gobir",NULL,K_AF,N_EST,G_MON,SC_AFR_OCID,18,14,8,FL(LISA,GREEN,GREEN,GREEN,ESTRELA,WHITE),1,{TR_ENTREPOSTO,TR_CAVALEIROS,0,0},13,6,3,0,NULL,"A mais guerreira das cidades-Estado hauçá."},
{"KNO","Kano",NULL,K_AF,N_EST,G_MON,SC_AFR_OCID,22,18,10,FL(BI_H,GREEN,WHITE,WHITE,NONE,WHITE),1,{TR_ENTREPOSTO,TR_CAVALEIROS,0,0},12,8,2,0,NULL,"Tecidos tingidos de índigo vendidos em todo o Sahel."},
{"KAT","Katsina",NULL,K_AF,N_EST,G_MON,SC_AFR_OCID,16,12,6,FL(BI_H,WHITE,GREEN,GREEN,NONE,WHITE),1,{TR_ENTREPOSTO,TR_CAVALEIROS,0,0},13,8,2,0,NULL,"Cidade de mercadores e eruditos do comércio transaariano."},
{"BOR","Bornu",NULL,K_AF,N_EST,G_MON,SC_AFR_OCID,12,18,14,FL(LISA,BLUE,BLUE,BLUE,CRESCENTE,WHITE),1,{TR_FE,TR_CAVALEIROS,0,0},13,12,5,0,NULL,"Mil anos de reino em torno do lago Chade."},
{"SEG","Império Bambara","Segu",K_AF,N_EST,G_MON,SC_AFR_OCID,22,12,4,FL(LISA,BROWN,BROWN,BROWN,ESCUDO,YELLOW),1,{TR_EXERCITO,TR_CELEIRO,0,0},13,-6,5,0,NULL,"Segu domina o médio Níger com guerreiros e canoas."},
{"FJA","Futa Jallon",NULL,K_AF,N_EST,G_TEO,SC_AFR_OCID,10,20,10,FL(LISA,WHITE,WHITE,WHITE,CRESCENTE,GREEN),1,{TR_FE,TR_MONTANHAS,0,0},11,-12,3,0,NULL,"Imamato das terras altas da Guiné, nascente de grandes rios."},
{"TUA","Kel Ahaggar",NULL,K_AF,N_POV,G_CON,SC_AFR_NORTE,26,22,16,FL(BI_H,BLUE,YELLOW,YELLOW,NONE,WHITE),1,{TR_MOVIMENTO,TR_TERRA,0,0},23,6,10,0,NULL,"Os tuaregues guardam as rotas do Saara."},
{"MOR","Marrocos","Al-Maghrib",K_AF,N_EST,G_MON,SC_AFR_NORTE,22,8,6,FL(LISA,RED,RED,RED,NONE,WHITE),0,{TR_MONTANHAS,TR_FE,TR_ISOLAMENTO,0},34,-5,6,0,NULL,"O sultanato do extremo ocidente, entre o Atlas e dois mares."},
{"DAR","Darfur",NULL,K_AF,N_EST,G_MON,SC_AFR_NORTE,20,14,8,FL(LISA,BROWN,BROWN,BROWN,CRESCENTE,WHITE),1,{TR_FE,TR_CAVALEIROS,0,0},13,25,5,0,NULL,"Sultanato do Sahel oriental, encruzilhada de caravanas."},
{"FUN","Sennar","Funj",K_AF,N_EST,G_MON,SC_AFR_NORTE,22,16,10,FL(LISA,ORANGE,ORANGE,ORANGE,CRESCENTE,WHITE),1,{TR_FE,TR_CRISE,TR_CAVALEIROS,0},14,34,4,0,NULL,"O sultanato Funj do Nilo Azul, em declínio."},
{"ETH","Etiópia","Ityop'ya",K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,10,20,6,FL(TRI_H,GREEN,YELLOW,RED,NONE,WHITE),1,{TR_FE,TR_MONTANHAS,TR_CRISE,0},13,37,6,0,NULL,"Antigo império cristão das montanhas, dividido entre príncipes."},
{"KAF","Kaffa",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,14,20,8,FL(BI_V,GREEN,WHITE,WHITE,NONE,WHITE),1,{TR_MONTANHAS,TR_ENTREPOSTO,0,0},7,36,2,0,NULL,"Reino das terras altas onde nasceu o café."},
{"SOM","Sultanato Majeerteen",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,14,22,28,FL(LISA,SKY,SKY,SKY,ESTRELA,WHITE),1,{TR_ENTREPOSTO,TR_MOVIMENTO,0,0},10,50,4,0,NULL,"Portos do Chifre da África e criadores de camelos."},
{"MAA","Maasai",NULL,K_AF,N_POV,G_CON,SC_AFR_LESTE_SUL,26,8,8,FL(TRI_H,BLACK,RED,GREEN,ESCUDO,WHITE),1,{TR_DISCIPLINA,TR_MOVIMENTO,0,0},-2,36,5,0,NULL,"Pastores guerreiros do vale do Rift."},
{"BUG","Buganda",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,14,22,18,FL(TRI_V,BLUE,WHITE,BLUE,ESCUDO,RED),1,{TR_COESA,TR_BUROCRACIA,0,0},0,32,3,0,NULL,"O reino do kabaka e do lukiiko, às margens do lago Vitória."},
{"RWA","Ruanda",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,10,24,12,FL(LISA,GREEN,GREEN,GREEN,SOL,YELLOW),1,{TR_MONTANHAS,TR_COESA,0,0},-2,30,2,0,NULL,"O reino das mil colinas."},
{"OMA","Omã","ʿUmān",K_AS,N_EST,G_MON,SC_ORIENTE_MEDIO,24,4,8,FL(LISA,RED,RED,RED,NONE,WHITE),0,{TR_MARES,TR_ENTREPOSTO,0,0},24,59,4,0,NULL,"Mascate e Zanzibar: um império de monções e dhows."},
{"KON","Reino do Kongo","Kongo",K_AF,N_EST,G_MON,SC_AFR_OCID,18,10,18,FL(LISA,RED,RED,RED,CRUZ,WHITE),1,{TR_FE,TR_COESA,TR_CRISE,0},-6,14,4,0,NULL,"Reino cristão desde o século XV, hoje dividido entre linhagens."},
{"LUN","Império Lunda",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,22,16,20,FL(BI_H,RED,BLACK,BLACK,NONE,WHITE),1,{TR_ENTREPOSTO,TR_CONFEDERACAO,0,0},-8,23,7,0,NULL,"Rede de chefias da África central, ligada ao comércio dos dois oceanos."},
{"ROZ","Império Rozvi",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,20,12,6,FL(LISA,BROWN,BROWN,BROWN,SOL,GOLD),1,{TR_MINERAL,TR_CRISE,TR_COESA,0},-20,30,5,0,NULL,"Herdeiros das pedras do Grande Zimbábue."},
{"MTH","Mthethwa",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,16,10,6,FL(BI_H,BLACK,WHITE,WHITE,ESCUDO,WHITE),1,{TR_DISCIPLINA,TR_TERRA,0,0},-28,32,3,0,NULL,"A confederação de Dingiswayo, onde serve um jovem chamado Shaka."},
{"NDW","Ndwandwe",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,14,14,8,FL(BI_H,WHITE,BLACK,BLACK,ESCUDO,BLACK),1,{TR_DISCIPLINA,TR_EXERCITO,0,0},-27,31,3,0,NULL,"Rival dos Mthethwa pelo domínio do norte do Natal."},
{"SWZ","Ngwane","eSwatini",K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,10,16,22,FL(TRI_H,BLUE,YELLOW,BLUE,ESCUDO,BLACK),1,{TR_MONTANHAS,TR_COESA,0,0},-26,31,2,0,NULL,"Reino das montanhas que sobreviverá a todas as tempestades."},
{"XHO","amaXhosa",NULL,K_AF,N_POV,G_CON,SC_AFR_LESTE_SUL,12,16,10,FL(BI_H,GREEN,BLACK,BLACK,NONE,WHITE),1,{TR_TERRA,TR_GUERRILHA,0,0},-32,27,4,0,NULL,"Cem anos de guerras na fronteira do Cabo, sem se dobrar."},
{"HER","Ovaherero",NULL,K_AF,N_POV,G_CON,SC_AFR_LESTE_SUL,24,20,12,FL(BI_V,RED,WHITE,WHITE,NONE,WHITE),1,{TR_MOVIMENTO,TR_COESA,0,0},-21,17,6,0,NULL,"Criadores de gado das terras secas do sudoeste africano."},
{"CAP","Colônia do Cabo",NULL,K_AF,N_SUD,G_CIA,SC_AFR_LESTE_SUL,28,18,12,FL(TRI_H,ORANGE,WHITE,BLUE,NONE,WHITE),0,{TR_FRONTEIRA,0,0,0},-34,18,6,0,"NED","Escala das rotas da Índia, com colonos avançando para o interior."},
{"MER","Reino Merina","Imerina",K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,26,10,10,FL(BI_H,WHITE,RED,RED,NONE,WHITE),0,{TR_REFORMADOR,TR_COESA,0,0},-19,48,4,0,NULL,"As terras altas de Madagascar, unidas por Andrianampoinimerina."},
// ===== ORIENTE MEDIO, ESTEPE E SUL DA ASIA =====
{"QAJ","Pérsia","Iran",K_AS,N_EST,G_MON,SC_ORIENTE_MEDIO,8,22,18,FL(LISA,WHITE,WHITE,WHITE,LEAO,GOLD),0,{TR_ABSOLUTISMO,TR_FE,TR_CAVALEIROS,0},36,51,9,0,NULL,"O trono do leão e do sol, entre a Rússia e o Grande Jogo."},
{"DRY","Emirado de Diriyah",NULL,K_AS,N_EST,G_TEO,SC_ORIENTE_MEDIO,20,18,10,FL(LISA,GREEN,GREEN,GREEN,NONE,WHITE),1,{TR_FE,TR_MOVIMENTO,0,0},25,47,8,0,NULL,"O primeiro Estado saudita se espalha pela Arábia."},
{"YEM","Imamato do Iêmen",NULL,K_AS,N_EST,G_TEO,SC_ORIENTE_MEDIO,22,10,10,FL(LISA,RED,RED,RED,ESTRELA,WHITE),1,{TR_FE,TR_MONTANHAS,0,0},15,44,4,0,NULL,"Montanhas de Saná e o porto do café em Mocha."},
{"AFG","Império Durrani","Afghanistan",K_AS,N_EST,G_MON,SC_ESTEPE,14,12,6,FL(LISA,BLACK,BLACK,BLACK,NONE,WHITE),1,{TR_MONTANHAS,TR_GUERRILHA,TR_CONFEDERACAO,0},35,69,7,0,NULL,"O cemitério dos impérios, das passagens do Hindu Kush."},
{"BUK","Emirado de Bukhara",NULL,K_AS,N_EST,G_MON,SC_ESTEPE,24,22,14,FL(LISA,SKY,SKY,SKY,CRESCENTE,WHITE),1,{TR_CAVALEIROS,TR_FE,0,0},40,64,5,0,NULL,"Cidade sagrada da Rota da Seda."},
{"KHI","Canato de Khiva",NULL,K_AS,N_EST,G_MON,SC_ESTEPE,20,14,12,FL(LISA,GREEN,GREEN,GREEN,CRESCENTE,WHITE),1,{TR_CAVALEIROS,TR_TERRA,0,0},41,60,5,0,NULL,"Oásis de Corásmia cercado de desertos."},
{"KOK","Canato de Kokand",NULL,K_AS,N_EST,G_MON,SC_ESTEPE,18,24,16,FL(LISA,BLUE,BLUE,BLUE,CRESCENTE,WHITE),1,{TR_CAVALEIROS,TR_CELEIRO,0,0},41,71,4,0,NULL,"O vale de Fergana, fértil e cobiçado."},
{"KAZ","Cazaques","Qazaq",K_AS,N_POV,G_CON,SC_ESTEPE,24,24,18,FL(LISA,SKY,SKY,SKY,SOL,GOLD),1,{TR_MOVIMENTO,TR_CAVALEIROS,0,0},48,68,11,0,NULL,"As Três Hordas da estepe sem fim."},
{"SIK","Império Sikh","Sarkar-i-Khalsa",K_AS,N_EST,G_MON,SC_ASIA_SUL,28,18,6,FL(BI_H,ORANGE,BLUE,BLUE,NONE,WHITE),1,{TR_EXERCITO,TR_FE,0,0},32,74,5,0,NULL,"Ranjit Singh e o exército Khalsa, senhores do Punjab."},
{"MAR","Confederação Maratha",NULL,K_AS,N_EST,G_CON,SC_ASIA_SUL,30,14,2,FL(LISA,ORANGE,ORANGE,ORANGE,NONE,WHITE),0,{TR_CONFEDERACAO,TR_CAVALEIROS,0,0},19,74,8,0,NULL,"A maior potência indiana, cavalaria veloz e chefes rivais."},
{"MUG","Império Mogol",NULL,K_AS,N_EST,G_MON,SC_ASIA_SUL,6,20,12,FL(LISA,GREEN,GREEN,GREEN,SOL,GOLD),0,{TR_GLORIA,0,0,0},29,77,2,0,NULL,"Délhi guarda o trono dos Grandes Mogóis, e quase mais nada."},
{"EIC","Companhia das Índias",NULL,K_AS,N_SUD,G_CIA,SC_ASIA_SUL,22,8,8,FL(LISTRAS,RED,WHITE,NAVY,CRUZ,WHITE),0,{TR_COLONIAL,TR_ENTREPOSTO,0,0},23,88,8,0,"GBR","Uma empresa com exército próprio e Bengala nas mãos."},
{"HYD","Hyderabad",NULL,K_AS,N_SUD,G_MON,SC_ASIA_SUL,16,24,10,FL(LISA,YELLOW,YELLOW,YELLOW,NONE,WHITE),1,{TR_MINERAL,0,0,0},17,78,6,0,"EIC","O Nizam, aliado subsidiário da Companhia, e seus diamantes."},
{"NEP","Nepal",NULL,K_AS,N_EST,G_MON,SC_ASIA_SUL,24,6,14,FL(LISA,CRIMSON,CRIMSON,CRIMSON,SOL,WHITE),1,{TR_MONTANHAS,TR_DISCIPLINA,0,0},28,85,3,0,NULL,"Os gurkhas e o reino nas encostas do Himalaia."},
{"BHU","Butão","Druk Yul",K_AS,N_EST,G_TEO,SC_ASIA_SUL,20,12,16,FL(DIAG,YELLOW,ORANGE,ORANGE,DRAGAO,WHITE),0,{TR_MONTANHAS,TR_ISOLAMENTO,TR_COESA,0},27,90,2,0,NULL,"O reino do dragão do trovão."},
{"KDY","Kandy",NULL,K_AS,N_EST,G_MON,SC_ASIA_SUL,22,14,8,FL(BORDA,YELLOW,CRIMSON,CRIMSON,LEAO,YELLOW),0,{TR_TERRA,TR_FE,0,0},7,81,2,0,NULL,"O último reino cingalês, protegido pelas montanhas."},
// ===== ASIA ORIENTAL E SUDESTE =====
{"QNG","Dinastia Qing","Da Qing",K_AS,N_EST,G_MON,SC_ASIA_LESTE,30,26,8,FL(LISA,YELLOW,YELLOW,YELLOW,DRAGAO,BLUE),1,{TR_BUROCRACIA,TR_CELEIRO,TR_CULTURA,TR_MULTIETNICO},40,116,10,0,NULL,"O maior Estado do mundo, confiante e fechado em seu sistema."},
{"JAP","Japão","Nihon",K_AS,N_EST,G_MON,SC_ASIA_LESTE,26,10,14,FL(LISA,WHITE,WHITE,WHITE,DISCO,RED),0,{TR_ISOLAMENTO,TR_COESA,TR_BUROCRACIA,TR_CULTURA},36,140,5,0,NULL,"O xogunato Tokugawa: dois séculos de paz atrás de portos fechados."},
{"KOR","Joseon","Joseon",K_AS,N_EST,G_MON,SC_ASIA_LESTE,14,18,26,FL(LISA,WHITE,WHITE,WHITE,DISCO,BLUE),1,{TR_BUROCRACIA,TR_COESA,TR_CULTURA,TR_ISOLAMENTO},38,127,4,0,NULL,"O Reino Eremita dos letrados confucianos."},
{"RYU","Ryukyu","Ruuchuu",K_AS,N_EST,G_MON,SC_ASIA_LESTE,26,18,22,FL(LISA,WHITE,WHITE,WHITE,ESTRELA,RED),1,{TR_ENTREPOSTO,TR_COESA,0,0},26,128,1,0,NULL,"Reino insular que comercia com a China e o Japão ao mesmo tempo."},
{"NGU","Nguyễn","Nguyễn",K_AS,N_EST,G_MON,SC_SUDESTE,20,24,8,FL(LISA,YELLOW,YELLOW,YELLOW,NONE,WHITE),1,{TR_GUERRILHA,TR_BUROCRACIA,0,0},11,107,4,0,NULL,"Nguyễn Ánh luta desde o sul para reunir o Vietnã."},
{"TSN","Tây Sơn","Tây Sơn",K_AS,N_EST,G_MON,SC_SUDESTE,26,14,4,FL(LISA,RED,RED,RED,NONE,WHITE),1,{TR_GUERRILHA,TR_LEVEE,0,0},16,108,4,0,NULL,"A dinastia nascida de uma revolta camponesa, agora na defensiva."},
{"SIA","Sião","Sayam",K_AS,N_EST,G_MON,SC_SUDESTE,10,22,22,FL(LISA,RED,RED,RED,NONE,WHITE),0,{TR_REFORMADOR,TR_ENTREPOSTO,0,0},14,101,6,0,NULL,"Bangcoc equilibra impérios rivais e nunca será colônia."},
{"BRM","Birmânia","Konbaung",K_AS,N_EST,G_MON,SC_SUDESTE,18,16,6,FL(LISA,WHITE,WHITE,WHITE,ESTRELA,GOLD),1,{TR_EXERCITO,TR_FE,0,0},22,96,6,0,NULL,"A dinastia Konbaung, conquistadora e orgulhosa."},
{"VTE","Vientiane","Lan Xang",K_AS,N_SUD,G_MON,SC_SUDESTE,16,22,12,FL(LISA,RED,RED,RED,DISCO,WHITE),1,{TR_TERRA,TR_FE,0,0},18,103,4,0,"SIA","Herdeiro de Lan Xang, vassalo de Bangcoc."},
{"KHM","Camboja","Kampuchea",K_AS,N_SUD,G_MON,SC_SUDESTE,22,18,16,FL(BORDA,RED,BLUE,BLUE,NONE,WHITE),1,{TR_CELEIRO,TR_FE,0,0},12,105,3,0,"SIA","O reino de Angkor, disputado por vizinhos maiores."},
{"ACE","Sultanato de Aceh",NULL,K_AS,N_EST,G_MON,SC_SUDESTE,10,18,10,FL(LISA,RED,RED,RED,CRESCENTE,WHITE),1,{TR_ENTREPOSTO,TR_GUERRILHA,0,0},6,95,3,0,NULL,"A varanda de Meca: pimenta, fé e resistência."},
{"SUL","Sultanato de Sulu",NULL,K_AS,N_EST,G_MON,SC_SUDESTE,20,10,20,FL(LISA,RED,RED,RED,CRESCENTE,YELLOW),1,{TR_ENTREPOSTO,TR_MARES,0,0},6,121,2,0,NULL,"Navegantes e comerciantes do mar de Sulu."},
{"BRU","Brunei",NULL,K_AS,N_EST,G_MON,SC_SUDESTE,24,24,8,FL(LISA,YELLOW,YELLOW,YELLOW,NONE,WHITE),1,{TR_ENTREPOSTO,TR_MARES,0,0},5,115,3,0,NULL,"Antigo senhor de Bornéu, ainda rico em portos."},
{"JOH","Johor",NULL,K_AS,N_EST,G_MON,SC_SUDESTE,14,10,20,FL(CANTAO,BLUE,RED,WHITE,CRESCENTE,WHITE),1,{TR_ENTREPOSTO,TR_MARES,0,0},1,104,2,0,NULL,"O sultanato do estreito onde nascerá Singapura."},
{"DEI","Índias Orientais Neerlandesas",NULL,K_AS,N_SUD,G_CIA,SC_SUDESTE,28,16,8,FL(TRI_H,RED,WHITE,BLUE,NONE,WHITE),0,{TR_ENTREPOSTO,TR_COLONIAL,0,0},-6,107,5,0,"NED","Batávia e as ilhas das especiarias sob domínio neerlandês."},
{"PHI","Filipinas",NULL,K_AS,N_SUD,G_MON,SC_SUDESTE,26,22,8,FL(SALTIRE,WHITE,RED,RED,NONE,WHITE),0,{TR_ENTREPOSTO,0,0,0},15,121,4,0,"ESP","Manila e o galeão que liga a Ásia à América."},
{"CHK","Luoravetlan",NULL,K_AS,N_POV,G_CON,SC_ARTICO,22,24,26,FL(BI_H,WHITE,RED,RED,NONE,WHITE),1,{TR_TERRA,TR_INVERNO,0,0},66,175,6,0,NULL,"Os Chukchi, invictos nas tundras do extremo oriente."},
// ===== OCEANIA =====
{"NGP","Ngāpuhi","Ngāpuhi",K_OC,N_POV,G_CON,SC_OCEANIA,20,6,6,FL(BI_H,BLACK,RED,RED,NONE,WHITE),1,{TR_TERRA,TR_MARES,0,0},-35,174,2,0,NULL,"Iwi do norte da Aotearoa, mestres das fortificações pā."},
{"TAI","Waikato-Tainui","Tainui",K_OC,N_POV,G_CON,SC_OCEANIA,8,14,20,FL(BI_H,RED,WHITE,WHITE,NONE,WHITE),1,{TR_TERRA,TR_MARES,0,0},-38,175,2,0,NULL,"Os povos da canoa Tainui, ao longo do rio Waikato."},
{"NTA","Ngāi Tahu","Ngāi Tahu",K_OC,N_POV,G_CON,SC_OCEANIA,14,20,14,FL(BI_H,WHITE,BLACK,BLACK,NONE,WHITE),1,{TR_TERRA,TR_MOVIMENTO,0,0},-43,172,4,0,NULL,"O principal iwi da Te Waipounamu, a ilha do Sul."},
{"TON","Tonga",NULL,K_OC,N_EST,G_MON,SC_OCEANIA,26,6,8,FL(CANTAO,RED,WHITE,RED,CRUZ,RED),1,{TR_MARES,TR_COESA,0,0},-21,-175,1,0,NULL,"Reino de navegadores que nunca será colonizado."},
{"TAH","Taiti","Tahiti",K_OC,N_EST,G_MON,SC_OCEANIA,10,20,26,FL(TRI_H,RED,WHITE,RED,NONE,WHITE),1,{TR_MARES,TR_COESA,0,0},-18,-150,1,0,NULL,"A dinastia Pōmare une as ilhas da Sociedade."},
{"NSW","Nova Gales do Sul",NULL,K_OC,N_SUD,G_MON,SC_OCEANIA,24,10,10,FL(CANTAO,BLUE,NAVY,WHITE,ESTRELA,WHITE),0,{TR_FRONTEIRA,0,0,0},-34,151,2,0,"GBR","Colônia britânica fundada em 1788 em terra eora."},
{"WIR","Wiradjuri",NULL,K_OC,N_POV,G_CON,SC_OCEANIA,22,14,8,FL(BI_H,BLACK,RED,RED,SOL,YELLOW),1,{TR_TERRA,TR_COESA,0,0},-33,147,6,0,NULL,"O maior povo das planícies do interior da Nova Gales do Sul."},
{"KUL","Kulin",NULL,K_OC,N_POV,G_CON,SC_OCEANIA,16,14,10,FL(BI_H,RED,BLACK,BLACK,SOL,YELLOW),1,{TR_TERRA,TR_CONFEDERACAO,0,0},-38,145,4,0,NULL,"A aliança de povos de Port Phillip."},
{"NOO","Noongar",NULL,K_OC,N_POV,G_CON,SC_OCEANIA,18,10,6,FL(BI_H,BLACK,ORANGE,ORANGE,SOL,YELLOW),1,{TR_TERRA,TR_COESA,0,0},-32,116,7,0,NULL,"O sudoeste da Austrália, terra de seis estações."},
{"YOL","Yolŋu",NULL,K_OC,N_POV,G_CON,SC_OCEANIA,24,18,6,FL(BI_H,BLACK,YELLOW,YELLOW,SOL,RED),1,{TR_TERRA,TR_ENTREPOSTO,0,0},-12,136,6,0,NULL,"Arnhem Land: comércio de trepang com os makassar há séculos."},
{"ARR","Arrernte",NULL,K_OC,N_POV,G_CON,SC_OCEANIA,26,12,6,FL(BI_H,BLACK,RED,RED,SOL,YELLOW),1,{TR_TERRA,TR_MOVIMENTO,0,0},-24,134,10,0,NULL,"O coração vermelho do continente."},
// ===== SURGEM POR EVENTO (yFrom > 0) =====
{"VIE","Vietnã","Việt Nam",K_AS,N_EST,G_MON,SC_SUDESTE,20,24,8,FL(LISA,YELLOW,YELLOW,YELLOW,NONE,WHITE),1,{TR_GUERRILHA,TR_BUROCRACIA,0,0},16,108,6,1802,NULL,"Gia Long reúne o país de norte a sul."},
{"SOK","Califado de Sokoto",NULL,K_AF,N_EST,G_TEO,SC_AFR_OCID,12,20,10,FL(LISA,WHITE,WHITE,WHITE,CRESCENTE,GREEN),1,{TR_FE,TR_CAVALEIROS,TR_CONFEDERACAO,0},13,5,6,1808,NULL,"Uma reforma religiosa une os emirados hauçás."},
{"ZUL","Reino Zulu","amaZulu",K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,16,10,6,FL(BI_H,BLACK,WHITE,WHITE,ESCUDO,WHITE),1,{TR_DISCIPLINA,TR_EXERCITO,TR_TERRA,0},-28,31,5,1818,NULL,"Shaka transforma regimentos em um reino temido."},
{"SRB","Sérvia","Srbija",K_EU,N_EST,G_MON,SC_MEDITERRANEO,22,8,10,FL(TRI_H,RED,BLUE,WHITE,NONE,WHITE),0,{TR_GUERRILHA,TR_FE,0,0},45,21,3,1817,NULL,"Principado nascido de revoltas contra Istambul."},
{"PAR","Paraguai",NULL,K_AM,N_EST,G_REP,SC_AMS,24,10,8,FL(TRI_H,RED,WHITE,BLUE,ESTRELA,YELLOW),0,{TR_COESA,TR_EXERCITO,0,0},-25,-58,4,1811,NULL,"Isolado e determinado, no coração da América do Sul."},
{"ARG","Argentina",NULL,K_AM,N_EST,G_REP,SC_AMS,22,26,28,FL(TRI_H,SKY,WHITE,SKY,SOL,GOLD),0,{TR_CELEIRO,TR_FRONTEIRA,0,0},-35,-58,9,1816,NULL,"As Províncias Unidas do Rio da Prata."},
{"CHL","Chile",NULL,K_AM,N_EST,G_REP,SC_AMS,24,6,10,FL(CANTAO,WHITE,BLUE,RED,ESTRELA,WHITE),0,{TR_EXERCITO,TR_MINERAL,0,0},-33,-71,5,1818,NULL,"Uma faixa entre os Andes e o Pacífico."},
{"GCO","Gran Colombia",NULL,K_AM,N_EST,G_REP,SC_AMS,28,24,12,FL(TRI_H,YELLOW,BLUE,RED,NONE,WHITE),0,{TR_JOVEM,TR_REVOLUCAO,0,0},5,-74,9,1819,NULL,"O sonho de Bolívar: Venezuela, Nova Granada e Quito unidos."},
{"MEX","México",NULL,K_AM,N_EST,G_REP,SC_AMS,10,18,10,FL(TRI_V,GREEN,WHITE,RED,AGUIA,BROWN),0,{TR_MINERAL,TR_JOVEM,0,0},19,-99,10,1821,NULL,"Herdeiro de Tenochtitlan e da Nova Espanha."},
{"PER","Peru",NULL,K_AM,N_EST,G_REP,SC_AMS,24,20,6,FL(TRI_V,RED,WHITE,RED,NONE,WHITE),0,{TR_MINERAL,TR_JOVEM,0,0},-12,-77,8,1821,NULL,"Lima livre, guano e prata."},
{"BOL","Bolívia",NULL,K_AM,N_EST,G_REP,SC_AMS,20,18,8,FL(TRI_H,RED,YELLOW,GREEN,NONE,WHITE),0,{TR_MINERAL,TR_MONTANHAS,0,0},-19,-65,6,1825,NULL,"A república do Altiplano, batizada em honra a Bolívar."},
{"URU","Uruguai",NULL,K_AM,N_EST,G_REP,SC_AMS,16,22,28,FL(LISTRAS,WHITE,BLUE,WHITE,SOL,GOLD),0,{TR_CELEIRO,TR_ENTREPOSTO,0,0},-35,-56,2,1828,NULL,"A Banda Oriental, entre dois gigantes."},
{"GRE","Grécia","Ellada",K_EU,N_EST,G_MON,SC_MEDITERRANEO,10,18,30,FL(CRUZ,BLUE,WHITE,WHITE,NONE,WHITE),0,{TR_MARES,TR_CULTURA,0,0},38,24,3,1830,NULL,"O berço da democracia livre de novo."},
{"BEL","Bélgica",NULL,K_EU,N_EST,G_MON,SC_EUR_OCID,26,24,6,FL(TRI_V,BLACK,YELLOW,RED,NONE,WHITE),0,{TR_INDUSTRIAL,TR_ENTREPOSTO,0,0},51,4,2,1830,NULL,"Carvão, aço e a primeira ferrovia do continente."},
{"TEX","Texas",NULL,K_AM,N_EST,G_REP,SC_AMN_PLANICIES,24,18,20,FL(BI_H,WHITE,RED,BLUE,ESTRELA,WHITE),0,{TR_FRONTEIRA,TR_JOVEM,0,0},30,-97,6,1836,NULL,"A república da estrela solitária."},
{"LIB","Libéria",NULL,K_AF,N_EST,G_REP,SC_AFR_OCID,22,14,22,FL(LISTRAS,RED,WHITE,NAVY,ESTRELA,WHITE),0,{TR_JOVEM,TR_ENTREPOSTO,0,0},6,-11,3,1847,NULL,"A primeira república independente da África."},
{"MAY","Chan Santa Cruz",NULL,K_AM,N_POV,G_TEO,SC_CARIBE,12,22,14,FL(LISA,GREEN,GREEN,GREEN,CRUZ,WHITE),1,{TR_GUERRILHA,TR_TERRA,TR_FE,0},19,-88,3,1847,NULL,"Os maias do Yucatán se levantam e fundam seu próprio Estado."},
{"TRV","Transvaal",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,10,16,8,FL(TRI_H,RED,WHITE,BLUE,NONE,WHITE),0,{TR_FRONTEIRA,TR_MINERAL,0,0},-26,28,6,1852,NULL,"A República Sul-Africana dos bôeres."},
{"ORA","Estado Livre de Orange",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,28,20,8,FL(LISTRAS,ORANGE,WHITE,RED,NONE,WHITE),0,{TR_FRONTEIRA,TR_CELEIRO,0,0},-29,26,5,1854,NULL,"República bôer entre os rios Orange e Vaal."},
{"SOT","Lesoto",NULL,K_AF,N_EST,G_MON,SC_AFR_LESTE_SUL,12,18,26,FL(TRI_H,BLUE,WHITE,GREEN,ESCUDO,BLACK),1,{TR_MONTANHAS,TR_COESA,0,0},-29,28,2,1822,NULL,"Moshoeshoe reúne refugiados numa fortaleza de montanha."},
{"RAJ","Raj Britânico",NULL,K_AS,N_SUD,G_MON,SC_ASIA_SUL,22,8,8,FL(CANTAO,RED,NAVY,WHITE,ESTRELA,WHITE),0,{TR_COLONIAL,TR_BUROCRACIA,0,0},23,88,12,1858,"GBR","A coroa toma o lugar da Companhia."},
{"ITA","Itália",NULL,K_EU,N_EST,G_MON,SC_MEDITERRANEO,10,20,10,FL(TRI_V,GREEN,WHITE,RED,ESCUDO,RED),0,{TR_CULTURA,TR_JOVEM,0,0},42,12,6,1861,NULL,"Do Piemonte à Sicília, uma nação reunida."},
{"CSA","Estados Confederados",NULL,K_AM,N_EST,G_REP,SC_AMN_LESTE,18,16,18,FL(SALTIRE,RED,BLUE,BLUE,ESTRELA,WHITE),0,{TR_EXERCITO,0,0,0},38,-77,8,1861,NULL,"Os estados do sul rompem com a União."},
{"CAN","Canadá",NULL,K_AM,N_EST,G_MON,SC_AMN_LESTE,22,6,6,FL(CANTAO,RED,NAVY,WHITE,CRUZ,WHITE),0,{TR_FRONTEIRA,TR_CELEIRO,0,0},45,-76,10,1867,NULL,"A Confederação do Canadá, de costa a costa."},
{"AUH","Áustria-Hungria",NULL,K_EU,N_EST,G_MON,SC_EUR_CENT,28,28,25,FL(BI_H,BLACK,YELLOW,YELLOW,AGUIA2,BLACK),0,{TR_MULTIETNICO,TR_CULTURA,TR_INDUSTRIAL,0},48,16,8,1867,NULL,"A monarquia dual de Viena e Budapeste."},
{"GER","Alemanha","Deutschland",K_EU,N_EST,G_MON,SC_EUR_CENT,8,10,14,FL(TRI_H,BLACK,WHITE,RED,NONE,WHITE),0,{TR_EXERCITO,TR_INDUSTRIAL,0,0},52,13,8,1871,NULL,"O Império Alemão nasce em Versalhes."},
{"ROM","Romênia","România",K_EU,N_EST,G_MON,SC_EUR_LESTE,20,20,6,FL(TRI_V,BLUE,YELLOW,RED,NONE,WHITE),0,{TR_CELEIRO,TR_JOVEM,0,0},44,26,4,1878,NULL,"Valáquia e Moldávia unidas e independentes."},
{"ZAF","União Sul-Africana",NULL,K_AF,N_SUD,G_MON,SC_AFR_LESTE_SUL,28,18,12,FL(TRI_H,ORANGE,WHITE,BLUE,NONE,WHITE),0,{TR_MINERAL,TR_INDUSTRIAL,0,0},-26,28,8,1910,"GBR","Os quatro territórios do sul unidos sob a coroa."},
{"NOR","Noruega","Norge",K_EU,N_EST,G_MON,SC_EUR_CENT,24,8,12,FL(NORDICA,RED,NAVY,NAVY,NONE,WHITE),0,{TR_MARES,TR_COESA,0,0},60,11,6,1905,NULL,"Fiordes, frotas e uma coroa própria outra vez."},
{"CHN","República da China","Zhonghua Minguo",K_AS,N_EST,G_REP,SC_ASIA_LESTE,30,26,8,FL(CANTAO,RED,BLUE,WHITE,SOL,WHITE),0,{TR_JOVEM,TR_MULTIETNICO,TR_CELEIRO,0},32,119,10,1912,NULL,"Cai o último imperador; nasce a república."},
{"FIN","Finlândia","Suomi",K_EU,N_EST,G_REP,SC_EUR_CENT,22,26,30,FL(NORDICA,WHITE,BLUE,BLUE,NONE,WHITE),0,{TR_INVERNO,TR_COESA,0,0},60,25,6,1917,NULL,"Mil lagos e uma independência conquistada no inverno."},
{"POL","Polônia","Polska",K_EU,N_EST,G_REP,SC_EUR_LESTE,28,10,14,FL(BI_H,WHITE,RED,RED,NONE,WHITE),0,{TR_GUERRILHA,TR_FE,0,0},52,21,6,1918,NULL,"Renascida depois de 123 anos de partilha."},
{"CZE","Tchecoslováquia",NULL,K_EU,N_EST,G_REP,SC_EUR_CENT,12,20,24,FL(BI_H,WHITE,RED,BLUE,NONE,WHITE),0,{TR_INDUSTRIAL,TR_MONTANHAS,0,0},50,14,4,1918,NULL,"Indústria da Boêmia e montanhas da Eslováquia."},
{"YUG","Iugoslávia",NULL,K_EU,N_EST,G_MON,SC_MEDITERRANEO,22,8,10,FL(TRI_H,BLUE,WHITE,RED,NONE,WHITE),0,{TR_MULTIETNICO,TR_GUERRILHA,TR_MONTANHAS,0},45,20,6,1918,NULL,"O reino dos eslavos do sul."},
{"AUT","Áustria","Österreich",K_EU,N_EST,G_REP,SC_EUR_CENT,28,28,25,FL(TRI_H,RED,WHITE,RED,NONE,WHITE),0,{TR_CULTURA,TR_MONTANHAS,0,0},48,16,4,1918,NULL,"Viena sem império, mas com toda a sua música."},
{"HUN","Hungria","Magyarország",K_EU,N_EST,G_MON,SC_EUR_CENT,14,20,8,FL(TRI_H,RED,WHITE,GREEN,NONE,WHITE),0,{TR_CELEIRO,TR_CAVALEIROS,0,0},47,19,4,1918,NULL,"A planície do Danúbio, sozinha outra vez."},
{"IRF","Estado Livre Irlandês","Saorstát Éireann",K_EU,N_EST,G_REP,SC_EUR_OCID,6,20,8,FL(TRI_V,GREEN,WHITE,ORANGE,NONE,WHITE),0,{TR_COESA,TR_GUERRILHA,0,0},53,-6,4,1922,NULL,"Depois de séculos, a Irlanda se governa."},
{"URS","União Soviética","SSSR",K_EU,N_EST,G_REP,SC_EUR_LESTE,26,4,4,FL(LISA,RED,RED,RED,ESTRELA,YELLOW),0,{TR_INVERNO,TR_INDUSTRIAL,TR_MULTIETNICO,0},56,38,12,1922,NULL,"Um Estado operário do Báltico ao Pacífico."},
{"TUR","Turquia","Türkiye",K_AS,N_EST,G_REP,SC_MEDITERRANEO,16,20,10,FL(LISA,RED,RED,RED,CRESCENTE,WHITE),0,{TR_REFORMADOR,TR_EXERCITO,0,0},40,33,7,1923,NULL,"Uma república moderna nas ruínas do império."},
{"IND","Índia","Bhārat",K_AS,N_EST,G_REP,SC_ASIA_SUL,30,16,6,FL(TRI_H,ORANGE,WHITE,GREEN,SOL,NAVY),0,{TR_JOVEM,TR_MULTIETNICO,TR_CELEIRO,0},29,77,12,1947,NULL,"A maior democracia do mundo nasce à meia-noite."},
{"PAK","Paquistão",NULL,K_AS,N_EST,G_REP,SC_ASIA_SUL,6,20,10,FL(BI_V,WHITE,DKGREEN,DKGREEN,CRESCENTE,WHITE),0,{TR_FE,TR_JOVEM,0,0},25,67,6,1947,NULL,"Uma pátria criada na partição de 1947."},
{"ISR","Israel",NULL,K_AS,N_EST,G_REP,SC_ORIENTE_MEDIO,16,20,28,FL(TRI_H,WHITE,BLUE,WHITE,ESTRELA,BLUE),0,{TR_REFORMADOR,TR_DISCIPLINA,0,0},32,35,2,1948,NULL,"Estado criado em 1948, em guerra desde o primeiro dia."},
{"PRC","República Popular da China","Zhonghua Renmin Gongheguo",K_AS,N_EST,G_REP,SC_ASIA_LESTE,26,6,6,FL(LISA,RED,RED,RED,ESTRELA,YELLOW),0,{TR_INDUSTRIAL,TR_MULTIETNICO,TR_CELEIRO,0},40,116,12,1949,NULL,"Depois de um século de humilhações, um novo poder."},
{"IDN","Indonésia",NULL,K_AS,N_EST,G_REP,SC_SUDESTE,26,8,8,FL(BI_H,RED,WHITE,WHITE,NONE,WHITE),0,{TR_JOVEM,TR_MARES,TR_MULTIETNICO,0},-6,107,8,1949,NULL,"Milhares de ilhas, uma só república."},
{"GHA","Gana",NULL,K_AF,N_EST,G_REP,SC_AFR_OCID,28,24,4,FL(TRI_H,RED,GOLD,GREEN,ESTRELA,BLACK),0,{TR_JOVEM,TR_MINERAL,0,0},6,0,4,1957,NULL,"A primeira colônia da África subsaariana a se libertar."},
{"DZA","Argélia",NULL,K_AF,N_EST,G_REP,SC_AFR_NORTE,6,18,8,FL(BI_V,GREEN,WHITE,WHITE,CRESCENTE,RED),0,{TR_GUERRILHA,TR_JOVEM,0,0},37,3,8,1962,NULL,"Independência conquistada em oito anos de guerra."},
{"KEN","Quênia",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,6,14,6,FL(TRI_H,BLACK,RED,GREEN,ESCUDO,WHITE),0,{TR_JOVEM,TR_CELEIRO,0,0},-1,37,5,1963,NULL,"Uhuru: liberdade nas terras altas do Quênia."},
{"AGO","Angola",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,20,4,4,FL(BI_H,RED,BLACK,BLACK,ESTRELA,YELLOW),0,{TR_MINERAL,TR_GUERRILHA,0,0},-9,13,8,1975,NULL,"Petróleo, diamantes e uma longa luta pela liberdade."},
{"MOZ","Moçambique",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,6,18,14,FL(DIAG,GREEN,BLACK,YELLOW,ESTRELA,YELLOW),0,{TR_JOVEM,TR_MARES,0,0},-26,33,7,1975,NULL,"A costa do Índico, livre depois de cinco séculos."},
{"UKR","Ucrânia","Ukraïna",K_EU,N_EST,G_REP,SC_EUR_LESTE,28,26,8,FL(BI_H,SKY,YELLOW,YELLOW,NONE,WHITE),0,{TR_CELEIRO,TR_COESA,0,0},50,31,7,1991,NULL,"O celeiro da Europa, independente."},
{"SSD","Sudão do Sul",NULL,K_AF,N_EST,G_REP,SC_AFR_LESTE_SUL,10,12,22,FL(TRI_H,BLACK,RED,GREEN,ESTRELA,YELLOW),0,{TR_JOVEM,TR_TERRA,0,0},5,32,5,2011,NULL,"O país mais jovem do mundo."},
};
const int NATION_COUNT=(int)(sizeof(NATIONS)/sizeof(NATIONS[0]));

// Centros extras (mapa-mundi 1800): pintam imperios e colonias grandes sem
// precisar de fronteiras reais. Substituidos por dados de regiao no M4.
const NatAnchor NAT_ANCHORS[]={
 {"RUS",56,38,14},{"RUS",56,49,12},{"RUS",52,55,10},{"RUS",58,68,20},{"RUS",56,85,20},
 {"RUS",52,104,20},{"RUS",62,130,24},{"RUS",59,143,20},{"RUS",47,40,8},
 {"QNG",34,109,14},{"QNG",32,119,12},{"QNG",23,113,12},{"QNG",31,104,12},{"QNG",30,91,16},
 {"QNG",48,107,18},{"QNG",39,76,14},{"QNG",42,123,12},{"QNG",25,103,10},{"QNG",44,88,14},
 {"OTT",40,33,8},{"OTT",33,44,10},{"OTT",33,36,8},{"OTT",21,40,8},{"OTT",43,23,8},{"OTT",40,41,6},{"OTT",30,48,6},
 {"NSP",36,-106,14},{"NSP",15,-91,8},{"NSP",36,-121,10},{"NSP",29,-106,10},{"NSP",21,-103,8},
 {"NGR",10,-67,10},{"NGR",0,-78,8},
 {"VPE",-33,-71,8},{"VPE",-14,-72,8},
 {"RDP",-25,-58,8},{"RDP",-20,-66,8},{"RDP",-31,-64,8},
 {"BRA",-13,-38,12},{"BRA",-1,-48,14},{"BRA",-16,-56,12},{"BRA",-24,-47,8},{"BRA",-8,-35,8},
 {"USA",42,-71,6},{"USA",33,-80,6},{"USA",40,-80,6},{"USA",36,-86,6},
 {"BNA",44,-79,6},{"BNA",45,-64,6},
 {"EIC",13,80,8},{"EIC",19,73,6},{"EIC",26,82,6},
 {"HAB",47,19,8},{"HAB",50,14,5},{"HAB",45,12,4},{"HAB",46,24,6},
 {"GBR",56,-3,5},
 {"MAR",26,78,8},{"MAR",21,79,8},{"MAR",22,73,6},
 {"QAJ",36,59,8},{"QAJ",38,46,6},{"QAJ",30,52,8},
 {"AFG",31,65,6},{"AFG",34,62,6},
 {"JAP",35,135,5},{"JAP",43,142,4},
 {"TSN",21,105,6},
 {"SWE",63,26,8},{"SWE",65,18,8},
 {"DAN",62,10,8},{"DAN",68,16,6},{"DAN",64,-51,4},
 {"DEI",-7,112,4},{"DEI",-4,128,3},
 {"EGY",25,32,8},
 {"CAP",-32,22,6},
 {"KAZ",50,75,12},{"KAZ",45,62,10},
};
const int NAT_ANCHOR_COUNT=(int)(sizeof(NAT_ANCHORS)/sizeof(NAT_ANCHORS[0]));

static const char *const CONT_N[CONT_COUNT]={"Europa","Américas","África","Ásia","Oceania"};
static const char *const GOV_N[NG_COUNT]={"Monarquia","República","Teocracia","Conselho","Companhia"};
static const u8 FPAL[FC_COUNT][3]={
 {31,31,31},{26,4,4},{4,10,24},{2,4,14},{4,18,6},{30,26,4},{2,2,2},
 {30,16,2},{18,2,4},{14,22,30},{14,4,18},{14,9,4},{2,10,4},{26,20,6},
};

int Nat_Find(const char *tag){
 if(!tag)return -1;
 for(int i=0;i<NATION_COUNT;i++){
  const char *a=NATIONS[i].tag;
  if(a[0]==tag[0]&&a[1]==tag[1]&&a[2]==tag[2]&&tag[3]==0)return i;
 }
 return -1;
}
const char *Nat_ContName(u8 c){return c<CONT_COUNT?CONT_N[c]:"?";}
const char *Nat_GovName(u8 g){return g<NG_COUNT?GOV_N[g]:"?";}
void Flag_PalRGB(u8 c,u8 *r,u8 *g,u8 *b){
 if(c>=FC_COUNT)c=FC_WHITE;
 *r=FPAL[c][0];*g=FPAL[c][1];*b=FPAL[c][2];
}
