#!/usr/bin/env python3
# font_check.py — falha se algum caractere usado nas strings do jogo nao tiver
# glifo na Serifa gerada (SDD sec 14.3). Le os codepoints de source/gen/
# font_data.c e varre os literais dos arquivos de texto.
#
# Rode manualmente antes de commitar (o build NAO roda Python, ADR-14):
#   PYTHONPATH=... python3 tools/font_check.py
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(ROOT, "source", "gen", "font_data.c")
SCAN = ["source/str.c", "source/city.c", "source/nations.c", "source/history.c",
        "source/traits.c", "source/calendar.c", "source/economy.c",
        "include/config.h"]

def cached_cps():
    txt = open(GEN, encoding="utf-8").read()
    return set(int(m) for m in re.findall(r"\{\s*(\d+)\s*,", txt))

def used_cps():
    used = {}
    for rel in SCAN:
        p = os.path.join(ROOT, rel)
        if not os.path.exists(p):
            continue
        s = open(p, encoding="utf-8", errors="replace").read()
        for m in re.findall(r'"((?:[^"\\]|\\.)*)"', s):
            for ch in m:
                if ord(ch) > 31:
                    used.setdefault(ch, []).append(rel)
    return used

def main():
    have = cached_cps()
    used = used_cps()
    miss = {ch: f for ch, f in used.items() if ord(ch) not in have}
    if miss:
        print("FALTAM %d glifo(s) na Serifa:" % len(miss))
        for ch, f in sorted(miss.items()):
            print("  U+%04X %r  em %s" % (ord(ch), ch, ", ".join(sorted(set(f)))))
        sys.exit(1)
    print("OK: %d caracteres usados, todos cobertos pela Serifa." % len(used))

if __name__ == "__main__":
    main()
