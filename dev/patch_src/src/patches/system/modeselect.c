/* No resolution dialog: ShowModeDialog (00437270) is replaced by code that picks, in the driver list, the hardware
   device and the mode of PATCH OPTIONS > resolution (the desktop's by default), fullscreen.
     driver: +0x37c first device, +0x378 chosen device
     device: +0x138 hardware, +0x140 windowed, +0x144 chosen mode, +0x148 first mode, +0x14c next device
     mode:   DDSURFACEDESC2 copy (+8 height, +0xc width, +0x54 bits per pixel), +0xa4 next mode */
#include "hook.h"

void ConfigResolution(int *w, int *h); /* core/config.cpp */

#define FIELD(p, off) (*(int *)((char *)(p) + (off)))
#define g_driverList   (*(char **)A(0x0088ef70))
#define g_chosenDriver (*(char **)A(0x0088ef78))

static void __cdecl ShowModeDialog_Auto(void)
{
	char *drv = g_driverList, *dev, *mode, *best = 0;
	int dw = GetSystemMetrics(SM_CXSCREEN), dh = GetSystemMetrics(SM_CYSCREEN), bestScore = -1, tw, th;
	ConfigResolution(&tw, &th); /* PATCH OPTIONS > resolution, if it fits on the monitor */
	if (tw <= 0 || th <= 0 || tw > dw || th > dh) {
		tw = dw;
		th = dh;
	}
	if (!drv) {
		LogLine("ERROR: no display driver enumerated");
		return;
	}
	dev = (char *)FIELD(drv, 0x37c);
	for (char *d = dev; d; d = (char *)FIELD(d, 0x14c)) {
		if (FIELD(d, 0x138)) {
			dev = d;
			break;
		}
	}
	if (!dev) {
		LogLine("ERROR: the driver has no devices");
		return;
	}
	/* The chosen size first, then the largest one that fits; 32-bit before 16-bit */
	for (mode = (char *)FIELD(dev, 0x148); mode; mode = (char *)FIELD(mode, 0xa4)) {
		int w = FIELD(mode, 0xc), h = FIELD(mode, 8), bpp = FIELD(mode, 0x54), score;
		if (w > dw || h > dh) {
			continue;
		}
		score = (w == tw && h == th) * 1000000 + w * h / 16 + (bpp == 32) * 8;
		if (score > bestScore) {
			bestScore = score;
			best = mode;
		}
	}
	if (best) {
		FIELD(dev, 0x144) = (int)best;
	}
	FIELD(drv, 0x378) = (int)dev;
	FIELD(dev, 0x140) = 0; /* fullscreen (actually a borderless window: the engine decides) */
	g_chosenDriver = drv;
	if (best) {
		LogLine("mode chosen: %dx%d %d-bit (hardware %d)", FIELD(best, 0xc), FIELD(best, 8), FIELD(best, 0x54),
		        FIELD(dev, 0x138));
	}
}

void InstallModeSelect(void)
{
	static const unsigned char expect[] = {0x56, 0x8b, 0x35, 0x24, 0x86, 0x00, 0x01, 0x6a, 0x00, 0xff, 0xd6};
	HookReplace(A(0x00437270), ShowModeDialog_Auto, expect, sizeof expect, "no resolution dialog (ShowModeDialog)");
}
