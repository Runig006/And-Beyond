// DirectDraw 4 + Direct3D 3 (object creation and enumerations), palettes, clippers and surfaces.
// Surfaces: the screen ones (primary, back buffer) are the GPU back buffer of render.cpp; textures and 2D surfaces
// are copies in memory in the format the game asked for, converted to BGRA8 and uploaded when they change.
#include "objects.h"
#include <unordered_set>

// ------------------------------------------------------------------ handles
static std::vector<void *> g_handles(1, nullptr);
DWORD HandleNew(void *obj)
{
	for (size_t i = 1; i < g_handles.size(); i++) {
		if (!g_handles[i]) {
			g_handles[i] = obj;
			return (DWORD)i;
		}
	}
	g_handles.push_back(obj);
	return (DWORD)(g_handles.size() - 1);
}
void *HandleGet(DWORD h) { return h < g_handles.size() ? g_handles[h] : nullptr; }
void HandleFree(DWORD h)
{
	if (h && h < g_handles.size()) {
		g_handles[h] = nullptr;
	}
}

// ------------------------------------------------------------------ pixel formats
void MakePixelFormat(PixFmt f, DDPIXELFORMAT *pf)
{
	memset(pf, 0, sizeof *pf);
	pf->dwSize = sizeof *pf;
	switch (f) {
	case PF_565: pf->dwFlags = DDPF_RGB; pf->dwRGBBitCount = 16; pf->dwRBitMask = 0xf800; pf->dwGBitMask = 0x07e0; pf->dwBBitMask = 0x001f; break;
	case PF_1555: pf->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS; pf->dwRGBBitCount = 16; pf->dwRBitMask = 0x7c00; pf->dwGBitMask = 0x03e0; pf->dwBBitMask = 0x001f; pf->dwRGBAlphaBitMask = 0x8000; break;
	case PF_4444: pf->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS; pf->dwRGBBitCount = 16; pf->dwRBitMask = 0x0f00; pf->dwGBitMask = 0x00f0; pf->dwBBitMask = 0x000f; pf->dwRGBAlphaBitMask = 0xf000; break;
	case PF_8888: pf->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS; pf->dwRGBBitCount = 32; pf->dwRBitMask = 0xff0000; pf->dwGBitMask = 0xff00; pf->dwBBitMask = 0xff; pf->dwRGBAlphaBitMask = 0xff000000; break;
	case PF_X888: pf->dwFlags = DDPF_RGB; pf->dwRGBBitCount = 32; pf->dwRBitMask = 0xff0000; pf->dwGBitMask = 0xff00; pf->dwBBitMask = 0xff; break;
	case PF_P8: pf->dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8; pf->dwRGBBitCount = 8; break;
	case PF_Z16: pf->dwFlags = DDPF_ZBUFFER; pf->dwZBufferBitDepth = 16; pf->dwZBitMask = 0xffff; break;
	case PF_Z24S8: pf->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER; pf->dwZBufferBitDepth = 32; pf->dwStencilBitDepth = 8; pf->dwZBitMask = 0xffffff00; pf->dwStencilBitMask = 0xff; break;
	}
}

PixFmt PixFmtFrom(const DDPIXELFORMAT &pf)
{
	if (pf.dwFlags & DDPF_ZBUFFER) {
		return pf.dwZBufferBitDepth == 16 ? PF_Z16 : PF_Z24S8;
	}
	if (pf.dwFlags & DDPF_PALETTEINDEXED8) {
		return PF_P8;
	}
	if (pf.dwRGBBitCount == 32) {
		return (pf.dwFlags & DDPF_ALPHAPIXELS) ? PF_8888 : PF_X888;
	}
	if (pf.dwRGBBitCount == 16) {
		if (pf.dwRGBAlphaBitMask == 0xf000) {
			return PF_4444;
		}
		if (pf.dwRGBAlphaBitMask == 0x8000 || pf.dwRBitMask == 0x7c00) {
			return PF_1555;
		}
		return PF_565;
	}
	LogLine("unknown pixel format (%lu bits, flags %lx): using 565", pf.dwRGBBitCount, pf.dwFlags);
	return PF_565;
}

int PixFmtBytes(PixFmt f)
{
	switch (f) {
	case PF_8888: case PF_X888: case PF_Z24S8: return 4;
	case PF_P8: return 1;
	default: return 2;
	}
}

static uint32_t Expand(uint32_t v, int bits) { return bits == 5 ? (v << 3) | (v >> 2) : bits == 6 ? (v << 2) | (v >> 4) : (v << 4) | v; }

static uint32_t ToArgb(PixFmt f, uint32_t p, const PALETTEENTRY *pal)
{
	switch (f) {
	case PF_565: return 0xff000000 | Expand(p >> 11, 5) << 16 | Expand((p >> 5) & 63, 6) << 8 | Expand(p & 31, 5);
	case PF_1555: return ((p & 0x8000) ? 0xff000000 : 0) | Expand((p >> 10) & 31, 5) << 16 | Expand((p >> 5) & 31, 5) << 8 | Expand(p & 31, 5);
	case PF_4444: return Expand(p >> 12, 4) << 24 | Expand((p >> 8) & 15, 4) << 16 | Expand((p >> 4) & 15, 4) << 8 | Expand(p & 15, 4);
	case PF_8888: return p;
	case PF_X888: return p | 0xff000000;
	case PF_P8: return pal ? 0xff000000 | pal[p & 255].peRed << 16 | pal[p & 255].peGreen << 8 | pal[p & 255].peBlue : 0xff000000;
	default: return 0xff000000;
	}
}

