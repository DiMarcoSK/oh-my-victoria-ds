#include "history.h"
#include "wdiplo.h"
#include "str.h"
// history.c — linha do tempo 1801-2025 (DESIGN.md sec 7.2) e eventos
// aleatorios por era (sec 7.5). Datas verificadas; efeitos sao proposta de
// design. Regras de texto: DESIGN.md sec 3.4 e 6.6 (tom sobrio, sem
// glorificar violencia, povos com agencia). Eventos de conquista de povos
// indigenas so disparam se o agressor for MUITO mais forte (HC_STRONGER):
// um povo forte no seu mundo resiste e a historia muda.

#define CX(a) {HC_EXISTS,a,0,0,0}
#define CN(a) {HC_NOT_EXISTS,a,0,0,0}
#define CSUB(a,b) {HC_SUBJECT,a,b,0,0}
#define CSTR(a,b,v) {HC_STRONGER,a,b,0,v}
#define CWAR(a,b) {HC_AT_WAR,a,b,0,0}
#define CFIRED(id) {HC_FIRED,id,0,0,0}
#define EPR(a,v) {HE_PRESTIGE,a,0,0,v}
#define EGO(a,v) {HE_GOLD,a,0,0,v}
#define ETR(a,v) {HE_TROOPS,a,0,0,v}
#define EHA(a,v) {HE_HAPPY,a,0,0,v}
#define EMI(a,v) {HE_MILIT,a,0,0,v}
#define EIND(a) {HE_INDEP,a,0,0,0}
#define ESUB(a,b) {HE_SUBJECT,a,b,0,0}
#define EANX(a,b) {HE_ANNEX,a,b,0,0}
#define EUNI(a,b) {HE_UNIFY,a,b,0,0}
#define EBEC(a,b) {HE_BECOME,a,b,0,0}
#define ESPN(a,b,v) {HE_SPAWN,a,b,0,v}
#define ECED(a,b,c,v) {HE_CEDE,a,b,c,v}
#define EREL(a,b,c,v) {HE_RELOCATE,a,b,c,v}
#define EWAR(a,b) {HE_WAR,a,b,0,0}
#define EPAZ(a,b) {HE_PEACE,a,b,0,0}
#define EPAZALL {HE_PEACEALL,0,0,0,0}
#define EGOV(a,v) {HE_GOV,a,0,0,v}
#define EADD(a,v) {HE_ADDTRAIT,a,0,0,v}
#define EDEL(a,v) {HE_DELTRAIT,a,0,0,v}
#define EWGO(v) {HE_WORLD_GOLD,0,0,0,v}
#define EWHA(v) {HE_WORLD_HAPPY,0,0,0,v}
#define EDECOL(v) {HE_DECOLONIZE,0,0,0,v}
#define EPOVOS(v) {HE_POVOS_PRESTIGE,0,0,0,v}
#define EIDEO(a,v) {HE_IDEO,a,0,0,v}
#define EWMI(v) {HE_WORLD_MILIT,0,0,0,v}
#define MON NG_MONARQUIA
#define REP NG_REPUBLICA
#define TEO NG_TEOCRACIA
#define COMU ID_COMUNISTA
#define AUTO ID_AUTORITARIO
#define LIBE ID_LIBERAL

