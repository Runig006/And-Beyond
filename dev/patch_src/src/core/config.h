// User settings, kept after the game's own ones in buzz2k.cfg (core/config.cpp).
#pragma once
#include "common.h"

// Rows of the control configuration screen (patches/input/controls_menu.cpp): what each key, mouse input or pad input does
enum Bind {
	BIND_UP, BIND_DOWN, BIND_LEFT, BIND_RIGHT, BIND_JUMP, BIND_FIRE, BIND_CHANGE_WEAPON, BIND_STRAFE_LEFT,
	BIND_STRAFE_RIGHT, BIND_CANCEL, BIND_LOOK_LEFT, BIND_LOOK_RIGHT, BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_COUNT,
	BIND_FIRST_LOOK = BIND_LOOK_LEFT,
};
// Pad inputs (patches/input/gamepad.cpp): buttons, stick clicks, triggers and each stick direction
enum PadInput {
	PI_A, PI_B, PI_X, PI_Y, PI_LB, PI_RB, PI_LT, PI_RT, PI_LS, PI_RS,
	PI_LS_LEFT, PI_LS_RIGHT, PI_LS_DOWN, PI_LS_UP, PI_RS_LEFT, PI_RS_RIGHT, PI_RS_DOWN, PI_RS_UP,
	PI_COUNT, PI_NONE = 0xff,
};
// Mouse inputs (patches/input/mouse.cpp); movement only for the look rows
enum MouseInput {
	MI_LEFT, MI_RIGHT, MI_MIDDLE, MI_BUTTON4, MI_BUTTON5, MI_WHEEL_UP, MI_WHEEL_DOWN,
	MI_MOVE_LEFT, MI_MOVE_RIGHT, MI_MOVE_UP, MI_MOVE_DOWN, MI_COUNT, MI_NONE = 0xff,
	MI_FIRST_MOVE = MI_MOVE_LEFT,
};

struct Config {
	bool windowed = false;      // false = borderless fullscreen; Alt+Enter toggles
	int windowWidth = 1280;     // client size of the window in windowed mode
	int windowHeight = 720;
	int resWidth = 0, resHeight = 0; // resolution the game is started with (PATCH OPTIONS); 0 = the desktop's
	// Most of these are in the game's PATCH OPTIONS menu (patches/menus/menu.cpp)
	bool modernControls = true; // right stick camera; firing faces the camera and side-steps (patches/input/controls.cpp)
	bool autoAim = true;        // shots bend onto an enemy close to the reticle
	int cameraSpeed = 1;        // right stick: 0 slow, 1 normal, 2 fast
	bool invertLook = false;    // right stick up looks down
	bool weaponInfo = true;     // HUD: weapon level and ammo limit (patches/hud/hud.cpp)
	bool bossCatchUp = true;    // original rubber banding of the boss races (patches/gameplay/bosses.cpp)
	bool cheats = false;        // the game's cheat mode (ZEBNEBULA): in the level select, Up unlocks every level
	// Pad and mouse input of each row of the control screen (Bind order; PadInput / MouseInput, 0xff none). The
	// keyboard keys of the game's rows are the game's own key map (keys below); the look rows' keys are lookKeys.
	unsigned char padBind[BIND_COUNT] = {PI_LS_UP, PI_LS_DOWN, PI_LS_LEFT, PI_LS_RIGHT, PI_A, PI_RT, PI_B,
	                                     PI_LB, PI_RB, PI_Y, PI_RS_LEFT, PI_RS_RIGHT, PI_RS_UP, PI_RS_DOWN};
	unsigned char mouseBind[BIND_COUNT] = {MI_NONE, MI_NONE, MI_NONE, MI_NONE, MI_NONE, MI_LEFT, MI_WHEEL_DOWN,
	                                       MI_NONE, MI_NONE, MI_NONE, MI_MOVE_LEFT, MI_MOVE_RIGHT, MI_MOVE_UP,
	                                       MI_MOVE_DOWN};
	unsigned char lookKeys[4] = {}; // DirectInput key codes of the look rows, 0 none
	bool drawDistanceFar = true;  // scenery drawn twice as far, fog moved with it (patches/graphics/drawdistance.cpp)
	bool enemyLevelHealth = false; // enemies get more health in later levels, as in the game (off by default) (patches/gameplay/enemies.cpp)
	bool enemyHitGuard = false;   // after a hit an enemy cannot die for a while, as in the game (off by default) (patches/gameplay/enemies.cpp)
	bool autoSave = true;         // save after each level and load at the title, to the slot in use (patches/gameplay/autosave.cpp)
	int saveSlot = 0;             // that slot, 1..8 (0: none yet)
	bool goalInfo = true;         // HUD: credits the medal asks for, and the time to beat (patches/hud/hud.cpp)
	// The game's keyboard map (0x184 bytes from 0052b4e0), formerly in buzz99.sav (patches/gameplay/autosave.cpp)
	static const int kKeyBytes = 0x184;
	bool haveKeys = false;
	unsigned char keys[kKeyBytes] = {};
	bool vehicleAim = true;       // modern controls: vehicles shoot where the camera looks (patches/input/controls.cpp)
	bool vehicleSteer = false;    // modern controls: mouse / right stick turn the bike and the board, not the camera
	bool fogNear = true;          // planet fog in some levels, 1500 to 25000 (patches/graphics/drawdistance.cpp)
	bool seeThrough = true;       // what stands between the camera and Buzz is see-through (patches/graphics/seethrough.cpp)
	bool wreckHarmless = true;    // a destroyed turret's wreck pushes Buzz without hurting him (patches/gameplay/enemies.cpp)
	bool fairMines = true;        // enemy mines go off only when Buzz is close (patches/gameplay/enemies.cpp)
	bool priceBlink = true;       // weapon prices blink red while Buzz cannot pay them (patches/graphics/effects.cpp)
	int language = 0;             // the game's texts: 0 not chosen yet, else 1 + en es fr de it pt (patches/menus/language.cpp)
	bool bathyosCamera = true;    // the follow camera higher in Bathyos: the submarine hid what is ahead (patches/graphics/bathyoscamera.cpp)
	bool bossScene = true;        // the boss's scene also after Mira's when Buzz wins the race (patches/gameplay/bosses.cpp)
};

extern Config g_config;
void ConfigLoad();
bool ConfigSave();
void ApplyOurDefaultKeys(); // patches/input/controls_menu.cpp: W A S D unless the game's keys were changed
