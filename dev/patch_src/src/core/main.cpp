// buzzpatch: our DDraw.dll for Buzz Lightyear of Star Command (PC).
// The game imports DDRAW.DLL (DirectDrawCreate and DirectDrawEnumerateA) and Windows looks in the game folder first,
// so this DLL is loaded. On startup it applies the in-memory patches (the .exe on disk stays untouched) and then acts
// as DirectDraw / Direct3D for the game, rendering with Direct3D 11.
#include "objects.h"
#include "config.h"
#include "language.h"

extern "C" void InstallPortable(void);
extern "C" void InstallResolution(void);
extern "C" void InstallModeSelect(void);
extern "C" void InstallDiagnostics(void);
extern "C" void InstallWidescreen(void);
extern "C" void InstallGamepad(void);
extern "C" void InstallFrameRate(void);
extern "C" void InstallControls(void);
extern "C" void InstallBosses(void);
extern "C" void InstallMenu(void);
extern "C" void InstallControlsMenu(void);
extern "C" void InstallDrawDistance(void);
extern "C" void InstallEnemies(void);
extern "C" void InstallEffects(void);
extern "C" void InstallAutosave(void);
extern "C" void InstallSeeThrough(void);
extern "C" void InstallBathyosCamera(void);
extern "C" void InstallDifficulty(void);
extern "C" void InstallConfig(void);

extern "C" HRESULT WINAPI BuzzDirectDrawCreate(GUID *guid, LPDIRECTDRAW *dd, IUnknown *outer)
{
	DDraw *obj = new DDraw();
	*dd = new DDraw1(obj);
	obj->Release(); // the DDraw1 wrapper holds the reference
	return DD_OK;
}

extern "C" HRESULT WINAPI BuzzDirectDrawEnumerateA(LPDDENUMCALLBACKA cb, LPVOID ctx)
{
	cb(nullptr, (LPSTR) "Direct3D 11 (buzzpatch)", (LPSTR) "display", ctx);
	return DD_OK;
}

static void Install(void (*install)(void), const char *name)
{
	__try {
		install();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		LogLine("ERROR %s stopped half-way (exception %08lx)", name, GetExceptionCode());
	}
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(inst);
		GetModuleFileNameA(nullptr, g_exeDir, MAX_PATH);
		char *slash = strrchr(g_exeDir, '\\');
		if (slash) {
			slash[1] = 0;
		}
		LogLine("--- buzzpatch loaded ---");
		// The patches need the addresses of this buzz.exe (gameaddr.h): with one the engine does not know, only the
		// renderer runs
		if (RunningGame() == GAME_UNKNOWN) {
			LogLine("buzz.exe is not a PC version the engine knows: patches not applied");
			return TRUE;
		}
		LogLine("buzz.exe: %s", RunningGameName());
		ConfigLoad();
		// Each group of patches on its own: one that fails (a place this buzz.exe has not) does not stop the game
		Install(LanguageInit, "LanguageInit"); // before the patches that keep pointers to texts
		Install(InstallPortable, "InstallPortable");
		Install(InstallResolution, "InstallResolution");
		Install(InstallModeSelect, "InstallModeSelect");
		Install(InstallDiagnostics, "InstallDiagnostics");
		Install(InstallWidescreen, "InstallWidescreen");
		Install(InstallGamepad, "InstallGamepad");
		Install(InstallFrameRate, "InstallFrameRate");
		Install(InstallControls, "InstallControls");
		Install(InstallBosses, "InstallBosses");
		Install(InstallMenu, "InstallMenu");
		Install(InstallControlsMenu, "InstallControlsMenu");
		Install(InstallDrawDistance, "InstallDrawDistance");
		Install(InstallEnemies, "InstallEnemies");
		Install(InstallEffects, "InstallEffects");
		Install(InstallAutosave, "InstallAutosave");
		Install(InstallSeeThrough, "InstallSeeThrough");
		Install(InstallBathyosCamera, "InstallBathyosCamera");
		Install(InstallDifficulty, "InstallDifficulty");
		Install(InstallConfig, "InstallConfig");
#ifdef BUZZ_LOG
		// Self-test (log build only): every patch applied and logged, then out before the game opens its window
		if (GetEnvironmentVariableA("BUZZPATCH_SELFTEST", nullptr, 0)) {
			LogLine("selftest done");
			TerminateProcess(GetCurrentProcess(), 0);
		}
#endif
	} else if (reason == DLL_PROCESS_DETACH) {
		LogLine("process exit");
	}
	return TRUE;
}