static const HistEvent HIST[]={
// ---- Era Napoleonica ----
{1801,1,1,2,"uniao_irlanda","Ato de União","Grã-Bretanha e Irlanda se fundem no Reino Unido.",{CX("GBR"),CX("IRL")},{EANX("GBR","IRL"),EPR("GBR",5)}},
{1801,4,12,1,"sikh","Império Sikh","Ranjit Singh é proclamado marajá do Punjab em Lahore.",{CX("SIK")},{EPR("SIK",20),ETR("SIK",20)}},
{1801,5,10,2,"berberesca","Guerra Berberesca","Trípoli exige tributo dos navios americanos e declara guerra.",{CX("TRP"),CX("USA")},{EWAR("TRP","USA")}},
{1801,9,12,3,"georgia","A Rússia anexa a Geórgia","O reino de Kartli-Kakheti é incorporado ao Império Russo.",{CX("RUS"),CX("GEO")},{EANX("RUS","GEO")}},
{1802,3,25,1,"amiens","Paz de Amiens","França e Grã-Bretanha assinam uma trégua frágil.",{CWAR("FRA","GBR")},{EPAZ("FRA","GBR")}},
{1802,6,1,6,"gia_long","Gia Long","Nguyễn Ánh vence os Tây Sơn e reunifica o Vietnã.",{CX("NGU"),CSTR("NGU","TSN",80)},{EUNI("NGU","TSN"),EBEC("NGU","VIE"),EPR("VIE",20)}},
{1803,4,30,2,"louisiana","Compra da Louisiana","Os Estados Unidos compram da França o vale do Mississippi.",{CX("USA"),CX("FRA")},{EGO("USA",-150),EGO("FRA",150),ECED("USA","NSP","LOU",10)}},
{1803,5,18,1,"amiens_fim","Fim da Paz de Amiens","A Grã-Bretanha volta à guerra contra Napoleão.",{CX("FRA"),CX("GBR")},{EWAR("GBR","FRA")}},
{1803,8,7,2,"anglo_maratha","Segunda Guerra Anglo-Maratha","A Companhia ataca a Confederação Maratha.",{CX("EIC"),CX("MAR")},{EWAR("EIC","MAR")}},
{1804,1,1,5,"haiti","Independência do Haiti","Dessalines proclama o primeiro Estado nascido de uma revolução de escravizados.",{CX("HAI")},{EIND("HAI"),EGOV("HAI",MON),EPR("HAI",30),EPAZ("FRA","HAI")}},
{1804,12,2,1,"napoleao","Napoleão Imperador","Napoleão Bonaparte coroa a si mesmo em Notre-Dame.",{CX("FRA")},{EGOV("FRA",MON),EPR("FRA",20),ETR("FRA",15)}},
{1805,6,10,2,"paz_tripoli","Tratado de Trípoli","Termina a guerra berberesca.",{CWAR("TRP","USA")},{EPAZ("TRP","USA"),EPR("USA",5)}},
{1805,10,21,1,"trafalgar","Trafalgar","A Royal Navy destrói a frota franco-espanhola.",{CX("GBR"),CWAR("FRA","GBR")},{EPR("GBR",20),EGO("FRA",-100),EGO("ESP",-100)}},
{1805,12,2,1,"austerlitz","Austerlitz","Napoleão vence Áustria e Rússia na batalha dos três imperadores.",{CWAR("FRA","HAB")},{EPR("FRA",25),ETR("HAB",-25),EPAZ("FRA","HAB")}},
{1806,1,10,2,"cabo","Os britânicos tomam o Cabo","Depois de Blaauwberg, a Colônia do Cabo passa à coroa britânica.",{CX("CAP"),CX("GBR")},{ESUB("CAP","GBR")}},
{1806,8,6,1,"fim_sacro","Fim do Sacro Império","Francisco II abdica: mil anos de Sacro Império chegam ao fim.",{CX("HAB")},{EPR("HAB",-15),EPR("FRA",10)}},
{1806,10,14,1,"jena","Jena-Auerstedt","O exército prussiano é esmagado pelos franceses.",{CX("PRU"),CX("FRA")},{EWAR("FRA","PRU"),ETR("PRU",-40),EPR("FRA",15)}},
{1807,3,25,1,"trafico","Abolição do tráfico","O Parlamento britânico proíbe o tráfico de pessoas escravizadas.",{CX("GBR")},{EPR("GBR",15),EHA("GBR",5)}},
{1807,7,7,1,"tilsit","Tratados de Tilsit","França, Rússia e Prússia redesenham o mapa da Europa.",{CX("FRA")},{EPAZ("FRA","RUS"),EPAZ("FRA","PRU"),EPR("FRA",10)}},
{1807,10,9,2,"reformas_prussia","Reformas prussianas","Stein e Hardenberg abolem a servidão; Scharnhorst refaz o exército.",{CX("PRU")},{EADD("PRU",TR_DISCIPLINA),ETR("PRU",20)}},
{1808,3,7,1,"corte_rio","A corte no Rio","Fugindo de Napoleão, a família real portuguesa chega ao Brasil.",{CX("POR"),CX("BRA")},{EPR("BRA",20),EGO("BRA",200),EWAR("FRA","POR")}},
{1808,5,2,1,"peninsular","Dos de Mayo","Madri se levanta contra Napoleão; começa a Guerra Peninsular.",{CX("ESP"),CX("FRA")},{EWAR("FRA","ESP"),EMI("ESP",10)}},
{1808,10,1,4,"sokoto","Califado de Sokoto","A reforma de Usman dan Fodio derruba Gobir e une os emirados hauçás.",{CX("HSA")},{EBEC("HSA","SOK"),EUNI("SOK","KNO"),EUNI("SOK","KAT"),EPR("SOK",20)}},
{1809,9,17,2,"finlandia","Tratado de Fredrikshamn","A Suécia cede a Finlândia ao Império Russo.",{CX("RUS"),CX("SWE")},{ECED("RUS","SWE","FIN",12)}},
{1810,3,1,3,"havai","Havaí unificado","Kaumualiʻi aceita Kamehameha: todas as ilhas sob um só reino.",{CX("HAW")},{EPR("HAW",20),ETR("HAW",30)}},
{1810,5,25,1,"maio","Revolução de Maio","Buenos Aires forma sua própria junta de governo.",{CX("RDP")},{EMI("RDP",15)}},
{1810,9,16,1,"grito","Grito de Dolores","O padre Hidalgo chama o povo da Nova Espanha à revolta.",{CX("NSP")},{EMI("NSP",20),ETR("NSP",-15)}},
{1811,5,14,4,"paraguai","Independência do Paraguai","Assunção rompe com Buenos Aires e com a Espanha.",{CN("PAR")},{ESPN("PAR","RDP",6)}},
{1812,6,18,1,"guerra1812","Guerra de 1812","Os Estados Unidos declaram guerra à Grã-Bretanha.",{CX("USA"),CX("GBR")},{EWAR("USA","GBR")}},
{1812,6,24,1,"russia1812","Invasão da Rússia","A Grande Armée cruza o rio Neman rumo a Moscou.",{CX("FRA"),CX("RUS")},{EWAR("FRA","RUS")}},
{1812,12,14,1,"retirada","A retirada de Moscou","O inverno russo destrói a Grande Armée.",{CWAR("FRA","RUS")},{ETR("FRA",-50),EPR("RUS",20)}},
{1813,10,5,1,"tecumseh","Batalha do Tâmisa","Tecumseh morre em combate; sua confederação se desfaz.",{CX("SHA")},{ETR("SHA",-40),EPR("SHA",10)}},
{1814,1,14,2,"kiel","Tratado de Kiel","A Dinamarca cede a Noruega à Suécia.",{CX("DAN"),CX("SWE")},{ECED("SWE","DAN","NOR",14)}},
{1814,12,24,1,"gante","Tratado de Gante","Termina a guerra anglo-americana, sem vencedores.",{CWAR("USA","GBR")},{EPAZ("USA","GBR")}},
// ---- Restauracao ----
{1815,4,10,1,"tambora","Erupção do Tambora","O maior vulcão da história recente escurece o céu do planeta.",{{0,0,0,0,0}},{EWHA(-5)}},
{1815,6,9,1,"viena","Congresso de Viena","As potências redesenham a Europa e buscam o equilíbrio.",{CX("HAB")},{EPAZALL,EPR("HAB",15),EPR("RUS",10),EPR("GBR",10)}},
{1815,6,18,1,"waterloo","Waterloo","Napoleão é derrotado pela última vez; os Bourbon voltam ao trono.",{CX("FRA")},{EGOV("FRA",MON),ETR("FRA",-30),EPR("GBR",15)}},
{1816,6,1,1,"sem_verao","O ano sem verão","Geadas em junho: colheitas falham na Europa e na América do Norte.",{{0,0,0,0,0}},{EWGO(-10),EWHA(-5)}},
{1816,7,9,8,"argentina","Independência da Argentina","O Congresso de Tucumán declara as Províncias Unidas livres.",{CX("RDP")},{EBEC("RDP","ARG"),EIND("ARG"),EPR("ARG",15)}},
{1817,11,6,13,"servia","Principado da Sérvia","Depois de duas revoltas, a Sérvia conquista autonomia.",{CX("OTT")},{ESPN("SRB","OTT",5)}},
{1818,2,12,8,"chile","Independência do Chile","O'Higgins e San Martín libertam o Chile.",{CN("CHL")},{ESPN("CHL","VPE",9)}},
{1818,6,3,5,"fim_maratha","Fim da Confederação Maratha","A Companhia derrota o último Peshwa.",{CX("MAR"),CSTR("EIC","MAR",200)},{EANX("EIC","MAR")}},
{1818,8,1,4,"zulu","Reino Zulu","Shaka reorganiza os regimentos e funda o reino zulu.",{CX("MTH")},{EBEC("MTH","ZUL"),EUNI("ZUL","NDW"),EPR("ZUL",15)}},
{1818,9,15,2,"diriyah","Queda de Diriyah","Tropas egípcias destroem o primeiro Estado saudita.",{CX("DRY"),CSTR("EGY","DRY",150)},{EANX("EGY","DRY")}},
{1819,2,6,2,"singapura","Fundação de Singapura","Raffles funda um porto britânico no estreito de Johor.",{CX("GBR"),CX("JOH")},{EGO("GBR",100),ECED("GBR","JOH","SIN",1)}},
{1819,12,17,8,"gran_colombia","Gran Colombia","Bolívar une Venezuela, Nova Granada e Quito.",{CX("NGR")},{EBEC("NGR","GCO"),EIND("GCO"),EPR("GCO",15)}},
{1821,1,1,3,"sequoyah","Silabário de Sequoyah","Os Tsalagi ganham uma escrita própria; a alfabetização explode.",{CX("TSA")},{EPR("TSA",15),EHA("TSA",10)}},
{1821,3,25,2,"grecia_revolta","Revolução grega","Os gregos se levantam contra o domínio otomano.",{CX("OTT")},{EMI("OTT",10),ETR("OTT",-10)}},
{1821,7,28,8,"peru","Independência do Peru","San Martín proclama a independência em Lima.",{CX("VPE")},{EBEC("VPE","PER"),EIND("PER")}},
{1821,9,27,8,"mexico","Independência do México","O Exército Trigarante entra na Cidade do México.",{CX("NSP")},{EBEC("NSP","MEX"),EIND("MEX"),EPR("MEX",15)}},
{1822,6,1,5,"lesoto","Moshoeshoe","Refugiados do Mfecane se reúnem na fortaleza de Thaba Bosiu.",{CN("SOT")},{ESPN("SOT",0,3)}},
{1822,9,7,10,"brasil","Independência do Brasil","Dom Pedro proclama a independência às margens do Ipiranga.",{CX("BRA")},{EIND("BRA"),EGOV("BRA",MON),EPR("BRA",20)}},
{1824,3,5,1,"birmania1","Primeira Guerra Anglo-Birmanesa","A Companhia invade a Birmânia pela costa.",{CX("EIC"),CX("BRM")},{EWAR("EIC","BRM")}},
{1825,8,6,6,"bolivia","Independência da Bolívia","O Alto Peru vira república em honra a Bolívar.",{CN("BOL")},{ESPN("BOL",0,7)}},
{1828,8,27,4,"uruguai","Independência do Uruguai","A Banda Oriental vira Estado entre Brasil e Argentina.",{CN("URU")},{ESPN("URU",0,3)}},
{1830,2,3,10,"grecia","Independência da Grécia","O Protocolo de Londres reconhece a Grécia livre.",{CX("OTT")},{ESPN("GRE","OTT",6)}},
{1830,5,28,1,"remocao","Lei de Remoção Indígena","O Congresso americano autoriza a expulsão dos povos do sudeste.",{CX("USA")},{EMI("TSA",20),EMI("MUS",20),EMI("SEM",20)}},
{1830,7,5,2,"argel","França em Argel","Os franceses tomam Argel; começa a conquista da Argélia.",{CX("FRA"),CX("ALG")},{EIND("ALG"),EWAR("FRA","ALG")}},
{1830,10,4,10,"belgica","Independência da Bélgica","Bruxelas se separa do Reino Unido dos Países Baixos.",{CX("NED")},{ESPN("BEL","NED",3)}},
{1833,8,28,1,"abolicao_gbr","Abolição no Império Britânico","A escravidão é abolida em todas as colônias britânicas.",{CX("GBR")},{EPR("GBR",15),EHA("GBR",5)}},
{1834,1,1,3,"zollverein","Zollverein","A Prússia une os Estados alemães numa união aduaneira.",{CX("PRU")},{EPR("PRU",20),EADD("PRU",TR_ENTREPOSTO),EGO("PRU",200)}},
{1835,12,28,2,"seminole2","Segunda Guerra Seminole","Os Seminole recusam a remoção e defendem os pântanos.",{CX("USA"),CX("SEM")},{EWAR("USA","SEM")}},
{1836,3,2,6,"texas","Independência do Texas","Colonos do Texas rompem com o México.",{CX("MEX")},{ESPN("TEX","MEX",9)}},
{1838,5,26,5,"trilha","Trilha das Lágrimas","Os Tsalagi são expulsos à força para o oeste; milhares morrem no caminho.",{CX("TSA"),CSTR("USA","TSA",500)},{EREL("TSA","USA","OKL",4),EPR("TSA",10),EMI("TSA",20)}},
{1839,9,4,2,"opio","Primeira Guerra do Ópio","Navios britânicos atacam a China para impor o comércio de ópio.",{CX("GBR"),CX("QNG")},{EWAR("GBR","QNG")}},
{1839,11,3,1,"tanzimat","Tanzimat","O sultão anuncia reformas que modernizam o império.",{CX("OTT")},{EGO("OTT",200),EMI("OTT",-10)}},
{1840,2,6,2,"waitangi","Tratado de Waitangi","Chefes māori e a coroa britânica assinam um tratado — em duas versões diferentes.",{CX("NGP")},{EPR("NGP",10),EPR("GBR",5)}},
{1840,12,1,2,"zanzibar","Capital em Zanzibar","O sultão de Omã muda sua corte para Zanzibar.",{CX("OMA")},{EGO("OMA",150),EPR("OMA",5)}},
{1842,8,29,3,"nanquim","Tratado de Nanquim","A China abre portos e cede Hong Kong.",{CWAR("GBR","QNG")},{EPAZ("GBR","QNG"),EDEL("QNG",TR_ISOLAMENTO),EGO("QNG",-200),ECED("GBR","QNG","HKG",1),EPR("QNG",-15)}},
{1845,12,11,2,"anglo_sikh","Primeira Guerra Anglo-Sikh","A Companhia e o Império Sikh se enfrentam.",{CX("EIC"),CX("SIK")},{EWAR("EIC","SIK")}},
{1846,4,25,2,"mex_usa","Guerra Mexicano-Americana","Os Estados Unidos invadem o México.",{CX("USA"),CX("MEX")},{EWAR("USA","MEX")}},
// ---- Primavera dos Povos ----
{1847,7,26,5,"liberia","Independência da Libéria","Colonos afro-americanos fundam a primeira república da África.",{CN("LIB")},{ESPN("LIB",0,3)}},
{1847,7,30,4,"castas","Guerra de Castas","Os maias do Yucatán se levantam e fundam seu próprio Estado.",{CX("MEX")},{ESPN("MAY","MEX",3)}},
{1848,2,2,3,"guadalupe","Tratado de Guadalupe Hidalgo","O México perde metade do território para os Estados Unidos.",{CWAR("USA","MEX"),CSTR("USA","MEX",120)},{EPAZ("USA","MEX"),ECED("USA","MEX","CAL",18),ECED("USA","MEX","NMX",10)}},
{1848,2,21,1,"manifesto","Manifesto Comunista","Marx e Engels publicam um panfleto que vai assombrar o século.",{{0,0,0,0,0}},{EWMI(5)}},
{1848,2,22,1,"1848","Primavera dos Povos","Revoluções sacodem Paris, Viena, Berlim, Milão e Budapeste.",{{0,0,0,0,0}},{EWHA(-10),EGOV("FRA",REP),EMI("HAB",20)}},
{1849,3,29,4,"punjab","Anexação do Punjab","A Companhia anexa o Império Sikh.",{CX("SIK"),CSTR("EIC","SIK",150)},{EANX("EIC","SIK")}},
{1850,12,11,2,"taiping","Rebelião Taiping","A guerra civil mais sangrenta do século devasta a China.",{CX("QNG")},{ETR("QNG",-30),EGO("QNG",-200),EMI("QNG",20)}},
{1852,1,17,4,"transvaal","República do Transvaal","Bôeres fundam a República Sul-Africana além do rio Vaal.",{CN("TRV")},{ESPN("TRV",0,6)}},
{1852,12,2,2,"segundo_imperio","Segundo Império","Luís Napoleão se proclama imperador dos franceses.",{CX("FRA")},{EGOV("FRA",MON),EPR("FRA",10)}},
{1853,7,8,2,"perry","Os navios negros","O comodoro Perry exige que o Japão abra seus portos.",{CX("JAP")},{EDEL("JAP",TR_ISOLAMENTO),EGO("JAP",100),EMI("JAP",15)}},
{1853,10,16,1,"crimeia","Guerra da Crimeia","Rússia contra Otomanos, Grã-Bretanha e França.",{CX("RUS"),CX("OTT")},{EWAR("RUS","OTT"),EWAR("GBR","RUS"),EWAR("FRA","RUS")}},
{1854,2,23,4,"orange","Estado Livre de Orange","Bôeres fundam uma república entre os rios Orange e Vaal.",{CN("ORA")},{ESPN("ORA",0,5)}},
{1855,2,11,3,"tewodros","Tewodros II","Kassa Hailu é coroado e reunifica a Etiópia.",{CX("ETH")},{EDEL("ETH",TR_CRISE),EPR("ETH",15),ETR("ETH",30)}},
{1856,3,30,2,"paris1856","Paz de Paris","Termina a Guerra da Crimeia.",{CWAR("RUS","OTT")},{EPAZ("RUS","OTT"),EPAZ("GBR","RUS"),EPAZ("FRA","RUS"),EPR("RUS",-15)}},
{1857,5,10,1,"rebeliao1857","Rebelião de 1857","Soldados indianos se levantam contra a Companhia.",{CX("EIC")},{EMI("EIC",30),ETR("EIC",-20),EWAR("EIC","MUG")}},
{1858,5,2,3,"kingitanga","Kīngitanga","Os iwi do Waikato elegem um rei māori para defender suas terras.",{CX("TAI")},{EADD("TAI",TR_CONFEDERACAO),EPR("TAI",10)}},
{1858,8,2,3,"raj","Raj Britânico","A coroa assume a Índia; acaba o Império Mogol.",{CX("EIC")},{EBEC("EIC","RAJ"),EANX("RAJ","MUG"),ESUB("HYD","RAJ")}},
{1859,8,27,1,"petroleo","Primeiro poço de petróleo","Em Titusville, começa a era do petróleo.",{{0,0,0,0,0}},{EWGO(5),EPR("USA",5)}},
{1861,3,3,2,"servos","Emancipação dos servos","Alexandre II liberta 23 milhões de servos.",{CX("RUS")},{EDEL("RUS",TR_ABSOLUTISMO),EADD("RUS",TR_REFORMADOR),EHA("RUS",10)}},
{1861,3,17,10,"italia","Reino da Itália","Vítor Emanuel II é proclamado rei de uma Itália unida.",{CX("SAR")},{EBEC("SAR","ITA"),EUNI("ITA","NAP"),EPR("ITA",25)}},
{1861,4,12,2,"guerra_civil","Guerra Civil Americana","Os estados do sul se separam; Fort Sumter é bombardeado.",{CX("USA"),CN("CSA")},{ESPN("CSA","USA",12),EWAR("USA","CSA")}},
{1861,11,1,3,"araucania","Ocupação da Araucanía","O Chile avança sobre Wallmapu.",{CX("CHL"),CX("MAP")},{EWAR("CHL","MAP")}},
{1862,1,1,2,"mexico_frances","Intervenção francesa","A França invade o México e impõe um imperador.",{CX("FRA"),CX("MEX")},{EWAR("FRA","MEX")}},
{1864,1,1,3,"longa_marcha","A Longa Marcha","Os Diné são deportados para Bosque Redondo.",{CX("DIN"),CSTR("USA","DIN",500)},{ETR("DIN",-40),EGO("DIN",-50),EMI("DIN",20)}},
{1864,11,12,2,"guerra_paraguai","Guerra do Paraguai","O Paraguai enfrenta Brasil, Argentina e Uruguai.",{CX("PAR")},{EWAR("PAR","BRA"),EWAR("PAR","ARG"),EWAR("PAR","URU")}},
{1865,4,9,4,"appomattox","Appomattox","A Confederação se rende; a escravidão é abolida nos EUA.",{CX("CSA"),CSTR("USA","CSA",120)},{EUNI("USA","CSA"),EPR("USA",10)}},
{1867,3,15,2,"austro_hungria","Compromisso de 1867","Nasce a monarquia dual da Áustria-Hungria.",{CX("HAB")},{EBEC("HAB","AUH")}},
{1867,3,30,2,"alasca","Compra do Alasca","A Rússia vende o Alasca aos Estados Unidos.",{CX("USA"),CX("RAM")},{EANX("USA","RAM"),EGO("USA",-100),EGO("RUS",100)}},
{1867,6,19,2,"fim_intervencao","Fim da intervenção francesa","Os mexicanos derrotam o império imposto pela França.",{CWAR("FRA","MEX")},{EPAZ("FRA","MEX"),EPR("MEX",15)}},
{1867,7,1,3,"canada","Confederação do Canadá","Nasce o Domínio do Canadá.",{CX("BNA")},{EBEC("BNA","CAN")}},
{1868,1,3,2,"meiji","Restauração Meiji","O imperador volta ao centro do poder; o Japão se moderniza.",{CX("JAP")},{EDEL("JAP",TR_ISOLAMENTO),EDEL("JAP",TR_BUROCRACIA),EADD("JAP",TR_REFORMADOR),EADD("JAP",TR_INDUSTRIAL),EPR("JAP",20)}},
{1868,6,1,4,"retorno_dine","Tratado de Bosque Redondo","Os Diné voltam para sua terra entre as quatro montanhas.",{CFIRED("longa_marcha"),CX("DIN")},{EPR("DIN",15),EHA("DIN",10)}},
{1869,11,17,1,"suez","Canal de Suez","Abre-se o atalho entre o Mediterrâneo e o Índico.",{CX("EGY")},{EGO("EGY",300),EPR("EGY",15),EWGO(5)}},
{1870,3,1,3,"fim_paraguai","Fim da Guerra do Paraguai","O Paraguai sai devastado da guerra.",{CX("PAR")},{EPAZ("PAR","BRA"),EPAZ("PAR","ARG"),EPAZ("PAR","URU"),ETR("PAR",-60)}},
{1870,7,19,1,"franco_prussiana","Guerra Franco-Prussiana","A Prússia e seus aliados enfrentam a França.",{CX("PRU"),CX("FRA")},{EWAR("PRU","FRA")}},
{1870,9,20,5,"roma","Roma capital","As tropas italianas entram em Roma; fim dos Estados Papais.",{CX("ITA"),CX("PAP")},{EUNI("ITA","PAP")}},
// ---- Imperialismo ----
{1871,1,18,5,"alemanha","Império Alemão","Guilherme I é proclamado imperador em Versalhes.",{CX("PRU")},{EBEC("PRU","GER"),EUNI("GER","BAV"),EUNI("GER","SAX"),EUNI("GER","WUR"),EUNI("GER","HAN"),EPR("GER",20)}},
{1871,3,18,1,"comuna","Comuna de Paris","Operários governam Paris por dois meses antes de serem esmagados.",{CX("FRA")},{EMI("FRA",15),ETR("FRA",-10),EWMI(3)}},
{1871,5,10,2,"frankfurt","Tratado de Frankfurt","A França perde a Alsácia-Lorena e vira república.",{CWAR("GER","FRA")},{EPAZ("GER","FRA"),EGO("FRA",-300),ECED("GER","FRA","ALS",3),EGOV("FRA",REP)}},
{1873,3,26,2,"aceh","Guerra de Aceh","Os Países Baixos atacam o sultanato de Aceh.",{CX("NED"),CX("ACE")},{EWAR("NED","ACE")}},
{1876,2,26,2,"ganghwa","Tratado de Ganghwa","O Japão força a Coreia a abrir seus portos.",{CX("KOR")},{EDEL("KOR",TR_ISOLAMENTO),EMI("KOR",10)}},
{1876,6,25,1,"little_bighorn","Little Bighorn","Lakota e Cheyenne derrotam a 7ª Cavalaria.",{CX("LAK"),CX("USA")},{EPR("LAK",20),EWAR("USA","LAK")}},
{1878,7,13,2,"berlim1878","Congresso de Berlim","Sérvia, Montenegro e Romênia ganham independência.",{CX("OTT")},{EIND("SRB"),ESPN("ROM","OTT",5),EPR("OTT",-15)}},
{1879,1,22,1,"isandlwana","Isandlwana","O exército zulu derrota os britânicos.",{CX("ZUL"),CX("GBR")},{EWAR("GBR","ZUL"),EPR("ZUL",25),ETR("GBR",-10)}},
{1879,3,27,2,"ryukyu","Anexação de Ryukyu","O Japão anexa o reino de Ryukyu.",{CX("JAP"),CX("RYU")},{EANX("JAP","RYU")}},
{1879,4,1,6,"deserto","Conquista do Deserto","O exército argentino avança sobre a Patagônia.",{CX("AON"),CSTR("ARG","AON",500)},{EANX("ARG","AON")}},
{1879,4,5,2,"pacifico","Guerra do Pacífico","Chile contra Peru e Bolívia pelo salitre do deserto.",{CX("CHL")},{EWAR("CHL","PER"),EWAR("CHL","BOL")}},
{1879,7,4,3,"ulundi","Ulundi","Os britânicos queimam a capital zulu.",{CWAR("GBR","ZUL"),CSTR("GBR","ZUL",300)},{EPAZ("GBR","ZUL"),ESUB("ZUL","GBR")}},
{1882,9,13,2,"egito_britanico","Ocupação britânica do Egito","A Grã-Bretanha ocupa o Egito.",{CX("EGY"),CX("GBR")},{ESUB("EGY","GBR")}},
{1883,1,1,8,"wallmapu","Ocupação de Wallmapu","O Chile ocupa Wallmapu; os Mapuche resistem em suas comunidades.",{CX("MAP"),CSTR("CHL","MAP",500)},{EANX("CHL","MAP")}},
{1884,10,20,3,"ancon","Tratado de Ancón","O Chile fica com o deserto do salitre.",{CWAR("CHL","PER")},{EPAZ("CHL","PER"),EPAZ("CHL","BOL"),ECED("CHL","BOL","ATA",4),ECED("CHL","PER","TAR",3)}},
{1884,11,15,1,"berlim1884","Conferência de Berlim","As potências europeias dividem a África entre si.",{{0,0,0,0,0}},{EPR("GBR",5),EPR("FRA",5),EWGO(3)}},
{1886,9,4,3,"geronimo","Rendição de Gerônimo","Termina a última resistência armada dos Ndé.",{CX("NDE"),CSTR("USA","NDE",500)},{EANX("USA","NDE")}},
{1888,5,13,1,"lei_aurea","Lei Áurea","A princesa Isabel assina o fim da escravidão no Brasil.",{CX("BRA")},{EPR("BRA",15),EHA("BRA",10)}},
{1889,11,15,1,"republica_br","Proclamação da República","O Império cai; o Brasil vira república.",{CX("BRA")},{EGOV("BRA",REP)}},
{1890,12,29,5,"wounded_knee","Wounded Knee","Tropas americanas matam centenas de Lakota; termina a resistência armada nas Planícies.",{CX("LAK"),CSTR("USA","LAK",500)},{EANX("USA","LAK")}},
{1893,1,17,2,"havai_golpe","Derrubada do Reino do Havaí","Fazendeiros apoiados por fuzileiros americanos depõem a rainha Liliʻuokalani.",{CX("HAW"),CSTR("USA","HAW",300)},{ESUB("HAW","USA")}},
{1894,8,1,1,"sino_japonesa","Primeira Guerra Sino-Japonesa","Japão e China disputam a Coreia.",{CX("JAP"),CX("QNG")},{EWAR("JAP","QNG")}},
{1895,4,17,2,"shimonoseki","Tratado de Shimonoseki","A China cede Taiwan ao Japão.",{CWAR("JAP","QNG")},{EPAZ("JAP","QNG"),ECED("JAP","QNG","TWN",3),EPR("JAP",20),EGO("QNG",-300)}},
{1895,12,1,3,"madagascar","Conquista de Madagascar","A França invade Imerina e depõe a rainha Ranavalona III.",{CX("MER"),CSTR("FRA","MER",300)},{EANX("FRA","MER")}},
{1896,3,1,1,"adwa","Batalha de Adwa","Menelik II e Taytu Betul derrotam a Itália e garantem a independência da Etiópia.",{CX("ETH"),CX("ITA"),CSTR("ETH","ITA",30)},{EPR("ETH",40),EPR("ITA",-20),EPAZ("ITA","ETH")}},
{1898,4,25,1,"hispano_americana","Guerra Hispano-Americana","Estados Unidos e Espanha disputam Cuba e as Filipinas.",{CX("USA"),CX("ESP")},{EWAR("USA","ESP")}},
{1898,8,12,2,"havai_anexacao","Anexação do Havaí","Os Estados Unidos anexam o Havaí.",{CSUB("HAW","USA")},{EANX("USA","HAW")}},
{1898,12,10,2,"paris1898","Tratado de Paris","A Espanha perde Cuba, Porto Rico e as Filipinas.",{CWAR("USA","ESP")},{EPAZ("USA","ESP"),ESUB("CUB","USA"),ESUB("PHI","USA"),EPR("ESP",-20)}},
{1899,10,11,1,"boer","Guerra dos Bôeres","A Grã-Bretanha ataca as repúblicas bôeres.",{CX("GBR")},{EWAR("GBR","TRV"),EWAR("GBR","ORA")}},
{1900,6,20,1,"boxers","Levante dos Boxers","Um levante contra estrangeiros sacode o norte da China.",{CX("QNG")},{ETR("QNG",-20),EGO("QNG",-200)}},
{1902,5,20,2,"cuba","República de Cuba","Cuba se torna independente.",{CSUB("CUB","USA")},{EIND("CUB")}},
{1902,5,31,3,"vereeniging","Tratado de Vereeniging","As repúblicas bôeres viram colônias britânicas.",{CX("TRV"),CSTR("GBR","TRV",150)},{EANX("GBR","TRV"),EANX("GBR","ORA")}},
{1904,2,8,1,"russo_japonesa","Guerra Russo-Japonesa","Japão e Rússia disputam a Manchúria.",{CX("JAP"),CX("RUS")},{EWAR("JAP","RUS")}},
{1905,6,7,3,"noruega","Independência da Noruega","A Noruega dissolve a união com a Suécia.",{CN("NOR")},{ESPN("NOR","SWE",10)}},
{1905,9,5,2,"portsmouth","Tratado de Portsmouth","Pela primeira vez uma potência asiática vence uma europeia.",{CWAR("JAP","RUS")},{EPAZ("JAP","RUS"),EPR("JAP",30),EPR("RUS",-20),EMI("RUS",20)}},
{1910,5,31,3,"uniao_sul","União Sul-Africana","Os territórios do sul se unem sob a coroa.",{CX("CAP")},{EBEC("CAP","ZAF")}},
{1910,8,29,5,"coreia","Anexação da Coreia","O Japão anexa a Coreia.",{CX("KOR"),CSTR("JAP","KOR",300)},{EANX("JAP","KOR")}},
{1910,10,5,1,"rep_portugal","República Portuguesa","A monarquia cai em Lisboa depois de dois dias de revolta.",{CX("POR")},{EGOV("POR",REP)}},
{1910,11,20,1,"rev_mexicana","Revolução Mexicana","Uma década de revolução sacode o México.",{CX("MEX")},{ETR("MEX",-30),EMI("MEX",20)}},
{1911,10,10,2,"xinhai","Revolução Xinhai","Cai a dinastia Qing; nasce a República da China.",{CX("QNG")},{EBEC("QNG","CHN"),EPR("CHN",10)}},
{1912,10,8,1,"balcas","Guerras Balcânicas","Os Estados balcânicos atacam os Otomanos.",{CX("OTT")},{EWAR("SRB","OTT"),EWAR("GRE","OTT"),ETR("OTT",-20)}},
// ---- Guerras Mundiais ----
{1914,7,28,1,"grande_guerra","A Grande Guerra","Um tiro em Sarajevo arrasta as potências para a guerra.",{{0,0,0,0,0}},{EWAR("AUH","SRB"),EWAR("GER","RUS"),EWAR("GER","FRA"),EWAR("GBR","GER"),EWAR("OTT","RUS")}},
{1917,11,7,1,"outubro","Revolução de Outubro","Os bolcheviques tomam o poder em Petrogrado.",{CX("RUS")},{EIDEO("RUS",COMU),EPAZ("RUS","GER"),ETR("RUS",-40),ESPN("FIN","RUS",12),EWMI(8)}},
{1918,11,11,1,"armisticio","Armistício","Termina a Grande Guerra; impérios desabam.",{{0,0,0,0,0}},{EPAZALL,ESPN("POL",0,8),ESPN("CZE","AUH",5),ESPN("YUG","AUH",6),EGOV("GER",REP),EBEC("AUH","AUT")}},
{1918,11,16,2,"hungria","Hungria independente","A Hungria se separa da Áustria.",{CX("AUT")},{ESPN("HUN","AUT",5)}},
{1919,6,28,1,"versalhes","Tratado de Versalhes","A Alemanha paga reparações e perde territórios.",{CX("GER")},{EGO("GER",-400),ETR("GER",-40),EPR("FRA",10)}},
{1922,10,28,2,"marcha_roma","Marcha sobre Roma","Mussolini toma o poder; nasce o primeiro regime fascista.",{CX("ITA")},{EIDEO("ITA",AUTO),ETR("ITA",15)}},
{1922,12,6,3,"irlanda","Estado Livre Irlandês","Depois de séculos, a Irlanda se governa.",{CN("IRF")},{ESPN("IRF","GBR",4)}},
{1922,12,30,2,"urss","União Soviética","Os bolcheviques fundam a URSS.",{CX("RUS")},{EBEC("RUS","URS")}},
{1923,10,29,2,"turquia","República da Turquia","Atatürk funda uma república nas ruínas do império.",{CX("OTT")},{EBEC("OTT","TUR"),EGOV("TUR",REP)}},
{1929,10,24,1,"depressao","Grande Depressão","A quebra da bolsa de Nova York arrasta o mundo.",{{0,0,0,0,0}},{EWGO(-30),EWHA(-10)}},
{1931,4,14,1,"rep_espanha","Segunda República Espanhola","O rei parte para o exílio; a Espanha vira república.",{CX("ESP")},{EGOV("ESP",REP)}},
{1933,1,30,1,"nazismo","O nazismo no poder","Hitler é nomeado chanceler; a democracia alemã desmorona em semanas.",{CX("GER")},{EIDEO("GER",AUTO),ETR("GER",20),EMI("GER",-15)}},
{1933,3,19,2,"estado_novo","Estado Novo","Salazar impõe uma ditadura corporativa em Portugal.",{CX("POR")},{EIDEO("POR",AUTO)}},
{1935,10,3,1,"etiopia_italia","Itália invade a Etiópia","A Itália fascista ataca o último império africano livre.",{CX("ITA"),CX("ETH")},{EWAR("ITA","ETH")}},
{1936,5,5,2,"adis","Ocupação de Adis Abeba","A Itália ocupa a capital etíope; Hailé Selassié vai ao exílio.",{CWAR("ITA","ETH"),CSTR("ITA","ETH",200)},{ESUB("ETH","ITA"),EPAZ("ITA","ETH")}},
{1936,7,17,1,"espanha_civil","Guerra Civil Espanhola","Um golpe militar mergulha a Espanha em guerra civil.",{CX("ESP")},{ETR("ESP",-40),EGO("ESP",-200),EMI("ESP",20)}},
{1937,7,7,1,"china_japao","Japão invade a China","Começa a guerra sino-japonesa.",{CX("JAP"),CX("CHN")},{EWAR("JAP","CHN")}},
{1939,4,1,1,"franco","Fim da Guerra Civil Espanhola","Franco vence e instala uma ditadura que vai durar 36 anos.",{CX("ESP")},{EIDEO("ESP",AUTO),EHA("ESP",-10)}},
{1939,9,1,1,"ww2","Segunda Guerra Mundial","A Alemanha invade a Polônia.",{CX("GER")},{EWAR("GER","POL"),EWAR("GBR","GER"),EWAR("FRA","GER")}},
{1941,5,5,2,"haile","Hailé Selassié volta","A Etiópia recupera a independência.",{CSUB("ETH","ITA")},{EIND("ETH"),EPR("ETH",15)}},
{1941,6,22,1,"barbarossa","Operação Barbarossa","A Alemanha invade a União Soviética.",{CX("GER"),CX("URS")},{EWAR("GER","URS")}},
{1941,12,7,1,"pearl","Pearl Harbor","O Japão ataca a frota americana no Havaí.",{CX("JAP"),CX("USA")},{EWAR("JAP","USA"),EWAR("USA","GER")}},
{1945,5,8,1,"fim_europa","Fim da guerra na Europa","A Alemanha se rende.",{CX("GER")},{EPAZ("GER","URS"),EPAZ("GER","GBR"),EPAZ("GER","FRA"),EPAZ("GER","USA"),ETR("GER",-80),EGOV("GER",REP)}},
// ---- Guerra Fria ----
{1945,8,15,1,"fim_pacifico","Rendição do Japão","Depois de Hiroshima e Nagasaki, o Japão se rende.",{CX("JAP")},{EPAZ("JAP","USA"),EPAZ("JAP","CHN"),ETR("JAP",-80),ECED("CHN","JAP","TWN",3),ESPN("KOR","JAP",7)}},
{1945,10,24,1,"onu","Nações Unidas","51 países fundam a ONU para evitar outra guerra mundial.",{{0,0,0,0,0}},{EWHA(5)}},
{1946,6,2,1,"rep_italia","República Italiana","Um plebiscito acaba com a monarquia de Saboia.",{CX("ITA")},{EGOV("ITA",REP),EIDEO("ITA",LIBE)}},
{1947,8,15,3,"india","Independência da Índia","À meia-noite, a Índia desperta para a liberdade; a partição cria o Paquistão.",{CX("RAJ")},{EBEC("RAJ","IND"),EIND("IND"),ESPN("PAK","IND",8)}},
{1948,5,14,2,"israel","Criação de Israel","O Estado de Israel é proclamado; começa a guerra com os vizinhos árabes.",{CN("ISR")},{ESPN("ISR",0,2)}},
{1949,10,1,2,"prc","República Popular da China","Mao Zedong proclama a república em Pequim.",{CX("CHN")},{EBEC("CHN","PRC"),EIDEO("PRC",COMU)}},
{1949,12,27,3,"indonesia","Independência da Indonésia","Os Países Baixos reconhecem a Indonésia.",{CX("DEI")},{EBEC("DEI","IDN"),EIND("IDN")}},
{1957,3,6,3,"gana","Independência de Gana","Kwame Nkrumah proclama a independência.",{CN("GHA")},{ESPN("GHA",0,4)}},
{1957,10,4,1,"sputnik","Sputnik","O primeiro satélite artificial orbita a Terra.",{CX("URS")},{EPR("URS",25)}},
{1959,1,1,1,"rev_cubana","Revolução Cubana","Os barbudos de Sierra Maestra entram em Havana.",{CX("CUB")},{EIND("CUB"),EIDEO("CUB",COMU),EPR("CUB",10)}},
{1960,6,30,1,"ano_africa","O Ano da África","Dezessete países africanos conquistam a independência.",{{0,0,0,0,0}},{EDECOL(CONT_AFRICA)}},
{1962,7,5,3,"argelia","Independência da Argélia","Depois de oito anos de guerra, a Argélia é livre.",{CN("DZA")},{ESPN("DZA",0,8)}},
{1962,10,16,1,"misseis","Crise dos Mísseis","O mundo passa treze dias à beira da guerra nuclear.",{{0,0,0,0,0}},{EWHA(-5)}},
{1963,12,12,3,"quenia","Independência do Quênia","Uhuru: o Quênia é livre.",{CN("KEN")},{ESPN("KEN",0,6)}},
{1964,4,1,1,"golpe64","Golpe de 1964","Militares derrubam o governo e instalam uma ditadura no Brasil.",{CX("BRA")},{EIDEO("BRA",AUTO),EHA("BRA",-8)}},
{1969,7,20,1,"lua","Pessoa na Lua","Neil Armstrong pisa na Lua.",{CX("USA")},{EPR("USA",40)}},
{1973,9,11,1,"golpe_chile","Golpe no Chile","Tanques cercam o Palácio de La Moneda.",{CX("CHL")},{EIDEO("CHL",AUTO),EHA("CHL",-10)}},
{1973,10,17,1,"petroleo73","Crise do petróleo","O preço do petróleo quadruplica.",{{0,0,0,0,0}},{EWGO(-15)}},
{1974,4,25,1,"cravos","Revolução dos Cravos","Uma revolução sem sangue derruba a ditadura portuguesa.",{CX("POR")},{EGOV("POR",REP),EIDEO("POR",LIBE),EHA("POR",10)}},
{1975,6,25,3,"mocambique","Independência de Moçambique","Moçambique é livre depois de cinco séculos.",{CN("MOZ")},{ESPN("MOZ",0,7)}},
{1975,11,11,3,"angola","Independência de Angola","Angola é livre; começa uma longa guerra civil.",{CN("AGO")},{ESPN("AGO",0,8)}},
{1976,3,24,1,"golpe_arg","Golpe na Argentina","Uma junta militar toma o poder em Buenos Aires.",{CX("ARG")},{EIDEO("ARG",AUTO)}},
{1979,2,11,1,"ira","Revolução Iraniana","Uma revolução derruba o xá e funda uma república islâmica.",{CX("QAJ")},{EGOV("QAJ",TEO)}},
{1983,12,10,1,"dem_arg","Democracia na Argentina","Depois da junta, os argentinos voltam às urnas.",{CX("ARG")},{EIDEO("ARG",LIBE),EHA("ARG",8)}},
{1985,3,15,2,"redemocratizacao","Redemocratização do Brasil","Termina a ditadura militar.",{CX("BRA")},{EGOV("BRA",REP),EIDEO("BRA",LIBE),EHA("BRA",10)}},
{1988,10,5,1,"constituicao88","Constituição de 1988","O Brasil reconhece os direitos originários dos povos indígenas.",{CX("BRA")},{EPR("BRA",10),EPR("MDK",10),EPR("KAY",10),EPR("YAN",10)}},
{1989,11,9,1,"muro","Queda do Muro de Berlim","O muro que dividia a Europa cai.",{{0,0,0,0,0}},{EWHA(5)}},
// ---- Globalizacao ----
{1989,12,1,1,"leste","Outono das Nações","Polônia, Hungria e Tchecoslováquia derrubam seus regimes de partido único.",{{0,0,0,0,0}},{EIDEO("POL",LIBE),EIDEO("HUN",LIBE),EIDEO("CZE",LIBE)}},
{1990,3,11,1,"dem_chile","Fim da ditadura no Chile","Pinochet deixa o poder depois do plebiscito.",{CX("CHL")},{EIDEO("CHL",LIBE),EHA("CHL",8)}},
{1991,12,26,2,"fim_urss","Fim da União Soviética","A URSS se dissolve em quinze países.",{CX("URS")},{EBEC("URS","RUS"),EIDEO("RUS",LIBE),ESPN("UKR","RUS",8),ESPN("KAZ","RUS",14)}},
{1992,2,7,1,"maastricht","Tratado de Maastricht","Nasce a União Europeia.",{{0,0,0,0,0}},{EPR("FRA",5),EPR("GER",5)}},
{1994,5,10,2,"mandela","Mandela presidente","Fim do apartheid: Nelson Mandela assume a presidência.",{CX("ZAF")},{EIND("ZAF"),EPR("ZAF",30),EHA("ZAF",15)}},
{1997,7,1,1,"hong_kong","Hong Kong volta à China","Termina o domínio britânico em Hong Kong.",{CX("PRC")},{ECED("PRC","GBR","HKG",1)}},
{2001,9,11,1,"11set","11 de Setembro","Atentados terroristas matam quase três mil pessoas nos Estados Unidos.",{CX("USA")},{EWAR("USA","AFG"),EMI("USA",10)}},
{2007,9,13,1,"undrip","Direitos dos Povos Indígenas","A ONU adota a Declaração sobre os Direitos dos Povos Indígenas.",{{0,0,0,0,0}},{EPOVOS(15)}},
{2008,9,15,1,"crise2008","Crise financeira","A quebra de um banco de Nova York derruba a economia mundial.",{{0,0,0,0,0}},{EWGO(-20)}},
{2011,7,9,3,"sudao_sul","Independência do Sudão do Sul","Nasce o país mais jovem do mundo.",{CN("SSD")},{ESPN("SSD",0,5)}},
{2016,6,23,1,"brexit","Referendo do Brexit","O Reino Unido vota para deixar a União Europeia.",{CX("GBR")},{EPR("GBR",-5),EMI("GBR",10)}},
{2020,3,11,1,"covid","Pandemia de COVID-19","Um novo vírus se espalha pelo mundo.",{{0,0,0,0,0}},{EWGO(-15),EWHA(-10)}},
{2022,2,24,1,"ucrania","Invasão da Ucrânia","A Rússia invade a Ucrânia.",{CX("RUS"),CX("UKR")},{EWAR("RUS","UKR")}},
};
#define HIST_N ((int)(sizeof(HIST)/sizeof(HIST[0])))

