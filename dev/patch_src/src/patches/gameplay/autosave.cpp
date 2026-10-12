// Autosave and autoload (PATCH OPTIONS > autosave). The slot in use is the last one loaded or saved in LoadSaveMenu
// (0049dae0); after a level AskSaveGame (0043dae0) saves straight to it, and GameMain's title reset (00431f52) loads
// it again. The keyboard map goes to buzz2k.cfg instead of buzz99.sav (SaveSlot 0049d8f0, OneInit 00490d84).
// Each autosave blinks a floppy disk icon for a moment.
#include "common.h"
#include "config.h"
#include "render.h"
#include <stdio.h>

#define g_gameOptions ((unsigned char *)A(0x005358a8))
static const int kSaveBytes = 0x184;

typedef void(__cdecl *ResetRecordsFn)(unsigned char *options);
typedef void(__cdecl *SaveSlotFn)(int slot, const char *name);
static const ResetRecordsFn ResetRecords = (ResetRecordsFn)A(0x00433fc0);
static const SaveSlotFn SaveSlot = (SaveSlotFn)A(0x0049d8f0);

#define g_keyBlock ((unsigned char *)A(0x0052b4e0))
static void(__cdecl *g_saveOriginal)(int slot, const char *name);
static int(__cdecl *g_askOriginal)();
static int(__cdecl *g_loadSaveOriginal)(int save);

// Bytes of the progress in a slot file (u32 name length, name, 0x184 bytes); false if missing or short
static bool ReadSlot(int slot, unsigned char *out, FILETIME *written)
{
	char path[16];
	wsprintfA(path, "buzz%02d.sav", slot);
	if (written) {
		WIN32_FILE_ATTRIBUTE_DATA a;
		if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a)) {
			return false;
		}
		*written = a.ftLastWriteTime;
	}
	FILE *f = fopen(path, "rb");
	if (!f) {
		return false;
	}
	unsigned len = 0;
	bool ok = fread(&len, 4, 1, f) == 1 && len < 256 && fseek(f, (long)len, SEEK_CUR) == 0 &&
	          fread(out, 1, kSaveBytes, f) == (size_t)kSaveBytes;
	fclose(f);
	return ok;
}

// SaveSlot: slot 99 (the keys) goes to buzz2k.cfg
static void __cdecl SaveSlot_Keys(int slot, const char *name)
{
	if (slot != 99) {
		g_saveOriginal(slot, name);
		return;
	}
	memcpy(g_config.keys, g_keyBlock, Config::kKeyBytes);
	g_config.haveKeys = true;
	ConfigSave();
}

// OneInit, after reading buzz99.sav (if any): the keys from buzz2k.cfg. The first time, what OneInit read (an old
// buzz99.sav, or the game's defaults) moves there, with our default keys if they were the game's untouched ones.
static void __cdecl StartupKeys(unsigned char *options)
{
	if (g_config.haveKeys) {
		memcpy(g_keyBlock, g_config.keys, Config::kKeyBytes);
	} else {
		bool old = GetFileAttributesA("buzz99.sav") != INVALID_FILE_ATTRIBUTES;
		ApplyOurDefaultKeys();
		memcpy(g_config.keys, g_keyBlock, Config::kKeyBytes);
		g_config.haveKeys = true;
		// The old file stays as buzz99.sav.backup: renamed back, the game without the patch has its keys again
		if (ConfigSave() && old) {
			MoveFileExA("buzz99.sav", "buzz99.sav.backup", MOVEFILE_REPLACE_EXISTING);
			LogLine("keys: buzz99.sav moved into buzz2k.cfg (old file kept as buzz99.sav.backup)");
		}
	}
	ResetRecords(options);
}

// LoadSaveMenu(save): after it, the slot that matches the progress in memory is the one in use
static int __cdecl LoadSaveMenu_Track(int save)
{
	int result = g_loadSaveOriginal(save);
	unsigned char data[kSaveBytes];
	int best = 0;
	FILETIME bestTime = {}, t;
	for (int slot = 1; slot <= 8; slot++) {
		if (ReadSlot(slot, data, &t) && !memcmp(data, g_gameOptions, kSaveBytes) &&
		    (!best || CompareFileTime(&t, &bestTime) > 0)) {
			best = slot;
			bestTime = t;
		}
	}
	if (best && best != g_config.saveSlot) {
		g_config.saveSlot = best;
		ConfigSave();
		LogLine("autosave: slot %d", best);
	}
	return result;
}

static DWORD g_savedAt; // GetTickCount of the last autosave (0: none to show)
static const DWORD kSaveIconMs = 2400;

