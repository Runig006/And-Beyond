// Install, update and play (see setup.h). The CD layout: SETUP\BUZZLY\HD = the game folder (buzz.exe, data\),
// SETUP\BUZZLY\CD = cd\ (music, voices and videos; the original read them from the CD).
#include "setup.h"

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <algorithm>
#include <cstring>

#include "hlp.h"
#include "resource.h"
#include "texts.h"
#include "voices.h"

// The PC versions, by the date in buzz.exe's PE header; the engine has the addresses of those with motor (its
// core\game_maps.c)
static const GameVersion kVersions[] = {
	{0x3a59fed5, "en", true, false, "EULAUK.TXT"},
	{0x3a5b0bb4, "es", true, false, "EULAES.TXT"},
	{0x3a59f61a, "it", true, false, "EULAIT.TXT"},
	{0x3a644183, "de", true, false, "EULADE.TXT"},
	{0x3a66d4e4, "fr", true, true, "EULAFR.TXT"},
	{0x3bd430f8, "pt", false, false, "EULAPOR.TXT"},  // another build: not installed, its disc only for Translation
};
static GameVersion g_other = {0, "?", false, false, "EULAUK.TXT"};

// A few words of the pause and save menus in each language; the one with most of them wins. Checked with the PC
// English, Italian and Portuguese buzz.exe and the Spanish Dreamcast and PlayStation ones (the Italian and
// Portuguese keep some English leftovers). French and German: words of those languages, no disc to check them.
static bool ReadWhole(const std::wstring &path, std::vector<uint8_t> &out);

const char *TextLanguage(const std::vector<uint8_t> &exe)
{
	static const struct {
		const char *code;
		const char *words[4];
	} kWords[] = {
		{"en", {"exit level", "are you sure", "difficulty", "load game"}},
		{"es", {"salir del nivel", "dificultad", "cargar partida", "guardar partida"}},
		{"it", {"abbandona", "sei sicuro", "difficolt", "carica partita"}},
		{"pt", {"certeza", "dificuldade", "carregar", "gravar jogo"}},
		{"fr", {"quitter", "difficult\xe9", "charger", "sauvegarder"}},
		{"de", {"beenden", "schwierigkeit", "laden", "speichern"}},
	};
	std::vector<uint8_t> lower(exe);
	for (uint8_t &c : lower) {
		c = (c >= 'A' && c <= 'Z') ? (uint8_t)(c + 32) : c;
	}
	const char *best = "?";
	int bestScore = 0;
	for (const auto &lang : kWords) {
		int score = 0;
		for (const char *w : lang.words) {
			auto same = [](uint8_t a, char b) { return a == (uint8_t)b; };  // "é" is negative as a char
			if (std::search(lower.begin(), lower.end(), w, w + strlen(w), same) != lower.end()) {
				score++;
			}
		}
		if (score > bestScore) {
			bestScore = score;
			best = lang.code;
		}
	}
	return best;
}

const GameVersion *IdentifyVersion(IsoImage &iso, bool *known)
{
	std::vector<uint8_t> exe;
	if (!iso.Read("SETUP\\BUZZLY\\HD\\BUZZ.EXE", exe) || exe.size() < 0x400 || exe[0] != 'M' || exe[1] != 'Z') {
		return nullptr;
	}
	uint32_t pe;
	memcpy(&pe, &exe[0x3c], 4);
	if (pe + 12 > exe.size() || memcmp(&exe[pe], "PE\0\0", 4) != 0) {
		return nullptr;
	}
	uint32_t stamp;
	memcpy(&stamp, &exe[pe + 8], 4);
	for (const GameVersion &v : kVersions) {
		if (v.stamp == stamp) {
			*known = true;
			return &v;
		}
	}
	*known = false;
	g_other.stamp = stamp;
	g_other.name = TextLanguage(exe);
	return &g_other;
}

const char *ExeLanguage(const std::wstring &exePath)
{
	std::vector<uint8_t> exe;
	return ReadWhole(exePath, exe) ? TextLanguage(exe) : "?";
}

bool ExeHasEngine(const std::wstring &exePath)
{
	std::vector<uint8_t> exe;
	uint32_t pe = 0, stamp = 0;
	if (!ReadWhole(exePath, exe) || exe.size() < 0x40) {
		return false;
	}
	memcpy(&pe, &exe[0x3c], 4);
	if (pe + 12 > exe.size()) {
		return false;
	}
	memcpy(&stamp, &exe[pe + 8], 4);
	for (const GameVersion &v : kVersions) {
		if (v.stamp == stamp) {
			return v.motor;
		}
	}
	return false;
}