// Locais nomeados (efeitos CEDE/RELOCATE) que nao sao capital de nacao.
typedef struct { const char *tag; s8 lat; s16 lon; } HLoc;
static const HLoc LOC[]={
 {"LOU",34,-92},{"SIN",1,104},{"OKL",36,-96},{"HKG",22,114},{"CAL",37,-120},
 {"NMX",35,-106},{"ALS",48,7},{"TWN",24,121},{"ATA",-23,-70},{"TAR",-20,-69},
};

// ===================== eventos aleatorios =====================
enum {
 RQ_COAST=1, RQ_WAR=2, RQ_PEACE=4, RQ_MON=8, RQ_REP=16, RQ_POVO=32,
 RQ_TROPICS=64, RQ_NORTH=128, RQ_FAITH=256, RQ_SEA=512, RQ_POOR=1024,
 RQ_RICH=2048, RQ_UNSTABLE=4096,
};
typedef struct {
 const char *title, *body;
 u8 eraMin, eraMax; u16 req;
 HOp eff[3];   // a = NULL: o proprio pais (decisao: opcao A)
} RndEvent;
// Decisao: evento com 2 opcoes (o jogador escolhe no despacho; a IA decide
// pelo temperamento). Indice global = RND_N + k.
typedef struct { RndEvent e; const char *optA, *optB; HOp effB[3]; } RndDecision;
#define S(op,v) {op,0,0,0,v}
#define E0 ERA_NAPOLEONICA
#define E1 ERA_RESTAURACAO
#define E2 ERA_POVOS
#define E3 ERA_IMPERIALISMO
#define E4 ERA_GUERRAS
#define E5 ERA_GUERRA_FRIA
#define E6 ERA_GLOBALIZACAO
#define E7 ERA_ETERNA

