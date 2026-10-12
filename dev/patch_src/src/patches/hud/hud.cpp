// HUD additions, drawn at the end of DrawHud (called from patches/system/widescreen.cpp, so they follow the screen edges).
//   - weapon info: "lv N of M" over the weapon icon (its place in g_weapons[].next chain) and "max N" under the ammo.
//   - goal info: the credits the medal asks for (g_creditMedal) after the credit count; in timed modes a gold clock
//     counting down the time left to beat the record, the two clocks centred as a pair (DrawHud's clock x rewritten).
#include "common.h"
#include "config.h"
#include "texts.h"

#define g_weapons ((const unsigned char *)A(0x00502488)) // Weapon[74], 0x2c bytes: +0 family, +2 next
#define g_currentWeapon (*(unsigned char *)A(0x0088b028))
#define g_hudWeaponIconScale (*(int *)A(0x0088b034))      // DrawHud draws the weapon only above 0x20 (it shrinks away)
#define g_sinTable ((const short *)A(0x004fd320))         // 4096 entries
#define g_hudSlide ((const short *)A(0x00535bf0))         // the HUD's slide-in slots: 0 clock, 2 health / credits / weapon
#define g_credits (*(short *)A(0x00535cae))               // g_player.credits
#define g_creditMedal ((const int *)A(0x004f9adc))        // per g_levelSlot
#define g_levelSlot (*(unsigned short *)A(0x0052ddea))
#define g_gameMode (*(int *)A(0x0052b1a0))                // 0 normal, 1 time attack, 2 XR
#define g_modeFlags (*(unsigned int *)A(0x0088d38c))      // 0x2: timed
#define g_timeLimit (*(int *)A(0x0088d0e4))               // ticks (60 a second)
#define g_timeElapsed (*(int *)A(0x0088d114))
#define g_timeRecord (*(int *)A(0x0088d098))              // record of this level and mode, copied at the start
#define g_xrPiecesLeft (*(int *)A(0x0088d0b8))            // the clock moves down when the XR count shows

static const int kWeaponSize = 0x2c;
static const int kBuzzWeapons = 30; // Buzz's weapons come first in g_weapons (enemies' from 32)

typedef void(__cdecl *DrawSpriteTintedFn)(int x, int y, int sheet, int frame, int r, int g, int b);
static const DrawSpriteTintedFn DrawSpriteTinted = (DrawSpriteTintedFn)A(0x00496080); // 0x80 = normal colour
typedef void(__cdecl *DrawCharFn)(int x, int y, int ch, int r, int g, int b, int hiRes);
static const DrawCharFn DrawChar = (DrawCharFn)A(0x00402f10); // hiRes: 512 wide

static int MaxAmmo(int w) { return *(const unsigned short *)(g_weapons + w * kWeaponSize + 6) >> 4; }
static int Family(int w) { return g_weapons[w * kWeaponSize]; }
static int Next(int w) { return g_weapons[w * kWeaponSize + 2]; }

// Level of weapon w (1 = first) and how many levels its family has; 0 if the chain does not reach it
static int WeaponLevel(int w, int *levels)
{
	int family = Family(w);
	for (int root = 0; root < kBuzzWeapons; root++) {
		if (Family(root) != family) {
			continue;
		}
		bool isRoot = true; // no other weapon upgrades into it
		for (int i = 0; i < kBuzzWeapons && isRoot; i++) {
			if (Next(i) == root && Family(i) == family && i != root) {
				isRoot = false;
			}
		}
		if (!isRoot) {
			continue;
		}
		int level = 0, count = 0;
		for (int c = root, guard = 0; guard < 16; guard++) {
			count++;
			if (c == w) {
				level = count;
			}
			if (!Next(c) || Next(c) == c) {
				break;
			}
			c = Next(c);
		}
		*levels = count;
		if (level) {
			return level;
		}
	}
	return 0;
}

// the HUD slide's offset of a slot (0 = in place, down to -0x40 = away); false while hidden
static bool Slide(int slot, int *offset)
{
	if (g_hudSlide[slot] == 0) {
		return false;
	}
	*offset = (g_sinTable[g_hudSlide[slot] & 0xfff] >> 8) - 0x40;
	return true;
}

extern "C" void HudCentered(int on); // patches/system/widescreen.cpp

// The game's clock: DrawHud's "push x" for its six digits, and how far it is moved
static const DWORD kClockPush[6] = {A(0x0040292f), A(0x00402965), A(0x00402999), A(0x004029d2), A(0x00402a03), A(0x00402a39)};
static const int kClockX[6] = {0xe8, 0xf0, 0xfc, 0x104, 0x110, 0x118};
static const int kGoldClockGap = 0x48; // the gold clock's offset from the game's
static int g_clockShift, g_clockOk = -1;