static uint32_t FromArgb(PixFmt f, uint32_t c)
{
	uint32_t a = c >> 24, r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
	switch (f) {
	case PF_565: return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
	case PF_1555: return (a >= 128 ? 0x8000 : 0) | (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3);
	case PF_4444: return (a >> 4) << 12 | (r >> 4) << 8 | (g >> 4) << 4 | (b >> 4);
	case PF_8888: case PF_X888: return c;
	default: return 0;
	}
}

// ------------------------------------------------------------------ DirectDraw
DDraw::DDraw() {}

HRESULT DDraw::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_IDirectDraw4) {
		*out = static_cast<IDirectDraw4 *>(this);
	} else if (riid == IID_IDirect3D3) {
		*out = static_cast<IDirect3D3 *>(this);
	} else if (riid == IID_IDirectDraw) {
		*out = new DDraw1(this);
		return S_OK;
	} else if (riid == IID_IDirectDraw2) {
		*out = NewDDraw2(this);
		return S_OK;
	} else if (riid == IID_IDirect3D2) {
		*out = NewD3D2(this);
		return S_OK;
	} else {
		LogLine("DDraw::QueryInterface: unsupported interface %08lx-...", riid.Data1);
		*out = nullptr;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT DDraw::Compact() { return DD_OK; }

HRESULT DDraw::CreateClipper(DWORD, LPDIRECTDRAWCLIPPER *c, IUnknown *)
{
	*c = new Clipper();
	return DD_OK;
}

HRESULT DDraw::CreatePalette(DWORD flags, LPPALETTEENTRY e, LPDIRECTDRAWPALETTE *p, IUnknown *)
{
	Palette *pal = new Palette();
	if (e) {
		memcpy(pal->entries, e, sizeof pal->entries);
	}
	*p = pal;
	return DD_OK;
}

HRESULT DDraw::CreateSurface(LPDDSURFACEDESC2 d, LPDIRECTDRAWSURFACE4 *out, IUnknown *)
{
	DDSURFACEDESC2 desc = *d;
	desc.dwSize = sizeof desc;
	DWORD caps = desc.ddsCaps.dwCaps;
	Surface *s;
	if (caps & DDSCAPS_PRIMARYSURFACE) {
		desc.dwWidth = modeW ? modeW : g_render.Width();
		desc.dwHeight = modeH ? modeH : g_render.Height();
		s = new Surface(this, SK_PRIMARY, desc);
		primary = s;
		if ((desc.dwFlags & DDSD_BACKBUFFERCOUNT) && desc.dwBackBufferCount) {
			DDSURFACEDESC2 bd = desc;
			bd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
			s->backbuffer = new Surface(this, SK_BACK, bd);
		}
	} else if (caps & DDSCAPS_ZBUFFER) {
		s = new Surface(this, SK_ZBUFFER, desc);
	} else if (caps & DDSCAPS_3DDEVICE) {
		// Window back buffer (windowed mode of the SDK framework)
		if (!g_render.Ready() && hwnd) {
			g_render.Init(hwnd);
		}
		if ((int)desc.dwWidth != g_render.Width() || (int)desc.dwHeight != g_render.Height()) {
			g_render.SetMode(desc.dwWidth, desc.dwHeight, false);
		}
		s = new Surface(this, SK_BACK, desc);
	} else {
		s = new Surface(this, SK_CPU, desc);
		// Mipmap chain: only level 0 is drawn, but the game may fill the others
		if ((caps & DDSCAPS_MIPMAP) && (desc.dwFlags & DDSD_MIPMAPCOUNT) && desc.dwMipMapCount > 1) {
			Surface *prev = s;
			DDSURFACEDESC2 md = desc;
			for (DWORD i = 1; i < desc.dwMipMapCount; i++) {
				md.dwWidth = md.dwWidth > 1 ? md.dwWidth / 2 : 1;
				md.dwHeight = md.dwHeight > 1 ? md.dwHeight / 2 : 1;
				md.dwMipMapCount = desc.dwMipMapCount - i;
				prev->nextMip = new Surface(this, SK_CPU, md);
				prev = prev->nextMip;
			}
		}
	}
	*out = s;
	return DD_OK;
}

HRESULT DDraw::DuplicateSurface(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 *)
{
	NOT_IMPL("DDraw::DuplicateSurface");
	return DDERR_UNSUPPORTED;
}

// Modes we offer: the desktop one and the usual ones that fit on the monitor (16 and 32-bit)
static void ListModes(std::vector<std::pair<int, int>> &modes)
{
	int dw = GetSystemMetrics(SM_CXSCREEN), dh = GetSystemMetrics(SM_CYSCREEN);
	static const struct {
		int width, height;
	} kUsual[] = {{640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 720}, {1280, 960}, {1280, 1024}, {1366, 768},
	              {1440, 900}, {1600, 900}, {1600, 1200}, {1680, 1050}, {1920, 1080}, {1920, 1200}, {2560, 1080},
	              {2560, 1440}, {3440, 1440}, {3840, 2160}};
	modes.push_back({dw, dh});
	for (const auto &m : kUsual) {
		if (m.width <= dw && m.height <= dh && !(m.width == dw && m.height == dh)) {
			modes.push_back({m.width, m.height});
		}
	}
}

// For PATCH OPTIONS > resolution: the same list, desktop first
int DisplayModes(int (*out)[2], int max)
{
	std::vector<std::pair<int, int>> modes;
	ListModes(modes);
	int n = 0;
	for (auto &m : modes) {
		if (n < max) {
			out[n][0] = m.first;
			out[n][1] = m.second;
			n++;
		}
	}
	return n;
}

HRESULT DDraw::EnumDisplayModes(DWORD, LPDDSURFACEDESC2, LPVOID ctx, LPDDENUMMODESCALLBACK2 cb)
{
	std::vector<std::pair<int, int>> modes;
	ListModes(modes);
	for (int bpp : {16, 32}) {
		for (auto &m : modes) {
			DDSURFACEDESC2 d = {sizeof d};
			d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
			d.dwWidth = m.first;
			d.dwHeight = m.second;
			d.lPitch = m.first * bpp / 8;
			d.dwRefreshRate = 60;
			MakePixelFormat(bpp == 16 ? PF_565 : PF_X888, &d.ddpfPixelFormat);
			if (cb(&d, ctx) == DDENUMRET_CANCEL) {
				return DD_OK;
			}
		}
	}
	return DD_OK;
}

HRESULT DDraw::EnumSurfaces(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK2)
{
	NOT_IMPL("DDraw::EnumSurfaces");
	return DDERR_UNSUPPORTED;
}

HRESULT DDraw::FlipToGDISurface() { return DD_OK; }

static void FillCaps(LPDDCAPS c)
{
	DWORD size = c->dwSize ? c->dwSize : sizeof(DDCAPS);
	memset(c, 0, size);
	c->dwSize = size;
	c->dwCaps = DDCAPS_3D | DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_GDI |
	            DDCAPS_CANBLTSYSMEM | DDCAPS_PALETTE;
	c->dwCaps2 = DDCAPS2_WIDESURFACES | DDCAPS2_NO2DDURING3DSCENE;
	c->dwCKeyCaps = DDCKEYCAPS_SRCBLT;
	c->dwPalCaps = DDPCAPS_8BIT;
	c->dwVidMemTotal = c->dwVidMemFree = 256 << 20;
	c->ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER | DDSCAPS_FLIP | DDSCAPS_BACKBUFFER |
	                    DDSCAPS_FRONTBUFFER | DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY |
	                    DDSCAPS_SYSTEMMEMORY | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
	c->dwZBufferBitDepths = DDBD_16 | DDBD_24 | DDBD_32;
}

HRESULT DDraw::GetCaps(LPDDCAPS hal, LPDDCAPS hel)
{
	if (hal) {
		FillCaps(hal);
	}
	if (hel) {
		FillCaps(hel);
	}
	return DD_OK;
}

HRESULT DDraw::GetDisplayMode(LPDDSURFACEDESC2 d)
{
	DWORD size = d->dwSize;
	memset(d, 0, sizeof *d);
	d->dwSize = size;
	d->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
	d->dwWidth = modeW ? modeW : GetSystemMetrics(SM_CXSCREEN);
	d->dwHeight = modeH ? modeH : GetSystemMetrics(SM_CYSCREEN);
	d->dwRefreshRate = 60;
	int bpp = modeW ? modeBpp : 32;
	d->lPitch = d->dwWidth * bpp / 8;
	MakePixelFormat(bpp == 16 ? PF_565 : PF_X888, &d->ddpfPixelFormat);
	return DD_OK;
}

HRESULT DDraw::GetFourCCCodes(LPDWORD n, LPDWORD) { *n = 0; return DD_OK; }
HRESULT DDraw::GetGDISurface(LPDIRECTDRAWSURFACE4 *s)
{
	*s = primary;
	if (primary) {
		primary->AddRef();
	}
	return primary ? DD_OK : DDERR_NOTFOUND;
}
HRESULT DDraw::GetMonitorFrequency(LPDWORD f) { *f = 60; return DD_OK; }
HRESULT DDraw::GetScanLine(LPDWORD l) { *l = 0; return DD_OK; }
HRESULT DDraw::GetVerticalBlankStatus(LPBOOL b) { *b = TRUE; return DD_OK; }
HRESULT DDraw::Initialize(GUID *) { return DD_OK; }
HRESULT DDraw::RestoreDisplayMode() { return DD_OK; }

HRESULT DDraw::SetCooperativeLevel(HWND h, DWORD flags)
{
	LogLine("SetCooperativeLevel window %p flags %08lx (called from %p)", h, flags, _ReturnAddress());
	if (h) {
		hwnd = h;
		g_render.SetWindow(h);
	}
	return DD_OK;
}

HRESULT DDraw::SetDisplayMode(DWORD w, DWORD h, DWORD bpp, DWORD, DWORD)
{
	LogLine("SetDisplayMode %lux%lu %lu-bit (called from %p)", w, h, bpp, _ReturnAddress());
	modeW = w;
	modeH = h;
	modeBpp = bpp;
	if (hwnd && !g_render.Ready()) {
		g_render.Init(hwnd);
	}
	g_render.SetMode(w, h, true);
	return DD_OK;
}

HRESULT DDraw::WaitForVerticalBlank(DWORD, HANDLE) { return DD_OK; }
HRESULT DDraw::GetAvailableVidMem(LPDDSCAPS2, LPDWORD total, LPDWORD freeMem)
{
	if (total) {
		*total = 256 << 20;
	}
	if (freeMem) {
		*freeMem = 256 << 20;
	}
	return DD_OK;
}
HRESULT DDraw::GetSurfaceFromDC(HDC, LPDIRECTDRAWSURFACE4 *) { NOT_IMPL("DDraw::GetSurfaceFromDC"); return DDERR_NOTFOUND; }
HRESULT DDraw::RestoreAllSurfaces() { return DD_OK; }
HRESULT DDraw::TestCooperativeLevel() { return DD_OK; }
HRESULT DDraw::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER id, DWORD)
{
	memset(id, 0, sizeof *id);
	strcpy(id->szDriver, "buzzpatch");
	strcpy(id->szDescription, "Buzz Lightyear - Direct3D 11 engine");
	return DD_OK;
}

