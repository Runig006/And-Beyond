/* Patch helpers and the log (buzzpatch.log next to buzz.exe). See hook.h. */
#include <stdio.h>
#include <stdarg.h>
#include "hook.h"
#include "gameaddr.h"

char g_exeDir[MAX_PATH];

/* Only in a build made with "build.bat log" (BUZZ_LOG): the one handed out writes no log */
void LogLine(const char *fmt, ...)
{
#ifdef BUZZ_LOG
	char path[MAX_PATH];
	FILE *f;
	va_list ap;
	lstrcpyA(path, g_exeDir);
	lstrcatA(path, "buzzpatch.log");
	f = fopen(path, "a");
	if (!f) {
		return;
	}
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fputc('\n', f);
	fclose(f);
#else
	(void)fmt;
#endif
}

/* 'addr' is a place in the running buzz.exe (from A()); 'expect' may be the English buzz.exe's bytes */
static int Matches(DWORD addr, const unsigned char *expect, int len)
{
	return addr && SameCode(B(addr), addr, expect, len);
}

int PatchBytes(DWORD addr, const unsigned char *expect, const unsigned char *bytes, int len, const char *name)
{
	DWORD old;
	if (!addr || (expect && !Matches(addr, expect, len))) {
		LogLine("SKIPPED %s (%08lx): unexpected bytes", name, addr);
		return 0;
	}
	VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &old);
	memcpy((void *)addr, bytes, len);
	VirtualProtect((void *)addr, len, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void *)addr, len);
	LogLine("ok %s (%08lx)", name, addr);
	return 1;
}

void WriteCode(DWORD addr, const void *bytes, int len)
{
	DWORD old;
	if (!addr) {
		return;
	}
	VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &old);
	memcpy((void *)addr, bytes, len);
	VirtualProtect((void *)addr, len, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void *)addr, len);
}

void *HookTrampoline(DWORD addr, const void *replacement, const unsigned char *expect, int len, const char *name)
{
	/* The trampoline: a copy of the first 'len' bytes followed by a jump back to the rest of the function */
	unsigned char *tramp;
	DWORD back, at = addr;
	if (!Matches(addr, expect, len)) {
		LogLine("SKIPPED %s (%08lx): unexpected bytes", name, addr);
		return NULL;
	}
	tramp = (unsigned char *)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memcpy(tramp, (const void *)at, len);
	tramp[len] = 0xE9;
	back = (at + len) - ((DWORD)tramp + len + 5);
	memcpy(tramp + len + 1, &back, 4);
	if (!HookReplace(addr, replacement, expect, len, name)) {
		return NULL;
	}
	return tramp;
}

int HookReplace(DWORD addr, const void *replacement, const unsigned char *expect, int len, const char *name)
{
	unsigned char jmp[5];
	DWORD rel = (DWORD)replacement - (addr + 5);
	jmp[0] = 0xE9;
	memcpy(jmp + 1, &rel, 4);
	if (!Matches(addr, expect, len)) {
		LogLine("SKIPPED %s (%08lx): unexpected bytes", name, addr);
		return 0;
	}
	return PatchBytes(addr, NULL, jmp, 5, name);
}
