// The modern controls' aiming reticle (placed by patches/input/controls.cpp): a green dot outlined in purple that turns red
// and pulses when the auto-aim locks an enemy. Drawn each frame into a small texture at the exact pixel size.
#include "render.h"
#include <math.h>

static const float kLockSpeed = 8; // how fast it changes to / from locked (per second)

struct Rgb { float r, g, b; };
static const Rgb kGreen = {120, 230, 60}, kRed = {255, 60, 40}, kPurple = {110, 40, 160};

static GpuTexture g_image;
static uint32_t *g_pixels;
static float g_lock; // 0 = free, 1 = locked (eased)
static LARGE_INTEGER g_last;

static float Clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

void DrawReticleAt(float cx, float cy, bool locked)
{
	int size = g_render.Height() / 90 | 1; // odd, so the centre falls on a pixel
	if (size < 7) {
		size = 7;
	}
	if (g_image.w != size) {
		g_render.Free(g_image);
		g_image = g_render.CreateTexture(size, size);
		delete[] g_pixels;
		g_pixels = new uint32_t[size * size];
	}

	// Animation, by real time
	LARGE_INTEGER now, freq;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&freq);
	float dt = g_last.QuadPart ? (float)(now.QuadPart - g_last.QuadPart) / freq.QuadPart : 0;
	if (dt > 0.1f) {
		dt = 0.1f;
	}
	g_last = now;
	float step = kLockSpeed * dt;
	g_lock += locked ? (g_lock + step > 1 ? 1 - g_lock : step) : (g_lock - step < 0 ? -g_lock : -step);
	float t = (float)now.QuadPart / freq.QuadPart;
	float pulse = 1 + g_lock * 0.15f * sinf(t * 9);

	float half = size / 2.f;
	float O = size * 0.12f + 0.75f;          // purple outline
	float dot = (half - O - 0.5f) * pulse;   // radius of the coloured part
	if (dot > half - O) {
		dot = half - O;
	}
	Rgb c = {kGreen.r + (kRed.r - kGreen.r) * g_lock, kGreen.g + (kRed.g - kGreen.g) * g_lock,
	         kGreen.b + (kRed.b - kGreen.b) * g_lock};

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			float dx = x + 0.5f - half, dy = y + 0.5f - half, r = sqrtf(dx * dx + dy * dy);
			float inner = Clamp01(dot + 0.5f - r), outer = Clamp01(dot + O + 0.5f - r);
			// Colour over the outline: alpha = outer; colour mixes purple -> dot colour by inner / outer
			float k = outer > 0 ? inner / outer : 0;
			float rr = kPurple.r + (c.r - kPurple.r) * k, gg = kPurple.g + (c.g - kPurple.g) * k,
			      bb = kPurple.b + (c.b - kPurple.b) * k;
			g_pixels[y * size + x] = ((uint32_t)(outer * 255) << 24) | ((uint32_t)rr << 16) | ((uint32_t)gg << 8) |
			                         (uint32_t)bb;
		}
	}
	g_render.Upload(g_image, g_pixels, size);

	RECT src = {0, 0, size, size};
	int left = (int)(cx - half + 0.5f), top = (int)(cy - half + 0.5f);
	RECT dst = {left, top, left + size, top + size};
	g_render.Blit(g_image.srv, size, size, src, dst, false, true);
}
