#include "txt.h"
// txt.c — decodificador UTF-8 minimo + dobra de diacriticos p/ ASCII.

// U+0100..U+017F (Latim Estendido-A) -> letra base, na ordem dos codepoints.
static const char EXT_A[129]=
 "AaAaAa" "CcCcCcCc" "DdDd" "EeEeEeEeEe" "GgGgGgGg" "HhHh" "IiIiIiIiIi"
 "Jj" "Jj" "Kkk" "LlLlLlLlLl" "NnNnNnnNn" "OoOoOoOo" "RrRrRr" "SsSsSsSs"
 "TtTtTt" "UuUuUuUuUuUu" "Ww" "YyY" "ZzZzZz" "s";

int Txt_Decode(const char **s){
 const u8 *p=(const u8*)*s;
 int c=p[0];
 if(c==0)return 0;
 int n=1,cp=c;
 if(c>=0xF0){cp=c&7;n=4;}
 else if(c>=0xE0){cp=c&15;n=3;}
 else if(c>=0xC0){cp=c&31;n=2;}
 for(int i=1;i<n;i++){
  if((p[i]&0xC0)!=0x80){n=i;cp='?';break;}
  cp=(cp<<6)|(p[i]&63);
 }
 *s+=n;
 return cp;
}

static char Base(int cp){
 if(cp<128)return (char)cp;
 if(cp>=0xC0&&cp<=0xFF){
  static const char L1[65]=
   "AAAAAAACEEEEIIII" "DNOOOOOxOUUUUYTs" "aaaaaaaceeeeiiii" "dnooooo/ouuuuyty";
  return L1[cp-0xC0];
 }
 if(cp==0xAA)return 'a';
 if(cp==0xBA)return 'o';
 if(cp>=0x100&&cp<=0x17F)return EXT_A[cp-0x100];
 if(cp==0x1A0||cp==0x1A1)return 'o';
 if(cp==0x1AF||cp==0x1B0||cp==0x244||cp==0x289)return 'u'; // ư, ʉ
 if(cp>=0x300&&cp<=0x36F)return 0;  // diacritico combinante: some
 if(cp>=0x1E00&&cp<=0x1E9F){        // Latim Estendido Adicional (ọ, ṣ...)
  static const char LA[]="AaBbBbBbCcDdDdDdDdDdEeEeEeEeEeFfGgHhHhHhHhHhIiIiKkKkKkLlLlLlLlMmMmMmNnNnNnNnOoOoOoOoPpPpRrRrRrRrSsSsSsSsSsTtTtTtTtUuUuUuUuUuVvVvWwWwWwWwWwXxXxYyZzZzZzhtwyas";
  int k=cp-0x1E00;return k<(int)sizeof(LA)-1?LA[k]:'?';
 }
 if(cp==0x2BB||cp==0x2BC||cp==0x2BF||cp==0x2018||cp==0x2019)return '\'';
 if(cp>=0x1EA0&&cp<=0x1EB7)return 'a'; // vietnamita
 if(cp>=0x1EB8&&cp<=0x1EC7)return 'e';
 if(cp>=0x1EC8&&cp<=0x1ECB)return 'i';
 if(cp>=0x1ECC&&cp<=0x1EE3)return 'o';
 if(cp>=0x1EE4&&cp<=0x1EF1)return 'u';
 if(cp>=0x1EF2&&cp<=0x1EF9)return 'y';
 if(cp==0x2013||cp==0x2014)return '-';
 if(cp==0x2026)return '.';
 return '?';
}

void Txt_Fold(const char *in,char *out,int max){
 int p=0;
 if(max<=0)return;
 while(p<max-1){
  int cp=Txt_Decode(&in);
  if(!cp)break;
  char c=Base(cp);
  if(!c)continue;
  if(c>='a'&&c<='z')c=(char)(c-32);
  out[p++]=c;
 }
 out[p]=0;
}

int Txt_Len(const char *s){int n=0;while(Txt_Decode(&s))n++;return n;}
