// Control configuration screen, rebuilt (replaces ConfigureControlsMenu 0049fa70, keyboard only): every row (the
// game's actions, walking and looking) with a keyboard key, a mouse input and a pad input side by side. Keys of the
// game's rows are the game's key map (g_keyMap, saved by SaveSlot 99); the rest is ours (g_config.lookKeys,
// mouseBind, padBind; patches/input/gamepad.cpp and controls.cpp use them). Giving an input that another row has swaps
// them. LevelSelectMenu closes the screen on jump, which here means "change": the press is hidden from it.
#include "common.h"
#include "config.h"
#include "texts.h"
#include "gamepad.h"
#include "mouse.h"

#define g_padButtons (*(unsigned short *)A(0x0088995c))
#define g_frameStep (*(int *)A(0x00535be4))

struct KeyBind { int key, control; };
#define g_keyMap ((KeyBind *)A(0x0052b534)) // 38 pairs; key -1 = empty
static const int kKeyMapSize = 38;

typedef void(__cdecl *DrawMenuTextFn)(int x, int y, const char *text, int a, int align, int scale);
typedef int(__cdecl *ReadPressedKeyFn)();
typedef const char *(__cdecl *GetControlNameFn)(int key);
typedef void(__cdecl *BindKeyFn)(int key, int control);
typedef void(__cdecl *SaveSlotFn)(int slot, const char *name);
typedef void(__cdecl *PlaySoundFn)(int sound, int pitch, int volL, int volR);
static const DrawMenuTextFn DrawMenuText = (DrawMenuTextFn)A(0x0049f310);
static const ReadPressedKeyFn ReadPressedKey = (ReadPressedKeyFn)A(0x004156f0);
static const GetControlNameFn GetKeyName = (GetControlNameFn)A(0x00415870);
static const BindKeyFn BindKey = (BindKeyFn)A(0x00415750);
static const SaveSlotFn SaveSlot = (SaveSlotFn)A(0x0049d8f0);
static const PlaySoundFn PlaySoundPitched = (PlaySoundFn)A(0x004a11e0);
#define kTitle (*(const char **)A(0x004f9dbc)) // the options list entry: "configure control" in the game's language

enum { PAD_SELECT = 0x0001, PAD_START = 0x0008, PAD_UP = 0x0010, PAD_RIGHT = 0x0020, PAD_DOWN = 0x0040,
       PAD_LEFT = 0x0080, PAD_TRIANGLE = 0x1000, PAD_CROSS = 0x4000 };
enum { ALIGN_LEFT, ALIGN_CENTRE, ALIGN_RIGHT };
enum { KEY_ESC = 1, KEY_BACKSPACE = 0x0e, KEY_ENTER = 0x1c, NO_INPUT = -1, ANY_PAD = 0x800 }; // ReadPressedKey
enum { COL_KEY, COL_MOUSE, COL_PAD, COL_COUNT };

struct Action {
	int control;    // the game's control bit; 0 for the look rows (ours)
	TextId name;
	int defaultKey; // InitControls; 0 none
};
// Our defaults: W A S D to walk, Q E to side-step (the arrows still walk: they are fixed menu keys)
static const Action kActions[BIND_COUNT] = { // Bind order
	{0x0010, TXT_UP, 0x11},         {0x0040, TXT_DOWN, 0x1f},        {0x0080, TXT_LEFT, 0x1e},
	{0x0020, TXT_RIGHT, 0x20},      {0x4000, TXT_JUMP, 0x39},        {0x8000, TXT_FIRE, 0x1d},
	{0x2000, TXT_CHANGE_WEAPON, 0x2a}, {0x0400, TXT_STRAFE_LEFT, 0x10}, {0x0800, TXT_STRAFE_RIGHT, 0x12},
	{0x1000, TXT_CANCEL, 0x2d},     {0, TXT_LOOK_LEFT, 0},           {0, TXT_LOOK_RIGHT, 0},
	{0, TXT_LOOK_UP, 0},            {0, TXT_LOOK_DOWN, 0},
};
// The game's own defaults (InitControls), game rows only: arrows, space, ctrl, shift, tab, Q, X
static const int kGameDefaultKeys[BIND_FIRST_LOOK] = {0xc8, 0xd0, 0xcb, 0xcd, 0x39, 0x1d, 0x2a, 0x0f, 0x10, 0x2d};
static const int kRowDefaults = BIND_COUNT; // last row: "default controls"

