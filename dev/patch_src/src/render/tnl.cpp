// CPU transform and lighting (see tnl.h): matrices, vertex layouts, the DirectX 6 lighting model and vertex fog.
#include "tnl.h"
#include <math.h>

Mat4 MatIdentity()
{
	Mat4 r = {};
	for (int i = 0; i < 4; i++) {
		r.m[i][i] = 1;
	}
	return r;
}

Mat4 MatMul(const Mat4 &a, const Mat4 &b)
{
	Mat4 r;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
		}
	}
	return r;
}

FvfLayout DecodeFvf(DWORD fvf)
{
	FvfLayout l;
	int off = 0;
	switch (fvf & D3DFVF_POSITION_MASK) {
	case D3DFVF_XYZ: off = 12; break;
	case D3DFVF_XYZRHW: off = 16; l.rhw = true; break;
	default: off = 12 + 4 * (((fvf & D3DFVF_POSITION_MASK) - 4) / 2); break; // D3DFVF_XYZB1..B5
	}
	if (fvf & D3DFVF_NORMAL) {
		l.normal = off;
		off += 12;
	}
	if (fvf & D3DFVF_RESERVED1) {
		off += 4;
	}
	if (fvf & D3DFVF_DIFFUSE) {
		l.diffuse = off;
		off += 4;
	}
	if (fvf & D3DFVF_SPECULAR) {
		l.specular = off;
		off += 4;
	}
	l.texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	if (l.texCount) {
		l.tex = off;
		off += 8 * l.texCount;
	}
	l.stride = off;
	return l;
}

struct V3 {
	float x, y, z;
};
static V3 Sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 Norm(V3 a)
{
	float l = sqrtf(Dot(a, a));
	return l > 1e-12f ? V3{a.x / l, a.y / l, a.z / l} : a;
}
static V3 XformPoint(V3 p, const Mat4 &m)
{
	return {p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
	        p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
	        p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2]};
}
static V3 XformDir(V3 p, const Mat4 &m)
{
	return {p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0], p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1],
	        p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2]};
}

// Camera position in world space (inverse of the view matrix, which is rotation + translation)
static V3 CameraPos(const Mat4 &v)
{
	V3 t = {v.m[3][0], v.m[3][1], v.m[3][2]};
	return {-(t.x * v.m[0][0] + t.y * v.m[0][1] + t.z * v.m[0][2]), -(t.x * v.m[1][0] + t.y * v.m[1][1] + t.z * v.m[1][2]),
	        -(t.x * v.m[2][0] + t.y * v.m[2][1] + t.z * v.m[2][2])};
}

