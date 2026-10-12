// The DirectX 6 COM objects the game talks to (DirectDraw 4, Direct3D 3 and the old versions it asks for), drawn with
// Direct3D 11 by render/.
#pragma once
#include "tnl.h"
#include <vector>

class Surface;
class Device;
class Viewport;

// DirectX 6 handle table (materials and textures are referred to by number)
DWORD HandleNew(void *obj);
void *HandleGet(DWORD h);
void HandleFree(DWORD h);

// ---------------------------------------------------------------- DirectDraw + Direct3D (one object)
class DDraw : public IDirectDraw4, public IDirect3D3 {
public:
	DDraw();
	STDMETHOD(QueryInterface)(REFIID riid, void **out) override;
	COM_REFCOUNT

	// IDirectDraw4
	STDMETHOD(Compact)() override;
	STDMETHOD(CreateClipper)(DWORD, LPDIRECTDRAWCLIPPER *, IUnknown *) override;
	STDMETHOD(CreatePalette)(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown *) override;
	STDMETHOD(CreateSurface)(LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE4 *, IUnknown *) override;
	STDMETHOD(DuplicateSurface)(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 *) override;
	STDMETHOD(EnumDisplayModes)(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2) override;
	STDMETHOD(EnumSurfaces)(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK2) override;
	STDMETHOD(FlipToGDISurface)() override;
	STDMETHOD(GetCaps)(LPDDCAPS, LPDDCAPS) override;
	STDMETHOD(GetDisplayMode)(LPDDSURFACEDESC2) override;
	STDMETHOD(GetFourCCCodes)(LPDWORD, LPDWORD) override;
	STDMETHOD(GetGDISurface)(LPDIRECTDRAWSURFACE4 *) override;
	STDMETHOD(GetMonitorFrequency)(LPDWORD) override;
	STDMETHOD(GetScanLine)(LPDWORD) override;
	STDMETHOD(GetVerticalBlankStatus)(LPBOOL) override;
	STDMETHOD(Initialize)(GUID *) override;
	STDMETHOD(RestoreDisplayMode)() override;
	STDMETHOD(SetCooperativeLevel)(HWND, DWORD) override;
	STDMETHOD(SetDisplayMode)(DWORD, DWORD, DWORD, DWORD, DWORD) override;
	STDMETHOD(WaitForVerticalBlank)(DWORD, HANDLE) override;
	STDMETHOD(GetAvailableVidMem)(LPDDSCAPS2, LPDWORD, LPDWORD) override;
	STDMETHOD(GetSurfaceFromDC)(HDC, LPDIRECTDRAWSURFACE4 *) override;
	STDMETHOD(RestoreAllSurfaces)() override;
	STDMETHOD(TestCooperativeLevel)() override;
	STDMETHOD(GetDeviceIdentifier)(LPDDDEVICEIDENTIFIER, DWORD) override;

	// IDirect3D3
	STDMETHOD(EnumDevices)(LPD3DENUMDEVICESCALLBACK, LPVOID) override;
	STDMETHOD(CreateLight)(LPDIRECT3DLIGHT *, LPUNKNOWN) override;
	STDMETHOD(CreateMaterial)(LPDIRECT3DMATERIAL3 *, LPUNKNOWN) override;
	STDMETHOD(CreateViewport)(LPDIRECT3DVIEWPORT3 *, LPUNKNOWN) override;
	STDMETHOD(FindDevice)(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT) override;
	STDMETHOD(CreateDevice)(REFCLSID, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3 *, LPUNKNOWN) override;
	STDMETHOD(CreateVertexBuffer)(LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER *, DWORD, LPUNKNOWN) override;
	STDMETHOD(EnumZBufferFormats)(REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID) override;
	STDMETHOD(EvictManagedTextures)() override;

	int modeW = 0, modeH = 0, modeBpp = 16;
	HWND hwnd = nullptr;
	Surface *primary = nullptr;
};