// ---- Direct3D 3
static void FillDeviceDesc(LPD3DDEVICEDESC d)
{
	DWORD size = d->dwSize ? d->dwSize : sizeof(D3DDEVICEDESC);
	memset(d, 0, size);
	d->dwSize = size;
	d->dwFlags = D3DDD_COLORMODEL | D3DDD_DEVCAPS | D3DDD_TRANSFORMCAPS | D3DDD_LIGHTINGCAPS | D3DDD_BCLIPPING |
	             D3DDD_LINECAPS | D3DDD_TRICAPS | D3DDD_DEVICERENDERBITDEPTH | D3DDD_DEVICEZBUFFERBITDEPTH |
	             D3DDD_MAXBUFFERSIZE | D3DDD_MAXVERTEXCOUNT;
	d->dcmColorModel = D3DCOLOR_RGB;
	d->dwDevCaps = D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_DRAWPRIMTLVERTEX |
	               D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TEXTURESYSTEMMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
	               D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMITIVES2;
	d->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
	d->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
	d->bClipping = TRUE;
	d->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
	d->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT | D3DLIGHTCAPS_DIRECTIONAL;
	d->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
	d->dlcLightingCaps.dwNumLights = 32;
	D3DPRIMCAPS p = {sizeof p};
	p.dwMiscCaps = D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_MASKZ;
	p.dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGTABLE |
	                 D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_WFOG;
	p.dwZCmpCaps = p.dwAlphaCmpCaps = 0xff; // every comparison
	p.dwSrcBlendCaps = p.dwDestBlendCaps = 0x1fff;
	p.dwShadeCaps = D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_SPECULARGOURAUDRGB | D3DPSHADECAPS_ALPHAGOURAUDBLEND |
	                D3DPSHADECAPS_FOGGOURAUD | D3DPSHADECAPS_COLORFLATRGB;
	p.dwTextureCaps = D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_TRANSPARENCY |
	                  D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_BORDER;
	p.dwTextureFilterCaps = D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
	                        D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
	                        D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MAGFPOINT |
	                        D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR;
	p.dwTextureBlendCaps = D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_DECALALPHA |
	                       D3DPTBLENDCAPS_MODULATEALPHA | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_ADD;
	p.dwTextureAddressCaps = D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_MIRROR | D3DPTADDRESSCAPS_CLAMP |
	                         D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_INDEPENDENTUV;
	d->dpcTriCaps = p;
	d->dpcLineCaps = p;
	d->dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
	d->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
	d->dwMaxBufferSize = 0;
	d->dwMaxVertexCount = 65535;
	if (size >= sizeof(D3DDEVICEDESC)) {
		d->dwMinTextureWidth = d->dwMinTextureHeight = 1;
		d->dwMaxTextureWidth = d->dwMaxTextureHeight = 4096;
		d->dwMinStippleWidth = d->dwMinStippleHeight = 1;
		d->dwMaxStippleWidth = d->dwMaxStippleHeight = 32;
		d->dwMaxTextureRepeat = 8192;
		d->dwMaxTextureAspectRatio = 4096;
		d->dwMaxAnisotropy = 1;
		d->dvGuardBandLeft = d->dvGuardBandTop = -4096;
		d->dvGuardBandRight = d->dvGuardBandBottom = 4096;
		d->dvExtentsAdjust = 0;
		d->dwStencilCaps = 0;
		d->dwFVFCaps = 2;
		d->dwTextureOpCaps = 0x00ffffff;
		d->wMaxTextureBlendStages = 2;
		d->wMaxSimultaneousTextures = 2;
	}
}

