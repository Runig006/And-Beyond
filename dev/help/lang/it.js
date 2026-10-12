// Help page texts: Italian. Loaded by index.html after help.js.
HELP.it = {
  lang_name: "Italiano",
  tab: "Ambulatorio 1",
  room: "Sala d'attesa",
  patch: "Patch",
  eyebrow: "Cartella clinica · senza appuntamento",
  beyond: "& Beyond",
  urgent: "Pronto soccorso retrò",
  tagline: "Il tuo gioco del 2000, funzionante su Windows 11",
  h_reason: "Anamnesi: motivo della visita",
  reason_intro: "Il paziente (Disney/Pixar, Traveller's Tales, 2000) arriva al pronto soccorso con questi sintomi:",
  symptoms: [
    "<b>Non si installa:</b> il suo installatore è a 16 bit e Windows a 64 bit non lo apre.",
    "<b>Grafica DirectX 6:</b> sui computer di oggi si blocca o si vede male.",
    "<b>Va a scatti:</b> la velocità saltava tra 60 e 30 fotogrammi al secondo.",
    "<b>Schermo piccolo:</b> niente oltre 1024×768 e nessuno schermo panoramico.",
    "<b>Niente controller moderni</b> e una telecamera che non si poteva muovere.",
    "<b>Poche lingue:</b> su PC è uscito in inglese, italiano e portoghese (e poche altre)."
  ],
  h_always: "Diagnosi e trattamento di base",
  always_intro: "Sempre incluso, niente da attivare:",
  always: [
    "<b>Nuovo motore grafico</b> (Direct3D 11) dentro <code>DDraw.dll</code>: il gioco del tuo disco non viene modificato.",
    "<b>60 fotogrammi al secondo</b> stabili.",
    "<b>Qualsiasi risoluzione</b> e schermi panoramici: l'interfaccia resta ai bordi; menu e video in 4:3 con bande.",
    "<b>Portatile:</b> nessun registro di Windows; la cartella del gioco si può spostare o copiare.",
    "<b>Controller e mouse:</b> nuova schermata dei comandi in cui ogni azione ha un tasto, un pulsante del mouse e uno del controller. Di base <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> per muoversi e <kbd>Q</kbd> <kbd>E</kbd> per lo spostamento laterale; tutto si può cambiare.",
    "<b>Due nuove difficoltà:</b> <i>comandante</i> (i colpi fanno 1,7 volte più male) e <i>oltre</i> (2,5 volte di più e metà nemici in più).",
    "<b>Nella pausa:</b> «opzioni di controllo» e «riavvia livello».",
    "<b>Sei lingue</b> per i testi del gioco."
  ],
  h_options: "Posologia: opzioni della patch",
  options_intro: "Nel menu opzioni del gioco c'è «opzioni patch». Ogni opzione, cosa fa e perché c'è:",
  legend: "I valori come li mostra il gioco; quello evidenziato è il predefinito della patch.",
  why: "Perché:",
  h_installer: "Istruzioni per l'uso: l'installatore",
  installer: [
    "<b>Installa:</b> chiede l'ISO del gioco per PC e copia tutto; il CD non serve più per giocare.",
    "<b>Traduzione:</b> mette le voci e i video di un altro disco (Dreamcast, PlayStation o il PC in un'altra lingua). Gli originali vengono conservati per poterci tornare.",
    "<b>Aggiorna:</b> quando esce una nuova versione, apri il nuovo installatore e scegli il <code>buzz.exe</code> del tuo gioco. I salvataggi non vengono toccati.",
    "<b>Disinstalla:</b> cancella la cartella del gioco (e i suoi collegamenti, se li hai creati). Non si installa nient'altro in Windows."
  ],
  h_patient: "Scheda del paziente",
  fields: [
    {
      label: "Paziente",
      value: "Buzz Lightyear of Star Command"
    },
    {
      label: "Nascita",
      value: "2000 · Traveller's Tales"
    },
    {
      label: "Età",
      value: "{age} anni"
    },
    {
      label: "Sistema d'origine",
      value: "Windows 95/98 · DirectX 6"
    },
    {
      label: "Allergie",
      value: "Windows a 64 bit, schermi panoramici"
    },
    {
      label: "Medico",
      value: "Dott. Runig"
    }
  ],
  stamp: "Stabile",
  stamp_by: "Dott. Runig",
  vitals: [
    {
      label: "Fotogrammi al secondo",
      value: "60",
      note: "stabili"
    },
    {
      label: "Risoluzione",
      value: "la tua",
      note: "panoramica"
    },
    {
      label: "Lingue",
      value: "6",
      note: "testi"
    },
    {
      label: "Opzioni",
      value: "22",
      note: "a piacere"
    }
  ],
  h_leaflet: "Foglietto illustrativo: effetti indesiderati",
  leaflet: [
    "<b>Come ogni cura, ogni tanto può non funzionare.</b> Il gioco è del 2000 e ogni computer è diverso.",
    "<b>Se qualcosa va storto:</b> disattiva l'opzione relativa in «opzioni patch»; se non basta, riapri l'installatore e premi Aggiorna (o reinstalla sopra). I salvataggi non si perdono.",
    "<b>Controindicazioni:</b> solo per la versione PC. Il motore funziona con le versioni inglese, spagnola, italiana, tedesca e francese; la portoghese non si può installare (il suo buzz.exe è troppo diverso), ma il suo disco serve in Traduzione per le voci e i video. La francese ha una protezione anticopia (DRM): si avvia solo con il disco originale nel lettore (un'immagine non va). Consigliamo di installare un'altra versione e usare il disco francese in Traduzione per le voci e i video."
  ],
  h_small: "Clausole in piccolo",
  small: [
    "Patch non ufficiale, gratuita e fatta da appassionati. Nessun legame con Disney, Pixar, Activision o Traveller's Tales.",
    "Serve la tua copia del gioco per PC: la patch non contiene nulla del gioco.",
    "Il pulsante Traduzione usa ffmpeg (licenza LGPL), incluso nell'installer: la sua licenza e dove trovare il codice sorgente sono nella cartella <code>setup</code> del gioco."
  ],
  footer: "Dimesso. Avanti il prossimo.",
  options: [
    {
      name: "lingua",
      values: ["english", "espanol", "francais", "deutsch", "italiano", "portugues"],
      what: "Lingua dei testi del gioco e della patch. Cambia subito.",
      why: "Le versioni PC uscirono in poche lingue."
    },
    {
      name: "schermo",
      values: ["intero", "finestra"],
      default: "intero",
      what: "Schermo intero (senza bordi) o finestra. Anche <kbd>Alt</kbd>+<kbd>Invio</kbd> cambia.",
      why: "L'originale cambiava la risoluzione di tutto il desktop."
    },
    {
      name: "risoluzione",
      values: ["1920x1080", "..."],
      what: "Risoluzione del gioco. Si applica al prossimo avvio.",
      why: "L'originale non andava oltre 1024×768."
    },
    {
      name: "controlli",
      values: ["moderno", "direzionale", "rotazionale"],
      default: "moderno",
      what: "<i>Moderno</i>: la telecamera si muove con il mouse o lo stick destro e Buzz spara dove guardi. <i>Direzionale</i> e <i>rotazionale</i> sono i due schemi originali.",
      why: "Giocare come un gioco di oggi senza perdere i comandi di sempre."
    },
    {
      name: "mira auto",
      values: ["no", "si"],
      default: "si",
      what: "Con i comandi moderni, i colpi deviano un po' verso il nemico più vicino al mirino.",
      why: "Con la telecamera libera è difficile colpire i nemici piccoli."
    },
    {
      name: "velocita camera",
      values: ["lenta", "normale", "veloce"],
      default: "normale",
      what: "Velocità con cui lo stick destro gira la telecamera.",
      why: "Ogni controller e ogni mano è diverso."
    },
    {
      name: "inverti visuale",
      values: ["no", "si"],
      default: "no",
      what: "Alzando lo stick o il mouse si guarda in basso.",
      why: "C'è chi ha sempre giocato così."
    },
    {
      name: "info armi",
      values: ["no", "si"],
      default: "si",
      what: "Livello dell'arma e munizioni massime, nell'interfaccia.",
      why: "Il gioco non diceva quanto migliora l'arma né fin dove si può caricare."
    },
    {
      name: "salvataggio auto",
      values: ["no", "si"],
      default: "si",
      what: "Salva alla fine di ogni livello nello slot in uso e lo ricarica al titolo. Un dischetto lampeggia durante il salvataggio.",
      why: "Nell'originale, morendo tornavi al menu e perdevi ciò che non avevi salvato a mano."
    },
    {
      name: "info obiettivi",
      values: ["no", "si"],
      default: "si",
      what: "Crediti richiesti dalla medaglia e tempo da battere, nell'interfaccia.",
      why: "Il gioco non li mostrava mentre giocavi."
    },
    {
      name: "distanza visiva",
      values: ["originale", "lontano"],
      default: "lontano",
      what: "Scenario e nemici disegnati il doppio più lontano.",
      why: "Nel 2000 bisognava tagliare; i computer di oggi hanno margine."
    },
    {
      name: "nebbia pianeti",
      values: ["originale", "attiva"],
      default: "attiva",
      what: "Nebbia colorata su Canis Lunis 1, Ice World, Bathyos e Planet Z.",
      why: "Atmosfera: nel PC originale la nebbia era programmata, ma non si vedeva mai."
    },
    {
      name: "trasparenze",
      values: ["no", "si"],
      default: "si",
      what: "Ciò che sta tra la telecamera e Buzz diventa trasparente.",
      why: "Muri e colonne nascondevano Buzz."
    },
    {
      name: "scena boss",
      values: ["originale", "sempre"],
      default: "sempre",
      what: "Se vinci la corsa contro il boss, vedi comunque la sua scena prima dello scontro.",
      why: "Vincendo la corsa perdevi l'entrata del boss."
    },
    {
      name: "vita nemici",
      values: ["originale", "no"],
      default: "no",
      what: "<i>Originale</i>: i nemici hanno più vita nei livelli avanzati. <i>No</i>: sempre quella dei primi livelli.",
      why: "Per chi vuole un gioco più leggero."
    },
    {
      name: "immunita nemici",
      values: ["originale", "no"],
      default: "no",
      what: "<i>Originale</i>: dopo un colpo il nemico non può morire per un attimo. <i>No</i>: ogni colpo conta.",
      why: "Con le armi rapide, molti colpi non servivano a nulla."
    },
    {
      name: "avviso prezzi",
      values: ["no", "si"],
      default: "si",
      what: "I prezzi delle armi lampeggiano in rosso finché non hai abbastanza soldi.",
      why: "Su PC i prezzi erano sempre bianchi; altre versioni avvisavano."
    },
    {
      name: "mine eque",
      values: ["no", "si"],
      default: "si",
      what: "Le mine nemiche esplodono solo quando Buzz è davvero vicino.",
      why: "La loro portata superava il disegno: esplodevano da lontano."
    },
    {
      name: "rottami torrette",
      values: ["originale", "innocui"],
      default: "innocui",
      what: "I rottami di una torretta distrutta spingono Buzz senza ferirlo.",
      why: "Una torretta distrutta continuava a fare danni."
    },
    {
      name: "mira veicolo",
      values: ["originale", "camera"],
      default: "camera",
      what: "Con i comandi moderni, i veicoli sparano dove guarda la telecamera.",
      why: "Mirare con la telecamera, come a piedi."
    },
    {
      name: "camera veicolo",
      values: ["libera", "sterzo"],
      default: "libera",
      what: "<i>Libera</i>: il mouse o lo stick muovono la telecamera. <i>Sterzo</i>: girano la moto e la tavola.",
      why: "Ognuno preferisce qualcosa di diverso."
    },
    {
      name: "camera bathyos",
      values: ["originale", "alta"],
      default: "alta",
      what: "Su Bathyos la telecamera sale e mira sopra Buzz.",
      why: "Il sottomarino nascondeva ciò che c'era davanti."
    }
  ]
};
