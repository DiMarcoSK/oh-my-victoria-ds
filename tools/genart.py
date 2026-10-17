#!/usr/bin/env python3
# genart.py — gera source/gen/art_data.c: o LOGO e o RETRATO do oficial do menu
# principal (concept_arts/image.png), em pixel art indexada (4bpp, paleta
# fixa em 15 bits). Desenha a 4x com PIL e reduz para a paleta (media do bloco
# 4x4 -> cor mais proxima): bordas limpas, sem antisserrilhado sujo.
# O arquivo gerado e' COMMITADO (o build nao roda Python, ADR-14).
#   python tools/genart.py        (Windows: usa Bookman Old Style do sistema)
import os, math, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = ["C:/Windows/Fonts/", "/mnt/c/Windows/Fonts/"]
def font(name, size):
    for d in FONTS:
        if os.path.exists(d + name):
            return ImageFont.truetype(d + name, size)
    raise SystemExit("fonte nao encontrada: " + name)

S = 4  # super-amostragem

def reduce(img, pal, transparent=True):
    """img RGBA a S x -> indices na paleta: cada subpixel vira a cor mais
    proxima e o bloco SxS fica com a mais votada (sem misturar tinta e creme
    numa terceira cor). Bloco quase vazio = transparente."""
    w, h = img.size[0] // S, img.size[1] // S
    px = img.load()
    cache = {}
    def near(c):
        if c in cache: return cache[c]
        best, bd = 1, 1 << 30
        for k, q in enumerate(pal):
            if k == 0 and transparent: continue
            d = (q[0] - c[0]) ** 2 + (q[1] - c[1]) ** 2 + (q[2] - c[2]) ** 2
            if d < bd: bd, best = d, k
        cache[c] = best
        return best
    out = []
    for y in range(h):
        row = []
        for x in range(w):
            votes = {}
            empty = 0
            for j in range(S):
                for i in range(S):
                    p = px[x * S + i, y * S + j]
                    if transparent and p[3] < 128:
                        empty += 1; continue
                    k = near(p[:3]); votes[k] = votes.get(k, 0) + 1
            if transparent and empty * 10 >= S * S * 6:
                row.append(0); continue
            row.append(max(votes.items(), key=lambda kv: kv[1])[0])
        out.append(row)
    return w, h, out

# ------------------------------------------------------------------ logo
LOGO_PAL = [(0, 0, 0), (28, 22, 34), (246, 236, 210), (200, 176, 136), (214, 44, 36),
            (140, 22, 22), (255, 252, 240), (10, 8, 16)]

def logo():
    W, H = 216 * S, 110 * S
    base = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    # pincelada vermelha (atras do "Victoria"), borda irregular
    sw = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(sw)
    rnd = random.Random(7)
    top, bot = [], []
    x0, x1 = 26 * S, 206 * S
    for k in range(41):
        t = k / 40
        x = x0 + (x1 - x0) * t
        yc = (88 - 15 * t) * S
        th = (2 + 10 * math.sin(math.pi * min(1, t * 1.1))) * S
        top.append((x, yc - th / 2 + rnd.randint(-S, S)))
        bot.append((x, yc + th / 2 + rnd.randint(-S, S)))
    d.polygon(top + bot[::-1], fill=LOGO_PAL[4] + (255,))
    for k in range(0, 40, 3):  # fios secos do pincel
        a, b = top[k], top[min(40, k + 3)]
        d.line([a, b], fill=LOGO_PAL[5] + (255,), width=S)
    # texto: "Oh My" e "Victoria" levemente subindo para a direita
    txt = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    td = ImageDraw.Draw(txt)
    f1, f2 = font("BOOKOSB.TTF", 27 * S), font("BOOKOSB.TTF", 46 * S)
    w1 = td.textlength("Oh My", font=f1)
    td.text(((W - w1) / 2 - 6 * S, 2 * S), "Oh My", font=f1, fill=(255, 255, 255, 255))
    w2 = td.textlength("Victoria", font=f2)
    td.text(((W - w2) / 2, 26 * S), "Victoria", font=f2, fill=(255, 255, 255, 255))
    txt = txt.rotate(4, resample=Image.BICUBIC, center=(W / 2, H / 2))
    m = txt.split()[3]
    # contorno grosso de tinta + sombra deslocada
    out = m.filter(ImageFilter.MaxFilter(2 * S + 1))
    shadow = Image.new("L", (W, H), 0); shadow.paste(out, (S * 2, S * 3))
    canvas = Image.alpha_composite(base, sw)
    canvas.paste(Image.new("RGBA", (W, H), LOGO_PAL[7] + (255,)), (0, 0), shadow)
    canvas.paste(Image.new("RGBA", (W, H), LOGO_PAL[1] + (255,)), (0, 0), out)
    # recheio creme com sombra na metade de baixo de cada linha + brilho no topo
    fill = Image.new("RGBA", (W, H), LOGO_PAL[2] + (255,))
    fd = ImageDraw.Draw(fill)
    for (y0, y1) in ((2 * S, 30 * S), (26 * S, 80 * S)):
        mid = y0 + (y1 - y0) * 62 // 100
        fd.rectangle([0, mid, W, y1], fill=LOGO_PAL[3] + (255,))
    canvas.paste(fill, (0, 0), m)
    # "DS" vermelho em italico, embaixo a direita
    ds = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    dd = ImageDraw.Draw(ds)
    f3 = font("BOOKOSBI.TTF", 26 * S)
    dd.text((150 * S, 78 * S), "DS", font=f3, fill=(255, 255, 255, 255))
    dm = ds.split()[3]
    dout = dm.filter(ImageFilter.MaxFilter(2 * S + 1))
    canvas.paste(Image.new("RGBA", (W, H), LOGO_PAL[1] + (255,)), (0, 0), dout)
    canvas.paste(Image.new("RGBA", (W, H), LOGO_PAL[4] + (255,)), (0, 0), dm)
    return reduce(canvas, LOGO_PAL)

