# tools/armbench/run.py <dir>: roda o game.bin no unicorn (ARM) e imprime o
# relatorio do prof.c com o relogio = instrucoes executadas / 2 (ticks de
# 33,5 MHz se 1 instrucao = 1 ciclo a 67 MHz). E' um PISO: o ARM946 real
# gasta ~1,3-1,6 ciclos/instrucao (cargas, multiplicacao, desvios) + espera
# de RAM. Serve p/ comparar ANTES x DEPOIS e achar o gargalo, sem hardware.
import sys
from unicorn import *
from unicorn.arm_const import *
d=sys.argv[1]
sym={}
for l in open(d+"/game.sym"):
    p=l.split()
    if len(p)==3: sym[p[2]]=int(p[0],16)
BASE=0x02000000;SIZE=32<<20;HALT=0x0FFFFF00
mu=Uc(UC_ARCH_ARM,UC_MODE_THUMB)
mu.mem_map(BASE,SIZE);mu.mem_map(HALT&~0xFFF,0x1000)
mu.mem_write(BASE,open(d+"/game.bin","rb").read())
mu.reg_write(UC_ARM_REG_SP,BASE+SIZE-64);mu.reg_write(UC_ARM_REG_LR,HALT|1)
now=sym["Prof_Now"]&~1;tick=sym["bench_ticks"]
# Px_Ticks (orcamento do quadro, render-ahead): ciclos estimados = instrucoes x 1,4
cnow=sym["Px_Ticks"]&~1;ctick=sym["bench_cticks"]
n=[0]
import os,bisect
HIST=os.environ.get("HIST")
F={};DV={}
# entradas das rotinas de divisao da libgcc: quem chama (LR) entra no ranking
DIVS={sym[k]&~1 for k in sym if k in ("__udivsi3","__aeabi_uidiv","__divsi3","__aeabi_idiv","__aeabi_uidivmod","__aeabi_idivmod","__udivmoddi4","__aeabi_uldivmod","__aeabi_ldivmod")}   # veneers Thumb pulam com b (LR intacto): nao contar 2x
# geracao do mundo + aquecimento rodam sem contar (rapido): instrumenta so'
# a partir do 1o Prof_Reset (comeco dos cenarios medidos)
mu.emu_start(sym["main"]|1,sym["Prof_Reset"]&~1)
def hb(uc,addr,size,ud):
    c=size//2 if uc.reg_read(UC_ARM_REG_CPSR)&0x20 else size//4
    n[0]+=c
    if HIST:
        F[addr]=F.get(addr,0)+c
        if addr in DIVS:
            lr=uc.reg_read(UC_ARM_REG_LR)&~1;DV[lr]=DV.get(lr,0)+1
    if addr==now: uc.mem_write(tick,((n[0]//2)&0xFFFFFFFF).to_bytes(4,"little"))
    elif addr==cnow: uc.mem_write(ctick,((n[0]*14//1280)&0xFFFFFFFF).to_bytes(4,"little"))
mu.hook_add(UC_HOOK_BLOCK,hb)
mu.ctl_remove_cache(BASE,BASE+SIZE)   # blocos ja' traduzidos nao chamariam o hook
mu.emu_start((sym["Prof_Reset"]&~1)|1,HALT)
o=sym["OUTB"];k=int.from_bytes(mu.mem_read(sym["outN"],4),"little")
print(bytes(mu.mem_read(o,k)).decode("latin-1"),end="")
print("(instrucoes totais: %s; 'us' = piso no DS a 67 MHz com 1 ciclo/instrucao)"%format(n[0],","))
# HIST=1: tambem soma instrucoes por funcao (blocos -> simbolo) e lista as 40 maiores
if HIST:
    fs=sorted((a&~1,nm) for nm,a in sym.items() if not nm.startswith("$"))
    ad=[a for a,_ in fs];agg={}
    for a,c in F.items():
        i=bisect.bisect_right(ad,a)-1
        if i>=0: agg[fs[i][1]]=agg.get(fs[i][1],0)+c
    tot=sum(agg.values())
    for nm,c in sorted(agg.items(),key=lambda x:-x[1])[:40]: print(f"  {nm:32s} {c:>13,d} {100*c/tot:5.1f}%")
    agd={}
    for a,c in DV.items():
        i=bisect.bisect_right(ad,a)-1
        if i>=0: agd[fs[i][1]]=agd.get(fs[i][1],0)+c
    print("  -- quem chama divisao em software (chamadas):")
    for nm,c in sorted(agd.items(),key=lambda x:-x[1])[:25]: print(f"  {nm:32s} {c:>11,d}")
