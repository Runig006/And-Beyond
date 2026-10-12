// Bosses.
//   - bossCatchUp off (buzz2k.cfg only): each LevelNN_VillainMove's 17-byte speed table (indexed by how far the boss
//     is ahead of Buzz) is flattened to its middle value, so the boss keeps the level's speed wherever Buzz is.
//   - PATCH OPTIONS > boss scene always: when Buzz wins the race, Mira's scene plays, then the boss's (UpdateBossMusic
//     004a1060), and the boss stays still until it ends (LevelNN_VillainUpdate calls in UpdateMissionLogic).
#include "common.h"
#include "config.h"

struct CatchUpTable {
	int level; // internal g_level
	DWORD addr;
	unsigned char original[17];
};

static const CatchUpTable kTables[] = {
	{ 1, A(0x004f492c), {204, 187, 175, 166, 158, 149, 142, 134, 128, 128, 120, 112, 100, 80, 70, 60, 50}},
	{ 2, A(0x004f4a20), {204, 187, 175, 166, 158, 149, 142, 134, 128, 128, 120, 110, 100, 90, 80, 70, 60}},
	{ 3, A(0x004f4b6c), {204, 187, 175, 166, 158, 149, 142, 134, 128, 120, 110, 190, 90, 80, 70, 60, 50}},
	{ 4, A(0x004f4cfc), {204, 187, 175, 166, 158, 149, 142, 134, 128, 120, 110, 100, 90, 80, 70, 60, 50}},
	{ 5, A(0x004f4e4c), {204, 187, 175, 166, 158, 149, 142, 134, 128, 120, 110, 100, 90, 80, 70, 60, 50}},
	{ 6, A(0x004f4f34), {240, 220, 200, 180, 170, 160, 150, 140, 128, 120, 110, 100, 90, 80, 70, 60, 50}},
	{ 7, A(0x004f501c), {230, 210, 190, 170, 160, 150, 140, 130, 118, 110, 100, 90, 80, 70, 60, 50, 50}},
	{ 8, A(0x004f5158), {240, 220, 200, 180, 170, 160, 150, 140, 128, 120, 110, 100, 90, 80, 70, 60, 50}},
	{ 9, A(0x004f53ac), {204, 187, 175, 166, 158, 149, 132, 124, 118, 120, 110, 100, 90, 80, 70, 60, 50}},
	{10, A(0x004f54f4), {204, 187, 175, 166, 158, 149, 142, 134, 128, 128, 120, 110, 100, 90, 80, 70, 60}},
	{11, A(0x004f564c), {194, 177, 165, 156, 148, 139, 132, 124, 118, 115, 110, 100, 90, 85, 80, 75, 70}},
	{12, A(0x004f57dc), {204, 187, 175, 166, 158, 149, 142, 134, 128, 128, 120, 115, 110, 100, 90, 80, 70}},
	{13, A(0x004f5914), {130, 125, 125, 120, 120, 120, 115, 110, 105, 100, 95, 90, 80, 70, 60, 50, 40}},
};

#define g_level (*(int *)A(0x0088994c))
#define g_bossMusicState (*(int *)A(0x00739278)) // UpdateBossMusic: 0x14 play the line, 0x15 playing, 0x16 boss music
#define g_miraCreature (*(int *)A(0x0088d770))   // Mira met Buzz at the finish
#define g_cameraScript (*(unsigned short *)A(0x00531676)) // bit 0: a camera scene is running
#define g_player ((void *)A(0x00535c10))
#define g_objects ((void *)A(0x005326a0))              // [0] is the boss
static void(__cdecl *const StartCameraCutscene)(void *a, void *b, int mode, int resetLevel) =
	(void(__cdecl *)(void *, void *, int, int))A(0x00403630);

static void(__cdecl *g_updateBossMusicOriginal)();

#define g_cameraMode (*(short *)A(0x00531674)) // 2/1: a camera scene is starting (fade), 0 afterwards (UpdateGameCamera)
#define g_playerFlags (*(unsigned short *)A(0x00535c50))   // g_player.flags: bit 4 = no control (camera scenes)
#define g_playerButtons (*(unsigned short *)A(0x0052dde8))

