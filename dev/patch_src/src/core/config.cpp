// Our settings, stored in buzz2k.cfg after the game's 0x1c bytes (g_gfxCfg; the game never reads further):
// "BZPT", version byte, one byte per value in F_ order. Newer versions only append bytes, so older files still load.
// WriteCfg (004d40b0) is hooked to append our part again. An old buzzpatch.ini is moved here once.
#include "config.h"
#include <stdio.h>

Config g_config;

#define g_gfxCfg ((const unsigned char *)A(0x005073e0)) // the game's 0x1c bytes (valid once the game has read buzz2k.cfg)
static const int kGameBytes = 0x1c;
static const unsigned char kMagic[4] = {'B', 'Z', 'P', 'T'};
static const unsigned char kVersion = 17;

// File order (window size / 8; resolution 2 bytes each, low first). F_COUNT_Vn = bytes in version n.
enum {
	F_WINDOWED, F_WINDOW_W8, F_WINDOW_H8, F_MODERN, F_AUTO_AIM, F_CAMERA_SPEED, F_INVERT_LOOK, F_WEAPON_INFO,
	F_BOSS_CATCH_UP, F_CHEATS,
	F_COUNT_V1, F_PAD_BUTTONS = F_COUNT_V1,
	F_COUNT_V2 = F_PAD_BUTTONS + 6, F_DRAW_DISTANCE = F_COUNT_V2,
	F_COUNT_V3, F_ENEMY_LEVEL_HP = F_COUNT_V3, F_ENEMY_HIT_GUARD,
	F_COUNT_V4, F_VEHICLE_STEER = F_COUNT_V4,
	F_COUNT_V5, F_GOAL_INFO = F_COUNT_V5, F_AUTOSAVE, F_SAVE_SLOT, F_VEHICLE_AIM,
	F_COUNT_V6, F_HAVE_KEYS = F_COUNT_V6, F_KEYS,
	F_COUNT_V7 = F_KEYS + Config::kKeyBytes, F_RES_W = F_COUNT_V7, F_RES_H = F_RES_W + 2,
	F_COUNT_V8 = F_RES_H + 2, F_SEE_THROUGH = F_COUNT_V8,
	F_COUNT_V9, F_FOG_NEAR = F_COUNT_V9,
	F_COUNT_V10, F_BOSS_SCENE = F_COUNT_V10,
	F_COUNT_V11, F_WRECK_HARMLESS = F_COUNT_V11,
	F_COUNT_V12, F_PRICE_BLINK = F_COUNT_V12,
	F_COUNT_V13, F_FAIR_MINES = F_COUNT_V13,
	F_COUNT_V14, F_PAD_BIND = F_COUNT_V14, F_MOUSE_BIND = F_PAD_BIND + BIND_COUNT,
	F_LOOK_KEYS = F_MOUSE_BIND + BIND_COUNT,
	F_COUNT_V15 = F_LOOK_KEYS + 4, F_LANGUAGE = F_COUNT_V15,
	F_COUNT_V16, F_BATHYOS_CAMERA = F_COUNT_V16,
	F_COUNT
};

// Bytes stored by each version (index = version byte)
static const int kVersionBytes[kVersion + 1] = {0, F_COUNT_V1, F_COUNT_V2, F_COUNT_V3, F_COUNT_V4, F_COUNT_V5,
                                                F_COUNT_V6, F_COUNT_V7, F_COUNT_V8, F_COUNT_V9, F_COUNT_V10,
                                                F_COUNT_V11, F_COUNT_V12, F_COUNT_V13, F_COUNT_V14, F_COUNT_V15,
                                                F_COUNT_V16, F_COUNT};

static void CfgPath(char *out)
{
	lstrcpyA(out, g_exeDir);
	lstrcatA(out, "buzz2k.cfg");
}