static const RndEvent RND[]={
 // natureza
 {"Colheita farta","Os celeiros transbordam.",E0,E7,0,{S(HE_GOLD,40),S(HE_HAPPY,5)}},
 {"Seca","O sol queima as plantações.",E0,E7,0,{S(HE_GOLD,-30),S(HE_HAPPY,-5)}},
 {"Enchente","Os rios saem do leito e levam vilas inteiras.",E0,E7,0,{S(HE_GOLD,-40),S(HE_TROOPS,-3)}},
 {"Inverno rigoroso","A neve isola cidades por semanas.",E0,E7,RQ_NORTH,{S(HE_TROOPS,-5),S(HE_HAPPY,-5)}},
 {"Praga de gafanhotos","Nuvens de gafanhotos devoram os campos.",E0,E7,RQ_TROPICS,{S(HE_GOLD,-40)}},
 {"Terremoto","A terra treme e derruba casas e igrejas.",E0,E7,0,{S(HE_GOLD,-60),S(HE_HAPPY,-5)}},
 {"Erupção vulcânica","Cinzas cobrem as lavouras.",E0,E7,0,{S(HE_GOLD,-40),S(HE_HAPPY,-10)}},
 {"Furacão","Ventos furiosos varrem a costa.",E0,E7,RQ_COAST|RQ_TROPICS,{S(HE_GOLD,-50)}},
 {"Incêndio na capital","Um incêndio consome bairros inteiros.",E0,E7,0,{S(HE_GOLD,-50),S(HE_PRESTIGE,-2)}},
 {"Cometa no céu","Astrólogos e cientistas discutem o presságio.",E0,E7,0,{S(HE_PRESTIGE,2),S(HE_MILIT,3)}},
 // saude
 {"Varíola","Uma epidemia de varíola se espalha.",E0,E5,0,{S(HE_TROOPS,-5),S(HE_HAPPY,-8)}},
 {"Cólera","O cólera chega pelas rotas de comércio.",E1,E5,0,{S(HE_TROOPS,-6),S(HE_HAPPY,-6)}},
 {"Febre amarela","Mosquitos trazem a febre aos portos.",E0,E4,RQ_TROPICS,{S(HE_TROOPS,-6)}},
 {"Gripe pandêmica","Uma gripe mortal atravessa fronteiras.",E3,E6,0,{S(HE_TROOPS,-5),S(HE_GOLD,-20)}},
 {"Vacinação em massa","Médicos vacinam cidades inteiras.",E2,E7,0,{S(HE_HAPPY,8),S(HE_PRESTIGE,3)}},
 {"Descoberta médica","Um médico nacional muda a medicina.",E3,E7,0,{S(HE_PRESTIGE,8)}},
 // economia
 {"Corrida do ouro","Ouro! Milhares correm para as minas.",E1,E4,0,{S(HE_GOLD,150),S(HE_HAPPY,5),S(HE_MILIT,5)}},
 {"Nova mina","Geólogos encontram um veio rico.",E0,E7,0,{S(HE_GOLD,80)}},
 {"Bolha especulativa","Ações sobem sem parar... até cair.",E2,E7,RQ_RICH,{S(HE_GOLD,-100)}},
 {"Pânico bancário","Correntistas fazem fila para sacar tudo.",E2,E7,0,{S(HE_GOLD,-80),S(HE_HAPPY,-5)}},
 {"Boom de exportação","Os portos não dão conta de tanta carga.",E0,E7,RQ_COAST,{S(HE_GOLD,100)}},
 {"Descoberta de petróleo","Um poço jorra petróleo.",E3,E7,0,{S(HE_GOLD,200)}},
 {"Greve geral","Operários cruzam os braços.",E3,E7,0,{S(HE_GOLD,-50),S(HE_MILIT,10)}},
 {"Grande invenção","Um inventor nacional surpreende o mundo.",E1,E7,0,{S(HE_PRESTIGE,6),S(HE_GOLD,40)}},
 {"Crise da dívida","Os credores batem à porta.",E4,E7,RQ_POOR,{S(HE_GOLD,-40),S(HE_MILIT,8)}},
 // sociedade
 {"Onda de imigração","Navios chegam cheios de famílias.",E1,E7,0,{S(HE_TROOPS,5),S(HE_GOLD,30)}},
 {"Êxodo rural","O campo se esvazia; as cidades crescem.",E3,E7,0,{S(HE_GOLD,30),S(HE_HAPPY,-3)}},
 {"Revolta camponesa","Camponeses se levantam contra os senhores.",E0,E4,RQ_MON,{S(HE_MILIT,15),S(HE_TROOPS,-5)}},
 {"Movimento abolicionista","Panfletos e comícios exigem o fim da escravidão.",E0,E2,0,{S(HE_PRESTIGE,3),S(HE_MILIT,5)}},
 {"Sufragistas","Mulheres exigem o direito de votar.",E3,E5,0,{S(HE_HAPPY,5),S(HE_MILIT,5)}},
 {"Nasce um gênio","Um talento raro surge no país.",E0,E7,0,{S(HE_PRESTIGE,10)}},
 {"Festival nacional","Dias de festa, música e banquetes.",E0,E7,0,{S(HE_HAPPY,10),S(HE_GOLD,-20)}},
 {"Exposição universal","O mundo vem ver as maravilhas do país.",E2,E5,RQ_RICH,{S(HE_PRESTIGE,10),S(HE_GOLD,-50)}},
 {"Jogos olímpicos","Atletas do mundo inteiro competem aqui.",E4,E7,RQ_RICH,{S(HE_PRESTIGE,12),S(HE_GOLD,-60)}},
 {"Movimento estudantil","Estudantes ocupam as praças.",E5,E7,0,{S(HE_MILIT,10),S(HE_HAPPY,-3)}},
 // politica
 {"Escândalo na corte","Intrigas e amantes enchem os jornais.",E0,E7,RQ_MON,{S(HE_PRESTIGE,-5),S(HE_MILIT,5)}},
 {"Escândalo no parlamento","Ministros são pegos com as mãos no cofre.",E1,E7,RQ_REP,{S(HE_PRESTIGE,-5),S(HE_HAPPY,-5)}},
 {"Sucessão disputada","Dois herdeiros reclamam o trono.",E0,E5,RQ_MON,{S(HE_TROOPS,-10),S(HE_MILIT,10)}},
 {"Golpe militar","Generais tomam o palácio.",E1,E6,0,{S(HE_TROOPS,10),S(HE_HAPPY,-10),S(HE_MILIT,10)}},
 {"Atentado frustrado","O chefe de Estado escapa por pouco.",E0,E7,0,{S(HE_PRESTIGE,3),S(HE_MILIT,-5)}},
 {"Reforma constitucional","Uma nova carta amplia direitos.",E1,E7,0,{S(HE_HAPPY,8),S(HE_MILIT,-8)}},
 // militar
 {"Motim","Soldados sem soldo se amotinam.",E0,E7,RQ_WAR,{S(HE_TROOPS,-8)}},
 {"Desertores","Recrutas fogem à noite.",E0,E7,RQ_WAR,{S(HE_TROOPS,-5)}},
 {"Herói de guerra","Um oficial vira lenda nacional.",E0,E7,RQ_WAR,{S(HE_PRESTIGE,8),S(HE_HAPPY,5)}},
 {"Espiões descobertos","Uma rede estrangeira é desmantelada.",E0,E7,0,{S(HE_PRESTIGE,3)}},
 {"Nova doutrina","Oficiais trazem novas táticas.",E1,E7,0,{S(HE_TROOPS,8)}},
 // diplomacia
 {"Casamento real","Duas casas reais se unem.",E0,E6,RQ_MON,{S(HE_PRESTIGE,6),S(HE_GOLD,30)}},
 {"Tratado comercial","Um vizinho propõe comércio livre.",E0,E7,RQ_PEACE,{S(HE_GOLD,60)}},
 {"Missão estrangeira","Embaixadores chegam com presentes.",E0,E7,0,{S(HE_PRESTIGE,4)}},
 {"Pedido de asilo","Um exilado famoso pede proteção.",E0,E7,0,{S(HE_PRESTIGE,3),S(HE_HAPPY,2)}},
 // povos
 {"Renascimento cultural","Anciãos e jovens revivem cantos e histórias.",E0,E7,RQ_POVO,{S(HE_HAPPY,10),S(HE_PRESTIGE,5)}},
 {"Aliança entre povos","Povos vizinhos selam uma aliança.",E0,E7,RQ_POVO,{S(HE_TROOPS,10)}},
 {"Retomada de território","Comunidades voltam a terras ancestrais.",E5,E7,RQ_POVO,{S(HE_PRESTIGE,8),S(HE_HAPPY,8)}},
 // moderno
 {"Boom tecnológico","Empresas nacionais conquistam o mundo.",E6,E7,0,{S(HE_GOLD,150),S(HE_PRESTIGE,5)}},
 {"Ataque hacker","Sistemas do governo são invadidos.",E6,E7,0,{S(HE_GOLD,-60)}},
 {"Crise climática","Ondas de calor e secas recordes.",E6,E7,0,{S(HE_GOLD,-40),S(HE_HAPPY,-5)}},
 {"Mega-evento esportivo","Estádios lotados e contas altas.",E5,E7,0,{S(HE_PRESTIGE,8),S(HE_GOLD,-50)}},
 {"Descoberta de lítio","O metal das baterias está no subsolo.",E6,E7,0,{S(HE_GOLD,120)}},
 // corridas do futuro (Modo Eterno)
 {"Energia de fusão","Um reator de fusão acende pela primeira vez.",E7,E7,RQ_RICH,{S(HE_PRESTIGE,20),S(HE_GOLD,200)}},
 {"Base lunar","Uma base permanente é inaugurada na Lua.",E7,E7,RQ_RICH,{S(HE_PRESTIGE,15)}},
 {"Colônia em Marte","Os primeiros colonos pousam em Marte.",E7,E7,RQ_RICH,{S(HE_PRESTIGE,25)}},
 // ideologia e separatismo (v1.0)
 {"Agitação revolucionária","Clubes secretos pregam a derrubada do regime.",E2,E6,0,{S(HE_MILIT,12),S(HE_HAPPY,-3)}},
 {"Golpe de Estado","Generais cercam o palácio e assumem o governo.",E1,E6,RQ_UNSTABLE,{S(HE_IDEO,ID_AUTORITARIO+16)}},
 {"Revolução vermelha","Sovietes de operários e soldados tomam a capital.",E4,E6,RQ_POOR|RQ_UNSTABLE,{S(HE_IDEO,ID_COMUNISTA+16)}},
 {"Restauração da coroa","Monarquistas trazem de volta um herdeiro do trono.",E1,E4,RQ_REP|RQ_UNSTABLE,{S(HE_IDEO,ID_TRADICIONAL+16)}},
 {"Reforma liberal","Uma constituição limita o poder do trono.",E2,E7,RQ_MON|RQ_UNSTABLE,{S(HE_IDEO,ID_LIBERAL),S(HE_HAPPY,5)}},
 {"Reavivamento religioso","Pregadores enchem as praças; a fé volta ao governo.",E0,E7,RQ_FAITH|RQ_UNSTABLE,{S(HE_HAPPY,5),S(HE_IDEO,ID_TEOCRATICO)}},
 {"Movimento separatista","Uma província pega em armas pela independência.",E1,E7,RQ_UNSTABLE,{S(HE_SEPARA,0),S(HE_MILIT,5)}},
 {"Piratas","Corsários saqueiam navios mercantes.",E0,E3,RQ_COAST,{S(HE_SHIPS,-1),S(HE_GOLD,-30)}},
 {"Novo navio de linha","O estaleiro lança um navio com orgulho.",E0,E7,RQ_SEA,{S(HE_SHIPS,2),S(HE_PRESTIGE,2)}},
 {"Boom do algodão","As fiações pagam qualquer preço pela fibra.",E1,E3,0,{S(HE_GOLD,60)}},
 {"Febre do café","O mundo inteiro quer o nosso café.",E1,E5,RQ_TROPICS,{S(HE_GOLD,80)}},
 {"Peste bovina","O gado morre aos milhares.",E0,E5,0,{S(HE_GOLD,-40),S(HE_HAPPY,-3)}},
 {"Telégrafo","Mensagens cruzam o país em minutos.",E2,E3,0,{S(HE_PRESTIGE,4),S(HE_GOLD,20)}},
 {"Eletrificação","Lâmpadas acendem nas cidades.",E3,E5,0,{S(HE_GOLD,50),S(HE_HAPPY,4)}},
 {"Rádio nacional","A voz do governo chega a todas as casas.",E4,E5,0,{S(HE_HAPPY,4),S(HE_MILIT,-3)}},
 {"Internet","Uma rede liga o país ao mundo.",E6,E7,0,{S(HE_GOLD,80),S(HE_PRESTIGE,3)}},
};
#define RND_N ((int)(sizeof(RND)/sizeof(RND[0])))
static const RndDecision DEC[]={
 {{"Greve nas fábricas","Operários param as máquinas por salários melhores.",E3,E7,0,{S(HE_GOLD,-40),S(HE_HAPPY,8),S(HE_MILIT,-8)}},"Negociar","Reprimir",{S(HE_MILIT,12),S(HE_HAPPY,-6),S(HE_TROOPS,-3)}},
 {{"Ferrovia transcontinental","Engenheiros propõem trilhos de costa a costa.",E2,E4,0,{S(HE_GOLD,-80),S(HE_PRESTIGE,8),S(HE_HAPPY,4)}},"Construir","Adiar",{S(HE_GOLD,20)}},
 {{"Refugiados na fronteira","Milhares fogem de uma guerra vizinha.",E0,E7,0,{S(HE_TROOPS,3),S(HE_HAPPY,-3),S(HE_PRESTIGE,5)}},"Acolher","Fechar a fronteira",{S(HE_PRESTIGE,-3),S(HE_MILIT,3)}},
 {{"Reforma eleitoral","A burguesia exige o direito de votar.",E1,E5,0,{S(HE_HAPPY,8),S(HE_MILIT,-10)}},"Conceder","Negar",{S(HE_MILIT,12),S(HE_PRESTIGE,2)}},
 {{"Minas do Estado","Ministros propõem estatizar as minas.",E3,E6,0,{S(HE_GOLD,60),S(HE_HAPPY,-4)}},"Estatizar","Manter privadas",{S(HE_GOLD,20)}},
 {{"Conselho de guerra","Os generais pedem mais recrutas.",E0,E7,RQ_WAR,{S(HE_TROOPS,12),S(HE_HAPPY,-6)}},"Convocar","Poupar o povo",{S(HE_MILIT,-5),S(HE_PRESTIGE,-3)}},
 {{"Expedição científica","Exploradores pedem patrocínio para mapear terras distantes.",E1,E7,0,{S(HE_GOLD,-40),S(HE_PRESTIGE,10)}},"Patrocinar","Recusar",{S(HE_HAPPY,-1)}},
 {{"Panfletos vermelhos","Ideias socialistas circulam nas fábricas.",E2,E6,0,{S(HE_GOLD,-50),S(HE_MILIT,-12),S(HE_HAPPY,6)}},"Leis trabalhistas","Proibir",{S(HE_MILIT,10),S(HE_PRESTIGE,1)}},
 {{"Nacionalismo nas ruas","Multidões pedem glória e territórios.",E2,E5,0,{S(HE_PRESTIGE,6),S(HE_MILIT,6),S(HE_TROOPS,5)}},"Abraçar a causa","Pedir calma",{S(HE_PRESTIGE,-2),S(HE_MILIT,-4)}},
 {{"Fome no interior","A colheita falhou em várias províncias.",E0,E5,0,{S(HE_GOLD,-60),S(HE_HAPPY,6)}},"Abrir os celeiros","Deixar o mercado agir",{S(HE_HAPPY,-10),S(HE_MILIT,8)}},
 {{"Corsários na costa","Navios piratas atacam nossos mercadores.",E0,E4,RQ_COAST,{S(HE_SHIPS,-1),S(HE_PRESTIGE,5),S(HE_GOLD,40)}},"Mandar a frota","Pagar proteção",{S(HE_GOLD,-50)}},
 {{"Proposta de casamento","Uma casa estrangeira oferece uma aliança de sangue.",E0,E4,RQ_MON,{S(HE_PRESTIGE,6),S(HE_GOLD,40)}},"Aceitar","Recusar",{S(HE_HAPPY,2)}},
 {{"Escândalo de corrupção","Um ministro desviou o tesouro.",E1,E7,0,{S(HE_PRESTIGE,-4),S(HE_MILIT,-6),S(HE_HAPPY,4)}},"Julgamento público","Abafar o caso",{S(HE_GOLD,40),S(HE_MILIT,8)}},
 {{"Imigrantes no porto","Navios trazem trabalhadores do outro lado do mar.",E2,E7,RQ_COAST,{S(HE_TROOPS,4),S(HE_GOLD,40),S(HE_HAPPY,-2)}},"Receber","Restringir",{S(HE_HAPPY,3)}},
 {{"O clero e as escolas","Os religiosos querem a educação nas mãos da fé.",E0,E4,0,{S(HE_HAPPY,6),S(HE_PRESTIGE,-2)}},"Ceder","Laicizar",{S(HE_MILIT,6),S(HE_PRESTIGE,4)}},
 {{"Conspiração nos quartéis","Oficiais tramam contra o governo.",E1,E6,0,{S(HE_TROOPS,-6),S(HE_MILIT,-6)}},"Prender os conspiradores","Comprar a lealdade",{S(HE_GOLD,-80)}},
 {{"Corrida naval","Um rival dobrou a frota.",E3,E5,RQ_COAST,{S(HE_GOLD,-100),S(HE_SHIPS,3)}},"Construir navios","Apostar na diplomacia",{S(HE_PRESTIGE,-2),S(HE_HAPPY,2)}},
 {{"Jazida de carvão e ferro","Geólogos encontram um veio enorme.",E1,E7,0,{S(HE_GOLD,100),S(HE_HAPPY,-3)}},"Explorar já","Explorar com calma",{S(HE_GOLD,40)}},
 {{"Sufrágio feminino","Sufragistas enchem as praças.",E3,E5,0,{S(HE_HAPPY,8),S(HE_PRESTIGE,6)}},"Aprovar","Adiar",{S(HE_MILIT,8)}},
 {{"Credores à porta","Bancos estrangeiros exigem o pagamento da dívida.",E2,E7,RQ_POOR,{S(HE_GOLD,-60),S(HE_HAPPY,-8)}},"Pagar com austeridade","Dar o calote",{S(HE_PRESTIGE,-8),S(HE_MILIT,4)}},
 {{"Barricadas na capital","O povo ergue barricadas e exige um novo regime.",E2,E6,RQ_UNSTABLE,{S(HE_IDEO,-1),S(HE_HAPPY,10)}},"Ceder ao povo","Esmagar a revolta",{S(HE_TROOPS,-10),S(HE_MILIT,10),S(HE_PRESTIGE,-4)}},
 {{"Ameaça de bloqueio","Um rival ameaça fechar nossos portos.",E1,E7,RQ_COAST,{S(HE_GOLD,-40),S(HE_SHIPS,1)}},"Escoltar os mercadores","Aceitar as perdas",{S(HE_GOLD,-60)}},
};
#define DEC_N ((int)(sizeof(DEC)/sizeof(DEC[0])))

