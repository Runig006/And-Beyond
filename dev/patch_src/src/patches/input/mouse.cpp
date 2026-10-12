// The mouse for the modern controls (the game has none): raw input (WM_INPUT) on a hidden window of our own thread,
// which adds up the movement for the game to take once per frame. Not the game window: the game's message loop
// (004abcb0) handles one message per frame, and hundreds of mouse messages a second queued up behind it.
// While captured the pointer is kept inside the game window.
#include "render.h"
#include "mouse.h"
#include "config.h"
#include <stdlib.h>

static volatile LONG g_moveX, g_moveY;
static volatile LONG g_buttons; // held, bit = MouseInput (MI_LEFT..MI_BUTTON5); only presses made while captured
static volatile bool g_captured;
static volatile LONG g_wheelUp, g_wheelDown; // wheel notches not used yet
// For the control screen (MouseTakeInput): the last press or notch, and movement, also when not captured
static volatile LONG g_heardInput = MI_NONE, g_heardX, g_heardY;
static const int kHeardMove = 25; // counts in one direction, in one frame, that make a movement input

static bool GameInFront()
{
	DWORD pid = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &pid);
	return pid == GetCurrentProcessId();
}

static LRESULT CALLBACK RawWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_INPUT) {
		RAWINPUT in;
		UINT size = sizeof in;
		if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, &in, &size, sizeof(RAWINPUTHEADER)) != (UINT)-1 &&
		    in.header.dwType == RIM_TYPEMOUSE) {
			const RAWMOUSE &m = in.data.mouse;
			if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE)) { // tablets and remote desktops give positions, not movement
				InterlockedExchangeAdd(&g_moveX, m.lLastX);
				InterlockedExchangeAdd(&g_moveY, m.lLastY);
				InterlockedExchangeAdd(&g_heardX, m.lLastX);
				InterlockedExchangeAdd(&g_heardY, m.lLastY);
			}
			// Button n goes down with flag 1 << 2n and up with 1 << (2n + 1)
			for (int n = MI_LEFT; n <= MI_BUTTON5; n++) {
				if (m.usButtonFlags & (1 << (2 * n))) {
					if (g_captured) { // not the click that brings the window up
						InterlockedOr(&g_buttons, 1 << n);
					}
					if (GameInFront()) {
						g_heardInput = n;
					}
				}
				if (m.usButtonFlags & (1 << (2 * n + 1))) {
					InterlockedAnd(&g_buttons, ~(1 << n));
				}
			}
			if (m.usButtonFlags & RI_MOUSE_WHEEL) {
				bool up = (SHORT)m.usButtonData > 0;
				volatile LONG &steps = up ? g_wheelUp : g_wheelDown;
				if (g_captured && steps < 3) { // a few at most, not a spin
					InterlockedIncrement(&steps);
				}
				if (GameInFront()) {
					g_heardInput = up ? MI_WHEEL_UP : MI_WHEEL_DOWN;
				}
			}
		}
	}
	return DefWindowProcA(h, msg, wp, lp);
}

static DWORD WINAPI MouseThread(void *)
{
	WNDCLASSA wc = {};
	wc.lpfnWndProc = RawWndProc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.lpszClassName = "BuzzRawMouse";
	RegisterClassA(&wc);
	HWND hwnd = CreateWindowA(wc.lpszClassName, "", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
	RAWINPUTDEVICE dev = {0x01, 0x02, RIDEV_INPUTSINK, hwnd}; // generic desktop, mouse; also when not in front
	if (!hwnd || !RegisterRawInputDevices(&dev, 1, sizeof dev)) {
		LogLine("ERROR mouse: raw input window %p, error %lu", hwnd, GetLastError());
		return 0;
	}
	LogLine("mouse: raw input ready");
	MSG msg;
	while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
		DispatchMessageA(&msg);
	}
	return 0;
}

void MouseTakeMove(int *dx, int *dy)
{
	int x = InterlockedExchange(&g_moveX, 0), y = InterlockedExchange(&g_moveY, 0);
	*dx = g_captured ? x : 0;
	*dy = g_captured ? y : 0;
}

bool MouseButton(int input)
{
	return g_captured && input >= MI_LEFT && input <= MI_BUTTON5 && (g_buttons & (1 << input));
}

bool MouseWheel(int input)
{
	volatile LONG *steps = input == MI_WHEEL_UP ? &g_wheelUp : input == MI_WHEEL_DOWN ? &g_wheelDown : nullptr;
	if (!g_captured || !steps || *steps <= 0) {
		return false;
	}
	InterlockedDecrement(steps);
	return true;
}

// Not at start-up: no thread may be started while our DLL is being loaded
static void StartMouseThread()
{
	static bool started;
	if (!started) {
		started = true;
		HANDLE t = CreateThread(nullptr, 0, MouseThread, nullptr, 0, nullptr);
		if (t) {
			CloseHandle(t);
		}
	}
}

int MouseTakeInput()
{
	StartMouseThread();
	int x = g_heardX, y = g_heardY;
	int heard = InterlockedExchange(&g_heardInput, MI_NONE);
	if (heard == MI_NONE && (abs(x) >= kHeardMove || abs(y) >= kHeardMove)) {
		heard = abs(x) >= abs(y) ? (x < 0 ? MI_MOVE_LEFT : MI_MOVE_RIGHT) : (y < 0 ? MI_MOVE_UP : MI_MOVE_DOWN);
	}
	InterlockedExchange(&g_heardX, 0); // a movement is what one frame brings: a slow drift is not one
	InterlockedExchange(&g_heardY, 0);
	return GameInFront() ? heard : MI_NONE;
}

void MouseForgetInput()
{
	g_heardInput = MI_NONE;
	InterlockedExchange(&g_heardX, 0);
	InterlockedExchange(&g_heardY, 0);
}

void MouseCapture(bool on)
{
	HWND hwnd = g_render.Window();
	if (!hwnd) {
		return;
	}
	StartMouseThread();
	// In front: the foreground window is one of ours (not necessarily the one we draw to)
	DWORD pid = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &pid);
	bool front = pid == GetCurrentProcessId() && !IsIconic(hwnd);
	on = on && front;
	if (on) { // again every frame: the window may have moved or changed size
		RECT rc;
		GetClientRect(hwnd, &rc);
		MapWindowPoints(hwnd, nullptr, (POINT *)&rc, 2);
		ClipCursor(&rc);
	} else if (g_captured) {
		ClipCursor(nullptr);
	}
	if (on != g_captured) {
		InterlockedExchange(&g_moveX, 0); // movement made while free is not camera movement
		InterlockedExchange(&g_moveY, 0);
		if (!on) {
			g_buttons = 0;
			g_wheelUp = g_wheelDown = 0;
		}
		g_captured = on;
	}
}
