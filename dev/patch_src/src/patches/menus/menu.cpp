// PATCH OPTIONS: the options menu's "configure graphics" entry renamed, and its screen (ConfigureGraphicsMenu
// 004a0280) replaced by our settings list. Left / right apply at once; accept saves to buzz2k.cfg, back undoes.
// Also the pause menu: "modern" in camera mode, and "restart level" (UpdatePauseMenu 00433330, DrawHud's lines).
#include "common.h"
#include "config.h"
#include "texts.h"
#include "language.h"
#include "render.h"

#define g_padButtons (*(unsigned short *)A(0x0088995c)) // pad bits of this frame (UpdatePadButtons)
#define g_frameStep (*(int *)A(0x00535be4))
#define g_gameOptions (*(unsigned char *)A(0x005358a8))  // bit 0x40: directional control (else rotational)

typedef void(__cdecl *DrawGraphicsMenuTextFn)(int y, const char *text, int flag);
typedef void(__cdecl *DrawMenuTextFn)(int x, int y, const char *text, int a, int b, int c);
typedef void(__cdecl *PlaySoundFn)(int sound, int pitch, int volL, int volR);
typedef char(__cdecl *KeyPressedNowFn)(int key);
static const DrawGraphicsMenuTextFn DrawGraphicsMenuText = (DrawGraphicsMenuTextFn)A(0x0049e880);
static const DrawMenuTextFn DrawMenuText = (DrawMenuTextFn)A(0x0049f310);
static const PlaySoundFn PlaySoundPitched = (PlaySoundFn)A(0x004a11e0);
static const KeyPressedNowFn KeyPressedNow = (KeyPressedNowFn)A(0x004166c0);

// Help lines of the original screen ("AB to select CD to change", key names, "jump accept esc go back", key names).
// The other versions write them as four fixed texts ("AB per selezionare", "CD per modificare"...): their own four.
#define kHelpSelect ((const char *)A(0x004ff5ac))
#define kHelpKeys1 ((const char *)A(0x00837fa8))
#define kHelpAccept ((const char *)A(0x004ff5c8))
#define kHelpKeys2 ((const char *)A(0x00837fac))

enum { PAD_START = 0x0008, PAD_UP = 0x0010, PAD_RIGHT = 0x0020, PAD_DOWN = 0x0040, PAD_LEFT = 0x0080,
       PAD_TRIANGLE = 0x1000 };

extern "C" void DrawDistanceApply(); // patches/graphics/drawdistance.cpp
extern "C" void EnemiesApply();      // patches/gameplay/enemies.cpp

// A line of PATCH OPTIONS. Toggles change a bool of g_config; the others have their own get / set.
struct Option {
	TextId name;
	int count;                    // number of values (0: countOf)
	TextId values[3];
	bool *flag;                   // toggle: the g_config field
	bool inverted;                // toggle: the first value is "true" (the game's behaviour, shown first)
	void (*apply)();              // toggle: run after a change
	int (*get)();
	void (*set)(int);
	int (*countOf)();             // list known only when running (resolutions), named by valueOf
	const char *(*valueOf)(int);
};

enum { NORMAL = false, INVERTED = true };

static Option Toggle(TextId name, TextId off, TextId on, bool *flag, bool inverted = NORMAL, void (*apply)() = nullptr)
{
	Option o = {name, 2, {off, on}};
	o.flag = flag;
	o.inverted = inverted;
	o.apply = apply;
	return o;
}

static Option Choice(TextId name, int count, const TextId (&values)[3], int (*get)(), void (*set)(int))
{
	Option o = {name, count, {values[0], values[1], values[2]}};
	o.get = get;
	o.set = set;
	return o;
}

static Option List(TextId name, int (*get)(), void (*set)(int), int (*countOf)(), const char *(*valueOf)(int))
{
	Option o = {name};
	o.get = get;
	o.set = set;
	o.countOf = countOf;
	o.valueOf = valueOf;
	return o;
}

static int ValueOf(const Option &o)
{
	if (o.flag) {
		return *o.flag != o.inverted;
	}
	return o.get();
}

static void SetValue(const Option &o, int v)
{
	if (o.flag) {
		*o.flag = (v != 0) != o.inverted;
		if (o.apply) {
			o.apply();
		}
	} else {
		o.set(v);
	}
}

static int CountOf(const Option &o)
{
	return o.countOf ? o.countOf() : o.count;
}