static void FromBytes(const unsigned char *b, int count)
{
	Config &c = g_config;
	c.windowed = b[F_WINDOWED] != 0;
	c.windowWidth = b[F_WINDOW_W8] * 8;
	c.windowHeight = b[F_WINDOW_H8] * 8;
	c.modernControls = b[F_MODERN] != 0;
	c.autoAim = b[F_AUTO_AIM] != 0;
	c.cameraSpeed = b[F_CAMERA_SPEED] <= 2 ? b[F_CAMERA_SPEED] : 1;
	c.invertLook = b[F_INVERT_LOOK] != 0;
	c.weaponInfo = b[F_WEAPON_INFO] != 0;
	c.bossCatchUp = b[F_BOSS_CATCH_UP] != 0;
	c.cheats = b[F_CHEATS] != 0;
	if (count >= F_COUNT_V2 && count < F_COUNT_V15) { // the six buttons of the game's actions, before every row had one
		for (int i = 0; i < 6; i++) {
			if (b[F_PAD_BUTTONS + i] <= PI_RT) {
				c.padBind[BIND_JUMP + i] = b[F_PAD_BUTTONS + i];
			}
		}
		// RT used to fire on top of the fire button: now it is fire's button, if nothing else had it
		bool rtTaken = false;
		for (int i = 0; i < BIND_COUNT; i++) {
			rtTaken |= i != BIND_FIRE && c.padBind[i] == PI_RT;
		}
		if (!rtTaken) {
			c.padBind[BIND_FIRE] = PI_RT;
		}
	}
	if (count >= F_COUNT_V3) {
		c.drawDistanceFar = b[F_DRAW_DISTANCE] != 0;
	}
	if (count >= F_COUNT_V4) {
		c.enemyLevelHealth = b[F_ENEMY_LEVEL_HP] != 0;
		c.enemyHitGuard = b[F_ENEMY_HIT_GUARD] != 0;
	}
	if (count >= F_COUNT_V5) {
		c.vehicleSteer = b[F_VEHICLE_STEER] != 0;
	}
	if (count >= F_COUNT_V6) {
		c.goalInfo = b[F_GOAL_INFO] != 0;
		c.autoSave = b[F_AUTOSAVE] != 0;
		c.vehicleAim = b[F_VEHICLE_AIM] != 0;
		c.saveSlot = b[F_SAVE_SLOT] <= 8 ? b[F_SAVE_SLOT] : 0;
	}
	if (count >= F_COUNT_V7) {
		c.haveKeys = b[F_HAVE_KEYS] != 0;
		memcpy(c.keys, b + F_KEYS, Config::kKeyBytes);
	}
	if (count >= F_COUNT_V8) {
		c.resWidth = b[F_RES_W] | b[F_RES_W + 1] << 8;
		c.resHeight = b[F_RES_H] | b[F_RES_H + 1] << 8;
	}
	if (count >= F_COUNT_V9) {
		c.seeThrough = b[F_SEE_THROUGH] != 0;
	}
	if (count >= F_COUNT_V10) {
		c.fogNear = b[F_FOG_NEAR] != 0;
	}
	if (count >= F_COUNT_V11) {
		c.bossScene = b[F_BOSS_SCENE] != 0;
	}
	if (count >= F_COUNT_V12) {
		c.wreckHarmless = b[F_WRECK_HARMLESS] != 0;
	}
	if (count >= F_COUNT_V13) {
		c.priceBlink = b[F_PRICE_BLINK] != 0;
	}
	if (count >= F_COUNT_V14) {
		c.fairMines = b[F_FAIR_MINES] != 0;
	}
	if (count >= F_COUNT_V15) {
		for (int i = 0; i < BIND_COUNT; i++) {
			unsigned char p = b[F_PAD_BIND + i], m = b[F_MOUSE_BIND + i];
			c.padBind[i] = p < PI_COUNT ? p : PI_NONE;
			c.mouseBind[i] = m < MI_COUNT && (m < MI_FIRST_MOVE || i >= BIND_FIRST_LOOK) ? m : MI_NONE;
		}
		memcpy(c.lookKeys, b + F_LOOK_KEYS, 4);
	}
	if (count >= F_COUNT_V16) {
		c.language = b[F_LANGUAGE] <= 6 ? b[F_LANGUAGE] : 0;
	}
	if (count >= F_COUNT) {
		c.bathyosCamera = b[F_BATHYOS_CAMERA] != 0;
	}
}

