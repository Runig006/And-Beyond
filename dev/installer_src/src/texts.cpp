// Installer texts (see texts.h). Our own ones in English, Spanish, French, German, Italian and Portuguese, in the
// language chosen at the start.
#include "texts.h"

#include <windows.h>

#include <cstring>

static const wchar_t *const kEnglish[] = {
	// The product's name, never translated (only the difficulty "beyond" is, inside the game)
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Choose the game's PC CD image",
	/* T_PICK_ISO_OR_GAME */ L"Choose the game's PC CD image, or the buzz.exe of the installed game",
	/* T_ISO_OR_GAME_FILTER */ L"CD image or installed game",
	/* T_NOT_OURS */ L"That game was not installed with this program. Choose the CD image to install it.",
	/* T_ISO_FILTER */ L"CD image",
	/* T_BAD_ISO */ L"This file is not the PC CD of Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"This version cannot be installed with the patch: its buzz.exe is too different from the others. Its disc can be used in Translation, to put its voices and videos in another version.",
	/* T_NEEDS_DISC */ L"This version has a copy protection (DRM): the game only starts with the original disc in the drive (a disc image does not work). We recommend installing another version and using this disc in Translation for the French voices and videos (the French texts: patch options > language).",
	/* T_PICK_FOLDER */ L"Choose where to install (a folder for the game will be created)",
	// %s = the game's folder
	/* T_FOLDER_NOTE */ L"This is the root folder: the folder \"%s\" will be created inside it, with the game.",
	/* T_INSTALL_HERE */ L"Install here",
	/* T_CONFIRM_DIR */ L"The game will be installed in:",
	/* T_INSTALLING */ L"Installing...",
	/* T_INSTALLED */ L"The game is installed.",
	/* T_COPY_FAILED */ L"Could not write:",
	/* T_TRANSLATION */ L"Translation",
	/* T_ASK_LINKS */ L"The game is installed. Create shortcuts on the desktop and in the Start menu?",
	/* T_ASK_VOICES */ L"Do you want the voices and videos from another disc (Dreamcast, PlayStation or PC)? It can also be done later with setup\\BuzzSetup.exe in the game folder.",
	/* T_VOICES_NOTE */ L"The game's texts are chosen inside the game (options). Here: the voices and videos.",
	/* T_FROM_DISC */ L"Voices and videos from another disc\nDreamcast (.cdi), PlayStation (.bin) or PC in another language (.iso)",
	/* T_RESTORE */ L"Back to the voices and videos of the original installation",
	/* T_PICK_DISC */ L"Choose the disc image: Dreamcast, PlayStation or PC",
	/* T_DISC_FILTER */ L"Disc image",
	/* T_BAD_DISC */ L"This file is not a disc of Buzz Lightyear of Star Command (Dreamcast, PlayStation or PC).",
	/* T_NO_FFMPEG */ L"Could not write ffmpeg.exe into the game's setup folder.",
	/* T_CONVERTING */ L"Converting voices and videos...",
	/* T_VOICES_DONE */ L"Voices and videos are in place.",
	/* T_RESTORED */ L"The voices and videos of the original installation are back.",
	/* T_CONVERT_FAILED */ L"Could not convert:",
	/* T_NO_PATCH */ L"no patch",
	/* T_OLD_PATCH */ L"old patch",
	/* T_INFO_SETUP */ L"Installer:",
	/* T_INFO_INSTALLED */ L"Installed:",
	/* T_INFO_CD */ L"CD:",
	/* T_UPDATE */ L"Update",
	/* T_UPDATED */ L"The patch is up to date. Now \"Translation\" is available.",
	/* T_INFO_CONTENT */ L"Voices and videos:",
	/* T_INFO_EXE */ L"Executable:",
	/* T_LANG_EN */ L"English",
	/* T_LANG_ES */ L"Spanish",
	/* T_LANG_IT */ L"Italian",
	/* T_LANG_PT */ L"Portuguese",
	/* T_LANG_FR */ L"French",
	/* T_LANG_DE */ L"German",
	/* T_LANG_OTHER */ L"another language",
	/* T_HELP_LINK */ L"Patch help",
	/* T_GAME_HELP_LINK */ L"Game help",
	// Short lines: it goes in a narrow box
	/* T_DISCLAIMER */ L"Unofficial patch.\nNot affiliated with\nDisney, Pixar,\nActivision or\nTraveller's Tales.",
};

