#include "traits.h"
// traits.c — tabela de traits (DESIGN.md sec 6.4). Valores sao proposta de
// design: balancear aqui, sem mexer em quem le TraitMods.

typedef struct { s8 w; const char *name, *desc; } TraitDef;

static const TraitDef T[TR_COUNT]={
 {0,"",""},
 {2,"Berço Industrial","+25% produção industrial, +10% renda"},
 {2,"Senhora dos Mares","+10% comércio; guerra longe de casa custa metade"},
 {1,"Entreposto Comercial","+15% comércio externo, +5% renda"},
 {1,"Império Colonial","Expansão sobre terras sem dono 30% mais barata"},
 {1,"Celeiro","Fazendas fartas: +10% regeneração do exército"},
 {1,"Riqueza Mineral","+15% renda de minas"},
 {2,"Exército Profissional","+20% de ataque"},
 {2,"Levée en Masse","+25% regeneração; recrutar 25% mais barato"},
 {1,"Disciplina Regimental","+10% defesa, +5% ataque"},
 {2,"Inverno Implacável","Invasores perdem 3%/dia no inverno"},
 {2,"Cavaleiros das Planícies","Frente avança +50% em planície e estepe"},
 {2,"Conhecimento da Terra","+40% defesa em terreno difícil; atrito ao invasor"},
 {1,"Montanhas-Fortaleza","+40% defesa em colinas e montanhas"},
 {1,"Guerra de Guerrilha","+10% defesa; terras perdidas se revoltam"},
 {1,"Confederação","+5 felicidade; membros não podem ser anexados"},
 {1,"Povo em Movimento","Perder a capital não derruba; terras voltam"},
 {1,"Comunidade Coesa","+10 felicidade, -10 militância"},
 {1,"Fé Unificadora","+5 felicidade, -5 militância, +1 prestígio/ano"},
 {1,"Centro Cultural","+3 prestígio por ano"},
 {1,"Burocracia Antiga","+25% impostos; literacia cresce devagar"},
 {1,"Reformador","Reformas 50% mais baratas, +10% literacia"},
 {1,"Fervor Revolucionário","+5 felicidade, +5% ataque"},
 {1,"Nação Jovem","+15% regeneração do exército"},
 {1,"Fronteira Aberta","Expansão sobre terras sem dono 40% mais barata"},
 {1,"Corsários","+10% renda com saque marítimo"},
 {-1,"Isolamento","-50% comércio externo; +5 felicidade"},
 {-1,"Absolutismo","+15% impostos; -10 felicidade, +10 militância"},
 {-1,"Império Multiétnico","+20% teto do exército; +10 militância"},
 {-1,"Gigante em Crise","-15% renda, -10% ataque, -10% regeneração"},
 {0,"Glória Antiga","+5 prestígio por ano; -20% de ataque"},
};

s8 Tr_Weight(u8 t){return t<TR_COUNT?T[t].w:0;}
const char *Tr_Name(u8 t){return t<TR_COUNT?T[t].name:"";}
const char *Tr_Desc(u8 t){return t<TR_COUNT?T[t].desc:"";}

int Tr_Budget(const u8 tr[NAT_TRAITS]){
 int s=0;
 for(int i=0;i<NAT_TRAITS;i++)s+=Tr_Weight(tr[i]);
 return s;
}

void Tr_Mods(const u8 tr[NAT_TRAITS],TraitMods *m){
 u8 *z=(u8*)m;
 for(unsigned i=0;i<sizeof(*m);i++)z[i]=0;
 for(int i=0;i<NAT_TRAITS;i++){
  switch(tr[i]){
  case TR_INDUSTRIAL: m->prodPct+=25;m->goldPct+=10;break;
  case TR_MARES: m->tradePct+=10;m->goldPct+=5;m->supplyPct-=50;break;
  case TR_ENTREPOSTO: m->tradePct+=15;m->goldPct+=5;break;
  case TR_COLONIAL: m->colonizePct+=30;m->supplyPct-=33;break;
  case TR_CELEIRO: m->regenPct+=10;break;
  case TR_MINERAL: m->goldPct+=15;break;
  case TR_EXERCITO: m->atkPct+=20;break;
  case TR_LEVEE: m->regenPct+=25;m->recruitCostPct-=25;break;
  case TR_DISCIPLINA: m->defPct+=10;m->atkPct+=5;break;
  case TR_INVERNO: m->winterAttrition+=3;break;
  case TR_CAVALEIROS: m->plainsSpeedPct+=50;break;
  case TR_TERRA: m->defRoughPct+=40;m->roughAttrition+=1;break;
  case TR_MONTANHAS: m->defMountPct+=40;break;
  case TR_GUERRILHA: m->defPct+=10;m->revertPct+=1;break;
  case TR_CONFEDERACAO: m->happy+=5;m->confed=true;break;
  case TR_MOVIMENTO: m->capitalNotFatal=true;m->revertPct+=2;break;
  case TR_COESA: m->happy+=10;m->militancy-=10;break;
  case TR_FE: m->happy+=5;m->militancy-=5;m->prestigeYear+=1;break;
  case TR_CULTURA: m->prestigeYear+=3;break;
  case TR_BUROCRACIA: m->goldPct+=25;m->literacyPct-=20;break;
  case TR_REFORMADOR: m->reformPct+=50;m->literacyPct+=10;break;
  case TR_REVOLUCAO: m->happy+=5;m->atkPct+=5;break;
  case TR_JOVEM: m->regenPct+=15;break;
  case TR_FRONTEIRA: m->colonizePct+=40;break;
  case TR_CORSARIOS: m->goldPct+=10;break;
  case TR_ISOLAMENTO: m->tradePct-=50;m->happy+=5;m->isolated=true;break;
  case TR_ABSOLUTISMO: m->goldPct+=15;m->happy-=10;m->militancy+=10;break;
  case TR_MULTIETNICO: m->capPct+=20;m->militancy+=10;break;
  case TR_CRISE: m->goldPct-=15;m->atkPct-=10;m->regenPct-=10;break;
  case TR_GLORIA: m->prestigeYear+=5;m->atkPct-=20;break;
  default: break;
  }
 }
}