static u32 ANCH[HIST_N];
static bool anchOk=false;

int Hist_Count(void){return HIST_N;}
const HistEvent *Hist_Get(int i){return (i>=0&&i<HIST_N)?&HIST[i]:0;}
int Rnd_Count(void){return RND_N+DEC_N;}
u8 Hist_State(const World *w,int i){return (u8)((w->hState[i>>2]>>((i&3)*2))&3);}
static void SetState(World *w,int i,u8 s){
 w->hState[i>>2]=(u8)((w->hState[i>>2]&~(3<<((i&3)*2)))|(s<<((i&3)*2)));
}
void Hist_Init(World *w){
 if(!anchOk){
  for(int i=0;i<HIST_N;i++)ANCH[i]=Cal_DaysFrom(HIST[i].y,HIST[i].m,HIST[i].d);
  anchOk=true;
 }
 for(unsigned i=0;i<sizeof(w->hState);i++)w->hState[i]=0;
}

static u8 C(const World *w,const char *tag){return tag?World_CountryOfNat(w,Nat_Find(tag)):0;}
static int FindEvt(const char *id){
 for(int i=0;i<HIST_N;i++){const char *a=HIST[i].id;const char *b=id;
  while(*a&&*a==*b){a++;b++;} if(!*a&&!*b)return i;}
 return -1;
}
static bool LocPos(const World *w,const char *tag,int *x,int *y){
 int n=Nat_Find(tag);
 if(n>=0){World_NatPos(w,n,x,y);return true;}
 for(unsigned i=0;i<sizeof(LOC)/sizeof(LOC[0]);i++){
  const char *a=LOC[i].tag;
  if(a[0]==tag[0]&&a[1]==tag[1]&&a[2]==tag[2]){World_LatLonPos(w,LOC[i].lat,LOC[i].lon,x,y);return true;}
 }
 return false;
}
static bool Cond(const World *w,const HOp *o){
 u8 a=C(w,o->a),b=C(w,o->b);
 switch(o->op){
 case HC_EXISTS:return a!=0;
 case HC_NOT_EXISTS:return a==0;
 case HC_SUBJECT:return a&&b&&w->c[a].overlord==b;
 case HC_STRONGER:return a&&b&&(s64)w->c[a].troops*100>=(s64)w->c[b].troops*o->v;
 case HC_AT_WAR:return World_AtWar(w,a,b);
 case HC_FIRED:{int e=FindEvt(o->a);return e>=0&&Hist_State(w,e)==1;}
 default:return true;
 }
}
static s32 Clamp100(s32 v){return v<0?0:(v>100?100:v);}