static const wchar_t *const kSpanish[] = {
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Elige la imagen del CD de PC del juego",
	/* T_PICK_ISO_OR_GAME */ L"Elige la imagen del CD de PC del juego, o el buzz.exe del juego instalado",
	/* T_ISO_OR_GAME_FILTER */ L"Imagen de CD o juego instalado",
	/* T_NOT_OURS */ L"Ese juego no se instaló con este programa. Elige la imagen del CD para instalarlo.",
	/* T_ISO_FILTER */ L"Imagen de CD",
	/* T_BAD_ISO */ L"Este fichero no es el CD de PC de Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"Esta versión no se puede instalar con el parche: su buzz.exe es demasiado distinto de los demás. Su disco sí sirve en Traducción, para poner sus voces y vídeos en otra versión.",
	/* T_NEEDS_DISC */ L"Esta versión tiene una protección anticopia (DRM): el juego solo arranca con el disco original en la unidad (una imagen de disco no vale). Recomendamos instalar otra versión y usar este disco en Traducción para las voces y los vídeos en francés (los textos en francés: opciones parche > idioma).",
	/* T_PICK_FOLDER */ L"Elige dónde instalar (se creará una carpeta para el juego)",
	/* T_FOLDER_NOTE */ L"Esta es la carpeta raíz: dentro se creará la carpeta «%s» con el juego.",
	/* T_INSTALL_HERE */ L"Instalar aquí",
	/* T_CONFIRM_DIR */ L"El juego se instalará en:",
	/* T_INSTALLING */ L"Instalando...",
	/* T_INSTALLED */ L"El juego está instalado.",
	/* T_COPY_FAILED */ L"No se pudo escribir:",
	/* T_TRANSLATION */ L"Traducción",
	/* T_ASK_LINKS */ L"El juego está instalado. ¿Crear accesos directos en el escritorio y en el menú Inicio?",
	/* T_ASK_VOICES */ L"¿Quieres poner las voces y los vídeos de otro disco (Dreamcast, PlayStation o PC)? También se puede hacer luego con setup\\BuzzSetup.exe, en la carpeta del juego.",
	/* T_VOICES_NOTE */ L"Los textos del juego se eligen dentro del juego (opciones). Aquí: las voces y los vídeos.",
	/* T_FROM_DISC */ L"Voces y vídeos de otro disco\nDreamcast (.cdi), PlayStation (.bin) o PC en otro idioma (.iso)",
	/* T_RESTORE */ L"Volver a las voces y vídeos de la instalación original",
	/* T_PICK_DISC */ L"Elige la imagen del disco: Dreamcast, PlayStation o PC",
	/* T_DISC_FILTER */ L"Imagen de disco",
	/* T_BAD_DISC */ L"Este fichero no es un disco de Buzz Lightyear of Star Command (Dreamcast, PlayStation o PC).",
	/* T_NO_FFMPEG */ L"No se ha podido escribir ffmpeg.exe en la carpeta setup del juego.",
	/* T_CONVERTING */ L"Convirtiendo voces y vídeos...",
	/* T_VOICES_DONE */ L"Las voces y los vídeos ya están puestos.",
	/* T_RESTORED */ L"Ya vuelven a estar las voces y los vídeos de la instalación original.",
	/* T_CONVERT_FAILED */ L"No se pudo convertir:",
	/* T_NO_PATCH */ L"sin parche",
	/* T_OLD_PATCH */ L"parche antiguo",
	/* T_INFO_SETUP */ L"Instalador:",
	/* T_INFO_INSTALLED */ L"Instalado:",
	/* T_INFO_CD */ L"CD:",
	/* T_UPDATE */ L"Actualizar",
	/* T_UPDATED */ L"El parche está actualizado. Ya puedes usar \"Traducción\".",
	/* T_INFO_CONTENT */ L"Voces y vídeos:",
	/* T_INFO_EXE */ L"Ejecutable:",
	/* T_LANG_EN */ L"inglés",
	/* T_LANG_ES */ L"castellano",
	/* T_LANG_IT */ L"italiano",
	/* T_LANG_PT */ L"portugués",
	/* T_LANG_FR */ L"francés",
	/* T_LANG_DE */ L"alemán",
	/* T_LANG_OTHER */ L"otro idioma",
	/* T_HELP_LINK */ L"Ayuda del parche",
	/* T_GAME_HELP_LINK */ L"Ayuda del juego",
	/* T_DISCLAIMER */ L"Parche no oficial.\nSin relación con\nDisney, Pixar,\nActivision ni\nTraveller's Tales.",
};

