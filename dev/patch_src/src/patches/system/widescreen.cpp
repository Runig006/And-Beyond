// Widescreen 2D (the 3D already uses a fixed vertical field of view). Sprites (QueueSprite 004bc6a0) keep their
// 4:3 size: in the HUD (DrawHud) each sticks to its side of the screen, with the villain bar, the top centre block
// and texts / pause box centred; the sky covers the whole width; everything else is centred 4:3. Frames without
// DrawHud are menus: black bars outside 4:3, and pictures / videos drawn into the 4:3 area.
#include "render.h"

enum Anchor { ANCHOR_NONE, ANCHOR_LEFT, ANCHOR_CENTER, ANCHOR_RIGHT, ANCHOR_STRETCH };

static int g_inHud;                  // > 0 while DrawHud runs
static int g_inCentered;             // > 0 while a centered HUD part (texts, pause box) is drawn
static bool g_hudThisFrame;          // DrawHud ran since the last Present: a level frame
static bool g_menu = true;           // the previous frame was a menu
static int g_inBackground;           // > 0 while DrawSky runs
static Anchor g_pending = ANCHOR_NONE; // chosen by the DrawSprite* hook for the QueueSprite call it makes

typedef void(__cdecl *QueueSpriteFn)(float x, float y, float w, float h, void *uv0, void *uv1, int texture,
                                       DWORD color, int flags);
static QueueSpriteFn g_queueOriginal;
static void *g_drawHudOriginal;
static void *g_drawOriginal[5];

// DrawHud's top centre block (race timer, position, XR pieces): drawn by the calls between these addresses
static const DWORD kTopCentreFrom = A(0x004027ae), kTopCentreTo = A(0x00402ad1);

// Our own HUD parts that belong to the centre (patches/hud/hud.cpp: the gold clock next to the game's)
extern "C" void HudCentered(int on)
{
	g_inCentered += on ? 1 : -1;
}

// The anchor of the next sprite, from its sheet, its x on the virtual screen and the DrawHud call it comes from;
// sheets 0x24-0x27: villain bar and arrow
extern "C" void __cdecl ChooseHudAnchor(int x, int sheet, int virtualWidth, DWORD caller)
{
	if (!g_inHud) {
		g_pending = ANCHOR_NONE;
	} else if (g_inCentered || (caller > kTopCentreFrom && caller < kTopCentreTo)) {
		g_pending = ANCHOR_CENTER;
	} else if ((short)sheet >= 0x24 && (short)sheet <= 0x27) {
		g_pending = ANCHOR_CENTER;
	} else {
		g_pending = (short)x < virtualWidth / 2 ? ANCHOR_LEFT : ANCHOR_RIGHT;
	}
}

static void __cdecl QueueSprite_Wide(float x, float y, float w, float h, void *uv0, void *uv1, int texture,
                                       DWORD color, int flags)
{
	int sw = g_render.Width(), sh = g_render.Height();
	Anchor a = g_pending;
	g_pending = ANCHOR_NONE;
	if (sw * 3 > sh * 4) { // wider than 4:3
		if (a == ANCHOR_NONE) {
			a = !g_menu && (g_inBackground || (x <= 0.01f && x + w >= 0.99f)) ? ANCHOR_STRETCH : ANCHOR_CENTER;
		}
		float k = sh * 4.f / 3.f / sw; // 4:3 width as a fraction of the screen width
		switch (a) {
		case ANCHOR_LEFT: x = x * k; break;
		case ANCHOR_RIGHT: x = 1 - (1 - x) * k; break;
		case ANCHOR_CENTER: x = 0.5f + (x - 0.5f) * k; break;
		default: break;
		}
		if (a != ANCHOR_STRETCH) {
			w *= k;
		}
	}
	g_queueOriginal(x, y, w, h, uv0, uv1, texture, color, flags);
}

extern "C" void DrawHudExtras(void); // patches/hud/hud.cpp

// DrawHud(void): mark the HUD while it runs, and add our HUD parts at its end
static __declspec(naked) void DrawHud_Wide()
{
	__asm {
		mov g_hudThisFrame, 1
		inc g_inHud
		call g_drawHudOriginal
		call DrawHudExtras
		dec g_inHud
		ret
	}
}

// DrawSky(int mode): the sky in tiles
typedef void(__cdecl *BackgroundFn)(int mode);
static BackgroundFn g_backgroundOriginal;
static void __cdecl DrawSky_Wide(int mode)
{
	g_inBackground++;
	g_backgroundOriginal(mode);
	g_inBackground--;
}

// DrawCenteredText(int y, char *text, byte r, byte g, byte b, int hiRes): one centered line of text
typedef void(__cdecl *DrawCenteredTextFn)(int y, const char *text, int r, int g, int b, int hiRes);
static DrawCenteredTextFn g_centeredTextOriginal;
static void __cdecl DrawCenteredText_Wide(int y, const char *text, int r, int g, int b, int hiRes)
{
	g_inCentered++;
	g_centeredTextOriginal(y, text, r, g, b, hiRes);
	g_inCentered--;
}