HRESULT DDraw::EnumDevices(LPD3DENUMDEVICESCALLBACK cb, LPVOID ctx)
{
	D3DDEVICEDESC hal = {sizeof hal}, hel = {sizeof hel};
	FillDeviceDesc(&hal);
	FillDeviceDesc(&hel);
	GUID g = IID_IDirect3DHALDevice;
	cb(&g, (LPSTR) "Direct3D 11 (buzzpatch)", (LPSTR) "Direct3D HAL", &hal, &hel, ctx);
	return D3D_OK;
}

HRESULT DDraw::CreateLight(LPDIRECT3DLIGHT *l, LPUNKNOWN)
{
	*l = new LightObj();
	return D3D_OK;
}
HRESULT DDraw::CreateMaterial(LPDIRECT3DMATERIAL3 *m, LPUNKNOWN)
{
	*m = new Material();
	return D3D_OK;
}
HRESULT DDraw::CreateViewport(LPDIRECT3DVIEWPORT3 *v, LPUNKNOWN)
{
	*v = new Viewport();
	return D3D_OK;
}

HRESULT DDraw::FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT r)
{
	r->guid = IID_IDirect3DHALDevice;
	r->ddHwDesc.dwSize = r->ddSwDesc.dwSize = sizeof(D3DDEVICEDESC);
	FillDeviceDesc(&r->ddHwDesc);
	FillDeviceDesc(&r->ddSwDesc);
	return D3D_OK;
}

HRESULT DDraw::CreateDevice(REFCLSID, LPDIRECTDRAWSURFACE4 target, LPDIRECT3DDEVICE3 *out, LPUNKNOWN)
{
	*out = new Device(this, static_cast<Surface *>(target));
	return D3D_OK;
}

HRESULT DDraw::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC d, LPDIRECT3DVERTEXBUFFER *out, DWORD, LPUNKNOWN)
{
	*out = new VertexBuffer(*d);
	return D3D_OK;
}