# ------------------------------------------------------------------ retrato
PORT_PAL = [(0, 0, 0), (12, 14, 26), (22, 28, 48), (34, 44, 72), (52, 66, 100),
            (232, 196, 160), (196, 150, 118), (140, 96, 74), (60, 40, 32),
            (226, 184, 80), (160, 120, 44), (250, 240, 214), (170, 30, 28),
            (24, 20, 20), (96, 104, 120), (6, 6, 10)]

def portrait():
    W, H = 76 * S, 136 * S
    im = Image.new("RGBA", (W, H), PORT_PAL[2] + (255,))
    d = ImageDraw.Draw(im)
    def P(c): return PORT_PAL[c] + (255,)
    def sc(pts): return [(x * S, y * S) for x, y in pts]
    # fundo: noite azul com janela clara (luz vinda da esquerda)
    for y in range(H):
        t = y / H
        c = tuple(int(PORT_PAL[3][k] * (1 - t) + PORT_PAL[1][k] * t) for k in range(3))
        d.line([(0, y), (W, y)], fill=c + (255,))
    d.rectangle([4 * S, 8 * S, 22 * S, 52 * S], fill=P(4))
    d.line([(13 * S, 8 * S), (13 * S, 52 * S)], fill=P(2), width=S)
    d.line([(4 * S, 30 * S), (22 * S, 30 * S)], fill=P(2), width=S)
    # tronco: casaca azul-marinho de ombros largos
    d.polygon(sc([(2, 136), (6, 92), (18, 80), (30, 76), (46, 76), (58, 80), (70, 92), (74, 136)]), fill=P(2))
    d.polygon(sc([(30, 76), (38, 96), (46, 76)]), fill=P(1))
    d.polygon(sc([(38, 96), (34, 136), (42, 136)]), fill=P(1))
    # dragonas douradas
    for cx in (14, 62):
        d.ellipse(sc([(cx - 9, 76), (cx + 9, 86)]), fill=P(9))
        for k in range(-8, 9, 3):
            d.line(sc([(cx + k, 84), (cx + k, 92)]), fill=P(10), width=S)
    # gola alta dourada, faixa vermelha, botoes
    d.polygon(sc([(28, 70), (48, 70), (50, 80), (38, 86), (26, 80)]), fill=P(9))
    d.polygon(sc([(48, 92), (58, 92), (26, 136), (14, 136)]), fill=P(12))
    for y in range(98, 134, 8):
        d.ellipse(sc([(36, y), (40, y + 3)]), fill=P(9))
    # pescoco e rosto
    d.rectangle(sc([(32, 60), (44, 72)]), fill=P(6))
    d.ellipse(sc([(24, 26), (52, 66)]), fill=P(5))
    d.polygon(sc([(24, 44), (30, 64), (38, 68), (24, 58)]), fill=P(6))       # sombra da face
    d.ellipse(sc([(22, 40), (27, 50)]), fill=P(6))                         # orelha
    # costeletas, bigode e sobrancelhas
    d.polygon(sc([(24, 34), (28, 34), (28, 54), (24, 50)]), fill=P(8))
    d.polygon(sc([(48, 34), (52, 34), (52, 50), (48, 54)]), fill=P(8))
    d.polygon(sc([(31, 55), (38, 52), (45, 55), (44, 58), (38, 56), (32, 58)]), fill=P(8))
    d.line(sc([(30, 40), (35, 39)]), fill=P(8), width=2 * S)
    d.line(sc([(41, 39), (46, 40)]), fill=P(8), width=2 * S)
    d.rectangle(sc([(31, 43), (34, 45)]), fill=P(13))
    d.rectangle(sc([(42, 43), (45, 45)]), fill=P(13))
    d.line(sc([(38, 45), (37, 51), (39, 51)]), fill=P(7), width=S)
    d.line(sc([(34, 61), (42, 61)]), fill=P(7), width=S)
    # quepe: copa azul, faixa dourada, pala preta, insignia
    d.ellipse(sc([(20, 10), (56, 30)]), fill=P(2))
    d.rectangle(sc([(22, 20), (54, 30)]), fill=P(2))
    d.rectangle(sc([(22, 24), (54, 30)]), fill=P(9))
    d.polygon(sc([(22, 30), (54, 30), (50, 36), (26, 36)]), fill=P(15))
    d.ellipse(sc([(35, 16), (41, 23)]), fill=P(9))
    d.ellipse(sc([(36, 17), (40, 22)]), fill=P(12))
    # luz de contorno (janela a esquerda)
    d.line(sc([(25, 38), (25, 52)]), fill=P(11), width=S)
    d.line(sc([(7, 94), (16, 82)]), fill=P(4), width=S)
    return reduce(im, PORT_PAL, transparent=False)

