// Direct3D 11 core: device, the game's back buffer (a texture of the display mode's size), drawing, 2D copies,
// presenting (the back buffer scaled into the window, black bars if the aspect ratios differ) and textures.
#include "render.h"
#include <d3dcompiler.h>

Render g_render;

// The shaders live in shader.hlsl, built into the DLL as a resource (buzzpatch.rc)
static const char *ShaderSource(size_t *len)
{
	HMODULE self = nullptr;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                   (LPCSTR)&ShaderSource, &self);
	HRSRC res = FindResourceA(self, "SHADER_HLSL", MAKEINTRESOURCEA(10)); // RT_RCDATA
	if (!res) {
		LogLine("ERROR: shader.hlsl resource not found");
		*len = 0;
		return "";
	}
	*len = SizeofResource(self, res);
	return (const char *)LockResource(LoadResource(self, res));
}

static ID3DBlob *Compile(const char *entry, const char *target)
{
	ID3DBlob *code = nullptr, *err = nullptr;
	size_t len;
	const char *src = ShaderSource(&len);
	HRESULT hr = D3DCompile(src, len, "shader.hlsl", nullptr, nullptr, entry, target,
	                        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &err);
	if (FAILED(hr)) {
		LogLine("ERROR compiling %s: %s", entry, err ? (const char *)err->GetBufferPointer() : "?");
	}
	SafeRelease(err);
	return code;
}

bool Render::Init(HWND hwnd)
{
	if (dev_) {
		return true;
	}
	hwnd_ = hwnd;
	D3D_FEATURE_LEVEL want[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, want, 3, D3D11_SDK_VERSION,
	                               &dev_, nullptr, &ctx_);
	if (FAILED(hr)) {
		LogLine("ERROR D3D11CreateDevice %08lx", hr);
		return false;
	}
	ID3DBlob *vsb = Compile("vs", "vs_4_0"), *psb = Compile("ps", "ps_4_0");
	ID3DBlob *bvs = Compile("blitvs", "vs_4_0"), *bps = Compile("blitps", "ps_4_0");
	if (!vsb || !psb || !bvs || !bps) {
		return false;
	}
	dev_->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs_);
	dev_->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps_);
	dev_->CreateVertexShader(bvs->GetBufferPointer(), bvs->GetBufferSize(), nullptr, &blitVs_);
	dev_->CreatePixelShader(bps->GetBufferPointer(), bps->GetBufferSize(), nullptr, &blitPs_);
	D3D11_INPUT_ELEMENT_DESC el[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 1, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	dev_->CreateInputLayout(el, 6, vsb->GetBufferPointer(), vsb->GetBufferSize(), &layout_);
	vsb->Release();
	psb->Release();
	bvs->Release();
	bps->Release();
	D3D11_BUFFER_DESC cbd = {sizeof(PsConsts), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER,
	                         D3D11_CPU_ACCESS_WRITE, 0, 0};
	dev_->CreateBuffer(&cbd, nullptr, &cb_);

	if (!CreateSwapChain(hwnd)) {
		return false;
	}
	LogLine("D3D11 ready (window %dx%d)", swapW_, swapH_);
	if (!w_) {
		SetMode(swapW_, swapH_, false);
	}
	return true;
}

void Render::SetMode(int w, int h, bool fullscreen)
{
	if (fullscreen) {
		ApplyWindowMode();
	}
	if (w == w_ && h == h_ && rt_) {
		return;
	}
	w_ = w;
	h_ = h;
	if (dev_) {
		CreateTargets();
	}
	LogLine("mode %dx%d %s", w, h, fullscreen ? "fullscreen" : "windowed");
}

