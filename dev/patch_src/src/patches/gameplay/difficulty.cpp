// Two more difficulty levels after cadet and ranger (g_difficulty 005358f8: 0, 1): commander (2) and beyond (3).
// Both play as ranger (g_difficultyScale 0x80) but Buzz takes 1.7x / 2.5x damage (DamagePlayer 00409240), and on
// beyond every second enemy the generator spawns gets a copy beside it (UpdateCreatureGenerator, 0040a463).
// The difficulty screen in LevelSelectMenu masks the value with & 1 (0043c226, 0043cd92): now & 3, and its drawing
// (0043cc59) is ours (DifficultyScreen).
#include "common.h"
#include "texts.h"

#define g_difficulty (*(unsigned char *)A(0x005358f8))
#define g_padButtons (*(unsigned short *)A(0x0088995c))
#define g_padButtonsPrev (*(unsigned short *)A(0x00889954))
#define kCadet ((const char *)A(0x004f96ec)) // the game's texts (translated in the Spanish buzz.exe)
#define kRanger ((const char *)A(0x004f96e4))

enum { DIFF_COMMANDER = 2, DIFF_BEYOND = 3, PAD_RIGHT = 0x20, PAD_LEFT = 0x80, SFX_CURDOWN = 2 };

typedef void(__cdecl *DrawTextLineFn)(int x, int y, const char *text, int width, int size, DWORD colour);
static const DrawTextLineFn DrawTextLine = (DrawTextLineFn)A(0x0049ee40);
typedef void(__cdecl *PlaySfxFn)(int sfx, int *pos);
static const PlaySfxFn PlaySfx = (PlaySfxFn)A(0x00432c50);

// The difficulty screen, every frame: value is LevelSelectMenu's (0..3), handed to g_difficulty on accept. Returns
// the arrows LevelSelectMenu draws next frame (bits: 1 left, 2 right, 4 up, 8 down; the game's gave one of the two)
static int __cdecl DifficultyScreen(int *value)
{
	unsigned short pressed = g_padButtons & ~g_padButtonsPrev;
	if ((pressed & PAD_RIGHT) && *value < DIFF_BEYOND) {
		++*value;
		PlaySfx(SFX_CURDOWN, nullptr);
	} else if ((pressed & PAD_LEFT) && *value > 0) {
		--*value;
		PlaySfx(SFX_CURDOWN, nullptr);
	}
	const char *names[] = {kCadet, kRanger, Tr(TXT_COMMANDER), Tr(TXT_BEYOND)};
	DrawTextLine(0, 0x78, names[*value & 3], 0x200, 0x10, 0x808080);
	return (*value > 0 ? 1 : 0) | (*value < DIFF_BEYOND ? 2 : 0);
}

// Jumped to from 0043cc59 (the value is at [esp+0x20] there, the arrows at [esp+0x40]); back at 0043ccb5, where the
// game's code goes on (in the running buzz.exe: set when the patch goes in)
static DWORD g_difficultyBack;
static __declspec(naked) void DifficultyScreenThunk()
{
	__asm {
		lea eax, [esp + 0x20]
		push eax
		call DifficultyScreen
		add esp, 4
		mov [esp + 0x40], eax
		mov eax, g_difficultyBack
		jmp eax
	}
}

// DamagePlayer(angle, flags, damage): 1.7x / 2.5x the damage on commander / beyond
typedef void(__cdecl *DamagePlayerFn)(unsigned angle, unsigned char flags, int damage);
static DamagePlayerFn g_damagePlayerOriginal;

static void __cdecl DamagePlayer_Difficulty(unsigned angle, unsigned char flags, int damage)
{
	if (g_difficulty == DIFF_COMMANDER) {
		damage = damage * 17 / 10;
	} else if (g_difficulty == DIFF_BEYOND) {
		damage = damage * 25 / 10;
	}
	g_damagePlayerOriginal(angle, flags, damage);
}

#define g_objects ((unsigned char *)A(0x005326a0))   // 64 GameObject of 200 bytes; [0] is the boss
#define g_creatList ((unsigned char *)A(0x0052f638)) // 128 CreatEntry of 64 bytes
#define g_floorUncached (*(int *)A(0x0050cf58))     // SpawnCreature: floor without the cached triangle
static const int kObjectSize = 200, kEntrySize = 64, kBossEntries = 0x72, kFreeLeft = 6, kSideStep = 0x800;

typedef void(__cdecl *SpawnCreatureFn)(unsigned char *obj, unsigned char *entry);
static const SpawnCreatureFn SpawnCreature = (SpawnCreatureFn)A(0x00409d70);
typedef int(__cdecl *FloorYFn)(unsigned char *obj, int shadow, void *cache);
static const FloorYFn GetObjectFloorYCached = (FloorYFn)A(0x004888c0);
typedef int(__cdecl *FloorYBelowFn)(unsigned char *obj, int shadow);
static const FloorYBelowFn GetFloorYBelow = (FloorYBelowFn)A(0x004884f0);

