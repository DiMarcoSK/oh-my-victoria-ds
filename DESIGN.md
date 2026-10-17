# DESIGN.md — OH MY VICTORIA DS!

> **O que é este documento:** a fonte de verdade de *design* do jogo — como ele deve parecer,
> como deve se jogar, que mundo ele representa e quais padrões todo conteúdo novo segue.
> Vale para quem programa, desenha, escreve eventos ou cadastra países, humano ou agent.
>
> **Hierarquia dos documentos:**
> | Documento | Responde | Manda em |
> |---|---|---|
> | `DESIGN.md` (este) | *o quê* e *por quê*: visão, mapa, países, história, padrões | design de jogo, mapa, conteúdo, direção de arte |
> | `concept_arts/image.png` | *como as telas parecem* | aparência dos menus e do HUD |
> | `docs/SDD.md` | *como está/será no código*: arquivos, structs, pixels, fases | implementação (sec 14 = telas em pixel) |
> | `AGENTS.md` | *como trabalhar no repo* | regras de build, teste e entrega |
>
> Em conflito de design, este documento vence e o SDD é corrigido. Em conflito técnico
> (RAM, CPU, formato de pixel), o SDD e as restrições do DS vencem e este documento é
> corrigido. Nunca deixe os dois se contradizendo.
>
> Status: **visão aprovada; implementação começou (branch `feat/mundo`).** O jogo jogável hoje
> (v0.7/v0.8) é um mapa procedural 32x24 com 3 facções. Já existem, como módulos de núcleo
> testados no PC: calendário 1800–2025, catálogo de países com traits, bandeiras procedurais,
> motor de eventos (históricos + aleatórios) e o mundo celular com frentes (M1). O caminho
> e o que falta estão na sec 9.

---

## 1. A visão em uma frase

**Victoria 3 no bolso: o mundo inteiro de 1800 a 2025, com centenas de países históricos e
povos indígenas que têm identidade própria, pintado num mapa político vivo onde as fronteiras
avançam como em frontwars.io — e a história real acontecendo ao seu redor até você mudá-la.
Chegou a 2025? Continue jogando o futuro, se quiser.**

## 2. Pilares (todo recurso novo tem que servir a pelo menos um, e não ferir nenhum)

1. **Profundidade de Victoria.** Sociedade (POPs, felicidade, literacia, militância), economia em cadeia, mercados, leis/governos, diplomacia. Decisões têm consequências que levam anos para aparecer.
2. **Legibilidade e ritmo de frontwars.io.** Território é cor pintada célula a célula; guerra é uma *frente* que avança e recua visivelmente; você manda "50% do exército para lá" e vê acontecer. Em 2 segundos olhando o mapa, o jogador sabe quem está ganhando.
3. **História viva.** O modo Mapa-múndi começa em **1º de janeiro de 1800** e vai até **31 de dezembro de 2025**. A história real acontece — Haiti independente em 1804, fim do Sacro Império em 1806, independências latino-americanas, unificações, partilha da África, guerras mundiais, descolonização, corrida espacial, fim da URSS — **a menos que o mundo do jogo já tenha divergido**. Cada evento histórico tem condição; se ela não é verdade, a história segue outro rumo. Em 2025 a partida termina com um balanço, e o jogador pode **continuar jogando** (Modo Eterno, sec 7.6).
4. **Todos os povos importam.** O mundo de 1800 não tem "terra vazia". Impérios, reinos, sultanatos, confederações e povos indígenas são entidades com nome, território, traits, objetivos e voz. Ninguém é cenário.
5. **Feito para o DS.** Duas telas de 256x192, D-pad + botões + caneta, 4MB de RAM, ARM9 de 67MHz. Se uma ideia não cabe legível em 256x192 ou não roda em 30ms por dia de jogo, ela é redesenhada — não espremida.

**Teste do pilar:** antes de implementar algo, escreva em uma linha qual pilar ele serve. Se não conseguir, não implemente.

### 2.1 Por que é divertido (os laços de jogo)

Um grande estratégia de 225 anos só funciona se, em qualquer escala de tempo, houver algo para querer e algo para ver acontecer:

| Laço | Duração | O que o jogador faz | O que ele sente |
|---|---|---|---|
| **Instante** | ~10 s | manda 50% do exército, vê a frente pintar o mapa | controle, impacto visível (frontwars) |
| **Decisão** | ~2 min | responde a um evento, constrói, ajusta imposto, negocia | "minha escolha mudou algo" |
| **Ambição** | 10–20 min | persegue a meta de 5–10 anos que escolheu | progresso claro, recompensa |
| **Era** | 1–2 h | atravessa uma era, corre pelos "Primeiros" do mundo | o mundo muda ao redor; a história diverge |
| **Partida** | dezenas de horas | chega a 2025 (ou ao Modo Eterno) com a *sua* história | orgulho, vontade de jogar de novo com outro povo |

Recursos que alimentam esses laços (todos fazem parte do design, não são opcionais):

1. **Ambições.** A cada ~5 anos de jogo o jogador escolhe 1 entre 3 metas sorteadas que façam sentido para o país e o momento ("Controlar 3 portos até 1820", "Unir os povos das Planícies", "Primeira ferrovia da Ásia", "Sobreviver à Partilha da África independente"). Cumpriu: prestígio e um bônus temporário. Sempre há um objetivo claro na tela.
2. **Os Primeiros (corridas mundiais).** Marcos globais com troféu para quem chegar primeiro: primeira ferrovia, telégrafo, abolição total, sufrágio universal, avião, rádio, antibiótico, satélite, pessoa na Lua, internet, energia limpa. O mundo inteiro é avisado: "O Sião inaugurou a primeira ferrovia da Ásia!". Qualquer país pode vencer qualquer corrida.
3. **Grandes Personagens.** Figuras históricas surgem nas datas e lugares reais como general, estadista, cientista ou artista, com um bônus forte e uma frase própria (Toussaint Louverture, Bolívar, Tecumseh, Shaka, Moshoeshoe, Garibaldi, Bismarck, Tȟatȟáŋka Íyotake, Menelik II, Taytu Betul, Rani Lakshmibai, Marie Curie, Santos-Dumont, Gandhi, Mandela...). Se o país dela não existe mais, ela pode surgir em outro lugar — ou não surgir. **Responsáveis por genocídio ou crimes contra a humanidade nunca são bônus jogáveis**: aparecem só em eventos históricos, com tom sóbrio.
4. **Momentos decisivos.** Crises com contagem regressiva visível ("Ultimato: 30 dias", "A frota inimiga zarpou") e escolhas grandes. O jogo pausa sozinho para a decisão (opção nas Configurações).
5. **A Gazeta.** A cada década, um jornal ilustrado de uma página na tela de baixo conta o que aconteceu: seus feitos, o maior império, as guerras, as corridas, curiosidades do mundo ("Mapuche e Chile assinam tratado de fronteira"). É o replay narrativo da partida e o que o jogador vai querer mostrar para os outros.
6. **Rivais com personalidade.** Cada IA tem um arquétipo derivado dos traits (Expansionista, Mercador, Isolacionista, Revolucionário, Diplomata, Oportunista), manda despachos com tom próprio e toma decisões coerentes. O jogador sempre tem um **Rival** e um **Amigo** destacados no HUD.
7. **Nunca um fim amargo.** Perdeu o país? Vire **governo no exílio** (revoltas para voltar) ou **jogue como outro país** a qualquer momento, como no Victoria.
8. **Tempo que respeita o jogador.** Velocidades 1 a 5 e **"Avançar até o próximo evento"**: 225 anos passam rápido quando nada pede a sua atenção.
9. **Conquistas com nome histórico.** "Adwa" (vença uma grande potência como Etiópia), "Zollverein" (união aduaneira com 5 países), "Waitangi honrado" (chegue a 2025 com os iwi māori soberanos), "Sol que nunca se põe" (território em todos os continentes).
10. **Blitz.** Mapa aleatório pequeno, 8 países, ~15 minutos, só frentes e ouro: a experiência frontwars pura, para uma partida rápida no ônibus.
11. **Modo Eterno.** Depois de 2025, o mundo continua: eventos aleatórios e as corridas do futuro (sec 7.6).
12. **Pequeno país, grande potência (Victoria 3).** O mapa não é tudo: um país pequeno pode virar potência **pela economia**. Especialize cidades (até 3 obras de produção iguais por cidade), monte a cadeia longa (minério → aço → máquinas/armamentos/eletrônicos), exporte (o lucro das casas exportadoras é tributado) e deixe o **investimento privado** erguer fábricas sozinho. Exemplo de estratégia: o **mercador de armas** — arsenais e fábricas de armamentos, **contratos de armas** com quem está em guerra (pagam no preço mundial, que dispara nas guerras) e, se faltar freguês, **instigar uma guerra** entre vizinhos (sec 4.5 e 4.8).

---

## 3. Identidade visual (reforço da concept art)

A concept art (`concept_arts/image.png`) define a aparência de todas as telas de menu e do HUD. O SDD sec 14 traduz isso em pixels. Esta seção explica o **espírito**, para que qualquer tela nova pareça do mesmo jogo.

### 3.1 O tom: "gabinete de um ministro vitoriano"

O jogador é um chefe de Estado em 1800 olhando mapas, despachos e brasões numa mesa de madeira escura à luz de vela. Tudo na interface vem desse mundo:

| Elemento real | Vira na UI |
|---|---|
| Papel de despacho, mapa antigo | **pergaminho** (`C_PARCH`) para todo conteúdo legível |
| Couro e madeira escura da mesa | **navy profundo** (`C_NAVY_BG`, `C_NAVY`) para moldura, HUD e fundo |
| Selo de cera, fita de condecoração | **crimson** (`C_CRIMSON`) — só para o que está *selecionado* |
| Douração, filete de moldura | **ouro** (`C_GOLD`) para bordas, ornamentos e ícones |
| Tinta ferrogálica | **tinta** (`C_INK`) para texto sobre pergaminho |
| Coroa, brasão, estandarte | ornamentos — com parcimônia |
| Gravura em metal | ilustrações secundárias em traço sépia sobre pergaminho |

Os valores exatos dos tokens estão no SDD sec 14.1. Não invente cor fora deles na UI.

### 3.2 Regras de composição

1. **Uma tela controla, a outra contextualiza.** Lista/controle numa tela; descrição, ilustração ou mapa da escolha na outra. Nunca duas listas ao mesmo tempo.
2. **Pergaminho para conteúdo, navy para estrutura.** Texto longo sempre sobre pergaminho.
3. **Crimson é sagrado:** só a seleção atual. Erro não é crimson (é texto em tinta + som de erro); inimigo no mapa não usa o crimson da UI.
4. **Um ornamento por painel.** No máximo uma coroa e um divisor `—◆—`. Excesso de ornamento vira kitsch.
5. **Respiro:** margem interna mínima de 8px em pergaminho, 4px em navy. Texto nunca encosta na borda.
6. **Hierarquia por fonte, não por cor:** Serifa 12 para títulos e itens, Serifa 8 para corpo. Cor só muda para selecionado/desabilitado.
7. **Silêncio visual:** menu parado não anima, exceto o pulso da seleção. Movimento chama atenção; use só quando algo pede atenção (evento, ataque, fim de guerra).

### 3.3 Ilustração (retratos, cenas, brasões)

- **Resolução nativa 1:1** para 256x192. As pinturas dos menus (pôr do sol de Westminster com o logo, oficial, cidade gótica, porto com estandarte, brasão, gravura do porto, papel e cantos) **são a própria concept**, recortada e reamostrada com filtro suave (Lanczos) para o pixel do DS — menos pixelada, igual à concept (SDD ADR-29). Nunca amplie por vizinho mais próximo e nunca recorte texto: o texto vem sempre do jogo (`str.c`).
- Luz de cima-esquerda, outline escuro de 1px, sem anti-aliasing contra transparente, paleta quantizada para 15 bits.
- **Pintura histórica do séc. XIX como referência** (Turner, Delacroix, gravuras de jornal ilustrado da época, retratos oficiais), traduzida para pixel. Cores quentes de pôr do sol, céus dramáticos, arquitetura reconhecível.
- **Retratos e figuras de povos não europeus** são desenhados a partir de referência histórica documentada (roupas, armas, arquitetura, paisagem do período), nunca de caricatura ou fantasia genérica de "tribo". Ver sec 6.5.
- Cenas de seleção de país: uma por **região cultural** (sec 4.6) e cenas únicas para os países de destaque. Cada cena mostra **gente + lugar + época** (ex.: a concept do Reino Unido: soldado, Union Jack, Tâmisa, Westminster). Hoje a cena do porto vale para todos: o **pano do estandarte é repintado com a bandeira da nação escolhida**, mantendo as dobras e sombras da pintura (máscara de sombra gerada com a arte).