static bool IsLook(int row) { return row >= BIND_FIRST_LOOK; }

// Menu keys the original screen always puts back (ConfigureControlsMenu): never an action's own key
static const KeyBind kFixed[] = {{1, 0x1000}, {0xc8, 0x10}, {0xd0, 0x40}, {0xcb, 0x80}, {0xcd, 0x20},
                                 {0x3b, 0x4000}, {0x3c, 8}, {0x1c, 8}};

static bool IsFixed(const KeyBind &b)
{
	for (const KeyBind &f : kFixed) {
		if (f.key == b.key && f.control == b.control) {
			return true;
		}
	}
	return false;
}

static bool IsKeyboard(int key) { return key >= 0 && key < 0x400; }

// The game action's own key (not a fixed one), or -1
static int OwnKey(int control)
{
	for (int i = 0; i < kKeyMapSize; i++) {
		if (g_keyMap[i].control == control && IsKeyboard(g_keyMap[i].key) && !IsFixed(g_keyMap[i])) {
			return g_keyMap[i].key;
		}
	}
	return -1;
}

// Gives the game action this key as its own (-1: none); the key stops doing anything else, fixed keys aside
static void SetOwnKey(int control, int key)
{
	for (int i = 0; i < kKeyMapSize; i++) {
		KeyBind &b = g_keyMap[i];
		if (b.key == -1 || IsFixed(b)) {
			continue;
		}
		if ((b.control == control && IsKeyboard(b.key)) || b.key == key) {
			b.key = -1;
			b.control = 0;
		}
	}
	// The 38 slots come full of repeated pairs (arrows twice, fixed keys added again): drop repeats so a key fits
	for (int i = 0; i < kKeyMapSize; i++) {
		for (int j = i + 1; j < kKeyMapSize; j++) {
			if (g_keyMap[i].key != -1 && g_keyMap[j].key == g_keyMap[i].key &&
			    g_keyMap[j].control == g_keyMap[i].control) {
				g_keyMap[j].key = -1;
				g_keyMap[j].control = 0;
			}
		}
	}
	if (key >= 0) {
		BindKey(key, control); // also packs the list
	}
	if (key >= 0 && OwnKey(control) != key) {
		LogLine("ERROR controls: key map full, key %d not given", key);
	}
}

// A row's own key, or -1
static int RowKey(int row)
{
	if (IsLook(row)) {
		int k = g_config.lookKeys[row - BIND_FIRST_LOOK];
		return k ? k : -1;
	}
	return OwnKey(kActions[row].control);
}

static void SetRowKey(int row, int key)
{
	if (IsLook(row)) {
		g_config.lookKeys[row - BIND_FIRST_LOOK] = (unsigned char)(key > 0 ? key : 0);
	} else {
		SetOwnKey(kActions[row].control, key);
	}
}

// What to show: its own key, else (game rows) its fixed one
static int ShownKey(int row)
{
	int own = RowKey(row);
	if (own >= 0 || IsLook(row)) {
		return own;
	}
	for (int i = 0; i < kKeyMapSize; i++) {
		if (g_keyMap[i].control == kActions[row].control && IsKeyboard(g_keyMap[i].key)) {
			return g_keyMap[i].key;
		}
	}
	return -1;
}

static void SaveAll()
{
	SaveSlot(99, nullptr);
	ConfigSave();
}

// Gives the row this key (-1: none); a row that had it gets this row's old key
static void AssignKey(int row, int key)
{
	int old = RowKey(row);
	for (int j = 0; j < BIND_COUNT; j++) {
		if (j != row && key >= 0 && RowKey(j) == key) {
			SetRowKey(j, old);
		}
	}
	SetRowKey(row, key);
	SaveAll();
}

