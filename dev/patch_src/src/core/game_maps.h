/* The PC buzz.exe versions and their address maps (game_maps.c, generated: do not edit) */
#pragma once
#include "gameaddr.h"

typedef struct {
	DWORD en;    /* the English buzz.exe's address */
	DWORD here;  /* the same place in this version */
} AddrPair;

/* A place whose code this version writes differently: the bytes a patch checks there instead of the English ones */
typedef struct {
	DWORD en;
	int len;
	unsigned char bytes[16];
} OwnBytes;

typedef struct {
	DWORD stamp;          /* TimeDateStamp of the PE header */
	GameVersion game;
	const char *name;
	const AddrPair *map;  /* sorted by en; NULL for the English buzz.exe (the addresses are its own) */
	int count;
	const OwnBytes *own;
	int ownCount;
} GameInfo;

extern const GameInfo kGames[];
extern const int kGameCount;