### 3.4 Texto e voz

- PT-BR com acento (fonte Serifa, SDD sec 14.3). Inglês como segundo idioma.
- Tom de **despacho de chancelaria**: frases curtas, sóbrias, levemente solenes. "Conquiste territórios, desenvolva sua nação e escreva sua história." — não "Bora dominar o mundo!!!".
- Descrição de país na seleção: **até 2 linhas, ~90 caracteres**, dizendo o que o torna único *mecanicamente* e *historicamente* ao mesmo tempo.
- Evento: título de até 24 caracteres + corpo de até 4 linhas Serifa 8 (~160 caracteres) + 1 a 3 opções.
- **Nomes próprios com a grafia correta**, incluindo diacríticos de outras línguas (Māori, Očhéthi Šakówiŋ, Đại Việt, Asante). O conjunto de glifos da fonte é **gerado a partir de `str.c`**: um nome novo com letra nova obriga a desenhar o glifo, não a remover o acento.

### 3.5 Não faça

- ❌ Vidro, neon, flat design moderno, emoji. Degradê só sutil e pontilhado (botões, faixa de seleção, céu, papel), como na concept.
- ❌ Fonte sem serifa ou fonte 3x5 em tela do jogador (a 3x5 é só debug).
- ❌ Crimson em qualquer coisa que não seja seleção.
- ❌ Cor literal fora dos tokens (SDD sec 14.1) e `RGB15()` cru (vira pixel transparente — ADR-13).
- ❌ Recorte da concept com texto embutido, ou ampliado/reduzido sem filtro (bloco duplicado).
- ❌ Estereótipo visual de qualquer povo (sec 6.5).

### 3.6 Controles na partida (mira no centro, painel só caneta)

O DS tem duas telas com papéis diferentes, e os controles seguem isso:

| Entrada | Tela de cima (mapa) | Tela de baixo (painel) |
|---|---|---|
| **Setas** | movem o **mapa** com inércia (acelera segurando, desliza ao soltar, pixel a pixel em todo zoom); a **mira** (anel pequeno com ponto) fica no centro e o mapa corre por baixo dela | — |
| **A** | **interage com o que está sob a mira**: abre a *folha de ações* no painel | — |
| **Caneta** | — | **tudo** no painel: folha de ações, botões rápidos, despachos (Aceitar/Recusar), Gabinete, minimapa e catálogo da cidade, **bolinha do ranking** (canto superior esquerdo) |
| B | fecha a folha; recolhe a ofensiva | volta (cidade, Gabinete) |
| L / R | zoom (a mira continua no mesmo alvo) | troca a aba do Gabinete / a cidade |
| X / Y / START / SELECT | Gabinete / modo de mapa / pausa (menu) / pausa rápida | — |

**Folha de ações:** lista só o que dá para fazer com o alvo, com o preço no rótulo (nada de 2º toque escondido):
- **outro país:** em paz — propor aliança, romper, pedir entrada / convidar ao mercado comum, *declarar guerra e atacar*; em guerra — enviar tropas (% da reserva), propor paz;
- **seu território:** fundar cidade aqui (preço), recrutar tropas (preço);
- **sua cidade:** abrir a cidade, recrutar;
- **terra livre:** ocupar (% da reserva), recrutar;
- **outro país (em paz), intriga e armas:** **vender armas a eles** (contrato, só se estão em guerra), **encerrar o contrato**, **instigar uma guerra** (preço no rótulo; recarga de 180 dias).

**Botão Construir** (casinha no painel; antes era só "cidade"): abre a folha de obras da célula sob a mira — **abrir a cidade** (se for sua), **fundar cidade aqui**, **erguer forte**, **cavar trincheiras** (1860+) e **bateria costeira** (só na costa), cada uma com o preço. Só aparece o que dá para construir ali.

**Atacar com foco:** "Atacar com foco na mira" manda a % da reserva e faz o avanço **puxar para a célula sob a mira** (sec 4.5). Mire na capital, numa cidade ou num passo de montanha antes de mandar.

**Barra de estatísticas** (topo do mapa): caixa (com seta verde/vermelha do saldo fiscal da semana), população, exército, alfabetização, prestígio e data.

**Resposta da IA:** quem é atacado pelo jogador (e os aliados dele) fica em alerta e pensa todo dia — contra-ataca em 1–2 dias, não no próximo turno de 8 dias.

Mover o mapa com a folha aberta atualiza a folha para o novo alvo. No layout **Invertida** (mapa na tela de toque) o painel fica em cima, sem toque: aí a folha e os despachos aceitam CIMA/BAIXO + A/B.

---

## 4. O mapa: Victoria 3 + frontwars.io

O mapa é o jogo. Ele combina duas coisas:

| De **Victoria 3** vem | De **frontwars.io** vem |
|---|---|
| Mapa político com cor por país sobre terreno pintado | Território como **células pintadas**, sem hexágono nem quadrado visível |
| Nome do país escrito grande, espaçado, atravessando o território | Fronteira que **avança e recua célula a célula** durante a guerra |
| Zoom afastado = mapa de papel estilizado; zoom próximo = terreno detalhado com cidades | Número de tropas grande e legível sobre o território |
| Estados/regiões com economia, POPs e prédios | Ordem de ataque por **porcentagem do exército** (25/50/75/100%) |
| Modos de mapa (político, mercado, diplomático, cultura) | Exército como **reserva que se regenera** com o tamanho do país |
| Frentes de guerra entre países, generais, infâmia, diplomacia | Partida rápida e legível no mapa aleatório pequeno |

### 4.1 Estrutura do mundo (do menor para o maior)

| Camada | O que é | Quantidade (mapa-múndi) | Para que serve |
|---|---|---|---|
| **Célula** | menor unidade de território, 1 dono | grade 512x192 (≈0,7° por célula, de 80°N a 55°S) | pintura do território, avanço da frente (frontwars) |
| **Região** | conjunto fixo de células com nome histórico ("Île-de-France", "Paha Sápa", "Bengala") | ~600 | economia, POPs, prédios, recursos (o "estado" do Victoria 3) |
| **Fatia de região** | a parte de uma região que pertence a um dono | até 4 por região | uma região pode estar dividida entre países, como no Victoria 3; a produção é proporcional às células |
| **País / povo** | entidade política (sec 6) | ≥150 em 1800, ≥250 no catálogo total | diplomacia, governo, exército, traits |
| **Bloco de mercado** | países ligados por união aduaneira | dinâmico | preços convergem (sistema da v0.7, sec 3 do SDD) |

**A célula é visual e militar; a região é econômica.** Construir, produzir e taxar acontecem por fatia de região. Conquistar acontece por célula. Isso mantém a economia barata para o DS (600 regiões, não 98 mil células) e a guerra fluida.

### 4.2 Níveis de zoom (L = afastar, R = aproximar)

| Zoom | Escala | Aparência | Mostra |
|---|---|---|---|
| **Z0 — Mundo** | mundo inteiro (512x192 -> 256x96, 1 pixel = 2x2 células) | **mapa de papel**: oceano em pergaminho azulado com hachuras de gravura, países em cor chapada | nomes só das grandes potências, frentes ativas como linha piscante |
| **Z1 — Continente** | 1 pixel = 1 célula | papel + cor de país + **fronteiras grossas** | nomes de todos os países que couberem, tropas em número |
| **Z2 — Países** (no código) | 1 célula = 2px | papel + cor de país, cantos suavizados, relevo só nas montanhas | nomes de países médios, tropas, capitais |
| **Z2 — Região** | 1 célula = 4px | **terreno pintado** (floresta, montanha, deserto, estepe, selva, tundra) com a cor do país por cima a ~30% | nomes de regiões, cidades, capitais, exércitos como figurinhas |
| **Z3 — Detalhe** | 1 célula = 8px | terreno detalhado no estilo da concept: castelos nas capitais com a bandeira tremulando, casinhas nas cidades, navios nos portos, pinheiros densos | prédios da região, frente célula a célula, **soldados na frente** |
| **Batalha** (Z5 no código) | 1 célula = 16px | o zoom mais perto: cidades com ruas, hortas e a fábrica soltando fumaça; fortes em estrela com o raio defendido tracejado | **soldados dos dois lados de cada aresta da frente** (cor da nação; cáqui depois de 1900), tiros piscando, fumaça de pólvora e explosões de artilharia |

Afastado parece um mapa de chancelaria; aproximado parece o painel "JOGO EM ANDAMENTO" da concept. **É a mesma transição papel -> terreno do Victoria 3.** O minimapa (SDD sec 14.4) sempre mostra Z0.

### 4.3 Como desenhar o mapa

- **Cor do país:** cada país tem uma *cor de mapa* histórica e convencional (Reino Unido vermelho, França azul, Rússia verde-escuro, Qing amarelo, Otomano verde-oliva, Espanha amarelo-ouro, Países Baixos laranja...). Regra dura: **países vizinhos precisam contrastar** (diferença mínima de 6 em algum canal de 5 bits). A cor não pode ser igual a nenhum token de UI.
- **Fronteiras (3 pesos):** país = 2px na cor do país escurecida + 1px de brilho interno (o "glow" do Victoria 3); região = 1px pontilhado em 40% de tinta; célula = invisível.
- **Nomes de país:** Serifa 12 em caixa alta, **letras espaçadas para cobrir a largura do território** (como no Victoria 3), cor = cor do país escurecida, contorno de 1px em pergaminho. Só aparece se couber; senão aparece no próximo zoom. Nunca dois nomes sobrepostos (o maior território vence).
- **Capital:** estrela dourada (Z0-Z1), castelo/palácio com bandeira (Z2-Z3).
- **Mar:** águas profundas mais escuras, plataforma continental mais clara, espuma de 1px na costa. Nomes de mares em Serifa 8 espaçada, cor de tinta azul.
- **Povos não estatais / autônomos** (sec 6.3): cor própria com **hachura diagonal leve** — território reconhecido, não vazio.
- **Guerra:** células contestadas piscam entre as duas cores (dither 50%, 2 frames); a frente ganha seta na direção do avanço; célula recém-conquistada fica 1 tom mais clara por alguns dias (rastro do avanço, efeito frontwars).
- **Cache:** como hoje no `px_map.c`, o mapa é "cozido" num buffer e só células alteradas são redesenhadas. Animação só em frente ativa, bandeira de capital e água.
- **Suave e realista (no código, SDD ADR-24):** nada de célula quadrada na tela. Cada pixel mistura as células vizinhas com um leve "serpentear" da costa e da fronteira; relevo sombreado (luz de noroeste), mar do turquesa raso ao azul profundo com espuma na costa, praia, copas de árvore e areia em textura. Gradiente aqui é permitido, sempre pontilhado (Bayer) — a regra "sem gradiente" (sec 3.5) é da UI.

### 4.4 Modos de mapa (botão Y alterna; ícone no painel de baixo também)

| Modo | Cor por | Pergunta que responde |
|---|---|---|
| **Político** (padrão) | país | quem é dono do quê? |
| **Terreno** | bioma, sem cor de país | onde a defesa é forte, onde a marcha é lenta? |
| **Diplomático** | relação com você: aliado, união, paz, rival, guerra | com quem posso contar? |
| **Mercado** | bloco de mercado / união aduaneira | onde meus preços são formados? |
| **Povos e culturas** | grupo cultural das POPs da região | onde há povos dominados que podem se revoltar ou se libertar? |
| **Riqueza** | produção da região (frio -> quente) | onde investir, o que conquistar? |

### 4.5 Guerra no estilo frontwars (com camada Victoria)

