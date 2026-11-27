#ifndef REVENGE_SDL_GAME_PATHS_H
#define REVENGE_SDL_GAME_PATHS_H

#include <stddef.h>

/*
 * Where the game's files are. The player's install (the current directory,
 * once SDLHost_ConfigureAssetDirectory has run) is only read; everything the
 * game writes goes to our own folder: REVENGE_SAVE_DIRECTORY when it is set,
 * otherwise SDL's per-user preferences folder, BattleTech/Revenge Rebuild. Names are DOS names (8.3,
 * any case, '\' or '/', an optional drive letter) and are found in either
 * place whatever their case on disk.
 */
typedef enum GamePathUse
{
	GAME_PATH_READ,     /* the save folder's copy, else the install's */
	GAME_PATH_WRITE,    /* the save folder's copy, made from the install's first */
	GAME_PATH_CREATE    /* always in the save folder */
} GamePathUse;

/* Puts the host path for dosPath in out. Returns 0, or a negative errno:
 * -ENOENT when a read or write finds no file (out still names where it was
 * looked for), -ENAMETOOLONG, -EIO when the copy or the folder fails. */
int GamePaths_Resolve(const char *dosPath, GamePathUse use, char *out, size_t capacity);

/* Deletes dosPath from the save folder. -EACCES when the file is only in the
 * install, which is never changed. */
int GamePaths_Delete(const char *dosPath);

/* 1 when the file GAME_PATH_READ resolves to is the install's. */
int GamePaths_IsInstallFile(const char *dosPath);

/* The install's REVENGE.EXE, in whatever case it is spelled on disk. */
const char *GamePaths_Executable(void);

/* The save folder, ending in a separator; NULL when none can be made. */
const char *GamePaths_WriteDirectory(void);

/* Our own data (data/ in the repository: the rules, revenge/rules.h), beside
 * the program, or the folder REVENGE_DATA_DIRECTORY names. NULL when there is
 * none to name. */
const char *GamePaths_DataDirectory(void);

#endif