// Mouse movement only for the look rows: a row that cannot take the old input is left without one
static bool MouseFits(int row, int input) { return input < MI_FIRST_MOVE || IsLook(row); }

static void AssignMouse(int row, int input)
{
	unsigned char *m = g_config.mouseBind;
	int old = m[row];
	for (int j = 0; j < BIND_COUNT; j++) {
		if (j != row && input != MI_NONE && m[j] == input) {
			m[j] = (unsigned char)(old != MI_NONE && MouseFits(j, old) ? old : MI_NONE);
		}
	}
	m[row] = (unsigned char)input;
	SaveAll();
}

static void AssignPad(int row, int input)
{
	unsigned char *p = g_config.padBind;
	int old = p[row];
	for (int j = 0; j < BIND_COUNT; j++) {
		if (j != row && input != PI_NONE && p[j] == input) {
			p[j] = (unsigned char)old;
		}
	}
	p[row] = (unsigned char)input;
	SaveAll();
}

static void SetDefaultKeys()
{
	for (int i = 0; i < BIND_FIRST_LOOK; i++) {
		SetOwnKey(kActions[i].control, -1);
	}
	for (int i = 0; i < BIND_FIRST_LOOK; i++) {
		if (!IsFixed({kActions[i].defaultKey, kActions[i].control})) {
			SetOwnKey(kActions[i].control, kActions[i].defaultKey);
		}
	}
}

// First start with the patch: our default keys, unless the player had already changed the game's
void ApplyOurDefaultKeys()
{
	for (int i = 0; i < BIND_FIRST_LOOK; i++) {
		KeyBind original = {kGameDefaultKeys[i], kActions[i].control};
		if (OwnKey(kActions[i].control) != (IsFixed(original) ? -1 : original.key)) {
			LogLine("controls: the player's own keys kept");
			return;
		}
	}
	SetDefaultKeys();
	LogLine("controls: default keys W A S D");
}

static void ResetDefaults()
{
	SetDefaultKeys();
	static const Config kDefault = Config();
	memcpy(g_config.lookKeys, kDefault.lookKeys, sizeof kDefault.lookKeys);
	memcpy(g_config.mouseBind, kDefault.mouseBind, sizeof kDefault.mouseBind);
	memcpy(g_config.padBind, kDefault.padBind, sizeof kDefault.padBind);
	SaveAll();
}

// Keys a row cannot take: esc, enter, backspace, the arrows and the function keys belong to the menus (as in the
// original; backspace clears)
static bool KeyAllowed(int key)
{
	if (key == KEY_ESC || key == KEY_ENTER || key == KEY_BACKSPACE || key == 0xc8 || key == 0xd0 || key == 0xcb ||
	    key == 0xcd) {
		return false;
	}
	if ((key >= 0x3b && key <= 0x44) || key == 0x57 || key == 0x58 || (key >= 0x64 && key <= 0x66)) {
		return false;
	}
	return IsKeyboard(key);
}

// Names as the game's font writes them (lower case: its capitals are symbols)
static const char *PadName(int input)
{
	static const char *const kButtons[PI_LS_LEFT] = {"a", "b", "x", "y", "lb", "rb", "lt", "rt", "ls", "rs"};
	static const TextId kDirs[4] = {TXT_LEFT, TXT_RIGHT, TXT_DOWN, TXT_UP};
	static char name[48];
	if (input < PI_LS_LEFT) {
		return kButtons[input];
	}
	if (input >= PI_COUNT) {
		return "-";
	}
	int i = input - PI_LS_LEFT;
	wsprintfA(name, "%s %s", i < 4 ? "ls" : "rs", Tr(kDirs[i % 4]));
	return name;
}

static const char *MouseName(int input)
{
	static const TextId kNames[MI_COUNT] = {TXT_MOUSE_LEFT, TXT_MOUSE_RIGHT, TXT_MOUSE_MIDDLE, TXT_MOUSE_4,
	                                        TXT_MOUSE_5,    TXT_WHEEL_UP,    TXT_WHEEL_DOWN,   TXT_LEFT,
	                                        TXT_RIGHT,      TXT_UP,          TXT_DOWN};
	return input < MI_COUNT ? Tr(kNames[input]) : "-";
}

