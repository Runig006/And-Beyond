// XInput gamepad and the mouse buttons, read every frame after ReadInputDevices (004150c0) into the game's PlayStation
// pad bits and stick; the DirectInput joystick is skipped once an XInput pad has been seen (same controller). Start,
// Back and the D-pad are fixed (menus); every other pad input (buttons, triggers, stick clicks, each stick direction)
// and every mouse button does what the control screen gave it (g_config.padBind / mouseBind): an action, walking, or
// looking (g_padExtra.lookX / lookY, with the look rows' keys). Also: no input while the window is not active, Esc
// picks "continue" in the pause menu.
#include "common.h"
#include "config.h"
#include "gamepad.h"
#include "mouse.h"
#include <xinput.h>

// PlayStation pad bits used by the game (default bindings: InitControls, 004154f0)
enum {
	PAD_SELECT = 0x0001,
	PAD_START = 0x0008,
	PAD_UP = 0x0010,
	PAD_RIGHT = 0x0020,
	PAD_DOWN = 0x0040,
	PAD_LEFT = 0x0080,
	PAD_L2 = 0x0100,
	PAD_R2 = 0x0200,
	PAD_L1 = 0x0400,
	PAD_R1 = 0x0800,
	PAD_TRIANGLE = 0x1000,
	PAD_CIRCLE = 0x2000,
	PAD_CROSS = 0x4000,
	PAD_SQUARE = 0x8000,
};

#define g_inputButtons (*(unsigned short *)A(0x0052b4d8))
#define g_stickX (*(int *)A(0x00739034))
#define g_stickY (*(int *)A(0x00739038))
#define g_joystickAcquired (*(int *)A(0x0052b678)) // first DirectInput joystick ready (InitInput)
#define g_controllerState (*(int *)A(0x00837fc8)) // 1 = pad read this frame (3 shows "insert controller" in pause)

GamepadExtra g_padExtra;

// The game's control bit of each row of the control screen (Bind order; 0 for the look rows)
static const unsigned short kBindControl[BIND_COUNT] = {PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_CROSS, PAD_SQUARE,
                                                        PAD_CIRCLE, PAD_L1, PAD_R1, PAD_TRIANGLE};
static const WORD kPadButtons[PI_RS + 1] = {
	XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_LEFT_SHOULDER,
	XINPUT_GAMEPAD_RIGHT_SHOULDER, 0, 0, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB}; // LT, RT apart

static const int kStickThreshold = 0x2bc0; // the game's own threshold for turning the stick into direction bits
static const float kHeld = 0.5f;           // a stick direction counts as a held button past half way

typedef DWORD(WINAPI *XInputGetStateFn)(DWORD, XINPUT_STATE *);
static XInputGetStateFn g_getState;
static void (*g_readOriginal)();
static int g_pad = -1;         // connected pad slot, -1 when none
static DWORD g_nextSearch;     // looking for a pad in empty slots is slow: at most once a second

// XInput is loaded on first use: loading a DLL while ours is being loaded (DllMain) is not allowed
static bool LoadXInput()
{
	static bool tried;
	if (!tried) {
		tried = true;
		HMODULE xi = LoadLibraryA("xinput1_4.dll");
		if (!xi) {
			xi = LoadLibraryA("xinput9_1_0.dll");
		}
		g_getState = xi ? (XInputGetStateFn)GetProcAddress(xi, "XInputGetState") : nullptr;
		LogLine(g_getState ? "gamepad: XInput ready" : "gamepad: XInput not available");
	}
	return g_getState != nullptr;
}

static bool ReadPad(XINPUT_STATE *s)
{
	if (!LoadXInput()) {
		return false;
	}
	if (g_pad >= 0 && g_getState(g_pad, s) == ERROR_SUCCESS) {
		return true;
	}
	if (g_pad >= 0) {
		LogLine("gamepad %d disconnected", g_pad);
		g_pad = -1;
	}
	if (GetTickCount() < g_nextSearch) {
		return false;
	}
	g_nextSearch = GetTickCount() + 1000;
	for (int i = 0; i < XUSER_MAX_COUNT; i++) {
		if (g_getState(i, s) == ERROR_SUCCESS) {
			g_pad = i;
			LogLine("gamepad %d connected", i);
			return true;
		}
	}
	return false;
}

// Stick axis from XInput (-32768..32767) to the game's range, 0 inside the dead zone
static int Axis(SHORT v, int deadZone)
{
	if (v > -deadZone && v < deadZone) {
		return 0;
	}
	return v < -0x8000 ? -0x8000 : v;
}

