/* Resolutions: IsDisplayModeAllowed (00435cb0) rejects modes wider than 1024 ("cmp eax, 0x400" at 00435cb9).
   Raised to 4096 (4K). The mode list itself comes from our DirectDraw (all common modes up to the desktop size). */
#include "hook.h"

void InstallResolution(void)
{
	static const unsigned char expect[] = {0x3d, 0x00, 0x04, 0x00, 0x00};
	static const unsigned char patch[] = {0x3d, 0x00, 0x10, 0x00, 0x00};
	PatchBytes(A(0x00435cb9), expect, patch, sizeof patch, "resolutions up to 4096 wide (IsDisplayModeAllowed)");
}