HRESULT DDraw::EnumZBufferFormats(REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK cb, LPVOID ctx)
{
	DDPIXELFORMAT pf;
	MakePixelFormat(PF_Z16, &pf);
	if (cb(&pf, ctx) == D3DENUMRET_CANCEL) {
		return D3D_OK;
	}
	MakePixelFormat(PF_Z24S8, &pf);
	cb(&pf, ctx);
	return D3D_OK;
}

HRESULT DDraw::EvictManagedTextures() { return D3D_OK; }

// ---- DirectDraw version 1
HRESULT DDraw1::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_IDirectDraw) {
		*out = this;
		AddRef();
		return S_OK;
	}
	return dd_->QueryInterface(riid, out);
}

HRESULT DDraw1::CreateSurface(LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE *out, IUnknown *u)
{
	DDSURFACEDESC2 d2 = {sizeof d2};
	memcpy(&d2, d, sizeof *d);
	d2.dwSize = sizeof d2;
	LPDIRECTDRAWSURFACE4 s;
	HRESULT hr = dd_->CreateSurface(&d2, &s, u);
	*out = (LPDIRECTDRAWSURFACE)s; // same vtable layout for the shared methods
	NOT_IMPL("DD1::CreateSurface (version 1 surface)");
	return hr;
}

HRESULT DDraw1::EnumDisplayModes(DWORD f, LPDDSURFACEDESC, LPVOID ctx, LPDDENUMMODESCALLBACK cb)
{
	std::vector<std::pair<int, int>> modes;
	ListModes(modes);
	for (int bpp : {16, 32}) {
		for (auto &m : modes) {
			DDSURFACEDESC d = {sizeof d};
			d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
			d.dwWidth = m.first;
			d.dwHeight = m.second;
			d.lPitch = m.first * bpp / 8;
			d.dwRefreshRate = 60;
			MakePixelFormat(bpp == 16 ? PF_565 : PF_X888, &d.ddpfPixelFormat);
			if (cb(&d, ctx) == DDENUMRET_CANCEL) {
				return DD_OK;
			}
		}
	}
	return DD_OK;
}

HRESULT DDraw1::GetDisplayMode(LPDDSURFACEDESC d)
{
	DDSURFACEDESC2 d2 = {sizeof d2};
	dd_->GetDisplayMode(&d2);
	memcpy(d, &d2, sizeof *d);
	d->dwSize = sizeof *d;
	return DD_OK;
}

// ------------------------------------------------------------------ palette and clipper
HRESULT Palette::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_IDirectDrawPalette) {
		*out = this;
		AddRef();
		return S_OK;
	}
	*out = nullptr;
	return E_NOINTERFACE;
}
HRESULT Palette::GetEntries(DWORD, DWORD base, DWORD n, LPPALETTEENTRY e)
{
	memcpy(e, entries + base, n * sizeof *e);
	return DD_OK;
}
HRESULT Palette::SetEntries(DWORD, DWORD base, DWORD n, LPPALETTEENTRY e)
{
	memcpy(entries + base, e, n * sizeof *e);
	version++;
	return DD_OK;
}
HRESULT Clipper::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_IDirectDrawClipper) {
		*out = this;
		AddRef();
		return S_OK;
	}
	*out = nullptr;
	return E_NOINTERFACE;
}

// ------------------------------------------------------------------ surfaces
static std::unordered_set<Surface *> g_surfaces; // every live surface, to recognise texture pointers

Surface *SurfaceFromTexture(void *p, const char *who)
{
	if (!p) {
		return nullptr;
	}
	Surface *s = static_cast<Surface *>((IDirect3DTexture2 *)p);
	if (g_surfaces.count(s)) {
		return s;
	}
	s = static_cast<Surface *>((IDirectDrawSurface4 *)p);
	if (g_surfaces.count(s)) {
		NOT_IMPL("texture given as IDirectDrawSurface4 instead of IDirect3DTexture2");
		return s;
	}
	static int bad;
	if (bad++ < 8) {
		LogLine("%s: unknown texture pointer %p", who, p);
	}
	return nullptr;
}

Surface::Surface(DDraw *dd, SurfKind k, const DDSURFACEDESC2 &d) : kind(k), dd_(dd), desc_(d)
{
	desc_.dwSize = sizeof desc_;
	width = d.dwWidth ? d.dwWidth : g_render.Width();
	height = d.dwHeight ? d.dwHeight : g_render.Height();
	desc_.dwWidth = width;
	desc_.dwHeight = height;
	desc_.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_CAPS;
	if (k == SK_ZBUFFER) {
		fmt = (d.dwFlags & DDSD_PIXELFORMAT) ? PixFmtFrom(d.ddpfPixelFormat) : PF_Z16;
	} else if (d.dwFlags & DDSD_PIXELFORMAT) {
		fmt = PixFmtFrom(d.ddpfPixelFormat);
	} else {
		fmt = dd->modeBpp == 32 ? PF_X888 : PF_565;
	}
	MakePixelFormat(fmt, &desc_.ddpfPixelFormat);
	pitch_ = (width * PixFmtBytes(fmt) + 3) & ~3;
	desc_.lPitch = pitch_;
	if (k == SK_CPU) {
		mem_.assign((size_t)pitch_ * height, 0);
	}
	if (k == SK_ZBUFFER) {
		desc_.ddsCaps.dwCaps |= DDSCAPS_ZBUFFER;
	}
	g_surfaces.insert(this);
}

Surface::~Surface()
{
	g_surfaces.erase(this);
	g_render.Free(gpu_);
	if (handle_) {
		HandleFree(handle_);
	}
	if (pal_) {
		pal_->Release();
	}
	if (clip_) {
		clip_->Release();
	}
	if (zbuffer) {
		zbuffer->Release();
	}
	if (backbuffer) {
		backbuffer->Release();
	}
	if (nextMip) {
		nextMip->Release();
	}
	if (dd_->primary == this) {
		dd_->primary = nullptr;
	}
}

