// See-through walls (PATCH OPTIONS > see through): solid 3D between the camera and Buzz is dithered away by the
// shader (SeeThroughHole); this gives it Buzz's position in view space. Only during play: RenderFrame (004444d0)
// called from UpdateGameFrame / UpdatePausedFrame, not during camera scenes (g_cameraScript bit 0).
#include "common.h"
#include "config.h"
#include "render.h"

#define g_playerX (*(int *)A(0x00535c10))
#define g_playerY (*(int *)A(0x00535c14)) // grows downwards
#define g_playerZ (*(int *)A(0x00535c18))
#define g_cameraScript (*(unsigned short *)A(0x00531676))
#define g_moveFlags (*(unsigned int *)A(0x00541160)) // 0x10 wings, 8 bike, 4 board
#define g_viewMatrix ((float *)A(0x00a5aa78))       // set by SetCameraMatrices (RenderWorld) before the frame is drawn
#define g_projectionMatrix ((float *)A(0x00a5a938))

static const int kBuzzMiddle = 0x2000; // Buzz's centre above his feet (the follow camera looks at 0x3000: his head)
static const float kRadius = 0x2400 / 32.f; // the renderer gets positions / 32
static const float kMargin = 0x2000 / 32.f;
static const float kMarginVehicle = 0x5000 / 32.f; // the bike and the board reach well behind Buzz: they stay solid

static void(__cdecl *g_renderFrameOriginal)(int);
static bool g_inPlay;

static void __cdecl RenderFrame_SeeThrough(int actors)
{
	DWORD caller = (DWORD)_ReturnAddress();
	g_inPlay = caller >= A(0x004323e0) && caller < A(0x004329df); // UpdateGameFrame, UpdatePausedFrame
	g_renderFrameOriginal(actors);
	g_inPlay = false;
}

// RenderFrame is drawing a frame of play (also for patches/graphics/drawdistance.cpp: no fog near in the level select)
extern "C" bool RenderingPlay() { return g_inPlay; }

static bool SeeThroughSource(SeeThrough &out)
{
	if (!g_inPlay || !g_config.seeThrough || (g_cameraScript & 1)) {
		return false;
	}
	float v[4] = {g_playerX / 32.f, (g_playerY - kBuzzMiddle) / 32.f, g_playerZ / 32.f, 1};
	for (int c = 0; c < 3; c++) { // row vector x view matrix
		float s = 0;
		for (int k = 0; k < 4; k++) {
			s += v[k] * g_viewMatrix[k * 4 + c];
		}
		out.buzz[c] = s;
	}
	float margin = (g_moveFlags & 0x1c) ? kMarginVehicle : kMargin;
	if (out.buzz[2] <= margin) {
		return false; // camera too close (or behind): nothing can be in between
	}
	out.radius = kRadius;
	out.margin = margin;
	memcpy(out.proj, g_projectionMatrix, sizeof out.proj);
	return true;
}

extern "C" void InstallSeeThrough(void)
{
	static const unsigned char expect[] = {0xa1, 0x60, 0x11, 0x54, 0x00}; // mov eax, [00541160]
	g_renderFrameOriginal = (void(__cdecl *)(int))HookTrampoline(A(0x004444d0), (const void *)RenderFrame_SeeThrough,
	                                                              expect, sizeof expect, "see-through walls (RenderFrame)");
	g_render.seeThrough = SeeThroughSource;
}