static void ToBytes(unsigned char *b)
{
	int w8 = g_config.windowWidth / 8, h8 = g_config.windowHeight / 8;
	b[F_WINDOWED] = g_config.windowed;
	b[F_WINDOW_W8] = (unsigned char)(w8 > 255 ? 255 : w8);
	b[F_WINDOW_H8] = (unsigned char)(h8 > 255 ? 255 : h8);
	b[F_MODERN] = g_config.modernControls;
	b[F_AUTO_AIM] = g_config.autoAim;
	b[F_CAMERA_SPEED] = (unsigned char)g_config.cameraSpeed;
	b[F_INVERT_LOOK] = g_config.invertLook;
	b[F_WEAPON_INFO] = g_config.weaponInfo;
	b[F_BOSS_CATCH_UP] = g_config.bossCatchUp;
	b[F_CHEATS] = g_config.cheats;
	for (int i = 0; i < 6; i++) { // for older builds: they keep their default for what they cannot show
		unsigned char p = g_config.padBind[BIND_JUMP + i];
		b[F_PAD_BUTTONS + i] = p <= PI_RT ? p : 0xff;
	}
	memcpy(b + F_PAD_BIND, g_config.padBind, BIND_COUNT);
	memcpy(b + F_MOUSE_BIND, g_config.mouseBind, BIND_COUNT);
	memcpy(b + F_LOOK_KEYS, g_config.lookKeys, 4);
	b[F_DRAW_DISTANCE] = g_config.drawDistanceFar;
	b[F_ENEMY_LEVEL_HP] = g_config.enemyLevelHealth;
	b[F_ENEMY_HIT_GUARD] = g_config.enemyHitGuard;
	b[F_VEHICLE_STEER] = g_config.vehicleSteer;
	b[F_GOAL_INFO] = g_config.goalInfo;
	b[F_AUTOSAVE] = g_config.autoSave;
	b[F_VEHICLE_AIM] = g_config.vehicleAim;
	b[F_HAVE_KEYS] = g_config.haveKeys;
	memcpy(b + F_KEYS, g_config.keys, Config::kKeyBytes);
	b[F_RES_W] = (unsigned char)g_config.resWidth;
	b[F_RES_W + 1] = (unsigned char)(g_config.resWidth >> 8);
	b[F_RES_H] = (unsigned char)g_config.resHeight;
	b[F_RES_H + 1] = (unsigned char)(g_config.resHeight >> 8);
	b[F_SAVE_SLOT] = (unsigned char)g_config.saveSlot;
	b[F_SEE_THROUGH] = g_config.seeThrough;
	b[F_FOG_NEAR] = g_config.fogNear;
	b[F_BOSS_SCENE] = g_config.bossScene;
	b[F_WRECK_HARMLESS] = g_config.wreckHarmless;
	b[F_PRICE_BLINK] = g_config.priceBlink;
	b[F_FAIR_MINES] = g_config.fairMines;
	b[F_LANGUAGE] = (unsigned char)g_config.language;
	b[F_BATHYOS_CAMERA] = g_config.bathyosCamera;
}

// Settings from the buzzpatch.ini of early builds; true if there was one
static bool FromOldIni(char *iniPath)
{
	lstrcpyA(iniPath, g_exeDir);
	lstrcatA(iniPath, "buzzpatch.ini");
	if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	Config &c = g_config;
	c.windowed = GetPrivateProfileIntA("display", "windowed", c.windowed, iniPath) != 0;
	c.windowWidth = GetPrivateProfileIntA("display", "window_width", c.windowWidth, iniPath);
	c.windowHeight = GetPrivateProfileIntA("display", "window_height", c.windowHeight, iniPath);
	c.modernControls = GetPrivateProfileIntA("gameplay", "modern_controls", c.modernControls, iniPath) != 0;
	c.autoAim = GetPrivateProfileIntA("gameplay", "auto_aim", c.autoAim, iniPath) != 0;
	c.cameraSpeed = GetPrivateProfileIntA("gameplay", "camera_speed", c.cameraSpeed, iniPath);
	c.invertLook = GetPrivateProfileIntA("gameplay", "invert_look", c.invertLook, iniPath) != 0;
	c.weaponInfo = GetPrivateProfileIntA("gameplay", "weapon_info", c.weaponInfo, iniPath) != 0;
	c.bossCatchUp = GetPrivateProfileIntA("gameplay", "boss_catch_up", c.bossCatchUp, iniPath) != 0;
	c.cheats = GetPrivateProfileIntA("gameplay", "cheats", c.cheats, iniPath) != 0;
	if (c.cameraSpeed < 0 || c.cameraSpeed > 2) {
		c.cameraSpeed = 1;
	}
	return true;
}