// Aplica um efeito. self = pais alvo quando a == NULL (eventos aleatorios).
static void Apply(World *w,const HOp *o,u8 self){
 u8 a=o->a?C(w,o->a):self, b=C(w,o->b);
 WCountry *ca=a?&w->c[a]:0;
 switch(o->op){
 case HE_PRESTIGE:if(ca)ca->prestige+=o->v;break;
 case HE_GOLD:if(ca){s32 g=o->v;if(!o->a)g=g*(s32)(1+ca->cells/100);ca->gold+=g;if(ca->gold<0)ca->gold=0;}break;
 case HE_TROOPS:if(ca){ca->troops+=ca->troops*o->v/100;if(ca->troops<0)ca->troops=0;}break;
 case HE_HAPPY:if(ca)ca->happy=(u8)Clamp100(ca->happy+o->v);break;
 case HE_MILIT:if(ca)ca->militancy=(u8)Clamp100(ca->militancy+o->v);break;
 case HE_INDEP:if(ca){ca->overlord=0;ca->kind=NK_ESTADO;}break;
 case HE_SUBJECT:if(ca&&b){ca->overlord=b;ca->kind=NK_SUDITO;World_Peace(w,a,b);}break;
 case HE_ANNEX:if(a&&b)World_Annex(w,a,b);break;
 case HE_UNIFY:if(a&&b)World_Unify(w,a,b);break;
 case HE_BECOME:if(a){int nb=Nat_Find(o->b);if(nb>=0&&!World_CountryOfNat(w,nb))World_Become(w,a,nb);}break;
 case HE_SPAWN:{int na=Nat_Find(o->a);if(na<0||World_CountryOfNat(w,na))break;
  int src=-1;if(o->b){if(!b)break;src=b;}
  int x,y;World_NatPos(w,na,&x,&y);World_Spawn(w,na,src,x,y,o->v);}break;
 case HE_CEDE:{if(!a||!b)break;int x,y;if(!LocPos(w,o->c,&x,&y))break;World_Transfer(w,a,b,x,y,o->v);}break;
 case HE_RELOCATE:{if(!a||!b)break;int x,y;if(!LocPos(w,o->c,&x,&y))break;
  World_MoveAll(w,a,b);           // a fica vivo com 0 celulas...
  WCountry *c=&w->c[a];
  c->bx0=c->bx1=(u16)x;c->by0=c->by1=(u16)y;c->capX=(u16)x;c->capY=(u16)y;
  if(!World_Transfer(w,a,-1,x,y,o->v))World_Kill(w,a); // ...ate ganhar o novo lar
 }break;
 case HE_WAR:if(a&&b)World_DeclareHistWar(w,a,b);break;
 case HE_PEACE:if(a&&b)World_Peace(w,a,b);break;
 case HE_PEACEALL:for(u8 i=1;i<w->nC;i++)for(u8 j=(u8)(i+1);j<w->nC;j++)if(World_AtWar(w,i,j))World_Peace(w,i,j);break;
 case HE_GOV:if(ca){ca->gov=(u8)o->v;
  // a ideologia acompanha a forma de governo (sem revolucao)
  if(o->v==NG_MONARQUIA)WDip_SetIdeo(w,a,ID_TRADICIONAL,false);
  else if(o->v==NG_TEOCRACIA)WDip_SetIdeo(w,a,ID_TEOCRATICO,false);
  else if(o->v==NG_REPUBLICA&&(ca->ideo==ID_TRADICIONAL||ca->ideo==ID_TEOCRATICO))WDip_SetIdeo(w,a,ID_LIBERAL,false);
 }break;
 case HE_IDEO:if(ca){
  bool rev=o->v<0||o->v>=16;
  u8 to=o->v<0?WDip_RandomIdeo(w,a):(u8)(o->v&15);
  if(to!=ca->ideo){
   WDip_SetIdeo(w,a,to,rev);
   if(!o->a&&!ca->player&&ca->cells>=150){
    char b[96];int n=0;const char *p=NATIONS[ca->nat].name;while(*p&&n<40)b[n++]=*p++;
    p=Str(STR_D_AGORA);while(*p&&n<70)b[n++]=*p++;p=Str((u16)(STR_ID_0+to));while(*p&&n<94)b[n++]=*p++;b[n]=0;
    World_Msg(w,Str(STR_D_REVOL),b);
   }
  }
 }break;
 case HE_SHIPS:if(ca){ca->ships=(s16)(ca->ships+o->v);if(ca->ships<0)ca->ships=0;}break;
 case HE_SEPARA:if(a)WDip_Separatism(w,a);break;
 case HE_WORLD_MILIT:for(u8 i=1;i<w->nC;i++)if(w->c[i].alive)w->c[i].militancy=(u8)Clamp100(w->c[i].militancy+o->v);break;
 case HE_ADDTRAIT:if(ca){bool has=false;for(int i=0;i<NAT_TRAITS;i++)if(ca->traits[i]==o->v)has=true;
  if(!has)for(int i=0;i<NAT_TRAITS;i++)if(!ca->traits[i]){ca->traits[i]=(u8)o->v;break;}
  World_SetTraits(w,a);}break;
 case HE_DELTRAIT:if(ca){for(int i=0;i<NAT_TRAITS;i++)if(ca->traits[i]==o->v)ca->traits[i]=0;World_SetTraits(w,a);}break;
 case HE_WORLD_GOLD:for(u8 i=1;i<w->nC;i++)if(w->c[i].alive){w->c[i].gold+=w->c[i].gold*o->v/100;if(w->c[i].gold<0)w->c[i].gold=0;}break;
 case HE_WORLD_HAPPY:for(u8 i=1;i<w->nC;i++)if(w->c[i].alive)w->c[i].happy=(u8)Clamp100(w->c[i].happy+o->v);break;
 case HE_DECOLONIZE:for(u8 i=1;i<w->nC;i++){WCountry *c=&w->c[i];
  if(c->alive&&c->kind==NK_SUDITO&&NATIONS[c->nat].cont==o->v){c->kind=NK_ESTADO;c->overlord=0;}}break;
 case HE_POVOS_PRESTIGE:for(u8 i=1;i<w->nC;i++){WCountry *c=&w->c[i];
  if(c->alive&&(c->kind==NK_POVO||c->kind==NK_AUTONOMO))c->prestige+=o->v;}break;
 default:break;
 }
}