static const wchar_t *const kFrench[] = {
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Choisis l'image du CD PC du jeu",
	/* T_PICK_ISO_OR_GAME */ L"Choisis l'image du CD PC du jeu, ou le buzz.exe du jeu installé",
	/* T_ISO_OR_GAME_FILTER */ L"Image de CD ou jeu installé",
	/* T_NOT_OURS */ L"Ce jeu n'a pas été installé avec ce programme. Choisis l'image du CD pour l'installer.",
	/* T_ISO_FILTER */ L"Image de CD",
	/* T_BAD_ISO */ L"Ce fichier n'est pas le CD PC de Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"Cette version ne peut pas être installée avec le patch : son buzz.exe est trop différent des autres. Son disque peut servir dans Traduction, pour mettre ses voix et ses vidéos dans une autre version.",
	/* T_NEEDS_DISC */ L"Cette version a une protection anti-copie (DRM) : le jeu ne démarre qu'avec le disque original dans le lecteur (une image disque ne marche pas). Nous recommandons d'installer une autre version et d'utiliser ce disque dans Traduction pour les voix et les vidéos en français (les textes en français : options patch > langue).",
	/* T_PICK_FOLDER */ L"Choisis où installer (un dossier pour le jeu sera créé)",
	/* T_FOLDER_NOTE */ L"C'est le dossier racine : le dossier « %s » sera créé dedans, avec le jeu.",
	/* T_INSTALL_HERE */ L"Installer ici",
	/* T_CONFIRM_DIR */ L"Le jeu sera installé dans :",
	/* T_INSTALLING */ L"Installation...",
	/* T_INSTALLED */ L"Le jeu est installé.",
	/* T_COPY_FAILED */ L"Impossible d'écrire :",
	/* T_TRANSLATION */ L"Traduction",
	/* T_ASK_LINKS */ L"Le jeu est installé. Créer des raccourcis sur le bureau et dans le menu Démarrer ?",
	/* T_ASK_VOICES */ L"Veux-tu les voix et les vidéos d'un autre disque (Dreamcast, PlayStation ou PC) ? Cela peut aussi se faire plus tard avec setup\\BuzzSetup.exe, dans le dossier du jeu.",
	/* T_VOICES_NOTE */ L"Les textes du jeu se choisissent dans le jeu (options). Ici : les voix et les vidéos.",
	/* T_FROM_DISC */ L"Voix et vidéos d'un autre disque\nDreamcast (.cdi), PlayStation (.bin) ou PC dans une autre langue (.iso)",
	/* T_RESTORE */ L"Revenir aux voix et vidéos de l'installation d'origine",
	/* T_PICK_DISC */ L"Choisis l'image du disque : Dreamcast, PlayStation ou PC",
	/* T_DISC_FILTER */ L"Image de disque",
	/* T_BAD_DISC */ L"Ce fichier n'est pas un disque de Buzz Lightyear of Star Command (Dreamcast, PlayStation ou PC).",
	/* T_NO_FFMPEG */ L"Impossible d'écrire ffmpeg.exe dans le dossier setup du jeu.",
	/* T_CONVERTING */ L"Conversion des voix et vidéos...",
	/* T_VOICES_DONE */ L"Les voix et les vidéos sont en place.",
	/* T_RESTORED */ L"Les voix et vidéos de l'installation d'origine sont revenues.",
	/* T_CONVERT_FAILED */ L"Conversion impossible :",
	/* T_NO_PATCH */ L"sans patch",
	/* T_OLD_PATCH */ L"ancien patch",
	/* T_INFO_SETUP */ L"Installateur :",
	/* T_INFO_INSTALLED */ L"Installé :",
	/* T_INFO_CD */ L"CD :",
	/* T_UPDATE */ L"Mettre à jour",
	/* T_UPDATED */ L"Le patch est à jour. \"Traduction\" est maintenant disponible.",
	/* T_INFO_CONTENT */ L"Voix et vidéos :",
	/* T_INFO_EXE */ L"Exécutable :",
	/* T_LANG_EN */ L"anglais",
	/* T_LANG_ES */ L"espagnol",
	/* T_LANG_IT */ L"italien",
	/* T_LANG_PT */ L"portugais",
	/* T_LANG_FR */ L"français",
	/* T_LANG_DE */ L"allemand",
	/* T_LANG_OTHER */ L"autre langue",
	/* T_HELP_LINK */ L"Aide du patch",
	/* T_GAME_HELP_LINK */ L"Aide du jeu",
	/* T_DISCLAIMER */ L"Patch non officiel.\nSans lien avec\nDisney, Pixar,\nActivision ou\nTraveller's Tales.",
};