// DirectDraw version 1 (what DirectDrawCreate returns): only the basics, the rest goes to the version 4 object
class DDraw1 : public IDirectDraw {
public:
	explicit DDraw1(DDraw *dd) : dd_(dd) { dd_->AddRef(); }
	~DDraw1() { dd_->Release(); }
	STDMETHOD(QueryInterface)(REFIID riid, void **out) override;
	COM_REFCOUNT
	STDMETHOD(Compact)() override { return DD_OK; }
	STDMETHOD(CreateClipper)(DWORD f, LPDIRECTDRAWCLIPPER *c, IUnknown *u) override { return dd_->CreateClipper(f, c, u); }
	STDMETHOD(CreatePalette)(DWORD f, LPPALETTEENTRY e, LPDIRECTDRAWPALETTE *p, IUnknown *u) override { return dd_->CreatePalette(f, e, p, u); }
	STDMETHOD(CreateSurface)(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) override;
	STDMETHOD(DuplicateSurface)(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) override { NOT_IMPL("DD1::DuplicateSurface"); return DDERR_UNSUPPORTED; }
	STDMETHOD(EnumDisplayModes)(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) override;
	STDMETHOD(EnumSurfaces)(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) override { NOT_IMPL("DD1::EnumSurfaces"); return DDERR_UNSUPPORTED; }
	STDMETHOD(FlipToGDISurface)() override { return DD_OK; }
	STDMETHOD(GetCaps)(LPDDCAPS a, LPDDCAPS b) override { return dd_->GetCaps(a, b); }
	STDMETHOD(GetDisplayMode)(LPDDSURFACEDESC) override;
	STDMETHOD(GetFourCCCodes)(LPDWORD n, LPDWORD c) override { return dd_->GetFourCCCodes(n, c); }
	STDMETHOD(GetGDISurface)(LPDIRECTDRAWSURFACE *) override { NOT_IMPL("DD1::GetGDISurface"); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetMonitorFrequency)(LPDWORD f) override { return dd_->GetMonitorFrequency(f); }
	STDMETHOD(GetScanLine)(LPDWORD l) override { return dd_->GetScanLine(l); }
	STDMETHOD(GetVerticalBlankStatus)(LPBOOL b) override { return dd_->GetVerticalBlankStatus(b); }
	STDMETHOD(Initialize)(GUID *g) override { return DD_OK; }
	STDMETHOD(RestoreDisplayMode)() override { return dd_->RestoreDisplayMode(); }
	STDMETHOD(SetCooperativeLevel)(HWND h, DWORD f) override { return dd_->SetCooperativeLevel(h, f); }
	STDMETHOD(SetDisplayMode)(DWORD w, DWORD h, DWORD b) override { return dd_->SetDisplayMode(w, h, b, 0, 0); }
	STDMETHOD(WaitForVerticalBlank)(DWORD f, HANDLE e) override { return DD_OK; }

private:
	DDraw *dd_;
};

// IDirectDraw2 and IDirect3D2 (ddraw_old.cpp)
IUnknown *NewDDraw2(DDraw *dd);
IUnknown *NewD3D2(DDraw *dd);

// Surface from a pointer the game passes as texture (IDirect3DTexture2 or, by mistake, IDirectDrawSurface4)
Surface *SurfaceFromTexture(void *p, const char *who);

// ---------------------------------------------------------------- Surfaces (screen, back buffer, Z, textures, 2D)
enum SurfKind { SK_PRIMARY, SK_BACK, SK_ZBUFFER, SK_CPU };

class Palette : public IDirectDrawPalette {
public:
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(GetCaps)(LPDWORD c) override { *c = DDPCAPS_8BIT; return DD_OK; }
	STDMETHOD(GetEntries)(DWORD, DWORD, DWORD, LPPALETTEENTRY) override;
	STDMETHOD(Initialize)(LPDIRECTDRAW, DWORD, LPPALETTEENTRY) override { return DD_OK; }
	STDMETHOD(SetEntries)(DWORD, DWORD, DWORD, LPPALETTEENTRY) override;
	PALETTEENTRY entries[256] = {};
	int version = 0;
};

class Clipper : public IDirectDrawClipper {
public:
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(GetClipList)(LPRECT, LPRGNDATA, LPDWORD) override { NOT_IMPL("Clipper::GetClipList"); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetHWnd)(HWND *h) override { *h = hwnd; return DD_OK; }
	STDMETHOD(Initialize)(LPDIRECTDRAW, DWORD) override { return DD_OK; }
	STDMETHOD(IsClipListChanged)(BOOL *b) override { *b = FALSE; return DD_OK; }
	STDMETHOD(SetClipList)(LPRGNDATA, DWORD) override { return DD_OK; }
	STDMETHOD(SetHWnd)(DWORD, HWND h) override { hwnd = h; return DD_OK; }
	HWND hwnd = nullptr;
};

