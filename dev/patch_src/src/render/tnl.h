// CPU transform and lighting, like DirectX 6's software pipeline (D3DVOP_TRANSFORM | D3DVOP_LIGHT).
// One implementation for DrawPrimitive (outputs clip space) and ProcessVertices (outputs screen space).
#pragma once
#include "render.h"
#include <vector>

// Where each part of a vertex is, from its D3DFVF_* flags
struct FvfLayout {
	int stride = 0;
	bool rhw = false;   // D3DFVF_XYZRHW: already transformed
	int normal = -1;    // byte offsets (-1 = not present)
	int diffuse = -1;
	int specular = -1;
	int texCount = 0;
	int tex = -1;
};
FvfLayout DecodeFvf(DWORD fvf);

struct LightState {
	D3DLIGHT2 light = {};
	bool active = false;
};

// Fixed-function state the CPU side needs
struct TnlState {
	Mat4 world, view, proj;
	D3DVIEWPORT2 viewport = {};
	D3DMATERIAL material = {};
	DWORD ambient = 0;
	bool colorVertex = true;
	bool specular = false;
	int fogMode = 0; // D3DLIGHTSTATE_FOGMODE (vertex fog)
	float fogStart = 0, fogEnd = 1, fogDensity = 1;
	std::vector<const LightState *> lights;
	int rtWidth = 640, rtHeight = 480; // back buffer size (for pre-transformed vertices)
};

// Converts game vertices into GPU vertices (clip space). 'light' = apply lighting.
void TransformToClip(const TnlState &s, const BYTE *src, const FvfLayout &fl, int count, bool light, GpuVertex *out);

// ProcessVertices: writes the result into a game-format buffer (usually D3DTLVERTEX)
void ProcessToScreen(const TnlState &s, const BYTE *src, const FvfLayout &srcFl, int count, bool light, BYTE *dst,
                     const FvfLayout &dstFl);
