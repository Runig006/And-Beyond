// Direct3D 11 side of the engine: window, the game's back buffer, the fixed-function pixel shader and presentation.
#pragma once
#include "common.h"

// Vertex as the GPU receives it: already in clip space (the CPU did transform and lighting, see tnl.cpp).
struct GpuVertex {
	float x, y, z, w;
	uint32_t diffuse, specular; // D3DCOLOR 0xAARRGGBB
	float u0, v0, u1, v1;
	float fogz; // depth used by table fog
};

// Pixel shader constants (same order as the cbuffer in shader.hlsl)
struct PsConsts {
	int32_t stage0[4]; // colorOp, colorArg1, colorArg2, 0
	int32_t alpha0[4]; // alphaOp, alphaArg1, alphaArg2, 0
	int32_t stage1[4];
	int32_t alpha1[4];
	float tfactor[4];
	float fogColor[4];
	float fogParams[4]; // start, end, density, mode (0 per vertex, 1 exp, 2 exp2, 3 linear)
	int32_t flags[4];   // fog, alpha test function (0 = off), specular, color key on texture 0
	float alphaRef[4];
	int32_t texOn[4];   // stage 0 / 1 have a texture
	// See-through around Buzz (filled by Render::Draw from SeeThrough, see shader.hlsl)
	float seeBuzz[4];   // Buzz's centre in view space, radius
	float seeProj[4];   // projection: [0][0], [1][1], [2][2], [3][2]
	float seeProj2[4];  // projection [2][0], [2][1]; margin; 1 = on for this draw
	float seeVp[4];     // viewport x, y, width, height (pixels)
};

// What the see-through effect needs for one frame, given by a patch (patches/graphics/seethrough.cpp)
struct SeeThrough {
	float buzz[3];   // Buzz's centre in view space
	float radius;    // of the see-through tube from the camera to Buzz
	float margin;    // only what is at least this much nearer the camera than Buzz
	float proj[16];  // projection matrix (row vectors)
};

struct GpuState {
	bool zEnable = true, zWrite = true;
	int zFunc = D3DCMP_LESSEQUAL;
	bool blend = false;
	int srcBlend = D3DBLEND_ONE, dstBlend = D3DBLEND_ZERO;
	int cull = D3DCULL_CCW;
	bool wireframe = false;
	int filter[2] = {D3DTFG_LINEAR, D3DTFG_LINEAR}; // per stage: 1 point, 2 linear
	int addrU[2] = {D3DTADDRESS_WRAP, D3DTADDRESS_WRAP}, addrV[2] = {D3DTADDRESS_WRAP, D3DTADDRESS_WRAP};
	ID3D11ShaderResourceView *tex[2] = {};
	D3D11_VIEWPORT viewport = {};
	RECT scissor = {};
	PsConsts ps = {};
};

struct GpuTexture {
	ID3D11Texture2D *tex = nullptr;
	ID3D11ShaderResourceView *srv = nullptr;
	int w = 0, h = 0;
};

class Render {
public:
	bool Init(HWND hwnd);
	void SetWindow(HWND hwnd); // follows the window the game gives to DirectDraw
	void ApplyWindowMode();   // borderless fullscreen or normal window (saved setting / Alt+Enter)
	bool Ready() const { return dev_ != nullptr; }
	HWND Window() const { return hwnd_; }

	// Size of the back buffer the game sees (the chosen display mode)
	void SetMode(int w, int h, bool fullscreen);
	int Width() const { return w_; }
	int Height() const { return h_; }

	void Clear(const D3DRECT *rects, int n, DWORD flags, DWORD color, float z);
	void Draw(D3D_PRIMITIVE_TOPOLOGY topo, const GpuVertex *v, int nv, const uint16_t *idx, int ni,
	          const GpuState &st);
	// Copies a texture rectangle to the back buffer (Blt/BltFast from 2D surfaces)
	void Blit(ID3D11ShaderResourceView *srv, int sw, int sh, const RECT &src, const RECT &dst, bool keyed,
	          bool alphaBlend = false); // alphaBlend: mixed by its alpha (overlays)
	void Present();
	// Patches that finish the frame (overlays, bars) register here; called in order at the start of Present
	void AddBeforePresent(void (*fn)()) { if (beforePresentCount_ < 8) {
		beforePresent_[beforePresentCount_++] = fn;
	} }
	// See-through source: fills the frame's values and returns true while the effect applies (game in play)
	bool (*seeThrough)(SeeThrough &out) = nullptr;
	// Solid rectangle on the game's frame, in its pixels (overlays such as the aiming reticle)
	void FillRect(int x1, int y1, int x2, int y2, DWORD argb);
	// Paints black the parts of the frame outside a centered 4:3 area (menus on wide screens)
	void MaskOutside4x3();
	// Screen copies (Blit: videos, full-screen pictures) go into the centered 4:3 area instead of the whole width
	bool blitTo4x3 = false;
	// The frame was paced by the game clock (patches/system/framerate.cpp): show it without waiting for vsync
	bool paced = false;

	// Textures of the memory surfaces (BGRA8)
	GpuTexture CreateTexture(int w, int h);
	void Upload(GpuTexture &t, const uint32_t *bgra, int pitchPixels);
	void Free(GpuTexture &t);
	// CPU access to the back buffer (Lock/GetDC on the screen): read it, write it back
	void ReadBack(uint32_t *out, int pitchPixels);
	void WriteBack(const uint32_t *in, int pitchPixels, const RECT &rect);

private:
	bool CreateSwapChain(HWND hwnd);
	void CreateTargets();
	void ResizeSwapChainIfNeeded();
	ID3D11BlendState *Blend(const GpuState &st);
	ID3D11DepthStencilState *Depth(const GpuState &st);
	ID3D11RasterizerState *Raster(const GpuState &st);
	ID3D11SamplerState *Sampler(int filter, int u, int v);
	void EnsureBuffers(int vbytes, int ibytes);

	void (*beforePresent_[8])() = {};
	int beforePresentCount_ = 0;
	HWND hwnd_ = nullptr;
	int w_ = 0, h_ = 0;
	ID3D11Device *dev_ = nullptr;
	ID3D11DeviceContext *ctx_ = nullptr;
	IDXGISwapChain *swap_ = nullptr;
	int swapW_ = 0, swapH_ = 0;
	ID3D11Texture2D *rt_ = nullptr;
	ID3D11RenderTargetView *rtv_ = nullptr;
	ID3D11ShaderResourceView *rtSrv_ = nullptr;
	ID3D11Texture2D *staging_ = nullptr;
	ID3D11Texture2D *depth_ = nullptr;
	ID3D11DepthStencilView *dsv_ = nullptr;
	ID3D11VertexShader *vs_ = nullptr;
	ID3D11PixelShader *ps_ = nullptr;
	ID3D11InputLayout *layout_ = nullptr;
	ID3D11VertexShader *blitVs_ = nullptr;
	ID3D11PixelShader *blitPs_ = nullptr;
	ID3D11Buffer *cb_ = nullptr;
	ID3D11Buffer *vb_ = nullptr, *ib_ = nullptr;
	int vbBytes_ = 0, ibBytes_ = 0;
};

extern Render g_render;