std::wstring DefaultInstallDir()
{
	wchar_t *base = nullptr;
	std::wstring dir = L"C:\\Games";
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_UserProgramFiles, 0, nullptr, &base))) {
		dir = base;
	}
	CoTaskMemFree(base);
	return dir + L"\\" + Tr(T_TITLE);
}

std::wstring InstalledDir()
{
	// No registry, like the game: installed = this program is <game>\setup\BuzzSetup.exe
	wchar_t self[MAX_PATH];
	GetModuleFileNameW(nullptr, self, MAX_PATH);
	std::wstring path = self;
	std::wstring setupDir = path.substr(0, path.find_last_of(L'\\'));
	size_t slash = setupDir.find_last_of(L'\\');
	if (slash == std::wstring::npos || _wcsicmp(setupDir.c_str() + slash + 1, L"setup") != 0) {
		return L"";
	}
	std::wstring dir = setupDir.substr(0, slash);
	if (GetFileAttributesW((dir + L"\\buzz.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
		return L"";
	}
	return dir;
}

static std::wstring Widen(const char *s)
{
	return std::wstring(s, s + strlen(s));
}

static bool ReadWhole(const std::wstring &path, std::vector<uint8_t> &out)
{
	HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD size = GetFileSize(h, nullptr), got = 0;
	out.resize(size);
	bool ok = size == 0 || (ReadFile(h, out.data(), size, &got, nullptr) && got == size);
	CloseHandle(h);
	return ok;
}

static bool WriteWhole(const std::wstring &path, const void *data, size_t size)
{
	std::wstring parent = path.substr(0, path.find_last_of(L'\\'));
	SHCreateDirectoryExW(nullptr, parent.c_str(), nullptr);
	HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD put = 0;
	bool ok = size == 0 || (WriteFile(h, data, (DWORD)size, &put, nullptr) && put == size);
	CloseHandle(h);
	return ok;
}

bool LoadSetupFile(IsoImage *iso, const std::wstring &installDir, const char *path, std::vector<uint8_t> &out)
{
	if (iso) {
		return iso->Read(path, out);
	}
	// "SETUP\..." on the CD = "setup\..." in the game folder; the icons went to the game folder itself
	const char *name = strrchr(path, '\\');
	return ReadWhole(installDir + L"\\setup\\" + Widen(path + 6), out) ||
		(name && ReadWhole(installDir + L"\\" + Widen(name + 1), out));
}

static bool StartsWith(const std::string &s, const char *prefix)
{
	return s.compare(0, strlen(prefix), prefix) == 0;
}

// Where a file of the CD goes, relative to the game folder; "" = not installed
static std::wstring Target(const std::string &key, const std::wstring &name)
{
	static const char *const kArt[] = {"SETUP\\BURST\\BITMAPS\\", "SETUP\\BURST\\MEDIA\\", "SETUP\\BURST\\CURSORS\\"};
	if (StartsWith(key, "SETUP\\BUZZLY\\HD\\")) {
		return name.substr(16);
	}
	if (StartsWith(key, "SETUP\\BUZZLY\\CD\\")) {
		return L"cd\\" + name.substr(16);
	}
	if (key == "BUZZAG.HLP" || key == "BUZZAG.GID" || key == "ANET.INF") {
		return name;
	}
	if (key == "SETUP\\MEDIA\\BUZZ.ICO" || key == "SETUP\\MEDIA\\DELBUZZ.ICO" || key == "SETUP\\MEDIA\\HELP.ICO") {
		return name.substr(12);
	}
	bool art = key == "SETUP\\AUTOPLAY.INI" || key == "SETUP\\MEDIA\\BURSTBG.BMP" || key == "SETUP\\MEDIA\\SPLASH.BMP" ||
		key == "SETUP\\MEDIA\\BUZZ.WAV" || key == "SETUP\\MEDIA\\INSTBG.BMP";
	for (const char *prefix : kArt) {
		art = art || StartsWith(key, prefix);
	}
	art = art || (StartsWith(key, "SETUP\\BURST\\STRINGS\\") && key.size() > 4 && key.substr(key.size() - 4) == ".APS");
	if (art) {
		return L"setup\\" + name.substr(6);
	}
	return L"";
}

// buzzNN.sav and buzz2k.cfg in the game folder
static bool IsPlayerFile(const std::wstring &rel)
{
	if (rel.find(L'\\') != std::wstring::npos || rel.size() < 4) {
		return false;
	}
	const wchar_t *ext = rel.c_str() + rel.size() - 4;
	return _wcsicmp(ext, L".sav") == 0 || _wcsicmp(rel.c_str(), L"buzz2k.cfg") == 0;
}

static bool WriteResource(int id, const std::wstring &path)
{
	HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
	HGLOBAL data = res ? LoadResource(nullptr, res) : nullptr;
	if (!data) {
		return false;
	}
	return WriteWhole(path, LockResource(data), SizeofResource(nullptr, res));
}

static std::wstring KnownFolder(REFKNOWNFOLDERID id)
{
	wchar_t *path = nullptr;
	std::wstring out;
	if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &path))) {
		out = path;
	}
	CoTaskMemFree(path);
	return out;
}

