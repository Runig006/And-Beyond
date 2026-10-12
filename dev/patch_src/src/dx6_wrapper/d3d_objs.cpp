// Direct3D 3: device, viewport, lights, materials and vertex buffers. The device keeps the DirectX 6 render, light
// and texture stage states; each draw transforms the vertices on the CPU (tnl.cpp), turns the states into a GpuState
// (BuildState) and hands both to render.cpp.
#include "objects.h"

#define SIMPLE_QI(cls, iid)                                                              \
	HRESULT cls::QueryInterface(REFIID riid, void **out)                                  \
	{                                                                                    \
		if (riid == IID_IUnknown || riid == iid) {                                       \
			*out = this;                                                                 \
			AddRef();                                                                    \
			return S_OK;                                                                 \
		}                                                                                \
		*out = nullptr;                                                                  \
		return E_NOINTERFACE;                                                            \
	}
SIMPLE_QI(Material, IID_IDirect3DMaterial3)
SIMPLE_QI(LightObj, IID_IDirect3DLight)
SIMPLE_QI(Viewport, IID_IDirect3DViewport3)
SIMPLE_QI(VertexBuffer, IID_IDirect3DVertexBuffer)
SIMPLE_QI(Device, IID_IDirect3DDevice3)

// ------------------------------------------------------------------ material and light
HRESULT Material::SetMaterial(LPD3DMATERIAL m)
{
	mat = *m;
	return D3D_OK;
}
HRESULT Material::GetMaterial(LPD3DMATERIAL m)
{
	*m = mat;
	return D3D_OK;
}

HRESULT LightObj::SetLight(LPD3DLIGHT l)
{
	// D3DLIGHT (version 1) and D3DLIGHT2 share the start; D3DLIGHT2 adds dwFlags
	memset(&state.light, 0, sizeof state.light);
	memcpy(&state.light, l, l->dwSize < sizeof(D3DLIGHT2) ? l->dwSize : sizeof(D3DLIGHT2));
	if (l->dwSize < sizeof(D3DLIGHT2)) {
		state.light.dwFlags = D3DLIGHT_ACTIVE;
	}
	state.active = true;
	return D3D_OK;
}
HRESULT LightObj::GetLight(LPD3DLIGHT l)
{
	memcpy(l, &state.light, l->dwSize < sizeof(D3DLIGHT2) ? l->dwSize : sizeof(D3DLIGHT2));
	return D3D_OK;
}

// ------------------------------------------------------------------ viewport
HRESULT Viewport::GetViewport(LPD3DVIEWPORT v)
{
	v->dwX = vp.dwX;
	v->dwY = vp.dwY;
	v->dwWidth = vp.dwWidth;
	v->dwHeight = vp.dwHeight;
	v->dvMinZ = vp.dvMinZ;
	v->dvMaxZ = vp.dvMaxZ;
	v->dvScaleX = vp.dwWidth / 2.f;
	v->dvScaleY = vp.dwHeight / 2.f;
	v->dvMaxX = v->dvMaxY = 1;
	return D3D_OK;
}
HRESULT Viewport::SetViewport(LPD3DVIEWPORT v)
{
	vp.dwSize = sizeof vp;
	vp.dwX = v->dwX;
	vp.dwY = v->dwY;
	vp.dwWidth = v->dwWidth;
	vp.dwHeight = v->dwHeight;
	vp.dvClipX = -v->dvMaxX;
	vp.dvClipY = v->dvMaxY;
	vp.dvClipWidth = 2 * v->dvMaxX;
	vp.dvClipHeight = 2 * v->dvMaxY;
	vp.dvMinZ = v->dvMinZ;
	vp.dvMaxZ = v->dvMaxZ;
	return D3D_OK;
}
HRESULT Viewport::GetViewport2(LPD3DVIEWPORT2 v)
{
	*v = vp;
	return D3D_OK;
}
HRESULT Viewport::SetViewport2(LPD3DVIEWPORT2 v)
{
	vp = *v;
	if (vp.dvMaxZ <= vp.dvMinZ) {
		vp.dvMaxZ = vp.dvMinZ + 1;
	}
	return D3D_OK;
}