HRESULT Surface::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_IDirectDrawSurface4) {
		*out = static_cast<IDirectDrawSurface4 *>(this);
	} else if (riid == IID_IDirect3DTexture2) {
		*out = static_cast<IDirect3DTexture2 *>(this);
	} else {
		LogLine("Surface::QueryInterface: unsupported interface %08lx", riid.Data1);
		*out = nullptr;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT Surface::AddAttachedSurface(LPDIRECTDRAWSURFACE4 s)
{
	Surface *o = static_cast<Surface *>(s);
	if (o->kind == SK_ZBUFFER) {
		if (zbuffer) {
			zbuffer->Release();
		}
		zbuffer = o;
		o->AddRef();
	}
	return DD_OK;
}

HRESULT Surface::DeleteAttachedSurface(DWORD, LPDIRECTDRAWSURFACE4 s)
{
	if (s && static_cast<Surface *>(s) == zbuffer) {
		zbuffer->Release();
		zbuffer = nullptr;
	}
	return DD_OK;
}

HRESULT Surface::EnumAttachedSurfaces(LPVOID ctx, LPDDENUMSURFACESCALLBACK2 cb)
{
	Surface *list[] = {backbuffer, zbuffer, nextMip};
	for (Surface *s : list) {
		if (s) {
			s->AddRef();
			if (cb(s, &s->desc_, ctx) == DDENUMRET_CANCEL) {
				break;
			}
		}
	}
	return DD_OK;
}

HRESULT Surface::GetAttachedSurface(LPDDSCAPS2 caps, LPDIRECTDRAWSURFACE4 *out)
{
	Surface *s = nullptr;
	if (caps->dwCaps & DDSCAPS_BACKBUFFER) {
		s = backbuffer;
	} else if (caps->dwCaps & DDSCAPS_ZBUFFER) {
		s = zbuffer;
	} else if (caps->dwCaps & DDSCAPS_MIPMAP) {
		s = nextMip;
	}
	if (!s) {
		*out = nullptr;
		return DDERR_NOTFOUND;
	}
	s->AddRef();
	*out = s;
	return DD_OK;
}

HRESULT Surface::GetCaps(LPDDSCAPS2 c)
{
	*c = desc_.ddsCaps;
	return DD_OK;
}

HRESULT Surface::GetClipper(LPDIRECTDRAWCLIPPER *c)
{
	*c = clip_;
	if (clip_) {
		clip_->AddRef();
	}
	return clip_ ? DD_OK : DDERR_NOCLIPPERATTACHED;
}

HRESULT Surface::SetClipper(LPDIRECTDRAWCLIPPER c)
{
	if (clip_) {
		clip_->Release();
	}
	clip_ = static_cast<Clipper *>(c);
	if (clip_) {
		clip_->AddRef();
	}
	return DD_OK;
}

HRESULT Surface::GetColorKey(DWORD, LPDDCOLORKEY k)
{
	if (!hasKey_) {
		return DDERR_NOCOLORKEY;
	}
	*k = key_;
	return DD_OK;
}

HRESULT Surface::SetColorKey(DWORD flags, LPDDCOLORKEY k)
{
	if (!(flags & DDCKEY_SRCBLT)) {
		NOT_IMPL("Surface::SetColorKey (non-source key)");
		return DD_OK;
	}
	hasKey_ = k != nullptr;
	if (k) {
		key_ = *k;
	}
	dirty_ = true;
	return DD_OK;
}

HRESULT Surface::GetPalette(LPDIRECTDRAWPALETTE *p)
{
	*p = pal_;
	if (pal_) {
		pal_->AddRef();
	}
	return pal_ ? DD_OK : DDERR_NOPALETTEATTACHED;
}

HRESULT Surface::SetPalette(LPDIRECTDRAWPALETTE p)
{
	if (pal_) {
		pal_->Release();
	}
	pal_ = static_cast<Palette *>(p);
	if (pal_) {
		pal_->AddRef();
	}
	dirty_ = true;
	return DD_OK;
}

HRESULT Surface::GetPixelFormat(LPDDPIXELFORMAT pf)
{
	*pf = desc_.ddpfPixelFormat;
	return DD_OK;
}

HRESULT Surface::GetSurfaceDesc(LPDDSURFACEDESC2 d)
{
	DWORD size = d->dwSize;
	memcpy(d, &desc_, size && size < sizeof desc_ ? size : sizeof desc_);
	d->dwSize = size ? size : sizeof desc_;
	return DD_OK;
}

HRESULT Surface::GetDDInterface(LPVOID *out)
{
	*out = static_cast<IDirectDraw4 *>(dd_);
	dd_->AddRef();
	return DD_OK;
}

// Screen surfaces (primary / back buffer) live on the GPU: Lock and GetDC read them into memory and the matching
// Unlock / ReleaseDC write them back. Slow, but the game only does it outside the 3D frame.
bool Surface::IsScreen() const { return kind == SK_BACK || kind == SK_PRIMARY; }

void Surface::ScreenToMemory()
{
	width = g_render.Width();
	height = g_render.Height();
	pitch_ = (width * PixFmtBytes(fmt) + 3) & ~3;
	desc_.dwWidth = width;
	desc_.dwHeight = height;
	desc_.lPitch = pitch_;
	std::vector<uint32_t> px((size_t)width * height);
	g_render.ReadBack(px.data(), width);
	mem_.assign((size_t)pitch_ * height, 0);
	FromBgra(px.data(), width);
}

void Surface::MemoryToScreen()
{
	std::vector<uint32_t> px;
	ToBgra(px);
	RECT r = {0, 0, width, height};
	g_render.WriteBack(px.data(), width, r);
}

HRESULT Surface::Lock(LPRECT r, LPDDSURFACEDESC2 d, DWORD flags, HANDLE)
{
	if (kind == SK_ZBUFFER) {
		NOT_IMPL("Surface::Lock on the Z buffer");
		if (mem_.empty()) {
			mem_.assign((size_t)pitch_ * height, 0);
		}
	} else if (IsScreen()) {
		ScreenToMemory();
		lockedScreen_ = !(flags & DDLOCK_READONLY);
	}
	DWORD size = d->dwSize;
	memcpy(d, &desc_, size && size < sizeof desc_ ? size : sizeof desc_);
	d->dwSize = size ? size : sizeof desc_;
	d->dwFlags |= DDSD_LPSURFACE;
	uint8_t *p = mem_.data();
	if (r) {
		p += r->top * pitch_ + r->left * PixFmtBytes(fmt);
	}
	d->lpSurface = p;
	return DD_OK;
}

HRESULT Surface::Unlock(LPRECT)
{
	if (lockedScreen_) {
		MemoryToScreen();
		lockedScreen_ = false;
	}
	dirty_ = true;
	return DD_OK;
}

// GetDC: GDI draws on a 32-bit DIB holding a copy of the surface; ReleaseDC puts it back into the surface
HRESULT Surface::GetDC(HDC *out)
{
	if (kind == SK_ZBUFFER) {
		return DDERR_UNSUPPORTED;
	}
	if (IsScreen()) {
		ScreenToMemory();
	}
	BITMAPINFO bi = {};
	bi.bmiHeader.biSize = sizeof bi.bmiHeader;
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	dc_ = CreateCompatibleDC(nullptr);
	dib_ = CreateDIBSection(dc_, &bi, DIB_RGB_COLORS, (void **)&dibBits_, nullptr, 0);
	dibOld_ = SelectObject(dc_, dib_);
	std::vector<uint32_t> px;
	ToBgra(px);
	memcpy(dibBits_, px.data(), px.size() * 4);
	*out = dc_;
	return DD_OK;
}

HRESULT Surface::ReleaseDC(HDC)
{
	if (!dc_) {
		return DD_OK;
	}
	GdiFlush();
	FromBgra(dibBits_, width);
	SelectObject(dc_, dibOld_);
	DeleteObject(dib_);
	DeleteDC(dc_);
	dc_ = nullptr;
	dib_ = nullptr;
	dirty_ = true;
	if (IsScreen()) {
		MemoryToScreen();
	}
	return DD_OK;
}

void Surface::ToBgra(std::vector<uint32_t> &out) const
{
	out.resize((size_t)width * height);
	int bpp = PixFmtBytes(fmt);
	const PALETTEENTRY *pal = pal_ ? pal_->entries : nullptr;
	for (int y = 0; y < height; y++) {
		const uint8_t *row = mem_.data() + (size_t)y * pitch_;
		for (int x = 0; x < width; x++) {
			uint32_t p = bpp == 4 ? ((const uint32_t *)row)[x] : bpp == 2 ? ((const uint16_t *)row)[x] : row[x];
			uint32_t c = ToArgb(fmt, p, pal);
			if (hasKey_ && p >= key_.dwColorSpaceLowValue && p <= key_.dwColorSpaceHighValue) {
				c &= 0x00ffffff; // color key -> transparent
			}
			out[(size_t)y * width + x] = c;
		}
	}
}

void Surface::FromBgra(const uint32_t *in, int pitchPx)
{
	int bpp = PixFmtBytes(fmt);
	for (int y = 0; y < height; y++) {
		uint8_t *row = mem_.data() + (size_t)y * pitch_;
		for (int x = 0; x < width; x++) {
			uint32_t c = in[(size_t)y * pitchPx + x] | 0xff000000;
			uint32_t p = FromArgb(fmt, c);
			if (bpp == 4) {
				((uint32_t *)row)[x] = p;
			} else if (bpp == 2) {
				((uint16_t *)row)[x] = (uint16_t)p;
			}
		}
	}
}

ID3D11ShaderResourceView *Surface::Srv()
{
	if (kind != SK_CPU) {
		return nullptr;
	}
	int palVer = pal_ ? pal_->version : -1;
	if (!gpu_.tex || gpu_.w != width || gpu_.h != height) {
		g_render.Free(gpu_);
		gpu_ = g_render.CreateTexture(width, height);
		dirty_ = true;
	}
	if (dirty_ || palVer != palVersion_) {
		std::vector<uint32_t> px;
		ToBgra(px);
		g_render.Upload(gpu_, px.data(), width);
		dirty_ = false;
		palVersion_ = palVer;
	}
	return gpu_.srv;
}

// Software copy between memory surfaces (same format, or through ARGB if they differ)
void Surface::CopyRect(Surface *src, const RECT &sr, int dx, int dy, int dw, int dh, bool key)
{
	int sw = sr.right - sr.left, sh = sr.bottom - sr.top;
	if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) {
		return;
	}
	int sb = PixFmtBytes(src->fmt), db = PixFmtBytes(fmt);
	const PALETTEENTRY *pal = src->pal_ ? src->pal_->entries : nullptr;
	for (int y = 0; y < dh; y++) {
		int ty = dy + y, syy = sr.top + y * sh / dh;
		if (ty < 0 || ty >= height || syy < 0 || syy >= src->height) {
			continue;
		}
		const uint8_t *srow = src->mem_.data() + (size_t)syy * src->pitch_;
		uint8_t *drow = mem_.data() + (size_t)ty * pitch_;
		for (int x = 0; x < dw; x++) {
			int tx = dx + x, sxx = sr.left + x * sw / dw;
			if (tx < 0 || tx >= width || sxx < 0 || sxx >= src->width) {
				continue;
			}
			uint32_t p = sb == 4 ? ((const uint32_t *)srow)[sxx] : sb == 2 ? ((const uint16_t *)srow)[sxx] : srow[sxx];
			if (key && src->hasKey_ && p >= src->key_.dwColorSpaceLowValue && p <= src->key_.dwColorSpaceHighValue) {
				continue;
			}
			uint32_t o = src->fmt == fmt ? p : FromArgb(fmt, ToArgb(src->fmt, p, pal));
			if (db == 4) {
				((uint32_t *)drow)[tx] = o;
			} else if (db == 2) {
				((uint16_t *)drow)[tx] = (uint16_t)o;
			} else {
				drow[tx] = (uint8_t)o;
			}
		}
	}
	dirty_ = true;
}