void ConfigLoad()
{
	char path[MAX_PATH], ini[MAX_PATH];
	CfgPath(path);
	bool found = false, haveFile = false;
	FILE *f = fopen(path, "rb");
	if (f) {
		haveFile = true;
		unsigned char head[kGameBytes], magic[5], b[F_COUNT] = {};
		if (fread(head, 1, kGameBytes, f) == kGameBytes && fread(magic, 1, 5, f) == 5 && !memcmp(magic, kMagic, 4) &&
		    magic[4] >= 1 && magic[4] <= kVersion) {
			int count = kVersionBytes[magic[4]];
			if (fread(b, 1, count, f) == (size_t)count) {
				FromBytes(b, count);
				found = true;
			}
		}
		fclose(f);
	}
	if (!found && FromOldIni(ini)) {
		if (haveFile && ConfigSave()) { // moved into buzz2k.cfg: the old file goes
			DeleteFileA(ini);
			LogLine("settings moved from buzzpatch.ini to buzz2k.cfg");
		}
	}
	if (g_config.windowWidth < 320) {
		g_config.windowWidth = 320;
	}
	if (g_config.windowHeight < 240) {
		g_config.windowHeight = 240;
	}
}

bool ConfigSave()
{
	char path[MAX_PATH];
	CfgPath(path);
	// The game's part: what is in the file (the game writes it), or what it has in memory if there is no file yet
	unsigned char head[kGameBytes];
	bool haveHead = false;
	FILE *f = fopen(path, "rb");
	if (f) {
		haveHead = fread(head, 1, kGameBytes, f) == kGameBytes;
		fclose(f);
	}
	if (!haveHead) {
		memcpy(head, g_gfxCfg, kGameBytes);
	}
	unsigned char b[F_COUNT] = {}; // unused bytes as 0
	ToBytes(b);
	f = fopen(path, "wb");
	if (!f) {
		LogLine("ERROR saving settings: cannot write %s", path);
		return false;
	}
	bool ok = fwrite(head, 1, kGameBytes, f) == kGameBytes && fwrite(kMagic, 1, 4, f) == 4 &&
	          fwrite(&kVersion, 1, 1, f) == 1 && fwrite(b, 1, F_COUNT, f) == F_COUNT;
	fclose(f);
	return ok;
}

// WriteCfg (004d40b0) rewrites buzz2k.cfg with only the game's part: put ours back after it
static int(__cdecl *g_writeCfgOriginal)();
static int __cdecl WriteCfg_KeepOurs()
{
	int r = g_writeCfgOriginal();
	ConfigSave();
	return r;
}

extern "C" void InstallConfig(void)
{
	static const unsigned char writeBytes[] = {0x56, 0x68, 0xf0, 0x06, 0x50, 0x00}; // push esi; push 005006f0
	g_writeCfgOriginal = (int(__cdecl *)())HookTrampoline(A(0x004d40b0), (const void *)WriteCfg_KeepOurs, writeBytes, 6,
	                                                     "settings in buzz2k.cfg (WriteCfg)");
}

// The resolution the game picks at start-up (patches/system/modeselect.c, plain C): 0 x 0 = the desktop's
extern "C" void ConfigResolution(int *w, int *h)
{
	*w = g_config.resWidth;
	*h = g_config.resHeight;
}
