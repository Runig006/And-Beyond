// Shared definitions of the C++ files: Windows and DirectX 6 headers, COM helpers, matrices and pixel formats.
#pragma once
#define DIRECTDRAW_VERSION 0x0600
#define DIRECT3D_VERSION 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3d11_1.h>
#include <intrin.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "hook.h"
}

// Logs the first call to anything we have not implemented, with the game address that called it.
#define NOT_IMPL(name)                                                                   \
	do {                                                                                 \
		static bool once_;                                                               \
		if (!once_) {                                                                    \
			once_ = true;                                                                \
			LogLine("not implemented: %s (called from %p)", name, _ReturnAddress());     \
		}                                                                                \
	} while (0)

// COM reference counting shared by all our objects.
#define COM_REFCOUNT                                                                     \
	ULONG refs_ = 1;                                                                     \
	STDMETHOD_(ULONG, AddRef)() override { return ++refs_; }                             \
	STDMETHOD_(ULONG, Release)() override                                                \
	{                                                                                    \
		ULONG r = --refs_;                                                               \
		if (!r) {                                                                        \
			delete this;                                                                 \
		}                                                                                \
		return r;                                                                        \
	}

// Releases a COM object and clears the pointer
template <class T> inline void SafeRelease(T *&p)
{
	if (p) {
		p->Release();
		p = nullptr;
	}
}

struct Mat4 {
	float m[4][4];
};
Mat4 MatMul(const Mat4 &a, const Mat4 &b);
Mat4 MatIdentity();

// Pixel formats we advertise and can convert
enum PixFmt { PF_565, PF_1555, PF_4444, PF_8888, PF_X888, PF_P8, PF_Z16, PF_Z24S8 };
void MakePixelFormat(PixFmt f, DDPIXELFORMAT *pf);
PixFmt PixFmtFrom(const DDPIXELFORMAT &pf);
int PixFmtBytes(PixFmt f);

// Draw and frame counters for the heartbeat log (core/diag.cpp)
void DiagCountDraw(int vertices);
void DiagCountFrame();
