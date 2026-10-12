// BuzzSetup: asks for the PC CD image or an installed game first (the menu's pictures, sounds and texts are the
// original installer's, SETUP\MEDIA and SETUP\BURST on the CD), then shows that menu: install, or update and voices and
// videos once the game is installed (then from the copies in setup\, without the CD). The text language is an
// option inside the game.
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <thread>

#include "iso.h"
#include "resource.h"
#include "setup.h"
#include "texts.h"
#include "voices.h"

enum Action { A_INSTALL, A_VOICES, A_UPDATE, A_EXIT };

struct Button {
	Action action;
	HBITMAP img[3];  // normal, mouse over, pressed
	HICON icon;      // our own buttons: a Windows icon instead of the pictures
	int x, y, w, h;
	std::wstring label;
};

enum { WM_PROGRESS = WM_APP, WM_INSTALLED, WM_CONVERTED };

static const int kRowX = 232;    // centre of the button row
static const int kRowY = 303;    // centre of the icons
static const int kRowStep = 116; // between icon centres
static const int kLabelY = 324;  // top of the labels
static const int kNoteX = 6;     // the "unofficial" note, in the purple box at the bottom left
static const int kNoteY = 268;
// Mickey's ears (centres measured on BURSTBG.BMP): the patch in the left one, the languages in the right one
static const int kLeftEarX = 82, kLeftEarY = 160;
static const int kRightEarX = 316, kRightEarY = 118;
static const int kSparkX = 45;   // where SPRKL01-11 match the background (found by comparing the pictures)
static const int kSparkY = 0;
static const int kSparkW = 212;
static const int kSparkH = 80;
static const int kSparkFrames = 11;
static const int kSparkMs = 80;    // per frame
static const int kSparkPause = 30; // frames of pause between sweeps
static const int kSplashMs = 2500;

static HINSTANCE g_inst;
static HWND g_wnd;
static IsoImage *g_iso;
static IsoImage *g_disc;  // the console disc of "Translation"
static const GameVersion *g_version;
static std::wstring g_dir;  // installed game, "" if none
static std::wstring g_target;  // where the install running now goes
static ApsStrings g_aps;
static HBITMAP g_back;
static HBITMAP g_instBg, g_splash;  // the install screen
static RECT g_menuRect;  // where the menu window was before going full screen
static int g_backW = 404, g_backH = 355;
static std::vector<Button> g_buttons;
static std::vector<uint8_t> g_soundOver, g_soundClick;
static int g_hover = -1, g_pressed = -1;
static bool g_busy;
static int g_done, g_total;
static std::wstring g_status;
static HFONT g_font;
static void SaveLanguage(const std::wstring &dir);  // the installer's language, saved with the game
static HFONT g_small;  // the "unofficial" note
static std::vector<std::wstring> g_patchInfo, g_languageInfo;  // lines in the left and in the right ear
static HBITMAP g_sparks[11];
static int g_sparkFrame = -10;  // < 0: pause
static std::vector<uint8_t> g_music;
static HWAVEOUT g_waveOut;
static WAVEHDR g_musicHdr;

static bool Load(const char *path, std::vector<uint8_t> &out)
{
	return LoadSetupFile(g_iso, g_dir, path, out);
}

static HBITMAP LoadBmp(const char *path, int *w = nullptr, int *h = nullptr)
{
	std::vector<uint8_t> data;
	if (!Load(path, data) || data.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		return nullptr;
	}
	auto *file = (BITMAPFILEHEADER *)data.data();
	auto *info = (BITMAPINFO *)(data.data() + sizeof(BITMAPFILEHEADER));
	if (file->bfType != 0x4d42 || file->bfOffBits >= data.size()) {
		return nullptr;
	}
	HDC dc = GetDC(nullptr);
	HBITMAP bmp = CreateDIBitmap(dc, &info->bmiHeader, CBM_INIT, data.data() + file->bfOffBits, info, DIB_RGB_COLORS);
	ReleaseDC(nullptr, dc);
	if (w) {
		*w = info->bmiHeader.biWidth;
	}
	if (h) {
		*h = abs(info->bmiHeader.biHeight);
	}
	return bmp;
}

static HICON LoadIco(const char *path)
{
	std::vector<uint8_t> data;
	if (!Load(path, data) || data.size() < 22) {
		return nullptr;
	}
	// .ico file: 6-byte header, 16 bytes per image (size at +8, offset at +12); the first image will do
	uint32_t size, offset;
	memcpy(&size, &data[6 + 8], 4);
	memcpy(&offset, &data[6 + 12], 4);
	if (offset + size > data.size()) {
		return nullptr;
	}
	return CreateIconFromResourceEx(&data[offset], size, TRUE, 0x30000, 0, 0, LR_DEFAULTCOLOR);
}

