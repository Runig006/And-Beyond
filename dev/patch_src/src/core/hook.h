/* In-memory patches on buzz.exe (the file on disk is never modified).
   Every patch checks the original bytes before writing: if the .exe is not the expected version the patch is skipped
   (a build made with "build.bat log" writes the reason to buzzpatch.log). The addresses are the running buzz.exe's
   (A() in gameaddr.h); the bytes to check may be the English buzz.exe's: their addresses are translated. */
#pragma once
#include <windows.h>
#include "gameaddr.h"

/* Appends one line (printf format) to buzzpatch.log, next to buzz.exe */
void LogLine(const char *fmt, ...);

/* Replace the buzz.exe function at 'addr' with 'replacement' (5-byte jump).
   'expect' holds the bytes the function must start with ('len' of them). Returns 1 if applied. */
int HookReplace(DWORD addr, const void *replacement, const unsigned char *expect, int len, const char *name);

/* Like HookReplace, but keeps the original callable: returns a "trampoline" that runs the original function.
   The first 'len' bytes must be whole instructions that do not depend on their address (checked with a
   disassembler when writing the patch). */
void *HookTrampoline(DWORD addr, const void *replacement, const unsigned char *expect, int len, const char *name);

/* Write raw bytes into buzz.exe after checking the previous ones. */
int PatchBytes(DWORD addr, const unsigned char *expect, const unsigned char *bytes, int len, const char *name);

/* Rewrite bytes already checked by PatchBytes (options switched at run time), without checking or logging. */
void WriteCode(DWORD addr, const void *bytes, int len);

/* Folder of the .exe, with trailing backslash */
extern char g_exeDir[MAX_PATH];
