#include "calendar.h"
// calendar.c — datas sem float. Cal_Date/Cal_DaysFrom fazem laco por ano
// (~225 iteracoes): ok p/ formatar/testar; o jogo avanca com Cal_Next.

static const u8 MDAYS[12]={31,28,31,30,31,30,31,31,30,31,30,31};
static const char *const MON[12]={"jan","fev","mar","abr","mai","jun",
 "jul","ago","set","out","nov","dez"};
static const char *const ERA_N[ERA_COUNT]={"Napoleônica","Restauração",
 "Primavera dos Povos","Imperialismo","Guerras Mundiais","Guerra Fria",
 "Globalização","Eterna"};

bool Cal_Leap(int y){return (y%4==0&&y%100!=0)||y%400==0;}
int Cal_MonthDays(int y,int m){
 if(m<1||m>12)return 30;
 return (m==2&&Cal_Leap(y))?29:MDAYS[m-1];
}
u32 Cal_DaysFrom(int y,int m,int d){
 u32 n=0;
 for(int i=CAL_START_YEAR;i<y;i++)n+=Cal_Leap(i)?366u:365u;
 for(int i=1;i<m;i++)n+=(u32)Cal_MonthDays(y,i);
 return n+(u32)(d-1);
}
Date Cal_Date(u32 day){
 int y=CAL_START_YEAR;
 for(;;){u32 len=Cal_Leap(y)?366u:365u;if(day<len)break;day-=len;y++;}
 int m=1;
 for(;;){u32 ml=(u32)Cal_MonthDays(y,m);if(day<ml)break;day-=ml;m++;}
 Date r;r.y=(u16)y;r.m=(u8)m;r.d=(u8)(day+1);return r;
}
void Cal_Next(Date *d){
 if(d->d<Cal_MonthDays(d->y,d->m)){d->d++;return;}
 d->d=1;
 if(d->m<12){d->m++;return;}
 d->m=1;d->y++;
}
u32 Cal_EndDay(void){return Cal_DaysFrom(CAL_END_YEAR,12,31);}
Era Cal_Era(int y){
 if(y<1815)return ERA_NAPOLEONICA;
 if(y<1848)return ERA_RESTAURACAO;
 if(y<1871)return ERA_POVOS;
 if(y<1914)return ERA_IMPERIALISMO;
 if(y<1945)return ERA_GUERRAS;
 if(y<1991)return ERA_GUERRA_FRIA;
 if(y<=CAL_END_YEAR)return ERA_GLOBALIZACAO;
 return ERA_ETERNA;
}
const char *Cal_EraName(Era e){return (unsigned)e<ERA_COUNT?ERA_N[e]:"?";}
const char *Cal_MonthShort(int m){return (m>=1&&m<=12)?MON[m-1]:"?";}

static int PutU(char *o,int p,int max,unsigned v){
 char t[10];int n=0;
 do{t[n++]=(char)('0'+v%10);v/=10;}while(v&&n<10);
 while(n&&p<max-1)o[p++]=t[--n];
 return p;
}
void Cal_Format(const Date *d,char *out,int max){
 if(max<=0)return;
 int p=PutU(out,0,max,d->d);
 if(p<max-1)out[p++]=' ';
 const char *s=Cal_MonthShort(d->m);
 while(*s&&p<max-1)out[p++]=*s++;
 if(p<max-1)out[p++]=' ';
 p=PutU(out,p,max,d->y);
 out[p]=0;
}