static void AddButton(Action action, const char *art, const char *key, const wchar_t *fallback)
{
	static const char *const kStates[3] = {"ZERO", "_RO", "_MD"};
	Button b = {action, {}, nullptr, 0, 0, 0, 0, g_aps.Get(key, fallback)};
	for (int i = 0; art && i < 3; i++) {
		char path[MAX_PATH];
		// PLAYZERO / PLAY_RO, INS_ZERO / INS_RO...
		bool play = strcmp(art, "PLAY") == 0;
		wsprintfA(path, "SETUP\\BURST\\BITMAPS\\%s%s%s.BMP", art, i == 0 && !play ? "_" : "", kStates[i]);
		b.img[i] = LoadBmp(path, i == 0 ? &b.w : nullptr, i == 0 ? &b.h : nullptr);
	}
	g_buttons.push_back(b);
}

// Ours, not in the original installer: a globe (globe.ico, drawn by make_globe.py)
static void AddTranslationButton()
{
	AddButton(A_VOICES, nullptr, "", Tr(T_TRANSLATION));
	Button &b = g_buttons.back();
	b.icon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_GLOBE), IMAGE_ICON, 32, 32, 0);
	b.w = b.h = 32;
}

static const wchar_t *LanguageName(const std::string &code)
{
	return LanguageName(LanguageFromCode(code.c_str()));
}

// "1.0.409229": the version and its build as one number, minutes since 1 January 2026 (the build's day and minute)
static std::wstring PatchText(uint64_t version)
{
	if (!version) {
		return Tr(T_NO_PATCH);
	}
	unsigned day = (unsigned)(version >> 16 & 0xffff), minute = (unsigned)(version & 0xffff);
	return VersionText(version) + L"." + std::to_wstring(day * 1440 + minute);
}

// Three things apart: the patch this program carries; the installed game (its patch, the language of its voices and
// videos and of its buzz.exe); or, before installing, the CD's language
static void BuildInfo()
{
	g_patchInfo.clear();
	g_languageInfo.clear();
	uint64_t carried = CarriedEngineVersion();
	uint64_t installed = g_dir.empty() ? 0 : InstalledPatchVersion(g_dir);
	g_patchInfo.push_back(Tr(T_INFO_SETUP));
	g_patchInfo.push_back(PatchText(carried));
	if (g_dir.empty()) {
		g_languageInfo.push_back(Tr(T_INFO_CD));
		g_languageInfo.push_back(LanguageName(g_version ? g_version->name : "?"));
		return;
	}
	bool old = !installed && GetFileAttributesW((g_dir + L"\\DDraw.dll").c_str()) != INVALID_FILE_ATTRIBUTES;
	g_patchInfo.push_back(L"");
	g_patchInfo.push_back(Tr(T_INFO_INSTALLED));
	g_patchInfo.push_back(old ? Tr(T_OLD_PATCH) : PatchText(installed));
	std::string exe = ExeLanguage(g_dir + L"\\buzz.exe");
	std::string content = exe;
	DiscKind kind = DISC_PC;
	VoicesFrom(g_dir, &content, &kind);
	std::wstring name = LanguageName(content);
	if (kind == DISC_DREAMCAST) {
		name += L" (Dreamcast)";
	} else if (kind == DISC_PLAYSTATION) {
		name += L" (PlayStation)";
	}
	g_languageInfo.push_back(Tr(T_INFO_CONTENT));
	g_languageInfo.push_back(name);
	g_languageInfo.push_back(L"");
	g_languageInfo.push_back(Tr(T_INFO_EXE));
	g_languageInfo.push_back(LanguageName(exe));
}

static void BuildMenu()
{
	BuildInfo();
	g_buttons.clear();
	if (g_dir.empty()) {
		AddButton(A_INSTALL, "INS", "Install", L"Install");
	} else {
		// The game must have this program's very patch before anything else is done to it (an update may one day
		// need more than new files): a newer one here means "Update" instead of "Translation"; an older one here (an
		// old copy of the installer) means neither, except in the versions without engine
		uint64_t carried = CarriedEngineVersion(), installed = InstalledPatchVersion(g_dir);
		bool engine = ExeHasEngine(g_dir + L"\\buzz.exe");
		if (carried > installed) {
			AddButton(A_UPDATE, "INS", "", Tr(T_UPDATE));
		} else if (!engine || carried == installed) {
			AddTranslationButton();
		}
	}
	AddButton(A_EXIT, "CAN", "Exit", L"Exit");
	// A row at the bottom of Mickey's face, centred where AUTOPLAY.INI puts the exit button (182,304, 27x31);
	// labels under the icons
	int n = (int)g_buttons.size();
	for (int i = 0; i < n; i++) {
		Button &b = g_buttons[i];
		int cx = kRowX + (2 * i - (n - 1)) * kRowStep / 2;
		b.x = cx - b.w / 2;
		b.y = kRowY - b.h / 2;
	}
	g_hover = g_pressed = -1;
}

static void DrawOutlined(HDC dc, int x, int y, const std::wstring &text, COLORREF color)
{
	SetTextColor(dc, RGB(40, 0, 80));
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			TextOutW(dc, x + dx, y + dy, text.c_str(), (int)text.size());
		}
	}
	SetTextColor(dc, color);
	TextOutW(dc, x, y, text.c_str(), (int)text.size());
}