1. **Declarar guerra** é diplomacia: custa prestígio, gera **infâmia** (Victoria) proporcional ao tamanho do alvo, e chama os aliados dos dois lados.
2. **Exército = reserva** por país (não mais tropas por tile). Regenera por dia conforme população, moral, quartéis e traits; tem teto conforme população e regiões. Mostrada como número grande sobre o país (frontwars).
3. **Atacar:** botão Marchar -> toca/seleciona o país alvo no mapa -> escolhe **25/50/75/100%** da reserva (ESQ/DIR ou slider tocável). Abre-se uma **frente** em toda a fronteira comum. Sem fronteira terrestre, as tropas **desembarcam** na praia do alvo mais próxima da sua capital (até ~96 células de mar, custa 1/10 da reserva) — é assim que ilhas jogam (no código: `World_Landing`, ADR-23 do SDD).
4. **A frente avança célula a célula, de forma gradual (ADR-38).** Cada célula da fronteira tem um custo (terreno, densidade do defensor, cidade, forte, distância da capital, traits). Por dia, só **1/5 das tropas da frente (+12) entra em combate**: esse orçamento vira a chance de tomar cada célula (no máximo ~60% da linha anda num dia). O avanço sai espalhado e contínuo, não um salto de toda a fronteira. A frente **sangra** 1% ao dia (suprimento) e mais 1% se o defensor tem exército: empate parado custa caro. Frente sem tropas, sem fronteira ou travada (nem a célula mais barata cabe) fecha e as tropas voltam.
4b. **Foco:** a ordem de ataque leva um foco (a célula sob a mira). Células a até 4 do foco pesam 2,5x, até 12 pesam 1,25x, longe pesam 0,15x: o avanço **puxa para lá** sem abandonar o resto da linha. A IA foca a capital do alvo.
5. **Terreno importa como no Victoria:** montanha, floresta, pântano, deserto, rio cortam o avanço; fortes seguram regiões.
5b. **Fortificações (botão Construir, ADR-38):** **Forte** (raio 3, +80% no custo de tomar as células do dono no raio; a própria célula do forte vale o dobro + 100%), **Trincheira** (1860+, raio 2, +120%, barata e rápida), **Bateria costeira** (só na costa, raio 4, +40%; desembarque no raio custa o triplo). Preço sobe com quantas o país já tem; 3 células de distância mínima entre obras. Quem toma a célula do forte **fica com ele** (canteiro inacabado se perde). A IA fortifica a fronteira com quem está em guerra com ela ou com o rival.
6. **Paz:** tratado escolhendo exigências (regiões, indenização, independência de um povo, união aduaneira). Ocupação militar sem tratado não é posse definitiva.
7. **Desempenho:** só células de fronteira ativa entram no cálculo, com orçamento fixo por frame (o resto espera o próximo frame).

### 4.6 Os dois tipos de mapa

O jogador escolhe na tela "Preparar partida" (SDD sec 14.5): **Mapa: `◂ Mapa-múndi 1800 ▸` / `◂ Aleatório ▸`**.

| | **Mapa-múndi 1800** | **Aleatório** |
|---|---|---|
| Geografia | Terra real, 512x192 células, ~600 regiões nomeadas | continentes gerados por seed (ruído inteiro, determinístico) |
| Tamanho | fixo | Pequeno 256x96 (partida rápida estilo frontwars, ~30 min) / Médio 384x144 / Grande 512x192 |
| Países | os ≥150 de 1800, nas posições reais | 8 / 16 / 32 países sorteados do catálogo histórico, **com seus traits** ("e se o Japão e os Asante fossem vizinhos?") |
| Terra sem dono | **não existe** — povos autônomos ocupam o que não é Estado (sec 6.3) | existe (neutra), como no frontwars |
| História | trilho histórico ligado (sec 7) | sem trilho; eventos aleatórios (sec 7.5), Os Primeiros e flavor de cada país |
| Data | começa em 1/1/1800, termina em **31/12/2025**; depois, **Modo Eterno** opcional | calendário a partir de 1/1/1800, sem âncoras históricas; Blitz sem calendário |
| Vitória | balanço de **Grande Potência** em 2025 (ou antes por objetivos), objetivos históricos do seu país, conquistas | % do território ou maior pontuação no limite de tempo |
| Modos | **Campanha** (objetivos fixos do país) e **Contra IA** (sandbox) | **Contra IA**, **Blitz**, **Multijogador** |

Na tela "Preparar partida" do mapa-múndi há também **História: `◂ Ligada ▸` / `◂ Só flavor ▸` / `◂ Desligada ▸`** (sec 7.4).

---

### 4.7 Cidades (Victoria 3 + Imperator Rome)

O território é célula; a **vida** está nas cidades. Cada país começa com as suas — no mapa-múndi, as cidades reais de 1800 (Londres, Paris, Edo, Kumasi, Cusco, Kororāreka...), mais vilas espalhadas pelo interior.

| Nível | População | Slots de obra | Para subir |
|---|---|---|---|
| **Povoado** | < 10 mil | 3 | crescer |
| **Vilarejo** | 10 mil | 6 | 50 mil + **Mercado** |
| **Cidade** | 50 mil | 10 | 400 mil + **Universidade** ou **Fábrica** |
| **Metrópole** | 400 mil | 16 | — |

**Obras** (uma em construção por vez; 3/6/10/16 lotes de obra por nível). **Obras de produção podem repetir até 3 vezes na mesma cidade** (fazenda, oficina, fábrica, quartel, serraria, mina, arsenal, poço e as da cadeia longa) — é assim que se especializa uma cidade (ADR-37). As demais são uma por cidade. **Cadeia longa (ADR-37):** **Siderúrgica** (1850+, minério → aço), **Manufatura** (madeira + têxteis → bens de consumo), **Fábrica de máquinas** (1870+, aço → máquinas), **Fábrica de eletrônicos** (1950+, aço + petróleo → eletrônicos), **Fábrica de armamentos** (1850+, exige arsenal; aço + ferramentas → muitas armas; tanque no pátio depois de 1914) e **Bolsa de valores** (exige banco; ouro e o dobro do investimento privado). As obras de antes: Fazenda (comida), Mercado (ouro, exigido p/ cidade), Oficina -> Fábrica (ouro; fábrica a partir de 1830), Quartel (tropas, defesa), Porto (só no litoral), Templo (prestígio), Muralha (defesa), Universidade (prestígio; abre a metrópole), Estação (a partir de 1830), Palácio (só na capital), Serraria, Mina, Arsenal, Estaleiro, Poço e as cívicas: **Escola** (alfabetização), **Hospital** (cresce, 1850+), **Banco** (ouro e juros menores), **Teatro** (prestígio e felicidade), **Parque** (felicidade, 1830+) e **Usina** (+25% na indústria da cidade, 1890+). Tudo vira número no motor: ouro por dia, crescimento, teto de tropas, prestígio por ano, defesa da cidade na guerra.

**A cidade é um mapa (ADR-35):** abrir a cidade mostra a planta dela vista do alto, 16x10 lotes: mar do lado certo (pela costa real) ou um rio, avenidas em cruz, ruas, praça central com chafariz, campos e mata nos arredores. **Você escolhe o lote de cada obra** (setas + A, ou o minimapa e "Construir aqui" na caneta); porto e estaleiro só na beira d'água. **O lugar importa:** oficina, fábrica, arsenal ou usina colada numa estação ou porto produz +25%. **Bairros nascem sozinhos:** a praça e, a cada ~60 mil habitantes, uma nova semente de bairro puxam casas para perto delas; a densidade sobe de chalés a sobrados e prédios (torres depois de 1950 nas metrópoles). Gente anda nas avenidas, a carroça vira carro em 1910, barcos balançam no porto e as chaminés soltam fumaça.

**No mapa, a cidade cresce como cidade (ADR-39):** de Z3 em diante cada casa nova nasce **colada numa casa que já existe**, de preferência ao longo das duas ruas principais, nunca no mar. Quanto mais gente, mais casas e mais longe a mancha vai — o contorno sai irregular, com o miolo mais alto (sobrados e prédios) e chalés na borda. Em Z4+ aparecem ruas de terra, hortas listradas na borda, a obra-símbolo no centro (palácio/templo) e a indústria na periferia; em Z5 a chaminé solta fumaça. O desenho é fixo pela posição (o bairro novo aparece colado no velho quando ela cresce).

**Fundar cidade:** no seu território, a 5 células de qualquer outra cidade, por **200 + 120 de ouro por cidade que você já tem** — o preço aparece antes de confirmar. Começa como povoado com uma fazenda.

**Ver a cidade viver:** cada obra aparece no lote dela quando fica pronta (com andaime enquanto constrói) e projeta sombra; a arquitetura segue a cultura de quem governa (europeia, leste-asiática, islâmica, africana, indígena) e a era (madeira e estuque -> tijolo -> concreto e vidro); árvores do clima (copa redonda, choupo, conífera, palmeira). Na conquista, a cidade muda de dono, perde gente e a obra em curso — e a tela da cidade fecha.

**IA:** constrói seguindo o caminho de nível e funda quando sobra ouro e terra — o mundo inteiro se urbaniza ao longo de 225 anos.

### 4.8 Economia, comércio marítimo e o Gabinete

O dinheiro vem do que o país **produz e vende**, não do tamanho do mapa.

- **13 bens:** grãos, madeira, fibra, minério, luxo (café, açúcar, chá, especiarias — só nos trópicos), têxteis, ferramentas, armas, petróleo (1860+), **aço** (1850+), **bens de consumo**, **máquinas** (1870+) e **eletrônicos** (1950+). O campo produz pelo terreno; as cidades, pelas obras: Fazenda, Serraria, Mina (só com mata/serra por perto), Oficina e Fábrica (têxteis e ferramentas, consomem fibra, minério e madeira), Arsenal e Fábrica de armamentos (armas), Poço (só sobre jazida), Siderúrgica, Manufatura, Fábrica de máquinas e de eletrônicos (ADR-37).
- **O que o povo pede:** comer, vestir, lenha; da vila em diante **bens de consumo** e luxo; a cidade industrial pede ferramentas, carvão (minério) e **aço** para as obras (1880+); no século XX, petróleo e, depois de 1950, **eletrônicos**. Bens de consumo e eletrônicos entram no nível de vida.
- **Pedidos do povo (ADR-37):** a cada ~8 semanas, um bem de consumo em falta vira **pedido** no despacho ("Bens de consumo faltam no mercado. Atenda em 12 semanas"). Atender (90% da demanda) alegra o povo e dá prestígio; ignorar gera protestos (felicidade cai, militância sobe). O pedido em aberto aparece no rodapé do Gabinete > Mercado.
- **Empregos (micro):** cada obra industrial ou de extração tem vagas (oficina 4 mil, fábrica 10 mil, siderúrgica 10 mil...). 40% da população da cidade trabalha nelas; cidade pequena com fábrica demais produz menos (Gabinete > Mercado mostra "Vagas %"). Fazer a cidade crescer é parte de industrializar.
- **Máquinas:** depois de 1870 cada obra industrial grande pede 1 carga de máquinas por semana; o país com máquinas abastecidas produz até +25%.
- **Exportar enriquece:** o lucro das **casas exportadoras** é tributado (linha *Exportação* nas Finanças): o país pequeno que vende muito ao mundo enriquece sem precisar de território. Gabinete > Comércio mostra quanto se exporta e importa.
- **Guerra move o mercado:** país em guerra triplica a demanda por armas — o preço mundial das armas sobe com as guerras.
- **Contratos de armas (ADR-37):** pela folha de um país em guerra, "vender armas a eles": toda semana a sua sobra de armas vai direto para eles (até o que o exército deles pede), paga no preço mundial com ágio; eles ganham tropas e gostam mais de você. A IA mercadora faz o mesmo (o mundo arma os dois lados). **Instigar uma guerra:** agentes e ouro empurram um país a atacar o rival dele (ou o vizinho mais fraco); chance pelo temperamento e pela força; falhar vaza e derruba a opinião.
- **Investimento privado (macro, ADR-37):** parte do PIB vira um **fundo privado** (imposto baixo atrai mais; bancos e bolsa multiplicam). Quando o fundo cobre uma obra, os investidores erguem a que o mercado mais pede, na cidade que mais combina. Nas suas cidades eles só entram se sobrarem 2 lotes (1 fica para você). Obras de Estado (quartel, muralha, palácio, templo, escola, universidade, hospital, parque, estaleiro) são sempre do governo.
- **Quatro mercados:** a cidade come primeiro do próprio entorno (**local**); o resto vai ao **mercado nacional**; países de um **mercado comum** somam tudo e dividem o preço; o que sobra ou falta cruza o mar para o **mercado mundial** — só até a capacidade dos **portos**.
- **Preço** sobe com a falta e cai com a sobra. Falta de comida faz a cidade encolher; falta de têxteis e luxo derruba o nível de vida e a felicidade; falta de armas trava o exército; falta de ferramentas atrasa as obras e encarece a construção.
- **Balanço fiscal (ADR-32):** todo país fecha a semana com **receita** (imposto sobre a produção, imposto per capita — mais gente letrada arrecada mais —, taxas das cidades, renda da terra e alfândega) menos **despesa** (exército, marinha, administração, educação, manutenção das obras, juros da dívida e corrupção sobre o cofre parado). **Déficit vira dívida** com juros pelo risco (dívida/limite, calotes passados; bancos baixam); acima do limite (meio ano de receita) vem o **calote**: a dívida cai a 1/3, mas o prestígio despenca, o povo se irrita, o exército deserta e os juros sobem para sempre. **Quatro políticas** (Gabinete > Finanças, ou a IA 1x por mês): imposto, tarifa (livre comércio abre o porto, protecionismo arrecada mais), educação (puxa a alfabetização) e gasto militar (acelera ou freia o recrutamento).
- **População e alfabetização reais:** cada país começa com a população e a alfabetização estimadas de 1800 (Maddison; Our World in Data). A população cresce pela transição demográfica (sobe com a era, cai com fome, guerra e, depois de 1950, com a alfabetização alta) e muda de dono junto com as células conquistadas. Alfabetização: escolas, universidades e o gasto em educação puxam até o teto da era; a difusão do mundo garante um piso. Letrados produzem até +25%.
- **Portos e rotas:** cada porto dá capacidade de comércio; as 3 maiores rotas de cada país aparecem no **mapa comercial** como linhas tracejadas **pelo mar, de porto a porto** (contornam continentes; ADR-33), com navios mercantes andando por elas; as frotas ficam ancoradas na água.
- **Marinha:** o Estaleiro (precisa de porto) lança navios de guerra. Frota protege as rotas, **bloqueia** os portos inimigos (o comércio deles cai a 1/3), garante desembarques e trava batalhas navais.
- **Gabinete (X), 7 abas em 2 linhas:** Mercado (tabela dos 13 bens, vagas, pedido do povo), Comércio e Frota (portos, rotas, lançar navio), Diplomacia (aliados, rival, mercado comum, guerras, quem gosta de você), Política (ideologia, felicidade, risco de revolução, ambições), **Finanças** (balanço linha a linha, dívida, juros, PIB, inflação e índice de preços), **Leis** (sec 4.10) e **Gráficos** (sec 4.11).
- **Preços nominais e inflação (ADR-40):** cada país tem um **índice de preços** (1,00 em 1800). Receitas e custos (obras, recrutar, fortes, navios, fundar) saem em ouro de hoje; a **dívida antiga não sobe com os preços** — a inflação a dissolve. A inflação vem da lei de moeda (padrão-ouro 0; papel-moeda ~2%/ano; banco central ~1,5%/ano), da guerra com papel-moeda e, na **Emissão livre**, do déficit pago com dinheiro novo (sem dívida, mas a inflação dispara). Inflação alta derruba a felicidade (até -15).

