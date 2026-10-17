# OH MY VICTORIA DS! v0.7.0 (RTS Vic-like, pixel-art)

## COMPILAR E JOGAR NO DS
- Oficial: instala devkitPro (devkitARM + libnds). `make` -> `ohmyvictoria_ds.nds`.
- Aqui (rootless): `source ~/devkitpro/env.sh` uma vez por terminal, depois `make`.
3. Copia p/ flashcard (R4/Acekard) ou abre no melonDS. 4. `make clean` limpa.

## O JOGO EM 5 MIN
Menu pixel (L/R dificuldade, B seed, A comeca) -> fazenda (Y) -> B compra grao ->
X recruta -> A marcha (seta direcional, leva dias) -> evento surpresa -> clima
muda colheita -> precos mexem -> R paz / R+CIMA uniao / SELECT+L governo ->
prestigio 600 vence.

## BOTOES (tudo sem menu)
D-pad cursor+camera | A marcha | X recruta/forte | Y obra (23, gira) | L imposto |
R paz / R+CIMA uniao | SELECT+L governo MON>DEM>TEO (50 ouro, 15d cd) |
B compra int / B+BAIXO exporta (gira 8 bens) |
SELECT+B save | SELECT+R load | START pausa | SELECT vel | touch mapa 1:1.

## SISTEMAS
8 bens em cadeia (grao/roupa/movel/arma/luxo/ferram/carvao/vinho) + 4 POPs.
23 obras (rural/industria/urbana/estado/mar) com throughput (sem insumo = menos).
3 governos (MONARQUIA imposto+ataque / DEMOCRACIA feliz+letras / TEOCRACIA fe+defesa).
Mercado interno/faccao + externo unico + tarifa (0 com uniao) + uniao aduaneira (ALLY).
7 terrenos transitaveis + 7 sites + clima seca/chuva + 8 eventos RNG por seed.
2 bots, 4 dificuldades, marcha com delay+dado+moral, diplomacia paz/uniao.
Docs: `docs/SDD.md` + `docs/diagramas/*.mmd`.