// Lines centred on an ear, both ways
static void DrawEarLines(HDC dc, const std::vector<std::wstring> &lines, int cx, int cy)
{
	int y = cy - (int)lines.size() * 13 / 2;
	for (const std::wstring &l : lines) {
		SIZE size;
		GetTextExtentPoint32W(dc, l.c_str(), (int)l.size(), &size);
		TextOutW(dc, cx - size.cx / 2, y, l.c_str(), (int)l.size());
		y += 13;
	}
}

static void PaintMenu(HDC dc, HDC src)
{
	if (g_back) {
		SelectObject(src, g_back);
		BitBlt(dc, 0, 0, g_backW, g_backH, src, 0, 0, SRCCOPY);
	}
	if (g_sparkFrame >= 0 && g_sparks[g_sparkFrame]) {
		SelectObject(src, g_sparks[g_sparkFrame]);
		BitBlt(dc, kSparkX, kSparkY, kSparkW, kSparkH, src, 0, 0, SRCCOPY);
	}
	SelectObject(dc, g_font);
	for (size_t i = 0; i < g_buttons.size(); i++) {
		const Button &b = g_buttons[i];
		int state = (int)i == g_pressed ? 2 : (int)i == g_hover ? 1 : 0;
		HBITMAP img = b.img[state] ? b.img[state] : b.img[0];
		if (img) {
			SelectObject(src, img);
			BitBlt(dc, b.x, b.y, b.w, b.h, src, 0, 0, SRCCOPY);
		}
		if (b.icon) {
			int push = state == 2 ? 1 : 0;  // pressed: one pixel down and right, like the original buttons
			DrawIconEx(dc, b.x + push, b.y + push, b.icon, b.w, b.h, 0, nullptr, DI_NORMAL);
		}
		SIZE size;
		GetTextExtentPoint32W(dc, b.label.c_str(), (int)b.label.size(), &size);
		DrawOutlined(dc, b.x + b.w / 2 - size.cx / 2, kLabelY, b.label, state ? RGB(255, 255, 255) : RGB(255, 210, 0));
	}
	SelectObject(dc, g_small);
	SetTextColor(dc, RGB(40, 0, 80));
	DrawEarLines(dc, g_patchInfo, kLeftEarX, kLeftEarY);
	DrawEarLines(dc, g_languageInfo, kRightEarX, kRightEarY);
	// "Unofficial patch..." in the purple box at the bottom left, one line per \n
	std::wstring text = Tr(T_DISCLAIMER);
	int y = kNoteY;
	for (size_t pos = 0; pos <= text.size();) {
		size_t end = text.find(L'\n', pos);
		if (end == std::wstring::npos) {
			end = text.size();
		}
		DrawOutlined(dc, kNoteX, y, text.substr(pos, end - pos), RGB(255, 255, 255));
		y += 13;
		pos = end + 1;
	}
}

// While installing, like the original (INSTALL.INI [BACKGROUND]): the whole screen tiled with INSTBG.BMP, the
// splash picture in the middle and a green progress bar under it
static void PaintInstalling(HDC dc, HDC src, int w, int h)
{
	BITMAP tile = {};
	if (g_instBg && GetObject(g_instBg, sizeof tile, &tile) && tile.bmWidth > 0 && tile.bmHeight > 0) {
		SelectObject(src, g_instBg);
		for (int y = 0; y < h; y += tile.bmHeight) {
			for (int x = 0; x < w; x += tile.bmWidth) {
				BitBlt(dc, x, y, tile.bmWidth, tile.bmHeight, src, 0, 0, SRCCOPY);
			}
		}
	} else {
		RECT all = {0, 0, w, h};
		HBRUSH blue = CreateSolidBrush(RGB(0, 150, 230));
		FillRect(dc, &all, blue);
		DeleteObject(blue);
	}
	BITMAP splash = {450, 275};
	if (g_splash) {
		GetObject(g_splash, sizeof splash, &splash);
	}
	int sx = (w - splash.bmWidth) / 2;
	int sy = (h - splash.bmHeight) / 2 - 30;
	if (g_splash) {
		SelectObject(src, g_splash);
		BitBlt(dc, sx, sy, splash.bmWidth, splash.bmHeight, src, 0, 0, SRCCOPY);
	}
	SelectObject(dc, g_font);
	int by = sy + splash.bmHeight + 16;
	DrawOutlined(dc, sx, by, g_status, RGB(255, 255, 255));
	RECT bar = {sx, by + 24, sx + splash.bmWidth, by + 40};
	FillRect(dc, &bar, (HBRUSH)GetStockObject(BLACK_BRUSH));
	if (g_total > 0) {
		RECT fill = {bar.left + 2, bar.top + 2, bar.left + 2 + (bar.right - bar.left - 4) * g_done / g_total,
			bar.bottom - 2};
		HBRUSH green = CreateSolidBrush(RGB(0, 150, 0));  // PROGRESS_BAR_RED/GREEN/BLUE
		FillRect(dc, &fill, green);
		DeleteObject(green);
	}
}

