#!/usr/bin/env python3
# genpics.py — gera source/gen/pic_data.c: as PINTURAS dos menus tiradas da
# propria concept art (concept_arts/image.png), em resolucao nativa do DS.
# Cada tela da concept tem ~290x200 px (1,13x a do DS): recortamos a regiao,
# reamostramos com Lanczos, devolvemos o contorno com um unsharp leve e
# gravamos em COR DIRETA de 15 bits (u16 com BIT(15); 0 = transparente) com
# pontilhado Bayer 4x4 na passagem de 8 p/ 5 bits por canal: nada de paleta
# de 255 cores (que fazia degrau no ceu do por do sol). Blit por linha, sem
# RLE. O arquivo gerado e' COMMITADO (build sem Python).
#   python tools/genpics.py      (preview em docs/pics_preview.png)
import os
import numpy as np
from PIL import Image, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = Image.open(os.path.join(ROOT, "concept_arts", "image.png")).convert("RGB")
K = 256 / 290  # escala concept -> DS (uma tela da concept tem ~290 px de largura)

def crop(box, size=None, scale=K, sharp=True):
    c = SRC.crop(box)
    if size is None:
        size = (max(1, round(c.width * scale)), max(1, round(c.height * scale)))
    r = c.resize(size, Image.LANCZOS)
    # a reducao amacia o pixel art da concept: devolve o contorno sem halo
    return r.filter(ImageFilter.UnsharpMask(radius=1.0, percent=70, threshold=2)) if sharp else r

def lum(a):
    return a[..., 0] * 0.3 + a[..., 1] * 0.59 + a[..., 2] * 0.11

