#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/representation.h"
#include "original_import_for_tests.h"

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

/* A file the game can't open stops it with a fault naming the file. The
 * DOS game asked for another disk instead and tried again; here every file
 * is in the install or the save folder, so asking can't help.
 *
 *   read    a file in neither folder (the drive selection, 3390:007C-0091)
 *   make    a file the save folder can't hold (the retries at 3390:00BB-010A)
 */
int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "read";
	char *current = SDL_GetCurrentDirectory();
	char folder[1024];
	char saves[1024];
	char path[] = "ABSENT.DAT";

	assert(current != NULL);
	RequireOriginalImport();
	SDLBackend_InitializeDOSMemoryArena();
	/* The cached files' names, as the start sets them. */
	OriginalData_InitializeResourceDescriptorPointers();
	SDL_snprintf(folder, sizeof(folder), "%stracked-file-test/", current);
	SDL_free(current);
	assert(SDL_CreateDirectory(folder));
	if (strcmp(mode, "make") == 0)
	{
		/* A save folder under a plain file can't be made. */
		char blocker[1024];

		SDL_snprintf(blocker, sizeof(blocker), "%sblocker", folder);
		assert(SDL_SaveFile(blocker, "x", 1));
		SDL_snprintf(saves, sizeof(saves), "%s/saves/", blocker);
	}
	else
		SDL_snprintf(saves, sizeof(saves), "%ssaves/", folder);
	assert(SDL_setenv_unsafe("REVENGE_SAVE_DIRECTORY", saves, 1) == 0);
	assert(CHANGE_DIRECTORY(folder) == 0);

	(void) OpenTrackedFile(path, strcmp(mode, "make") == 0 ? 0x0003 : 0x0001);
	return 0;
}
