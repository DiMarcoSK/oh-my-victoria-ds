#!/usr/bin/env python3
# genfont.py — gera source/gen/font_data.c (Serifa 12 e 8, 1bpp) a partir de
# TTFs do sistema. Ver SDD sec 14.3: bitmap proporcional, Serifa 12 com caixa
# alta 9px / linha 14px; Serifa 8 com caixa alta 7px / linha 11px.
#
# O arquivo gerado e' COMMITADO (o build nao roda Python, ADR-14). Para
# regenerar: PYTHONPATH=... python3 tools/genfont.py
#
# Primaria: Cambria (serifa historica, boa cobertura). Fallback: DejaVu Sans
# (simbolos geometricos ◂▸▶◆✦ e letras IPA que faltam na serifa).
import os, sys, struct
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont, TTCollection

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRIMARY = ["/mnt/c/Windows/Fonts/cambria.ttc",
           "/System/Library/Fonts/Supplemental/Cambria.ttc",
           "/usr/share/fonts/truetype/msttcorefonts/Cambria.ttf"]
FALLBACK = ["/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/mnt/c/Windows/Fonts/segoeui.ttf"]
# Windows nativo (Python do Windows): Cambria do sistema; DejaVu via env
# GENFONT_DEJAVU (copie de uma instalacao Linux) p/ os simbolos sairem iguais.
PRIMARY += ["C:/Windows/Fonts/cambria.ttc"]
if os.environ.get("GENFONT_DEJAVU"):
    FALLBACK.insert(0, os.environ["GENFONT_DEJAVU"])

def first(paths):
    for p in paths:
        if os.path.exists(p):
            return p
    return paths[0]

# Glifos: ASCII imprimivel + acentos PT + letras dos nomes do catalogo +
# simbolos de UI (SDD 14.3). Tudo que o jogo pode desenhar tem que estar aqui;
# tools/font_check.py falha se algum caractere usado ficar de fora.
EXTRA = ("ªºÀÁÂÃÉÊÍÓÔÕÚÜÇ"
         "àáâãéêíóôõúüç"
         "ÖößïÿñÑ"
         "āēīōūŋčšžŠł"
         "ơưʉʻʿ̀́̃"
         "ẹẽễệỌọ"
         "·—…“”‘’«»")
SYMBOLS = "◂▸▶◆✦"

def cmap_of(path):
    try:
        f = TTCollection(path).fonts[0] if path.lower().endswith(".ttc") else TTFont(path, fontNumber=0)
    except Exception:
        return None
    cps = set()
    for t in f["cmap"].tables:
        cps |= set(t.cmap.keys())
    return cps

def pick_font(cp, fonts):
    # fonts = lista de (path, cmap, loaded_lazy)
    for path, cmap in fonts:
        if cp in cmap:
            return path
    return fonts[0][0]

def find_size(path, target_caph):
    # acha o tamanho (px) cujo 'H' tem altura de tinta == target_caph
    best = None
    for sz in range(8, 24):
        f = ImageFont.truetype(path, sz)
        h = render_glyph(path, sz, "H")["h"]
        d = abs(h - target_caph)
        if best is None or d < best[0]:
            best = (d, sz, f)
        if h == target_caph:
            break
    return best[1]

CANVAS = 40
def render_glyph(path, size, ch):
    f = ImageFont.truetype(path, size)
    BASE_Y, PEN_X = 28, 6
    img = Image.new("L", (CANVAS, CANVAS), 0)
    d = ImageDraw.Draw(img)
    # hinting monocromatico do FreeType: haste inteira em vez de limiar 50%
    # sobre o antisserrilhado (que comia serifa e haste fina)
    d.fontmode = "1"
    try:
        d.text((PEN_X, BASE_Y), ch, font=f, fill=255, anchor="ls")
    except Exception:
        d.text((PEN_X, BASE_Y), ch, font=f, fill=255)
    px = img.load()
    xs = [x for y in range(CANVAS) for x in range(CANVAS) if px[x, y] >= 128]
    ys = [y for y in range(CANVAS) for x in range(CANVAS) if px[x, y] >= 128]
    adv = int(round(f.getlength(ch)))
    if not xs:  # espaco / sem tinta
        return dict(w=0, h=0, left=0, top=0, adv=adv, bits=b"")
    x0, x1 = min(xs), max(xs) + 1
    y0, y1 = min(ys), max(ys) + 1
    w, h = x1 - x0, y1 - y0
    stride = (w + 7) // 8
    bits = bytearray(stride * h)
    for yy in range(h):
        for xx in range(w):
            if px[x0 + xx, y0 + yy] >= 128:
                bits[yy * stride + (xx >> 3)] |= (0x80 >> (xx & 7))
    return dict(w=w, h=h, left=x0 - PEN_X, top=(BASE_Y - y0), adv=adv, bits=bytes(bits))

def c_bytes(b, per=16):
    out = []
    for i in range(0, len(b), per):
        out.append(" " + ",".join("0x%02X" % v for v in b[i:i + per]) + ",")
    return "\n".join(out)