int DisplayModes(int (*out)[2], int max); // dx6_wrapper/ddraw_objs.cpp: the desktop's first

// PATCH OPTIONS > resolution: the size the game is started with (patches/system/modeselect.c), from our mode list; it
// takes effect at the next start ("restart" shows until then). The first one is the desktop's (kept as 0 x 0).
static int g_modes[32][2], g_modeCount;
static int ModeCount()
{
	if (!g_modeCount) {
		g_modeCount = DisplayModes(g_modes, 32);
	}
	return g_modeCount;
}
static int ModeIndex()
{
	for (int i = 1; i < ModeCount(); i++) {
		if (g_modes[i][0] == g_config.resWidth && g_modes[i][1] == g_config.resHeight) {
			return i;
		}
	}
	return 0;
}
static const char *ModeName(int i)
{
	static char text[40];
	ModeCount();
	bool pending = g_modes[i][0] != g_render.Width() || g_modes[i][1] != g_render.Height();
	wsprintfA(text, pending ? "%dx%d %s" : "%dx%d", g_modes[i][0], g_modes[i][1], Tr(TXT_RESTART));
	// The zero of the digit sheet (0x60) is not a zero on PC: the letter o looks like one (lower case: in that font
	// the capitals are symbols, such as the A and B arrows). The digits' size: see InstallMenu.
	for (char *c = text; *c; c++) {
		if (*c == '0') {
			*c = 'o';
		}
	}
	return text;
}
static void SetMode(int i)
{
	g_config.resWidth = i ? g_modes[i][0] : 0;
	g_config.resHeight = i ? g_modes[i][1] : 0;
}

static int GetDisplay() { return g_config.windowed; }
static void SetDisplay(int v)
{
	g_config.windowed = v != 0;
	g_render.ApplyWindowMode();
}

// The game's two control schemes (g_gameOptions bit 0x40: 1 directional, 0 rotational) plus ours, on directional
static int GetControls()
{
	if (!(g_gameOptions & 0x40)) {
		return 2;
	}
	return g_config.modernControls ? 0 : 1;
}
static void SetControls(int v)
{
	g_config.modernControls = v == 0;
	g_gameOptions = (unsigned char)(v == 2 ? g_gameOptions & ~0x40 : g_gameOptions | 0x40);
}

static int GetCameraSpeed() { return g_config.cameraSpeed; }
static void SetCameraSpeed(int v) { g_config.cameraSpeed = v; }

// Values: "off" / "on" for what the game does not have; "original" first for what it does, then ours
static const Option kOptions[] = {
	List(TXT_LANGUAGE, LanguageChoice, SetLanguageChoice, LanguageChoiceCount, LanguageChoiceName),
	Choice(TXT_DISPLAY, 2, {TXT_FULLSCREEN, TXT_WINDOW}, GetDisplay, SetDisplay),
	List(TXT_RESOLUTION, ModeIndex, SetMode, ModeCount, ModeName),
	Choice(TXT_CONTROLS, 3, {TXT_MODERN, TXT_DIRECTIONAL, TXT_ROTATIONAL}, GetControls, SetControls),
	Toggle(TXT_AUTO_AIM, TXT_OFF, TXT_ON, &g_config.autoAim),
	Choice(TXT_CAMERA_SPEED, 3, {TXT_SLOW, TXT_NORMAL, TXT_FAST}, GetCameraSpeed, SetCameraSpeed),
	Toggle(TXT_INVERT_LOOK, TXT_OFF, TXT_ON, &g_config.invertLook),
	Toggle(TXT_WEAPON_INFO, TXT_OFF, TXT_ON, &g_config.weaponInfo),
	Toggle(TXT_AUTOSAVE, TXT_OFF, TXT_ON, &g_config.autoSave),
	Toggle(TXT_GOAL_INFO, TXT_OFF, TXT_ON, &g_config.goalInfo),
	Toggle(TXT_DRAW_DISTANCE, TXT_ORIGINAL, TXT_FAR, &g_config.drawDistanceFar, NORMAL, DrawDistanceApply),
	Toggle(TXT_PLANET_FOG, TXT_ORIGINAL, TXT_ACTIVATED, &g_config.fogNear),
	Toggle(TXT_SEE_THROUGH, TXT_OFF, TXT_ON, &g_config.seeThrough),
	Toggle(TXT_BOSS_SCENE, TXT_ORIGINAL, TXT_ALWAYS, &g_config.bossScene),
	Toggle(TXT_ENEMY_LEVEL_HP, TXT_ORIGINAL, TXT_OFF, &g_config.enemyLevelHealth, INVERTED, EnemiesApply),
	Toggle(TXT_ENEMY_IFRAMES, TXT_ORIGINAL, TXT_OFF, &g_config.enemyHitGuard, INVERTED),
	Toggle(TXT_PRICE_BLINK, TXT_OFF, TXT_ON, &g_config.priceBlink),
	Toggle(TXT_FAIR_MINES, TXT_OFF, TXT_ON, &g_config.fairMines, NORMAL, EnemiesApply),
	Toggle(TXT_TURRET_WRECK, TXT_ORIGINAL, TXT_HARMLESS, &g_config.wreckHarmless, NORMAL, EnemiesApply),
	Toggle(TXT_VEHICLE_AIM, TXT_ORIGINAL, TXT_CAMERA, &g_config.vehicleAim),
	Toggle(TXT_VEHICLE_CAMERA, TXT_FREE, TXT_STEER, &g_config.vehicleSteer),
	Toggle(TXT_BATHYOS_CAMERA, TXT_ORIGINAL, TXT_HIGH, &g_config.bathyosCamera),
};
static const int kCount = sizeof kOptions / sizeof kOptions[0];