static const wchar_t *const kGerman[] = {
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Wähle das Abbild der PC-CD des Spiels",
	/* T_PICK_ISO_OR_GAME */ L"Wähle das Abbild der PC-CD des Spiels oder die buzz.exe des installierten Spiels",
	/* T_ISO_OR_GAME_FILTER */ L"CD-Abbild oder installiertes Spiel",
	/* T_NOT_OURS */ L"Dieses Spiel wurde nicht mit diesem Programm installiert. Wähle das CD-Abbild, um es zu installieren.",
	/* T_ISO_FILTER */ L"CD-Abbild",
	/* T_BAD_ISO */ L"Diese Datei ist nicht die PC-CD von Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"Diese Version kann mit dem Patch nicht installiert werden: Ihre buzz.exe unterscheidet sich zu sehr von den anderen. Ihre CD lässt sich bei Übersetzung nutzen, um ihre Stimmen und Videos in eine andere Version zu bringen.",
	/* T_NEEDS_DISC */ L"Diese Version hat einen Kopierschutz (DRM): Das Spiel startet nur mit der Original-CD im Laufwerk (ein CD-Abbild geht nicht). Wir empfehlen, eine andere Version zu installieren und diese CD bei Übersetzung für die französischen Stimmen und Videos zu nutzen (die französischen Texte: Patch-Optionen > Sprache).",
	/* T_PICK_FOLDER */ L"Wähle, wohin installiert wird (ein Ordner für das Spiel wird angelegt)",
	/* T_FOLDER_NOTE */ L"Das ist der Stammordner: Darin wird der Ordner „%s“ mit dem Spiel angelegt.",
	/* T_INSTALL_HERE */ L"Hier installieren",
	/* T_CONFIRM_DIR */ L"Das Spiel wird installiert in:",
	/* T_INSTALLING */ L"Installiere...",
	/* T_INSTALLED */ L"Das Spiel ist installiert.",
	/* T_COPY_FAILED */ L"Konnte nicht schreiben:",
	/* T_TRANSLATION */ L"Übersetzung",
	/* T_ASK_LINKS */ L"Das Spiel ist installiert. Verknüpfungen auf dem Desktop und im Startmenü anlegen?",
	/* T_ASK_VOICES */ L"Möchtest du die Stimmen und Videos von einer anderen CD (Dreamcast, PlayStation oder PC)? Das geht auch später mit setup\\BuzzSetup.exe im Spielordner.",
	/* T_VOICES_NOTE */ L"Die Texte des Spiels werden im Spiel gewählt (Optionen). Hier: die Stimmen und Videos.",
	/* T_FROM_DISC */ L"Stimmen und Videos von einer anderen CD\nDreamcast (.cdi), PlayStation (.bin) oder PC in einer anderen Sprache (.iso)",
	/* T_RESTORE */ L"Zurück zu den Stimmen und Videos der ursprünglichen Installation",
	/* T_PICK_DISC */ L"Wähle das CD-Abbild: Dreamcast, PlayStation oder PC",
	/* T_DISC_FILTER */ L"CD-Abbild",
	/* T_BAD_DISC */ L"Diese Datei ist keine CD von Buzz Lightyear of Star Command (Dreamcast, PlayStation oder PC).",
	/* T_NO_FFMPEG */ L"ffmpeg.exe konnte nicht in den Ordner setup des Spiels geschrieben werden.",
	/* T_CONVERTING */ L"Stimmen und Videos werden umgewandelt...",
	/* T_VOICES_DONE */ L"Die Stimmen und Videos sind eingesetzt.",
	/* T_RESTORED */ L"Die Stimmen und Videos der ursprünglichen Installation sind zurück.",
	/* T_CONVERT_FAILED */ L"Konnte nicht umwandeln:",
	/* T_NO_PATCH */ L"ohne Patch",
	/* T_OLD_PATCH */ L"alter Patch",
	/* T_INFO_SETUP */ L"Installer:",
	/* T_INFO_INSTALLED */ L"Installiert:",
	/* T_INFO_CD */ L"CD:",
	/* T_UPDATE */ L"Aktualisieren",
	/* T_UPDATED */ L"Der Patch ist aktuell. Jetzt ist \"Übersetzung\" verfügbar.",
	/* T_INFO_CONTENT */ L"Stimmen und Videos:",
	/* T_INFO_EXE */ L"Programmdatei:",
	/* T_LANG_EN */ L"Englisch",
	/* T_LANG_ES */ L"Spanisch",
	/* T_LANG_IT */ L"Italienisch",
	/* T_LANG_PT */ L"Portugiesisch",
	/* T_LANG_FR */ L"Französisch",
	/* T_LANG_DE */ L"Deutsch",
	/* T_LANG_OTHER */ L"andere Sprache",
	/* T_HELP_LINK */ L"Patch-Hilfe",
	/* T_GAME_HELP_LINK */ L"Spielhilfe",
	/* T_DISCLAIMER */ L"Inoffizieller Patch.\nKeine Verbindung zu\nDisney, Pixar,\nActivision oder\nTraveller's Tales.",
};

