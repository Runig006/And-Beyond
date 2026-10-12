// Diagnostics: the game's fatal errors, crashes (with the buzz.exe addresses on the stack) and slow seconds of play go
// to buzzpatch.log.
#include "objects.h"
#include <stdio.h>
#include <stdarg.h>

// FatalError (004ad700): formats the message, shows it with MessageBoxA only when g_fullscreenErrors (0088e2a4)
// is 0 and calls _exit(-1), so in fullscreen the game quit without a word. Replaced: the error is always logged and
// shown. SetErrorPlace stores the source file and line just before (00bb64f4 / 00bb64f8).
#define g_errorFile (*(const char **)A(0x00bb64f4))
#define g_errorLine (*(int *)A(0x00bb64f8))

static void __cdecl FatalError_Logged(const char *fmt, ...)
{
	char msg[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof msg - 1, fmt ? fmt : "(null)", ap);
	va_end(ap);
	msg[sizeof msg - 1] = 0;
	if (!msg[0]) {
		// The game quits through FatalError("") when its window is closed
		LogLine("game quit");
		ExitProcess(0);
	}
	LogLine("GAME FATAL ERROR at %s:%d (called from %p): %s", g_errorFile ? g_errorFile : "?", g_errorLine,
	        _ReturnAddress(), msg);
	ShowWindow(g_render.Window(), SW_MINIMIZE);
	MessageBoxA(nullptr, msg, "Buzz Lightyear - fatal error", MB_OK | MB_ICONERROR);
	ExitProcess((UINT)-1);
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS *ep)
{
	const EXCEPTION_RECORD *er = ep->ExceptionRecord;
	const CONTEXT *c = ep->ContextRecord;
	// Which module the crash is in, and the offset inside it
	HMODULE mod = nullptr;
	char modName[MAX_PATH] = "?";
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)er->ExceptionAddress, &mod)) {
		GetModuleFileNameA(mod, modName, MAX_PATH);
	}
	LogLine("CRASH in %s +%08lx", strrchr(modName, '\\') ? strrchr(modName, '\\') + 1 : modName,
	        (DWORD)er->ExceptionAddress - (DWORD)mod);
	LogLine("CRASH: exception %08lx at %p (data %p) eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx "
	        "ebp=%08lx esp=%08lx",
	        er->ExceptionCode, er->ExceptionAddress,
	        er->NumberParameters > 1 ? (void *)er->ExceptionInformation[1] : nullptr, c->Eax, c->Ebx, c->Ecx, c->Edx,
	        c->Esi, c->Edi, c->Ebp, c->Esp);
	// Likely return addresses: values on the stack that point into buzz.exe's code (its first section)
	BYTE *base = (BYTE *)GetModuleHandleA(nullptr);
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
	IMAGE_SECTION_HEADER *text = IMAGE_FIRST_SECTION(nt);
	DWORD codeStart = (DWORD)base + text->VirtualAddress, codeEnd = codeStart + text->Misc.VirtualSize;
	char line[512];
	int n = 0, found = 0;
	const DWORD *sp = (const DWORD *)c->Esp;
	for (int i = 0; i < 256 && found < 16; i++) {
		DWORD v;
		__try {
			v = sp[i];
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			break;
		}
		if (v >= codeStart && v < codeEnd) {
			n += sprintf(line + n, " %08lx", v);
			found++;
		}
	}
	line[n] = 0;
	LogLine("CRASH stack (buzz.exe addresses):%s", line);
	return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" void InstallDiagnostics(void)
{
	static const unsigned char expect[] = {0x8b, 0x44, 0x24, 0x04, 0x81, 0xec, 0x00, 0x04, 0x00, 0x00};
	HookReplace(A(0x004ad700), (const void *)FatalError_Logged, expect, sizeof expect, "log fatal errors (FatalError)");
	SetUnhandledExceptionFilter(CrashFilter);
}

// Heartbeat: frames presented and draw calls per second, logged only for a slow second of play (under 50 frames
// with over 100000 vertices drawn: menus and videos run slower on purpose)
static int g_frames, g_draws, g_vertices;
static DWORD g_lastBeat;
void DiagCountDraw(int vertices)
{
	g_draws++;
	g_vertices += vertices;
}
void DiagCountFrame()
{
	g_frames++;
	DWORD now = GetTickCount();
	if (!g_lastBeat) {
		g_lastBeat = now;
	}
	if (now - g_lastBeat >= 1000) {
		if (g_frames < 50 && g_vertices > 100000) {
			LogLine("heartbeat: %d frames, %d draws, %d vertices in %lu ms", g_frames, g_draws, g_vertices, now - g_lastBeat);
		}
		g_frames = g_draws = g_vertices = 0;
		g_lastBeat = now;
	}
}