static float Clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static uint32_t PackColor(float r, float g, float b, float a)
{
	return ((uint32_t)(Clamp01(a) * 255 + .5f) << 24) | ((uint32_t)(Clamp01(r) * 255 + .5f) << 16) |
	       ((uint32_t)(Clamp01(g) * 255 + .5f) << 8) | (uint32_t)(Clamp01(b) * 255 + .5f);
}
static D3DCOLORVALUE Unpack(DWORD c)
{
	return {((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f, (c >> 24) / 255.f};
}

static float FogFactor(int mode, float z, float start, float end, float density)
{
	switch (mode) {
	case D3DFOG_EXP: return expf(-density * z);
	case D3DFOG_EXP2: return expf(-(density * z) * (density * z));
	case D3DFOG_LINEAR: return end != start ? (end - z) / (end - start) : 1.f;
	default: return 1.f;
	}
}

// Lights one vertex in world space (DirectX 6 model with D3DLIGHT2 and D3DMATERIAL)
static void Light(const TnlState &s, V3 pos, V3 n, const D3DCOLORVALUE *vtxDiffuse, V3 cam, uint32_t *outDiffuse,
                  uint32_t *outSpecular, float eyeZ)
{
	const D3DMATERIAL &m = s.material;
	D3DCOLORVALUE md = m.diffuse, ma = m.ambient;
	if (s.colorVertex && vtxDiffuse) {
		md = ma = *vtxDiffuse;
	}
	D3DCOLORVALUE amb = Unpack(s.ambient);
	float dr = m.emissive.r + amb.r * ma.r, dg = m.emissive.g + amb.g * ma.g, db = m.emissive.b + amb.b * ma.b;
	float sr = 0, sg = 0, sb = 0;
	V3 view = Norm(Sub(cam, pos));
	for (const LightState *ls : s.lights) {
		const D3DLIGHT2 &L = ls->light;
		if (!ls->active || !(L.dwFlags & D3DLIGHT_ACTIVE)) {
			continue;
		}
		V3 dir;
		float atten = 1, spot = 1;
		if (L.dltType == D3DLIGHT_DIRECTIONAL) {
			dir = Norm(V3{-L.dvDirection.x, -L.dvDirection.y, -L.dvDirection.z});
		} else {
			V3 d = Sub(V3{L.dvPosition.x, L.dvPosition.y, L.dvPosition.z}, pos);
			float dist = sqrtf(Dot(d, d));
			if (L.dvRange > 0 && dist > L.dvRange) {
				continue;
			}
			dir = dist > 0 ? V3{d.x / dist, d.y / dist, d.z / dist} : V3{0, 1, 0};
			float a = L.dvAttenuation0 + L.dvAttenuation1 * dist + L.dvAttenuation2 * dist * dist;
			atten = a > 0 ? 1.f / a : 1.f;
			if (L.dltType == D3DLIGHT_SPOT) {
				V3 sd = Norm(V3{L.dvDirection.x, L.dvDirection.y, L.dvDirection.z});
				float rho = -Dot(dir, sd), ct = cosf(L.dvTheta * .5f), cp = cosf(L.dvPhi * .5f);
				if (rho <= cp) {
					continue;
				}
				if (rho < ct) {
					spot = powf((rho - cp) / (ct - cp), L.dvFalloff > 0 ? L.dvFalloff : 1.f);
				}
			}
		}
		float ndl = Dot(n, dir);
		if (ndl <= 0) {
			continue;
		}
		float k = ndl * atten * spot;
		dr += L.dcvColor.r * md.r * k;
		dg += L.dcvColor.g * md.g * k;
		db += L.dcvColor.b * md.b * k;
		if (s.specular && m.power > 0 && !(L.dwFlags & D3DLIGHT_NO_SPECULAR)) {
			V3 h = Norm(V3{dir.x + view.x, dir.y + view.y, dir.z + view.z});
			float nh = Dot(n, h);
			if (nh > 0) {
				float p = powf(nh, m.power) * atten * spot;
				sr += L.dcvColor.r * m.specular.r * p;
				sg += L.dcvColor.g * m.specular.g * p;
				sb += L.dcvColor.b * m.specular.b * p;
			}
		}
	}
	*outDiffuse = PackColor(dr, dg, db, md.a);
	float fog = s.fogMode ? FogFactor(s.fogMode, eyeZ, s.fogStart, s.fogEnd, s.fogDensity) : 1.f;
	*outSpecular = PackColor(sr, sg, sb, fog);
}

static void ReadCommon(const BYTE *v, const FvfLayout &fl, GpuVertex *o)
{
	o->diffuse = fl.diffuse >= 0 ? *(const uint32_t *)(v + fl.diffuse) : 0xffffffff;
	o->specular = fl.specular >= 0 ? *(const uint32_t *)(v + fl.specular) : 0xff000000;
	o->u0 = o->v0 = o->u1 = o->v1 = 0;
	if (fl.texCount >= 1) {
		o->u0 = ((const float *)(v + fl.tex))[0];
		o->v0 = ((const float *)(v + fl.tex))[1];
		o->u1 = o->u0;
		o->v1 = o->v0;
	}
	if (fl.texCount >= 2) {
		o->u1 = ((const float *)(v + fl.tex))[2];
		o->v1 = ((const float *)(v + fl.tex))[3];
	}
}

// World-view-projection of an untransformed vertex; also returns its view-space depth
static void Project(const TnlState &s, const Mat4 &wvp, const Mat4 &wv, V3 p, float clip[4], float *eyeZ)
{
	clip[0] = p.x * wvp.m[0][0] + p.y * wvp.m[1][0] + p.z * wvp.m[2][0] + wvp.m[3][0];
	clip[1] = p.x * wvp.m[0][1] + p.y * wvp.m[1][1] + p.z * wvp.m[2][1] + wvp.m[3][1];
	clip[2] = p.x * wvp.m[0][2] + p.y * wvp.m[1][2] + p.z * wvp.m[2][2] + wvp.m[3][2];
	clip[3] = p.x * wvp.m[0][3] + p.y * wvp.m[1][3] + p.z * wvp.m[2][3] + wvp.m[3][3];
	*eyeZ = p.x * wv.m[0][2] + p.y * wv.m[1][2] + p.z * wv.m[2][2] + wv.m[3][2];
	// D3DVIEWPORT2 clip volume (dvClipX..) -> standard D3D11 one (-1..1)
	const D3DVIEWPORT2 &vp = s.viewport;
	float cw = vp.dvClipWidth != 0 ? vp.dvClipWidth : 2.f, ch = vp.dvClipHeight != 0 ? vp.dvClipHeight : 2.f;
	float cx = vp.dvClipWidth != 0 ? vp.dvClipX : -1.f, cy = vp.dvClipHeight != 0 ? vp.dvClipY : 1.f;
	float w = clip[3];
	clip[0] = 2 * (clip[0] - cx * w) / cw - w;
	clip[1] = 2 * (clip[1] - (cy - ch) * w) / ch - w;
}

void TransformToClip(const TnlState &s, const BYTE *src, const FvfLayout &fl, int count, bool light, GpuVertex *out)
{
	if (fl.rhw) {
		float iw = 2.f / s.rtWidth, ih = 2.f / s.rtHeight;
		for (int i = 0; i < count; i++, src += fl.stride) {
			const float *p = (const float *)src;
			GpuVertex &o = out[i];
			// Keep the sign: a vertex behind the camera has a negative rhw, and rebuilding clip space with the
			// real w lets the GPU clip the triangle instead of stretching it across the screen
			float w = p[3] != 0 ? 1.f / p[3] : 1.f;
			// DirectX 6 pixel centers are on integers, D3D11 ones at the middle: +0.5
			o.x = ((p[0] + .5f) * iw - 1) * w;
			o.y = (1 - (p[1] + .5f) * ih) * w;
			o.z = p[2] * w;
			o.w = w;
			o.fogz = w;
			ReadCommon(src, fl, &o);
		}
		return;
	}
	Mat4 wv = MatMul(s.world, s.view), wvp = MatMul(wv, s.proj);
	bool doLight = light && fl.normal >= 0;
	V3 cam = CameraPos(s.view);
	float px = s.viewport.dwWidth ? 2.f / s.viewport.dwWidth : 0, py = s.viewport.dwHeight ? 2.f / s.viewport.dwHeight : 0;
	for (int i = 0; i < count; i++, src += fl.stride) {
		const float *p = (const float *)src;
		GpuVertex &o = out[i];
		float clip[4], eyeZ;
		Project(s, wvp, wv, V3{p[0], p[1], p[2]}, clip, &eyeZ);
		// Same half-pixel offset as above, in clip space
		o.x = clip[0] + px * .5f * clip[3];
		o.y = clip[1] - py * .5f * clip[3];
		o.z = clip[2];
		o.w = clip[3];
		o.fogz = eyeZ;
		ReadCommon(src, fl, &o);
		if (doLight) {
			const float *n = (const float *)(src + fl.normal);
			V3 wp = XformPoint(V3{p[0], p[1], p[2]}, s.world), wn = Norm(XformDir(V3{n[0], n[1], n[2]}, s.world));
			D3DCOLORVALUE vd;
			if (fl.diffuse >= 0) {
				vd = Unpack(o.diffuse);
			}
			Light(s, wp, wn, fl.diffuse >= 0 ? &vd : nullptr, cam, &o.diffuse, &o.specular, eyeZ);
		} else if (s.fogMode) {
			// Pre-lit vertices (the game's world: D3DLVERTEX, D3DDP_DONOTLIGHT) still get vertex fog by distance,
			// as DirectX 6 did: the game only sets the fog states (SetFogStates, linear 24000-48000)
			float fog = FogFactor(s.fogMode, eyeZ, s.fogStart, s.fogEnd, s.fogDensity);
			fog = fog < 0 ? 0 : fog > 1 ? 1 : fog;
			o.specular = (o.specular & 0xffffff) | ((uint32_t)(fog * 255 + .5f) << 24);
		}
	}
}

void ProcessToScreen(const TnlState &s, const BYTE *src, const FvfLayout &sfl, int count, bool light, BYTE *dst,
                     const FvfLayout &dfl)
{
	Mat4 wv = MatMul(s.world, s.view), wvp = MatMul(wv, s.proj);
	bool doLight = light && sfl.normal >= 0;
	V3 cam = CameraPos(s.view);
	const D3DVIEWPORT2 &vp = s.viewport;
	for (int i = 0; i < count; i++, src += sfl.stride, dst += dfl.stride) {
		const float *p = (const float *)src;
		float clip[4], eyeZ;
		Project(s, wvp, wv, V3{p[0], p[1], p[2]}, clip, &eyeZ);
		float rhw = clip[3] != 0 ? 1.f / clip[3] : 1.f;
		float *d = (float *)dst;
		d[0] = vp.dwX + (clip[0] * rhw + 1) * .5f * vp.dwWidth;
		d[1] = vp.dwY + (1 - clip[1] * rhw) * .5f * vp.dwHeight;
		d[2] = vp.dvMinZ + clip[2] * rhw * (vp.dvMaxZ - vp.dvMinZ);
		if (dfl.rhw) {
			d[3] = rhw;
		}
		GpuVertex o;
		ReadCommon(src, sfl, &o);
		if (doLight) {
			const float *n = (const float *)(src + sfl.normal);
			V3 wp = XformPoint(V3{p[0], p[1], p[2]}, s.world), wn = Norm(XformDir(V3{n[0], n[1], n[2]}, s.world));
			D3DCOLORVALUE vd;
			if (sfl.diffuse >= 0) {
				vd = Unpack(o.diffuse);
			}
			Light(s, wp, wn, sfl.diffuse >= 0 ? &vd : nullptr, cam, &o.diffuse, &o.specular, eyeZ);
		} else if (s.fogMode) { // same as TransformToClip
			float fog = FogFactor(s.fogMode, eyeZ, s.fogStart, s.fogEnd, s.fogDensity);
			fog = fog < 0 ? 0 : fog > 1 ? 1 : fog;
			o.specular = (o.specular & 0xffffff) | ((uint32_t)(fog * 255 + .5f) << 24);
		}
		if (dfl.diffuse >= 0) {
			*(uint32_t *)(dst + dfl.diffuse) = o.diffuse;
		}
		if (dfl.specular >= 0) {
			*(uint32_t *)(dst + dfl.specular) = o.specular;
		}
		if (dfl.texCount && sfl.texCount) {
			memcpy(dst + dfl.tex, src + sfl.tex, 8 * (dfl.texCount < sfl.texCount ? dfl.texCount : sfl.texCount));
		}
	}
}
