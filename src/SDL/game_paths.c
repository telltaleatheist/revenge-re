/* The player's install read-only, our own folder for what the game writes
 * (game_paths.h). */
#include "game_paths.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

enum
{
	GAME_PATH_CAPACITY = 1024
};

static const char ApplicationName[] = "Crescent Hawks Revenge";
static char writeDirectory[GAME_PATH_CAPACITY];
static int writeDirectoryState;     /* 0 not yet, 1 ready, -1 unavailable */

const char *GamePaths_WriteDirectory(void)
{
	if (writeDirectoryState == 0)
	{
		const char *chosen = SDL_getenv("REVENGE_SAVE_DIRECTORY");
		char *preferences = NULL;
		size_t length;

		if (chosen == NULL || chosen[0] == '\0')
			chosen = preferences = SDL_GetPrefPath("", ApplicationName);
		writeDirectoryState = -1;
		length = chosen != NULL ? strlen(chosen) : 0;
		if (length > 0 && length + 2 < sizeof(writeDirectory))
		{
			memcpy(writeDirectory, chosen, length + 1);
			if (writeDirectory[length - 1] != '/' && writeDirectory[length - 1] != '\\')
				memcpy(writeDirectory + length, "/", 2);
			if (SDL_CreateDirectory(writeDirectory))
				writeDirectoryState = 1;
		}
		SDL_free(preferences);
	}
	return writeDirectoryState == 1 ? writeDirectory : NULL;
}

/* The DOS name as a relative host path: '/' separators, no drive, no "./". */
static int NormaliseName(const char *dosPath, char *out, size_t capacity)
{
	size_t length;
	size_t index;

	if (dosPath[0] != '\0' && dosPath[1] == ':')
		dosPath += 2;
	while (dosPath[0] == '\\' || dosPath[0] == '/' || (dosPath[0] == '.' && (dosPath[1] == '/' || dosPath[1] == '\\')))
		dosPath += dosPath[0] == '.' ? 2 : 1;
	length = strlen(dosPath);
	if (length == 0)
		return -ENOENT;
	if (length >= capacity)
		return -ENAMETOOLONG;
	for (index = 0; index <= length; ++index)
		out[index] = dosPath[index] == '\\' ? '/' : dosPath[index];
	return 0;
}

static int Join(char *out, size_t capacity, const char *directory, const char *name)
{
	size_t first = strlen(directory);
	size_t second = strlen(name);

	if (first + second + 1 > capacity)
		return -ENAMETOOLONG;
	memcpy(out, directory, first);
	memcpy(out + first, name, second + 1);
	return 0;
}

typedef struct EntrySearch
{
	const char *wanted;
	char found[256];
	int matched;
} EntrySearch;

static SDL_EnumerationResult SDLCALL MatchEntry(void *userdata, const char *directory, const char *name)
{
	EntrySearch *search = (EntrySearch *) userdata;

	(void) directory;
	if (SDL_strcasecmp(name, search->wanted) == 0 && strlen(name) < sizeof(search->found))
	{
		memcpy(search->found, name, strlen(name) + 1);
		search->matched = 1;
		return SDL_ENUM_SUCCESS;
	}
	return SDL_ENUM_CONTINUE;
}

/* Finds root + relative whatever the case of each part on disk. On a match
 * out is the path as the disk spells it and 1 is returned; otherwise out is
 * root + relative as given and 0 is returned. */
