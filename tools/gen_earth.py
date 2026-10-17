#!/usr/bin/env python3
"""gen_earth.py — rasteriza a terra do Natural Earth na grade do mapa-mundi.

Uso:  python tools/gen_earth.py ne_50m_land.geojson source/earth_mask.c [preview.png]

Entrada: ne_50m_land.geojson (Natural Earth, dominio publico):
  https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_50m_land.geojson
Saida: source/earth_mask.c (COMMITADO — o build nao roda Python, ADR-14).

Grade: 512x192 celulas de ~0,703 grau, de 80N a 55S (DESIGN.md sec 4.1).
Celula e terra se o seu centro esta dentro de algum poligono (par-impar por
poligono, unido entre poligonos). Ilhas menores que uma celula ganham a celula
onde estao seus vertices, para Havai, Tonga e Taiti existirem no mapa.
Formato: por linha, comprimentos de corrida alternados comecando por agua
(u8; corrida > 255 vira 255,0,...). EARTH_ROW[y] = offset da linha em EARTH_RLE.
"""
import json
import sys

W, H = 512, 192
LAT_TOP, LAT_SPAN = 80.0, 135.0


def lat_of(y):
    return LAT_TOP - (y + 0.5) * LAT_SPAN / H


def lon_of(x):
    return -180.0 + (x + 0.5) * 360.0 / W


def cell_of(lon, lat):
    x = int((lon + 180.0) * W / 360.0)
    y = int((LAT_TOP - lat) * H / LAT_SPAN)
    return min(max(x, 0), W - 1), y


def polygons(gj):
    for feat in gj["features"]:
        g = feat["geometry"]
        if g["type"] == "Polygon":
            yield g["coordinates"]
        elif g["type"] == "MultiPolygon":
            for p in g["coordinates"]:
                yield p


def raster_polygon(rings, grid):
    """Preenche grid[y][x] com par-impar sobre todos os aneis do poligono."""
    lats = [p[1] for r in rings for p in r]
    y0 = max(0, int((LAT_TOP - max(lats)) * H / LAT_SPAN) - 1)
    y1 = min(H - 1, int((LAT_TOP - min(lats)) * H / LAT_SPAN) + 1)
    filled = 0
    for y in range(y0, y1 + 1):
        lat = lat_of(y)
        xs = []
        for r in rings:
            n = len(r)
            for i in range(n - 1):
                (ax, ay), (bx, by) = r[i][:2], r[i + 1][:2]
                if (ay > lat) != (by > lat):
                    xs.append(ax + (lat - ay) * (bx - ax) / (by - ay))
        xs.sort()
        for k in range(0, len(xs) - 1, 2):
            a, b = xs[k], xs[k + 1]
            xa = max(0, int((a + 180.0) * W / 360.0 - 0.5) + 1)
            xb = min(W - 1, int((b + 180.0) * W / 360.0 - 0.5))
            for x in range(xa, xb + 1):
                if a <= lon_of(x) <= b and not grid[y][x]:
                    grid[y][x] = 1
                    filled += 1
    return filled


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    gj = json.load(open(sys.argv[1], encoding="utf-8"))
    grid = [[0] * W for _ in range(H)]
    small = 0
    for rings in polygons(gj):
        n = raster_polygon(rings, grid)
        if n < 2:  # ilha pequena: marca as celulas dos vertices
            for p in rings[0]:
                x, y = cell_of(p[0], p[1])
                if 0 <= y < H:
                    grid[y][x] = 1
            small += 1

    rle, rows = [], []
    for y in range(H):
        rows.append(len(rle))
        cur, run = 0, 0
        for x in range(W):
            v = grid[y][x]
            if v == cur:
                run += 1
            else:
                while run > 255:
                    rle += [255, 0]
                    run -= 255
                rle.append(run)
                cur, run = v, 1
        while run > 255:
            rle += [255, 0]
            run -= 255
        rle.append(run)

    land = sum(map(sum, grid))
    out = open(sys.argv[2], "w", encoding="utf-8", newline="\n")
    out.write("// earth_mask.c — GERADO por tools/gen_earth.py. NAO EDITE A MAO.\n")
    out.write("// Fonte: Natural Earth 1:50m land (dominio publico, naturalearthdata.com).\n")
    out.write(f"// Grade {W}x{H}, 80N..55S; {land} celulas de terra; {small} ilhas pequenas.\n")
    out.write('#include "earth.h"\n\n')
    out.write(f"const u16 EARTH_ROW[{H}]={{")
    out.write(",".join(map(str, rows)))
    out.write("};\n")
    out.write(f"const u8 EARTH_RLE[{len(rle)}]={{\n")
    for i in range(0, len(rle), 32):
        out.write(",".join(map(str, rle[i:i + 32])) + ",\n")
    out.write("};\n")
    out.write(f"const u32 EARTH_RLE_LEN={len(rle)};\n")
    out.close()
    print(f"ok: {land} celulas de terra, {small} ilhas pequenas, RLE {len(rle)} bytes")

    if len(sys.argv) > 3:
        from PIL import Image
        img = Image.new("RGB", (W, H))
        px = img.load()
        for y in range(H):
            for x in range(W):
                px[x, y] = (90, 140, 70) if grid[y][x] else (30, 50, 110)
        img.resize((W * 2, H * 2), Image.NEAREST).save(sys.argv[3])


if __name__ == "__main__":
    main()
