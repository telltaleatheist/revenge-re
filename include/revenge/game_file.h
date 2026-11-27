#ifndef REVENGE_GAME_FILE_H
#define REVENGE_GAME_FILE_H

#include "revenge/types.h"

/*
 * The game's files. The original reached them through Borland's C library
 * (open, _read, _write, lseek, close, unlink: 3915-39C0) over DOS's INT 21h;
 * every file it opens is binary, so the library's text translation never
 * ran, and its error numbers (38C3) were never read. GameFile takes their
 * place: a file is read from the player's install, or the save folder's copy
 * when there is one, and written only in the save folder
 * (src/SDL/game_paths.h). Names are the game's DOS names.
 *
 * A handle is a small number, from 5 as DOS gave them, at most 20 open; the
 * game keeps them in its tracked-file table (DS:B632). Every failure is -1,
 * as the library returned it.
 */
typedef enum GameFileAccess
{
    GAME_FILE_READ,        /* open 8001h */
    GAME_FILE_WRITE,       /* open 8002h */
    GAME_FILE_READ_WRITE,  /* open 8004h */
    GAME_FILE_CREATE       /* open 8102h: write, the file made empty when
	                        * missing and left as it is when not */
} GameFileAccess;

typedef enum GameFileOrigin
{
    GAME_FILE_FROM_START,
    GAME_FILE_FROM_CURRENT,
    GAME_FILE_FROM_END
} GameFileOrigin;

int16_t GameFile_Open(const char *name, GameFileAccess access);
int16_t GameFile_Close(int16_t handle);

/* The bytes moved, as the 16-bit count DOS returned in AX: a count of 8000h
 * or more reads as negative, and callers treat only -1 as a failure.
 * 0 bytes asked for: 0, with nothing read or written. */
int16_t GameFile_Read(int16_t handle, void *destination, uint16_t byteCount);
int16_t GameFile_Write(int16_t handle, const void *source, uint16_t byteCount);

/* The new position. */
int32_t GameFile_Seek(int16_t handle, int32_t offset, GameFileOrigin origin);
/* The file's length, the position left where it was (39C0:0000). */
int32_t GameFile_Length(int16_t handle);

/* The save folder's copy; the install's own files are never deleted. */
int16_t GameFile_Delete(const char *name);

#endif