static const char *kPatchOptions() { return Tr(TXT_PATCH_OPTIONS); }

// The list scrolls like the original control configuration screen: a window of kVisible lines that follows the
// cursor, with blinking arrows (the font's A and B glyphs) when there is more above or below
static const int kVisible = 5, kTop = 0x4c, kStep = 0x14;

static int g_cursor, g_first, g_repeat, g_blink;
static unsigned short g_prevButtons;

static void Click() { PlaySoundPitched(8, 0x1200, 0x50, 0x50); } // the original screen's sound

// The settings as they were when the screen opened, for going back without keeping the changes
static Config g_before;
static unsigned char g_gameOptionsBefore;
static bool g_open, g_shownThisFrame;

static void UndoChanges()
{
	bool window = g_config.windowed != g_before.windowed;
	g_config = g_before;
	g_gameOptions = g_gameOptionsBefore;
	if (window) {
		g_render.ApplyWindowMode();
	}
	DrawDistanceApply();
	EnemiesApply();
}

// Replaces ConfigureGraphicsMenu: draws one frame of the screen and reads the pad; 1 = close
static int __cdecl PatchOptionsMenu()
{
	if (!g_open) {
		g_open = true;
		g_before = g_config;
		g_gameOptionsBefore = g_gameOptions;
	}
	g_shownThisFrame = true;
	DrawGraphicsMenuText(0x30, kPatchOptions(), 0);
	g_blink -= g_frameStep;
	if (g_blink < 0) {
		g_blink += 20;
	}
	if (g_cursor < g_first) {
		g_first = g_cursor;
	} else if (g_cursor >= g_first + kVisible) {
		g_first = g_cursor - kVisible + 1;
	}
	for (int i = g_first; i < kCount && i < g_first + kVisible; i++) {
		if (i == g_cursor && g_blink >= 10) {
			continue; // the selected item blinks, as in the original screen
		}
		char line[64];
		const Option &o = kOptions[i];
		wsprintfA(line, "%s %s", Tr(o.name), o.valueOf ? o.valueOf(ValueOf(o)) : Tr(o.values[ValueOf(o)]));
		DrawGraphicsMenuText(kTop + (i - g_first) * kStep, line, 0);
	}
	if (g_blink >= 10) {
		if (g_first > 0) {
			DrawMenuText(0x100, kTop - 0xc, "A", 0, 1, 0x600);
		}
		if (g_first + kVisible < kCount) {
			DrawMenuText(0x100, kTop + kVisible * kStep - 0x4, "B", 0, 1, 0x600);
		}
	}
	DrawMenuText(0x100, 0xb6, kHelpSelect, 0, 1, 0x600);
	DrawMenuText(0x100, 0xc2, kHelpKeys1, 0, 1, 0x600);
	DrawMenuText(0x100, 0xce, kHelpAccept, 0, 1, 0x600);
	DrawMenuText(0x100, 0xda, kHelpKeys2, 0, 1, 0x600);

	unsigned short pad = g_padButtons, pressed = pad & ~g_prevButtons;
	g_prevButtons = pad;
	if (g_repeat > 0) {
		g_repeat -= g_frameStep;
	} else {
		const Option &o = kOptions[g_cursor];
		int v = ValueOf(o);
		if (pad & PAD_UP) {
			g_cursor = (g_cursor + kCount - 1) % kCount;
			g_repeat = 15;
			Click();
		} else if (pad & PAD_DOWN) {
			g_cursor = (g_cursor + 1) % kCount;
			g_repeat = 15;
			Click();
		} else if ((pad & PAD_LEFT) && v > 0) {
			SetValue(o, v - 1);
			g_repeat = 15;
			Click();
		} else if ((pad & PAD_RIGHT) && v < CountOf(o) - 1) {
			SetValue(o, v + 1);
			g_repeat = 15;
			Click();
		}
	}
	bool cancel = KeyPressedNow(1) || (pressed & PAD_TRIANGLE), accept = KeyPressedNow(0x1c) || (pressed & PAD_START);
	if (cancel || accept) {
		if (cancel) {
			UndoChanges();
		} else {
			ConfigSave();
		}
		g_open = false;
		g_prevButtons = 0xffff; // a button still held when the screen opens again is not a new press
		return 1;
	}
	return 0;
}