#define g_keyboardReady (*(int *)A(0x0052b718))  // DirectInput keyboard ready (InitInput)
#define g_keyState ((unsigned char *)A(0x0052b3d8)) // DirectInput keyboard state, 256 keys
#define g_gameWindow (*(HWND *)A(0x00508784))

// No input while the game window is not active (the DirectInput keyboard and XInput ignore focus)
static bool WindowActive()
{
	DWORD pid = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &pid);
	return pid == GetCurrentProcessId() && !(g_gameWindow && IsIconic(g_gameWindow));
}

#define g_paused (*(short *)A(0x00531678))
#define g_pauseSubmenu (*(short *)A(0x0052f610)) // 0: the pause menu itself
#define g_pauseCursor (*(short *)A(0x00531644))   // 0: "continue"

bool g_escPressed;

// Esc is "back" (Triangle) in menus; in the pause menu itself it picks "continue", so Esc also unpauses
static void EscPause()
{
	static bool wasDown;
	bool down = (g_keyState[1] & 0x80) != 0; // DIK_ESCAPE
	g_escPressed = down && !wasDown;
	wasDown = down;
	if (g_escPressed && g_paused && g_pauseSubmenu == 0) {
		g_pauseCursor = 0;
		g_inputButtons = (g_inputButtons & ~PAD_TRIANGLE) | PAD_CROSS;
		g_escPressed = false; // not paused again by the game frame that may follow in this same frame
	}
}