### 4.10 Leis (micro-gestão do Estado, ADR-40)

Nove grandes assuntos, **cinco leis cada** (45 leis), todas com bônus e penalidades visíveis antes de escolher. A lei "do meio" de cada assunto é a referência (sem efeito); as outras puxam para um lado e cobram do outro.

| Assunto | As 5 leis (da mais branda à mais extrema) | O que mexe |
|---|---|---|
| **Tributação** | Mínimos · Baixos · Moderados · Altos · Confiscatórios | receita (-50%..+60%) contra investimento privado e felicidade |
| **Comércio exterior** | Livre comércio · Tarifa baixa · Moderada · Protecionismo · Autarquia | capacidade dos portos e exportação contra alfândega e produção nacional protegida |
| **Educação** | Sem escola pública · Religiosas · Básico · Pública (1850) · Universal (1880) | ritmo da alfabetização e produtividade contra gasto |
| **Serviço militar** | Voluntário · Profissional · Seletivo · Conscrição · Mobilização total | teto, recrutamento e ataque contra custo, produção e felicidade |
| **Trabalho** | Servidão · Sem regulação · Jornada regulada (1840) · Sindicatos (1870) · Bem-estar (1910) | produção e investimento contra felicidade e militância |
| **Modelo econômico** | Mercantilismo · Laissez-faire · Mista (1870) · Dirigismo (1900) · Planificação (1917) | exportação, investimento **privado** (até +80%) ou **estatal** (até 20% da receita vira obras) |
| **Saúde** | Sem assistência · Caridade · Hospitais (1850) · Seguro social (1883) · Universal (1948) | crescimento da população e felicidade contra gasto social |
| **Moeda e crédito** | Padrão-ouro · Bimetalismo · Papel-moeda · Banco central (1913) · Emissão livre | inflação, limite da dívida e juros |
| **Imigração** | Fechadas · Restrita · Moderada · Portas abertas · Colonização | crescimento (só atrai gente se o país vive bem: porta aberta num país pobre é por onde o povo sai), colonização de terra livre |

- **Debate:** mudar uma lei abre um projeto no parlamento: **15 + 15 dias por degrau** e **3 de prestígio por degrau**; uma lei por vez; dá para retirar o projeto. A IA muda **1 degrau por mês** rumo ao que combina com ela (arquétipo, ideologia, era e caixa).
- **Aprovação ideológica:** cada ideologia prefere uma lei por assunto (o liberal quer livre comércio, laissez-faire e escola pública; o comunista, planificação, sindicatos e saúde universal; o conservador, protecionismo e mercantilismo...). Lei igual à preferida soma, a do outro extremo subtrai: **-6..+6 de felicidade**. Na tela, um quadradinho verde/amarelo/vermelho mostra o que o governo acha de cada lei.
- **Tela (Gabinete > Leis):** a grade 3x3 mostra a lei em vigor de cada assunto; toque num assunto, depois numa lei: a tela de cima lista bônus (verde) e penalidades (vermelho) dela e, abaixo, os da lei em vigor, com o custo do debate e o ano em que ela passa a existir.

### 4.11 Gráficos e marcos (ADR-41)

- **Gabinete > Gráficos:** 8 séries — **PIB, população, caixa, exército, dívida, alfabetização, nível de vida, prestígio** — da partida inteira (a série guarda até 96 pontos; quando enche, junta os pares e dobra o intervalo). Linha dourada com área pintada = você; linha cinza = o **líder do ranking** de cada época (se você lidera, o 2º). Mostra o valor de hoje e a variação em 10 anos. Dinheiro em **ouro de 1800** (descontada a inflação).
- **Marcos (o "só mais um mês"):** subir para a sua melhor posição no ranking (+2 de prestígio), **entrar para as 8 grandes potências** (+15), cair delas (aviso), **PIB real dobrou** desde o último marco (+10), **população dobrou** (+5). Cada marco aparece como faixa sobre o mapa.

### 4.9 Ranking das potências (Victoria 3)

A bolinha no canto superior esquerdo do painel abre o **ranking**: todos os países vivos em ordem, separados em **Grandes potências** (1–8), **Potências maiores** (9–20), **Potências menores** (21–50) e **Insignificantes**. Critérios: **Geral** (prestígio x3 + economia — PIB e população letrada — + exército + frota + alfabetização), Economia (PIB), Militar (exército + frota) e População. A ficha mostra a sua posição, a faixa e quanto falta para passar o país da frente. O prestígio de 1800 segue o concerto da época (Grã-Bretanha, França, Rússia, Habsburgo, Prússia, Qing, Otomanos, Espanha...).

## 5. Linha do tempo de jogo e eras

O mapa-múndi é dividido em eras. A era muda música, molduras de evento, tecnologias liberadas e a **variante das bandeiras**.

| Era | Anos | Marca do período | Muda no jogo |
|---|---|---|---|
| **Napoleônica** | 1800–1815 | Revolução exportada, coalizões, fim do Sacro Império | guerras frequentes; Levée en Masse; eventos de coalizão |
| **Restauração** | 1815–1848 | Congresso de Viena, independências latino-americanas | eventos de independência; ferrovia aparece (anos 1830) |
| **Primavera dos Povos** | 1848–1871 | revoluções de 1848, unificações italiana e alemã, Guerra Civil dos EUA | nacionalismo forte; telégrafo; aço |
| **Imperialismo** | 1871–1914 | partilha da África, Meiji, corrida naval | colonização acelerada; resistência anticolonial; eletricidade |
| **Guerras Mundiais** | 1914–1945 | duas guerras mundiais, revoluções, Grande Depressão | guerras totais; aviação; tanques; colapso de impérios; novos países |
| **Guerra Fria** | 1945–1991 | descolonização, dois blocos, corrida espacial, ONU | independências em massa; satélite e Lua; crises nucleares |
| **Globalização** | 1991–2025 | fim da URSS, internet, grandes blocos econômicos, pandemia | uniões aduaneiras grandes; internet; crises financeiras e climáticas |
| **Eterna** | 2026+ (opcional) | o futuro é seu | só eventos aleatórios e as corridas do futuro (sec 7.6) |

Tecnologias (ferrovia, telégrafo, aço, eletricidade, aviação, rádio, computação, internet...) entram como **prédios novos liberados por era + literacia**, reaproveitando o sistema de obras. Bens novos também chegam por era (petróleo a partir de 1860, borracha a partir de 1880, eletrônicos a partir de 1950); cada bem novo custa RAM por país e por região, então o SDD decide quantos cabem. Não crie árvore tecnológica separada (UI e RAM do DS).

**Armas nucleares** existem só como **dissuasão diplomática** (crises, tratados, corrida) — nunca como bombardeio jogável de cidades. Mantém o tom do jogo e evita transformar tragédia em botão.

**Ritmo:** 1800–2025 são ~82 mil dias. Na velocidade 5 (1 dia por frame), passam em ~23 minutos de tela sem pausa; na prática, a partida completa leva dezenas de horas com pausas e decisões. Save é obrigatório (SRAM, SDD F8).

---

## 6. Países e povos

### 6.1 Metas de quantidade

- **≥150 entidades no mapa em 1/1/1800**, de todos os continentes (grandes potências, Estados menores, súditos, povos indígenas, autônomos).
- **≥300 no catálogo total**, incluindo os que **surgem por evento** (México, Grécia, Bélgica, Itália, Alemanha, Libéria, Transvaal...) e **todos os Estados-membros da ONU de 2025**, para a história poder chegar ao mundo de hoje.
- Limite técnico: 254 donos simultâneos (dono da célula é `u8`; 0 = mar, 255 = reservado). Se passar disso, o SDD precisa decidir antes.
- **Nenhum continente com menos de 15% das entidades.** O mundo não é só a Europa.

### 6.2 Ficha de país (formato de dados)

Cada país é uma linha de dados em ROM, preenchida com este modelo. **Nenhum campo opcional fica em branco por preguiça**: ou tem valor, ou tem motivo.

```text
TAG          3 letras únicas (GBR, FRA, HDS, ASH...)
nome         nome exibido em PT; + EN
endônimo     nome na própria língua, se diferente (Asante, Haudenosaunee, Numunuu)
tipo         ESTADO | SÚDITO(de TAG) | POVO (jogável, descentralizado) | AUTÔNOMO (não jogável no início)
governo      MONARQUIA | REPÚBLICA (= a DEMOCRACIA da v0.7) | TEOCRACIA | CONSELHO (novo: poder descentralizado) | COMPANHIA (novo: EIC, VOC)
cultura      grupo cultural (para POPs, cena regional e nomes)
região-cena  qual cena regional usa na seleção (ou cena própria)
capital      região da capital em 1800
território   regiões/células em 1800 (fonte de dados do mapa)
cor de mapa  cor histórica, contrastando com vizinhos
bandeira     descritor procedural (sec 6.2.1) + variantes por era/evento
traits       2 a 4 (sec 6.4), dentro do orçamento
flavor       sec 6.5: unidade, eventos, decisão, objetivos, governantes
descrição    2 linhas, ~90 caracteres, na tela de seleção
fonte        referência histórica (livro/artigo/enciclopédia) que sustenta território, traits e flavor
```

#### 6.2.1 Bandeiras procedurais

Com 250 países, bandeira desenhada à mão não cabe no DS. Cada bandeira é um **descritor de ~6 bytes** desenhado pelo código em dois tamanhos (16x11 no mapa e listas; 40x58 no estandarte da seleção):

```text
padrão:   LISA | TRI_V | TRI_H | BI_H | BI_V | CRUZ_NÓRDICA | CRUZ | SALTIRE | UNION | CANTÃO | BORDA | DIAGONAL
cores:    até 3 (paleta de bandeiras, não os tokens de UI)
emblema:  NENHUM | ÁGUIA | ÁGUIA_DUPLA | COROA | CRESCENTE_ESTRELA | SOL | LEÃO | ESTRELA | CRUZ | DRAGÃO | ... (arte compartilhada 8x8 e 20x20)
```

- Use a **bandeira histórica do período** (França: tricolor até 1814, branca Bourbon 1815–1830, tricolor de novo). Mudanças de bandeira são efeitos de evento.
- **Muitos Estados e povos de 1800 não usavam bandeira nacional** (Qing até 1862, a maioria dos povos indígenas, muitos reinos africanos). Para eles o jogo usa um **estandarte de jogo**: padrão e cores tirados de símbolos documentados do povo (cores, padrões têxteis, emblemas de corte), **nunca de objetos sagrados**. A tela de créditos lista quais estandartes são do jogo e não históricos.

