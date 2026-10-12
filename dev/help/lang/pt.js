// Help page texts: Portuguese. Loaded by index.html after help.js.
HELP.pt = {
  lang_name: "Português",
  tab: "Consultório 1",
  room: "Sala de espera",
  patch: "Patch",
  eyebrow: "Ficha clínica · sem marcação",
  beyond: "& Beyond",
  urgent: "Urgências retro",
  tagline: "O teu jogo de 2000, a funcionar no Windows 11",
  h_reason: "Anamnese: motivo da consulta",
  reason_intro: "O paciente (Disney/Pixar, Traveller's Tales, 2000) chega às urgências com estes sintomas:",
  symptoms: [
    "<b>Não instala:</b> o instalador é de 16 bits e o Windows de 64 bits não o abre.",
    "<b>Gráficos DirectX 6:</b> nos computadores de hoje falha ou vê-se mal.",
    "<b>Aos solavancos:</b> a velocidade saltava entre 60 e 30 imagens por segundo.",
    "<b>Ecrã pequeno:</b> nada acima de 1024×768 e nenhum ecrã panorâmico.",
    "<b>Sem comandos modernos</b> e com uma câmara que não se podia mexer.",
    "<b>Poucas línguas:</b> no PC saiu em inglês, italiano e português (e mais algumas)."
  ],
  h_always: "Diagnóstico e tratamento de série",
  always_intro: "Vem sempre, sem ativar nada:",
  always: [
    "<b>Motor gráfico novo</b> (Direct3D 11) dentro de <code>DDraw.dll</code>: o jogo do teu disco não é alterado.",
    "<b>60 imagens por segundo</b> estáveis.",
    "<b>Qualquer resolução</b> e ecrãs panorâmicos: a interface fica nas margens; menus e vídeos em 4:3 com barras.",
    "<b>Portátil:</b> não usa o registo do Windows; a pasta do jogo pode ser movida ou copiada.",
    "<b>Comando e rato:</b> novo ecrã de controlos em que cada ação tem tecla, botão do rato e botão do comando. Por omissão <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> para andar e <kbd>Q</kbd> <kbd>E</kbd> para o passo lateral; tudo se pode mudar.",
    "<b>Duas dificuldades novas:</b> <i>comandante</i> (os golpes tiram 1,7 vezes mais) e <i>mais alem</i> (2,5 vezes mais e metade mais de inimigos).",
    "<b>Na pausa:</b> «opcoes de controle» e «reiniciar nivel».",
    "<b>Seis línguas</b> para os textos do jogo."
  ],
  h_options: "Posologia: opções do patch",
  options_intro: "No menu de opções do jogo está «opcoes do patch». Cada opção, o que faz e porque existe:",
  legend: "Os valores como o jogo os mostra; o marcado é o de origem do patch.",
  why: "Porquê:",
  h_installer: "Modo de usar: o instalador",
  installer: [
    "<b>Instalar:</b> pede a ISO do jogo de PC e copia tudo; o CD deixa de ser preciso para jogar.",
    "<b>Tradução:</b> põe as vozes e os vídeos de outro disco (Dreamcast, PlayStation ou o PC noutra língua). Os originais ficam guardados para voltar a eles.",
    "<b>Atualizar:</b> quando sair uma versão nova, abre o instalador novo e escolhe o <code>buzz.exe</code> do teu jogo. Os teus jogos guardados não são tocados.",
    "<b>Desinstalar:</b> apaga a pasta do jogo (e os seus atalhos, se os criaste). Nada mais é instalado no Windows."
  ],
  h_patient: "Ficha do paciente",
  fields: [
    {
      label: "Paciente",
      value: "Buzz Lightyear of Star Command"
    },
    {
      label: "Nascimento",
      value: "2000 · Traveller's Tales"
    },
    {
      label: "Idade",
      value: "{age} anos"
    },
    {
      label: "Sistema de origem",
      value: "Windows 95/98 · DirectX 6"
    },
    {
      label: "Alergias",
      value: "Windows de 64 bits, ecrãs panorâmicos"
    },
    {
      label: "Médico",
      value: "Dr. Runig"
    }
  ],
  stamp: "Estável",
  stamp_by: "Dr. Runig",
  vitals: [
    {
      label: "Imagens por segundo",
      value: "60",
      note: "estáveis"
    },
    {
      label: "Resolução",
      value: "a tua",
      note: "panorâmica"
    },
    {
      label: "Línguas",
      value: "6",
      note: "textos"
    },
    {
      label: "Opções",
      value: "22",
      note: "a gosto"
    }
  ],
  h_leaflet: "Folheto: efeitos secundários",
  leaflet: [
    "<b>Como qualquer tratamento, pode falhar de vez em quando.</b> O jogo é de 2000 e cada computador é um mundo.",
    "<b>Se algo correr mal:</b> desativa a opção relacionada em «opcoes do patch»; se não chegar, volta a abrir o instalador e carrega em Atualizar (ou instala por cima). Os teus jogos guardados não se perdem.",
    "<b>Contraindicações:</b> só para a versão de PC. O motor funciona com as versões inglesa, espanhola, italiana, alemã e francesa; a portuguesa não pode ser instalada (o seu buzz.exe é demasiado diferente), mas o seu disco serve na Tradução para as vozes e os vídeos. A francesa tem uma proteção anticópia (DRM): só arranca com o disco original na unidade (uma imagem não serve). Recomendamos instalar outra versão e usar o disco francês na Tradução para as vozes e os vídeos."
  ],
  h_small: "Letras pequenas",
  small: [
    "Patch não oficial, gratuito e feito por fãs. Sem relação com a Disney, Pixar, Activision ou Traveller's Tales.",
    "Precisas da tua própria cópia do jogo de PC: o patch não traz nada do jogo.",
    "O botão Tradução usa o ffmpeg (licença LGPL), que vem dentro do instalador: a sua licença e onde está o seu código-fonte ficam na pasta <code>setup</code> do jogo."
  ],
  footer: "Alta médica. O seguinte, por favor.",
  options: [
    {
      name: "idioma",
      values: ["english", "espanol", "francais", "deutsch", "italiano", "portugues"],
      what: "Língua dos textos do jogo e do patch. Muda logo.",
      why: "As versões de PC saíram em poucas línguas."
    },
    {
      name: "tela",
      values: ["cheia", "janela"],
      default: "cheia",
      what: "Ecrã inteiro (sem margens) ou janela. <kbd>Alt</kbd>+<kbd>Enter</kbd> também alterna.",
      why: "O original mudava a resolução de todo o ambiente de trabalho."
    },
    {
      name: "resolucao",
      values: ["1920x1080", "..."],
      what: "Resolução do jogo. Aplica-se no próximo arranque.",
      why: "O original não passava de 1024×768."
    },
    {
      name: "controle",
      values: ["moderno", "direcional", "giratorio"],
      default: "moderno",
      what: "<i>Moderno</i>: a câmara mexe-se com o rato ou o stick direito e o Buzz dispara para onde olhas. <i>Direcional</i> e <i>giratorio</i> são os dois esquemas originais.",
      why: "Jogar como um jogo de hoje sem perder os controlos de sempre."
    },
    {
      name: "mira automatica",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "Com o controlo moderno, os tiros desviam-se um pouco para o inimigo mais perto da mira.",
      why: "Com a câmara livre, custa acertar nos inimigos pequenos."
    },
    {
      name: "velocidade camera",
      values: ["lenta", "normal", "rapida"],
      default: "normal",
      what: "Rapidez com que o stick direito roda a câmara.",
      why: "Cada comando e cada mão é diferente."
    },
    {
      name: "inverter visao",
      values: ["desligado", "ligado"],
      default: "desligado",
      what: "Subir o stick ou o rato faz olhar para baixo.",
      why: "Há quem sempre tenha jogado assim."
    },
    {
      name: "info armas",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "Nível da arma e munição máxima, na interface.",
      why: "O jogo nunca dizia quanto a arma melhora nem até onde se pode carregar."
    },
    {
      name: "gravacao auto",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "Guarda no fim de cada nível na ranhura em uso e carrega-a no ecrã título. Uma disquete pisca ao guardar.",
      why: "No original, morrer levava ao menu e perdias o que não tinhas guardado à mão."
    },
    {
      name: "info objetivos",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "Créditos que a medalha pede e tempo a bater, na interface.",
      why: "O jogo não os mostrava durante o jogo."
    },
    {
      name: "distancia visual",
      values: ["original", "longe"],
      default: "longe",
      what: "O cenário e os inimigos desenham-se duas vezes mais longe.",
      why: "Em 2000 era preciso cortar; os computadores de hoje têm folga."
    },
    {
      name: "nevoa planetas",
      values: ["original", "ativada"],
      default: "ativada",
      what: "Nevoeiro colorido em Canis Lunis 1, Ice World, Bathyos e Planet Z.",
      why: "Ambiente: no PC original o nevoeiro estava programado, mas nunca se via."
    },
    {
      name: "transparencias",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "O que fica entre a câmara e o Buzz fica transparente.",
      why: "Paredes e colunas tapavam o Buzz."
    },
    {
      name: "cena do chefe",
      values: ["original", "sempre"],
      default: "sempre",
      what: "Se ganhares a corrida ao chefe, vês na mesma a cena dele antes do combate.",
      why: "Ganhar a corrida fazia perder a entrada do chefe."
    },
    {
      name: "vida inimigos",
      values: ["original", "desligado"],
      default: "desligado",
      what: "<i>Original</i>: os inimigos têm mais vida nos níveis avançados. <i>Desligado</i>: sempre a dos primeiros níveis.",
      why: "Para quem quer um jogo mais leve."
    },
    {
      name: "imunidade inimiga",
      values: ["original", "desligado"],
      default: "desligado",
      what: "<i>Original</i>: depois de um golpe, o inimigo não pode morrer durante um instante. <i>Desligado</i>: cada tiro conta.",
      why: "Com armas rápidas, muitos tiros não faziam nada."
    },
    {
      name: "aviso precos",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "Os preços das armas piscam a vermelho enquanto não tens dinheiro suficiente.",
      why: "No PC os preços eram sempre brancos; outras versões avisavam."
    },
    {
      name: "minas justas",
      values: ["desligado", "ligado"],
      default: "ligado",
      what: "As minas inimigas só rebentam quando o Buzz está mesmo perto.",
      why: "O alcance era maior do que o desenho: rebentavam de longe."
    },
    {
      name: "restos torres",
      values: ["original", "inofensivos"],
      default: "inofensivos",
      what: "Os destroços de uma torre destruída empurram o Buzz sem lhe tirar vida.",
      why: "Uma torre destruída continuava a fazer dano."
    },
    {
      name: "mira veiculo",
      values: ["original", "camera"],
      default: "camera",
      what: "Com o controlo moderno, os veículos disparam para onde a câmara olha.",
      why: "Apontar com a câmara, como a pé."
    },
    {
      name: "camera veiculo",
      values: ["livre", "dirigir"],
      default: "livre",
      what: "<i>Livre</i>: o rato ou o stick mexem a câmara. <i>Dirigir</i>: viram a mota e a prancha.",
      why: "Cada um prefere uma coisa."
    },
    {
      name: "camera bathyos",
      values: ["original", "alta"],
      default: "alta",
      what: "Em Bathyos a câmara sobe e aponta por cima do Buzz.",
      why: "O submarino tapava o que havia à frente."
    }
  ]
};
