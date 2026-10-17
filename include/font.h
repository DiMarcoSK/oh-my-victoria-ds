#pragma once
#include <nds.h>
// font.h — Serifa 8 (1bpp, desenhada a mao) e Serifa 12 (2bpp antisserrilhada:
// 4 niveis de cobertura misturados com o fundo) proporcionais (SDD sec 14.3). Os DADOS moram em
// source/gen/font_data.c (gerado por tools/genfont.py e commitado: o build nao
// roda Python, ADR-14). Aqui mora so' a API.
//
// O texto do jogo e' UTF-8 (source/str.c). Px_Text2 decodifica o UTF-8 e busca
// o glifo por codepoint; as tabelas sao ordenadas por cp (busca binaria),
// zero malloc. Se faltar glifo, desenha '?'; tools/font_check.py falha o build
// se algum caractere usado nas strings/nomes ficar de fora.
typedef struct { u16 cp, off; u8 adv, w, h; s8 left, top; } Glyph;
typedef struct { const Glyph *gl; const u8 *bits; u16 n; u8 caph, lineh, bpp; } Font;

extern const Font F_SERIF12; // titulos e itens: caixa alta 9px, linha 14px
extern const Font F_SERIF8;  // descricoes/rotulos/numeros: caixa alta 7px, linha 11px

// (x,y) = TOPO da caixa alta (nao a baseline). Sem quebra; '\n' pula linha.
void Px_Text2(u16 *dst,int x,int y,const char *utf8,const Font *f,u16 col);
// Largura em pixels de uma linha (utf8), sem desenhar.
int Px_Width2(const char *utf8,const Font *f);
// Quebra por palavra dentro de lim px a partir de (x,y); devolve a altura
// usada. Respeita '\n'. Usado pelas descricoes da SDD sec 14.4/14.5.
int Px_Wrap(u16 *dst,int x,int y,int lim,const char *utf8,const Font *f,u16 col);
// Versao estendida: sp = espacamento extra entre letras (nomes espacados do
// mapa, DESIGN 4.3), sc = escala inteira (logo), flags = TX_UPPER (caixa alta
// quando a fonte tem o glifo). Out = contorno de 1px nas 8 direcoes.
enum { TX_UPPER=1 };
void Px_Text2Ex(u16 *dst,int x,int y,const char *utf8,const Font *f,u16 col,int sp,int sc,int flags);
void Px_Text2Out(u16 *dst,int x,int y,const char *utf8,const Font *f,u16 col,u16 out,int sp,int sc,int flags);
int Px_Width2Ex(const char *utf8,const Font *f,int sp,int sc,int flags);
// Quebra com teto de linhas (ultima termina em "…") e centralizacao opcional.
int Px_WrapN(u16 *dst,int x,int y,int lim,const char *utf8,const Font *f,u16 col,int maxLines,bool center);
// Glifo de um codepoint (medir/desenhar um grafema); NULL se ausente.
const Glyph *F_Glyph(const Font *f,int cp);
