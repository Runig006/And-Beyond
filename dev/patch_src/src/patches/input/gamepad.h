// What the input has beyond the PlayStation pad bits the game knows (patches/input/gamepad.cpp), for other patches.
#pragma once

struct GamepadExtra {
	bool connected = false;
	float lookX = 0, lookY = 0; // look rows of this frame from pad and keys, -1..1 (right, up); the mouse is apart
	unsigned held = 0;          // pad inputs held, bit n = PadInput n (a stick direction: pushed past half way)
};

extern GamepadExtra g_padExtra;

// Esc went down this frame and nothing took it yet (patches/input/gamepad.cpp; the pause in game takes it, patches/input/controls.cpp)
extern bool g_escPressed;
