// Installing the game: files from the CD image, the engine and, if the player wants, shortcuts (no registry).
#pragma once
#include <string>

#include "iso.h"

struct GameVersion {
	uint32_t stamp;       // TimeDateStamp of buzz.exe's PE header
	const char *name;
	bool motor;           // the engine has this version's addresses
	bool needsDisc;       // its buzz.exe has a copy protection: the game asks for the original disc in the drive
	const char *eula;     // SETUP\LEGAL\<this> -> EULA.txt
};

// The version on the CD (from SETUP\BUZZLY\HD\BUZZ.EXE); null if it is not the game. *known = false for a build
// of buzz.exe that is not in the table.
const GameVersion *IdentifyVersion(IsoImage &iso, bool *known);

// Language of an executable (PC, Dreamcast or PlayStation) by its texts: "en", "es", "it", "pt", "fr", "de"; "?" if
// none of the words is there
const char *TextLanguage(const std::vector<uint8_t> &exe);
const char *ExeLanguage(const std::wstring &exePath);  // an installed buzz.exe
bool ExeHasEngine(const std::wstring &exePath);  // the engine has this buzz.exe's addresses

std::wstring DefaultInstallDir();
std::wstring InstalledDir();  // "" when the game is not installed

// Menu art, sounds and texts of the original installer: from the CD, or from setup\ of the installed game
bool LoadSetupFile(IsoImage *iso, const std::wstring &installDir, const char *path, std::vector<uint8_t> &out);

typedef void (*ProgressFn)(int done, int total, void *ctx);
// Patch version of a DDraw.dll, from its version resource: major, minor, build day, build minute, 16 bits each
// (the build tells two builds of the same version apart); 0 = none there
uint64_t EngineVersion(const std::wstring &dll);
uint64_t CarriedEngineVersion();  // the DDraw.dll inside this program
// The installed patch: its DDraw.dll's version, or for a version without engine setup\\patch.txt; 0 = none
uint64_t InstalledPatchVersion(const std::wstring &dir);
std::wstring VersionText(uint64_t version);  // "1.0"
bool SameVersion(uint64_t a, uint64_t b);    // same major and minor (maybe another build)
// An installed game gets this program's engine (if its buzz.exe takes it) and texts, and this program and ffmpeg
// in its setup folder
bool UpdateEngine(const std::wstring &dir, std::wstring &failed);
// The ffmpeg this program carries (with its licence and README) into a folder
bool WriteFfmpeg(const std::wstring &setupDir);

// Copies the game and ours; no shortcuts (CreateShortcuts, when the player wants them)
bool Install(IsoImage &iso, const std::wstring &dir, const GameVersion &version, ProgressFn progress, void *ctx,
	std::wstring &failed);
// The game and the installer, on the desktop and / or in the Start menu (there also the two helps)
void CreateShortcuts(const std::wstring &dir, bool desktop, bool startMenu);
