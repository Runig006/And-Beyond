// Enemy options (PATCH OPTIONS):
//   - enemy level hp off: SpawnCreature (0040a091) uses level factor 32 (x1) instead of g_levelHealthScale; the
//     second Zurg keeps his (DrawHud sizes his bar with it).
//   - enemy iframes off: DamageObject (004094d0) lets a killing shot kill during scriptFlags 0x80 (else health stays 1).
//   - turret wreck harmless: touching it (UpdateLevelItems 004a783b) only pushes Buzz, DamagePlayer(angle, 1, 0).
//   - fair mines: enemy mines' g_weapons hitSize 7/8 -> 0, so they go off at 150 like bullets (EnemyShotsHitPlayer).
#include "common.h"
#include "config.h"

#define g_objects0 ((char *)A(0x005326a0)) // g_objects[0], the level's villain
static const int kThemar = 10;  // creature type

// ---------------------------------------------------------------- level health
// Replaces "movsx edx, word [g_levelSlot]; mov bl, [edx + g_levelHealthScale]"; on entry edx = type * 5, ebx = 0.
// The two addresses are the running buzz.exe's, set when the patch goes in.
static int g_flatHealth;
static DWORD g_levelSlotAt, g_levelHealthScaleAt;
static __declspec(naked) void LevelHealthScale()
{
	__asm {
		push eax
		mov eax, g_levelSlotAt
		cmp edx, 0xaf // the second Zurg: model 0x23 * 5
		movsx edx, word ptr [eax]
		je original
		cmp g_flatHealth, 0
		je original
		pop eax
		mov bl, 32
		ret
	original:
		mov eax, g_levelHealthScaleAt
		mov bl, byte ptr [edx + eax]
		pop eax
		ret
	}
}

// ---------------------------------------------------------------- iframes
typedef int(__cdecl *DamageObjectFn)(char *obj, unsigned angle, int knockback, int damage, int source);
static DamageObjectFn g_damageOriginal;

static int __cdecl DamageObject_Iframes(char *obj, unsigned angle, int knockback, int damage, int source)
{
	unsigned short &flags = *(unsigned short *)(obj + 0xac); // scriptFlags: 0x80 cannot die, 0x20 dying
	short health = *(short *)(obj + 0xc0);
	// Not the villain, the appear animation (+0xc4) or the Themar (it uses the flag while holding Buzz)
	if (!g_config.enemyHitGuard && obj != g_objects0 && damage > 0 && (flags & 0xa0) == 0x80 && health - damage < 1 &&
	    *(unsigned char *)(obj + 0xc4) == 0 && *(short *)(obj + 0x14) != kThemar) {
		flags &= ~0x80; // the game's own code then runs the death
	}
	return g_damageOriginal(obj, angle, knockback, damage, source);
}

static bool g_wreckPatched, g_minesOk;
static const struct { DWORD addr; unsigned char hitSize; } kMines[] = {
	{A(0x00502b0a), 7}, {A(0x00502b36), 8}, {A(0x00502dca), 7}, {A(0x00502df6), 7}, // g_weapons[0x25 / 0x26 / 0x35 / 0x36].hitSize
};

extern "C" void EnemiesApply()
{
	g_flatHealth = !g_config.enemyLevelHealth;
	if (g_wreckPatched) {
		static const unsigned char hurts[] = {0x6a, 0x08, 0x6a, 0x03}, pushes[] = {0x6a, 0x00, 0x6a, 0x01};
		WriteCode(A(0x004a783b), g_config.wreckHarmless ? pushes : hurts, 4);
	}
	if (g_minesOk) {
		for (const auto &m : kMines) {
			*(unsigned char *)m.addr = g_config.fairMines ? 0 : m.hitSize; // .data: writable
		}
	}
}

extern "C" void InstallEnemies(void)
{
	static const unsigned char wreckBytes[] = {0x6a, 0x08, 0x6a, 0x03}; // push 8 (damage); push 3 (push and hurt)
	g_minesOk = true;
	for (const auto &m : kMines) {
		g_minesOk &= *(unsigned char *)m.addr == m.hitSize;
	}
	LogLine(g_minesOk ? "ok fair mines (g_weapons hitSize)" : "SKIPPED fair mines (g_weapons hitSize): unexpected bytes");
	g_wreckPatched = PatchBytes(A(0x004a783b), wreckBytes, wreckBytes, 4, "turret wreck damage (UpdateLevelItems)") != 0;
	EnemiesApply();
	static const unsigned char damageBytes[] = {0xa1, 0x00, 0xd1, 0x88, 0x00}; // mov eax, [0088d100]
	g_damageOriginal = (DamageObjectFn)HookTrampoline(A(0x004094d0), (const void *)DamageObject_Iframes, damageBytes, 5,
	                                                  "enemy iframes (DamageObject)");
	static const unsigned char scaleBytes[] = {0x0f, 0xbf, 0x15, 0xea, 0xdd, 0x52, 0x00,  // movsx edx, word [0052ddea]
	                                           0x8a, 0x9a, 0xb8, 0x65, 0x4e, 0x00};       // mov bl, [edx + 004e65b8]
	unsigned char call[13];
	call[0] = 0xe8; // call LevelHealthScale, then nops
	g_levelSlotAt = A(0x0052ddea);
	g_levelHealthScaleAt = A(0x004e65b8);
	*(DWORD *)(call + 1) = (DWORD)LevelHealthScale - (A(0x0040a091) + 5);
	memset(call + 5, 0x90, 8);
	PatchBytes(A(0x0040a091), scaleBytes, call, 13, "enemy level health (SpawnCreature)");
}