// The floppy disk, inside the centred 4:3 area (wide screen bars are painted black in menus)
static void DrawSaveIcon()
{
	if (!g_savedAt) {
		return;
	}
	DWORD t = GetTickCount() - g_savedAt;
	if (t >= kSaveIconMs) {
		g_savedAt = 0;
		return;
	}
	if (t / 300 % 2) {
		return; // blink
	}
	int h = g_render.Height(), w43 = h * 4 / 3, s = h / 16;
	int x = (g_render.Width() + w43) / 2 - s - h / 24, y = h - s - h / 24;
	g_render.FillRect(x, y, x + s, y + s, 0xff1c2c64);                                         // body
	g_render.FillRect(x + s / 4, y, x + s * 3 / 4, y + s * 3 / 8, 0xffc8c8d0);                   // metal shutter
	g_render.FillRect(x + s * 9 / 16, y + s / 16, x + s * 11 / 16, y + s * 5 / 16, 0xff1c2c64);  // its window
	g_render.FillRect(x + s / 8, y + s / 2, x + s * 7 / 8, y + s - s / 16, 0xfff0f0f0);          // label
}

// AskSaveGame (only called by AfterLevelSaveMenu): with a slot in use, save there without asking
static int __cdecl AskSaveGame_Auto()
{
	if (!g_config.autoSave || !g_config.saveSlot) {
		return g_askOriginal();
	}
	char name[16];
	wsprintfA(name, "savegame %d", g_config.saveSlot); // the name LoadSaveMenu gives
	SaveSlot(g_config.saveSlot, name);
	g_savedAt = GetTickCount() | 1;
	return 0; // no slot screen
}

// GameMain, back at the title: the game's reset, then the slot in use
static void __cdecl TitleReset(unsigned char *options)
{
	ResetRecords(options);
	g_gameOptions[0] = 0xc0; // what GameMain does next (that instruction is ours now)
	unsigned char data[kSaveBytes];
	if (g_config.autoSave && g_config.saveSlot && ReadSlot(g_config.saveSlot, data, nullptr)) {
		memcpy(g_gameOptions, data, kSaveBytes);
	}
}

extern "C" void InstallAutosave(void)
{
	g_render.AddBeforePresent(DrawSaveIcon);
	static const unsigned char saveBytes[] = {0x81, 0xec, 0x08, 0x01, 0x00, 0x00}; // sub esp, 0x108
	g_saveOriginal = (void(__cdecl *)(int, const char *))HookTrampoline(A(0x0049d8f0), (const void *)SaveSlot_Keys,
	                                                                     saveBytes, 6, "keys to buzz2k.cfg (SaveSlot)");
	static const unsigned char initCall[] = {0xe8, 0x37, 0x32, 0xfa, 0xff}; // OneInit: call ResetRecords
	unsigned char keysCall[5] = {0xe8};
	*(DWORD *)(keysCall + 1) = (DWORD)StartupKeys - (A(0x00490d84) + 5);
	if (g_saveOriginal) {
		PatchBytes(A(0x00490d84), initCall, keysCall, 5, "keys from buzz2k.cfg (OneInit)");
	}
	static const unsigned char askBytes[] = {0x53, 0x56, 0x57, 0x6a, 0x0c};             // push ebx/esi/edi; push 0xc
	static const unsigned char menuBytes[] = {0x81, 0xec, 0x28, 0x07, 0x00, 0x00};      // sub esp, 0x728
	g_askOriginal = (int(__cdecl *)())HookTrampoline(A(0x0043dae0), (const void *)AskSaveGame_Auto, askBytes, 5,
	                                                  "autosave (AskSaveGame)");
	g_loadSaveOriginal = (int(__cdecl *)(int))HookTrampoline(A(0x0049dae0), (const void *)LoadSaveMenu_Track, menuBytes,
	                                                         6, "autosave slot (LoadSaveMenu)");
	static const unsigned char callBytes[] = {0xe8, 0x69, 0x20, 0x00, 0x00}; // GameMain: call ResetRecords
	unsigned char call[5] = {0xe8};
	*(DWORD *)(call + 1) = (DWORD)TitleReset - (A(0x00431f52) + 5);
	if (!PatchBytes(A(0x00431f52), callBytes, call, 5, "autoload (GameMain title reset)")) {
		return;
	}
	static const unsigned char optionsBytes[] = {0xc6, 0x05, 0xa8, 0x58, 0x53, 0x00, 0xc0}; // mov [g_gameOptions], 0xc0
	static const unsigned char nops[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
	PatchBytes(A(0x00431f5a), optionsBytes, nops, 7, "autoload (GameMain options reset)");
}