class Surface : public IDirectDrawSurface4, public IDirect3DTexture2 {
public:
	Surface(DDraw *dd, SurfKind kind, const DDSURFACEDESC2 &desc);
	~Surface();
	STDMETHOD(QueryInterface)(REFIID riid, void **out) override;
	COM_REFCOUNT

	// IDirectDrawSurface4
	STDMETHOD(AddAttachedSurface)(LPDIRECTDRAWSURFACE4) override;
	STDMETHOD(AddOverlayDirtyRect)(LPRECT) override { return DD_OK; }
	STDMETHOD(Blt)(LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDBLTFX) override;
	STDMETHOD(BltBatch)(LPDDBLTBATCH, DWORD, DWORD) override { NOT_IMPL("Surface::BltBatch"); return DDERR_UNSUPPORTED; }
	STDMETHOD(BltFast)(DWORD, DWORD, LPDIRECTDRAWSURFACE4, LPRECT, DWORD) override;
	STDMETHOD(DeleteAttachedSurface)(DWORD, LPDIRECTDRAWSURFACE4) override;
	STDMETHOD(EnumAttachedSurfaces)(LPVOID, LPDDENUMSURFACESCALLBACK2) override;
	STDMETHOD(EnumOverlayZOrders)(DWORD, LPVOID, LPDDENUMSURFACESCALLBACK2) override { return DD_OK; }
	STDMETHOD(Flip)(LPDIRECTDRAWSURFACE4, DWORD) override;
	STDMETHOD(GetAttachedSurface)(LPDDSCAPS2, LPDIRECTDRAWSURFACE4 *) override;
	STDMETHOD(GetBltStatus)(DWORD) override { return DD_OK; }
	STDMETHOD(GetCaps)(LPDDSCAPS2) override;
	STDMETHOD(GetClipper)(LPDIRECTDRAWCLIPPER *) override;
	STDMETHOD(GetColorKey)(DWORD, LPDDCOLORKEY) override;
	STDMETHOD(GetDC)(HDC *) override;
	STDMETHOD(GetFlipStatus)(DWORD) override { return DD_OK; }
	STDMETHOD(GetOverlayPosition)(LPLONG, LPLONG) override { return DDERR_NOTAOVERLAYSURFACE; }
	STDMETHOD(GetPalette)(LPDIRECTDRAWPALETTE *) override;
	STDMETHOD(GetPixelFormat)(LPDDPIXELFORMAT) override;
	STDMETHOD(GetSurfaceDesc)(LPDDSURFACEDESC2) override;
	STDMETHOD(Initialize)(LPDIRECTDRAW, LPDDSURFACEDESC2) override { return DDERR_ALREADYINITIALIZED; }
	STDMETHOD(IsLost)() override { return DD_OK; }
	STDMETHOD(Lock)(LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE) override;
	STDMETHOD(ReleaseDC)(HDC) override;
	STDMETHOD(Restore)() override { return DD_OK; }
	STDMETHOD(SetClipper)(LPDIRECTDRAWCLIPPER) override;
	STDMETHOD(SetColorKey)(DWORD, LPDDCOLORKEY) override;
	STDMETHOD(SetOverlayPosition)(LONG, LONG) override { return DDERR_NOTAOVERLAYSURFACE; }
	STDMETHOD(SetPalette)(LPDIRECTDRAWPALETTE) override;
	STDMETHOD(Unlock)(LPRECT) override;
	STDMETHOD(UpdateOverlay)(LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDOVERLAYFX) override { return DDERR_NOTAOVERLAYSURFACE; }
	STDMETHOD(UpdateOverlayDisplay)(DWORD) override { return DDERR_NOTAOVERLAYSURFACE; }
	STDMETHOD(UpdateOverlayZOrder)(DWORD, LPDIRECTDRAWSURFACE4) override { return DDERR_NOTAOVERLAYSURFACE; }
	STDMETHOD(GetDDInterface)(LPVOID *) override;
	STDMETHOD(PageLock)(DWORD) override { return DD_OK; }
	STDMETHOD(PageUnlock)(DWORD) override { return DD_OK; }
	STDMETHOD(SetSurfaceDesc)(LPDDSURFACEDESC2, DWORD) override { NOT_IMPL("Surface::SetSurfaceDesc"); return DDERR_UNSUPPORTED; }
	STDMETHOD(SetPrivateData)(REFGUID, LPVOID, DWORD, DWORD) override { return DD_OK; }
	STDMETHOD(GetPrivateData)(REFGUID, LPVOID, LPDWORD) override { return DDERR_NOTFOUND; }
	STDMETHOD(FreePrivateData)(REFGUID) override { return DD_OK; }
	STDMETHOD(GetUniquenessValue)(LPDWORD v) override { *v = version_; return DD_OK; }
	STDMETHOD(ChangeUniquenessValue)() override { version_++; return DD_OK; }