HRESULT Viewport::Clear(DWORD n, LPD3DRECT r, DWORD flags)
{
	DWORD color = 0;
	if (Material *m = (Material *)HandleGet(background)) {
		const D3DCOLORVALUE &c = m->mat.diffuse;
		color = 0xff000000 | (DWORD)(c.r * 255) << 16 | (DWORD)(c.g * 255) << 8 | (DWORD)(c.b * 255);
	}
	g_render.Clear(r, n, flags, color, 1.f);
	return D3D_OK;
}

HRESULT Viewport::Clear2(DWORD n, LPD3DRECT r, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD)
{
	g_render.Clear(r, n, flags, color, z);
	return D3D_OK;
}

HRESULT Viewport::AddLight(LPDIRECT3DLIGHT l)
{
	LightObj *o = static_cast<LightObj *>(l);
	o->AddRef();
	lights.push_back(o);
	return D3D_OK;
}
HRESULT Viewport::DeleteLight(LPDIRECT3DLIGHT l)
{
	for (size_t i = 0; i < lights.size(); i++) {
		if (lights[i] == static_cast<LightObj *>(l)) {
			lights[i]->Release();
			lights.erase(lights.begin() + i);
			break;
		}
	}
	return D3D_OK;
}

// ------------------------------------------------------------------ vertex buffer
VertexBuffer::VertexBuffer(const D3DVERTEXBUFFERDESC &d) : desc(d)
{
	layout = DecodeFvf(d.dwFVF);
	data.assign((size_t)layout.stride * d.dwNumVertices, 0);
}
HRESULT VertexBuffer::Lock(DWORD, LPVOID *p, LPDWORD size)
{
	*p = data.data();
	if (size) {
		*size = (DWORD)data.size();
	}
	return D3D_OK;
}
HRESULT VertexBuffer::GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC d)
{
	*d = desc;
	return D3D_OK;
}
HRESULT VertexBuffer::ProcessVertices(DWORD op, DWORD dstIndex, DWORD count, LPDIRECT3DVERTEXBUFFER srcIf, DWORD srcIndex,
                                      LPDIRECT3DDEVICE3 devIf, DWORD)
{
	VertexBuffer *src = static_cast<VertexBuffer *>(srcIf);
	Device *dev = static_cast<Device *>(devIf);
	if (!(op & D3DVOP_TRANSFORM)) {
		NOT_IMPL("ProcessVertices without D3DVOP_TRANSFORM");
		return D3D_OK;
	}
	if (dstIndex + count > desc.dwNumVertices || srcIndex + count > src->desc.dwNumVertices) {
		return DDERR_INVALIDPARAMS;
	}
	TnlState s = dev->Tnl();
	ProcessToScreen(s, src->data.data() + (size_t)srcIndex * src->layout.stride, src->layout, count,
	                (op & D3DVOP_LIGHT) != 0, data.data() + (size_t)dstIndex * layout.stride, layout);
	return D3D_OK;
}

// ------------------------------------------------------------------ device
static float AsFloat(DWORD d)
{
	float f;
	memcpy(&f, &d, 4);
	return f;
}