static int FindAnyCase(const char *root, const char *relative, char *out, size_t capacity)
{
	char part[256];
	const char *cursor = relative;
	size_t at;

	if (Join(out, capacity, root, relative) != 0)
		return 0;
	if (SDL_GetPathInfo(out, NULL))
		return 1;
	if (Join(out, capacity, root, "") != 0)
		return 0;
	at = strlen(out);
	while (*cursor != '\0')
	{
		const char *end = strchr(cursor, '/');
		size_t length = end != NULL ? (size_t) (end - cursor) : strlen(cursor);
		EntrySearch search;

		if (length == 0 || length >= sizeof(part))
			break;
		memcpy(part, cursor, length);
		part[length] = '\0';
		search.wanted = part;
		search.matched = 0;
		SDL_EnumerateDirectory(at == 0 ? "." : out, MatchEntry, &search);
		if (!search.matched || at + strlen(search.found) + 2 > capacity)
			break;
		memcpy(out + at, search.found, strlen(search.found) + 1);
		at += strlen(search.found);
		cursor += length;
		if (*cursor == '/')
		{
			memcpy(out + at, "/", 2);
			++at;
			++cursor;
		}
		if (*cursor == '\0')
			return SDL_GetPathInfo(out, NULL);
	}
	(void) Join(out, capacity, root, relative);
	return 0;
}

/* Makes the folders a save-folder path needs. */
static int MakeParents(const char *path)
{
	char parent[GAME_PATH_CAPACITY];
	char *slash;

	if (strlen(path) >= sizeof(parent))
		return -ENAMETOOLONG;
	memcpy(parent, path, strlen(path) + 1);
	slash = strrchr(parent, '/');
	if (slash == NULL)
		return 0;
	*slash = '\0';
	return SDL_CreateDirectory(parent) ? 0 : -EIO;
}

int GamePaths_Resolve(const char *dosPath, GamePathUse use, char *out, size_t capacity)
{
	char name[GAME_PATH_CAPACITY];
	char install[GAME_PATH_CAPACITY];
	const char *saves = GamePaths_WriteDirectory();
	int result;

	if (dosPath == NULL || capacity == 0)
		return -EINVAL;
	result = NormaliseName(dosPath, name, sizeof(name));
	if (result != 0)
		return result;
	if (saves == NULL)
	{
		/* No folder of our own: reads still work, writes don't. */
		if (use != GAME_PATH_READ)
			return -EACCES;
		return FindAnyCase("", name, out, capacity) ? 0 : -ENOENT;
	}
	if (FindAnyCase(saves, name, out, capacity))
		return 0;
	if (use == GAME_PATH_CREATE)
		return MakeParents(out);
	if (!FindAnyCase("", name, install, sizeof(install)))
	{
		(void) Join(out, capacity, "", name);
		return -ENOENT;
	}
	if (use == GAME_PATH_READ)
		return Join(out, capacity, "", install);
	/* The first write of an install file works on a copy in the save folder;
	 * the install's file is never changed. */
	result = Join(out, capacity, saves, install);
	if (result == 0)
		result = MakeParents(out);
	if (result == 0 && !SDL_CopyFile(install, out))
		result = -EIO;
	return result;
}

int GamePaths_Delete(const char *dosPath)
{
	char path[GAME_PATH_CAPACITY];
	const char *saves = GamePaths_WriteDirectory();
	char name[GAME_PATH_CAPACITY];
	int result;

	if (dosPath == NULL)
		return -EINVAL;
	result = NormaliseName(dosPath, name, sizeof(name));
	if (result != 0)
		return result;
	if (saves != NULL && FindAnyCase(saves, name, path, sizeof(path)))
		return SDL_RemovePath(path) ? 0 : -EIO;
	return FindAnyCase("", name, path, sizeof(path)) ? -EACCES : -ENOENT;
}

int GamePaths_IsInstallFile(const char *dosPath)
{
	char path[GAME_PATH_CAPACITY];
	const char *saves = GamePaths_WriteDirectory();

	if (GamePaths_Resolve(dosPath, GAME_PATH_READ, path, sizeof(path)) != 0)
		return 0;
	return saves == NULL || strncmp(path, saves, strlen(saves)) != 0;
}

const char *GamePaths_Executable(void)
{
	static char path[GAME_PATH_CAPACITY];

	if (!FindAnyCase("", "REVENGE.EXE", path, sizeof(path)))
		(void) Join(path, sizeof(path), "", "REVENGE.EXE");
	return path;
}