### 6.3 Tipos de entidade

| Tipo | Exemplos | Jogável | Como funciona |
|---|---|---|---|
| **Estado** | Reino Unido, Qing, Asante, Sião | sim | completo |
| **Súdito** | Nova Espanha (ESP), América do Norte Britânica (GBR), Hyderabad (EIC) | sim | paga tributo e segue o suserano; independência por evento ou guerra |
| **Povo** | Haudenosaunee, Numunuu (Comanche), Mapuche, Xhosa, Ngāpuhi | **sim** | governo CONSELHO, poder descentralizado: menos imposto e prédios, muito mais resistência em casa, mobilidade, diplomacia própria |
| **Autônomo** | povos de regiões sem contato com Estados em 1800 (interior da Amazônia, deserto australiano, Ártico) | não no início (liberável depois) | território nomeado, atrito alto para quem entra, eventos de contato; nunca "terra vazia" |

### 6.4 Traits

Traits são o **sabor mecânico** de um país: dizem, em jogo, o que ele foi na história. Todo trait mexe em alavancas que o motor já tem (produção, custo, imposto, felicidade, militância, literacia, prestígio, tarifa, ataque, defesa, marcha, regeneração do exército) — não crie sistema novo para um trait só.

**Orçamento:** cada trait tem peso (+2 forte, +1, −1 fraqueza). A soma por país fica **entre +1 e +5** (súditos podem ter +1 com um trait só). Tamanho também é poder: **gigantes (Qing, Rússia, Otomanos, Habsburgo) ficam perto de +1**, carregando fraquezas de império; **países e povos pequenos ficam perto de +3**, para serem divertidos de jogar. A única exceção é Glória Antiga (Mogol), que soma 0 de propósito.

**Teste do flavor:** um jogador que nunca leu história deve *sentir* o trait em 10 minutos ("os Comanche andam rápido demais na planície", "a Rússia engole exércitos no inverno").

| Trait | Peso | Efeito (proposta, balancear) | Fundamento histórico (exemplos) |
|---|---|---|---|
| **Berço Industrial** | +2 | +25% produção em indústria (aço, oficina, têxtil) | Revolução Industrial britânica |
| **Senhora dos Mares** | +2 | +1 ouro por porto; frente naval/costeira avança mais rápido | Royal Navy; navegação polinésia (Havaí, Tonga) |
| **Entreposto Comercial** | +1 | −5 p.p. de tarifa externa; +1 ouro por exportação | Países Baixos, Omã/Zanzibar, Aceh, Ryukyu |
| **Império Colonial** | +1 | colonização mais rápida; −estabilidade nas colônias | Espanha, Portugal, França, Grã-Bretanha |
| **Celeiro** | +1 | fazendas +2 grão | planícies russas e norte-americanas, Bengala |
| **Riqueza Mineral** | +1 | minas +1; ouro/prata em regiões específicas | prata da Nova Espanha, ouro Asante e de Minas Gerais |
| **Exército Profissional** | +2 | +1 ataque | Prússia, exército Khalsa sikh |
| **Levée en Masse** | +2 | recrutamento 25% mais barato; +regeneração de reserva | França revolucionária |
| **Disciplina Regimental** | +1 | +moral; a frente recua mais devagar | Gurkhas do Nepal; *amabutho* zulu após as reformas de Shaka |
| **Inverno Implacável** | +2 | invasores perdem tropas por atrito no inverno, em território próprio | Rússia, 1812 |
| **Cavaleiros das Planícies** | +2 | frente avança 1 dia mais rápido em planície/estepe/deserto | Comanche, Lakota, cazaques, cossacos, cavalaria de Oió |
| **Conhecimento da Terra** | +2 | +2 defesa em floresta/pântano/colina/deserto próprios; atrito para invasores | Seminole, Mapuche, Xhosa, Diné, Aceh, povos da floresta |
| **Montanhas-Fortaleza** | +1 | +2 defesa em colina/montanha | Suíça, Montenegro, Etiópia, Nepal, Afeganistão |
| **Guerra de Guerrilha** | +1 | regiões perdidas geram revolta a seu favor; ocupante tem −produção | Espanha 1808, Haiti, Ndé (Apache), Vietnã |
| **Confederação** | +1 | +relações com vizinhos do mesmo grupo; aliados internos não podem ser anexados; decisões custam mais tempo | Haudenosaunee, Confederação Maratha, Suíça, Confederação Germânica |
| **Povo em Movimento** | +1 | perder a capital não derruba o país; células voltam sozinhas se o ocupante sair | Numunuu, cazaques, tuaregues, Lakota |
| **Comunidade Coesa** | +1 | +felicidade, −militância | Merina, Buganda, Japão Tokugawa, muitos povos |
| **Fé Unificadora** | +1 | templos +1 felicidade e −militância; prestígio religioso | Estados Papais, Etiópia, Sokoto, Primeiro Estado Saudita |
| **Centro Cultural** | +1 | teatro/universidade dão +prestígio | Paris, Viena, Estados italianos |
| **Burocracia Antiga** | +1 | +25% imposto; literacia cresce devagar | Qing, Joseon |
| **Reformador** | +1 | eventos e decisões de modernização mais baratos | Egito de Muhammad Ali, Sião, Prússia pós-1807, Tsalagi (Cherokee), Japão Meiji |
| **Fervor Revolucionário** | +1 | +felicidade em REPÚBLICA; vizinhos monárquicos ganham militância | França, Haiti, EUA |
| **Nação Jovem** | +1 | +crescimento de POPs; prestígio inicial baixo | EUA, repúblicas latino-americanas recém-criadas |
| **Fronteira Aberta** | +1 | expansão para terras adjacentes mais barata — **e gera eventos de conflito com os povos que vivem lá** | EUA, Rússia (Sibéria/Cáucaso), Argentina, Colônia do Cabo |
| **Corsários** | +1 | ouro por saque de rotas marítimas; eventos de guerra com potências comerciais | Argel, Túnis, Trípoli (Guerra Berberesca de 1801) |
| **Isolamento** | −1 | −50% comércio externo; +estabilidade; removido por evento histórico | Japão (*sakoku*), Joseon |
| **Absolutismo** | −1 | +imposto; −felicidade; revoluções mais prováveis | Espanha, Nápoles, Rússia |
| **Império Multiétnico** | −1 | +população; −estabilidade; povos internos podem se libertar | Áustria, Otomanos, Rússia, Qing |
| **Gigante em Crise** | −1 | −eficiência; eventos de reforma ou colapso | Otomanos, Mogol, Oió, Etiópia da *Zemene Mesafint* |
| **Glória Antiga** | ±0 | +prestígio alto; poder real mínimo | Império Mogol em 1800 |

**Instituições injustas** (escravidão, servidão, trabalho forçado) **não são traits**. São **leis/instituições** com custo crescente (militância, revoltas, pressão diplomática) e eventos de abolição nas datas históricas (abolição do tráfico britânico 1807, Haiti 1804, Império Britânico 1833, emancipação russa 1861, EUA 1865, Brasil 1888). Nunca são apresentadas como bônus a escolher.

### 6.5 Flavor (o que faz cada país ser *aquele* país)

Mínimo por país. Os de destaque passam muito do mínimo.

| Elemento | Mínimo (menor) | Destaque |
|---|---|---|
| Traits | 2 | 3–4 |
| Nome da unidade militar | sim ("Casacas Vermelhas", "Impi", "Khalsa", "Agojie") | + nome da frota |
| Eventos próprios | 1 | 4+ (datados e condicionais, sec 7) |
| Decisão única | — | 1–2 (ex.: Prússia "Fundar o Zollverein", Japão "Restauração Meiji", Tsalagi "Adotar o silabário de Sequoyah") |
| Objetivos históricos (Campanha) | 1 | 3 |
| Governantes datados | — | lista com anos |
| Bandeira | 1 descritor | + variantes por era/evento |
| Cena de seleção | regional | própria |

### 6.5.1 Ideologias e bandeiras que mudam

Todo país tem uma **ideologia**: tradicionalista, liberal, conservador, comunista, autoritário ou teocrático. Ela mexe em ataque, defesa, renda, comércio, produção e felicidade — e **muda a bandeira**: a França comunista é a tricolor em vermelho com a estrela dourada; um golpe autoritário pinta de preto e põe a águia; a república liberal tira a coroa; a restauração a devolve. Vale para todos os países, sem desenhar bandeira nova.

**Bandeiras históricas (ADR-34):** além da regra, uma tabela (`hflags.c`) dá a bandeira **real** de cada período e regime: Bourbon branca (França tradicional), Segundo Império com a águia, Espanha republicana (roxo) e franquista, Portugal monarquista azul e branco e republicano verde e vermelho, Weimar, Reich e RFA/RDA, Rússia 1858 (negro-ouro-branco) e URSS com foice e martelo, República Popular da China com as 5 estrelas, Pérsia com leão e sol, Egito, Grécia 1978, Sião/Tailândia, Etiópia com o leão de Judá, Marrocos, Canadá (Red Ensign até 1965, depois a folha de bordo), Raj, Israel, Cuba, Filipinas, Tchecoslováquia, Brasil Império (coroa no losango) e República (globo), Vietnã, Afeganistão real, os Confederados... Padrões novos: losango, triângulo na tralha, 5 faixas, *ensign* britânico; emblemas novos: foice e martelo, folha de bordo, estrela de Davi, esfera armilar, 5 estrelas.

Revoluções nascem da miséria, da militância, de guerras longas, do imposto alto e do espírito da época (o velho regime envelhece depois de 1848; o partido único cansa no fim do século XX). O vizinho rival pode aproveitar o caos. Às vezes o descontentamento vira **separatismo**: uma nação do catálogo renasce dentro do império e luta pela independência.

### 6.6 Povos indígenas — princípios de representação

Este jogo representa colonialismo, conquista e resistência. Isso exige cuidado. Regras **obrigatórias**:

1. **Agência:** povos indígenas são entidades políticas com território, diplomacia, guerra, comércio e objetivos — e são **jogáveis**. Podem sobreviver, se modernizar, se aliar, vencer. O jogo permite que a história seja diferente (o Havaí, Madagascar Merina, Sião e Etiópia mostram que caminhos alternativos existiram).
2. **Nome próprio:** use o endônimo quando houver um amplamente usado (Haudenosaunee, Numunuu, Diné, Asante), com o nome comum em português entre parênteses na primeira menção. Nada de "tribos" ou "selvagens" genéricos.
3. **Diferente, não "atrasado":** forças são reais e históricas (terreno, mobilidade, coesão, diplomacia, fortificações *pā* māori contra artilharia). Fraquezas vêm do **contato colonial** (epidemias, armas desiguais, tratados quebrados), apresentadas como consequências históricas, não como algo inerente ao povo.
4. **Terra não é vazia:** expandir para território indígena é guerra ou tratado com aquele povo, com infâmia, resistência e eventos. Tratados podem ser quebrados — e isso tem custo, como teve na história.
5. **Tragédias históricas aparecem com seriedade** (Lei de Remoção Indígena de 1830 e a Trilha das Lágrimas de 1838, a Conquista do Deserto, a "Pacificação da Araucanía", guerras de fronteira do Cabo): texto sóbrio, sem gamificar o sofrimento, com opções que permitem resistir ou mudar o rumo.
6. **Nada sagrado como enfeite:** não usar objetos, cerimônias ou símbolos sagrados como decoração, ícone ou recompensa. O Banco Dourado Asante (*Sika Dwa Kofi*) pode aparecer em evento como o que é — o centro da soberania asante —, nunca como item a saquear.
7. **Fonte e revisão:** todo povo cadastrado tem fonte histórica no campo `fonte`. Conteúdo sobre povos indígenas passa por uma revisão específica (checklist da sec 8.2) antes de entrar.

### 6.7 Catálogo inicial de 1800 (semente — completar até ≥150)

Abaixo, as entidades de partida já definidas, por região cultural (= cena de seleção). **Traits e textos são proposta de design para balancear.** Tudo segue a ficha da sec 6.2. Datas indicam mudanças por evento.

