#include "revenge/dos.h"
#include "game_paths.h"

#include <SDL3/SDL.h>
#include <assert.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#define CHANGE_DIRECTORY _chdir
#else
#include <unistd.h>
#define CHANGE_DIRECTORY chdir
#endif

/* The player's install is only read: reads find its files whatever their
 * case, writes go to a copy in the save folder, creates go there too, and
 * deleting an install file is refused. */

static char root[1024];
static char saves[1024];

static void Path(char *out, size_t capacity, const char *directory, const char *name)
{
	SDL_snprintf(out, capacity, "%s%s", directory, name);
}

static void WriteFile(const char *path, const char *text)
{
	SDL_IOStream *file = SDL_IOFromFile(path, "wb");

	assert(file != NULL);
	assert(SDL_WriteIO(file, text, strlen(text)) == strlen(text));
	SDL_CloseIO(file);
}

static int FileHolds(const char *path, const char *text)
{
	size_t length = 0;
	char *data = (char *) SDL_LoadFile(path, &length);
	int same = data != NULL && length == strlen(text) && memcmp(data, text, length) == 0;

	SDL_free(data);
	return same;
}

static void RemoveIfThere(const char *directory, const char *name)
{
	char path[1024];

	Path(path, sizeof(path), directory, name);
	(void) SDL_RemovePath(path);
}

int main(void)
{
	char install[1024];
	char path[1024];
	char buffer[16];
	char *current = SDL_GetCurrentDirectory();
	int16 handle;

	assert(current != NULL);
	SDL_snprintf(root, sizeof(root), "%sgame-paths-test/", current);
	SDL_free(current);
	SDL_snprintf(install, sizeof(install), "%sinstall/", root);
	SDL_snprintf(saves, sizeof(saves), "%ssaves/", root);
	RemoveIfThere(saves, "Mixed.Dat");
	RemoveIfThere(saves, "NEW.DAT");
	RemoveIfThere(install, "Mixed.Dat");
	RemoveIfThere(install, "Other.dat");
	assert(SDL_CreateDirectory(install));
	assert(SDL_CreateDirectory(saves));
	Path(path, sizeof(path), install, "Mixed.Dat");
	WriteFile(path, "install");
	Path(path, sizeof(path), install, "Other.dat");
	WriteFile(path, "keep");
	assert(SDL_setenv_unsafe("REVENGE_SAVE_DIRECTORY", saves, 1) == 0);
	assert(CHANGE_DIRECTORY(install) == 0);

	/* A read finds the install's file in any case. */
	handle = DOS_OpenFile("MIXED.DAT", 0x8000);
	assert(handle >= 0x0000);
	memset(buffer, 0, sizeof(buffer));
	assert(DOS_ReadFile((uint16) handle, buffer, 0x0007) == 0x0007);
	assert(strcmp(buffer, "install") == 0);
	assert(DOS_CloseFile(handle) == 0x0000);
	assert(DOS_OpenFile("ABSENT.DAT", 0x8000) < 0x0000);

	/* A write works on a copy in the save folder; the install keeps its own. */
	handle = DOS_OpenFile("a:\\mixed.dat", 0x8002);
	assert(handle >= 0x0000);
	assert(DOS_WriteFile((uint16) handle, "X", 0x0001) == 0x0001);
	assert(DOS_CloseFile(handle) == 0x0000);
	Path(path, sizeof(path), install, "Mixed.Dat");
	assert(FileHolds(path, "install"));
	Path(path, sizeof(path), saves, "Mixed.Dat");
	assert(FileHolds(path, "Xnstall"));

	/* Reads now see the save folder's copy. */
	handle = DOS_OpenFile("Mixed.dat", 0x8000);
	assert(handle >= 0x0000);
	memset(buffer, 0, sizeof(buffer));
	assert(DOS_ReadFile((uint16) handle, buffer, 0x0007) == 0x0007);
	assert(strcmp(buffer, "Xnstall") == 0);
	assert(DOS_CloseFile(handle) == 0x0000);

	/* A new file is made in the save folder only. */
	handle = DOS_CreateFile(0x0000, "NEW.DAT");
	assert(handle >= 0x0000);
	assert(DOS_CloseFile(handle) == 0x0000);
	Path(path, sizeof(path), saves, "NEW.DAT");
	assert(SDL_GetPathInfo(path, NULL));
	Path(path, sizeof(path), install, "NEW.DAT");
	assert(!SDL_GetPathInfo(path, NULL));

	/* The install's files can't be deleted; the save folder's can. */
	assert(DOS_DeleteFile("OTHER.DAT") < 0x0000);
	Path(path, sizeof(path), install, "Other.dat");
	assert(FileHolds(path, "keep"));
	assert(DOS_DeleteFile("NEW.DAT") == 0x0000);
	Path(path, sizeof(path), saves, "NEW.DAT");
	assert(!SDL_GetPathInfo(path, NULL));

	/* An install file reads as writable, whatever its mode on disk. */
	assert(DOS_GetSetFileAttributes("other.dat", DOS_FILE_ATTRIBUTES_GET, 0x0000) == 0x0000);
	return 0;
}