// End of each frame: the screen was open and nobody called it, so LevelSelectMenu closed it with jump: accepted
static void MenuClosedByJump()
{
	if (g_open && !g_shownThisFrame) {
		g_open = false;
		ConfigSave();
	}
	g_shownThisFrame = false;
}

// ---------------------------------------------------------------- pause menu > camera mode
// A third entry, "modern" (cursor 2), after rotational (0) and directional (1): the submenu gets 3 entries
// (004f80d0[1]), we draw the line, and accepting it is directional for the game plus our controls on.
#define g_paused (*(short *)A(0x00531678))
#define g_pauseMenuShown (*(int *)A(0x00541068))   // DrawHud draws the pause menu while this is not 0
#define g_pauseSubmenu (*(short *)A(0x0052f610))
#define g_pauseCursor (*(short *)A(0x00531644))
#define g_pauseBlink (*(short *)A(0x0052b828))     // DrawHud's highlight pulse (0..0x3f)
#define g_padButtonsPrev (*(unsigned short *)A(0x00889954))

typedef void(__cdecl *DrawCenteredTextFn)(int y, const char *text, int r, int g, int b, int hiRes);
static const DrawCenteredTextFn DrawCenteredText = (DrawCenteredTextFn)A(0x00405a00); // one centred line
static void (*g_pauseMenuOriginal)();

enum { PAD_CROSS = 0x4000, PAUSE_MAIN = 0, PAUSE_CAMERA_MODE = 1, PAUSE_EXIT_LEVEL = 3, CURSOR_MODERN = 2 };

// ---------------------------------------------------------------- pause menu > restart level
// A fifth line before exit level. The game keeps 4 entries (DrawHud's colour table has 4 slots): the cursor is ours
// (g_mainCursor), the game's follows it, and we draw the lines (PauseMainLine). Restart is "exit level? yes" for the
// game, then GameMain's RunMissionMenu call returns "play" so the same level starts again.
enum { MAIN_CONTINUE, MAIN_CAMERA, MAIN_VOLUME, MAIN_RESTART, MAIN_EXIT, MAIN_COUNT };
static const int kMainY[MAIN_COUNT] = {0x64, 0x72, 0x80, 0x8e, 0x9c};
static bool g_restartLevel;
static int g_mainCursor;
static short g_cursorLeft = -1; // the game's cursor as we left it

typedef void(__cdecl *PlaySfxFn)(int sfx, int *pos);
static const PlaySfxFn PlaySfx = (PlaySfxFn)A(0x00432c50);
enum { SFX_CURDOWN = 2 };

// The line colour of the pause menu: yellow, the selected one pulsing as in DrawHud
static void PauseLineColour(bool selected, int *r, int *g)
{
	*r = *g = 0x80;
	if (selected) {
		int v = g_pauseBlink & 0x3f;
		if (v > 0x1f) {
			v = 0x3f - v;
		}
		*r = *g = v * 2 + 0x40;
	}
}

typedef int(__cdecl *RunMissionMenuFn)();
static const RunMissionMenuFn RunMissionMenu = (RunMissionMenuFn)A(0x00458190);
static int __cdecl RunMissionMenu_Restart()
{
	if (g_restartLevel) {
		g_restartLevel = false;
		return 0; // "play": the loop starts the level of g_levelSlot in g_gameMode, both unchanged
	}
	return RunMissionMenu();
}