void Render::CreateTargets()
{
	SafeRelease(rtv_);
	SafeRelease(rtSrv_);
	SafeRelease(rt_);
	SafeRelease(staging_);
	SafeRelease(dsv_);
	SafeRelease(depth_);
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = w_;
	td.Height = h_;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	dev_->CreateTexture2D(&td, nullptr, &rt_);
	dev_->CreateRenderTargetView(rt_, nullptr, &rtv_);
	dev_->CreateShaderResourceView(rt_, nullptr, &rtSrv_);
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dev_->CreateTexture2D(&td, nullptr, &depth_);
	dev_->CreateDepthStencilView(depth_, nullptr, &dsv_);
	float black[4] = {0, 0, 0, 1};
	ctx_->ClearRenderTargetView(rtv_, black);
	ctx_->ClearDepthStencilView(dsv_, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Render::Clear(const D3DRECT *rects, int n, DWORD flags, DWORD color, float z)
{
	if (!rtv_) {
		return;
	}
	float c[4] = {((color >> 16) & 255) / 255.f, ((color >> 8) & 255) / 255.f, (color & 255) / 255.f,
	              (color >> 24) / 255.f};
	for (int i = 0; i < (n ? n : 1); i++) {
		D3D11_RECT r = {0, 0, w_, h_};
		if (n) {
			r = {rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2};
		}
		bool whole = r.left <= 0 && r.top <= 0 && r.right >= w_ && r.bottom >= h_;
		if (whole || !(flags & D3DCLEAR_TARGET)) {
			if (flags & D3DCLEAR_TARGET) {
				ctx_->ClearRenderTargetView(rtv_, c);
			}
			if (flags & D3DCLEAR_ZBUFFER) {
				ctx_->ClearDepthStencilView(dsv_, D3D11_CLEAR_DEPTH, z, 0);
			}
			continue;
		}
		ID3D11DeviceContext1 *ctx1 = nullptr;
		if (SUCCEEDED(ctx_->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)&ctx1))) {
			ctx1->ClearView(rtv_, c, &r, 1);
			if (flags & D3DCLEAR_ZBUFFER) {
				ctx1->ClearView(dsv_, &z, &r, 1);
			}
			ctx1->Release();
		}
	}
}

void Render::EnsureBuffers(int vbytes, int ibytes)
{
	if (vbytes > vbBytes_) {
		SafeRelease(vb_);
		vbBytes_ = vbytes * 2 > 1 << 20 ? vbytes * 2 : 1 << 20;
		D3D11_BUFFER_DESC d = {(UINT)vbBytes_, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE};
		dev_->CreateBuffer(&d, nullptr, &vb_);
	}
	if (ibytes > ibBytes_) {
		SafeRelease(ib_);
		ibBytes_ = ibytes * 2 > 1 << 18 ? ibytes * 2 : 1 << 18;
		D3D11_BUFFER_DESC d = {(UINT)ibBytes_, D3D11_USAGE_DYNAMIC, D3D11_BIND_INDEX_BUFFER, D3D11_CPU_ACCESS_WRITE};
		dev_->CreateBuffer(&d, nullptr, &ib_);
	}
}