	// IDirect3DTexture2
	STDMETHOD(GetHandle)(LPDIRECT3DDEVICE2, LPD3DTEXTUREHANDLE) override;
	STDMETHOD(PaletteChanged)(DWORD, DWORD) override { dirty_ = true; return D3D_OK; }
	STDMETHOD(Load)(LPDIRECT3DTEXTURE2) override;

	// Engine side
	SurfKind kind;
	int width, height;
	PixFmt fmt;
	ID3D11ShaderResourceView *Srv(); // uploads to the GPU when it changed
	bool HasColorKey() const { return hasKey_; }
	bool HasAlpha() const { return fmt == PF_1555 || fmt == PF_4444 || fmt == PF_8888 || hasKey_; }
	DWORD Caps() const { return desc_.ddsCaps.dwCaps; }
	Surface *zbuffer = nullptr;
	Surface *backbuffer = nullptr;
	Surface *nextMip = nullptr;

private:
	bool IsScreen() const;
	void ScreenToMemory();
	void MemoryToScreen();
	void ToBgra(std::vector<uint32_t> &out) const;
	void FromBgra(const uint32_t *in, int pitch);
	void CopyRect(Surface *src, const RECT &sr, int dx, int dy, int dw, int dh, bool key);
	void FillRect(const RECT &r, DWORD color);

	DDraw *dd_;
	DDSURFACEDESC2 desc_;
	std::vector<uint8_t> mem_;
	int pitch_ = 0;
	DDCOLORKEY key_ = {};
	bool hasKey_ = false;
	Palette *pal_ = nullptr;
	Clipper *clip_ = nullptr;
	GpuTexture gpu_;
	bool dirty_ = true;
	bool lockedScreen_ = false;
	int palVersion_ = -1;
	DWORD version_ = 1;
	DWORD handle_ = 0;
	HDC dc_ = nullptr;
	HBITMAP dib_ = nullptr;
	HGDIOBJ dibOld_ = nullptr;
	uint32_t *dibBits_ = nullptr;
};

// ---------------------------------------------------------------- Direct3D
class Material : public IDirect3DMaterial3 {
public:
	Material() { handle = HandleNew(this); }
	~Material() { HandleFree(handle); }
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(SetMaterial)(LPD3DMATERIAL m) override;
	STDMETHOD(GetMaterial)(LPD3DMATERIAL m) override;
	STDMETHOD(GetHandle)(LPDIRECT3DDEVICE3, LPD3DMATERIALHANDLE h) override { *h = handle; return D3D_OK; }
	D3DMATERIAL mat = {};
	DWORD handle;
};

class LightObj : public IDirect3DLight {
public:
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(Initialize)(LPDIRECT3D) override { return D3D_OK; }
	STDMETHOD(SetLight)(LPD3DLIGHT l) override;
	STDMETHOD(GetLight)(LPD3DLIGHT l) override;
	LightState state;
};

