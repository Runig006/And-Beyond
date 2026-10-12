/* The running buzz.exe and its addresses (see gameaddr.h) */
#include <stdlib.h>
#include <string.h>
#include "gameaddr.h"
#include "game_maps.h"
#include "hook.h"

static const GameInfo *Running(void)
{
	static const GameInfo *running = NULL;
	static int looked = 0;
	if (!looked) {
		BYTE *base = (BYTE *)GetModuleHandleA(NULL);
		IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
		int i;
		for (i = 0; i < kGameCount; i++) {
			if (kGames[i].stamp == nt->FileHeader.TimeDateStamp) {
				running = &kGames[i];
			}
		}
		looked = 1;
	}
	return running;
}

GameVersion RunningGame(void)
{
	return Running() ? Running()->game : GAME_UNKNOWN;
}

const char *RunningGameName(void)
{
	return Running() ? Running()->name : "unknown";
}

/* The mapped address, 0 if there is none */
static DWORD Find(DWORD en)
{
	const GameInfo *g = Running();
	int lo, hi;
	if (!g) {
		return 0;
	}
	if (!g->map) {
		return en; /* the English buzz.exe */
	}
	lo = 0;
	hi = g->count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (g->map[mid].en == en) {
			return g->map[mid].here;
		}
		if (g->map[mid].en < en) {
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return 0;
}

DWORD A(DWORD en)
{
	DWORD here = Find(en);
	if (!here) {
		LogLine("ERROR no address in the %s buzz.exe for %08lx", RunningGameName(), en);
	}
	return here;
}

/* The map sorted by the running buzz.exe's addresses, for B() */
static AddrPair *g_byHere;

static int CompareHere(const void *a, const void *b)
{
	DWORD x = ((const AddrPair *)a)->here, y = ((const AddrPair *)b)->here;
	return x < y ? -1 : x > y;
}

DWORD B(DWORD here)
{
	const GameInfo *g = Running();
	int lo, hi, best = -1;
	if (!g || !g->map) {
		return here;
	}
	if (!g_byHere) {
		g_byHere = (AddrPair *)malloc(sizeof(AddrPair) * g->count);
		memcpy(g_byHere, g->map, sizeof(AddrPair) * g->count);
		qsort(g_byHere, g->count, sizeof(AddrPair), CompareHere);
	}
	/* the mapped place at or just before it: patches also name a few bytes into one ("call + 1") */
	lo = 0;
	hi = g->count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (g_byHere[mid].here <= here) {
			best = mid;
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	if (best < 0 || here - g_byHere[best].here >= 0x40) {
		return 0;
	}
	return g_byHere[best].en + (here - g_byHere[best].here);
}

/* The running buzz.exe's relocation table: where its absolute addresses are */
static DWORD *g_relocs;
static int g_relocCount;

static int CompareDword(const void *a, const void *b)
{
	DWORD x = *(const DWORD *)a, y = *(const DWORD *)b;
	return x < y ? -1 : x > y;
}

static void LoadRelocs(void)
{
	BYTE *base = (BYTE *)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
	IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	BYTE *p = base + dir.VirtualAddress, *end = p + dir.Size;
	int room = (int)dir.Size;
	g_relocs = (DWORD *)malloc(sizeof(DWORD) * (room > 0 ? room : 1));
	g_relocCount = 0;
	while (dir.Size && p + 8 <= end) {
		IMAGE_BASE_RELOCATION *block = (IMAGE_BASE_RELOCATION *)p;
		int k, n;
		if (block->SizeOfBlock < 8) {
			break;
		}
		n = (block->SizeOfBlock - 8) / 2;
		for (k = 0; k < n; k++) {
			WORD e = ((WORD *)(p + 8))[k];
			if (e >> 12 == IMAGE_REL_BASED_HIGHLOW) {
				g_relocs[g_relocCount++] = (DWORD)base + block->VirtualAddress + (e & 0xFFF);
			}
		}
		p += block->SizeOfBlock;
	}
	qsort(g_relocs, g_relocCount, sizeof(DWORD), CompareDword);
}

static int IsReloc(DWORD at)
{
	if (!g_relocs) {
		LoadRelocs();
	}
	return bsearch(&at, g_relocs, g_relocCount, sizeof(DWORD), CompareDword) != NULL;
}

int SameCode(DWORD en, DWORD at, const unsigned char *expect, int len)
{
	const unsigned char *here = (const unsigned char *)at;
	const GameInfo *g = Running();
	int i = 0;
	if (RunningGame() == GAME_EN) {
		return memcmp(here, expect, len) == 0;
	}
	for (i = 0; g && i < g->ownCount; i++) {
		if (g->own[i].en == en) {
			/* this version's own bytes there (its code is written differently) */
			return memcmp(here, g->own[i].bytes, g->own[i].len) == 0;
		}
	}
	i = 0;
	while (i < len) {
		int op = 0;  /* length of the opcode before a 4-byte call / jump target, 0 if none */
		if (i + 4 <= len && IsReloc(at + i)) {
			/* an absolute address: this buzz.exe's, or the English one translated */
			DWORD value = *(const DWORD *)(here + i), want = *(const DWORD *)(expect + i);
			if (value != want && Find(want) != value) {
				return 0;
			}
			i += 4;
			continue;
		}
		if (expect[i] == 0xE8 || expect[i] == 0xE9) {
			op = 1;
		} else if (expect[i] == 0x0F && i + 1 < len && expect[i + 1] >= 0x80 && expect[i + 1] <= 0x8F) {
			op = 2;
		}
		if (op && i + op + 4 <= len && memcmp(here + i, expect + i, op) == 0) {
			/* a call / jump: the same target written for this buzz.exe, or the English one translated */
			DWORD rel = *(const DWORD *)(here + i + op), want = *(const DWORD *)(expect + i + op);
			if (rel != want && (!en || Find(en + i + op + 4 + want) != at + i + op + 4 + rel)) {
				return 0;
			}
			i += op + 4;
			continue;
		}
		if (here[i] != expect[i]) {
			return 0;
		}
		i++;
	}
	return 1;
}