def pack(name, w, h, rows):
    # RLE do px_art.c: pares (count,indice), linha a linha, count <= 255
    flat = [v for r in rows for v in r]
    data = bytearray(); i = 0
    while i < len(flat):
        v = flat[i]; n = 1
        while i + n < len(flat) and flat[i + n] == v and n < 255:
            n += 1
        data += bytes((n, v)); i += n
    lines = [",".join("0x%02X" % v for v in data[i:i + 20]) + "," for i in range(0, len(data), 20)]
    return ("static const u8 %s_D[]={\n%s\n};\nconst Art %s={%d,%d,\"%s\",%s_P,%s_D};\n"
            % (name, "\n".join(lines), name, w, h, name.lower(), name, name))

def pal(name, p):
    return "static const u16 %s_P[16]={%s};\n" % (name, ",".join(
        "PXC(%d,%d,%d)" % (c[0] >> 3, c[1] >> 3, c[2] >> 3) for c in (p + [(0, 0, 0)] * 16)[:16]))

def preview(items, path):
    W = sum(w for _, w, h, _, _ in items) + 10 * (len(items) + 1)
    H = max(h for _, w, h, _, _ in items) + 20
    img = Image.new("RGB", (W, H), (20, 22, 32))
    x = 10
    for _, w, h, rows, p in items:
        for y in range(h):
            for i in range(w):
                k = rows[y][i]
                if k: img.putpixel((x + i, 10 + y), p[k])
        x += w + 10
    img.resize((W * 3, H * 3), Image.NEAREST).save(path)

def main():
    lw, lh, lr = logo()
    pw, ph, pr = portrait()
    out = ['// GERADO por tools/genart.py — NAO EDITE A MAO.',
           '// Logo (Bookman Old Style Bold + pincelada) e retrato do oficial, 4bpp indexado.',
           '#include "px_art.h"', '#include "pal.h"']
    out.append(pal("ART_LOGO", LOGO_PAL)); out.append(pack("ART_LOGO", lw, lh, lr))
    out.append(pal("ART_OFFICER", PORT_PAL)); out.append(pack("ART_OFFICER", pw, ph, pr))
    path = os.path.join(ROOT, "source", "gen", "art_data.c")
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    preview([("logo", lw, lh, lr, LOGO_PAL), ("oficial", pw, ph, pr, PORT_PAL)],
            os.path.join(ROOT, "docs", "art_preview.png"))
    print("gerado", path, (lw, lh), (pw, ph))

if __name__ == "__main__":
    main()
