// The mouse for the modern controls (patches/input/mouse.cpp): raw movement and buttons, read from Windows directly (the
// game has no mouse support).
#pragma once

// Movement since the last call, in mouse counts (x right, y down); 0 while the mouse is not captured
void MouseTakeMove(int *dx, int *dy);
// Button held (MI_LEFT..MI_BUTTON5), while captured
bool MouseButton(int input);
// One notch of the wheel that way (MI_WHEEL_UP / MI_WHEEL_DOWN) waiting, taken; only while captured
bool MouseWheel(int input);
// For the control screen, also when not captured: a press, a notch or a clear movement since the last call
// (MouseInput), or MI_NONE. MouseForgetInput drops what came before.
int MouseTakeInput();
void MouseForgetInput();
// Called once per frame: keep the pointer inside the game window while true (released on false or when the game
// window is not in front)
void MouseCapture(bool on);
