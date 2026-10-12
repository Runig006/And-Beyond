// Effects that are wrong on PC:
//   - Bike exhaust flame (UpdatePlayerEffects 004349c0): spawned well below the PC model's pipe; y -100 -> -320.
//   - Price rings (DrawItemPriceRing 0044b230): the shared angle advanced on every call, so several rings spun too
//     fast; now only the first call of a frame advances it (the others run as if paused).
//   - PATCH OPTIONS > price blink: the digits (white, QueueQuad 004bcc70) blink red with the HUD blink
//     while Buzz cannot pay.
#include "common.h"
#include "config.h"
#include "render.h"

#define g_paused (*(short *)A(0x00531678))
#define g_credits (*(short *)A(0x00535cae)) // g_player.credits
#define g_blinkTimer32 (*(unsigned char *)A(0x0052ddc2))

typedef void(__cdecl *PriceRingFn)(float x, float y, float z, unsigned price);
static PriceRingFn g_priceRingOriginal;
static unsigned g_frame, g_ringFrame = ~0u; // frames presented; the frame the ring last turned in

static void NextFrame() { g_frame++; }

typedef void(__cdecl *QueueQuadFn)(void *verts, void *uv, void *a, int texture, DWORD color, int flags);
static QueueQuadFn g_queueQuadOriginal;
static DWORD g_ringColour; // 0: not inside DrawItemPriceRing

static void __cdecl QueueQuad_RingColour(void *verts, void *uv, void *a, int texture, DWORD color, int flags)
{
	if (g_ringColour && color == 0xffffffff) {
		color = g_ringColour;
	}
	g_queueQuadOriginal(verts, uv, a, texture, color, flags);
}

static void __cdecl PriceRing_OncePerFrame(float x, float y, float z, unsigned price)
{
	// The digits' colour for QueueQuad_RingColour while this call runs
	struct Colour {
		Colour(unsigned price)
		{
			g_ringColour = g_config.priceBlink && g_credits < (int)price && g_blinkTimer32 < 0x10 ? 0xffff2020 : 0;
		}
		~Colour() { g_ringColour = 0; }
	} colour(price);
	if (g_ringFrame != g_frame) {
		g_ringFrame = g_frame;
		g_priceRingOriginal(x, y, z, price);
		return;
	}
	short paused = g_paused;
	g_paused = 1;
	g_priceRingOriginal(x, y, z, price);
	g_paused = paused;
}

extern "C" void InstallEffects(void)
{
	static const unsigned char ringBytes[] = {0x81, 0xec, 0xc8, 0x01, 0x00, 0x00}; // sub esp, 0x1c8
	g_priceRingOriginal = (PriceRingFn)HookTrampoline(A(0x0044b230), (const void *)PriceRing_OncePerFrame, ringBytes, 6,
	                                                  "price ring spin once per frame (DrawItemPriceRing)");
	g_render.AddBeforePresent(NextFrame);
	static const unsigned char queueBytes[] = {0xa1, 0xb0, 0x6b, 0x50, 0x00}; // mov eax, [g_worldSpriteFree]
	if (g_priceRingOriginal) {
		g_queueQuadOriginal = (QueueQuadFn)HookTrampoline(A(0x004bcc70), (const void *)QueueQuad_RingColour, queueBytes, 5,
		                                                  "price ring blink (QueueQuad)");
	}
	static const unsigned char exhaustY[] = {0x9c, 0xff, 0xff, 0xff}; // mov [esp+30], -100
	static const unsigned char raised[] = {0xc0, 0xfe, 0xff, 0xff};   // -320
	PatchBytes(A(0x00434b56), exhaustY, raised, 4, "bike exhaust height (UpdatePlayerEffects)");
}
