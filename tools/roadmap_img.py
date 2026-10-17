#!/usr/bin/env python3
# roadmap_img.py — gera docs/roadmap.png a partir do docs/ROADMAP.md.
#
# Nao faz parte do build do jogo: e uma ferramenta de docs (roda no PC, com
# Pillow). Edite ESTE script e rode de novo, nunca o PNG:
#
#     pip install pillow        # ou: pip install --target ./pylibs pillow
#     python3 tools/roadmap_img.py
#
# Paleta = tokens de UI do SDD sec 14.1 (navy/gold/pergaminho/crimson/verde).
# Sem dithering, sem gradiente: chapado, como o DS pinta.
import os, sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("Falta o Pillow: pip install pillow (ou PYTHONPATH=./pylibs)")

OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "roadmap.png")

# --- paleta (SDD sec 14.1) ---
BG        = (18, 24, 38)     # C_NAVY_BG   #121826
NAVY      = (30, 39, 56)     # C_NAVY      #1E2738
NAVY_HI   = (44, 56, 80)     # C_NAVY_HI
PARCH     = (228, 210, 166)  # C_PARCH
PARCH_SH  = (196, 172, 122)  # C_PARCH_SH
INK       = (42, 32, 24)     # C_INK
CRIMSON   = (154, 28, 31)    # C_CRIMSON
CRIMSON_HI= (194, 48, 46)    # C_CRIMSON_HI
GOLD      = (200, 160, 74)   # C_GOLD
GOLD_DIM  = (126, 101, 50)   # C_GOLD_DIM
TEXT      = (239, 230, 210)  # C_TEXT
TEXT_DIM  = (138, 143, 153)  # C_TEXT_DIM
GREEN     = (56, 112, 66)    # "feito" (nao e token de UI; e status)

# --- fontes (Georgia no Windows/WSL; cai p/ DejaVu em Linux puro) ---
_FONTS = {
    "serif":  ["/mnt/c/Windows/Fonts/georgia.ttf",
               "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"],
    "serifb": ["/mnt/c/Windows/Fonts/georgiab.ttf",
               "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf"],
    "serifi": ["/mnt/c/Windows/Fonts/georgiai.ttf",
               "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Italic.ttf"],
    "mono":   ["/mnt/c/Windows/Fonts/consola.ttf",
               "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"],
}
_cache = {}
def F(size, style="serif"):
    key = (size, style)
    if key not in _cache:
        for path in _FONTS[style]:
            if os.path.exists(path):
                _cache[key] = ImageFont.truetype(path, size)
                break
        else:
            _cache[key] = ImageFont.load_default()
    return _cache[key]

W, H = 1800, 1180
M = 46                      # margem externa
img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)

def text(x, y, s, font, fill, anchor="la"):
    d.text((x, y), s, font=font, fill=fill, anchor=anchor)

def wrap(s, font, maxw):
    words, lines, cur = s.split(), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if d.textlength(t, font=font) <= maxw or not cur:
            cur = t
        else:
            lines.append(cur); cur = w
    if cur: lines.append(cur)
    return lines

def panel(x, y, w, h, fill, border, bw=2):
    d.rectangle([x, y, x + w, y + h], fill=fill, outline=border, width=bw)

def chevron(x, y, h, col=GOLD):
    # pequeno ">" dourado entre fases
    cy = y + h // 2
    d.line([(x, cy - 7), (x + 7, cy), (x, cy + 7)], fill=col, width=3, joint="curve")

# ---------------- moldura ----------------
panel(M - 24, M - 24, W - 2 * (M - 24), H - 2 * (M - 24), None, GOLD, 4)
d.rectangle([M - 16, M - 16, W - M + 16, H - M + 16], outline=GOLD_DIM, width=1)

# ---------------- cabecalho ----------------
text(M + 4, M - 8, "OH MY VICTORIA DS!", F(58, "serifb"), GOLD)
text(M + 6, M + 66, "ROADMAP — o mundo inteiro, de 1800 a 2025, num Nintendo DS",
     F(25, "serifi"), PARCH_SH)
text(W - M, M + 6, "branch feat/mundo · 2026-10-16", F(19), TEXT_DIM, anchor="ra")
text(W - M, M + 34, "build: make verde · ohmyvictoria_ds.nds 158 KB", F(19), TEXT_DIM, anchor="ra")

d.line([(M + 4, M + 104), (W - M, M + 104)], fill=GOLD_DIM, width=2)

# ---------------- legenda ----------------
lx = M + 4
ly = M + 120
for label, col in (("Feito", GREEN), ("Em andamento", CRIMSON), ("Planejado", NAVY)):
    d.rectangle([lx, ly, lx + 18, ly + 18], fill=col, outline=GOLD_DIM, width=1)
    text(lx + 28, ly - 2, label, F(19), TEXT)
    lx += 28 + int(d.textlength(label, font=F(19))) + 34
text(W - M, ly - 2, "workflow:  E0 » v0.9 (F1–F8) » v1.x (M1–M7)", F(19, "serifi"), PARCH_SH, anchor="ra")

# ---------------- faixa "estado atual" ----------------
sy = ly + 44
text(M + 4, sy, "ESTADO ATUAL", F(22, "serifb"), GOLD)
d.line([(M + 190, sy + 14), (W - M, sy + 14)], fill=GOLD_DIM, width=1)

