// IDirectDraw2 and IDirect3D2: requested by the old SDK framework enumeration (ExamineDDEnumCallback).
// It only asks questions (caps, modes, devices, memory), answered with the same data as versions 4 and 3.
#include "objects.h"

class DDraw2 : public IDirectDraw2 {
public:
	explicit DDraw2(DDraw *dd) : dd_(dd) { dd_->AddRef(); }
	~DDraw2() { dd_->Release(); }
	STDMETHOD(QueryInterface)(REFIID riid, void **out) override
	{
		if (riid == IID_IUnknown || riid == IID_IDirectDraw2) {
			*out = this;
			AddRef();
			return S_OK;
		}
		return dd_->QueryInterface(riid, out);
	}
	COM_REFCOUNT
	STDMETHOD(Compact)() override { return DD_OK; }
	STDMETHOD(CreateClipper)(DWORD f, LPDIRECTDRAWCLIPPER *c, IUnknown *u) override { return dd_->CreateClipper(f, c, u); }
	STDMETHOD(CreatePalette)(DWORD f, LPPALETTEENTRY e, LPDIRECTDRAWPALETTE *p, IUnknown *u) override { return dd_->CreatePalette(f, e, p, u); }
	STDMETHOD(CreateSurface)(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) override { NOT_IMPL("DD2::CreateSurface"); return DDERR_UNSUPPORTED; }
	STDMETHOD(DuplicateSurface)(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) override { NOT_IMPL("DD2::DuplicateSurface"); return DDERR_UNSUPPORTED; }
	STDMETHOD(EnumDisplayModes)(DWORD f, LPDDSURFACEDESC d, LPVOID ctx, LPDDENUMMODESCALLBACK cb) override
	{
		DDraw1 one(dd_);
		return one.EnumDisplayModes(f, d, ctx, cb);
	}
	STDMETHOD(EnumSurfaces)(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) override { NOT_IMPL("DD2::EnumSurfaces"); return DDERR_UNSUPPORTED; }
	STDMETHOD(FlipToGDISurface)() override { return DD_OK; }
	STDMETHOD(GetCaps)(LPDDCAPS a, LPDDCAPS b) override { return dd_->GetCaps(a, b); }
	STDMETHOD(GetDisplayMode)(LPDDSURFACEDESC d) override
	{
		DDraw1 one(dd_);
		return one.GetDisplayMode(d);
	}
	STDMETHOD(GetFourCCCodes)(LPDWORD n, LPDWORD c) override { return dd_->GetFourCCCodes(n, c); }
	STDMETHOD(GetGDISurface)(LPDIRECTDRAWSURFACE *) override { NOT_IMPL("DD2::GetGDISurface"); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetMonitorFrequency)(LPDWORD f) override { return dd_->GetMonitorFrequency(f); }
	STDMETHOD(GetScanLine)(LPDWORD l) override { return dd_->GetScanLine(l); }
	STDMETHOD(GetVerticalBlankStatus)(LPBOOL b) override { return dd_->GetVerticalBlankStatus(b); }
	STDMETHOD(Initialize)(GUID *) override { return DD_OK; }
	STDMETHOD(RestoreDisplayMode)() override { return dd_->RestoreDisplayMode(); }
	STDMETHOD(SetCooperativeLevel)(HWND h, DWORD f) override { return dd_->SetCooperativeLevel(h, f); }
	STDMETHOD(SetDisplayMode)(DWORD w, DWORD h, DWORD b, DWORD r, DWORD f) override { return dd_->SetDisplayMode(w, h, b, r, f); }
	STDMETHOD(WaitForVerticalBlank)(DWORD, HANDLE) override { return DD_OK; }
	STDMETHOD(GetAvailableVidMem)(LPDDSCAPS, LPDWORD total, LPDWORD freeMem) override { return dd_->GetAvailableVidMem(nullptr, total, freeMem); }

private:
	DDraw *dd_;
};

class D3D2 : public IDirect3D2 {
public:
	explicit D3D2(DDraw *dd) : dd_(dd) { dd_->AddRef(); }
	~D3D2() { dd_->Release(); }
	STDMETHOD(QueryInterface)(REFIID riid, void **out) override
	{
		if (riid == IID_IUnknown || riid == IID_IDirect3D2) {
			*out = this;
			AddRef();
			return S_OK;
		}
		return dd_->QueryInterface(riid, out);
	}
	COM_REFCOUNT
	STDMETHOD(EnumDevices)(LPD3DENUMDEVICESCALLBACK cb, LPVOID ctx) override { return dd_->EnumDevices(cb, ctx); }
	STDMETHOD(CreateLight)(LPDIRECT3DLIGHT *l, IUnknown *u) override { return dd_->CreateLight(l, u); }
	STDMETHOD(CreateMaterial)(LPDIRECT3DMATERIAL2 *, IUnknown *) override { NOT_IMPL("D3D2::CreateMaterial"); return DDERR_UNSUPPORTED; }
	STDMETHOD(CreateViewport)(LPDIRECT3DVIEWPORT2 *, IUnknown *) override { NOT_IMPL("D3D2::CreateViewport"); return DDERR_UNSUPPORTED; }
	STDMETHOD(FindDevice)(LPD3DFINDDEVICESEARCH s, LPD3DFINDDEVICERESULT r) override { return dd_->FindDevice(s, r); }
	STDMETHOD(CreateDevice)(REFCLSID, LPDIRECTDRAWSURFACE, LPDIRECT3DDEVICE2 *) override { NOT_IMPL("D3D2::CreateDevice"); return DDERR_UNSUPPORTED; }

private:
	DDraw *dd_;
};

IUnknown *NewDDraw2(DDraw *dd) { return new DDraw2(dd); }
IUnknown *NewD3D2(DDraw *dd) { return new D3D2(dd); }
