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
> Status: **visão aprovada, nada do mundo 1800 implementado ainda.** O jogo jogável hoje
> (v0.7) é um mapa procedural 32x24 com 3 facções. O caminho até aqui está na sec 9.

---

## 1. A visão em uma frase

**Victoria 3 no bolso: o mundo de 1800 inteiro, com dezenas de países históricos e povos
indígenas que têm identidade própria, pintado num mapa político vivo onde as fronteiras
avançam como em frontwars.io — e a história real acontecendo ao seu redor até você mudá-la.**

## 2. Pilares (todo recurso novo tem que servir a pelo menos um, e não ferir nenhum)

1. **Profundidade de Victoria.** Sociedade (POPs, felicidade, literacia, militância), economia em cadeia, mercados, leis/governos, diplomacia. Decisões têm consequências que levam anos para aparecer.
2. **Legibilidade e ritmo de frontwars.io.** Território é cor pintada célula a célula; guerra é uma *frente* que avança e recua visivelmente; você manda "50% do exército para lá" e vê acontecer. Em 2 segundos olhando o mapa, o jogador sabe quem está ganhando.
3. **História viva.** O modo Mapa-múndi começa em **1º de janeiro de 1800** e a história real acontece — Haiti independente em 1804, fim do Sacro Império em 1806, independências latino-americanas, unificações, partilha da África — **a menos que o mundo do jogo já tenha divergido**. Cada evento histórico tem condição; se ela não é verdade, a história segue outro rumo.
4. **Todos os povos importam.** O mundo de 1800 não tem "terra vazia". Impérios, reinos, sultanatos, confederações e povos indígenas são entidades com nome, território, traits, objetivos e voz. Ninguém é cenário.
5. **Feito para o DS.** Duas telas de 256x192, D-pad + botões + caneta, 4MB de RAM, ARM9 de 67MHz. Se uma ideia não cabe legível em 256x192 ou não roda em 30ms por dia de jogo, ela é redesenhada — não espremida.

**Teste do pilar:** antes de implementar algo, escreva em uma linha qual pilar ele serve. Se não conseguir, não implemente.

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

- **Pixel art nativo 1:1** para 256x192. Nunca reescale a concept (ela não está em resolução nativa).
- Luz de cima-esquerda, outline escuro de 1px, sem anti-aliasing contra transparente, paleta quantizada para 15 bits.
- **Pintura histórica do séc. XIX como referência** (Turner, Delacroix, gravuras de jornal ilustrado da época, retratos oficiais), traduzida para pixel. Cores quentes de pôr do sol, céus dramáticos, arquitetura reconhecível.
- **Retratos e figuras de povos não europeus** são desenhados a partir de referência histórica documentada (roupas, armas, arquitetura, paisagem do período), nunca de caricatura ou fantasia genérica de "tribo". Ver sec 6.5.
- Cenas de seleção de país: uma por **região cultural** (sec 4.6) e cenas únicas para os países de destaque. Cada cena mostra **gente + lugar + época** (ex.: a concept do Reino Unido: soldado, Union Jack, Tâmisa, Westminster).

### 3.4 Texto e voz

- PT-BR com acento (fonte Serifa, SDD sec 14.3). Inglês como segundo idioma.
- Tom de **despacho de chancelaria**: frases curtas, sóbrias, levemente solenes. "Conquiste territórios, desenvolva sua nação e escreva sua história." — não "Bora dominar o mundo!!!".
- Descrição de país na seleção: **até 2 linhas, ~90 caracteres**, dizendo o que o torna único *mecanicamente* e *historicamente* ao mesmo tempo.
- Evento: título de até 24 caracteres + corpo de até 4 linhas Serifa 8 (~160 caracteres) + 1 a 3 opções.
- **Nomes próprios com a grafia correta**, incluindo diacríticos de outras línguas (Māori, Očhéthi Šakówiŋ, Đại Việt, Asante). O conjunto de glifos da fonte é **gerado a partir de `str.c`**: um nome novo com letra nova obriga a desenhar o glifo, não a remover o acento.

### 3.5 Não faça

