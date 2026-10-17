#pragma once
#include <nds.h>
// txt.h — texto UTF-8 dos dados (nomes com acento, macron, ŋ, Đ...).
// Ate a fonte Serifa (SDD sec 14.3) existir, Txt_Fold converte p/ a fonte
// 3x5: tira diacriticos e passa p/ maiusculas ("Ngāpuhi" -> "NGAPUHI").
int Txt_Decode(const char **s);              // proximo codepoint (0 = fim)
void Txt_Fold(const char *in,char *out,int max);
int Txt_Len(const char *s);                  // em codepoints
