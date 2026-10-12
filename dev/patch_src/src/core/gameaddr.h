/* The PC buzz.exe versions the engine knows, and their addresses. Every address in the engine's code is the English
   buzz.exe's; A() gives the one of the buzz.exe running. The other versions' maps (game_versions.h) are made from
   the executables of each language. */
#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GAME_UNKNOWN = -1,
	GAME_EN,
	GAME_IT,
	GAME_DE,
	GAME_FR,
	GAME_PT,
	GAME_ES,
} GameVersion;

/* The buzz.exe running, by the date in its PE header */
GameVersion RunningGame(void);
const char *RunningGameName(void);

/* The address in the running buzz.exe of the English address 'en'; 0 if this version has none (or is unknown) */
DWORD A(DWORD en);

/* The other way: the English address of a place of the running buzz.exe (a mapped place, or a few bytes into one) */
DWORD B(DWORD here);

/* For checking a patch: are 'len' bytes at the running address 'at' the bytes 'expect'? An absolute address or a
   call / jump target in them may be written for the running buzz.exe or be the English one at 'en' (translated). */
int SameCode(DWORD en, DWORD at, const unsigned char *expect, int len);

#ifdef __cplusplus
}
#endif