// 0 nothing; 1 Mira's scene running; 2 the boss's scene started; 3 the boss's scene running
static int g_meeting;

// Not in g_level 14: Zurg's second fight has no meeting line
static bool BossFrozen()
{
	return g_level != 14 && ((g_miraCreature && g_config.bossScene) || g_meeting >= 2);
}

static void __cdecl UpdateBossMusic_ThenBoss()
{
	bool scene = (g_cameraScript & 1) || g_cameraMode;
	if (g_level == 14 || (!g_config.bossScene && g_meeting < 2)) {
		g_meeting = 0;
	} else if (g_miraCreature) {
		if (scene) {
			g_meeting = 1;
		} else if (g_meeting == 1 && g_bossMusicState == 0x15) { // her line still playing: Buzz waits
			g_playerFlags |= 4;
			g_playerButtons &= 0x309;
		} else if (g_meeting == 1) { // the boss's scene, as when he arrives first; state 0x14 plays his line
			g_miraCreature = 0;
			g_bossMusicState = 0x14;
			StartCameraCutscene(g_player, g_objects, 2, 0);
			g_meeting = 2;
		}
	} else if (g_meeting == 2 && scene) {
		g_meeting = 3;
	} else if (g_meeting == 3 && !scene) {
		g_meeting = 0; // the fight starts
	}
	else if (g_meeting == 1) {
		g_meeting = 0;
	}
	g_updateBossMusicOriginal();
}

// LevelNN_VillainUpdate, called from UpdateMissionLogic every tick of the fight
static const DWORD kVillainUpdate[15] = {0, A(0x00417b10), A(0x00419660), A(0x0041b370), A(0x0041d220), A(0x0041f130), A(0x00420d60),
                                         A(0x00422900), A(0x00424580), A(0x00426330), A(0x004280c0), A(0x00429ca0), A(0x0042bca0),
                                         A(0x0042dc60), A(0x0042faf0)};
static const DWORD kVillainUpdateCall[15] = {0, A(0x004aa416), A(0x004aa35c), A(0x004aa36b), A(0x004aa37a), A(0x004aa389), A(0x004aa398),
                                             A(0x004aa3a7), A(0x004aa3b6), A(0x004aa3c5), A(0x004aa3d4), A(0x004aa3e3), A(0x004aa3f2),
                                             A(0x004aa3fe), A(0x004aa40a)};

template <int N> static void __cdecl VillainUpdate_Frozen()
{
	if (!BossFrozen()) {
		((void(__cdecl *)())kVillainUpdate[N])();
	}
}

template <int N> static void RedirectVillainUpdate()
{
	DWORD at = kVillainUpdateCall[N];
	unsigned char expect[5] = {0xe8}, bytes[5] = {0xe8};
	int from = (int)kVillainUpdate[N] - (int)(at + 5), to = (int)(DWORD)&VillainUpdate_Frozen<N> - (int)(at + 5);
	memcpy(expect + 1, &from, 4);
	memcpy(bytes + 1, &to, 4);
	char name[64];
	wsprintfA(name, "boss still during the meeting scenes (level %d)", N);
	PatchBytes(at, expect, bytes, 5, name);
}

template <int... N> struct Levels {
	static void Redirect() { int unused[] = {(RedirectVillainUpdate<N>(), 0)...}; (void)unused; }
};

extern "C" void InstallBosses(void)
{
	static const unsigned char musicBytes[] = {0xa1, 0x78, 0x92, 0x73, 0x00}; // mov eax, [g_bossMusicState]
	g_updateBossMusicOriginal = (void(__cdecl *)())HookTrampoline(A(0x004a1060), (const void *)UpdateBossMusic_ThenBoss,
	                                                              musicBytes, sizeof musicBytes,
	                                                              "boss meeting scene after Mira's (UpdateBossMusic)");
	Levels<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13>::Redirect();
	if (g_config.bossCatchUp) {
		return;
	}
	char name[64];
	for (const CatchUpTable &t : kTables) {
		unsigned char flat[17];
		memset(flat, t.original[8], sizeof flat);
		wsprintfA(name, "no boss rubber band (level %d)", t.level);
		PatchBytes(t.addr, t.original, flat, sizeof flat, name);
	}
}