typedef void(__cdecl *DrawBoxFn)(int x, int y, int w, int h, int a, int b, int c);
static const DrawBoxFn DrawPanelBox = (DrawBoxFn)A(0x004055f0); // w and h in 1/4096
static void __cdecl PauseBox(int x, int y, int w, int h, int a, int b, int c)
{
	if (g_pauseSubmenu == PAUSE_MAIN) {
		h += 0xe000; // one line more (the lines are 0xe apart)
	}
	// 0x18 wider, still centred: the Spanish "control de volumen" is wider than the box (0xd8)
	x -= 0xc;
	w += 0x18000;
	DrawPanelBox(x, y, w, h, a, b, c);
}

// Drawn by DrawHud's pause box hook (patches/system/widescreen.cpp), right before the box: same colours as the game's lines
extern "C" void PauseMenuExtras(void)
{
	if (!g_paused || !g_pauseMenuShown) {
		return;
	}
	int r, g;
	if (g_pauseSubmenu == PAUSE_CAMERA_MODE) {
		PauseLineColour(g_pauseCursor == CURSOR_MODERN, &r, &g);
		DrawCenteredText(0x92, Tr(TXT_PAUSE_MODERN), r, g, 0, 0);
	} else if (g_pauseSubmenu == PAUSE_MAIN) {
		PauseLineColour(g_mainCursor == MAIN_RESTART, &r, &g);
		DrawCenteredText(kMainY[MAIN_RESTART], Tr(TXT_RESTART_LEVEL), r, g, 0, 0);
	}
}

// DrawHud's 4 lines of the pause menu (continue, camera mode, volume control, exit level), told apart by the call
static void __cdecl PauseMainLine(int y, const char *text, int r, int g, int b, int hiRes)
{
	DWORD from = (DWORD)_ReturnAddress();
	int item = from == A(0x00402c80) ? MAIN_CONTINUE : from == A(0x00402c91) ? MAIN_CAMERA
	         : from == A(0x00402cba) ? MAIN_VOLUME : MAIN_EXIT;
	int rr, gg;
	PauseLineColour(g_mainCursor == item, &rr, &gg);
	if (item == MAIN_CAMERA) {
		text = Tr(TXT_CONTROL_OPTIONS); // the game's "camera mode": it chooses the controls now
	}
	DrawCenteredText(kMainY[item], text, rr, gg, 0, hiRes);
}

// UpdatePauseMenu (00433330), once per paused frame: catch the accept in camera mode before the game handles it
static void UpdatePauseMenu_Modern()
{
	bool accept = (g_padButtons & PAD_CROSS) && !(g_padButtonsPrev & PAD_CROSS);
	if (g_pauseSubmenu == PAUSE_CAMERA_MODE && accept) {
		g_config.modernControls = g_pauseCursor == CURSOR_MODERN;
		if (g_pauseCursor == CURSOR_MODERN) {
			g_pauseCursor = 1; // directional, for the game
		}
		ConfigSave();
	}
	if (g_pauseSubmenu == PAUSE_MAIN) {
		if (g_pauseCursor != g_cursorLeft) { // the game set it: pause opened, back from a submenu, Esc
			g_mainCursor = g_pauseCursor == 3 ? MAIN_EXIT : g_pauseCursor;
		}
		static unsigned short prev; // ours: the game's previous-frame copy loses the bits taken below
		unsigned short pressed = g_padButtons & ~prev;
		prev = g_padButtons;
		int move = (pressed & PAD_DOWN) ? 1 : (pressed & PAD_UP) ? -1 : 0;
		if (move && g_mainCursor + move >= 0 && g_mainCursor + move < MAIN_COUNT) {
			g_mainCursor += move;
			PlaySfx(SFX_CURDOWN, nullptr);
			g_pauseMenuShown = 0xec4; // as the game does on a move
		}
		g_padButtons &= ~(PAD_UP | PAD_DOWN);
		g_pauseCursor = g_cursorLeft = (short)(g_mainCursor >= MAIN_RESTART ? 3 : g_mainCursor);
		if (accept && g_mainCursor == MAIN_RESTART) {
			g_restartLevel = true;
			g_pauseSubmenu = PAUSE_EXIT_LEVEL; // "exit level? yes"
			g_pauseCursor = 1;
		}
	}
	g_pauseMenuOriginal();
	if (g_pauseSubmenu != PAUSE_MAIN) {
		g_cursorLeft = -1;
	}
}