void Surface::FillRect(const RECT &r, DWORD color)
{
	int db = PixFmtBytes(fmt);
	for (int y = r.top < 0 ? 0 : r.top; y < r.bottom && y < height; y++) {
		uint8_t *row = mem_.data() + (size_t)y * pitch_;
		for (int x = r.left < 0 ? 0 : r.left; x < r.right && x < width; x++) {
			if (db == 4) {
				((uint32_t *)row)[x] = color;
			} else if (db == 2) {
				((uint16_t *)row)[x] = (uint16_t)color;
			} else {
				row[x] = (uint8_t)color;
			}
		}
	}
	dirty_ = true;
}

// Fill color in the surface format -> ARGB to clear the back buffer
static DWORD FillToArgb(PixFmt f, DWORD c) { return ToArgb(f, c, nullptr); }

HRESULT Surface::Blt(LPRECT dr, LPDIRECTDRAWSURFACE4 srcIf, LPRECT sr, DWORD flags, LPDDBLTFX fx)
{
	Surface *src = static_cast<Surface *>(srcIf);
	RECT d = dr ? *dr : RECT{0, 0, width, height};
	if (kind == SK_PRIMARY) {
		// Windowed mode: copying the back buffer to the screen = present
		if (src && src->kind == SK_BACK) {
			g_render.Present();
		} else {
			NOT_IMPL("Surface::Blt to the screen from something that is not the back buffer");
		}
		return DD_OK;
	}
	if (flags & DDBLT_COLORFILL) {
		if (kind == SK_BACK) {
			D3DRECT r = {d.left, d.top, d.right, d.bottom};
			g_render.Clear(&r, 1, D3DCLEAR_TARGET, FillToArgb(fmt, fx->dwFillColor), 1.f);
		} else if (kind == SK_CPU) {
			FillRect(d, fx->dwFillColor);
		}
		return DD_OK;
	}
	if (flags & DDBLT_DEPTHFILL) {
		if (kind == SK_ZBUFFER) {
			g_render.Clear(nullptr, 0, D3DCLEAR_ZBUFFER, 0, 1.f);
		}
		return DD_OK;
	}
	if (!src) {
		return DDERR_INVALIDPARAMS;
	}
	RECT s = sr ? *sr : RECT{0, 0, src->width, src->height};
	bool key = (flags & DDBLT_KEYSRC) != 0;
	if (kind == SK_BACK && src->kind == SK_CPU) {
		g_render.Blit(src->Srv(), src->width, src->height, s, d, key && src->hasKey_);
	} else if (kind == SK_CPU && src->kind == SK_CPU) {
		CopyRect(src, s, d.left, d.top, d.right - d.left, d.bottom - d.top, key);
	} else {
		NOT_IMPL("Surface::Blt (surface combination)");
	}
	return DD_OK;
}