static void ShiftGameClock(int shift)
{
	if (g_clockOk < 0) {
		g_clockOk = 1;
		for (int d = 0; d < 6; d++) {
			if (*(unsigned char *)kClockPush[d] != 0x68 || *(int *)(kClockPush[d] + 1) != kClockX[d]) {
				g_clockOk = 0;
			}
		}
		LogLine(g_clockOk ? "ok clocks centred (DrawHud)" : "SKIPPED clocks centred (DrawHud): unexpected bytes");
	}
	if (!g_clockOk || shift == g_clockShift) {
		return;
	}
	for (int d = 0; d < 6; d++) {
		int x = kClockX[d] + shift;
		WriteCode(kClockPush[d] + 1, &x, 4);
	}
	g_clockShift = shift;
}

static void DrawGoalInfo()
{
	int slide;
	if (g_gameMode == 0 && Slide(2, &slide)) {
		// After the game's credit count (sheet 0x19 at x 0x40, 8 apart, y 0xd8 - slide), in gold
		int credits = g_credits, medal = g_creditMedal[g_levelSlot], y = 0xd8 - slide;
		int x = 0x40 + 8 * (credits > 99 ? 3 : credits > 9 ? 2 : 1) + 6;
		if (medal > 99) {
			DrawSpriteTinted(x, y, 0x19, medal / 100 % 10, 0x80, 0x60, 0x00), x += 8;
		}
		if (medal > 9) {
			DrawSpriteTinted(x, y, 0x19, medal / 10 % 10, 0x80, 0x60, 0x00), x += 8;
		}
		DrawSpriteTinted(x, y, 0x19, medal % 10, 0x80, 0x60, 0x00);
	}
	if ((g_modeFlags & 2) && g_timeRecord > 0 && Slide(0, &slide)) {
		// Time left to beat the record, gold, grey at 0; same digits as the game's clock, kGoldClockGap to its right
		int left = g_timeRecord - g_timeElapsed;
		bool gone = left <= 0;
		if (gone) {
			left = 0;
		}
		int digits[6] = {left / 36000 % 10, left / 3600 % 10, left / 600 % 6, left / 60 % 10,
		                 left % 60 * 100 / 60 / 10, left % 60 * 100 / 60 % 10};
		int y = slide + 0x14 + (g_xrPiecesLeft ? 0x20 : 0);
		HudCentered(1); // stays in the centre with the game's clock on wide screens
		for (int d = 0; d < 6; d++) {
			DrawSpriteTinted(kClockX[d] + kGoldClockGap + g_clockShift, y, 0x19, digits[d], gone ? 0x40 : 0x80, gone ? 0x40 : 0x60, gone ? 0x40 : 0x00);
		}
		HudCentered(0);
	}
}

extern "C" void DrawHudExtras(void)
{
	// Both clocks centred while the gold one shows (it takes effect from the next DrawHud)
	ShiftGameClock(g_config.goalInfo && (g_modeFlags & 2) && g_timeRecord > 0 ? -kGoldClockGap / 2 : 0);
	if (g_config.goalInfo) {
		DrawGoalInfo();
	}
	if (!g_config.weaponInfo) {
		return;
	}
	int w = g_currentWeapon, levels = 0;
	if (w >= kBuzzWeapons || g_hudWeaponIconScale <= 0x20) {
		return;
	}
	int level = WeaponLevel(w, &levels);
	if (!level) {
		return;
	}
	int slide; // in and out with the weapon block
	if (!Slide(2, &slide)) {
		return;
	}
	char text[16];
	int len = wsprintfA(text, Tr(TXT_WEAPON_LEVEL), level, levels);
	int x = 0x1e8 - len * 8; // 512-wide HUD units: right-aligned over the weapon icon, 8 per letter
	for (const char *c = text; *c; c++, x += 8) {
		DrawChar(x, 0xb4 - slide, (unsigned char)*c, 0x80, 0x80, 0x00, 1);
	}

	// Ammo limit, right-aligned under the ammo count (DrawHud draws ammo only for weapons other than 0)
	if (w != 0) {
		len = wsprintfA(text, Tr(TXT_MAX_AMMO), MaxAmmo(w));
		x = 0x1b0 - len * 8;
		for (const char *c = text; *c; c++, x += 8) {
			DrawChar(x, 0xea - slide, (unsigned char)*c, 0x60, 0x60, 0x60, 1);
		}
	}
}