void Hist_Day(World *w){
 for(int i=0;i<HIST_N;i++){
  if(w->day<ANCH[i])break;          // tabela em ordem de data (teste garante)
  if(Hist_State(w,i))continue;
  const HistEvent *e=&HIST[i];
  if(w->day>ANCH[i]+(u32)e->win*365u+30u){SetState(w,i,2);continue;}
  bool ok=true;
  for(int k=0;k<3&&ok;k++)if(e->cond[k].op)ok=Cond(w,&e->cond[k]);
  if(!ok)continue;
  for(int k=0;k<6;k++)if(e->eff[k].op)Apply(w,&e->eff[k],0);
  SetState(w,i,1);w->nEvents++;
  World_Msg(w,e->title,e->body);
 }
}

static bool Eligible(const World *w,u8 ci,const RndEvent *r,Era era){
 const WCountry *c=&w->c[ci];
 if(era<r->eraMin||era>r->eraMax)return false;
 u16 q=r->req;
 if((q&RQ_COAST)&&!c->coastal)return false;
 if((q&RQ_WAR)&&!c->warN)return false;
 if((q&RQ_PEACE)&&c->warN)return false;
 if((q&RQ_MON)&&c->gov!=NG_MONARQUIA)return false;
 if((q&RQ_REP)&&c->gov!=NG_REPUBLICA)return false;
 if((q&RQ_POVO)&&c->kind!=NK_POVO&&c->kind!=NK_AUTONOMO)return false;
 int lat=NATIONS[c->nat].lat;
 if((q&RQ_TROPICS)&&(lat>23||lat<-23))return false;
 if((q&RQ_NORTH)&&lat<40)return false;
 if(q&RQ_FAITH){bool f=false;for(int i=0;i<NAT_TRAITS;i++)if(c->traits[i]==TR_FE)f=true;if(!f)return false;}
 if(q&RQ_SEA){bool f=false;for(int i=0;i<NAT_TRAITS;i++)if(c->traits[i]==TR_MARES)f=true;if(!f)return false;}
 if((q&RQ_POOR)&&c->gold>(s32)c->cells)return false;
 if((q&RQ_RICH)&&c->gold<(s32)c->cells*20)return false;
 if((q&RQ_UNSTABLE)&&WDip_RevoltRisk(w,ci)<15)return false;
 return true;
}

