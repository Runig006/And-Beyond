// DirectX 6 render states -> Direct3D 11 state objects (blend, depth, rasterizer, sampler), created once and cached.
#include "render.h"
#include <unordered_map>

static D3D11_BLEND MapBlend(int b, bool src)
{
	switch (b) {
	case D3DBLEND_ZERO: return D3D11_BLEND_ZERO;
	case D3DBLEND_ONE: return D3D11_BLEND_ONE;
	case D3DBLEND_SRCCOLOR: return D3D11_BLEND_SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR: return D3D11_BLEND_INV_SRC_COLOR;
	case D3DBLEND_SRCALPHA: return D3D11_BLEND_SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
	case D3DBLEND_DESTALPHA: return D3D11_BLEND_DEST_ALPHA;
	case D3DBLEND_INVDESTALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
	case D3DBLEND_DESTCOLOR: return D3D11_BLEND_DEST_COLOR;
	case D3DBLEND_INVDESTCOLOR: return D3D11_BLEND_INV_DEST_COLOR;
	case D3DBLEND_SRCALPHASAT: return D3D11_BLEND_SRC_ALPHA_SAT;
	case D3DBLEND_BOTHSRCALPHA: return src ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_INV_SRC_ALPHA;
	case D3DBLEND_BOTHINVSRCALPHA: return src ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_SRC_ALPHA;
	default: return src ? D3D11_BLEND_ONE : D3D11_BLEND_ZERO;
	}
}

ID3D11BlendState *Render::Blend(const GpuState &st)
{
	static std::unordered_map<uint32_t, ID3D11BlendState *> cache;
	int src = st.srcBlend, dst = st.dstBlend;
	if (src == D3DBLEND_BOTHSRCALPHA || src == D3DBLEND_BOTHINVSRCALPHA) {
		dst = src;
	}
	uint32_t key = st.blend ? (1u << 16) | (src << 8) | dst : 0;
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	D3D11_BLEND_DESC d = {};
	d.RenderTarget[0].BlendEnable = st.blend;
	d.RenderTarget[0].SrcBlend = MapBlend(src, true);
	d.RenderTarget[0].DestBlend = MapBlend(dst, false);
	d.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	d.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	d.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	d.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	d.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	ID3D11BlendState *s = nullptr;
	dev_->CreateBlendState(&d, &s);
	cache[key] = s;
	return s;
}

ID3D11DepthStencilState *Render::Depth(const GpuState &st)
{
	static std::unordered_map<uint32_t, ID3D11DepthStencilState *> cache;
	uint32_t key = (st.zEnable << 8) | (st.zWrite << 9) | st.zFunc;
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	D3D11_DEPTH_STENCIL_DESC d = {};
	d.DepthEnable = st.zEnable;
	d.DepthWriteMask = st.zWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	// D3DCMPFUNC and D3D11_COMPARISON_FUNC share values (1 never ... 8 always)
	d.DepthFunc = (st.zFunc >= 1 && st.zFunc <= 8) ? (D3D11_COMPARISON_FUNC)st.zFunc : D3D11_COMPARISON_LESS_EQUAL;
	ID3D11DepthStencilState *s = nullptr;
	dev_->CreateDepthStencilState(&d, &s);
	cache[key] = s;
	return s;
}

ID3D11RasterizerState *Render::Raster(const GpuState &st)
{
	static std::unordered_map<uint32_t, ID3D11RasterizerState *> cache;
	uint32_t key = st.cull | (st.wireframe << 4);
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	D3D11_RASTERIZER_DESC d = {};
	d.FillMode = st.wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	// DirectX 6: CCW culls counter-clockwise triangles (back faces when front faces are clockwise)
	d.CullMode = st.cull == D3DCULL_NONE ? D3D11_CULL_NONE : st.cull == D3DCULL_CW ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
	d.DepthClipEnable = TRUE;
	d.ScissorEnable = TRUE;
	ID3D11RasterizerState *s = nullptr;
	dev_->CreateRasterizerState(&d, &s);
	cache[key] = s;
	return s;
}

static D3D11_TEXTURE_ADDRESS_MODE MapAddr(int a)
{
	switch (a) {
	case D3DTADDRESS_MIRROR: return D3D11_TEXTURE_ADDRESS_MIRROR;
	case D3DTADDRESS_CLAMP: return D3D11_TEXTURE_ADDRESS_CLAMP;
	case D3DTADDRESS_BORDER: return D3D11_TEXTURE_ADDRESS_BORDER;
	default: return D3D11_TEXTURE_ADDRESS_WRAP;
	}
}

ID3D11SamplerState *Render::Sampler(int filter, int u, int v)
{
	static std::unordered_map<uint32_t, ID3D11SamplerState *> cache;
	uint32_t key = filter | (u << 8) | (v << 16);
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	D3D11_SAMPLER_DESC d = {};
	d.Filter = filter == D3DTFG_POINT ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	d.AddressU = MapAddr(u);
	d.AddressV = MapAddr(v);
	d.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	d.MaxLOD = D3D11_FLOAT32_MAX;
	d.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ID3D11SamplerState *s = nullptr;
	dev_->CreateSamplerState(&d, &s);
	cache[key] = s;
	return s;
}