static void Paint(HDC target, int w, int h)
{
	HDC dc = CreateCompatibleDC(target);
	HBITMAP frame = CreateCompatibleBitmap(target, w, h);
	HGDIOBJ oldFrame = SelectObject(dc, frame);
	HDC src = CreateCompatibleDC(target);
	SetBkMode(dc, TRANSPARENT);
	HGDIOBJ oldFont = SelectObject(dc, g_font);
	if (g_busy) {
		PaintInstalling(dc, src, w, h);
	} else {
		PaintMenu(dc, src);
	}
	SelectObject(dc, oldFont);
	BitBlt(target, 0, 0, w, h, dc, 0, 0, SRCCOPY);
	DeleteDC(src);
	SelectObject(dc, oldFrame);
	DeleteObject(frame);
	DeleteDC(dc);
}

static int HitButton(int x, int y)
{
	if (g_busy) {
		return -1;
	}
	for (size_t i = 0; i < g_buttons.size(); i++) {
		const Button &b = g_buttons[i];
		int cx = b.x + b.w / 2;
		if (x >= cx - kRowStep / 2 && x < cx + kRowStep / 2 && y >= b.y && y < kLabelY + 16) {
			return (int)i;
		}
	}
	return -1;
}

static void PlayWav(const std::vector<uint8_t> &wav)
{
	if (!wav.empty()) {
		PlaySoundW((LPCWSTR)wav.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
	}
}

// Background music: SETUP\MEDIA\BUZZ.WAV (the game's title theme), looped on its own waveOut channel so that
// PlaySound stays free for the button sounds
static void StartMusic()
{
	if (!Load("SETUP\\MEDIA\\BUZZ.WAV", g_music) || g_music.size() < 12 || memcmp(g_music.data(), "RIFF", 4) != 0) {
		return;
	}
	WAVEFORMATEX fmt = {};
	const uint8_t *data = nullptr;
	uint32_t size = 0;
	for (size_t pos = 12; pos + 8 <= g_music.size();) {
		uint32_t len;
		memcpy(&len, &g_music[pos + 4], 4);
		len = (uint32_t)min((size_t)len, g_music.size() - pos - 8);
		if (memcmp(&g_music[pos], "fmt ", 4) == 0) {
			memcpy(&fmt, &g_music[pos + 8], min(len, 16u));
		} else if (memcmp(&g_music[pos], "data", 4) == 0) {
			data = &g_music[pos + 8];
			size = len;
		}
		pos += 8 + len + (len & 1);
	}
	if (!data || fmt.wFormatTag != WAVE_FORMAT_PCM ||
		waveOutOpen(&g_waveOut, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		g_waveOut = nullptr;
		return;
	}
	g_musicHdr.lpData = (LPSTR)data;
	g_musicHdr.dwBufferLength = size;
	g_musicHdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
	g_musicHdr.dwLoops = 0xffffffff;
	waveOutPrepareHeader(g_waveOut, &g_musicHdr, sizeof g_musicHdr);
	waveOutWrite(g_waveOut, &g_musicHdr, sizeof g_musicHdr);
}

static void StopMusic()
{
	if (g_waveOut) {
		waveOutReset(g_waveOut);
		waveOutUnprepareHeader(g_waveOut, &g_musicHdr, sizeof g_musicHdr);
		waveOutClose(g_waveOut);
		g_waveOut = nullptr;
	}
}

// Sparkles over the Disney logo (SETUP\BURST\BITMAPS\SPRKL01-11): one sweep, then a pause
static void LoadSparks()
{
	for (int i = 0; i < kSparkFrames; i++) {
		char path[MAX_PATH];
		wsprintfA(path, "SETUP\\BURST\\BITMAPS\\SPRKL%02d.BMP", i + 1);
		g_sparks[i] = LoadBmp(path);
	}
}

static void StepSparks()
{
	g_sparkFrame++;
	if (g_sparkFrame >= kSparkFrames) {
		g_sparkFrame = -kSparkPause;
	}
	if (g_sparkFrame >= -1 || g_sparkFrame == -kSparkPause) {  // a sparkle frame, or the background back
		RECT r = {kSparkX, kSparkY, kSparkX + kSparkW, kSparkY + kSparkH};
		InvalidateRect(g_wnd, &r, FALSE);
	}
}

// SETUP\MEDIA\SPLASH.BMP for a moment before the menu; a click skips it
static LRESULT CALLBACK SplashProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(wnd, &ps);
		HDC src = CreateCompatibleDC(dc);
		HBITMAP bmp = (HBITMAP)GetWindowLongPtrW(wnd, GWLP_USERDATA);
		BITMAP info;
		GetObject(bmp, sizeof info, &info);
		SelectObject(src, bmp);
		BitBlt(dc, 0, 0, info.bmWidth, info.bmHeight, src, 0, 0, SRCCOPY);
		DeleteDC(src);
		EndPaint(wnd, &ps);
		return 0;
	}
	case WM_TIMER:
	case WM_LBUTTONDOWN:
	case WM_KEYDOWN:
		DestroyWindow(wnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}

static void ShowSplash(HINSTANCE inst)
{
	int w = 0, h = 0;
	HBITMAP bmp = LoadBmp("SETUP\\MEDIA\\SPLASH.BMP", &w, &h);
	if (!bmp) {
		return;
	}
	WNDCLASSW wc = {};
	wc.lpfnWndProc = SplashProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"BuzzSetupSplash";
	RegisterClassW(&wc);
	HWND wnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, Tr(T_TITLE), WS_POPUP,
		(GetSystemMetrics(SM_CXSCREEN) - w) / 2, (GetSystemMetrics(SM_CYSCREEN) - h) / 2, w, h, nullptr, nullptr, inst,
		nullptr);
	SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG_PTR)bmp);
	ShowWindow(wnd, SW_SHOW);
	SetTimer(wnd, 1, kSplashMs, nullptr);
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	DeleteObject(bmp);
}