static void Shortcut(const std::wstring &lnk, const std::wstring &target, const std::wstring &dir,
	const std::wstring &icon)
{
	IShellLinkW *link = nullptr;
	if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)))) {
		return;
	}
	link->SetPath(target.c_str());
	link->SetWorkingDirectory(dir.c_str());
	link->SetIconLocation(icon.c_str(), 0);
	IPersistFile *file = nullptr;
	if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
		file->Save(lnk.c_str(), TRUE);
		file->Release();
	}
	link->Release();
}

static std::wstring GameLink(REFKNOWNFOLDERID folder)
{
	return KnownFolder(folder) + L"\\" + Tr(T_TITLE) + L".lnk";
}

static std::wstring SetupLink()
{
	return KnownFolder(FOLDERID_Programs) + L"\\" + Tr(T_TITLE) + L" - Setup.lnk";
}

static std::wstring LinkTarget(const std::wstring &lnk)
{
	IShellLinkW *link = nullptr;
	wchar_t path[MAX_PATH] = L"";
	if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)))) {
		return path;
	}
	IPersistFile *file = nullptr;
	if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
		if (SUCCEEDED(file->Load(lnk.c_str(), STGM_READ))) {
			link->GetPath(path, MAX_PATH, nullptr, 0);
		}
		file->Release();
	}
	link->Release();
	return path;
}

// Deletes the shortcuts in a folder that lead into the game folder (whatever their name); true if there was any
static bool RemoveLinks(REFKNOWNFOLDERID folder, const std::wstring &dir)
{
	std::wstring base = KnownFolder(folder), prefix = dir + L"\\";
	bool any = false;
	WIN32_FIND_DATAW found;
	HANDLE h = FindFirstFileW((base + L"\\*.lnk").c_str(), &found);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	do {
		std::wstring lnk = base + L"\\" + found.cFileName, target = LinkTarget(lnk);
		if (target.size() > prefix.size() && _wcsnicmp(target.c_str(), prefix.c_str(), prefix.size()) == 0) {
			DeleteFileW(lnk.c_str());
			any = true;
		}
	} while (FindNextFileW(h, &found));
	FindClose(h);
	return any;
}

static std::wstring HelpLink(TextId which)
{
	return KnownFolder(FOLDERID_Programs) + L"\\" + Tr(T_TITLE) + L" - " + Tr(which) + L".lnk";
}

void CreateShortcuts(const std::wstring &dir, bool desktop, bool startMenu)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (desktop) {
		Shortcut(GameLink(FOLDERID_Desktop), dir + L"\\buzz.exe", dir, dir + L"\\buzz.ico");
	}
	if (startMenu) {
		Shortcut(GameLink(FOLDERID_Programs), dir + L"\\buzz.exe", dir, dir + L"\\buzz.ico");
		Shortcut(SetupLink(), dir + L"\\setup\\BuzzSetup.exe", dir + L"\\setup", dir + L"\\buzz.ico");
		Shortcut(HelpLink(T_HELP_LINK), dir + L"\\patch_help\\index.html", dir, dir + L"\\help.ico");
		if (GetFileAttributesW((dir + L"\\BUZZAG.html").c_str()) != INVALID_FILE_ATTRIBUTES) {
			Shortcut(HelpLink(T_GAME_HELP_LINK), dir + L"\\BUZZAG.html", dir, dir + L"\\help.ico");
		}
	}
	CoUninitialize();
}