// Entry with moveFlags 0x100 (off for little green men, Booster, Mira, bombs...), not driven by index by the bosses'
// code (0x72-0x7f), and not a turret or boss model
static bool IsEnemy(unsigned char *obj, unsigned char *entry)
{
	int type = *(short *)(obj + 0x14), index = (int)(entry - g_creatList) / kEntrySize;
	if (!(*(unsigned *)(entry + 0x0c) & 0x100) || index < 0 || index >= kBossEntries) {
		return false;
	}
	return !(type >= 0x11 && type <= 0x14) && type != 0x0b && type != 0x15 && type != 0x16 &&
	       !(type >= 0x1c && type <= 0x22) && !(type >= 0x24 && type <= 0x26);
}

// Beyond: a copy of every second enemy, 0x800 to one side (turning each time), if 6 objects stay free. The entry's
// spawns left, flags and timer are put back so the copy is extra; no floor there, no copy.
static void __cdecl SpawnCreature_Beyond(unsigned char *obj, unsigned char *entry)
{
	SpawnCreature(obj, entry);
	static unsigned spawned;
	if (g_difficulty != DIFF_BEYOND || !IsEnemy(obj, entry) || ++spawned % 2) {
		return;
	}
	unsigned char *copy = nullptr;
	int free = 0;
	for (int i = 1; i < 64; i++) {
		if (*(short *)(g_objects + i * kObjectSize + 0x14) == 0 && free++ == 0) {
			copy = g_objects + i * kObjectSize;
		}
	}
	if (!copy || free < kFreeLeft) {
		return;
	}
	unsigned char spawnsLeft = entry[0x36];
	unsigned moveFlags = *(unsigned *)(entry + 0x0c);
	short timer = *(short *)(entry + 0x20);
	SpawnCreature(copy, entry);
	entry[0x36] = spawnsLeft;
	*(unsigned *)(entry + 0x0c) = moveFlags;
	*(short *)(entry + 0x20) = timer;
	static const int kSides[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	const int *side = kSides[spawned / 2 % 4];
	for (int k = 0; k < 2; k++) { // x and z, and the target the script walks to
		*(int *)(copy + k * 8) += side[k] * kSideStep;
		*(int *)(copy + 0x8c + k * 4) += side[k] * kSideStep;
	}
	*(int *)(copy + 0x64) = 0; // floorPiece: the cached floor was the original's
	*(unsigned *)(copy + 0x68) = 0;
	int floor = g_floorUncached ? GetFloorYBelow(copy, 0) : GetObjectFloorYCached(copy, 0, copy + 0x64);
	if (floor == (int)0x80000000) { // off the ground: no copy
		*(short *)(copy + 0x14) = 0;
		entry[0x24]--;
		return;
	}
	*(int *)(copy + 0x5c) = floor;
}

extern "C" void InstallDifficulty(void)
{
	static const unsigned char readMask[] = {0x83, 0xe0, 0x01}, readMask3[] = {0x83, 0xe0, 0x03};   // and eax, 1
	static const unsigned char writeMask[] = {0x80, 0xe2, 0x01}, writeMask3[] = {0x80, 0xe2, 0x03}; // and dl, 1
	static const unsigned char screenBytes[] = {0x8b, 0x4c, 0x24, 0x20, 0x8b}; // mov ecx, [esp+0x20]; mov eax, ...
	unsigned char jump[5] = {0xe9};
	g_difficultyBack = A(0x0043ccb5);
	*(DWORD *)(jump + 1) = (DWORD)DifficultyScreenThunk - (A(0x0043cc59) + 5);
	if (PatchBytes(A(0x0043cc59), screenBytes, jump, 5, "difficulty screen: commander and beyond (LevelSelectMenu)")) {
		PatchBytes(A(0x0043c226), readMask, readMask3, 3, "difficulty value 0..3 (LevelSelectMenu)");
		PatchBytes(A(0x0043cd92), writeMask, writeMask3, 3, "difficulty saved 0..3 (LevelSelectMenu)");
	}
	static const unsigned char damageBytes[] = {0xa0, 0x50, 0x5c, 0x53, 0x00}; // mov al, [g_player.flags]
	static const unsigned char spawnCall[] = {0xe8, 0x08, 0xf9, 0xff, 0xff}; // UpdateCreatureGenerator: call SpawnCreature
	unsigned char call[5] = {0xe8};
	*(DWORD *)(call + 1) = (DWORD)SpawnCreature_Beyond - (A(0x0040a463) + 5);
	PatchBytes(A(0x0040a463), spawnCall, call, 5, "beyond: more enemies (UpdateCreatureGenerator)");
	g_damagePlayerOriginal = (DamagePlayerFn)HookTrampoline(A(0x00409240), (const void *)DamagePlayer_Difficulty,
	                                                        damageBytes, 5, "commander / beyond damage (DamagePlayer)");
}