// The PC CD image; with orGame, also the buzz.exe of a game installed by this program (then g_dir = its folder)
static bool AskIso(bool orGame)
{
	wchar_t file[MAX_PATH] = {};
	std::wstring filter = std::wstring(Tr(orGame ? T_ISO_OR_GAME_FILTER : T_ISO_FILTER));
	filter += orGame ? L" (*.iso, *.bin, *.cue, buzz.exe)" : L" (*.iso, *.bin, *.cue)";
	filter += L'\0';
	filter += orGame ? L"*.iso;*.bin;*.cue;buzz.exe" : L"*.iso;*.bin;*.cue";
	filter += L'\0';
	OPENFILENAMEW ofn = {sizeof ofn};
	ofn.hwndOwner = g_wnd;
	ofn.lpstrFilter = filter.c_str();
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = Tr(orGame ? T_PICK_ISO_OR_GAME : T_PICK_ISO);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	while (GetOpenFileNameW(&ofn)) {
		std::wstring path = file;
		std::wstring dir = path.substr(0, path.find_last_of(L'\\'));
		if (orGame && _wcsicmp(path.c_str() + dir.size() + 1, L"buzz.exe") == 0) {
			if (GetFileAttributesW((dir + L"\\setup\\BuzzSetup.exe").c_str()) != INVALID_FILE_ATTRIBUTES) {
				g_dir = dir;
				return true;
			}
			MessageBoxW(g_wnd, Tr(T_NOT_OURS), Tr(T_TITLE), MB_ICONWARNING);
			continue;
		}
		auto *iso = new IsoImage();
		bool known = false;
		const GameVersion *version = iso->Open(file) ? IdentifyVersion(*iso, &known) : nullptr;
		if (version && !version->motor) {
			// A version the patch does not install (the Portuguese one); its disc is good for "Translation"
			delete iso;
			MessageBoxW(g_wnd, Tr(T_OTHER_VERSION), Tr(T_TITLE), MB_ICONWARNING);
			continue;
		}
		if (version) {
			delete g_iso;
			g_iso = iso;
			g_version = version;
			if (version->needsDisc) {
				MessageBoxW(g_wnd, Tr(T_NEEDS_DISC), Tr(T_TITLE), MB_ICONWARNING);
			}
			return true;
		}
		delete iso;
		MessageBoxW(g_wnd, Tr(T_BAD_ISO), Tr(T_TITLE), MB_ICONWARNING);
	}
	return false;
}

static std::wstring AskFolder()
{
	std::wstring result;
	IFileOpenDialog *dlg = nullptr;
	if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)))) {
		return result;
	}
	DWORD opts;
	dlg->GetOptions(&opts);
	dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
	dlg->SetTitle(Tr(T_PICK_FOLDER));
	dlg->SetOkButtonLabel(Tr(T_INSTALL_HERE));
	std::wstring def = DefaultInstallDir();
	std::wstring name = def.substr(def.find_last_of(L'\\'));
	// The chosen folder is the root: the game always gets its own folder inside
	IFileDialogCustomize *custom = nullptr;
	if (SUCCEEDED(dlg->QueryInterface(IID_PPV_ARGS(&custom)))) {
		wchar_t note[512];
		swprintf(note, 512, Tr(T_FOLDER_NOTE), name.c_str() + 1);
		custom->AddText(1, note);
		custom->Release();
	}
	std::wstring parent = def.substr(0, def.find_last_of(L'\\'));
	CreateDirectoryW(parent.c_str(), nullptr);
	IShellItem *start = nullptr;
	if (SUCCEEDED(SHCreateItemFromParsingName(parent.c_str(), nullptr, IID_PPV_ARGS(&start)))) {
		dlg->SetFolder(start);
		start->Release();
	}
	IShellItem *item = nullptr;
	if (SUCCEEDED(dlg->Show(g_wnd)) && SUCCEEDED(dlg->GetResult(&item))) {
		wchar_t *path = nullptr;
		if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
			result = path;
			// Unless the chosen folder is already the game's (installing again over it)
			if (result.size() < name.size() || _wcsicmp(result.c_str() + result.size() - name.size(), name.c_str()) != 0) {
				result += name;
			}
		}
		CoTaskMemFree(path);
		item->Release();
	}
	dlg->Release();
	return result;
}

static void ProgressCb(int done, int total, void *)
{
	PostMessageW(g_wnd, WM_PROGRESS, done, total);
}

// Full screen while working, like the original installer while copying
static void StartBusy(TextId status)
{
	g_busy = true;
	g_done = 0;
	g_total = 1;
	g_status = Tr(status);
	GetWindowRect(g_wnd, &g_menuRect);
	MONITORINFO mi = {sizeof mi};
	GetMonitorInfoW(MonitorFromWindow(g_wnd, MONITOR_DEFAULTTONEAREST), &mi);
	SetWindowPos(g_wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER);
	InvalidateRect(g_wnd, nullptr, FALSE);
}

