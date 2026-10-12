// Help page texts: English. Loaded by index.html after help.js.
HELP.en = {
  lang_name: "English",
  tab: "Room 1",
  room: "Waiting room",
  patch: "Patch",
  eyebrow: "Medical record · walk-ins welcome",
  beyond: "& Beyond",
  urgent: "Retro ER",
  tagline: "Your game from 2000, running on Windows 11",
  h_reason: "History: reason for the visit",
  reason_intro: "The patient (Disney/Pixar, Traveller's Tales, 2000) arrives at the ER with these symptoms:",
  symptoms: [
    "<b>Won't install:</b> its installer is 16-bit and 64-bit Windows won't run it.",
    "<b>DirectX 6 graphics:</b> on today's computers it fails or looks wrong.",
    "<b>Stutters:</b> the frame rate jumped between 60 and 30.",
    "<b>Small screen:</b> nothing above 1024×768 and no idea of widescreens.",
    "<b>No modern controllers</b> and a camera you couldn't move.",
    "<b>Few languages:</b> on PC it came out in English, Italian and Portuguese (and a few more)."
  ],
  h_always: "Diagnosis and standard treatment",
  always_intro: "These come always, nothing to switch on:",
  always: [
    "<b>A new graphics engine</b> (Direct3D 11) inside <code>DDraw.dll</code>: the game from your disc isn't modified.",
    "<b>Steady 60 frames per second.</b>",
    "<b>Any resolution</b> and widescreens: the HUD sits at the edges; menus and videos in 4:3 with bars.",
    "<b>Portable:</b> no Windows registry; the game folder can be moved or copied.",
    "<b>Controller and mouse:</b> a new controls screen where each action has a key, a mouse button and a controller button. By default <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> to move and <kbd>Q</kbd> <kbd>E</kbd> to strafe; everything can be changed.",
    "<b>Two new difficulties:</b> <i>commander</i> (hits hurt 1.7 times more) and <i>beyond</i> (2.5 times more and half as many enemies again).",
    "<b>In the pause menu:</b> “control options” and “restart level”.",
    "<b>Six languages</b> for the game's texts."
  ],
  h_options: "Dosage: patch options",
  options_intro: "The game's options menu has “patch options”. Each option, what it does and why it's there:",
  legend: "The values as the game shows them; the marked one is the patch's default.",
  why: "Why:",
  h_installer: "Directions for use: the installer",
  installer: [
    "<b>Install:</b> asks for the PC game's ISO and copies everything; the CD isn't needed to play.",
    "<b>Translation:</b> puts in the voices and videos of another disc (Dreamcast, PlayStation or the PC version in another language). The originals are kept to go back to them.",
    "<b>Update:</b> when a new version comes out, open the new installer and choose your game's <code>buzz.exe</code>. Your saves are left alone.",
    "<b>Uninstall:</b> delete the game folder (and its shortcuts, if you created them). Nothing else is installed into Windows."
  ],
  h_patient: "Patient record",
  fields: [
    {
      label: "Patient",
      value: "Buzz Lightyear of Star Command"
    },
    {
      label: "Born",
      value: "2000 · Traveller's Tales"
    },
    {
      label: "Age",
      value: "{age} years"
    },
    {
      label: "Home system",
      value: "Windows 95/98 · DirectX 6"
    },
    {
      label: "Allergies",
      value: "64-bit Windows, widescreens"
    },
    {
      label: "Doctor",
      value: "Dr. Runig"
    }
  ],
  stamp: "Stable",
  stamp_by: "Dr. Runig",
  vitals: [
    {
      label: "Frames per second",
      value: "60",
      note: "steady"
    },
    {
      label: "Resolution",
      value: "yours",
      note: "widescreen"
    },
    {
      label: "Languages",
      value: "6",
      note: "texts"
    },
    {
      label: "Options",
      value: "22",
      note: "your way"
    }
  ],
  h_leaflet: "Leaflet: side effects",
  leaflet: [
    "<b>Like any treatment, it may fail now and then.</b> The game is from 2000 and every computer is different.",
    "<b>If something goes wrong:</b> turn off the related option in “patch options”; if it doesn't help, open the installer again and press Update (or install over it). Your saves aren't lost.",
    "<b>Contraindications:</b> PC version only. The engine works with the English, Spanish, Italian, German and French versions; the Portuguese one cannot be installed (its buzz.exe is too different), but its disc works in Translation for the voices and videos. The French one has a copy protection (DRM): it only starts with the original disc in the drive (an image does not work). We recommend installing another version and using the French disc in Translation for the voices and videos."
  ],
  h_small: "Small print",
  small: [
    "Unofficial patch, free and made by fans. Not affiliated with Disney, Pixar, Activision or Traveller's Tales.",
    "You need your own copy of the PC game: the patch brings nothing from the game.",
    "The Translation button uses ffmpeg (LGPL licence), which comes inside the installer: its licence and where to get its source code are in the game's <code>setup</code> folder."
  ],
  footer: "Discharged. Next, please.",
  options: [
    {
      name: "language",
      values: ["english", "espanol", "francais", "deutsch", "italiano", "portugues"],
      what: "Language of the game's and the patch's texts. Changes at once.",
      why: "The PC versions came out in few languages."
    },
    {
      name: "display",
      values: ["fullscreen", "window"],
      default: "fullscreen",
      what: "Fullscreen (borderless) or window. <kbd>Alt</kbd>+<kbd>Enter</kbd> switches too.",
      why: "The original changed the resolution of the whole desktop."
    },
    {
      name: "resolution",
      values: ["1920x1080", "..."],
      what: "The game's resolution. Applies the next time you start it.",
      why: "The original didn't go above 1024×768."
    },
    {
      name: "controls",
      values: ["modern", "directional", "rotational"],
      default: "modern",
      what: "<i>Modern</i>: the camera moves with the mouse or the right stick and Buzz fires where you look. <i>Directional</i> and <i>rotational</i> are the original's two schemes.",
      why: "Play it like a game of today without losing the old controls."
    },
    {
      name: "auto aim",
      values: ["off", "on"],
      default: "on",
      what: "With modern controls, shots bend a little towards the enemy closest to the reticle.",
      why: "With a free camera, small enemies are hard to hit."
    },
    {
      name: "camera speed",
      values: ["slow", "normal", "fast"],
      default: "normal",
      what: "How fast the right stick turns the camera.",
      why: "Every controller and every hand is different."
    },
    {
      name: "invert look",
      values: ["off", "on"],
      default: "off",
      what: "Pushing the stick or the mouse up looks down.",
      why: "Some people have always played that way."
    },
    {
      name: "weapon info",
      values: ["off", "on"],
      default: "on",
      what: "Weapon level and maximum ammo, on the HUD.",
      why: "The game never said how much a weapon improves or how far it can be loaded."
    },
    {
      name: "autosave",
      values: ["off", "on"],
      default: "on",
      what: "Saves after each level to the slot in use and loads it back at the title. A floppy disk blinks while saving.",
      why: "In the original, dying sent you back to the menu and lost whatever you hadn't saved by hand."
    },
    {
      name: "goal info",
      values: ["off", "on"],
      default: "on",
      what: "Credits the medal asks for and the time to beat, on the HUD.",
      why: "The game didn't show them while playing."
    },
    {
      name: "draw distance",
      values: ["original", "far"],
      default: "far",
      what: "Scenery and enemies are drawn twice as far.",
      why: "In 2000 things had to be cut; today's computers have plenty to spare."
    },
    {
      name: "planet fog",
      values: ["original", "activated"],
      default: "activated",
      what: "Coloured fog in Canis Lunis 1, Ice World, Bathyos and Planet Z.",
      why: "Atmosphere: the original PC game had fog programmed, but it never showed."
    },
    {
      name: "see through",
      values: ["off", "on"],
      default: "on",
      what: "What stands between the camera and Buzz becomes see-through.",
      why: "Walls and columns hid Buzz."
    },
    {
      name: "boss scene",
      values: ["original", "always"],
      default: "always",
      what: "If you win the race against the boss, you still see the boss's scene before the fight.",
      why: "Winning the race made you miss the boss's entrance."
    },
    {
      name: "enemy level hp",
      values: ["original", "off"],
      default: "off",
      what: "<i>Original</i>: enemies have more health in later levels. <i>Off</i>: always the health of the first levels.",
      why: "For those who want a lighter game."
    },
    {
      name: "enemy iframes",
      values: ["original", "off"],
      default: "off",
      what: "<i>Original</i>: after a hit, an enemy can't die for a moment. <i>Off</i>: every shot counts.",
      why: "With fast weapons, many shots did nothing."
    },
    {
      name: "price blink",
      values: ["off", "on"],
      default: "on",
      what: "Weapon prices blink red while you can't afford them.",
      why: "On PC prices were always white; other versions warned you."
    },
    {
      name: "fair mines",
      values: ["off", "on"],
      default: "on",
      what: "Enemy mines only go off when Buzz is really close.",
      why: "Their reach was bigger than their drawing: they blew up from afar."
    },
    {
      name: "turret wreck",
      values: ["original", "harmless"],
      default: "harmless",
      what: "The wreck of a destroyed turret pushes Buzz without hurting him.",
      why: "A destroyed turret kept doing damage."
    },
    {
      name: "vehicle aim",
      values: ["original", "camera"],
      default: "camera",
      what: "With modern controls, vehicles fire where the camera looks.",
      why: "Aim with the camera, as on foot."
    },
    {
      name: "vehicle camera",
      values: ["free", "steer"],
      default: "free",
      what: "<i>Free</i>: the mouse or the stick moves the camera. <i>Steer</i>: they turn the bike and the board.",
      why: "Everyone prefers something different."
    },
    {
      name: "bathyos camera",
      values: ["original", "high"],
      default: "high",
      what: "In Bathyos the camera goes higher and aims above Buzz.",
      why: "The submarine hid what was ahead."
    }
  ]
};
