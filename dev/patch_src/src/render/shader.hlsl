// The engine's shaders, built into the DLL (buzzpatch.rc) and compiled at startup by render.cpp.
//   vs: passes the vertices through (tnl.cpp already transformed and lit them on the CPU).
//   ps: DirectX 6 fixed function per pixel: texture stages, specular, alpha test, fog, color key, see-through.
//   blitvs / blitps: image copies (2D blits and the final present).

// ---------------------------------------------------------------- settings sent by render.cpp for each draw
// Same order as struct PsConsts in render.h
cbuffer PS : register(b0) {
	int4 stage0;    // texture stage 0 color: x = operation (D3DTOP_*), y = argument 1, z = argument 2
	int4 alpha0;    // texture stage 0 alpha: same layout
	int4 stage1;    // stage 1 (second texture), rarely used
	int4 alpha1;
	float4 tfactor; // constant color (D3DRENDERSTATE_TEXTUREFACTOR)
	float4 fogColor;
	float4 fogParams; // start, end, density, mode (0 per vertex, 1 exp, 2 exp2, 3 linear)
	int4 flags;     // x fog on, y alpha test function (0 = off), z add specular, w color-keyed texture
	float4 alphaRef;// x = alpha test reference (0..1)
	int4 texOn;     // x stage 0 has a texture, y stage 1 has a texture
	float4 seeBuzz;  // see-through: Buzz's centre in view space, radius of the tube
	float4 seeProj;  // projection [0][0], [1][1], [2][2], [3][2]
	float4 seeProj2; // projection [2][0], [2][1]; margin; w = 1 on for this draw
	float4 seeVp;    // viewport x, y, width, height
};