static void EndBusy()
{
	g_busy = false;
	SetWindowPos(g_wnd, nullptr, g_menuRect.left, g_menuRect.top, g_menuRect.right - g_menuRect.left,
		g_menuRect.bottom - g_menuRect.top, SWP_NOZORDER | SWP_NOOWNERZORDER);
	InvalidateRect(g_wnd, nullptr, FALSE);
}

static void StartInstall(const std::wstring &dir)
{
	g_target = dir;
	StartBusy(T_INSTALLING);
	std::thread([dir]() {
		auto *failed = new std::wstring();
		bool ok = Install(*g_iso, dir, *g_version, ProgressCb, nullptr, *failed);
		PostMessageW(g_wnd, WM_INSTALLED, ok, (LPARAM)failed);
	}).detach();
}

// "Translation": voices and videos from a console disc, or back to the PC ones. True if a conversion started (it
// ends with WM_CONVERTED)
static bool AskVoices()
{
	TASKDIALOG_BUTTON buttons[2] = {{100, Tr(T_FROM_DISC)}, {101, Tr(T_RESTORE)}};
	TASKDIALOGCONFIG td = {sizeof td};
	td.hwndParent = g_wnd;
	td.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
	td.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	td.pszWindowTitle = Tr(T_TITLE);
	td.pszMainInstruction = Tr(T_TRANSLATION);
	td.pszContent = Tr(T_VOICES_NOTE);
	td.pButtons = buttons;
	td.cButtons = HasOriginals(g_dir) ? 2 : 1;
	int choice = 0;
	if (FAILED(TaskDialogIndirect(&td, &choice, nullptr, nullptr))) {
		return false;
	}
	if (choice == 101) {
		std::wstring failed;
		if (RestoreOriginals(g_dir, failed)) {
			BuildMenu();
			InvalidateRect(g_wnd, nullptr, FALSE);
			MessageBoxW(g_wnd, Tr(T_RESTORED), Tr(T_TITLE), MB_ICONINFORMATION);
		} else {
			MessageBoxW(g_wnd, (std::wstring(Tr(T_COPY_FAILED)) + L"\n" + failed).c_str(), Tr(T_TITLE), MB_ICONERROR);
		}
		return false;
	}
	if (choice != 100) {
		return false;
	}
	std::wstring ffmpeg = FindFfmpeg(g_dir);
	if (ffmpeg.empty()) {
		MessageBoxW(g_wnd, Tr(T_NO_FFMPEG), Tr(T_TITLE), MB_ICONWARNING);
		return false;
	}
	wchar_t file[MAX_PATH] = {};
	std::wstring filter = std::wstring(Tr(T_DISC_FILTER)) + L" (*.cdi, *.bin, *.cue, *.iso)";
	filter += L'\0';
	filter += L"*.cdi;*.bin;*.cue;*.iso";
	filter += L'\0';
	OPENFILENAMEW ofn = {sizeof ofn};
	ofn.hwndOwner = g_wnd;
	ofn.lpstrFilter = filter.c_str();
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = Tr(T_PICK_DISC);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	DiscKind kind = DISC_NONE;
	while (kind == DISC_NONE) {
		if (!GetOpenFileNameW(&ofn)) {
			return false;
		}
		delete g_disc;
		g_disc = new IsoImage();
		kind = g_disc->Open(file) ? IdentifyDisc(*g_disc) : DISC_NONE;
		if (kind == DISC_NONE) {
			MessageBoxW(g_wnd, Tr(T_BAD_DISC), Tr(T_TITLE), MB_ICONWARNING);
		}
	}
	StartBusy(T_CONVERTING);
	std::wstring dir = g_dir;
	std::thread([kind, dir, ffmpeg]() {
		auto *failed = new std::wstring();
		bool ok = ApplyVoices(*g_disc, kind, dir, ffmpeg, ProgressCb, nullptr, *failed);
		PostMessageW(g_wnd, WM_CONVERTED, ok, (LPARAM)failed);
	}).detach();
	return true;
}

// The end of an install: the game folder opens and this program closes
static bool g_closeAfterConverting;