void Render::Draw(D3D_PRIMITIVE_TOPOLOGY topo, const GpuVertex *v, int nv, const uint16_t *idx, int ni,
                  const GpuState &st)
{
	if (!rtv_ || nv <= 0) {
		return;
	}
	DiagCountDraw(nv);
	EnsureBuffers(nv * sizeof(GpuVertex), ni * 2);
	D3D11_MAPPED_SUBRESOURCE m;
	ctx_->Map(vb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
	memcpy(m.pData, v, nv * sizeof(GpuVertex));
	ctx_->Unmap(vb_, 0);
	if (ni) {
		ctx_->Map(ib_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
		memcpy(m.pData, idx, ni * 2);
		ctx_->Unmap(ib_, 0);
	}
	PsConsts ps = st.ps;
	SeeThrough see;
	if (st.zEnable && st.zWrite && !st.blend && seeThrough && seeThrough(see)) { // solid 3D only: not 2D, glows, particles or shadows
		memcpy(ps.seeBuzz, see.buzz, sizeof see.buzz);
		ps.seeBuzz[3] = see.radius;
		float p[] = {see.proj[0], see.proj[5], see.proj[10], see.proj[14], see.proj[8], see.proj[9], see.margin, 1};
		memcpy(ps.seeProj, p, sizeof p);
		float vp[] = {st.viewport.TopLeftX, st.viewport.TopLeftY, st.viewport.Width, st.viewport.Height};
		memcpy(ps.seeVp, vp, sizeof vp);
	}
	ctx_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
	memcpy(m.pData, &ps, sizeof ps);
	ctx_->Unmap(cb_, 0);

	UINT stride = sizeof(GpuVertex), off = 0;
	ctx_->IASetInputLayout(layout_);
	ctx_->IASetVertexBuffers(0, 1, &vb_, &stride, &off);
	ctx_->IASetIndexBuffer(ib_, DXGI_FORMAT_R16_UINT, 0);
	ctx_->IASetPrimitiveTopology(topo);
	ctx_->VSSetShader(vs_, nullptr, 0);
	ctx_->PSSetShader(ps_, nullptr, 0);
	ctx_->PSSetConstantBuffers(0, 1, &cb_);
	ctx_->PSSetShaderResources(0, 2, (ID3D11ShaderResourceView *const *)st.tex);
	ID3D11SamplerState *samp[2] = {Sampler(st.filter[0], st.addrU[0], st.addrV[0]),
	                               Sampler(st.filter[1], st.addrU[1], st.addrV[1])};
	ctx_->PSSetSamplers(0, 2, samp);
	ctx_->OMSetRenderTargets(1, &rtv_, dsv_);
	ctx_->OMSetBlendState(Blend(st), nullptr, 0xffffffff);
	ctx_->OMSetDepthStencilState(Depth(st), 0);
	ctx_->RSSetState(Raster(st));
	ctx_->RSSetViewports(1, &st.viewport);
	D3D11_RECT sc = {st.scissor.left, st.scissor.top, st.scissor.right, st.scissor.bottom};
	ctx_->RSSetScissorRects(1, &sc);
	if (ni) {
		ctx_->DrawIndexed(ni, 0, 0);
	} else {
		ctx_->Draw(nv, 0);
	}
}

// The 4 vertices (x y z w u v) of a quad covering dst on a vw x vh target, showing src of a tw x th texture
static void QuadVerts(float out[4][6], const RECT &dst, int tw, int th, const RECT &src, int vw, int vh)
{
	float x0 = dst.left * 2.f / vw - 1, x1 = dst.right * 2.f / vw - 1;
	float y0 = 1 - dst.top * 2.f / vh, y1 = 1 - dst.bottom * 2.f / vh;
	float u0 = (float)src.left / tw, u1 = (float)src.right / tw, v0 = (float)src.top / th, v1 = (float)src.bottom / th;
	float q[4][6] = {{x0, y0, 0, 1, u0, v0}, {x1, y0, 0, 1, u1, v0}, {x0, y1, 0, 1, u0, v1}, {x1, y1, 0, 1, u1, v1}};
	memcpy(out, q, sizeof q);
}

// Draws a quad (triangle strip) with the copy shader
static void DrawQuad(ID3D11Device *dev, ID3D11DeviceContext *ctx, ID3D11VertexShader *vs, ID3D11PixelShader *ps,
                     float q[4][6])
{
	static ID3D11Buffer *vb;
	static ID3D11InputLayout *layout;
	if (!vb) {
		D3D11_BUFFER_DESC d = {sizeof(float) * 24, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE};
		dev->CreateBuffer(&d, nullptr, &vb);
		ID3DBlob *b = Compile("blitvs", "vs_4_0");
		D3D11_INPUT_ELEMENT_DESC el[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		dev->CreateInputLayout(el, 2, b->GetBufferPointer(), b->GetBufferSize(), &layout);
		b->Release();
	}
	D3D11_MAPPED_SUBRESOURCE m;
	ctx->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
	memcpy(m.pData, q, sizeof(float) * 24);
	ctx->Unmap(vb, 0);
	UINT stride = 24, off = 0;
	ctx->IASetInputLayout(layout);
	ctx->IASetVertexBuffers(0, 1, &vb, &stride, &off);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->VSSetShader(vs, nullptr, 0);
	ctx->PSSetShader(ps, nullptr, 0);
	ctx->Draw(4, 0);
}

void Render::Blit(ID3D11ShaderResourceView *srv, int sw, int sh, const RECT &src, const RECT &dst, bool keyed,
                  bool alphaBlend)
{
	if (!rtv_) {
		return;
	}
	GpuState st;
	st.zEnable = st.zWrite = false;
	st.cull = D3DCULL_NONE;
	st.ps.flags[3] = keyed;
	if (alphaBlend) {
		st.blend = true;
		st.srcBlend = D3DBLEND_SRCALPHA;
		st.dstBlend = D3DBLEND_INVSRCALPHA;
	}
	D3D11_MAPPED_SUBRESOURCE m;
	ctx_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
	memcpy(m.pData, &st.ps, sizeof st.ps);
	ctx_->Unmap(cb_, 0);
	ctx_->PSSetConstantBuffers(0, 1, &cb_);
	ID3D11SamplerState *s = Sampler(D3DTFG_POINT, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP);
	ctx_->PSSetSamplers(0, 1, &s);
	ctx_->PSSetShaderResources(0, 1, &srv);
	ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
	ctx_->OMSetBlendState(Blend(st), nullptr, 0xffffffff);
	ctx_->OMSetDepthStencilState(Depth(st), 0);
	ctx_->RSSetState(Raster(st));
	D3D11_VIEWPORT vp = {0, 0, (float)w_, (float)h_, 0, 1};
	ctx_->RSSetViewports(1, &vp);
	D3D11_RECT sc = {0, 0, w_, h_};
	ctx_->RSSetScissorRects(1, &sc);
	RECT d = dst;
	int w43 = h_ * 4 / 3;
	if (blitTo4x3 && w_ > w43) {
		int left = (w_ - w43) / 2;
		d.left = left + MulDiv(dst.left, w43, w_);
		d.right = left + MulDiv(dst.right, w43, w_);
	}
	float q[4][6];
	QuadVerts(q, d, sw, sh, src, w_, h_);
	DrawQuad(dev_, ctx_, blitVs_, blitPs_, q);
	ID3D11ShaderResourceView *none = nullptr;
	ctx_->PSSetShaderResources(0, 1, &none);
}

void Render::FillRect(int x1, int y1, int x2, int y2, DWORD argb)
{
	D3DRECT r = {x1 < 0 ? 0 : x1, y1 < 0 ? 0 : y1, x2 > w_ ? w_ : x2, y2 > h_ ? h_ : y2};
	if (r.x1 < r.x2 && r.y1 < r.y2) {
		Clear(&r, 1, D3DCLEAR_TARGET, argb, 1.f);
	}
}

void Render::MaskOutside4x3()
{
	int w43 = h_ * 4 / 3;
	if (w_ <= w43) {
		return;
	}
	int left = (w_ - w43) / 2;
	D3DRECT bars[2] = {{0, 0, left, h_}, {left + w43, 0, w_, h_}};
	Clear(bars, 2, D3DCLEAR_TARGET, 0xff000000, 1.f);
}

void Render::Present()
{
	if (!swap_ || !rtSrv_) {
		return;
	}
	for (int i = 0; i < beforePresentCount_; i++) {
		beforePresent_[i]();
	}
	ResizeSwapChainIfNeeded();
	ID3D11Texture2D *bb = nullptr;
	ID3D11RenderTargetView *bbv = nullptr;
	HRESULT hr = swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&bb);
	if (FAILED(hr) || !bb) {
		LogLine("ERROR Present: GetBuffer %08lx (swap chain %dx%d)", hr, swapW_, swapH_);
		return;
	}
	hr = dev_->CreateRenderTargetView(bb, nullptr, &bbv);
	if (FAILED(hr) || !bbv) {
		LogLine("ERROR Present: CreateRenderTargetView %08lx", hr);
		bb->Release();
		return;
	}
	float black[4] = {0, 0, 0, 1};
	ctx_->ClearRenderTargetView(bbv, black);
	// Fit the game image into the window without distortion (black bars if the aspect ratios differ)
	float sx = (float)swapW_ / w_, sy = (float)swapH_ / h_, s = sx < sy ? sx : sy;
	float vw = w_ * s, vh = h_ * s;
	D3D11_VIEWPORT vp = {(swapW_ - vw) / 2, (swapH_ - vh) / 2, vw, vh, 0, 1};
	ctx_->OMSetRenderTargets(1, &bbv, nullptr);
	GpuState st;
	st.zEnable = st.zWrite = false;
	st.cull = D3DCULL_NONE;
	D3D11_MAPPED_SUBRESOURCE m;
	ctx_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
	memcpy(m.pData, &st.ps, sizeof st.ps);
	ctx_->Unmap(cb_, 0);
	ctx_->PSSetConstantBuffers(0, 1, &cb_);
	ctx_->OMSetBlendState(Blend(st), nullptr, 0xffffffff);
	ctx_->OMSetDepthStencilState(Depth(st), 0);
	ctx_->RSSetState(Raster(st));
	ctx_->RSSetViewports(1, &vp);
	D3D11_RECT sc = {0, 0, swapW_, swapH_};
	ctx_->RSSetScissorRects(1, &sc);
	ID3D11SamplerState *smp = Sampler(s == 1.f ? D3DTFG_POINT : D3DTFG_LINEAR, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP);
	ctx_->PSSetSamplers(0, 1, &smp);
	ctx_->PSSetShaderResources(0, 1, &rtSrv_);
	float q[4][6] = {{-1, 1, 0, 1, 0, 0}, {1, 1, 0, 1, 1, 0}, {-1, -1, 0, 1, 0, 1}, {1, -1, 0, 1, 1, 1}};
	DrawQuad(dev_, ctx_, blitVs_, blitPs_, q);
	ID3D11ShaderResourceView *none = nullptr;
	ctx_->PSSetShaderResources(0, 1, &none);
	swap_->Present(paced ? 0 : 1, 0);
	paced = false;
	DiagCountFrame();
	bbv->Release();
	bb->Release();
}

GpuTexture Render::CreateTexture(int w, int h)
{
	GpuTexture t;
	t.w = w;
	t.h = h;
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = w;
	td.Height = h;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dev_->CreateTexture2D(&td, nullptr, &t.tex);
	dev_->CreateShaderResourceView(t.tex, nullptr, &t.srv);
	return t;
}

void Render::Upload(GpuTexture &t, const uint32_t *bgra, int pitchPixels)
{
	ctx_->UpdateSubresource(t.tex, 0, nullptr, bgra, pitchPixels * 4, 0);
}

void Render::Free(GpuTexture &t)
{
	SafeRelease(t.srv);
	SafeRelease(t.tex);
}

void Render::ReadBack(uint32_t *out, int pitchPixels)
{
	if (!rt_) {
		return;
	}
	if (!staging_) {
		D3D11_TEXTURE2D_DESC td;
		rt_->GetDesc(&td);
		td.Usage = D3D11_USAGE_STAGING;
		td.BindFlags = 0;
		td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		dev_->CreateTexture2D(&td, nullptr, &staging_);
	}
	ctx_->CopyResource(staging_, rt_);
	D3D11_MAPPED_SUBRESOURCE m;
	if (FAILED(ctx_->Map(staging_, 0, D3D11_MAP_READ, 0, &m))) {
		return;
	}
	for (int y = 0; y < h_; y++) {
		memcpy(out + (size_t)y * pitchPixels, (const uint8_t *)m.pData + (size_t)y * m.RowPitch, w_ * 4);
	}
	ctx_->Unmap(staging_, 0);
}

void Render::WriteBack(const uint32_t *in, int pitchPixels, const RECT &r)
{
	if (!rt_ || r.right <= r.left || r.bottom <= r.top) {
		return;
	}
	D3D11_BOX box = {(UINT)r.left, (UINT)r.top, 0, (UINT)r.right, (UINT)r.bottom, 1};
	ctx_->UpdateSubresource(rt_, 0, &box, in + (size_t)r.top * pitchPixels + r.left, pitchPixels * 4, 0);
}