#### Europa Ocidental
| TAG | País | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| GBR | Grã-Bretanha (Reino Unido a partir de 1801) | MON | Berço Industrial, Senhora dos Mares, Império Colonial | Ato de União 1801; abolição do tráfico 1807; "Casacas Vermelhas" |
| FRA | França (Consulado; Império 1804) | REP | Levée en Masse, Fervor Revolucionário, Centro Cultural | Código Napoleônico (decisão); bandeira tricolor/Bourbon; Grande Armée |
| ESP | Espanha | MON | Império Colonial, Absolutismo, Guerra de Guerrilha (ativa após 1808) | Constituição de Cádis 1812; independências americanas |
| POR | Portugal | MON | Império Colonial, Entreposto Comercial | Corte foge para o Rio de Janeiro em 1808 (a capital muda) |
| NED | República Batava (Países Baixos) | REP | Entreposto Comercial, Império Colonial | Reino da Holanda 1806; Reino Unido dos Países Baixos 1815; Bélgica 1830 |
| HEL | República Helvética (Suíça) | CONSELHO | Montanhas-Fortaleza, Confederação | Ato de Mediação 1803; neutralidade 1815 |

#### Europa Central e do Norte
| TAG | País | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| PRU | Prússia | MON | Exército Profissional, Reformador (após 1807) | Reformas Stein-Hardenberg; **Zollverein 1834** (a união aduaneira do jogo); unificação 1871 |
| HAB | Monarquia de Habsburgo (Império Austríaco 1804) | MON | Império Multiétnico, Centro Cultural, Celeiro (planície húngara) | Fim do Sacro Império 1806; Congresso de Viena 1815; Compromisso de 1867 |
| BAV, SAX, WUR, HAN, ... | Estados do Sacro Império (Baviera, Saxônia, Württemberg, Hanôver em união com GBR...) | MON | Confederação + 1 próprio | Confederação do Reno 1806; Confederação Germânica 1815 |
| SWE | Suécia (com a Finlândia) | MON | Exército Profissional | perde a Finlândia 1809; união com a Noruega 1814 |
| DAN | Dinamarca-Noruega | MON | Senhora dos Mares | Copenhague bombardeada 1807; perde a Noruega 1814 |

#### Europa Oriental, Mediterrâneo e Mundo Otomano
| TAG | País | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| RUS | Rússia | MON | Inverno Implacável, Absolutismo, Fronteira Aberta, Império Multiétnico | Geórgia anexada 1801; 1812; emancipação dos servos 1861; cossacos |
| OTT | Império Otomano | MON | Império Multiétnico, Gigante em Crise, Fé Unificadora (o sultão é califa), Reformador (Tanzimat 1839) | Revolta Sérvia 1804; Grécia 1821–1830; Crimeia 1853 |
| SAR | Reino da Sardenha (Piemonte) | MON | Reformador | Risorgimento -> Itália 1861 |
| NAP | Reino de Nápoles (Duas Sicílias 1816) | MON | Absolutismo, Celeiro, Centro Cultural | Expedição dos Mil 1860 |
| PAP | Estados Papais | TEO | Fé Unificadora, Centro Cultural, Glória Antiga | Roma 1870 |
| MNE | Montenegro | TEO | Montanhas-Fortaleza, Guerra de Guerrilha | príncipe-bispado; independência reconhecida 1878 |
| ALG / TUN / TRP | Regências de Argel, Túnis e Trípoli (súditos nominais de OTT) | MON | Corsários | Guerra Berberesca 1801–1805; França em Argel 1830 |
| EGY | Egito (súdito otomano; Muhammad Ali a partir de 1805) | MON | Reformador, Celeiro | modernização; campanha na Arábia 1811–1818 |

#### Américas — Estados e colônias
| TAG | País | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| USA | Estados Unidos | REP | Nação Jovem, Fronteira Aberta, Fervor Revolucionário | Compra da Louisiana 1803; 1812; Remoção Indígena 1830; Guerra Civil 1861 |
| HAI | Saint-Domingue / Haiti (independente 1804) | — (colônia francesa autônoma sob Louverture; Estado em 1804) | Guerra de Guerrilha, Conhecimento da Terra, Fervor Revolucionário | Toussaint Louverture; primeiro Estado nascido de uma revolução de escravizados; abolição imediata; Império 1804, divisão 1806–1820 (eventos) |
| NSP | Nova Espanha (súdito de ESP) | — | Riqueza Mineral | Grito de Dolores 1810 -> México 1821 |
| NGR / VPE / RDP | Nova Granada / Peru / Río de la Plata (súditos de ESP) | — | 1 cada | Gran Colombia 1819; Argentina 1816; Chile 1818; Bolívia 1825 |
| BRA | Brasil (colônia de POR) | — | Riqueza Mineral, Celeiro | Corte 1808; Independência 1822; Lei Áurea 1888; República 1889 |
| BNA | América do Norte Britânica (súdito de GBR) | — | Fronteira Aberta | Confederação do Canadá 1867 |
| RAM | América Russa (súdito de RUS) | — | Entreposto Comercial | vendida aos EUA em 1867 |

#### Américas — Povos
| TAG | Povo | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| HDS | Haudenosaunee (Confederação das Seis Nações) | CONSELHO | Confederação, Conhecimento da Terra | Grande Lei da Paz; diplomacia entre impérios |
| TSA | Tsalagi (Cherokee) | CONSELHO | Reformador, Comunidade Coesa | silabário de Sequoyah 1821; constituição 1827; Trilha das Lágrimas 1838 (evento a resistir) |
| MUS | Muscogee (Creek) | CONSELHO | Confederação, Conhecimento da Terra | Guerra Creek 1813–1814 |
| SEM | Seminole | CONSELHO | Conhecimento da Terra, Guerra de Guerrilha | Guerras Seminole 1816–1858; nunca se renderam formalmente |
| SHA | Shawnee | CONSELHO | Confederação, Conhecimento da Terra | Confederação de Tecumseh 1805–1813 (decisão: unir os povos do oeste) |
| ANI | Anishinaabe (Ojibwe e povos do Conselho dos Três Fogos) | CONSELHO | Confederação, Entreposto Comercial | comércio de peles; Grandes Lagos |
| CRE | Nēhiyawak (Cree) | CONSELHO | Conhecimento da Terra, Entreposto Comercial | rotas da Hudson's Bay Company |
| LAK | Očhéthi Šakówiŋ (Lakota, Dakota, Nakota) | CONSELHO | Cavaleiros das Planícies, Povo em Movimento | Paha Sápa (Black Hills); Little Bighorn 1876 |
| COM | Numunuu (Comanche) | CONSELHO | Cavaleiros das Planícies, Povo em Movimento | Comanchería, potência das planícies do sul |
| NII | Niitsitapi (Blackfoot) | CONSELHO | Cavaleiros das Planícies | confederação das planícies do norte |
| DIN | Diné (Navajo) | CONSELHO | Conhecimento da Terra, Comunidade Coesa | Longa Marcha 1864 (evento a resistir) |
| NDE | Ndé (Apache) | CONSELHO | Guerra de Guerrilha, Conhecimento da Terra | resistência até 1886 |
| INU | Inuit | AUTÔNOMO -> POVO | Conhecimento da Terra (ártico: atrito severo) | contato com baleeiros |
| HAW | Reino do Havaí (unificado em 1810) | MON | Senhora dos Mares, Reformador | Kamehameha I; constituição 1840; derrubada 1893 (evento a resistir) |
| MAP | Mapuche (Wallmapu) | CONSELHO | Conhecimento da Terra, Guerra de Guerrilha | fronteira do Biobío nunca conquistada pela Espanha; parlamentos/tratados; "Pacificação" 1861–1883 |
| AON | Aónikenk (Tehuelche) | CONSELHO | Povo em Movimento, Cavaleiros das Planícies | Patagônia; Conquista do Deserto 1878–1885 |
| WAY | Wayuu | CONSELHO | Conhecimento da Terra, Entreposto Comercial | La Guajira, nunca submetida pela Espanha; comércio com holandeses e britânicos |
| — | Povos da Amazônia (Munduruku, Mẽbêngôkre/Kayapó, ...) | AUTÔNOMO | Conhecimento da Terra | cadastrar um por bacia/região, com nome próprio |

#### África
| TAG | País/Povo | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| ASH | Império Asante (Ashanti) | MON | Riqueza Mineral, Exército Profissional | Banco Dourado (centro da soberania); guerras anglo-asante 1824–1900 |
| DAH | Daomé | MON | Disciplina Regimental | regimento feminino Agojie; economia ligada ao tráfico (instituição com custo, sec 6.4) |
| OYO | Império de Oió | MON | Cavaleiros das Planícies, Gigante em Crise | colapso nos anos 1830 |
| HSA | Cidades-Estado Hauçá (Gobir, Kano, Katsina... — uma TAG cada) | MON | Entreposto Comercial, Cavaleiros das Planícies | Jihad de Usman dan Fodio 1804 -> **Califado de Sokoto (SOK)** |
| BOR | Bornu | MON | Fé Unificadora, Cavaleiros das Planícies | rival de Sokoto |
| MOR | Marrocos (Sultanato Alauíta) | MON | Montanhas-Fortaleza, Fé Unificadora, Isolamento | guerra hispano-marroquina 1859 |
| ETH | Etiópia (Zemene Mesafint) | MON | Fé Unificadora, Montanhas-Fortaleza, Gigante em Crise | reunificação por Tewodros II 1855; **Adwa 1896** |
| MER | Reino Merina (Imerina) | MON | Reformador, Comunidade Coesa | Andrianampoinimerina; Radama I; França 1895 |
| MTH | Mthethwa -> **Zulu (ZUL)** | MON | Disciplina Regimental, Conhecimento da Terra | Shaka 1816; Mfecane; Isandlwana 1879 |
| XHO | amaXhosa | CONSELHO | Conhecimento da Terra, Guerra de Guerrilha | guerras de fronteira do Cabo 1779–1879 |
| CAP | Colônia do Cabo (VOC -> GBR 1806) | — | Fronteira Aberta | Grande Jornada 1835 -> Transvaal, Estado Livre de Orange |
| KON | Reino do Kongo | MON | Fé Unificadora, Comunidade Coesa, Gigante em Crise | reino cristão desde o séc. XV |
| LUN | Império Lunda | MON | Entreposto Comercial, Confederação | rotas comerciais da África Central |
| BUG | Buganda | MON | Comunidade Coesa, Burocracia Antiga | *kabaka* e *lukiiko* |
| RWA | Ruanda | MON | Montanhas-Fortaleza, Comunidade Coesa | reino das colinas |
| OMA | Omã (Mascate e Zanzibar) | MON | Senhora dos Mares, Entreposto Comercial | capital vai para Zanzibar em 1840 |

#### Ásia e Oriente Médio
| TAG | País/Povo | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| QNG | Dinastia Qing | MON | Burocracia Antiga, Celeiro, Centro Cultural, Império Multiétnico, Isolamento (Sistema de Cantão) | Guerras do Ópio 1839/1856; Taiping 1850; Autofortalecimento; 1911 |
| JAP | Japão (Xogunato Tokugawa) | MON | Isolamento (*sakoku*), Comunidade Coesa, Burocracia Antiga, Centro Cultural (alta alfabetização do período Edo) | Perry 1853; **Restauração Meiji 1868** troca Isolamento por Reformador |
| KOR | Joseon (Coreia) | MON | Burocracia Antiga, Comunidade Coesa, Centro Cultural, Isolamento | "Reino Eremita"; tratado de Ganghwa 1876 |
| NGU / TSN | Nguyễn e Tây Sơn (guerra pelo Vietnã) | MON | Guerra de Guerrilha, Burocracia Antiga | 1802: Gia Long unifica **Đại Việt / Việt Nam** se Nguyễn vencer |
| SIA | Sião (Rattanakosin) | MON | Reformador, Entreposto Comercial | Estado-tampão; Mongkut e Chulalongkorn; nunca colonizado |
| BRM | Birmânia (Konbaung) | MON | Exército Profissional | guerras anglo-birmanesas 1824/1852/1885 |
| NEP | Nepal (Gorkha) | MON | Montanhas-Fortaleza, Disciplina Regimental | Gurkhas; guerra anglo-nepalesa 1814 |
| SIK | Império Sikh (proclamado 1801) | MON | Exército Profissional, Fé Unificadora | Ranjit Singh; exército Khalsa; guerras anglo-sikh 1845/1848 |
| MAR | Confederação Maratha | CONSELHO | Confederação, Cavaleiros das Planícies | guerras anglo-maratha 1803–1818 |
| MUG | Império Mogol | MON | Glória Antiga | protetorado em Délhi; fim em 1857 |
| EIC | Companhia Britânica das Índias Orientais | COMPANHIA | Império Colonial, Entreposto Comercial | Rebelião de 1857 -> Raj britânico 1858 |
| HYD | Hyderabad (súdito: aliado subsidiário da EIC) | MON | Riqueza Mineral | Estado principesco |
| AFG | Império Durrani (Afeganistão) | MON | Montanhas-Fortaleza, Guerra de Guerrilha, Confederação | Grande Jogo; guerras anglo-afegãs 1839/1878 |
| QAJ | Pérsia (Qajar) | MON | Absolutismo, Fé Unificadora, Cavaleiros das Planícies | guerras russo-persas 1804/1826 |
| BUK / KHI / KOK | Bukhara, Khiva, Kokand | MON | Cavaleiros das Planícies | conquista russa 1865–1876 |
| KAZ | Cazaques (as Três Hordas) | CONSELHO | Povo em Movimento, Cavaleiros das Planícies | estepe; absorção russa |
| DRY | Emirado de Diriyah (Primeiro Estado Saudita) | TEO | Fé Unificadora, Povo em Movimento | destruído pelo Egito em 1818 (se a história seguir) |
| ACE | Sultanato de Aceh | MON | Entreposto Comercial, Guerra de Guerrilha | Guerra de Aceh 1873–1904 |
| SUL / BRU / JOH | Sulu, Brunei, Johor | MON | Entreposto Comercial, Senhora dos Mares | Singapura 1819 |
| RYU | Reino de Ryukyu | MON | Entreposto Comercial, Comunidade Coesa | tributário duplo; anexado 1879 |