cards = [
    ("v0.7", "sociedade + governos", "JOGÁVEL", GREEN),
    ("v0.8", "touch-first / mapa cozido", "EM ANDAMENTO", CRIMSON),
    ("núcleo", "calendar · traits · nations · history · world", "NO CÓDIGO, SEM TELA", CRIMSON),
    ("v0.9", "UI da concept art", "F1 EM ANDAMENTO", CRIMSON),
    ("v1.x", "o mundo (1800–2025)", "M1 NO NÚCLEO", NAVY),
]
cy = sy + 34
ch = 78
n = len(cards)
gap = 16
bw = (W - 2 * M - (n - 1) * gap) // n
for i, (tag, sub, status, col) in enumerate(cards):
    x = M + i * (bw + gap)
    panel(x, cy, bw, ch, NAVY, GOLD_DIM, 2)
    panel(x + 8, cy + 8, 6, ch - 16, col, col, 1)          # barra de status
    text(x + 26, cy + 12, tag, F(21, "serifb"), GOLD)
    for j, ln in enumerate(wrap(sub, F(15), bw - 40)[:2]):
        text(x + 26, cy + 40 + j * 18, ln, F(15), TEXT)
    text(x + 26, cy + ch - 20, status, F(13, "serifb"), col if col != NAVY else TEXT_DIM)

# ---------------- bandas de fase ----------------
def band(y, title, sub, items, col_border, bh=150):
    text(M + 4, y, title, F(22, "serifb"), GOLD)
    tw = int(d.textlength(title, font=F(22, "serifb")))
    text(M + 20 + tw, y + 4, sub, F(17, "serifi"), PARCH_SH)
    by = y + 34
    m = len(items)
    gap = 22
    bw = (W - 2 * M - (m - 1) * gap) // m
    for i, (code, name, detail, status) in enumerate(items):
        x = M + i * (bw + gap)
        fill = NAVY
        bcol = GOLD_DIM
        if status == "wip":
            bcol = CRIMSON
        elif status == "done":
            bcol = GREEN
        panel(x, by, bw, bh, fill, bcol, 2)
        text(x + 12, by + 10, code, F(19, "serifb"), GOLD)
        cw = int(d.textlength(code, font=F(19, "serifb")))
        for j, ln in enumerate(wrap(name, F(15), bw - cw - 26)[:2]):
            text(x + 14 + cw, by + 12 + j * 18, ln, F(15, "serifb"), TEXT)
        d.line([(x + 12, by + 52), (x + bw - 12, by + 52)], fill=GOLD_DIM, width=1)
        for j, ln in enumerate(wrap(detail, F(14), bw - 24)[:4]):
            text(x + 12, by + 62 + j * 19, ln, F(14), TEXT_DIM)
        if i < m - 1:
            chevron(x + bw + (gap - 7) // 2, by, bh)

y0 = cy + ch + 42
bh0 = 116
band(y0, "E0 · fechar v0.8", "documentar + saneamento antes da v0.9", [
    ("E0.1", "Playtest melonDS", "pan, abas de caneta, 60fps, sem travamento", "plan"),
    ("E0.2", "Documentar v0.8", "SDD sec 1 feito; falta sec 9 + diagramas", "wip"),
    ("E0.3", "Saneamento de docs", "nada de arquivo/seção que não existe", "done"),
    ("E0.4", "Build + SIM", "make verde ok; 200 dias + replay a confirmar", "wip"),
], GOLD, bh=bh0)

y1 = y0 + 34 + bh0 + 42
band(y1, "E1 · v0.9 (F1–F8)", "a concept art vira tela (SDD sec 14)", [
    ("F1", "Fundação", "feitos: pal.h, Serifa, str.c, g->screen, Settings", "wip"),
    ("F2", "Menus", "layouts 14.4/14.5 · melonDS ±4px", "plan"),
    ("F3", "HUD", "recursos, minimapa, 4 ações, lcdSwap", "plan"),
    ("F4", "Arte", "png2c.py, PNGs P1/P2, arte do mapa", "plan"),
    ("F5", "Países (ponte)", "traits + bandeiras nas 3 facções atuais", "plan"),
    ("F6", "Áudio", "MaxMod + sliders", "plan"),
    ("F7", "Multijogador", "2 DS sem fio, lockstep", "plan"),
    ("F8", "SRAM", "save + Settings persistentes, 3 slots", "plan"),
], GOLD)

y2 = y1 + 34 + 150 + 42
band(y2, "E2 · v1.x — O Mundo (M1–M7)", "mapa celular Victoria 3 + frontwars.io, história viva (DESIGN sec 4–9)",
     [
         ("M1", "Células e frentes", "guerra por frente no mapa pequeno, 60fps", "plan"),
         ("M2", "Mapa estilo Victoria", "zoom Z0–Z3, papel » terreno, modos de mapa", "plan"),
         ("M3", "Catálogo", "ficha, bandeira, traits, ≥40 países", "plan"),
         ("M4", "Mapa-múndi 1800", "≥150 entidades, 15% por continente", "plan"),
         ("M5", "História viva", "eventos ancorados, eras, alternativas", "plan"),
         ("M6", "Até 2025", "países modernos, balanço, Fim da história", "plan"),
         ("M7", "Diversão", "Ambições, Os Primeiros, Gazeta, Modo Eterno", "plan"),
     ], GOLD)

# ---------------- rodape ----------------
fy = y2 + 34 + 150 + 30
d.line([(M + 4, fy), (W - M, fy)], fill=GOLD_DIM, width=1)
text(M + 4, fy + 12,
     "Docs: DESIGN.md (design) · docs/SDD.md sec 14 (telas) · docs/ROADMAP.md (ordem) · AGENTS.md (pronto)",
     F(18), PARCH_SH)
text(W - M, fy + 12, "imagem gerada por tools/roadmap_img.py", F(18), TEXT_DIM, anchor="ra")

os.makedirs(os.path.dirname(OUT), exist_ok=True)
img.save(OUT)
print("ok ->", os.path.normpath(OUT), f"({W}x{H})")