static const wchar_t *const kItalian[] = {
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Scegli l'immagine del CD PC del gioco",
	/* T_PICK_ISO_OR_GAME */ L"Scegli l'immagine del CD PC del gioco, o il buzz.exe del gioco installato",
	/* T_ISO_OR_GAME_FILTER */ L"Immagine CD o gioco installato",
	/* T_NOT_OURS */ L"Questo gioco non è stato installato con questo programma. Scegli l'immagine del CD per installarlo.",
	/* T_ISO_FILTER */ L"Immagine CD",
	/* T_BAD_ISO */ L"Questo file non è il CD PC di Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"Questa versione non si può installare con la patch: il suo buzz.exe è troppo diverso dagli altri. Il suo disco si può usare in Traduzione, per mettere le sue voci e i suoi video in un'altra versione.",
	/* T_NEEDS_DISC */ L"Questa versione ha una protezione anticopia (DRM): il gioco si avvia solo con il disco originale nel lettore (un'immagine disco non va). Consigliamo di installare un'altra versione e usare questo disco in Traduzione per le voci e i video in francese (i testi in francese: opzioni patch > lingua).",
	/* T_PICK_FOLDER */ L"Scegli dove installare (verrà creata una cartella per il gioco)",
	/* T_FOLDER_NOTE */ L"Questa è la cartella radice: al suo interno verrà creata la cartella «%s» con il gioco.",
	/* T_INSTALL_HERE */ L"Installa qui",
	/* T_CONFIRM_DIR */ L"Il gioco verrà installato in:",
	/* T_INSTALLING */ L"Installazione...",
	/* T_INSTALLED */ L"Il gioco è installato.",
	/* T_COPY_FAILED */ L"Impossibile scrivere:",
	/* T_TRANSLATION */ L"Traduzione",
	/* T_ASK_LINKS */ L"Il gioco è installato. Creare i collegamenti sul desktop e nel menu Start?",
	/* T_ASK_VOICES */ L"Vuoi le voci e i video di un altro disco (Dreamcast, PlayStation o PC)? Si può fare anche dopo con setup\\BuzzSetup.exe, nella cartella del gioco.",
	/* T_VOICES_NOTE */ L"I testi del gioco si scelgono nel gioco (opzioni). Qui: le voci e i video.",
	/* T_FROM_DISC */ L"Voci e video da un altro disco\nDreamcast (.cdi), PlayStation (.bin) o PC in un'altra lingua (.iso)",
	/* T_RESTORE */ L"Tornare alle voci e ai video dell'installazione originale",
	/* T_PICK_DISC */ L"Scegli l'immagine del disco: Dreamcast, PlayStation o PC",
	/* T_DISC_FILTER */ L"Immagine disco",
	/* T_BAD_DISC */ L"Questo file non è un disco di Buzz Lightyear of Star Command (Dreamcast, PlayStation o PC).",
	/* T_NO_FFMPEG */ L"Impossibile scrivere ffmpeg.exe nella cartella setup del gioco.",
	/* T_CONVERTING */ L"Conversione di voci e video...",
	/* T_VOICES_DONE */ L"Voci e video sono stati sostituiti.",
	/* T_RESTORED */ L"Voci e video dell'installazione originale ripristinati.",
	/* T_CONVERT_FAILED */ L"Impossibile convertire:",
	/* T_NO_PATCH */ L"senza patch",
	/* T_OLD_PATCH */ L"patch vecchia",
	/* T_INFO_SETUP */ L"Installatore:",
	/* T_INFO_INSTALLED */ L"Installato:",
	/* T_INFO_CD */ L"CD:",
	/* T_UPDATE */ L"Aggiorna",
	/* T_UPDATED */ L"La patch è aggiornata. Ora puoi usare \"Traduzione\".",
	/* T_INFO_CONTENT */ L"Voci e video:",
	/* T_INFO_EXE */ L"Eseguibile:",
	/* T_LANG_EN */ L"inglese",
	/* T_LANG_ES */ L"spagnolo",
	/* T_LANG_IT */ L"italiano",
	/* T_LANG_PT */ L"portoghese",
	/* T_LANG_FR */ L"francese",
	/* T_LANG_DE */ L"tedesco",
	/* T_LANG_OTHER */ L"altra lingua",
	/* T_HELP_LINK */ L"Guida della patch",
	/* T_GAME_HELP_LINK */ L"Guida del gioco",
	/* T_DISCLAIMER */ L"Patch non ufficiale.\nNessun legame con\nDisney, Pixar,\nActivision o\nTraveller's Tales.",
};