HRESULT Surface::BltFast(DWORD x, DWORD y, LPDIRECTDRAWSURFACE4 srcIf, LPRECT sr, DWORD flags)
{
	Surface *src = static_cast<Surface *>(srcIf);
	if (!src) {
		return DDERR_INVALIDPARAMS;
	}
	RECT s = sr ? *sr : RECT{0, 0, src->width, src->height};
	RECT d = {(LONG)x, (LONG)y, (LONG)x + s.right - s.left, (LONG)y + s.bottom - s.top};
	return Blt(&d, srcIf, &s, (flags & DDBLTFAST_SRCCOLORKEY) ? DDBLT_KEYSRC : 0, nullptr);
}

HRESULT Surface::Flip(LPDIRECTDRAWSURFACE4, DWORD)
{
	g_render.Present();
	return DD_OK;
}

HRESULT Surface::GetHandle(LPDIRECT3DDEVICE2, LPD3DTEXTUREHANDLE h)
{
	if (!handle_) {
		handle_ = HandleNew(this);
	}
	*h = handle_;
	return D3D_OK;
}

// Load: copies one texture into another (the game loads in system memory and moves it to video memory)
HRESULT Surface::Load(LPDIRECT3DTEXTURE2 t)
{
	Surface *src = SurfaceFromTexture(t, "Texture::Load");
	if (!src) {
		return DDERR_INVALIDPARAMS;
	}
	if (src->width == width && src->height == height && src->fmt == fmt) {
		mem_ = src->mem_;
	} else {
		RECT r = {0, 0, src->width, src->height};
		CopyRect(src, r, 0, 0, width, height, false);
	}
	hasKey_ = src->hasKey_;
	key_ = src->key_;
	if (src->pal_) {
		SetPalette(src->pal_);
	}
	dirty_ = true;
	return D3D_OK;
}