class Viewport : public IDirect3DViewport3 {
public:
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(Initialize)(LPDIRECT3D) override { return D3D_OK; }
	STDMETHOD(GetViewport)(LPD3DVIEWPORT) override;
	STDMETHOD(SetViewport)(LPD3DVIEWPORT) override;
	STDMETHOD(TransformVertices)(DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD) override { NOT_IMPL("Viewport::TransformVertices"); return DDERR_UNSUPPORTED; }
	STDMETHOD(LightElements)(DWORD, LPD3DLIGHTDATA) override { NOT_IMPL("Viewport::LightElements"); return DDERR_UNSUPPORTED; }
	STDMETHOD(SetBackground)(D3DMATERIALHANDLE h) override { background = h; return D3D_OK; }
	STDMETHOD(GetBackground)(LPD3DMATERIALHANDLE h, LPBOOL b) override { *h = background; *b = background != 0; return D3D_OK; }
	STDMETHOD(SetBackgroundDepth)(LPDIRECTDRAWSURFACE) override { NOT_IMPL("Viewport::SetBackgroundDepth"); return D3D_OK; }
	STDMETHOD(GetBackgroundDepth)(LPDIRECTDRAWSURFACE *, LPBOOL) override { NOT_IMPL("Viewport::GetBackgroundDepth"); return DDERR_UNSUPPORTED; }
	STDMETHOD(Clear)(DWORD, LPD3DRECT, DWORD) override;
	STDMETHOD(AddLight)(LPDIRECT3DLIGHT) override;
	STDMETHOD(DeleteLight)(LPDIRECT3DLIGHT) override;
	STDMETHOD(NextLight)(LPDIRECT3DLIGHT, LPDIRECT3DLIGHT *, DWORD) override { NOT_IMPL("Viewport::NextLight"); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetViewport2)(LPD3DVIEWPORT2) override;
	STDMETHOD(SetViewport2)(LPD3DVIEWPORT2) override;
	STDMETHOD(SetBackgroundDepth2)(LPDIRECTDRAWSURFACE4) override { NOT_IMPL("Viewport::SetBackgroundDepth2"); return D3D_OK; }
	STDMETHOD(GetBackgroundDepth2)(LPDIRECTDRAWSURFACE4 *, LPBOOL) override { NOT_IMPL("Viewport::GetBackgroundDepth2"); return DDERR_UNSUPPORTED; }
	STDMETHOD(Clear2)(DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD) override;
	D3DVIEWPORT2 vp = {};
	std::vector<LightObj *> lights;
	D3DMATERIALHANDLE background = 0;
};

class VertexBuffer : public IDirect3DVertexBuffer {
public:
	VertexBuffer(const D3DVERTEXBUFFERDESC &d);
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(Lock)(DWORD, LPVOID *, LPDWORD) override;
	STDMETHOD(Unlock)() override { return D3D_OK; }
	STDMETHOD(ProcessVertices)(DWORD, DWORD, DWORD, LPDIRECT3DVERTEXBUFFER, DWORD, LPDIRECT3DDEVICE3, DWORD) override;
	STDMETHOD(GetVertexBufferDesc)(LPD3DVERTEXBUFFERDESC) override;
	STDMETHOD(Optimize)(LPDIRECT3DDEVICE3, DWORD) override { return D3D_OK; }
	D3DVERTEXBUFFERDESC desc;
	FvfLayout layout;
	std::vector<uint8_t> data;
};