static void Click() { PlaySoundPitched(8, 0x1200, 0x50, 0x50); } // the original screen's sound

enum { WAIT_NONE, WAIT_RELEASE, WAIT_INPUT };
static int g_row, g_first, g_column, g_repeat, g_blink, g_wait;
static unsigned short g_prevButtons = 0xffff;

// Layout on the 512 x 256 menu screen; the rows scroll in a window of kVisible
static const int kNameRight = 0xd0, kColumnX[COL_COUNT] = {0x108, 0x160, 0x1c0};
static const int kHeaderY = 0x48, kFirstRowY = 0x56, kRowStep = 0xb, kVisible = 10, kDefaultsY = 0xc8;
static const int kHelpY = 0xd8, kArrowX = 0x18;
static const int kSmall = 0x600, kBig = 0x800;
static const TextId kHeaders[COL_COUNT] = {TXT_KEYBOARD, TXT_MOUSE, TXT_PAD};

static const char *CellText(int row, int column)
{
	if (column == COL_KEY) {
		int key = ShownKey(row);
		const char *name = key >= 0 ? GetKeyName(key) : nullptr;
		return name ? name : "-";
	}
	return column == COL_MOUSE ? MouseName(g_config.mouseBind[row]) : PadName(g_config.padBind[row]);
}

static void Draw()
{
	DrawMenuText(0x100, 0x30, kTitle, 0, ALIGN_CENTRE, kBig);
	for (int c = 0; c < COL_COUNT; c++) {
		DrawMenuText(kColumnX[c], kHeaderY, Tr(kHeaders[c]), 0, ALIGN_CENTRE, kSmall);
	}
	bool blinkOff = g_blink >= 10;
	if (g_row < kRowDefaults) {
		if (g_row < g_first) {
			g_first = g_row;
		} else if (g_row >= g_first + kVisible) {
			g_first = g_row - kVisible + 1;
		}
	}
	for (int i = g_first; i < BIND_COUNT && i < g_first + kVisible; i++) {
		int y = kFirstRowY + (i - g_first) * kRowStep;
		DrawMenuText(kNameRight, y, Tr(kActions[i].name), 0, ALIGN_RIGHT, kSmall);
		for (int c = 0; c < COL_COUNT; c++) {
			bool selected = g_row == i && g_column == c;
			if (selected && g_wait) {
				TextId wait = c == COL_KEY ? TXT_PRESS_KEY : c == COL_MOUSE ? TXT_PRESS_MOUSE : TXT_PRESS_BUTTON;
				DrawMenuText(kColumnX[c], y, Tr(wait), 0, ALIGN_CENTRE, kSmall);
			} else if (!(selected && blinkOff)) {
				DrawMenuText(kColumnX[c], y, CellText(i, c), 0, ALIGN_CENTRE, kSmall);
			}
		}
	}
	if (blinkOff) { // the font's A and B are arrows up and down
		if (g_first > 0) {
			DrawMenuText(kArrowX, kFirstRowY, "A", 0, ALIGN_CENTRE, kSmall);
		}
		if (g_first + kVisible < BIND_COUNT) {
			DrawMenuText(kArrowX, kFirstRowY + (kVisible - 1) * kRowStep, "B", 0, ALIGN_CENTRE, kSmall);
		}
	}
	if (!(g_row == kRowDefaults && blinkOff)) {
		DrawMenuText(0x100, kDefaultsY, Tr(TXT_DEFAULT_CONTROLS), 0, ALIGN_CENTRE, kSmall);
	}
	if (g_wait) {
		DrawMenuText(0x100, kHelpY, Tr(TXT_ESC_TO_CANCEL), 0, ALIGN_CENTRE, kSmall);
		DrawMenuText(0x100, kHelpY + 0xc, Tr(TXT_CLEAR_HELP), 0, ALIGN_CENTRE, kSmall);
	} else {
		DrawMenuText(0x100, kHelpY, Tr(TXT_CONTROLS_HELP), 0, ALIGN_CENTRE, kSmall);
		DrawMenuText(0x100, kHelpY + 0xc, Tr(TXT_CANCEL_TO_GO_BACK), 0, ALIGN_CENTRE, kSmall);
	}
}