static const wchar_t *const kPortuguese[] = {
	/* T_TITLE */ L"Buzz Lightyear of Star Command & Beyond",
	/* T_PICK_ISO */ L"Escolhe a imagem do CD de PC do jogo",
	/* T_PICK_ISO_OR_GAME */ L"Escolhe a imagem do CD de PC do jogo, ou o buzz.exe do jogo instalado",
	/* T_ISO_OR_GAME_FILTER */ L"Imagem de CD ou jogo instalado",
	/* T_NOT_OURS */ L"Esse jogo não foi instalado com este programa. Escolhe a imagem do CD para o instalar.",
	/* T_ISO_FILTER */ L"Imagem de CD",
	/* T_BAD_ISO */ L"Este ficheiro não é o CD de PC de Buzz Lightyear of Star Command.",
	/* T_OTHER_VERSION */ L"Esta versão não pode ser instalada com o patch: o seu buzz.exe é demasiado diferente dos outros. O seu disco serve na Tradução, para pôr as suas vozes e vídeos noutra versão.",
	/* T_NEEDS_DISC */ L"Esta versão tem uma proteção anticópia (DRM): o jogo só arranca com o disco original na unidade (uma imagem de disco não serve). Recomendamos instalar outra versão e usar este disco na Tradução para as vozes e os vídeos em francês (os textos em francês: opções do patch > idioma).",
	/* T_PICK_FOLDER */ L"Escolhe onde instalar (será criada uma pasta para o jogo)",
	/* T_FOLDER_NOTE */ L"Esta é a pasta raiz: dentro será criada a pasta «%s» com o jogo.",
	/* T_INSTALL_HERE */ L"Instalar aqui",
	/* T_CONFIRM_DIR */ L"O jogo será instalado em:",
	/* T_INSTALLING */ L"A instalar...",
	/* T_INSTALLED */ L"O jogo está instalado.",
	/* T_COPY_FAILED */ L"Não foi possível escrever:",
	/* T_TRANSLATION */ L"Tradução",
	/* T_ASK_LINKS */ L"O jogo está instalado. Criar atalhos no ambiente de trabalho e no menu Iniciar?",
	/* T_ASK_VOICES */ L"Queres as vozes e os vídeos de outro disco (Dreamcast, PlayStation ou PC)? Também se pode fazer depois com setup\\BuzzSetup.exe, na pasta do jogo.",
	/* T_VOICES_NOTE */ L"Os textos do jogo escolhem-se dentro do jogo (opções). Aqui: as vozes e os vídeos.",
	/* T_FROM_DISC */ L"Vozes e vídeos de outro disco\nDreamcast (.cdi), PlayStation (.bin) ou PC noutra língua (.iso)",
	/* T_RESTORE */ L"Voltar às vozes e vídeos da instalação original",
	/* T_PICK_DISC */ L"Escolhe a imagem do disco: Dreamcast, PlayStation ou PC",
	/* T_DISC_FILTER */ L"Imagem de disco",
	/* T_BAD_DISC */ L"Este ficheiro não é um disco de Buzz Lightyear of Star Command (Dreamcast, PlayStation ou PC).",
	/* T_NO_FFMPEG */ L"Não foi possível escrever o ffmpeg.exe na pasta setup do jogo.",
	/* T_CONVERTING */ L"A converter vozes e vídeos...",
	/* T_VOICES_DONE */ L"As vozes e os vídeos já estão no lugar.",
	/* T_RESTORED */ L"As vozes e os vídeos da instalação original voltaram.",
	/* T_CONVERT_FAILED */ L"Não foi possível converter:",
	/* T_NO_PATCH */ L"sem patch",
	/* T_OLD_PATCH */ L"patch antigo",
	/* T_INFO_SETUP */ L"Instalador:",
	/* T_INFO_INSTALLED */ L"Instalado:",
	/* T_INFO_CD */ L"CD:",
	/* T_UPDATE */ L"Atualizar",
	/* T_UPDATED */ L"O patch está atualizado. Já podes usar \"Tradução\".",
	/* T_INFO_CONTENT */ L"Vozes e vídeos:",
	/* T_INFO_EXE */ L"Executável:",
	/* T_LANG_EN */ L"inglês",
	/* T_LANG_ES */ L"espanhol",
	/* T_LANG_IT */ L"italiano",
	/* T_LANG_PT */ L"português",
	/* T_LANG_FR */ L"francês",
	/* T_LANG_DE */ L"alemão",
	/* T_LANG_OTHER */ L"outra língua",
	/* T_HELP_LINK */ L"Ajuda do patch",
	/* T_GAME_HELP_LINK */ L"Ajuda do jogo",
	/* T_DISCLAIMER */ L"Patch não oficial.\nSem relação com a\nDisney, Pixar,\nActivision ou\nTraveller's Tales.",
};

