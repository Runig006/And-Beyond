// Bathyos camera (PATCH OPTIONS > bathyos camera): underwater, Buzz's submarine sits right in front of the follow
// camera. UpdateFollowCamera (00403ef0) picks the pitch it eases to (00508884: 0x180, 0x100 swimming or on the
// board, 0 while the look button is held) and then calls IsOnMovingMesh (00490600): that call goes through here, to
// raise the camera and aim it above Buzz in Bathyos. The aim is the "sub ecx, 0x3000" at 00404d42 (the view's
// vertical angle goes to 0x3000 above his feet, his head): rewritten each time for the level being played.
#include "common.h"
#include "config.h"

#define g_level (*(int *)A(0x0088994c))
#define g_cameraPitch (*(int *)A(0x00508884)) // more = looks down from higher; 0x1000 = a full turn

static const int kBathyosLevel = 10;   // internal g_level (the game's ninth)
static const int kHighPitch = 0x200;   // 45 degrees; 0x180 (34) in the other levels
static const DWORD kAimAt = A(0x00404d44); // the 0x3000 of "sub ecx, 0x3000" (81 e9 00 30 00 00 at 00404d42)
static const int kHeadAim = 0x3000;    // the game's: Buzz's head
static const int kHighAim = 0x5800;    // Bathyos: above him, so he goes down the screen and what is ahead shows

typedef int(__cdecl *IsOnMovingMeshFn)();
static const IsOnMovingMeshFn IsOnMovingMesh = (IsOnMovingMeshFn)A(0x00490600);
static bool g_aimPatched;

static int __cdecl IsOnMovingMesh_BathyosCamera()
{
	bool bathyos = g_config.bathyosCamera && g_level == kBathyosLevel;
	if (bathyos && g_cameraPitch > 0 && g_cameraPitch < kHighPitch) {
		g_cameraPitch = kHighPitch;
	}
	if (g_aimPatched) {
		*(int *)kAimAt = bathyos ? kHighAim : kHeadAim;
	}
	return IsOnMovingMesh();
}

extern "C" void InstallBathyosCamera(void)
{
	static const unsigned char expect[] = {0xe8, 0xa2, 0xc6, 0x08, 0x00}; // call IsOnMovingMesh
	unsigned char call[5] = {0xe8};
	*(DWORD *)(call + 1) = (DWORD)IsOnMovingMesh_BathyosCamera - (A(0x00403f59) + 5);
	if (!PatchBytes(A(0x00403f59), expect, call, 5, "bathyos camera (UpdateFollowCamera)")) {
		return;
	}
	// The aim's instruction stays writable: it changes with the level
	static const unsigned char aim[] = {0x81, 0xe9, 0x00, 0x30, 0x00, 0x00};
	DWORD old;
	if (memcmp((const void *)(kAimAt - 2), aim, sizeof aim) == 0 &&
	    VirtualProtect((void *)(kAimAt - 2), sizeof aim, PAGE_EXECUTE_READWRITE, &old)) {
		g_aimPatched = true;
		LogLine("ok bathyos camera aim (00404d42)");
	} else {
		LogLine("SKIPPED bathyos camera aim (00404d42): unexpected bytes");
	}
}