static void FinishInstall()
{
	ShellExecuteW(nullptr, L"open", g_dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	DestroyWindow(g_wnd);
}

static void Do(Action action)
{
	switch (action) {
	case A_INSTALL: {
		std::wstring dir = AskFolder();
		if (!dir.empty() &&
			MessageBoxW(g_wnd, (std::wstring(Tr(T_CONFIRM_DIR)) + L"\n\n" + dir).c_str(), Tr(T_TITLE),
				MB_YESNO | MB_ICONQUESTION) == IDYES) {
			g_dir = L"";
			StartInstall(dir);
		}
		break;
	}
	case A_VOICES:
		AskVoices();
		break;
	case A_UPDATE: {
		std::wstring failed;
		if (UpdateEngine(g_dir, failed)) {
			SaveLanguage(g_dir);
			BuildMenu();
			InvalidateRect(g_wnd, nullptr, FALSE);
			MessageBoxW(g_wnd, Tr(T_UPDATED), Tr(T_TITLE), MB_ICONINFORMATION);
		} else {
			MessageBoxW(g_wnd, (std::wstring(Tr(T_COPY_FAILED)) + L"\n" + failed).c_str(), Tr(T_TITLE), MB_ICONERROR);
		}
		break;
	}
	case A_EXIT:
		DestroyWindow(g_wnd);
		break;
	}
}

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(wnd, &ps);
		RECT client;
		GetClientRect(wnd, &client);
		Paint(dc, client.right, client.bottom);
		EndPaint(wnd, &ps);
		return 0;
	}
	case WM_NCHITTEST: {
		// Dragging the window by its picture (it has no title bar, like the original)
		POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
		ScreenToClient(wnd, &p);
		return HitButton(p.x, p.y) >= 0 || g_busy ? HTCLIENT : HTCAPTION;
	}
	case WM_MOUSEMOVE: {
		int hit = HitButton((short)LOWORD(lp), (short)HIWORD(lp));
		if (hit != g_hover) {
			g_hover = hit;
			if (hit >= 0) {
				PlayWav(g_soundOver);
				TRACKMOUSEEVENT tme = {sizeof tme, TME_LEAVE, wnd, 0};
				TrackMouseEvent(&tme);
			}
			InvalidateRect(wnd, nullptr, FALSE);
		}
		return 0;
	}
	case WM_MOUSELEAVE:
		g_hover = g_pressed = -1;
		InvalidateRect(wnd, nullptr, FALSE);
		return 0;
	case WM_LBUTTONDOWN:
		g_pressed = HitButton((short)LOWORD(lp), (short)HIWORD(lp));
		if (g_pressed >= 0) {
			PlayWav(g_soundClick);
			SetCapture(wnd);
		}
		InvalidateRect(wnd, nullptr, FALSE);
		return 0;
	case WM_LBUTTONUP: {
		int was = g_pressed;
		g_pressed = -1;
		ReleaseCapture();
		InvalidateRect(wnd, nullptr, FALSE);
		if (was >= 0 && was == HitButton((short)LOWORD(lp), (short)HIWORD(lp))) {
			Do(g_buttons[was].action);
		}
		return 0;
	}
	case WM_PROGRESS:
		g_done = (int)wp;
		g_total = (int)lp;
		InvalidateRect(wnd, nullptr, FALSE);
		return 0;
	case WM_INSTALLED: {
		auto *failed = (std::wstring *)lp;
		EndBusy();
		if (wp) {
			g_dir = g_target;
			SaveLanguage(g_dir);
			g_status = Tr(T_INSTALLED);
			BuildMenu();
		} else {
			g_status.clear();
			MessageBoxW(wnd, (std::wstring(Tr(T_COPY_FAILED)) + L"\n" + *failed).c_str(), Tr(T_TITLE), MB_ICONERROR);
		}
		delete failed;
		InvalidateRect(wnd, nullptr, FALSE);
		// Installed: shortcuts and translation only if the player wants them, then the game folder and that is all
		if (wp) {
			if (MessageBoxW(wnd, Tr(T_ASK_LINKS), Tr(T_TITLE), MB_YESNO | MB_ICONQUESTION) == IDYES) {
				CreateShortcuts(g_dir, true, true);
			}
			bool converting = MessageBoxW(wnd, Tr(T_ASK_VOICES), Tr(T_TITLE), MB_YESNO | MB_ICONQUESTION) == IDYES &&
				AskVoices();
			if (converting) {
				g_closeAfterConverting = true;
			} else {
				FinishInstall();
			}
		}
		return 0;
	}
	case WM_CONVERTED: {
		auto *failed = (std::wstring *)lp;
		EndBusy();
		if (wp) {
			MessageBoxW(wnd, Tr(T_VOICES_DONE), Tr(T_TITLE), MB_ICONINFORMATION);
		} else {
			MessageBoxW(wnd, (std::wstring(Tr(T_CONVERT_FAILED)) + L"\n" + *failed).c_str(), Tr(T_TITLE), MB_ICONERROR);
		}
		delete failed;
		delete g_disc;
		g_disc = nullptr;
		if (g_closeAfterConverting) {
			FinishInstall();
			return 0;
		}
		BuildMenu();
		InvalidateRect(wnd, nullptr, FALSE);
		return 0;
	}
	case WM_TIMER:
		StepSparks();
		return 0;
	case WM_CLOSE:
		if (g_busy) {
			return 0;  // closing now would leave the game half copied or half converted
		}
		break;
	case WM_DESTROY:
		StopMusic();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}

static void NoProgress(int, int, void *)
{
}