# ---------------------------------------------------------------- quantizacao
BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]])
def to_pic(img, mask=None, colors=None):
    """img RGB -> (w,h,px15): u16 aBBBBBGGGGGRRRRR com BIT(15); mask -> 0."""
    a = np.array(img).astype(int)
    h, w = a.shape[:2]
    t = BAYER[np.arange(h)[:, None] % 4, np.arange(w)[None, :] % 4]
    # 8 -> 5 bits com limiar ordenado (degrade liso no hardware de 15 bits)
    q = (a * 31 + (t[..., None] * 255) // 16) // 255
    q = np.clip(q, 0, 31)
    px = 0x8000 | (q[..., 2] << 10) | (q[..., 1] << 5) | q[..., 0]
    if mask is not None:
        px = np.where(mask, 0, px)
    return w, h, px

def ink_mask(img, keep=0.86):
    """gravura sobre papel: papel claro vira transparente (desenha so' a tinta)."""
    a = np.array(img).astype(float)
    L = lum(a)
    paper = np.percentile(L, 80)
    return L >= paper * keep

def gold_mask(img):
    """ornamento dourado sobre navy: o navy vira transparente."""
    a = np.array(img).astype(float)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    return (lum(a) < 70) | ((r - b) < 25)

# ---------------------------------------------------------------- papel
def paper(w=256, h=192, seed=7):
    """papel da concept (217,186,144) com manchas de baixa frequencia e grao."""
    rng = np.random.default_rng(seed)
    base = np.array([217, 186, 144], float)
    f = np.zeros((h, w))
    for gw, amp in ((4, 7.0), (9, 5.0), (20, 3.0)):
        gh = max(2, gw * 3 // 4)
        g = Image.fromarray(((rng.random((gh, gw)) - 0.5) * 2 * 127 + 128).astype(np.uint8), "L")
        g = np.array(g.resize((w, h), Image.BICUBIC)).astype(float)
        f += (g - 128) / 127 * amp
    f += rng.normal(0, 2.2, (h, w))
    # manchas de cha' raras
    for _ in range(9):
        cx, cy, r = rng.integers(0, w), rng.integers(0, h), rng.integers(6, 18)
        yy, xx = np.mgrid[0:h, 0:w]
        d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / r
        f -= np.clip(1 - d, 0, 1) ** 2 * rng.uniform(4, 9)
    a = base[None, None, :] + f[..., None] * np.array([1.0, 0.92, 0.8])[None, None, :]
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGB")

# ---------------------------------------------------------------- recortes
PICS = []

def add(name, pic, desc):
    PICS.append((name, pic, desc))

# 1) titulo (cima, menu principal): Westminster ao por do sol + logo + botao
t = SRC.crop((381, 49, 649, 250)).copy()
# apaga o texto "Aperte A" do botao (o jogo escreve no idioma escolhido)
a = np.array(t)
ref = a[176, 196].copy()                       # navy do botao
a[166:191, 186:262] = ref
t = Image.fromarray(a).resize((256, 192), Image.LANCZOS)
add("PIC_TITLE", to_pic(t), "titulo: por do sol, logo e botao (texto pelo jogo)")
# 2) retrato do oficial (menu principal, baixo)
add("PIC_OFFICER", to_pic(crop((379, 314, 456, 462))), "retrato do oficial")
# 3) cidade gotica (selecao de modo, baixo)
add("PIC_TOWN", to_pic(crop((788, 336, 886, 446))), "gravura colorida da cidade")
# 4) skyline em sepia (pausa, baixo): so' a tinta
s = crop((92, 860, 348, 932))
add("PIC_SKETCH", to_pic(s, ink_mask(s), 48), "porto em sepia (tinta sobre papel)")
# 5) brasao com leoes (configuracoes, baixo)
c = crop((553, 798, 677, 885))
add("PIC_CREST", to_pic(c, ink_mask(c, 0.80), 48), "brasao com leoes")
# 6) porto com estandarte (selecao de nacao, baixo). A bandeira pintada vira
#    uma MASCARA de sombra: o jogo pinta ali a bandeira da nacao escolhida,
#    modulada pelas dobras da pintura (FLAG_X/Y/W/H, FLAG_SH 0 = nao e' pano).
hb = crop((870, 797, 1140, 936), (256, 132))
# contorno do pano (medido na pintura reamostrada): borda de cima ondula,
# mastro a' esquerda, a cabeca do soldado (escura) tapa a barra de baixo
FX, FY, FW, FH = 7, 16, 64, 48
TOP = [(0, 17), (21, 19), (43, 22), (63, 24)]
BOT = 63
def top_at(x):
    for (x0, y0), (x1, y1) in zip(TOP, TOP[1:]):
        if x0 <= x <= x1:
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return TOP[-1][1]
ha = np.array(hb).astype(float)
REF = {"r": (190, 40, 40), "w": (225, 220, 205), "b": (40, 60, 105)}
cls = {}
for y in range(FH):
    for x in range(FW):
        gy = FY + y
        if gy < top_at(x) or gy > BOT:
            continue
        r, g, b = ha[gy, FX + x]
        L = r * 0.3 + g * 0.59 + b * 0.11
        if L < 64 and (b < r + 18 or y > 30):
            continue                                  # cabeca/sombra do soldado
        k = min(REF, key=lambda c: sum((v - w) ** 2 for v, w in zip((r, g, b), REF[c])))
        cls[(x, y)] = (k, L)
mean = {k: np.mean([L for (kk, L) in cls.values() if kk == k] or [1]) for k in REF}
shade = np.zeros((FH, FW), int)
for (x, y), (k, L) in cls.items():
    shade[y, x] = int(max(3, min(15, round(12 * L / mean[k]))))
FTOP = [max(0, int(round(top_at(x))) - FY) for x in range(FW)]
FLAGMAP = (FX, FY, FW, FH, shade)
add("PIC_HARBOR", to_pic(hb), "porto com estandarte")
# 7) papel (fundo de todos os pergaminhos)
add("PIC_PAPER", to_pic(paper()), "papel da concept, sem moldura")
# 8) cantos do pergaminho (ornamento de tinta) e da moldura navy (ouro)
for nm, box in (("PIC_PC_TL", (479, 784, 503, 806)), ("PIC_PC_TR", (726, 784, 750, 806)),
                ("PIC_PC_BL", (479, 923, 503, 945)), ("PIC_PC_BR", (726, 923, 750, 945))):
    im = crop(box)
    add(nm, to_pic(im, ink_mask(im, 0.82), 16), "canto do pergaminho")
for nm, box in (("PIC_NC_TL", (366, 302, 392, 326)), ("PIC_NC_TR", (636, 302, 662, 326)),
                ("PIC_NC_BL", (366, 452, 392, 476)), ("PIC_NC_BR", (636, 452, 662, 476))):
    im = crop(box)
    add(nm, to_pic(im, gold_mask(im), 16), "canto dourado da moldura navy")
# 9) coroa dourada (rodape do menu principal)
im = crop((501, 455, 529, 474))
add("PIC_CROWN", to_pic(im, gold_mask(im), 16), "coroa do rodape")

# ---------------------------------------------------------------- saida C
def c_arr(vals, per=24):
    out = []
    for i in range(0, len(vals), per):
        out.append(",".join(str(v) for v in vals[i:i + per]) + ",")
    return "\n".join(out)

def main():
    out = ["// GERADO por tools/genpics.py — NAO EDITE A MAO.",
           "// Pinturas dos menus recortadas da concept art (cor direta 15 bits, 0 = transparente).",
           '#include "px_art.h"']
    total = 0
    for name, (w, h, px), desc in PICS:
        out.append("// %s: %s (%dx%d)" % (name, desc, w, h))
        flat = px.flatten().tolist()
        lines = []
        for i in range(0, len(flat), 16):
            lines.append(",".join("0x%04X" % v for v in flat[i:i + 16]) + ",")
        out.append("static const u16 %s_D[%d]={\n%s\n};" % (name, len(flat), "\n".join(lines)))
        out.append("const Pic %s={%d,%d,%s_D};" % (name, w, h, name))
        total += 2 * len(flat)
    fx, fy, fw, fh, sh = FLAGMAP
    out.append("// sombra da bandeira pintada no PIC_HARBOR (0 = nao e' pano; 12 = luz normal)")
    out.append("const u8 HARBOR_FLAG[%d]={\n%s\n};" % (fw * fh, c_arr(sh.flatten().tolist())))
    out.append("const u8 HARBOR_FLAG_BOX[4]={%d,%d,%d,%d};" % (fx, fy, fw, fh))
    out.append("const u8 HARBOR_FLAG_TOP[%d]={%s};" % (fw, ",".join(str(v) for v in FTOP)))
    path = os.path.join(ROOT, "source", "gen", "pic_data.c")
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print("gerado %s (%d imagens, %.1f KB)" % (path, len(PICS), total / 1024))
    # preview: cada imagem sobre magenta (transparente)
    W = 4 + sum(p[1][0] + 4 for p in PICS[:7])
    sheet = Image.new("RGB", (W, 200 + 40), (255, 0, 255))
    x = 4
    for i, (name, (w, h, px), _) in enumerate(PICS):
        rgb = np.zeros((h, w, 3), np.uint8)
        rgb[..., 0] = (px & 31) << 3
        rgb[..., 1] = ((px >> 5) & 31) << 3
        rgb[..., 2] = ((px >> 10) & 31) << 3
        rgb[px == 0] = (255, 0, 255)
        im = Image.fromarray(rgb, "RGB")
        if i < 7:
            sheet.paste(im, (x, 4)); x += w + 4
        else:
            sheet.paste(im, (4 + (i - 7) * 30, 204))
    sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
    sheet.save(os.path.join(ROOT, "docs", "pics_preview.png"))

if __name__ == "__main__":
    main()