#### Oceania e Pacífico
| TAG | Povo/País | Gov. | Traits | Flavor-chave |
|---|---|---|---|---|
| NGP / TAI / NTA | Ngāpuhi, Waikato-Tainui, Ngāi Tahu (iwi māori) | CONSELHO | Conhecimento da Terra (fortificações *pā*), Senhora dos Mares | Tratado de Waitangi 1840; Kīngitanga 1858; Guerras da Nova Zelândia |
| TON | Tonga | MON | Senhora dos Mares | unificação e constituição 1875; nunca colonizada |
| TAH | Taiti | MON | Senhora dos Mares | protetorado francês 1842 |
| NSW | Nova Gales do Sul (colônia de GBR) | — | Fronteira Aberta | expansão a partir de 1788 |
| — | Nações aborígenes (Eora, Darug, Wiradjuri, Kulin, Noongar, Yolŋu, Arrernte, Pitjantjatjara, ...) | AUTÔNOMO / POVO | Conhecimento da Terra, Comunidade Coesa | cadastrar por região, com nome próprio; eventos de fronteira e resistência |

**Países que surgem por evento** (catálogo, não estão no mapa em 1800): México, Gran Colombia, Argentina, Chile, Peru, Bolívia, Paraguai, Uruguai, Brasil independente, Império do Brasil, Texas, Estados Confederados, Canadá, Grécia, Sérvia, Romênia, Bulgária, Bélgica, Itália, Alemanha, Áustria-Hungria, Libéria, Transvaal, Estado Livre de Orange, Califado de Sokoto, Zulu, Raj britânico, Japão Meiji, República da China, Turquia, Polônia, Tchecoslováquia, Iugoslávia, Irlanda, Finlândia, Estados bálticos, entre outros.

---

## 7. História viva (só no Mapa-múndi)

### 7.1 Como funciona

- **Eventos ancorados:** cada evento histórico tem *data âncora*, *janela* (quanto tempo pode atrasar), *condições* e *efeitos*. Ex.: "Independência do Brasil — âncora 7/9/1822, janela 1820–1830; condição: BRA é súdito de POR **e** a corte portuguesa voltou a Lisboa **ou** POR está em guerra; efeito: BRA vira Estado (Império do Brasil)".
- **Pressões:** a IA de cada país tem *objetivos históricos* que empurram a história no rumo real (a Prússia busca a unificação alemã; a Rússia, os Estreitos; os EUA, o oeste; o Japão, depois de 1868, a modernização). Pressão não é script: se o país estiver fraco, falha.
- **Divergência:** condição falsa = o evento não acontece. Se a França perdeu Paris em 1805, não há Austerlitz nem Confederação do Reno. O jogo não "conserta" a história.
- **Eventos alternativos:** os eventos importantes têm um "e se" pronto quando a condição falha (ex.: se o Haiti for reconquistado, há revolta contínua; se a Prússia for anexada, a unificação pode vir pela Áustria ou pela Baviera).
- **O jogador escolhe:** eventos do país do jogador oferecem opções. A opção histórica fica marcada com um pequeno `✦` na janela do evento.

### 7.2 Linha do tempo base (âncoras principais — completar)

| Data | Evento | Quem |
|---|---|---|
| 1801 | Ato de União (Reino Unido); Império Sikh proclamado; Rússia anexa Kartli-Kakheti (Geórgia); Guerra Berberesca | GBR, SIK, RUS, TRP/USA |
| 1802 | Gia Long unifica o Vietnã | NGU |
| 1803 | Compra da Louisiana; 2ª Guerra Anglo-Maratha | USA/FRA, EIC/MAR |
| 1804 | **Independência do Haiti**; Napoleão imperador; Império Austríaco; Jihad de Usman dan Fodio (Sokoto); 1ª Revolta Sérvia | HAI, FRA, HAB, HSA, OTT |
| 1805 | Muhammad Ali no Egito; Confederação de Tecumseh | EGY, SHA |
| 1806 | **Fim do Sacro Império**; Confederação do Reno; GBR toma o Cabo | HAB, FRA, CAP |
| 1807 | Ducado de Varsóvia; GBR abole o tráfico de escravizados; reformas prussianas | FRA, GBR, PRU |
| 1808 | Guerra Peninsular; corte portuguesa vai ao Rio | ESP, POR, BRA |
| 1810 | Havaí unificado; início das independências hispano-americanas (Buenos Aires, Grito de Dolores) | HAW, RDP, NSP |
| 1812 | Invasão da Rússia; guerra anglo-americana; Constituição de Cádis | FRA, RUS, USA, GBR, ESP |
| 1815 | **Congresso de Viena**; Confederação Germânica; Reino Unido dos Países Baixos | todos |
| 1816 | Independência argentina; Shaka lidera os Zulu | RDP, MTH |
| 1818–1825 | Chile, Gran Colombia, México, Peru, **Brasil (1822)**, Bolívia; fim do Estado de Diriyah | América Latina, DRY |
| 1821–1830 | **Independência grega** | OTT |
| 1830 | Bélgica; França em Argel; Lei de Remoção Indígena (EUA) | NED, ALG, USA |
| 1833–1838 | Abolição no Império Britânico; Grande Jornada; Trilha das Lágrimas | GBR, CAP, TSA |
| 1834 | **Zollverein** | PRU |
| 1836 | Independência do Texas | NSP/México |
| 1839–1842 | 1ª Guerra do Ópio; Tanzimat | QNG, OTT |
| 1840 | Tratado de Waitangi | iwi māori |
| 1846–1848 | Guerra Mexicano-Americana; Libéria (1847); Guerra de Castas em Yucatán (1847); **Revoluções de 1848** | USA, México, Europa |
| 1850–1864 | Rebelião Taiping | QNG |
| 1853–1856 | Guerra da Crimeia; Perry no Japão | RUS, OTT, JAP |
| 1855 | Tewodros II reunifica a Etiópia | ETH |
| 1857–1858 | Rebelião indiana -> Raj britânico; fim do Império Mogol | EIC, MUG |
| 1861 | **Reino da Itália**; emancipação dos servos; Guerra Civil americana | SAR, RUS, USA |
| 1864–1870 | Guerra do Paraguai | BRA, Argentina, Uruguai, Paraguai |
| 1867 | Áustria-Hungria; Confederação do Canadá; venda do Alasca | HAB, BNA, RAM |
| 1868 | **Restauração Meiji** | JAP |
| 1871 | **Império Alemão** | PRU |
| 1876–1886 | Little Bighorn; Conquista do Deserto; resistência apache até 1886 | LAK, AON, NDE |
| 1878 | Congresso de Berlim (Sérvia, Montenegro, Romênia independentes) | OTT, Bálcãs |
| 1879 | Guerra Anglo-Zulu; Guerra do Pacífico | ZUL, Chile/Peru/Bolívia |
| 1884–1885 | **Conferência de Berlim** (partilha da África) | potências, toda a África |
| 1888–1889 | Lei Áurea; República no Brasil | BRA |
| 1893–1898 | Derrubada do Reino do Havaí; anexação | HAW, USA |
| 1894–1895 | Guerra Sino-Japonesa | QNG, JAP |
| 1896 | **Batalha de Adwa** | ETH |
| 1898 | Guerra Hispano-Americana | ESP, USA |
| 1899–1902 | Guerra dos Bôeres | GBR, Transvaal, Orange |
| 1904–1905 | Guerra Russo-Japonesa | RUS, JAP |
| 1910–1912 | Revolução Mexicana; Coreia anexada; República da China | México, KOR, QNG |
| 1914–1918 | **Grande Guerra** | potências |
| 1917 | Revoluções russas | RUS |
| 1918–1923 | Fim dos impérios: Polônia, Tchecoslováquia, Iugoslávia, Irlanda, Turquia | HAB, OTT, RUS, GBR |
| 1929 | Grande Depressão | mundo |
| 1935–1936 | Itália invade a Etiópia; Guerra Civil Espanhola | ETH, Itália, ESP |
| 1937 | Japão invade a China | JAP, China |
| 1939–1945 | **Segunda Guerra Mundial**; ONU fundada (1945) | potências, mundo |
| 1947 | Independência e partição da Índia e do Paquistão | Raj britânico |
| 1948 | Criação de Israel e guerra árabe-israelense; Declaração Universal dos Direitos Humanos | Oriente Médio, mundo |
| 1949 | República Popular da China; OTAN; Indonésia independente | China, Ocidente, NED |
| 1950–1953 | Guerra da Coreia | KOR, China, USA |
| 1954 | Fim da Indochina francesa; início da Guerra da Argélia | Vietnã, FRA, Argélia |
| 1956–1957 | Crise de Suez; **Sputnik** (corrida espacial); Gana independente | Egito, URSS, Gana |
| 1959 | Revolução Cubana | Cuba |
| 1960 | **Ano da África** (17 países independentes) | África |
| 1961–1962 | Muro de Berlim; Crise dos Mísseis; Argélia independente | Alemanha, USA, URSS, Argélia |
| 1965–1975 | Guerra do Vietnã; reunificação (1975) | Vietnã, USA |
| 1967 | Guerra dos Seis Dias | Oriente Médio |
| 1969 | **Pessoa na Lua** | corrida espacial |
| 1973 | Crise do petróleo; golpe no Chile | mundo, Chile |
| 1974–1975 | Revolução dos Cravos; independência de Angola, Moçambique, Guiné-Bissau, Cabo Verde, São Tomé | POR, África lusófona |
| 1979 | Revolução Iraniana; guerra soviética no Afeganistão | Irã, AFG, URSS |
| 1985–1988 | Redemocratização do Brasil; Constituição de 1988 reconhece direitos indígenas | BRA |
| 1989–1991 | **Queda do Muro**; reunificação alemã; **fim da URSS** (15 países) | Europa, URSS |
| 1992–1994 | Maastricht (União Europeia); Mercosul; fim do apartheid (Mandela presidente, 1994) | Europa, América do Sul, África do Sul |
| 1997 | Hong Kong volta à China | GBR, China |
| 2001 | Atentados de 11 de setembro; guerra no Afeganistão | USA, AFG |
| 2003 | Guerra do Iraque | USA, Iraque |
| 2007 | ONU adota a Declaração sobre os Direitos dos Povos Indígenas | povos indígenas, mundo |
| 2008 | Crise financeira global | mundo |
| 2010–2012 | Primavera Árabe; Sudão do Sul independente (2011) | Oriente Médio, África |
| 2016 | Referendo do Brexit | GBR |
| 2020 | **Pandemia de COVID-19** | mundo |
| 2022 | Rússia invade a Ucrânia | RUS, Ucrânia |
| 31/12/2025 | **Fim da história** — balanço e opção de continuar (sec 7.6) | — |

Eventos do século XX e XXI envolvem guerras e tragédias recentes, com pessoas vivas que as sofreram: texto sóbrio, factual, sem glorificar, sem ridicularizar nenhum povo, e as mesmas regras da sec 6.6.

### 7.3 Padrão de evento

```text
id, título (≤24), corpo (≤160 caracteres, PT + EN)
âncora (data), janela (início–fim), chance/dia dentro da janela
condições (país existe, possui região, está em guerra com, lei/governo, ano ≥, flag de evento)
opções (1–3): efeitos + marca ✦ na histórica
alternativa (id do evento "e se", quando a condição falhar no fim da janela)
fonte (referência histórica)
```