// The installer's language, first of all: each one written in itself, Windows' already chosen
static bool AskLanguage()
{
	TASKDIALOG_BUTTON radios[INSTALLER_LANGUAGES];
	for (int i = 0; i < INSTALLER_LANGUAGES; i++) {
		radios[i] = {100 + i, LanguageOwnName(i)};
	}
	TASKDIALOGCONFIG td = {sizeof td};
	td.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
	td.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
	td.pszWindowTitle = L"Buzz Lightyear of Star Command";  // before choosing: no "& Beyond" in any language yet
	td.pszMainInstruction = L"Language · Idioma · Langue · Sprache · Lingua";
	td.pRadioButtons = radios;
	td.cRadioButtons = INSTALLER_LANGUAGES;
	td.nDefaultRadioButton = 100 + WindowsLanguage();
	int button = 0, radio = 0;
	if (FAILED(TaskDialogIndirect(&td, &button, &radio, nullptr)) || button != IDOK) {
		return false;
	}
	SetLanguage(radio - 100);
	return true;
}

// setup\language.txt of an installed game: the language chosen when it was installed (also the game's first one)
static bool LoadSavedLanguage(const std::wstring &dir)
{
	char code[8] = {};
	FILE *f = dir.empty() ? nullptr : _wfopen((dir + L"\\setup\\language.txt").c_str(), L"rb");
	if (!f) {
		return false;
	}
	fread(code, 1, sizeof code - 1, f);
	fclose(f);
	int lang = LanguageFromCode(code);
	if (lang < 0) {
		return false;
	}
	SetLanguage(lang);
	return true;
}

static void SaveLanguage(const std::wstring &dir)
{
	FILE *f = _wfopen((dir + L"\\setup\\language.txt").c_str(), L"wb");
	if (f) {
		fputs(LanguageCodeOf(CurrentLanguage()), f);
		fclose(f);
	}
}

// BuzzSetup.exe /voices <console disc> <game folder>: no window; 0 = done, 1 = not a known disc or no ffmpeg,
// 2 = a conversion failed
static int VoicesSilently(const wchar_t *image, const wchar_t *dir)
{
	IsoImage disc;
	DiscKind kind = disc.Open(image) ? IdentifyDisc(disc) : DISC_NONE;
	std::wstring ffmpeg = FindFfmpeg(dir);
	if (kind == DISC_NONE || ffmpeg.empty()) {
		return 1;
	}
	std::wstring failed;
	return ApplyVoices(disc, kind, dir, ffmpeg, NoProgress, nullptr, failed) ? 0 : 2;
}

// BuzzSetup.exe /install <CD image> <folder>: no window; exit code 0 = installed, 1 = not the game, 2 = write error
static int InstallSilently(const wchar_t *image, const wchar_t *dir)
{
	IsoImage iso;
	bool known = false;
	const GameVersion *version = iso.Open(image) ? IdentifyVersion(iso, &known) : nullptr;
	if (!version || !version->motor) {
		return 1;  // not the game, or a version the patch does not install
	}
	std::wstring failed;
	return Install(iso, dir, *version, NoProgress, nullptr, failed) ? 0 : 2;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show)
{
	g_inst = inst;
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc == 4 && _wcsicmp(argv[1], L"/install") == 0) {
		return InstallSilently(argv[2], argv[3]);
	}
	if (argc == 4 && _wcsicmp(argv[1], L"/voices") == 0) {
		return VoicesSilently(argv[2], argv[3]);
	}
	if (argc == 3 && _wcsicmp(argv[1], L"/update") == 0) {
		std::wstring failed;
		return UpdateEngine(argv[2], failed) ? 0 : 2;
	}
	if (argc == 3 && _wcsicmp(argv[1], L"/restore") == 0) {
		std::wstring failed;
		return RestoreOriginals(argv[2], failed) ? 0 : 2;
	}
	g_dir = InstalledDir();
	if (!LoadSavedLanguage(g_dir) && !AskLanguage()) {
		return 0;
	}
	if (g_dir.empty() && !AskIso(true)) {
		return 0;
	}
	std::vector<uint8_t> aps;
	if (Load((std::string("SETUP\\BURST\\STRINGS\\") + ApsName()).c_str(), aps) ||
		Load("SETUP\\BURST\\STRINGS\\USA.APS", aps)) {
		g_aps.Load(aps);
	}
	g_back = LoadBmp("SETUP\\MEDIA\\BURSTBG.BMP", &g_backW, &g_backH);
	g_instBg = LoadBmp("SETUP\\MEDIA\\INSTBG.BMP");
	g_splash = LoadBmp("SETUP\\MEDIA\\SPLASH.BMP");
	Load("SETUP\\BURST\\MEDIA\\INSTALLRO.WAV", g_soundOver);
	Load("SETUP\\BURST\\MEDIA\\INSTALLMD.WAV", g_soundClick);
	g_font = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");
	g_small = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");
	BuildMenu();
	LoadSparks();
	StartMusic();
	ShowSplash(inst);

	WNDCLASSW wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIco("SETUP\\MEDIA\\BUZZ.ICO");
	wc.lpszClassName = L"BuzzSetup";
	RegisterClassW(&wc);
	int x = (GetSystemMetrics(SM_CXSCREEN) - g_backW) / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) - g_backH) / 2;
	g_wnd = CreateWindowExW(0, wc.lpszClassName, Tr(T_TITLE), WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, x, y, g_backW,
		g_backH, nullptr, nullptr, inst, nullptr);
	ShowWindow(g_wnd, show);
	SetTimer(g_wnd, 1, kSparkMs, nullptr);
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	CoUninitialize();
	return 0;
}