int Rnd_DecisionCount(void){return DEC_N;}
static const RndDecision *Dec(int i){return (i>=RND_N&&i<RND_N+DEC_N)?&DEC[i-RND_N]:0;}
const char *Rnd_Title(int i){const RndDecision *d=Dec(i);return d?d->e.title:"";}
const char *Rnd_Body(int i){const RndDecision *d=Dec(i);return d?d->e.body:"";}
const char *Rnd_Opt(int i,bool a){const RndDecision *d=Dec(i);if(!d)return "";return a?d->optA:d->optB;}
void Rnd_Decide(World *w,u8 ci,int idx,bool optA){
 const RndDecision *d=Dec(idx);
 if(!d||!w->c[ci].alive)return;
 const HOp *e=optA?d->e.eff:d->effB;
 for(int k=0;k<3;k++)if(e[k].op)Apply(w,&e[k],ci);
}
void Rnd_Day(World *w){
 Era era=Cal_Era(w->date.y);
 for(u8 ci=1;ci<w->nC;ci++){
  WCountry *c=&w->c[ci];
  if(!c->alive||c->rndCd||((ci+w->day)%30)!=0)continue;
  if(World_RngRange(w,100)>=45)continue;
  for(int t=0;t<6;t++){
   int idx=World_RngRange(w,RND_N+DEC_N);
   const RndEvent *r=idx<RND_N?&RND[idx]:&DEC[idx-RND_N].e;
   bool dec=idx>=RND_N;
   if(!Eligible(w,ci,r,era))continue;
   if(dec&&c->player){
    // decisao do jogador: vira um despacho com Aceitar/Recusar (wgame pausa)
    if(!WDip_Propose(w,PR_DECISAO,ci,0,0,(u16)idx))continue;
   }else if(dec){
    // IA: temperamento e arquetipo decidem (sem RNG extra)
    bool a=((c->mood+(int)(w->day%41u)+c->archetype*13)%100)<55;
    Rnd_Decide(w,ci,idx,a);
   }else{
    for(int k=0;k<3;k++)if(r->eff[k].op)Apply(w,&r->eff[k],ci);
    if(c->player)World_Msg(w,r->title,r->body);
   }
   c->rndCd=(u8)(20+World_RngRange(w,20));
   w->nEvents++;
   break;
  }
 }
}