class Device : public IDirect3DDevice3 {
public:
	Device(DDraw *dd, Surface *target);
	STDMETHOD(QueryInterface)(REFIID, void **) override;
	COM_REFCOUNT
	STDMETHOD(GetCaps)(LPD3DDEVICEDESC, LPD3DDEVICEDESC) override;
	STDMETHOD(GetStats)(LPD3DSTATS) override { NOT_IMPL("Device::GetStats"); return DDERR_UNSUPPORTED; }
	STDMETHOD(AddViewport)(LPDIRECT3DVIEWPORT3) override;
	STDMETHOD(DeleteViewport)(LPDIRECT3DVIEWPORT3) override;
	STDMETHOD(NextViewport)(LPDIRECT3DVIEWPORT3, LPDIRECT3DVIEWPORT3 *, DWORD) override { NOT_IMPL("Device::NextViewport"); return DDERR_UNSUPPORTED; }
	STDMETHOD(EnumTextureFormats)(LPD3DENUMPIXELFORMATSCALLBACK, LPVOID) override;
	STDMETHOD(BeginScene)() override { return D3D_OK; }
	STDMETHOD(EndScene)() override { return D3D_OK; }
	STDMETHOD(GetDirect3D)(LPDIRECT3D3 *) override;
	STDMETHOD(SetCurrentViewport)(LPDIRECT3DVIEWPORT3) override;
	STDMETHOD(GetCurrentViewport)(LPDIRECT3DVIEWPORT3 *) override;
	STDMETHOD(SetRenderTarget)(LPDIRECTDRAWSURFACE4, DWORD) override { NOT_IMPL("Device::SetRenderTarget"); return D3D_OK; }
	STDMETHOD(GetRenderTarget)(LPDIRECTDRAWSURFACE4 *) override;
	STDMETHOD(Begin)(D3DPRIMITIVETYPE, DWORD, DWORD) override { NOT_IMPL("Device::Begin"); return DDERR_UNSUPPORTED; }
	STDMETHOD(BeginIndexed)(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD) override { NOT_IMPL("Device::BeginIndexed"); return DDERR_UNSUPPORTED; }
	STDMETHOD(Vertex)(LPVOID) override { return DDERR_UNSUPPORTED; }
	STDMETHOD(Index)(WORD) override { return DDERR_UNSUPPORTED; }
	STDMETHOD(End)(DWORD) override { return DDERR_UNSUPPORTED; }
	STDMETHOD(GetRenderState)(D3DRENDERSTATETYPE, LPDWORD) override;
	STDMETHOD(SetRenderState)(D3DRENDERSTATETYPE, DWORD) override;
	STDMETHOD(GetLightState)(D3DLIGHTSTATETYPE, LPDWORD) override;
	STDMETHOD(SetLightState)(D3DLIGHTSTATETYPE, DWORD) override;
	STDMETHOD(SetTransform)(D3DTRANSFORMSTATETYPE, LPD3DMATRIX) override;
	STDMETHOD(GetTransform)(D3DTRANSFORMSTATETYPE, LPD3DMATRIX) override;
	STDMETHOD(MultiplyTransform)(D3DTRANSFORMSTATETYPE, LPD3DMATRIX) override;
	STDMETHOD(DrawPrimitive)(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD) override;
	STDMETHOD(DrawIndexedPrimitive)(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD) override;
	STDMETHOD(SetClipStatus)(LPD3DCLIPSTATUS) override { return D3D_OK; }
	STDMETHOD(GetClipStatus)(LPD3DCLIPSTATUS) override { NOT_IMPL("Device::GetClipStatus"); return DDERR_UNSUPPORTED; }
	STDMETHOD(DrawPrimitiveStrided)(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD) override { NOT_IMPL("Device::DrawPrimitiveStrided"); return DDERR_UNSUPPORTED; }
	STDMETHOD(DrawIndexedPrimitiveStrided)(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD) override { NOT_IMPL("Device::DrawIndexedPrimitiveStrided"); return DDERR_UNSUPPORTED; }
	STDMETHOD(DrawPrimitiveVB)(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, DWORD, DWORD, DWORD) override;
	STDMETHOD(DrawIndexedPrimitiveVB)(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, LPWORD, DWORD, DWORD) override;
	STDMETHOD(ComputeSphereVisibility)(LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD) override { NOT_IMPL("Device::ComputeSphereVisibility"); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetTexture)(DWORD, LPDIRECT3DTEXTURE2 *) override;
	STDMETHOD(SetTexture)(DWORD, LPDIRECT3DTEXTURE2) override;
	STDMETHOD(GetTextureStageState)(DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD) override;
	STDMETHOD(SetTextureStageState)(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) override;
	STDMETHOD(ValidateDevice)(LPDWORD p) override { *p = 1; return D3D_OK; }

	TnlState Tnl(); // state for tnl.cpp
	void Draw(D3DPRIMITIVETYPE type, const FvfLayout &fl, const BYTE *verts, int nverts, const WORD *idx, int nidx, DWORD flags);

private:
	GpuState BuildState(bool pretransformed, Surface *&tex0);
	DDraw *dd_;
	Surface *target_;
	Viewport *viewport_ = nullptr;
	std::vector<Viewport *> viewports_;
	DWORD rs_[256] = {};
	DWORD ls_[16] = {};
	DWORD tss_[2][32] = {};
	Surface *tex_[2] = {};
	bool legacyBlend_ = false;
	Mat4 world_, view_, proj_;
	std::vector<GpuVertex> gv_;
	std::vector<uint16_t> gi_;
};