static_assert(sizeof(kEnglish) / sizeof(kEnglish[0]) == T_COUNT, "kEnglish: one text per TextId, in order");
static_assert(sizeof(kSpanish) / sizeof(kSpanish[0]) == T_COUNT, "kSpanish: one text per TextId, in order");
static_assert(sizeof(kFrench) / sizeof(kFrench[0]) == T_COUNT, "kFrench: one text per TextId, in order");
static_assert(sizeof(kGerman) / sizeof(kGerman[0]) == T_COUNT, "kGerman: one text per TextId, in order");
static_assert(sizeof(kItalian) / sizeof(kItalian[0]) == T_COUNT, "kItalian: one text per TextId, in order");
static_assert(sizeof(kPortuguese) / sizeof(kPortuguese[0]) == T_COUNT, "kPortuguese: one text per TextId, in order");

// The installer's languages; their index is the one SetLanguage and CurrentLanguage use
static const struct {
	const char *code;
	const wchar_t *ownName;  // written in itself, for choosing it
	TextId name;             // its name in the installer's language
	WORD windows;            // PRIMARYLANGID of a Windows in this language
	const wchar_t *const *texts;
	const char *aps;         // the original installer's .APS file
} kLanguages[] = {
	{"en", L"English", T_LANG_EN, LANG_ENGLISH, kEnglish, "USA.APS"},
	{"es", L"Español", T_LANG_ES, LANG_SPANISH, kSpanish, "SPANISH.APS"},
	{"fr", L"Français", T_LANG_FR, LANG_FRENCH, kFrench, "FRENCH.APS"},
	{"de", L"Deutsch", T_LANG_DE, LANG_GERMAN, kGerman, "GERMAN.APS"},
	{"it", L"Italiano", T_LANG_IT, LANG_ITALIAN, kItalian, "ITALIAN.APS"},
	{"pt", L"Português", T_LANG_PT, LANG_PORTUGUESE, kPortuguese, "BRZPORT.APS"},
};
static_assert(sizeof kLanguages / sizeof kLanguages[0] == INSTALLER_LANGUAGES, "one entry per installer language");