static std::string ResourceText(int id)
{
	HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
	HGLOBAL data = res ? LoadResource(nullptr, res) : nullptr;
	return data ? std::string((const char *)LockResource(data), SizeofResource(nullptr, res)) : std::string();
}

// Our help (the folder dev\\help: page, style, script, lang\\xx.js) in <game>\\patch_help, opening in the
// installer's language
static bool WriteHelp(const std::wstring &dir, std::wstring &failed)
{
	std::wstring help = dir + L"\\patch_help";
	std::string page = ResourceText(IDR_HELP);
	size_t at = page.find("__LANG__");
	if (at != std::string::npos) {
		page.replace(at, 8, LanguageCodeOf(CurrentLanguage()));
	}
	failed = L"patch_help\\index.html";
	if (page.empty() || !WriteWhole(help + L"\\index.html", page.data(), page.size())) {
		return false;
	}
	static const struct {
		int id;
		const wchar_t *file;
	} kFiles[] = {{IDR_RUNIG_CSS, L"runig.css"}, {IDR_HELP_JS, L"help.js"}, {IDR_HELP_ES, L"lang\\es.js"},
		{IDR_HELP_EN, L"lang\\en.js"}, {IDR_HELP_FR, L"lang\\fr.js"}, {IDR_HELP_DE, L"lang\\de.js"},
		{IDR_HELP_IT, L"lang\\it.js"}, {IDR_HELP_PT, L"lang\\pt.js"}};
	for (const auto &f : kFiles) {
		failed = std::wstring(L"patch_help\\") + f.file;
		if (!WriteResource(f.id, help + L"\\" + f.file)) {
			return false;
		}
	}
	// The single-file page of the first versions
	DeleteFileW((dir + L"\\patch_help.html").c_str());
	failed.clear();
	return true;
}

// The game's own help (BUZZAG.HLP, WinHelp: Windows 10/11 no longer open it) as a web page, BUZZAG.html
static void WriteGameHelp(const std::wstring &dir)
{
	std::vector<uint8_t> hlp;
	std::string page;
	if (!ReadWhole(dir + L"\\BUZZAG.HLP", hlp) || !HlpToHtml(hlp, page)) {
		return;
	}
	WriteWhole(dir + L"\\BUZZAG.html", page.data(), page.size());
}

bool WriteFfmpeg(const std::wstring &setupDir)
{
	return WriteResource(IDR_FFMPEG, setupDir + L"\\ffmpeg.exe") &&
		WriteResource(IDR_FFMPEG_LICENSE, setupDir + L"\\ffmpeg-LICENSE.txt") &&
		WriteResource(IDR_FFMPEG_README, setupDir + L"\\ffmpeg-README.txt");
}

// What is ours in the game folder: the engine (if the version has it), the texts, and in setup\\ this program (to open
// it again without the CD) and ffmpeg (for "Translation")
static bool CopyOurFiles(const std::wstring &dir, bool motor, std::wstring &failed)
{
	if (motor && !WriteResource(IDR_DDRAW, dir + L"\\DDraw.dll")) {
		failed = L"DDraw.dll";
		return false;
	}
	if (!motor) {
		DeleteFileW((dir + L"\\DDraw.dll").c_str());  // left by an install of a version that has it
	}
	static const struct {
		int id;
		const wchar_t *file;
	} kTables[] = {{IDR_LANG_ES, L"es"}, {IDR_LANG_IT, L"it"}, {IDR_LANG_PT, L"pt"}, {IDR_LANG_FR, L"fr"},
		{IDR_LANG_DE, L"de"}};
	for (const auto &t : kTables) {
		std::wstring rel = std::wstring(L"lang\\") + t.file + L".json";
		if (!WriteResource(t.id, dir + L"\\" + rel)) {
			failed = rel;
			return false;
		}
	}
	wchar_t self[MAX_PATH];
	GetModuleFileNameW(nullptr, self, MAX_PATH);
	std::wstring setupDir = dir + L"\\setup";
	std::wstring selfDir = std::wstring(self).substr(0, std::wstring(self).find_last_of(L'\\'));
	if (_wcsicmp(selfDir.c_str(), setupDir.c_str()) != 0) {
		SHCreateDirectoryExW(nullptr, setupDir.c_str(), nullptr);
		if (!CopyFileW(self, (setupDir + L"\\BuzzSetup.exe").c_str(), FALSE)) {
			failed = L"setup\\BuzzSetup.exe";
			return false;
		}
	}
	if (!WriteFfmpeg(setupDir)) {
		failed = L"setup\\ffmpeg.exe";
		return false;
	}
	if (!WriteHelp(dir, failed)) {
		return false;
	}
	WriteGameHelp(dir);
	// The patch's version, for the versions without engine (with one, DDraw.dll says it)
	std::string version = std::to_string(CarriedEngineVersion());
	WriteWhole(dir + L"\\setup\\patch.txt", version.data(), version.size());
	return true;
}