static void Clear()
{
	if (g_column == COL_KEY) {
		AssignKey(g_row, -1);
	} else if (g_column == COL_MOUSE) {
		AssignMouse(g_row, MI_NONE);
	} else {
		AssignPad(g_row, PI_NONE);
	}
}

// Waiting for the new input of the selected cell; true when done (set, cleared or cancelled)
static bool UpdateWait(unsigned short pressed)
{
	int key = ReadPressedKey();
	int mouse = MouseTakeInput();
	if (g_wait == WAIT_RELEASE) { // the press that started this must be let go first
		if ((key == NO_INPUT || key == ANY_PAD) && !g_padExtra.held) {
			g_wait = WAIT_INPUT;
			MouseForgetInput();
		}
		return false;
	}
	if (key == KEY_ESC || (pressed & PAD_START)) {
		Click();
		return true;
	}
	if (key == KEY_BACKSPACE || (pressed & PAD_SELECT)) {
		Clear();
		Click();
		return true;
	}
	if (g_column == COL_PAD) {
		for (int i = 0; i < PI_COUNT; i++) {
			if (g_padExtra.held & (1u << i)) {
				AssignPad(g_row, i);
				Click();
				return true;
			}
		}
	} else if (g_column == COL_MOUSE) {
		if (mouse != MI_NONE && MouseFits(g_row, mouse)) {
			AssignMouse(g_row, mouse);
			Click();
			return true;
		}
	} else if (key != NO_INPUT && key != ANY_PAD && KeyAllowed(key)) {
		AssignKey(g_row, key);
		Click();
		return true;
	}
	return false;
}

// Replaces ConfigureControlsMenu: one frame of the screen; 1 = close
static int __cdecl ControlsMenu()
{
	g_blink -= g_frameStep;
	if (g_blink < 0) {
		g_blink += 20;
	}
	Draw();

	unsigned short pad = g_padButtons, pressed = pad & ~g_prevButtons;
	g_prevButtons = pad;
	g_padButtons &= ~PAD_CROSS; // LevelSelectMenu would close the screen on it
	if (g_wait) {
		if (UpdateWait(pressed)) {
			g_wait = WAIT_NONE;
			g_prevButtons = 0xffff; // what is still held is not a new press
		}
		return 0;
	}

	if (pressed & PAD_TRIANGLE) {
		Click();
		g_prevButtons = 0xffff; // a button still held when the screen opens again is not a new press
		return 1;
	}
	if (pressed & (PAD_CROSS | PAD_START)) {
		Click();
		if (g_row == kRowDefaults) {
			ResetDefaults();
		} else {
			g_wait = WAIT_RELEASE;
		}
		return 0;
	}
	if (g_repeat > 0) {
		g_repeat -= g_frameStep;
		return 0;
	}
	int move = (pad & PAD_UP) ? -1 : (pad & PAD_DOWN) ? 1 : 0;
	if (move) {
		g_row = (g_row + move + kRowDefaults + 1) % (kRowDefaults + 1);
		g_repeat = 15;
		Click();
	} else if ((pad & PAD_LEFT) && g_column > 0) {
		g_column--;
		g_repeat = 15;
		Click();
	} else if ((pad & PAD_RIGHT) && g_column < COL_COUNT - 1) {
		g_column++;
		g_repeat = 15;
		Click();
	}
	return 0;
}

extern "C" void InstallControlsMenu(void)
{
	static const unsigned char menuBytes[] = {0x83, 0xec, 0x0c, 0xb8, 0x1e}; // sub esp, 0xc; mov eax, 0x1e
	HookReplace(A(0x0049fa70), (const void *)ControlsMenu, menuBytes, 5, "control configuration screen");
}
