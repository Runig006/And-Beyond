# …& Beyond

**Unofficial fan patch for the PC game *Buzz Lightyear of Star Command* (2000)** and its installer.

> This is an unofficial, free, fan-made project. It is not affiliated with, endorsed by or connected to Disney,
> Pixar, Activision or Traveller's Tales. All names and trademarks belong to their owners.
> **No game files are included**: you need your own original PC copy of the game.

## What it does

- A new graphics engine (Direct3D 11) inside `DDraw.dll`: the game files on your disc are not modified.
- Steady 60 frames per second, any resolution and widescreen.
- Gamepad and mouse, with every action remappable.
- Two new difficulty levels.
- "Restart level" in the pause menu.
- Game texts in six languages, and 22 options in the game's own menu ("patch options").
- An installer for modern Windows (the original one is 16-bit and does not run on 64-bit Windows), which installs
  from the PC disc image.

The full list, option by option, is in the help page: `dev/help/index.html`.

## How to install

1. Download `BuzzSetup.exe` from the [releases](../../releases).
2. Run it and choose the disc image (`.iso`, `.bin` / `.cue`) of your PC copy of the game.
3. Install. To update later, run the new `BuzzSetup.exe` and choose your installed game.

Requirements: Windows 10 or 11 and a graphics card with Direct3D 11.

## Known limitations

- The engine works with the English, Spanish, Italian, German and French PC versions. The Portuguese one cannot be
  installed (its `buzz.exe` is a different build), but its disc works in the installer's "Translation" button, for
  its voices and videos.
- The French version has a copy protection (DRM): the game only starts with the original disc in the drive (a disc
  image does not work). The patch does not remove it. We recommend installing another version and using the French
  disc in "Translation" for its voices and videos (the French texts: patch options > language).

Problems and suggestions: [issues](../../issues).

## What is here

| Folder | What it is |
|---|---|
| `dev/patch_src/` | The engine (`DDraw.dll`, C++). `build.bat`. |
| `dev/installer_src/` | The installer (`BuzzSetup.exe`, C++). `build.bat` (build the engine first). |
| `dev/help/` | The patch's help page, which the installer carries inside (one file per language in `lang/`). |
| `dev/lang/` | The game's texts translated into each language, which the installer carries inside. |

Building:

1. The engine: `dev/patch_src/build.bat`. Needs the Visual Studio 2019 Build Tools (another Visual Studio: set
   `VCVARS` to its `vcvars32.bat`).
2. ffmpeg, which the installer carries for its "Translation" button: `dev/installer_src/ffmpeg.sh` in the
   [MSYS2](https://www.msys2.org) MINGW64 shell. It downloads the official source, checks it and builds a small LGPL
   `ffmpeg.exe` with only what the installer uses.
3. The installer: `dev/installer_src/build.bat`.

## License

MIT (see `LICENSE`, which also lists what it does not cover). The installer carries ffmpeg, under the LGPL 2.1:
its unchanged source is attached to each release.