uint64_t InstalledPatchVersion(const std::wstring &dir)
{
	if (ExeHasEngine(dir + L"\\buzz.exe")) {
		return EngineVersion(dir + L"\\DDraw.dll");
	}
	std::vector<uint8_t> text;
	if (!ReadWhole(dir + L"\\setup\\patch.txt", text)) {
		return 0;
	}
	return strtoull(std::string(text.begin(), text.end()).c_str(), nullptr, 10);
}

uint64_t EngineVersion(const std::wstring &dll)
{
	DWORD ignored = 0, size = GetFileVersionInfoSizeW(dll.c_str(), &ignored);
	if (size == 0) {
		return 0;
	}
	std::vector<uint8_t> info(size);
	VS_FIXEDFILEINFO *fixed = nullptr;
	UINT len = 0;
	if (!GetFileVersionInfoW(dll.c_str(), 0, size, info.data()) ||
		!VerQueryValueW(info.data(), L"\\", (void **)&fixed, &len) || !fixed) {
		return 0;
	}
	return (uint64_t)fixed->dwFileVersionMS << 32 | fixed->dwFileVersionLS;
}

uint64_t CarriedEngineVersion()
{
	static uint64_t version = ~0ull;
	if (version == ~0ull) {
		wchar_t tmp[MAX_PATH];
		GetTempPathW(MAX_PATH, tmp);
		std::wstring path = std::wstring(tmp) + L"BuzzSetup_DDraw.dll";
		version = WriteResource(IDR_DDRAW, path) ? EngineVersion(path) : 0;
		DeleteFileW(path.c_str());
	}
	return version;
}

std::wstring VersionText(uint64_t version)
{
	return std::to_wstring(version >> 48) + L"." + std::to_wstring(version >> 32 & 0xffff);
}

bool SameVersion(uint64_t a, uint64_t b)
{
	return a >> 32 == b >> 32;
}

bool UpdateEngine(const std::wstring &dir, std::wstring &failed)
{
	// The shortcuts the player has, again (their names may have changed); none if they had none
	bool startMenu = RemoveLinks(FOLDERID_Programs, dir), desktop = RemoveLinks(FOLDERID_Desktop, dir);
	bool ok = CopyOurFiles(dir, ExeHasEngine(dir + L"\\buzz.exe"), failed);
	CreateShortcuts(dir, desktop, startMenu);
	return ok;
}

bool Install(IsoImage &iso, const std::wstring &dir, const GameVersion &version, ProgressFn progress, void *ctx,
	std::wstring &failed)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	std::vector<std::pair<std::string, std::wstring>> copies;
	for (const auto &it : iso.Files()) {
		std::wstring rel = Target(it.first, it.second.name);
		if (!rel.empty()) {
			copies.push_back({it.first, rel});
		}
	}
	std::string eula = std::string("SETUP\\LEGAL\\") + version.eula;
	if (iso.Has(eula)) {
		copies.push_back({eula, L"EULA.txt"});
	}
	// The CD's files are the PC voices and videos: a translation done before in this folder is no longer there
	ForgetVoices(dir);
	std::vector<uint8_t> data;
	int done = 0, total = (int)copies.size() + 3;
	for (const auto &it : copies) {
		std::wstring path = dir + L"\\" + it.second;
		// The CD brings empty saved games and settings: the player's own are never overwritten
		if (IsPlayerFile(it.second)) {
			if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES &&
				(!iso.Read(it.first, data) || !WriteWhole(path, data.data(), data.size()))) {
				failed = it.second;
				return false;
			}
			progress(++done, total, ctx);
			continue;
		}
		if (!iso.Read(it.first, data) || !WriteWhole(path, data.data(), data.size())) {
			failed = it.second;
			return false;
		}
		progress(++done, total, ctx);
	}
	if (!CopyOurFiles(dir, version.motor, failed)) {
		return false;
	}
	progress(++done, total, ctx);
	progress(++done, total, ctx);
	progress(++done, total, ctx);
	CoUninitialize();
	return true;
}