- ❌ Gradiente, sombra suave, vidro, neon, flat design moderno, emoji.
- ❌ Fonte sem serifa ou fonte 3x5 em tela do jogador (a 3x5 é só debug).
- ❌ Crimson em qualquer coisa que não seja seleção.
- ❌ Cor literal fora dos tokens (SDD sec 14.1) e `RGB15()` cru (vira pixel transparente — ADR-13).
- ❌ Asset recortado da concept ou reescalado.
- ❌ Estereótipo visual de qualquer povo (sec 6.5).

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
| **Z2 — Região** | 1 célula = 4px | **terreno pintado** (floresta, montanha, deserto, estepe, selva, tundra) com a cor do país por cima a ~30% | nomes de regiões, cidades, capitais, exércitos como figurinhas |
| **Z3 — Detalhe** | 1 célula = 8px | terreno detalhado no estilo da concept: castelos nas capitais com a bandeira tremulando, casinhas nas cidades, navios nos portos, pinheiros densos | prédios da região, frente célula a célula |

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
3. **Atacar:** botão Marchar -> toca/seleciona o país alvo no mapa -> escolhe **25/50/75/100%** da reserva (ESQ/DIR ou slider tocável). Abre-se uma **frente** em toda a fronteira comum.
4. **A frente avança célula a célula.** A cada tick, as células da fronteira comparam força de ataque (tropas na frente x moral x armas x traits) com defesa (densidade defensiva x terreno x fortes x traits). Vence -> célula muda de dono e a frente anda. As tropas da frente se gastam ao avançar; frente sem tropas para.
5. **Terreno importa como no Victoria:** montanha, floresta, pântano, deserto, rio cortam o avanço; fortes seguram regiões.
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
| História | trilho histórico ligado (sec 7) | sem trilho; só eventos genéricos (clima, colheita, revolta) e flavor de cada país |
| Data | começa em 1/1/1800, termina em 31/12/1936 | sem calendário histórico (dia 1, 2, 3...) |
| Vitória | ranking de **Grande Potência** em 1936 (ou antes por objetivos), mais objetivos históricos do seu país | % do território ou maior pontuação no limite de tempo |
| Modos | **Campanha** (objetivos fixos do país) e **Contra IA** (sandbox) | **Contra IA**, **Multijogador** |

Na tela "Preparar partida" do mapa-múndi há também **História: `◂ Ligada ▸` / `◂ Só flavor ▸` / `◂ Desligada ▸`** (sec 7.4).

---

## 5. Linha do tempo de jogo e eras

O mapa-múndi é dividido em eras. A era muda música, molduras de evento, tecnologias liberadas e a **variante das bandeiras**.

| Era | Anos | Marca do período | Muda no jogo |
|---|---|---|---|
| **Napoleônica** | 1800–1815 | Revolução exportada, coalizões, fim do Sacro Império | guerras frequentes; Levée en Masse; eventos de coalizão |
| **Restauração** | 1815–1848 | Congresso de Viena, independências latino-americanas | eventos de independência; ferrovia aparece (anos 1830) |
| **Primavera dos Povos** | 1848–1871 | revoluções de 1848, unificações italiana e alemã, Guerra Civil dos EUA | nacionalismo forte; telégrafo; aço |
| **Imperialismo** | 1871–1914 | partilha da África, Meiji, corrida naval | colonização acelerada; resistência anticolonial; eletricidade |
| **Crise** | 1914–1936 | Grande Guerra, revoluções, Depressão | guerras totais; novos países; colapso de impérios |

Tecnologias (ferrovia, telégrafo, aço, eletricidade...) entram como **prédios novos liberados por era + literacia**, reaproveitando o sistema de obras. Detalhar no SDD quando chegar a hora; não crie árvore tecnológica separada (UI e RAM do DS).

---

## 6. Países e povos

### 6.1 Metas de quantidade

- **≥150 entidades no mapa em 1/1/1800**, de todos os continentes (grandes potências, Estados menores, súditos, povos indígenas, autônomos).
- **≥250 no catálogo total**, incluindo os que **surgem por evento** (México, Grécia, Bélgica, Itália, Alemanha, Libéria, Transvaal...).
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
| 31/12/1936 | **Fim da partida** | — |

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

---

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
| **M5 — História viva** | eventos ancorados, pressões, alternativas, eras, bandeiras por era | 1800–1836 roda na velocidade 3 sem crash e com divergência visível entre seeds |
| **M6 — Até 1936** | eventos completos até 1936, objetivos de Campanha, ranking de Grande Potência | partida completa termina com ranking |

### 9.1 Impacto técnico que o SDD precisa resolver antes do M1
- O mundo sai do `GameState` (limite de 14KB): a grade de células, as regiões e os países vão para um estado próprio em `.bss`. Estimativa do mapa-múndi: dono das células `u8` 512x192 = 96KB de RAM; região por célula e terreno em ROM; ~600 regiões x 4 fatias + ~250 países ≈ 90KB. Cabe nos 4MB, mas precisa de ADR e de novo `_Static_assert`.
- Economia por fatia de região, **escalonada** (cada fatia atualiza 1x a cada N dias) para caber nos 30ms por dia; IA por país em rodízio.
- Texto de eventos e nomes em PT + EN pode passar de centenas de KB: reavaliar NitroFS (ADR-14 do SDD prevê isso).
- Seleção de país com ≥150 opções: a tela "Escolha sua nação" vira **carrossel de 5 estandartes com filtro por continente**, mais a opção de escolher tocando no mapa (SDD sec 14.4).