// Pad inputs of this frame, 0..1 (a stick direction: how far, past the dead zone, unscaled like the game's stick)
static void PadValues(const XINPUT_GAMEPAD &p, float v[PI_COUNT])
{
	for (int i = 0; i <= PI_RS; i++) {
		v[i] = kPadButtons[i] && (p.wButtons & kPadButtons[i]) ? 1.f : 0.f;
	}
	v[PI_LT] = p.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? 1.f : 0.f;
	v[PI_RT] = p.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? 1.f : 0.f;
	float lx = Axis(p.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / 32768.f;
	float ly = Axis(p.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / 32768.f;
	float rx = Axis(p.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / 32768.f;
	float ry = Axis(p.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / 32768.f;
	v[PI_LS_LEFT] = lx < 0 ? -lx : 0;
	v[PI_LS_RIGHT] = lx > 0 ? lx : 0;
	v[PI_LS_DOWN] = ly < 0 ? -ly : 0;
	v[PI_LS_UP] = ly > 0 ? ly : 0;
	v[PI_RS_LEFT] = rx < 0 ? -rx : 0;
	v[PI_RS_RIGHT] = rx > 0 ? rx : 0;
	v[PI_RS_DOWN] = ry < 0 ? -ry : 0;
	v[PI_RS_UP] = ry > 0 ? ry : 0;
}

// Mouse input of each row this frame (captured only): a button held, or one wheel notch as a press of one frame
// with a frame without it in between
static void MouseValues(float v[BIND_COUNT])
{
	static bool pulsed[BIND_COUNT];
	for (int r = 0; r < BIND_COUNT; r++) {
		int m = g_config.mouseBind[r];
		if (m == MI_WHEEL_UP || m == MI_WHEEL_DOWN) {
			pulsed[r] = !pulsed[r] && MouseWheel(m);
			v[r] = pulsed[r] ? 1.f : 0.f;
		} else {
			v[r] = MouseButton(m) ? 1.f : 0.f; // movement is the camera's (patches/input/controls.cpp)
		}
	}
}

// Every bound input into the game's pad bits and stick, and the look rows into g_padExtra
static void ApplyBinds(bool pad, const float padValue[PI_COUNT])
{
	float mouse[BIND_COUNT], row[BIND_COUNT];
	MouseValues(mouse);
	for (int r = 0; r < BIND_COUNT; r++) {
		int p = g_config.padBind[r];
		float v = pad && p < PI_COUNT ? padValue[p] : 0;
		row[r] = v > mouse[r] ? v : mouse[r];
	}
	unsigned short b = 0;
	for (int r = BIND_JUMP; r <= BIND_CANCEL; r++) {
		if (row[r] >= kHeld) {
			b |= kBindControl[r];
		}
	}

	// Walking: like the old joystick code, it only replaces the stick when pushed (keys keep working)
	int x = (int)((row[BIND_RIGHT] - row[BIND_LEFT]) * 32768), y = (int)((row[BIND_UP] - row[BIND_DOWN]) * 32768);
	x = x > 32767 ? 32767 : x;
	y = y > 32767 ? 32767 : y;
	if (x || y) {
		g_stickX = x;
		g_stickY = y;
	}
	if (x < -kStickThreshold) {
		b |= PAD_LEFT;
	}
	if (x > kStickThreshold) {
		b |= PAD_RIGHT;
	}
	if (y < -kStickThreshold) {
		b |= PAD_DOWN;
	}
	if (y > kStickThreshold) {
		b |= PAD_UP;
	}

	// D-pad without the stick: full push, like the keyboard arrows
	unsigned short dirs = b | g_inputButtons;
	if (!x && !y && pad && (dirs & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT))) {
		g_stickX = (dirs & PAD_RIGHT) ? 0x8000 : (dirs & PAD_LEFT) ? -0x8000 : 0;
		g_stickY = (dirs & PAD_UP) ? 0x8000 : (dirs & PAD_DOWN) ? -0x8000 : 0;
	}

	// Triggers nobody has: the game's L2 / R2, as before
	if (pad) {
		bool lt = false, rt = false;
		for (int r = 0; r < BIND_COUNT; r++) {
			lt |= g_config.padBind[r] == PI_LT;
			rt |= g_config.padBind[r] == PI_RT;
		}
		if (!lt && padValue[PI_LT] > 0) {
			b |= PAD_L2;
		}
		if (!rt && padValue[PI_RT] > 0) {
			b |= PAD_R2;
		}
	}

	// Looking: pad and mouse buttons as above, and the look rows' keys
	float look[4];
	for (int i = 0; i < 4; i++) {
		int key = g_config.lookKeys[i];
		look[i] = key && (g_keyState[key] & 0x80) ? 1.f : row[BIND_FIRST_LOOK + i];
	}
	g_padExtra.lookX = look[1] - look[0];
	g_padExtra.lookY = look[2] - look[3];
	g_inputButtons |= b;
}

static void ReadInputDevices_Pad()
{
	if (!WindowActive()) {
		int keyboard = g_keyboardReady, joystick = g_joystickAcquired;
		g_keyboardReady = 0;
		g_joystickAcquired = 0;
		g_readOriginal(); // clears the buttons; reads nothing
		g_keyboardReady = keyboard;
		g_joystickAcquired = joystick;
		memset(g_keyState, 0, 256);
		g_stickX = g_stickY = 0;
		g_padExtra = GamepadExtra();
		return;
	}
	XINPUT_STATE s;
	bool pad = ReadPad(&s);
	// Once an XInput pad has been seen, the DirectInput joystick stays off, also after the pad goes away (it gave
	// stale axes and Buzz walked on his own)
	static bool seenPad;
	seenPad |= pad;
	int joystick = g_joystickAcquired;
	if (seenPad) {
		g_joystickAcquired = 0;
	}
	g_readOriginal();
	g_joystickAcquired = joystick;
	bool lost = g_padExtra.connected && !pad;
	g_padExtra = GamepadExtra();
	g_padExtra.connected = pad;
	if (lost) {
		g_stickX = g_stickY = 0; // the game keeps the last stick push
	}
	float v[PI_COUNT] = {};
	if (pad) {
		const XINPUT_GAMEPAD &p = s.Gamepad;
		static const struct { WORD x; unsigned short ps; } fixed[] = {
			{XINPUT_GAMEPAD_START, PAD_START},     {XINPUT_GAMEPAD_BACK, PAD_SELECT},
			{XINPUT_GAMEPAD_DPAD_UP, PAD_UP},      {XINPUT_GAMEPAD_DPAD_DOWN, PAD_DOWN},
			{XINPUT_GAMEPAD_DPAD_LEFT, PAD_LEFT},  {XINPUT_GAMEPAD_DPAD_RIGHT, PAD_RIGHT},
		};
		for (const auto &m : fixed) {
			if (p.wButtons & m.x) {
				g_inputButtons |= m.ps;
			}
		}
		PadValues(p, v);
		for (int i = 0; i < PI_COUNT; i++) {
			if (v[i] >= kHeld) {
				g_padExtra.held |= 1u << i;
			}
		}
		g_controllerState = 1;
	}
	ApplyBinds(pad, v);
}

static void ReadInputDevices_Gamepad()
{
	ReadInputDevices_Pad();
	EscPause();
}

extern "C" void InstallGamepad(void)
{
	static const unsigned char readBytes[] = {0xa1, 0x18, 0xb7, 0x52, 0x00}; // mov eax, [0052b718]
	g_readOriginal = (void (*)())HookTrampoline(A(0x004150c0), (const void *)ReadInputDevices_Gamepad, readBytes, 5,
	                                            "gamepad (ReadInputDevices)");
}
