// Help page texts: Spanish. Loaded by index.html after help.js.
HELP.es = {
  lang_name: "Español",
  tab: "Consulta 1",
  room: "Sala de espera",
  patch: "Parche",
  eyebrow: "Historia clínica · sin cita previa",
  beyond: "& Beyond",
  urgent: "Urgencias retro",
  tagline: "Tu juego del 2000, funcionando en Windows 11",
  h_reason: "Anamnesis: motivo de la consulta",
  reason_intro: "El paciente (Disney/Pixar, Traveller's Tales, 2000) llega a urgencias con estos síntomas:",
  symptoms: [
    "<b>No se instala:</b> su instalador es de 16 bits y Windows de 64 bits no lo abre.",
    "<b>Gráficos de DirectX 6:</b> en los equipos de hoy falla o se ve mal.",
    "<b>Va a saltos:</b> la velocidad bailaba entre 60 y 30 imágenes por segundo.",
    "<b>Pantalla pequeña:</b> no pasaba de 1024×768 ni sabía de pantallas panorámicas.",
    "<b>Sin mandos modernos</b> y con una cámara que no se podía mover.",
    "<b>En pocos idiomas:</b> en PC salió en inglés, italiano y portugués (y algunos más)."
  ],
  h_always: "Diagnóstico y tratamiento de serie",
  always_intro: "Esto viene siempre, sin tener que activar nada:",
  always: [
    "<b>Motor gráfico nuevo</b> (Direct3D 11) dentro de <code>DDraw.dll</code>: el juego de tu disco no se modifica.",
    "<b>60 imágenes por segundo</b> estables.",
    "<b>Cualquier resolución</b> y pantallas panorámicas: el marcador va pegado a los bordes; menús y vídeos en 4:3 con bandas.",
    "<b>Portátil:</b> no usa el registro de Windows; la carpeta del juego se puede mover o copiar.",
    "<b>Mando y ratón:</b> pantalla de controles nueva, donde cada acción tiene tecla, botón de ratón y botón de mando. Por defecto <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> para moverse y <kbd>Q</kbd> <kbd>E</kbd> para el paso lateral; todo se puede cambiar.",
    "<b>Dos dificultades nuevas:</b> <i>comandante</i> (los golpes quitan 1,7 veces más) y <i>más allá</i> (2,5 veces más y la mitad más de enemigos).",
    "<b>En la pausa:</b> «opciones de control» y «reiniciar nivel».",
    "<b>Seis idiomas</b> para los textos del juego."
  ],
  h_options: "Posología: opciones del parche",
  options_intro: "En el menú de opciones del juego está «opciones parche». Cada opción, qué hace y por qué está:",
  legend: "Los valores que salen en el juego; el marcado es el de fábrica del parche.",
  why: "Por qué:",
  h_installer: "Instrucciones de uso: el instalador",
  installer: [
    "<b>Instalar:</b> pide la ISO del juego de PC y copia todo; el CD ya no hace falta para jugar.",
    "<b>Traducción:</b> pone las voces y los vídeos de otro disco (Dreamcast, PlayStation o el PC de otro idioma). Guarda los originales para poder volver a ellos.",
    "<b>Actualizar:</b> cuando salga una versión nueva, abre el instalador nuevo y elige el <code>buzz.exe</code> de tu juego. Tus partidas no se tocan.",
    "<b>Desinstalar:</b> borra la carpeta del juego (y sus accesos directos, si los creaste). No se instala nada más en Windows."
  ],
  h_patient: "Ficha del paciente",
  fields: [
    {
      label: "Paciente",
      value: "Buzz Lightyear of Star Command"
    },
    {
      label: "Nacimiento",
      value: "2000 · Traveller's Tales"
    },
    {
      label: "Edad",
      value: "{age} años"
    },
    {
      label: "Sistema de origen",
      value: "Windows 95/98 · DirectX 6"
    },
    {
      label: "Alergias",
      value: "Windows de 64 bits, pantallas panorámicas"
    },
    {
      label: "Médico",
      value: "Dr. Runig"
    }
  ],
  stamp: "Estable",
  stamp_by: "Dr. Runig",
  vitals: [
    {
      label: "Imágenes por segundo",
      value: "60",
      note: "estables"
    },
    {
      label: "Resolución",
      value: "la tuya",
      note: "panorámica"
    },
    {
      label: "Idiomas",
      value: "6",
      note: "textos"
    },
    {
      label: "Opciones",
      value: "22",
      note: "a tu gusto"
    }
  ],
  h_leaflet: "Prospecto: efectos secundarios",
  leaflet: [
    "<b>Como cualquier tratamiento, puede fallar alguna vez.</b> El juego es del año 2000 y cada ordenador es un mundo.",
    "<b>Si algo va mal:</b> desactiva en «opciones parche» la opción relacionada; si sigue igual, vuelve a abrir el instalador y pulsa Actualizar (o instala encima). Tus partidas no se pierden.",
    "<b>Contraindicaciones:</b> solo para la versión de PC. El motor funciona con la versión inglesa, española, italiana, alemana y francesa; la portuguesa no se puede instalar (su buzz.exe es demasiado distinto), pero su disco sirve en Traducción para las voces y los vídeos. La francesa tiene una protección anticopia (DRM): solo arranca con el disco original en la unidad (una imagen no vale). Recomendamos instalar otra versión y usar el disco francés en Traducción para las voces y los vídeos."
  ],
  h_small: "Letra pequeña",
  small: [
    "Parche no oficial, gratuito y hecho por aficionados. Sin relación con Disney, Pixar, Activision ni Traveller's Tales.",
    "Hace falta tu propia copia del juego de PC: el parche no trae nada del juego.",
    "El botón Traducción usa ffmpeg (licencia LGPL), que va dentro del instalador: su licencia y dónde está su código fuente quedan en la carpeta <code>setup</code> del juego."
  ],
  footer: "Alta médica. Pase el siguiente.",
  options: [
    {
      name: "idioma",
      values: ["english", "espanol", "francais", "deutsch", "italiano", "portugues"],
      what: "Idioma de los textos del juego y del parche. Se cambia al momento.",
      why: "Las versiones de PC salieron en pocos idiomas."
    },
    {
      name: "pantalla",
      values: ["completa", "ventana"],
      default: "completa",
      what: "Pantalla completa (sin bordes) o ventana. <kbd>Alt</kbd>+<kbd>Intro</kbd> también cambia.",
      why: "El original cambiaba la resolución de todo el escritorio."
    },
    {
      name: "resolucion",
      values: ["1920x1080", "..."],
      what: "Resolución del juego. Se aplica la próxima vez que lo arranques.",
      why: "El original no pasaba de 1024×768."
    },
    {
      name: "control",
      values: ["moderno", "direccional", "rotativo"],
      default: "moderno",
      what: "<i>Moderno</i>: la cámara se mueve con el ratón o el stick derecho y Buzz dispara hacia donde miras. <i>Direccional</i> y <i>rotativo</i> son los dos esquemas del original.",
      why: "Jugar como un juego de hoy, sin perder los controles de siempre."
    },
    {
      name: "autoapuntado",
      values: ["no", "si"],
      default: "si",
      what: "Con el control moderno, los disparos se tuercen un poco hacia el enemigo más cercano a la mira.",
      why: "Con la cámara libre cuesta acertar a los enemigos pequeños."
    },
    {
      name: "velocidad camara",
      values: ["lenta", "normal", "rapida"],
      default: "normal",
      what: "Lo rápido que gira la cámara con el stick derecho.",
      why: "Cada mando y cada mano es distinto."
    },
    {
      name: "invertir vista",
      values: ["no", "si"],
      default: "no",
      what: "Al subir el stick o el ratón, la cámara mira hacia abajo.",
      why: "Hay quien juega así desde siempre."
    },
    {
      name: "info armas",
      values: ["no", "si"],
      default: "si",
      what: "Nivel del arma y munición máxima, en el marcador.",
      why: "El juego no decía cuánto mejora el arma ni hasta dónde se puede cargar."
    },
    {
      name: "autoguardado",
      values: ["no", "si"],
      default: "si",
      what: "Guarda al acabar cada nivel en la ranura en uso y la carga al volver al título. Un disquete parpadea al guardar.",
      why: "En el original, si morías volvías al menú y perdías lo que no habías guardado a mano."
    },
    {
      name: "info objetivos",
      values: ["no", "si"],
      default: "si",
      what: "Créditos que pide la medalla y tiempo a batir, en el marcador.",
      why: "El juego no los enseñaba mientras jugabas."
    },
    {
      name: "distancia visual",
      values: ["original", "lejos"],
      default: "lejos",
      what: "El escenario y los enemigos se dibujan el doble de lejos.",
      why: "En el año 2000 había que recortar; los equipos de hoy van sobrados."
    },
    {
      name: "niebla planetas",
      values: ["original", "activada"],
      default: "activada",
      what: "Niebla de color en Canis Lunis 1, Ice World, Bathyos y Planet Z.",
      why: "Ambiente: en el PC original la niebla estaba programada, pero no llegaba a verse."
    },
    {
      name: "transparencias",
      values: ["no", "si"],
      default: "si",
      what: "Lo que queda entre la cámara y Buzz se vuelve transparente.",
      why: "Las paredes y columnas tapaban a Buzz."
    },
    {
      name: "escena jefe",
      values: ["original", "siempre"],
      default: "siempre",
      what: "Si ganas la carrera al jefe, se ve igualmente su escena antes del combate.",
      why: "Ganando la carrera te perdías su presentación."
    },
    {
      name: "vida enemigos",
      values: ["original", "no"],
      default: "no",
      what: "<i>Original</i>: los enemigos tienen más vida en los niveles avanzados. <i>No</i>: siempre la de los primeros niveles.",
      why: "Para quien quiera un juego más ligero."
    },
    {
      name: "inmunidad enemiga",
      values: ["original", "no"],
      default: "no",
      what: "<i>Original</i>: después de un golpe, el enemigo no puede morir durante un momento. <i>No</i>: cada disparo cuenta.",
      why: "Con armas rápidas, muchos disparos no hacían nada."
    },
    {
      name: "aviso precios",
      values: ["no", "si"],
      default: "si",
      what: "Los precios de las armas parpadean en rojo mientras no te llega el dinero.",
      why: "En PC los precios eran siempre blancos; en otras versiones avisaban."
    },
    {
      name: "minas justas",
      values: ["no", "si"],
      default: "si",
      what: "Las minas enemigas solo estallan cuando Buzz está cerca de verdad.",
      why: "Su alcance era mayor que su dibujo: explotaban desde lejos."
    },
    {
      name: "restos torretas",
      values: ["original", "inofensivos"],
      default: "inofensivos",
      what: "Los restos de una torreta destruida empujan a Buzz, pero no le quitan vida.",
      why: "Una torreta destruida seguía haciendo daño."
    },
    {
      name: "punteria vehiculo",
      values: ["original", "camara"],
      default: "camara",
      what: "Con el control moderno, los vehículos disparan hacia donde mira la cámara.",
      why: "Apuntar con la cámara, como a pie."
    },
    {
      name: "camara vehiculo",
      values: ["libre", "dirigir"],
      default: "libre",
      what: "<i>Libre</i>: el ratón o el stick mueven la cámara. <i>Dirigir</i>: giran la moto y la tabla.",
      why: "Cada uno prefiere una cosa."
    },
    {
      name: "camara bathyos",
      values: ["original", "alta"],
      default: "alta",
      what: "En Bathyos la cámara va más alta y apunta por encima de Buzz.",
      why: "El submarino tapaba lo que había delante."
    }
  ]
};