Device::Device(DDraw *dd, Surface *target) : dd_(dd), target_(target)
{
	world_ = view_ = proj_ = MatIdentity();
	// DirectX 6 defaults
	rs_[D3DRENDERSTATE_ZENABLE] = target && target->zbuffer ? D3DZB_TRUE : D3DZB_FALSE;
	rs_[D3DRENDERSTATE_ZWRITEENABLE] = TRUE;
	rs_[D3DRENDERSTATE_ZFUNC] = D3DCMP_LESSEQUAL;
	rs_[D3DRENDERSTATE_CULLMODE] = D3DCULL_CCW;
	rs_[D3DRENDERSTATE_SRCBLEND] = D3DBLEND_ONE;
	rs_[D3DRENDERSTATE_DESTBLEND] = D3DBLEND_ZERO;
	rs_[D3DRENDERSTATE_ALPHAFUNC] = D3DCMP_ALWAYS;
	rs_[D3DRENDERSTATE_SHADEMODE] = D3DSHADE_GOURAUD;
	rs_[D3DRENDERSTATE_FILLMODE] = D3DFILL_SOLID;
	rs_[D3DRENDERSTATE_TEXTUREMAPBLEND] = D3DTBLEND_MODULATE;
	rs_[D3DRENDERSTATE_TEXTUREADDRESS] = D3DTADDRESS_WRAP;
	rs_[D3DRENDERSTATE_TEXTUREMAG] = D3DFILTER_NEAREST;
	rs_[D3DRENDERSTATE_TEXTUREMIN] = D3DFILTER_NEAREST;
	rs_[D3DRENDERSTATE_TEXTUREPERSPECTIVE] = TRUE;
	rs_[D3DRENDERSTATE_FOGTABLEEND] = 0x3f800000; // 1.0
	rs_[D3DRENDERSTATE_FOGTABLEDENSITY] = 0x3f800000;
	rs_[D3DRENDERSTATE_TEXTUREFACTOR] = 0xffffffff;
	ls_[D3DLIGHTSTATE_COLORVERTEX] = TRUE;
	ls_[D3DLIGHTSTATE_FOGEND] = 0x3f800000;
	ls_[D3DLIGHTSTATE_FOGDENSITY] = 0x3f800000;
	for (int s = 0; s < 2; s++) {
		tss_[s][D3DTSS_COLOROP] = s == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
		tss_[s][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
		tss_[s][D3DTSS_COLORARG2] = D3DTA_CURRENT;
		tss_[s][D3DTSS_ALPHAOP] = s == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
		tss_[s][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
		tss_[s][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
		tss_[s][D3DTSS_ADDRESSU] = tss_[s][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
		tss_[s][D3DTSS_MAGFILTER] = tss_[s][D3DTSS_MINFILTER] = D3DTFG_POINT;
		tss_[s][D3DTSS_TEXCOORDINDEX] = s;
	}
	LogLine("Direct3D device created on %dx%d (Z %s)", target ? target->width : 0, target ? target->height : 0,
	        target && target->zbuffer ? "yes" : "no");
}

HRESULT Device::GetCaps(LPD3DDEVICEDESC hal, LPD3DDEVICEDESC hel)
{
	LPDIRECT3D3 d3d = static_cast<IDirect3D3 *>(dd_);
	D3DFINDDEVICESEARCH fs = {sizeof fs};
	D3DFINDDEVICERESULT fr = {sizeof fr};
	d3d->FindDevice(&fs, &fr);
	if (hal) {
		memcpy(hal, &fr.ddHwDesc, hal->dwSize ? hal->dwSize : sizeof(D3DDEVICEDESC));
	}
	if (hel) {
		memcpy(hel, &fr.ddSwDesc, hel->dwSize ? hel->dwSize : sizeof(D3DDEVICEDESC));
	}
	return D3D_OK;
}

HRESULT Device::AddViewport(LPDIRECT3DVIEWPORT3 v)
{
	Viewport *o = static_cast<Viewport *>(v);
	o->AddRef();
	viewports_.push_back(o);
	return D3D_OK;
}
HRESULT Device::DeleteViewport(LPDIRECT3DVIEWPORT3 v)
{
	for (size_t i = 0; i < viewports_.size(); i++) {
		if (viewports_[i] == static_cast<Viewport *>(v)) {
			if (viewport_ == viewports_[i]) {
				viewport_ = nullptr;
			}
			viewports_[i]->Release();
			viewports_.erase(viewports_.begin() + i);
			break;
		}
	}
	return D3D_OK;
}
HRESULT Device::SetCurrentViewport(LPDIRECT3DVIEWPORT3 v)
{
	viewport_ = static_cast<Viewport *>(v);
	return D3D_OK;
}
HRESULT Device::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *v)
{
	*v = viewport_;
	if (viewport_) {
		viewport_->AddRef();
	}
	return D3D_OK;
}

HRESULT Device::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK cb, LPVOID ctx)
{
	for (PixFmt f : {PF_565, PF_1555, PF_4444, PF_8888}) {
		DDPIXELFORMAT pf;
		MakePixelFormat(f, &pf);
		if (cb(&pf, ctx) == D3DENUMRET_CANCEL) {
			break;
		}
	}
	return D3D_OK;
}

HRESULT Device::GetDirect3D(LPDIRECT3D3 *d)
{
	*d = static_cast<IDirect3D3 *>(dd_);
	dd_->AddRef();
	return D3D_OK;
}
HRESULT Device::GetRenderTarget(LPDIRECTDRAWSURFACE4 *s)
{
	*s = target_;
	target_->AddRef();
	return D3D_OK;
}

HRESULT Device::GetRenderState(D3DRENDERSTATETYPE t, LPDWORD v)
{
	*v = t < 256 ? rs_[t] : 0;
	return D3D_OK;
}

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE t, DWORD v)
{
	if (t >= 256) {
		return DDERR_INVALIDPARAMS;
	}
	rs_[t] = v;
	switch (t) {
	case D3DRENDERSTATE_TEXTUREHANDLE: // textures by handle (old interface)
		tex_[0] = (Surface *)HandleGet(v);
		break;
	case D3DRENDERSTATE_TEXTUREMAPBLEND:
		legacyBlend_ = true;
		break;
	case D3DRENDERSTATE_ZENABLE: case D3DRENDERSTATE_ZWRITEENABLE: case D3DRENDERSTATE_ZFUNC:
	case D3DRENDERSTATE_ALPHABLENDENABLE: case D3DRENDERSTATE_SRCBLEND: case D3DRENDERSTATE_DESTBLEND:
	case D3DRENDERSTATE_CULLMODE: case D3DRENDERSTATE_FILLMODE: case D3DRENDERSTATE_SHADEMODE:
	case D3DRENDERSTATE_ALPHATESTENABLE: case D3DRENDERSTATE_ALPHAREF: case D3DRENDERSTATE_ALPHAFUNC:
	case D3DRENDERSTATE_FOGENABLE: case D3DRENDERSTATE_FOGCOLOR: case D3DRENDERSTATE_FOGTABLEMODE:
	case D3DRENDERSTATE_FOGTABLESTART: case D3DRENDERSTATE_FOGTABLEEND: case D3DRENDERSTATE_FOGTABLEDENSITY:
	case D3DRENDERSTATE_SPECULARENABLE: case D3DRENDERSTATE_COLORKEYENABLE: case D3DRENDERSTATE_TEXTUREFACTOR:
	case D3DRENDERSTATE_TEXTUREMAG: case D3DRENDERSTATE_TEXTUREMIN: case D3DRENDERSTATE_TEXTUREADDRESS:
	case D3DRENDERSTATE_TEXTUREADDRESSU: case D3DRENDERSTATE_TEXTUREADDRESSV:
	case D3DRENDERSTATE_TEXTUREPERSPECTIVE: case D3DRENDERSTATE_DITHERENABLE: case D3DRENDERSTATE_ZBIAS:
	case D3DRENDERSTATE_LASTPIXEL: case D3DRENDERSTATE_ANTIALIAS: case D3DRENDERSTATE_STIPPLEDALPHA:
	case D3DRENDERSTATE_SUBPIXEL: case D3DRENDERSTATE_SUBPIXELX: case D3DRENDERSTATE_EDGEANTIALIAS:
		break; // used by BuildState (or irrelevant for D3D11)
	default: {
		static bool seen[256];
		if (!seen[t]) {
			seen[t] = true;
			LogLine("unhandled render state: %d = %lu", t, v);
		}
	}
	}
	return D3D_OK;
}

HRESULT Device::GetLightState(D3DLIGHTSTATETYPE t, LPDWORD v)
{
	*v = t < 16 ? ls_[t] : 0;
	return D3D_OK;
}
HRESULT Device::SetLightState(D3DLIGHTSTATETYPE t, DWORD v)
{
	if (t < 16) {
		ls_[t] = v;
	}
	return D3D_OK;
}

static Mat4 &Pick(D3DTRANSFORMSTATETYPE t, Mat4 &w, Mat4 &v, Mat4 &p)
{
	return t == D3DTRANSFORMSTATE_VIEW ? v : t == D3DTRANSFORMSTATE_PROJECTION ? p : w;
}
HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE t, LPD3DMATRIX m)
{
	memcpy(&Pick(t, world_, view_, proj_), m, sizeof(Mat4));
	return D3D_OK;
}
HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE t, LPD3DMATRIX m)
{
	memcpy(m, &Pick(t, world_, view_, proj_), sizeof(Mat4));
	return D3D_OK;
}
HRESULT Device::MultiplyTransform(D3DTRANSFORMSTATETYPE t, LPD3DMATRIX m)
{
	Mat4 &dst = Pick(t, world_, view_, proj_);
	Mat4 b;
	memcpy(&b, m, sizeof b);
	dst = MatMul(b, dst);
	return D3D_OK;
}