Texture2D t0 : register(t0);
Texture2D t1 : register(t1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

// Data of one vertex (and, interpolated, of one pixel)
struct V {
	float4 pos : SV_Position;
	float4 dif : COLOR0;   // diffuse color (lit or from the vertex)
	float4 spec : COLOR1;  // specular color; its alpha is the per-vertex fog amount
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
	float fogz : TEXCOORD2; // depth used by table fog
};

V vs(float4 pos : POSITION, float4 dif : COLOR0, float4 spec : COLOR1, float2 uv0 : TEXCOORD0, float2 uv1 : TEXCOORD1,
     float fogz : TEXCOORD2)
{
	V o;
	o.pos = pos;
	o.dif = dif;
	o.spec = spec;
	o.uv0 = uv0;
	o.uv1 = uv1;
	o.fogz = fogz;
	return o;
}

// ---------------------------------------------------------------- DirectX 6 texture stages
// An argument picks a color source (D3DTA_*): diffuse, current result, texture, constant or specular
float4 Arg(int a, float4 cur, float4 tex, float4 dif, float4 spec)
{
	float4 r;
	switch (a & 15) {
	case 0: r = dif; break;   // D3DTA_DIFFUSE
	case 1: r = cur; break;   // D3DTA_CURRENT
	case 2: r = tex; break;   // D3DTA_TEXTURE
	case 3: r = tfactor; break; // D3DTA_TFACTOR
	default: r = spec; break; // D3DTA_SPECULAR
	}
	if (a & 0x10) {
		r = 1 - r;
	}            // D3DTA_COMPLEMENT
	if (a & 0x20) {
		r = r.aaaa;
	}           // D3DTA_ALPHAREPLICATE
	return r;
}

// An operation combines the two arguments (D3DTOP_*)
float4 Op(int op, float4 a1, float4 a2, float4 cur, float4 tex, float4 dif)
{
	switch (op) {
	case 2: return a1;                                  // SELECTARG1
	case 3: return a2;                                  // SELECTARG2
	case 4: return a1 * a2;                             // MODULATE
	case 5: return saturate(a1 * a2 * 2);               // MODULATE2X
	case 6: return saturate(a1 * a2 * 4);               // MODULATE4X
	case 7: return saturate(a1 + a2);                   // ADD
	case 8: return saturate(a1 + a2 - 0.5);             // ADDSIGNED
	case 9: return saturate((a1 + a2 - 0.5) * 2);       // ADDSIGNED2X
	case 10: return saturate(a1 - a2);                  // SUBTRACT
	case 11: return saturate(a1 + a2 - a1 * a2);        // ADDSMOOTH
	case 12: return lerp(a2, a1, dif.a);                // BLENDDIFFUSEALPHA
	case 13: return lerp(a2, a1, tex.a);                // BLENDTEXTUREALPHA
	case 14: return lerp(a2, a1, tfactor.a);            // BLENDFACTORALPHA
	case 15: return saturate(a1 + a2 * (1 - tex.a));    // BLENDTEXTUREALPHAPM
	case 16: return lerp(a2, a1, cur.a);                // BLENDCURRENTALPHA
	case 18: return saturate(float4(a1.rgb + a1.a * a2.rgb, a1.a)); // MODULATEALPHA_ADDCOLOR
	case 19: return saturate(float4(a1.rgb * a2.rgb + a1.a, a1.a)); // MODULATECOLOR_ADDALPHA
	default: return a1 * a2;
	}
}

// One stage: color and alpha are computed separately
float4 Stage(int4 c, int4 a, float4 cur, float4 tex, float4 dif, float4 spec)
{
	float4 rc = Op(c.x, Arg(c.y, cur, tex, dif, spec), Arg(c.z, cur, tex, dif, spec), cur, tex, dif);
	float4 ra = a.x == 1 ? cur : Op(a.x, Arg(a.y, cur, tex, dif, spec), Arg(a.z, cur, tex, dif, spec), cur, tex, dif);
	return float4(rc.rgb, ra.a);
}

// Alpha test comparison (D3DCMP_*: 1 never ... 8 always)
bool Cmp(int f, float a, float r)
{
	switch (f) {
	case 1: return false;
	case 2: return a < r;
	case 3: return a == r;
	case 4: return a <= r;
	case 5: return a > r;
	case 6: return a != r;
	case 7: return a >= r;
	default: return true;
	}
}

// ---------------------------------------------------------------- see-through around Buzz
// A pixel back in view space is dropped (dither pattern) when it lies inside the tube from the camera to Buzz and at
// least the margin nearer than him, so Buzz and his ground stay whole. Denser in the middle, fading at the edge.
static const float kBayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

bool SeeThroughHole(float4 pos)
{
	float d = seeProj.w / (pos.z - seeProj.z); // view depth from the depth value: z = [2][2] + [3][2] / depth
	if (d <= 0) {
		return false;
	}
	float nx = (pos.x - seeVp.x) / seeVp.z * 2 - 1, ny = 1 - (pos.y - seeVp.y) / seeVp.w * 2;
	float3 p = float3((nx - seeProj2.x) * d / seeProj.x, (ny - seeProj2.y) * d / seeProj.y, d);
	float3 b = seeBuzz.xyz;
	float len = length(b);
	float along = dot(p, b) / len; // distance from the camera along the line to Buzz
	if (along <= 0 || along > len - seeProj2.z) {
		return false;
	}
	float off = length(p - b * (along / len));
	float amount = saturate(1.5 - 1.5 * off / seeBuzz.w) * 0.7; // up to 70 % of the pixels
	uint2 q = uint2(pos.xy) & 3;
	return kBayer[q.y * 4 + q.x] < amount * 16;
}

// ---------------------------------------------------------------- color of one pixel
float4 ps(V i) : SV_Target
{
	if (seeProj2.w > 0 && SeeThroughHole(i.pos)) {
		discard;
	}

	// 1. Texture (white when the draw has none, so "texture x color" gives the color)
	float4 tex0 = texOn.x ? t0.Sample(s0, i.uv0) : float4(1, 1, 1, 1);
	if (flags.w && tex0.a < 0.5) {
		discard; // color key: transparent texel
	}

	// 2. Texture stages
	float4 c = i.dif;
	if (stage0.x != 1) { // 1 = D3DTOP_DISABLE
		c = Stage(stage0, alpha0, c, tex0, i.dif, i.spec);
		if (stage1.x != 1) {
			float4 tex1 = texOn.y ? t1.Sample(s1, i.uv1) : float4(1, 1, 1, 1);
			c = Stage(stage1, alpha1, c, tex1, i.dif, i.spec);
		}
	}

	// 3. Specular highlight
	if (flags.z) {
		c.rgb = saturate(c.rgb + i.spec.rgb);
	}

	// 4. Alpha test: drop the pixel when its alpha fails the comparison
	if (flags.y && !Cmp(flags.y, c.a, alphaRef.x)) {
		discard;
	}

	// 5. Fog: blend towards the fog color with distance
	if (flags.x) {
		float f;
		float z = i.fogz;
		if (fogParams.w == 1) {
			f = exp(-fogParams.z * z);
		} else if (fogParams.w == 2) {
			f = exp(-(fogParams.z * z) * (fogParams.z * z));
		} else if (fogParams.w == 3) {
			f = (fogParams.y - z) / max(fogParams.y - fogParams.x, 1e-6);
		} else {
			f = i.spec.a; // per-vertex fog computed by the CPU
		}
		c.rgb = lerp(fogColor.rgb, c.rgb, saturate(f));
	}
	return c;
}

// ---------------------------------------------------------------- image copy (2D blits and present)
Texture2D bt : register(t0);
SamplerState bs : register(s0);

struct B {
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

B blitvs(float4 pos : POSITION, float2 uv : TEXCOORD0)
{
	B o;
	o.pos = pos;
	o.uv = uv;
	return o;
}

float4 blitps(B i) : SV_Target
{
	float4 c = bt.Sample(bs, i.uv);
	if (flags.w && c.a < 0.5) {
		discard; // color key
	}
	return c;
}
