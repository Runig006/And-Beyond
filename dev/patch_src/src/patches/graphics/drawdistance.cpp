// Draw distance and fog (PATCH OPTIONS).
//   - draw distance far: g_renderDistanceTable (005074b8) far distances x2, the fog (SetFogRange 004bdf30)
//     x2, and the enemies' draw / free ranges in UpdateObjects and CullCreatures (0044ac45) x2.
//   - planet fog activated: some levels get fog from 1500 to 25000 in their own colour (the game's fog started past
//     the end of the scenery); levels with a backdrop get it from ClearFrame (004bdec0).
#include "common.h"
#include "config.h"

#define g_renderDistanceTable ((float *)A(0x005074b8)) // 3 rows x 6 floats
static const float kFar = 2;
static const float kFogStart = 1500, kFogEnd = 25000; // planet fog, renderer units (the camera is ~400 from Buzz)

static float g_original[18];
static bool g_appliedFar;

typedef void(__cdecl *SetFogRangeFn)(float start, float end, DWORD color);
static SetFogRangeFn g_fogRangeOriginal;

static bool g_actorRangePatched;

// CullCreatures: "movsx edi, [esi+0x42]; mov ax, [esi+0x40]; imul edi, edi", with the square clamped so the doubled
// range cannot overflow the "shl edi, 4" and the 250000 added after it
static __declspec(naked) void CullRange()
{
	__asm {
		movsx edi, word ptr [esi + 0x42]
		mov ax, word ptr [esi + 0x40]
		imul edi, edi
		cmp edi, 134201976
		jbe done
		mov edi, 134201976
	done:
		ret
	}
}

// Applies the setting: from SetFogRange every frame, and when the option changes. Range: GameObject +0x42 =
// model byte << 5 (<< 6 far), the boss 0x1000 (0x2000); objects are freed beyond 3000 squared (6000 far).
extern "C" void DrawDistanceApply()
{
	if (g_config.drawDistanceFar == g_appliedFar) {
		return;
	}
	g_appliedFar = g_config.drawDistanceFar;
	float k = g_appliedFar ? kFar : 1;
	for (int row = 0; row < 3; row++) {
		g_renderDistanceTable[row * 6 + 1] = g_original[row * 6 + 1] * k;
		g_renderDistanceTable[row * 6 + 4] = g_original[row * 6 + 4] * k;
	}
	if (g_actorRangePatched) {
		unsigned char shift = g_appliedFar ? 6 : 5;
		unsigned short boss = g_appliedFar ? 0x2000 : 0x1000;
		WriteCode(A(0x0040add2), &shift, 1);
		WriteCode(A(0x0040afbb), &shift, 1);
		WriteCode(A(0x0040af3b), &boss, 2);
		DWORD freeSq = g_appliedFar ? 36000000 : 9000000;
		WriteCode(A(0x0040ae77), &freeSq, 4);
	}
}

#define g_level (*(int *)A(0x0088994c))
extern "C" bool RenderingPlay(); // patches/graphics/seethrough.cpp

// Levels with planet fog (internal g_level) and its colour (the game's, the clear colour, is black at night)
static const struct { int level; DWORD colour; } kFogLevels[] = {
	{7, 0xff181528},  // Canis-Lunis 1: dark blue purple, as its far hills (SetFogRange brightens it: gamma)
	{9, 0xffb4c4d4},  // Ice World: pale blue white
	{10, 0xff184a58}, // Bathyos: deep sea blue green
	{13, 0xff401010}, // Planet Z: dark red, as its walls and floor
};

// The fog colour of this level, or 0 if it gets no fog
static DWORD FogLevel()
{
	if (!g_config.fogNear || !RenderingPlay()) {
		return 0;
	}
	for (const auto &f : kFogLevels) {
		if (f.level == g_level) {
			return f.colour;
		}
	}
	return 0;
}

static void __cdecl SetFogRange_Far(float start, float end, DWORD color)
{
	DrawDistanceApply();
	float k = g_config.drawDistanceFar ? kFar : 1;
	start *= k;
	end *= k;
	if (DWORD colour = FogLevel()) { // fixed distances (the levels are small), in the level's colour
		color = colour;
		start = kFogStart;
		end = kFogEnd;
	}
	g_fogRangeOriginal(start, end, color);
}

typedef void(__cdecl *ClearFrameFn)(DWORD color, int flags);
static ClearFrameFn g_clearFrameOriginal;

static void __cdecl ClearFrame_Fog(DWORD color, int flags)
{
	g_clearFrameOriginal(color, flags);
	DWORD caller = (DWORD)_ReturnAddress();
	if (FogLevel() && flags == 2 && caller >= A(0x004444d0) && caller < A(0x004449e6)) { // RenderFrame, level with backdrop
		SetFogRange_Far(24000, 48000, color); // what RenderFrame does in the other levels
	}
}

extern "C" void InstallDrawDistance(void)
{
	memcpy(g_original, g_renderDistanceTable, sizeof g_original);
	static const unsigned char cullBytes[] = {0x0f, 0xbf, 0x7e, 0x42, 0x66, 0x8b, 0x46, 0x40, 0x0f, 0xaf, 0xff};
	unsigned char cullCall[11] = {0xe8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
	*(DWORD *)(cullCall + 1) = (DWORD)CullRange - (A(0x0044ac45) + 5);
	static const unsigned char freeCmp[] = {0x81, 0x7e, 0x70, 0x40, 0x54, 0x89, 0x00}; // cmp [esi+0x70], 9000000
	static const unsigned char shl5[] = {0xc1, 0xe2, 0x05}, bossRange[] = {0x66, 0xc7, 0x40, 0x42, 0x00, 0x10};
	// The same bytes back: only checks them (DrawDistanceApply rewrites them for "far")
	g_actorRangePatched = PatchBytes(A(0x0044ac45), cullBytes, cullCall, 11, "enemy draw range clamp (CullCreatures)") &&
	                      PatchBytes(A(0x0040add0), shl5, shl5, 3, "enemy draw range (UpdateObjects)") &&
	                      PatchBytes(A(0x0040afb9), shl5, shl5, 3, "boss draw range (UpdateObjects)") &&
	                      PatchBytes(A(0x0040af37), bossRange, bossRange, 6, "boss draw range 0x1000 (UpdateObjects)") &&
	                      PatchBytes(A(0x0040ae74), freeCmp, freeCmp, 7, "enemy free distance (UpdateObjects)");
	DrawDistanceApply(); // the saved setting, also in levels where RenderFrame never calls SetFogRange
	static const unsigned char fogBytes[] = {0x8b, 0x54, 0x24, 0x0c, 0x8b, 0x44, 0x24, 0x04}; // mov edx/eax, [esp+..]
	g_fogRangeOriginal = (SetFogRangeFn)HookTrampoline(A(0x004bdf30), (const void *)SetFogRange_Far, fogBytes, 8,
	                                                        "draw distance (SetFogRange)");
	static const unsigned char clearBytes[] = {0x8b, 0x44, 0x24, 0x04, 0x50}; // mov eax, [esp+4]; push eax
	g_clearFrameOriginal = (ClearFrameFn)HookTrampoline(A(0x004bdec0), (const void *)ClearFrame_Fog, clearBytes, 5,
	                                                      "fog in levels with a backdrop (ClearFrame)");
}
