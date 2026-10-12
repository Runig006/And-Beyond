/* Portable: CheckInstallRegistry (004ab360) read path, cdpath and allow32bit from HKLM\SOFTWARE\travellerstalesbuzz2k.
   Replaced: data\ and cd\ next to the .exe, 32-bit allowed. */
#include "hook.h"

#define g_installPath ((char *)A(0x0088de9c))   /* char[512] */
#define g_cdPath      ((char *)A(0x0088e0a0))   /* char[512] */
#define g_registryRead (*(int *)A(0x0088de98))  /* 1 = already read */
#define g_no32bit     (*(int *)A(0x0088ef80))   /* 0 when allow32bit is set */

static void __cdecl CheckInstallRegistry_Portable(void)
{
	lstrcpyA(g_installPath, g_exeDir);
	lstrcatA(g_installPath, "data\\");
	lstrcpyA(g_cdPath, g_exeDir);
	lstrcatA(g_cdPath, "cd\\");
	g_no32bit = 0;
	g_registryRead = 1;
}

void InstallPortable(void)
{
	static const unsigned char expect[] = {0x81, 0xec, 0x10, 0x04, 0x00, 0x00, 0x8d, 0x44, 0x24, 0x08};
	HookReplace(A(0x004ab360), CheckInstallRegistry_Portable, expect, sizeof expect, "portable (CheckInstallRegistry)");
}