### 7.4 Opções de história (tela "Preparar partida")

- **Ligada:** eventos e pressões históricas completos.
- **Só flavor:** eventos nacionais e de era acontecem, mas não há pressão da IA para seguir a história.
- **Desligada:** sandbox; só eventos genéricos.

### 7.5 Eventos aleatórios (todos os mapas)

Eventos aleatórios dão textura ao dia a dia e são os mesmos no mapa-múndi e no aleatório. Regras:

- **Frequência:** cada país sorteia, em média, 1 evento a cada ~30 dias, com recarga de 20 dias depois de um evento (o jogador não é soterrado). Usa o RNG único do jogo (mesma seed = mesmos eventos).
- **Filtros:** cada evento tem **era mínima/máxima**, e pode exigir terreno (costa, montanha, deserto), governo, trait ou situação (em guerra, tesouro baixo, militância alta). Um evento fora do contexto nunca aparece (nada de "greve geral" em 1805 ou "ataque hacker" em 1880).
- **Escolha:** metade dos eventos tem 2 opções com troca real (dinheiro x felicidade, curto x longo prazo).
- **Peso por trait:** traits aumentam a chance de eventos coerentes (Senhora dos Mares -> mais eventos navais; Fé Unificadora -> mais eventos religiosos).

| Categoria | Eventos (era) |
|---|---|
| **Natureza** | Colheita farta · Seca · Enchente · Inverno rigoroso · Praga de gafanhotos · Terremoto · Erupção vulcânica ("ano sem verão", como em 1816) · Furacão (costa) · Incêndio na capital · Cometa no céu (só prestígio e superstição) |
| **Saúde** | Varíola (até 1980; vacina reduz) · Cólera (1817+) · Febre amarela (trópicos) · Gripe pandêmica (1889+, forte em 1918) · Vacinação em massa (1850+) · Descoberta médica (1880+) |
| **Economia** | Corrida do ouro · Descoberta de prata/carvão/ferro · Bolha especulativa · Pânico bancário · Boom de exportação · Praga na safra principal · Descoberta de petróleo (1859+) · Greve geral (1870+) · Grande invenção local (1830+) · Crise de dívida (1900+) |
| **Sociedade** | Onda de imigração · Êxodo para o campo/cidade · Revolta camponesa · Movimento abolicionista (até a abolição) · Sufragistas (1860+) · Gênio nasce (cientista, artista, inventor -> Grande Personagem menor) · Festival nacional · Exposição universal (1851+) · Jogos olímpicos (1896+) · Movimento estudantil (1960+) |
| **Política** | Escândalo na corte/no parlamento · Sucessão disputada (monarquias) · Golpe militar · Tentativa de assassinato · Reforma constitucional · Partido radical cresce · Rei vizinho pede exílio |
| **Militar** | Motim · Desertores · Herói de guerra · Espiões descobertos · Fortaleza cai por traição · Avanço em táticas (nova doutrina) |
| **Diplomacia** | Casamento real (monarquias) · Incidente de fronteira · Oferta de aliança · Pedido de asilo · Missão diplomática estrangeira · Tratado comercial proposto · Embargo |
| **Povos indígenas** | Tratado oferecido ou quebrado · Renascimento cultural · Aliança entre povos · Retomada de território · Reconhecimento de direitos (1950+) |
| **Moderno (1950+)** | Crise do petróleo local · Boom tecnológico · Ataque hacker (1990+) · Crise climática regional (1990+) · Bolha imobiliária · Mega-evento esportivo · Descoberta de lítio/terras raras (2000+) |

### 7.6 Fim da história (2025) e Modo Eterno

- **31/12/2025:** o jogo pausa e abre a tela **"Fim da história"**: balanço das Grandes Potências (prestígio, economia, exército, literacia, território), sua posição, objetivos e conquistas cumpridos, e a **Gazeta final** com os grandes momentos da sua partida.
- Duas escolhas: **"Encerrar"** (vai aos créditos; o save fica marcado como concluído) ou **"Continuar jogando"** (Modo Eterno).
- **Modo Eterno:** o calendário segue sem limite. Não há mais âncoras históricas; continuam os eventos aleatórios (eras Globalização e Eterna) e entram as **corridas do futuro**: energia de fusão, base lunar permanente, inteligência artificial geral, colônia em Marte, cura de uma grande doença, mundo neutro em carbono. A pontuação pós-2025 aparece separada da pontuação de 2025, para o balanço histórico não se perder.
- O jogador também pode escolher **"Continuar jogando"** depois de perder seu país em qualquer ano (sec 2.1, item 7).

---

### 7.7 O mundo que conversa (diplomacia da IA)

Cada país tem um **rival**, até **dois aliados** e às vezes um **mercado comum**. Quem ataca um aliado chama o outro para a guerra. Grandes potências mercantis formam blocos regionais. O temperamento de cada governo muda a cada década — o vizinho calmo de hoje pode ser o agressor de amanhã.

O jogador recebe **despachos** (o jogo pausa): propostas de aliança, convites ao mercado comum, pedidos de ajuda, exigências de tributo ("pague ou é guerra"), ofertas de paz e **decisões** com duas saídas (greve: negociar ou reprimir; barricadas: ceder ao povo ou esmagar a revolta). No mapa, a **bandeira** abre a diplomacia com o país selecionado. **Ambições** (3 por vez, com recompensa) apontam o próximo passo: fundar uma cidade, abrir uma mina, formar uma aliança, lucrar com o comércio.

## 8. Como seguir os padrões (checklists)

### 8.1 Adicionar um país ou povo
- [ ] Ficha completa (sec 6.2), com `fonte`.
- [ ] Endônimo correto + nome em PT; glifos novos desenhados na fonte, se precisar.
- [ ] Traits dentro do orçamento (sec 6.4) e passando no teste do flavor.
- [ ] Cor de mapa contrastando com todos os vizinhos de 1800 **e** com os vizinhos que surgem por evento.
- [ ] Bandeira histórica do período, ou estandarte de jogo documentado (sec 6.2.1).
- [ ] Mínimo de flavor da sec 6.5.
- [ ] Descrição de 2 linhas na voz do jogo (sec 3.4).
- [ ] Se for povo indígena: checklist 8.2.
- [ ] Contagem do continente atualizada (meta da sec 6.1).

### 8.2 Revisão de representação (povos indígenas e povos colonizados)
- [ ] Usa endônimo/nome respeitoso; nenhuma palavra como "tribo selvagem", "primitivo", "bárbaro" fora de citação histórica explícita.
- [ ] Forças e fraquezas têm base histórica e fonte; fraquezas vêm do contato colonial, não do povo.
- [ ] Nenhum símbolo ou objeto sagrado como decoração ou recompensa.
- [ ] Eventos de violência colonial com texto sóbrio e opção de resistência.
- [ ] Arte (retrato/cena/estandarte) baseada em referência documentada do período e da região.
- [ ] O povo é jogável ou tem motivo escrito para não ser (AUTÔNOMO), com caminho para se tornar jogável.

### 8.3 Adicionar um trait
- [ ] Usa alavanca existente do motor (sem sistema novo).
- [ ] Peso definido (+2/+1/−1) e ≥2 países históricos que o justificam.
- [ ] Efeito descrito em uma linha que cabe no tooltip (Serifa 8, ~60 caracteres).

### 8.4 Adicionar um evento histórico
- [ ] Padrão da sec 7.3 completo, com data verificada em fonte.
- [ ] Condições impedem o evento de acontecer num mundo que já divergiu.
- [ ] Tem alternativa se for um evento que muda fronteiras.
- [ ] Texto na voz do jogo; opção histórica marcada com `✦`.

### 8.5 Criar uma tela
- [ ] Segue a gramática do SDD sec 14.5 e as regras da sec 3.2 deste documento.
- [ ] Tem layout em pixels escrito no SDD antes do código.
- [ ] Funciona só com botões **e** só com a caneta.

### 8.6 Criar um asset
- [ ] Pixel nativo, regras da sec 3.3, entra no inventário do SDD sec 14.6 com tamanho e prioridade.
- [ ] Orçamento de memória conferido no `.map`.

---

## 9. Do jogo de hoje até esta visão (marcos de design)

O SDD sec 14.9 tem as fases técnicas F1–F8 das telas. O mundo entra em marcos próprios, depois da F4:

| Marco | Entrega | Pronto quando |
|---|---|---|
| **M1 — Células e frentes** | mapa celular + regiões + guerra por frente (sec 4.5) **no mapa aleatório pequeno**, com os 3 países atuais | partida de 30 min no DS a 60fps; frente avança visivelmente |
| **M2 — Mapa estilo Victoria** | zoom Z0–Z3, papel -> terreno, nomes espaçados, fronteiras de 3 pesos, modos de mapa | screenshot de cada zoom aprovado contra a sec 4.2–4.4 |
| **M3 — Catálogo** | ficha de país, bandeira procedural, traits, ≥40 países do catálogo no aleatório | aleatório com 16 países sorteados; teste do flavor passa em 5 países |
| **M4 — Mapa-múndi 1800** | Terra real, ~600 regiões, ≥150 entidades de 1800 | todos os continentes jogáveis; meta de 15% por continente |
| **M5 — História viva** | eventos ancorados, pressões, alternativas, eras, bandeiras por era, eventos aleatórios (sec 7.5) | 1800–1836 roda na velocidade 5 sem crash e com divergência visível entre seeds |
| **M6 — Até 2025** | eventos completos até 2025, países modernos, objetivos de Campanha, balanço de Grande Potência, tela "Fim da história" | partida completa chega a 2025 com balanço |
| **M7 — Diversão** | Ambições, Os Primeiros, Grandes Personagens, Gazeta, Rivais com personalidade, Blitz, Modo Eterno, conquistas (sec 2.1) | playtest: em 30 minutos o jogador cumpre 1 Ambição, vê 1 corrida e lê 1 Gazeta |

**Ja no codigo (branch `feat/mundo`), sem tela ainda:** o nucleo do mundo — calendario 1800-2025 + eras + Modo Eterno (`calendar.c`), 30 traits com efeitos (`traits.c`), texto UTF-8 -> 3x5 (`txt.c`), catalogo de paises e povos (`nations.c`, `include/nations.h`), bandeiras procedurais (`px_flag.c`), mascara de terra real (`earth_mask.c`), motor de historia viva + eventos aleatorios (`history.c`) e o mundo celular com frentes (`world.c`, M1). Testes de nucleo no PC em `host/` (`sh host/run_tests.sh`). O caminho ate a tela esta nos marcos M2-M7 acima e nas fases da v0.9 (SDD sec 14.9); o plano ordenado (baseline, ordem, dependencias, riscos) esta em `docs/ROADMAP.md` (+ `docs/roadmap.png`).

### 9.1 Impacto técnico que o SDD precisa resolver antes do M1
- O mundo sai do `GameState` (limite de 14KB): a grade de células, as regiões e os países vão para um estado próprio em `.bss`. Estimativa do mapa-múndi: dono das células `u8` 512x192 = 96KB de RAM; região por célula e terreno em ROM; ~600 regiões x 4 fatias + ~250 países ≈ 90KB. Cabe nos 4MB, mas precisa de ADR e de novo `_Static_assert`.
- Economia por fatia de região, **escalonada** (cada fatia atualiza 1x a cada N dias) para caber nos 30ms por dia; IA por país em rodízio.
- Texto de eventos e nomes em PT + EN pode passar de centenas de KB: reavaliar NitroFS (ADR-14 do SDD prevê isso).
- Em 2025 existem ~195 Estados, mais povos e entidades históricas: o catálogo passa de 300 e o limite de 254 donos simultâneos (`u8`) fica apertado. Se um mundo real chegar perto disso, o dono da célula vira `u16` (+96KB de RAM) — decisão do SDD.
- 82 mil dias de partida: tudo que é "por dia" precisa ser barato ou escalonado; contadores de dia são `u32`.
- **Fluidez (medida, SDD ADR-36):** o alvo é o mapa a 60 fps no pan no DSi (ARM9 a 134 MHz) e jogável no DS. No ARM9 emulado o dia do mapa-múndi custa ~36 ms de piso (acima dos 30 ms planejados), por isso ele roda em fases e, nas velocidades 4–5, por orçamento de tempo por quadro: quando a CPU não dá conta, quem cede é o relógio do jogo, nunca a fluidez da tela. Medir com `make PROF=1` no melonDS/DS.
- Seleção de país com ≥150 opções: a tela "Escolha sua nação" vira **carrossel de 5 estandartes com filtro por continente**, mais a opção de escolher tocando no mapa (SDD sec 14.4).