static bool Valid(int lang)
{
	return lang >= 0 && lang < INSTALLER_LANGUAGES;
}

int WindowsLanguage()
{
	WORD windows = PRIMARYLANGID(GetUserDefaultUILanguage());
	for (int l = 0; l < INSTALLER_LANGUAGES; l++) {
		if (kLanguages[l].windows == windows) {
			return l;
		}
	}
	return 0;
}

static int g_lang = -1;

void SetLanguage(int lang)
{
	g_lang = Valid(lang) ? lang : WindowsLanguage();
}

int CurrentLanguage()
{
	if (g_lang < 0) {
		g_lang = WindowsLanguage();
	}
	return g_lang;
}

const char *LanguageCodeOf(int lang)
{
	return Valid(lang) ? kLanguages[lang].code : "en";
}

int LanguageFromCode(const char *code)
{
	for (int l = 0; l < INSTALLER_LANGUAGES; l++) {
		if (strncmp(code, kLanguages[l].code, 2) == 0) {
			return l;
		}
	}
	return -1;
}

const wchar_t *LanguageOwnName(int lang)
{
	return Valid(lang) ? kLanguages[lang].ownName : L"";
}

const wchar_t *LanguageName(int lang)
{
	return Tr(Valid(lang) ? kLanguages[lang].name : T_LANG_OTHER);
}

const wchar_t *Tr(TextId id)
{
	return kLanguages[CurrentLanguage()].texts[id];
}

const char *ApsName()
{
	return kLanguages[CurrentLanguage()].aps;
}

void ApsStrings::Load(const std::vector<unsigned char> &data)
{
	std::string text(data.begin(), data.end());
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos) {
			end = text.size();
		}
		std::string line = text.substr(pos, end - pos);
		pos = end + 1;
		size_t eq = line.find('=');
		if (line.empty() || line[0] == ';' || line[0] == '[' || eq == std::string::npos) {
			continue;
		}
		std::string value = line.substr(eq + 1);
		while (!value.empty() && (value.back() == '\r' || value.back() == ' ')) {
			value.pop_back();
		}
		int n = MultiByteToWideChar(1252, 0, value.c_str(), (int)value.size(), nullptr, 0);
		std::wstring wide(n, L'\0');
		MultiByteToWideChar(1252, 0, value.c_str(), (int)value.size(), &wide[0], n);
		items_.push_back({line.substr(0, eq), wide});
	}
}

std::wstring ApsStrings::Get(const char *key, const wchar_t *fallback) const
{
	for (const auto &it : items_) {
		if (_stricmp(it.first.c_str(), key) == 0) {
			return it.second;
		}
	}
	return fallback;
}