static bool g_titlePatched;  // the pause's control options title (DrawHud push at 00402cf5)

// After a change of language (language.cpp): our texts the game keeps a pointer to
void RefreshPatchTexts()
{
	if (g_titlePatched) {
		const char *text = Tr(TXT_CONTROL_OPTIONS);
		DWORD old;
		if (VirtualProtect((void *)A(0x00402cf6), 4, PAGE_EXECUTE_READWRITE, &old)) {
			*(const char **)A(0x00402cf6) = text;
			VirtualProtect((void *)A(0x00402cf6), 4, old, &old);
		}
	}
	*(const char **)A(0x004f9dc0) = kPatchOptions();
}

extern "C" void InstallMenu(void)
{
	g_render.AddBeforePresent(MenuClosedByJump);
	// DrawGraphicsMenuText draws the digits (sheet 0x60) at scale 0x1000, twice the letters' 0x800, with the same
	// advance of 10: on PC those frames are big and overlap (resolution line). 0xc00 makes them about letter size.
	static const unsigned char digitScale[] = {0xbf, 0x00, 0x10, 0x00, 0x00}, digitSmaller[] = {0xbf, 0x00, 0x0c, 0x00, 0x00};
	PatchBytes(A(0x0049eaa4), digitScale, digitSmaller, 5, "menu digits size (DrawGraphicsMenuText)");
	static const unsigned char pauseBytes[] = {0x51, 0xa0, 0x5c, 0x99, 0x88, 0x00}; // push ecx; mov al, [0088995c]
	static const unsigned char twoItems[] = {2}, threeItems[] = {3};
	g_pauseMenuOriginal = (void (*)())HookTrampoline(A(0x00433330), (const void *)UpdatePauseMenu_Modern, pauseBytes, 6,
	                                                 "pause camera mode: modern (UpdatePauseMenu)");
	if (g_pauseMenuOriginal) {
		PatchBytes(A(0x004f80d1), twoItems, threeItems, 1, "pause camera mode: 3 entries");
	}

	static const unsigned char missionCall[] = {0xe8, 0xeb, 0x5d, 0x02, 0x00}; // GameMain: call RunMissionMenu
	static const unsigned char boxCall[] = {0xe8, 0x46, 0x27, 0x00, 0x00};     // DrawHud: call DrawPanelBox
	unsigned char call[5] = {0xe8};
	*(DWORD *)(call + 1) = (DWORD)RunMissionMenu_Restart - (A(0x004323a0) + 5);
	if (g_pauseMenuOriginal && PatchBytes(A(0x004323a0), missionCall, call, 5, "pause restart level (GameMain)")) {
		*(DWORD *)(call + 1) = (DWORD)PauseBox - (A(0x00402ea5) + 5);
		PatchBytes(A(0x00402ea5), boxCall, call, 5, "pause box size (DrawHud)");
		// Title of the camera mode screen: push "camera mode" (translated buzz.exe: its .lang copy), then DrawCenteredText
		const DWORD title = A(0x00402cf5);
		if (*(unsigned char *)title == 0x68 && *(unsigned char *)(title + 7) == 0xe8) {
			const char *text = Tr(TXT_CONTROL_OPTIONS);
			g_titlePatched = PatchBytes(title + 1, (const unsigned char *)(title + 1), (const unsigned char *)&text, 4,
			                            "pause control options title (DrawHud)");
		}
		static const DWORD lines[] = {A(0x00402c7b), A(0x00402c8c), A(0x00402cb5), A(0x00402cd5)}; // DrawHud: call DrawCenteredText
		for (DWORD at : lines) {
			unsigned char expect[5] = {0xe8};
			*(DWORD *)(expect + 1) = A(0x00405a00) - (at + 5);
			*(DWORD *)(call + 1) = (DWORD)PauseMainLine - (at + 5);
			PatchBytes(at, expect, call, 5, "pause menu lines (DrawHud)");
		}
	}

	static const unsigned char menuBytes[] = {0x53, 0x55, 0x56, 0x57, 0x6a}; // push ebx/ebp/esi/edi; push 0
	if (!HookReplace(A(0x004a0280), (const void *)PatchOptionsMenu, menuBytes, 5, "patch options menu")) {
		return;
	}
	// The entry's name: second pointer of the options list (004f9dbc), "configure graphics" or its translation
	*(const char **)A(0x004f9dc0) = kPatchOptions();
	LogLine("ok patch options menu name");
}
