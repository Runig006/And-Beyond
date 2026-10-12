// Smooth 60 FPS. WaitFrame (00492c90) set g_frameStep (ticks of 1/60 s per frame) from rounded-up
// milliseconds, locked 30 FPS for 60 frames after a slow one and slept with Sleep(1): it kept jumping between 60
// and 30. Replaced by a fixed 60 Hz clock with precise waits; g_frameStep = ticks that really went by (max 4).
// The demo keeps step 2 (its recorded inputs need it). Paced frames are presented without vsync.
#include "render.h"

#define g_frameStep (*(int *)A(0x00535be4))
#define g_demoMode (*(int *)A(0x0052f5f4))

static const int kMaxStep = 4; // the game's own value is at 004ff6a4

static LARGE_INTEGER g_freq;
static long long g_next; // when the current tick ends (performance counter units)
static HANDLE g_timer;

static long long Now()
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return t.QuadPart;
}

// Waits until 'until': sleeps on a high-resolution timer for most of it, then spins for the last half millisecond
static void WaitUntil(long long until)
{
	long long margin = g_freq.QuadPart / 2000;
	for (;;) {
		long long left = until - Now();
		if (left <= 0) {
			return;
		}
		if (left > margin && g_timer) {
			LARGE_INTEGER due;
			due.QuadPart = -(long long)((left - margin) * 10000000.0 / g_freq.QuadPart); // relative, 100 ns units
			if (SetWaitableTimer(g_timer, &due, 0, nullptr, nullptr, FALSE)) {
				WaitForSingleObject(g_timer, INFINITE);
			}
		} else if (left > margin) {
			Sleep(1);
		} else {
			YieldProcessor();
		}
	}
}

// WaitFrame(int inGame): the argument only fed the 30 FPS lock, which is gone
static void __cdecl WaitFrame_Smooth(int)
{
	int ticksPerStep = g_demoMode ? 2 : 1;
	long long tick = g_freq.QuadPart / 60 * ticksPerStep;
	long long now = Now();
	if (!g_next || now - g_next > tick * kMaxStep) { // first frame, or back from a load / Alt+Tab: start over
		g_next = now;
	}
	g_next += tick;
	int steps = 1;
	if (now < g_next) {
		WaitUntil(g_next);
	} else { // late: count the ticks that went by
		steps += (int)((now - g_next) / tick);
		g_next += (long long)(steps - 1) * tick;
	}
	g_frameStep = g_demoMode ? 2 : (steps > kMaxStep ? kMaxStep : steps);
	g_render.paced = true;
}

extern "C" void InstallFrameRate(void)
{
	QueryPerformanceFrequency(&g_freq);
	g_timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	if (!g_timer) { // Windows older than 10 (1803)
		g_timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
	}
	static const unsigned char delayBytes[] = {0x57, 0xe8, 0xaa, 0xaf, 0x01, 0x00}; // push edi; call ReadTimer
	HookReplace(A(0x00492c90), (const void *)WaitFrame_Smooth, delayBytes, 6, "smooth 60 FPS (WaitFrame)");
}