// DrawPanelBox: the box behind the pause menu (7 arguments)
typedef void(__cdecl *DrawBoxFn)(int a, int b, int c, int d, int e, int f, int g);
static DrawBoxFn g_drawBoxOriginal;
extern "C" void PauseMenuExtras(void); // patches/menus/menu.cpp: our lines go in before the box, so they show over it

static void __cdecl DrawPanelBox_Wide(int a, int b, int c, int d, int e, int f, int g)
{
	PauseMenuExtras();
	g_inCentered++;
	g_drawBoxOriginal(a, b, c, d, e, f, g);
	g_inCentered--;
}

// End of a frame: menus get black bars outside 4:3
static void BeforePresent()
{
	if (!g_hudThisFrame) {
		g_render.MaskOutside4x3();
	}
	g_menu = !g_hudThisFrame;
	g_render.blitTo4x3 = g_menu;
	g_hudThisFrame = false;
}

// DrawSprite*(short x, short y, short sheet, ...): return address at [esp], x at [esp+4], sheet at [esp+0xc]. Choose
// the anchor, then run the original with the stack untouched (any number of arguments).
#define DRAW_HOOK(n, vw)                                                                 \
	static __declspec(naked) void DrawHook##n()                                          \
	{                                                                                    \
		__asm push dword ptr [esp]                                                       \
		__asm push vw                                                                    \
		__asm push dword ptr [esp + 0x14]                                                \
		__asm push dword ptr [esp + 0x10]                                                \
		__asm call ChooseHudAnchor                                                       \
		__asm add esp, 0x10                                                              \
		__asm jmp g_drawOriginal[n * 4]                                                  \
	}
DRAW_HOOK(0, 512)
DRAW_HOOK(1, 320)
DRAW_HOOK(2, 512)
DRAW_HOOK(3, 512)
DRAW_HOOK(4, 512)

extern "C" void InstallWidescreen(void)
{
	// First instruction of each function (5 bytes, position-independent; checked with a disassembler)
	static const unsigned char queueBytes[] = {0xa1, 0xac, 0x6b, 0x50, 0x00};    // mov eax, [00506bac]
	static const unsigned char hudBytes[] = {0xa1, 0xe4, 0x5b, 0x53, 0x00};      // mov eax, [00535be4]
	static const unsigned char movsxBytes[] = {0x0f, 0xbf, 0x44, 0x24, 0x0c};    // movsx eax, word [esp+0xc]
	static const unsigned char movBytes[] = {0x66, 0x8b, 0x44, 0x24, 0x0c};      // mov ax, word [esp+0xc]
	g_queueOriginal = (QueueSpriteFn)HookTrampoline(A(0x004bc6a0), (const void *)QueueSprite_Wide, queueBytes, 5,
	                                                  "widescreen 2D (QueueSprite)");
	if (!g_queueOriginal) {
		return;
	}
	g_drawHudOriginal = HookTrampoline(A(0x00401960), (const void *)DrawHud_Wide, hudBytes, 5, "widescreen HUD (DrawHud)");
	static const unsigned char stringBytes[] = {0x8b, 0x44, 0x24, 0x18, 0x33, 0xc9}; // mov eax, [esp+0x18]; xor ecx, ecx
	static const unsigned char boxBytes[] = {0x53, 0x8b, 0x5c, 0x24, 0x14};           // push ebx; mov ebx, [esp+0x14]
	g_centeredTextOriginal = (DrawCenteredTextFn)HookTrampoline(A(0x00405a00), (const void *)DrawCenteredText_Wide,
	                                                            stringBytes, 6, "HUD centered text (DrawCenteredText)");
	g_drawBoxOriginal = (DrawBoxFn)HookTrampoline(A(0x004055f0), (const void *)DrawPanelBox_Wide, boxBytes, 5,
	                                              "HUD centered box (DrawPanelBox)");
	static const unsigned char backgroundBytes[] = {0xa1, 0x54, 0xf6, 0x4f, 0x00}; // mov eax, [004ff654]
	g_backgroundOriginal = (BackgroundFn)HookTrampoline(A(0x00491840), (const void *)DrawSky_Wide,
	                                                    backgroundBytes, 5, "widescreen sky (DrawSky)");
	g_render.AddBeforePresent(BeforePresent);
	g_render.blitTo4x3 = true;
	g_drawOriginal[0] = HookTrampoline(A(0x00496ea0), (const void *)DrawHook0, movsxBytes, 5, "HUD anchor (DrawSprite)");
	g_drawOriginal[1] = HookTrampoline(A(0x00496bf0), (const void *)DrawHook1, movsxBytes, 5, "HUD anchor (DrawSprite320)");
	g_drawOriginal[2] = HookTrampoline(A(0x00496080), (const void *)DrawHook2, movsxBytes, 5, "HUD anchor (DrawSpriteTinted)");
	g_drawOriginal[3] = HookTrampoline(A(0x00495e90), (const void *)DrawHook3, movsxBytes, 5, "HUD anchor (DrawSpriteBlend)");
	g_drawOriginal[4] = HookTrampoline(A(0x004965d0), (const void *)DrawHook4, movBytes, 5, "HUD anchor (DrawSpriteScaled)");
}