HRESULT Device::GetTexture(DWORD stage, LPDIRECT3DTEXTURE2 *t)
{
	*t = stage < 2 ? tex_[stage] : nullptr;
	if (*t) {
		(*t)->AddRef();
	}
	return D3D_OK;
}
HRESULT Device::SetTexture(DWORD stage, LPDIRECT3DTEXTURE2 t)
{
	if (stage < 2) {
		tex_[stage] = SurfaceFromTexture(t, "SetTexture");
	}
	return D3D_OK;
}
HRESULT Device::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE t, LPDWORD v)
{
	*v = stage < 2 && t < 32 ? tss_[stage][t] : 0;
	return D3D_OK;
}
HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD v)
{
	if (stage >= 2 || t >= 32) {
		NOT_IMPL("SetTextureStageState stage >= 2");
		return D3D_OK;
	}
	tss_[stage][t] = v;
	if (stage == 0 && (t == D3DTSS_COLOROP || t == D3DTSS_ALPHAOP)) {
		legacyBlend_ = false;
	}
	return D3D_OK;
}

TnlState Device::Tnl()
{
	TnlState s;
	s.world = world_;
	s.view = view_;
	s.proj = proj_;
	if (viewport_) {
		s.viewport = viewport_->vp;
		for (LightObj *l : viewport_->lights) {
			s.lights.push_back(&l->state);
		}
	} else {
		s.viewport.dwWidth = g_render.Width();
		s.viewport.dwHeight = g_render.Height();
		s.viewport.dvMaxZ = 1;
	}
	if (Material *m = (Material *)HandleGet(ls_[D3DLIGHTSTATE_MATERIAL])) {
		s.material = m->mat;
	} else {
		s.material.diffuse = s.material.ambient = {1, 1, 1, 1};
	}
	s.ambient = ls_[D3DLIGHTSTATE_AMBIENT];
	s.colorVertex = ls_[D3DLIGHTSTATE_COLORVERTEX] != 0;
	s.specular = rs_[D3DRENDERSTATE_SPECULARENABLE] != 0;
	s.fogMode = rs_[D3DRENDERSTATE_FOGENABLE] && !rs_[D3DRENDERSTATE_FOGTABLEMODE] ? ls_[D3DLIGHTSTATE_FOGMODE] : 0;
	s.fogStart = AsFloat(ls_[D3DLIGHTSTATE_FOGSTART]);
	s.fogEnd = AsFloat(ls_[D3DLIGHTSTATE_FOGEND]);
	s.fogDensity = AsFloat(ls_[D3DLIGHTSTATE_FOGDENSITY]);
	s.rtWidth = g_render.Width();
	s.rtHeight = g_render.Height();
	return s;
}