def build(paths, target_caph, extra):
    fonts = [(p, cmap_of(p)) for p in paths if os.path.exists(p)]
    fonts = [(p, c) for p, c in fonts if c]
    if not fonts:
        raise SystemExit("nenhuma fonte encontrada: %s" % paths)
    size = find_size(fonts[0][0], target_caph)
    cps = sorted(set(range(32, 127)) | set(ord(c) for c in extra))
    glyphs = []
    for cp in cps:
        fpath = pick_font(cp, fonts)
        g = render_glyph(fpath, size, chr(cp))
        glyphs.append((cp, g))
    return size, glyphs

def emit(name, font, glyphs, out):
    allbits = bytearray()
    recs = []
    for cp, g in glyphs:
        off = len(allbits)
        allbits += g["bits"]
        recs.append((cp, off, g["adv"], g["w"], g["h"], g["left"], g["top"]))
    out.append("static const u8 %s_B[]={\n%s\n};" % (name, c_bytes(bytes(allbits))))
    lines = []
    for cp, off, adv, w, h, left, top in recs:
        lines.append(" {%d,%d,%d,%d,%d,%d,%d}," % (cp, off, adv, w, h, left, top))
    out.append("static const Glyph %s_G[]={\n%s\n};" % (name, "\n".join(lines)))
    out.append("const Font %s={%s_G,%s_B,%d,%d,%d};" % (name, name, name, len(recs), font["caph"], font["lineh"]))
    return recs, bytes(allbits)

def draw_line(px, x, y, s, gl, bits, caph, col):
    # espelha DrawG/Px_Text2 do source/px_font2.c p/ o preview bater 1:1
    bycp = {cp: (off, adv, w, h, left, top) for cp, off, adv, w, h, left, top in gl}
    cx = x
    for ch in s:
        cp = ord(ch)
        if cp == 10:
            cx = x; y += 0
            continue
        if cp not in bycp:
            cp = ord('?')
        off, adv, w, h, left, top = bycp[cp]
        stride = (w + 7) // 8
        base = y + caph
        for r in range(h):
            for c in range(w):
                if bits[off + r * stride + (c >> 3)] & (0x80 >> (c & 7)):
                    xx, yy2 = cx + left + c, base - top + r
                    if 0 <= xx < px.width and 0 <= yy2 < px.height:
                        px.putpixel((xx, yy2), col)
        cx += adv

def make_preview(r12, b12, r8, b8, path):
    W, H = 320, 250
    img = Image.new("L", (W, H), 6)
    yy = [6]

    def line(s, recs, bits, caph):
        draw_line(img, 6, yy[0], s, recs, bits, caph, 255)
        yy[0] += 14 if caph == 9 else 11

    yy[0] = 6
    for s in ["Novo Jogo", "Carregar", "Opções", "Créditos",
              "Escolha o Modo", "Escolha sua nação", "Configurações"]:
        line(s, r12, b12, 9)
    yy[0] += 4
    for s in ["Conquiste territórios, desenvolva sua nação e escreva sua história.",
              "Partida livre no mapa-múndi de 1800 ou num mundo aleatório.",
              "Ajuste sua experiência de jogo. Música Efeitos Idioma Tela",
              "◂ Europa ▸ ▶ ◆ ✦ … · —",
              "ÁÀÂÃÉÊ ÍÓÔÕÚÜÇ ŋ ơ ʉ ʻ ʿ ẽ ễ ệ Ọ ọ ā ē ī ō č š ž",
              "Dia 142 · Reino Unido · Normal"]:
        line(s, r8, b8, 7)
    img = img.resize((W * 2, H * 2), Image.NEAREST)
    img.save(path)
    print("preview ->", path)

def pixel8(extra):
    # Serifa 8 = fonte desenhada a mao (tools/pixfont8.py): legivel em 7px
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import pixfont8
    tab = pixfont8.glyphs()
    want = sorted(set(range(32, 127)) | set(ord(c) for c in extra))
    miss = [chr(c) for c in want if c not in tab]
    if miss:
        raise SystemExit("pixfont8 sem glifo: %r" % "".join(miss))
    out = []
    for cp in want:
        t = tab[cp]
        w, h = t["w"], t["h"]
        stride = (w + 7) // 8
        bits = bytearray(stride * h)
        for yy, row in enumerate(t["rows"]):
            for xx, c in enumerate(row):
                if c == '#':
                    bits[yy * stride + (xx >> 3)] |= (0x80 >> (xx & 7))
        out.append((cp, dict(w=w, h=h, left=t["left"], top=t["top"], adv=t["adv"], bits=bytes(bits))))
    return out

def main():
    extra = EXTRA + SYMBOLS
    s12, g12 = build([first(PRIMARY), first(FALLBACK)], 9, extra)
    s8, g8 = 0, pixel8(extra)
    out = []
    out.append("// GERADO por tools/genfont.py — NAO EDITE A MAO.")
    out.append("// Serifa 12 (caixa alta 9px) e Serifa 8 (caixa alta 7px), 1bpp.")
    out.append('#include "font.h"')
    r12, b12 = emit("F_SERIF12", dict(caph=9, lineh=14), g12, out)
    r8, b8 = emit("F_SERIF8", dict(caph=7, lineh=11), g8, out)
    path = os.path.join(ROOT, "source", "gen", "font_data.c")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print("gerado %s (serifa12 px=%d, serifa8 px=%d, %d glifos cada)" % (path, s12, s8, len(g12)))
    make_preview(r12, b12, r8, b8, os.path.join(ROOT, "docs", "font_preview.png"))

if __name__ == "__main__":
    main()
