// Help page texts: French. Loaded by index.html after help.js.
HELP.fr = {
  lang_name: "Français",
  tab: "Cabinet 1",
  room: "Salle d'attente",
  patch: "Patch",
  eyebrow: "Dossier médical · sans rendez-vous",
  beyond: "& Beyond",
  urgent: "Urgences rétro",
  tagline: "Ton jeu de l'an 2000, qui marche sous Windows 11",
  h_reason: "Anamnèse : motif de la consultation",
  reason_intro: "Le patient (Disney/Pixar, Traveller's Tales, 2000) arrive aux urgences avec ces symptômes :",
  symptoms: [
    "<b>Ne s'installe pas :</b> son installateur est en 16 bits et Windows 64 bits ne l'ouvre pas.",
    "<b>Graphismes DirectX 6 :</b> sur les ordinateurs d'aujourd'hui, ça plante ou ça s'affiche mal.",
    "<b>Saccades :</b> la vitesse sautait entre 60 et 30 images par seconde.",
    "<b>Petit écran :</b> rien au-dessus de 1024×768 et aucun écran large.",
    "<b>Pas de manettes modernes</b> et une caméra impossible à bouger.",
    "<b>Peu de langues :</b> sur PC, il est sorti en anglais, italien et portugais (et quelques autres)."
  ],
  h_always: "Diagnostic et traitement de base",
  always_intro: "Toujours inclus, rien à activer :",
  always: [
    "<b>Un nouveau moteur graphique</b> (Direct3D 11) dans <code>DDraw.dll</code> : le jeu de ton disque n'est pas modifié.",
    "<b>60 images par seconde</b> stables.",
    "<b>N'importe quelle résolution</b> et les écrans larges : l'interface reste aux bords ; menus et vidéos en 4:3 avec bandes.",
    "<b>Portable :</b> pas de registre Windows ; le dossier du jeu peut être déplacé ou copié.",
    "<b>Manette et souris :</b> un nouvel écran des commandes où chaque action a une touche, un bouton de souris et un bouton de manette. Par défaut <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> pour bouger et <kbd>Q</kbd> <kbd>E</kbd> pour le pas de côté (sur un clavier AZERTY, change-les dans l'écran des commandes) ; tout se modifie.",
    "<b>Deux nouvelles difficultés :</b> <i>commandant</i> (les coups font 1,7 fois plus mal) et <i>au-delà</i> (2,5 fois plus et moitié plus d'ennemis).",
    "<b>Dans la pause :</b> « options de contrôle » et « recommencer niveau ».",
    "<b>Six langues</b> pour les textes du jeu."
  ],
  h_options: "Posologie : options du patch",
  options_intro: "Le menu des options du jeu contient « options du patch ». Chaque option, ce qu'elle fait et pourquoi elle est là :",
  legend: "Les valeurs telles que le jeu les affiche ; celle marquée est celle du patch par défaut.",
  why: "Pourquoi :",
  h_installer: "Mode d'emploi : l'installateur",
  installer: [
    "<b>Installer :</b> demande l'ISO du jeu PC et copie tout ; le CD n'est plus nécessaire pour jouer.",
    "<b>Traduction :</b> met les voix et les vidéos d'un autre disque (Dreamcast, PlayStation ou le PC dans une autre langue). Les originaux sont gardés pour pouvoir y revenir.",
    "<b>Mettre à jour :</b> quand une nouvelle version sort, ouvre le nouvel installateur et choisis le <code>buzz.exe</code> de ton jeu. Tes sauvegardes ne sont pas touchées.",
    "<b>Désinstaller :</b> supprimez le dossier du jeu (et ses raccourcis, si vous les avez créés). Rien d'autre n'est installé dans Windows."
  ],
  h_patient: "Fiche du patient",
  fields: [
    {
      label: "Patient",
      value: "Buzz Lightyear of Star Command"
    },
    {
      label: "Naissance",
      value: "2000 · Traveller's Tales"
    },
    {
      label: "Âge",
      value: "{age} ans"
    },
    {
      label: "Système d'origine",
      value: "Windows 95/98 · DirectX 6"
    },
    {
      label: "Allergies",
      value: "Windows 64 bits, écrans larges"
    },
    {
      label: "Médecin",
      value: "Dr Runig"
    }
  ],
  stamp: "Stable",
  stamp_by: "Dr Runig",
  vitals: [
    {
      label: "Images par seconde",
      value: "60",
      note: "stables"
    },
    {
      label: "Résolution",
      value: "la tienne",
      note: "écran large"
    },
    {
      label: "Langues",
      value: "6",
      note: "textes"
    },
    {
      label: "Options",
      value: "22",
      note: "à ton goût"
    }
  ],
  h_leaflet: "Notice : effets indésirables",
  leaflet: [
    "<b>Comme tout traitement, il peut échouer de temps en temps.</b> Le jeu date de 2000 et chaque ordinateur est différent.",
    "<b>Si quelque chose ne va pas :</b> désactive l'option concernée dans « options du patch » ; si rien ne change, rouvre l'installateur et clique sur Mettre à jour (ou réinstalle par-dessus). Tes sauvegardes ne sont pas perdues.",
    "<b>Contre-indications :</b> version PC uniquement. Le moteur marche avec les versions anglaise, espagnole, italienne, allemande et française ; la portugaise ne peut pas être installée (son buzz.exe est trop différent), mais son disque sert dans Traduction pour les voix et les vidéos. La française a une protection anti-copie (DRM) : elle ne démarre qu'avec le disque original dans le lecteur (une image ne marche pas). Nous recommandons d'installer une autre version et d'utiliser le disque français dans Traduction pour les voix et les vidéos."
  ],
  h_small: "Petits caractères",
  small: [
    "Patch non officiel, gratuit et fait par des fans. Sans lien avec Disney, Pixar, Activision ni Traveller's Tales.",
    "Il te faut ta propre copie du jeu PC : le patch n'apporte rien du jeu.",
    "Le bouton Traduction utilise ffmpeg (licence LGPL), fourni dans l'installateur : sa licence et l'emplacement de son code source se trouvent dans le dossier <code>setup</code> du jeu."
  ],
  footer: "Sortie autorisée. Au suivant !",
  options: [
    {
      name: "langue",
      values: ["english", "espanol", "francais", "deutsch", "italiano", "portugues"],
      what: "Langue des textes du jeu et du patch. Change tout de suite.",
      why: "Les versions PC sont sorties en peu de langues."
    },
    {
      name: "affichage",
      values: ["plein ecran", "fenetre"],
      default: "plein ecran",
      what: "Plein écran (sans bordure) ou fenêtre. <kbd>Alt</kbd>+<kbd>Entrée</kbd> bascule aussi.",
      why: "L'original changeait la résolution de tout le bureau."
    },
    {
      name: "resolution",
      values: ["1920x1080", "..."],
      what: "Résolution du jeu. Appliquée au prochain lancement.",
      why: "L'original ne dépassait pas 1024×768."
    },
    {
      name: "controles",
      values: ["moderne", "directionnel", "rotatif"],
      default: "moderne",
      what: "<i>Moderne</i> : la caméra bouge avec la souris ou le stick droit et Buzz tire là où tu regardes. <i>Directionnel</i> et <i>rotatif</i> sont les deux schémas d'origine.",
      why: "Jouer comme à un jeu d'aujourd'hui sans perdre les commandes d'antan."
    },
    {
      name: "visee auto",
      values: ["non", "oui"],
      default: "oui",
      what: "Avec les commandes modernes, les tirs dévient un peu vers l'ennemi le plus proche du viseur.",
      why: "Avec une caméra libre, les petits ennemis sont durs à toucher."
    },
    {
      name: "vitesse camera",
      values: ["lente", "normale", "rapide"],
      default: "normale",
      what: "Vitesse de rotation de la caméra au stick droit.",
      why: "Chaque manette et chaque main est différente."
    },
    {
      name: "inverser vue",
      values: ["non", "oui"],
      default: "non",
      what: "Monter le stick ou la souris fait regarder vers le bas.",
      why: "Certains ont toujours joué ainsi."
    },
    {
      name: "info armes",
      values: ["non", "oui"],
      default: "oui",
      what: "Niveau de l'arme et munitions maximum, dans l'interface.",
      why: "Le jeu ne disait jamais de combien l'arme s'améliore ni jusqu'où on peut la charger."
    },
    {
      name: "sauvegarde auto",
      values: ["non", "oui"],
      default: "oui",
      what: "Sauvegarde après chaque niveau dans l'emplacement utilisé et la recharge à l'écran titre. Une disquette clignote pendant la sauvegarde.",
      why: "Dans l'original, mourir renvoyait au menu et faisait perdre ce qui n'était pas sauvegardé à la main."
    },
    {
      name: "info objectifs",
      values: ["non", "oui"],
      default: "oui",
      what: "Crédits demandés par la médaille et temps à battre, dans l'interface.",
      why: "Le jeu ne les montrait pas en jouant."
    },
    {
      name: "distance de vue",
      values: ["original", "loin"],
      default: "loin",
      what: "Le décor et les ennemis sont dessinés deux fois plus loin.",
      why: "En 2000, il fallait couper ; les ordinateurs d'aujourd'hui ont de la marge."
    },
    {
      name: "brume planetes",
      values: ["original", "activee"],
      default: "activee",
      what: "Brume colorée sur Canis Lunis 1, Ice World, Bathyos et Planet Z.",
      why: "Ambiance : le jeu PC d'origine avait une brume programmée, mais elle ne se voyait jamais."
    },
    {
      name: "transparence",
      values: ["non", "oui"],
      default: "oui",
      what: "Ce qui se trouve entre la caméra et Buzz devient transparent.",
      why: "Les murs et les colonnes cachaient Buzz."
    },
    {
      name: "scene du boss",
      values: ["original", "toujours"],
      default: "toujours",
      what: "Si tu gagnes la course contre le boss, tu vois quand même sa scène avant le combat.",
      why: "Gagner la course faisait rater l'entrée du boss."
    },
    {
      name: "vie ennemis",
      values: ["original", "non"],
      default: "non",
      what: "<i>Original</i> : les ennemis ont plus de vie dans les niveaux avancés. <i>Non</i> : toujours celle des premiers niveaux.",
      why: "Pour qui veut un jeu plus léger."
    },
    {
      name: "immunite ennemis",
      values: ["original", "non"],
      default: "non",
      what: "<i>Original</i> : après un coup, l'ennemi ne peut pas mourir un instant. <i>Non</i> : chaque tir compte.",
      why: "Avec les armes rapides, beaucoup de tirs ne servaient à rien."
    },
    {
      name: "alerte prix",
      values: ["non", "oui"],
      default: "oui",
      what: "Les prix des armes clignotent en rouge tant que tu n'as pas assez.",
      why: "Sur PC, les prix étaient toujours blancs ; d'autres versions prévenaient."
    },
    {
      name: "mines justes",
      values: ["non", "oui"],
      default: "oui",
      what: "Les mines ennemies n'explosent que si Buzz est vraiment près.",
      why: "Leur portée dépassait leur dessin : elles explosaient de loin."
    },
    {
      name: "debris tourelles",
      values: ["original", "inoffensifs"],
      default: "inoffensifs",
      what: "Les débris d'une tourelle détruite poussent Buzz sans le blesser.",
      why: "Une tourelle détruite continuait à faire mal."
    },
    {
      name: "visee vehicule",
      values: ["original", "camera"],
      default: "camera",
      what: "Avec les commandes modernes, les véhicules tirent là où regarde la caméra.",
      why: "Viser avec la caméra, comme à pied."
    },
    {
      name: "camera vehicule",
      values: ["libre", "diriger"],
      default: "libre",
      what: "<i>Libre</i> : la souris ou le stick bougent la caméra. <i>Diriger</i> : ils tournent la moto et la planche.",
      why: "Chacun ses préférences."
    },
    {
      name: "camera bathyos",
      values: ["original", "haute"],
      default: "haute",
      what: "Sur Bathyos, la caméra monte et vise au-dessus de Buzz.",
      why: "Le sous-marin cachait ce qu'il y avait devant."
    }
  ]
};