static int MapFilter(DWORD tssFilter) { return tssFilter == D3DTFG_POINT ? D3DTFG_POINT : D3DTFG_LINEAR; }

GpuState Device::BuildState(bool pretransformed, Surface *&tex0)
{
	GpuState st;
	st.zEnable = rs_[D3DRENDERSTATE_ZENABLE] != D3DZB_FALSE && target_ && target_->zbuffer;
	st.zWrite = st.zEnable && rs_[D3DRENDERSTATE_ZWRITEENABLE];
	st.zFunc = rs_[D3DRENDERSTATE_ZFUNC];
	st.blend = rs_[D3DRENDERSTATE_ALPHABLENDENABLE] != 0;
	st.srcBlend = rs_[D3DRENDERSTATE_SRCBLEND];
	st.dstBlend = rs_[D3DRENDERSTATE_DESTBLEND];
	st.cull = rs_[D3DRENDERSTATE_CULLMODE];
	st.wireframe = rs_[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME;
	tex0 = tex_[0];

	PsConsts &p = st.ps;
	for (int s = 0; s < 2; s++) {
		Surface *t = tex_[s];
		st.tex[s] = t ? t->Srv() : nullptr;
		if (t && !st.tex[s]) {
			static int shown;
			if (shown++ < 16) {
				LogLine("texture not usable: stage %d kind %d %dx%d fmt %d caps %08lx", s, t->kind, t->width, t->height,
				        t->fmt, t->Caps());
			}
		}
		p.texOn[s] = st.tex[s] != nullptr;
		// Filter: stage states rule; the old TEXTUREMAG applies to stage 0 when it was used
		DWORD mag = tss_[s][D3DTSS_MAGFILTER];
		if (s == 0 && rs_[D3DRENDERSTATE_TEXTUREMAG] != D3DFILTER_NEAREST) {
			mag = D3DTFG_LINEAR;
		}
		st.filter[s] = MapFilter(mag);
		st.addrU[s] = tss_[s][D3DTSS_ADDRESSU];
		st.addrV[s] = tss_[s][D3DTSS_ADDRESSV];
		int32_t *c = s == 0 ? p.stage0 : p.stage1, *a = s == 0 ? p.alpha0 : p.alpha1;
		c[0] = tss_[s][D3DTSS_COLOROP];
		c[1] = tss_[s][D3DTSS_COLORARG1];
		c[2] = tss_[s][D3DTSS_COLORARG2];
		a[0] = tss_[s][D3DTSS_ALPHAOP];
		a[1] = tss_[s][D3DTSS_ALPHAARG1];
		a[2] = tss_[s][D3DTSS_ALPHAARG2];
	}
	// DirectX 5 TEXTUREMAPBLEND translated into stages (as DirectX 6 does)
	if (legacyBlend_) {
		bool alpha = tex0 && tex0->HasAlpha();
		p.stage1[0] = p.alpha1[0] = D3DTOP_DISABLE;
		switch (rs_[D3DRENDERSTATE_TEXTUREMAPBLEND]) {
		case D3DTBLEND_DECAL: case D3DTBLEND_COPY:
			p.stage0[0] = D3DTOP_SELECTARG1; p.stage0[1] = D3DTA_TEXTURE;
			p.alpha0[0] = D3DTOP_SELECTARG1; p.alpha0[1] = D3DTA_TEXTURE;
			break;
		case D3DTBLEND_DECALALPHA:
			p.stage0[0] = D3DTOP_BLENDTEXTUREALPHA; p.stage0[1] = D3DTA_TEXTURE; p.stage0[2] = D3DTA_DIFFUSE;
			p.alpha0[0] = D3DTOP_SELECTARG2; p.alpha0[2] = D3DTA_DIFFUSE;
			break;
		case D3DTBLEND_MODULATEALPHA:
			p.stage0[0] = D3DTOP_MODULATE; p.stage0[1] = D3DTA_TEXTURE; p.stage0[2] = D3DTA_DIFFUSE;
			p.alpha0[0] = D3DTOP_MODULATE; p.alpha0[1] = D3DTA_TEXTURE; p.alpha0[2] = D3DTA_DIFFUSE;
			break;
		case D3DTBLEND_ADD:
			p.stage0[0] = D3DTOP_ADD; p.stage0[1] = D3DTA_TEXTURE; p.stage0[2] = D3DTA_DIFFUSE;
			p.alpha0[0] = D3DTOP_SELECTARG2; p.alpha0[2] = D3DTA_DIFFUSE;
			break;
		default: // MODULATE: texture alpha if it has one, vertex alpha otherwise
			p.stage0[0] = D3DTOP_MODULATE; p.stage0[1] = D3DTA_TEXTURE; p.stage0[2] = D3DTA_DIFFUSE;
			p.alpha0[0] = alpha ? D3DTOP_SELECTARG1 : D3DTOP_SELECTARG2; p.alpha0[1] = D3DTA_TEXTURE; p.alpha0[2] = D3DTA_DIFFUSE;
			break;
		}
		if (rs_[D3DRENDERSTATE_TEXTUREADDRESS]) {
			st.addrU[0] = st.addrV[0] = rs_[D3DRENDERSTATE_TEXTUREADDRESS];
		}
	}
	if (rs_[D3DRENDERSTATE_TEXTUREADDRESSU]) {
		st.addrU[0] = rs_[D3DRENDERSTATE_TEXTUREADDRESSU];
	}
	if (rs_[D3DRENDERSTATE_TEXTUREADDRESSV]) {
		st.addrV[0] = rs_[D3DRENDERSTATE_TEXTUREADDRESSV];
	}
	DWORD tf = rs_[D3DRENDERSTATE_TEXTUREFACTOR];
	p.tfactor[0] = ((tf >> 16) & 255) / 255.f;
	p.tfactor[1] = ((tf >> 8) & 255) / 255.f;
	p.tfactor[2] = (tf & 255) / 255.f;
	p.tfactor[3] = (tf >> 24) / 255.f;
	DWORD fc = rs_[D3DRENDERSTATE_FOGCOLOR];
	p.fogColor[0] = ((fc >> 16) & 255) / 255.f;
	p.fogColor[1] = ((fc >> 8) & 255) / 255.f;
	p.fogColor[2] = (fc & 255) / 255.f;
	p.fogParams[0] = AsFloat(rs_[D3DRENDERSTATE_FOGTABLESTART]);
	p.fogParams[1] = AsFloat(rs_[D3DRENDERSTATE_FOGTABLEEND]);
	p.fogParams[2] = AsFloat(rs_[D3DRENDERSTATE_FOGTABLEDENSITY]);
	p.fogParams[3] = (float)rs_[D3DRENDERSTATE_FOGTABLEMODE];
	p.flags[0] = rs_[D3DRENDERSTATE_FOGENABLE] != 0;
	p.flags[1] = rs_[D3DRENDERSTATE_ALPHATESTENABLE] ? (int)rs_[D3DRENDERSTATE_ALPHAFUNC] : 0;
	p.flags[2] = rs_[D3DRENDERSTATE_SPECULARENABLE] != 0;
	p.flags[3] = rs_[D3DRENDERSTATE_COLORKEYENABLE] && tex0 && tex0->HasColorKey();
	DWORD ref = rs_[D3DRENDERSTATE_ALPHAREF];
	p.alphaRef[0] = ref > 255 ? ref / 65536.f : ref / 255.f; // DirectX 5 used 16.16, DirectX 6 uses 0..255

	int rtw = g_render.Width(), rth = g_render.Height();
	const D3DVIEWPORT2 *vp = viewport_ ? &viewport_->vp : nullptr;
	if (pretransformed || !vp || !vp->dwWidth) {
		st.viewport = {0, 0, (float)rtw, (float)rth, 0, 1};
	} else {
		st.viewport = {(float)vp->dwX, (float)vp->dwY, (float)vp->dwWidth, (float)vp->dwHeight, vp->dvMinZ, vp->dvMaxZ};
	}
	if (vp && vp->dwWidth) {
		st.scissor = {(LONG)vp->dwX, (LONG)vp->dwY, (LONG)(vp->dwX + vp->dwWidth), (LONG)(vp->dwY + vp->dwHeight)};
	} else {
		st.scissor = {0, 0, rtw, rth};
	}
	return st;
}

void Device::Draw(D3DPRIMITIVETYPE type, const FvfLayout &fl, const BYTE *verts, int nverts, const WORD *idx, int nidx,
                  DWORD flags)
{
	if (nverts <= 0) {
		return;
	}
	gv_.resize(nverts);
	TnlState s = Tnl();
	TransformToClip(s, verts, fl, nverts, !(flags & D3DDP_DONOTLIGHT), gv_.data());
	Surface *tex0;
	GpuState st = BuildState(fl.rhw, tex0);
	D3D_PRIMITIVE_TOPOLOGY topo;
	gi_.clear();
	int count = idx ? nidx : nverts;
	switch (type) {
	case D3DPT_POINTLIST: topo = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
	case D3DPT_LINELIST: topo = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
	case D3DPT_LINESTRIP: topo = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
	case D3DPT_TRIANGLELIST: topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
	case D3DPT_TRIANGLESTRIP: topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
	case D3DPT_TRIANGLEFAN:
		// D3D11 has no fans: convert to a list
		topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		for (int i = 1; i + 1 < count; i++) {
			gi_.push_back(idx ? idx[0] : 0);
			gi_.push_back(idx ? idx[i] : (uint16_t)i);
			gi_.push_back(idx ? idx[i + 1] : (uint16_t)(i + 1));
		}
		g_render.Draw(topo, gv_.data(), nverts, gi_.data(), (int)gi_.size(), st);
		return;
	default: NOT_IMPL("unknown primitive type"); return;
	}
	if (idx) {
		g_render.Draw(topo, gv_.data(), nverts, (const uint16_t *)idx, nidx, st);
	} else {
		g_render.Draw(topo, gv_.data(), nverts, nullptr, 0, st);
	}
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE t, DWORD fvf, LPVOID v, DWORD n, DWORD flags)
{
	Draw(t, DecodeFvf(fvf), (const BYTE *)v, n, nullptr, 0, flags);
	return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE t, DWORD fvf, LPVOID v, DWORD nv, LPWORD idx, DWORD ni, DWORD flags)
{
	Draw(t, DecodeFvf(fvf), (const BYTE *)v, nv, idx, ni, flags);
	return D3D_OK;
}

HRESULT Device::DrawPrimitiveVB(D3DPRIMITIVETYPE t, LPDIRECT3DVERTEXBUFFER vbIf, DWORD start, DWORD n, DWORD flags)
{
	VertexBuffer *vb = static_cast<VertexBuffer *>(vbIf);
	Draw(t, vb->layout, vb->data.data() + (size_t)start * vb->layout.stride, n, nullptr, 0, flags);
	return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE t, LPDIRECT3DVERTEXBUFFER vbIf, LPWORD idx, DWORD ni, DWORD flags)
{
	VertexBuffer *vb = static_cast<VertexBuffer *>(vbIf);
	Draw(t, vb->layout, vb->data.data(), vb->desc.dwNumVertices, idx, ni, flags);
	return D3D_OK;
}
