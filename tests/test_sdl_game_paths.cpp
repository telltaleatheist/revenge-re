#include "revenge/game_file.h"
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

int main(int argc, char **argv)
{
    char install[1024];
    char path[1024];
    char buffer[16];
    char *current = SDL_GetCurrentDirectory();
    int16_t handle;

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
    WriteFile(path, "first");
    Path(path, sizeof(path), install, "Other.dat");
    WriteFile(path, "keep");
    assert(SDL_setenv_unsafe("REVENGE_SAVE_DIRECTORY", saves, 1) == 0);
    assert(CHANGE_DIRECTORY(install) == 0);

    /* Play opens no program file, in any case: asking for one stops the
	 * game with a fault, and a run that gets past it fails. */
    if (argc > 1 && strcmp(argv[1], "program_file") == 0)
    {
        (void) GameFile_Open("a:\\REVENGE.exe", GAME_FILE_READ);
        return 1;
    }

    /* A read finds the install's file in any case. */
    handle = GameFile_Open("MIXED.DAT", GAME_FILE_READ);
    assert(handle >= 0x0000);
    memset(buffer, 0, sizeof(buffer));
    assert(GameFile_Read(handle, buffer, 0x0005) == 0x0005);
    assert(strcmp(buffer, "first") == 0);
    assert(GameFile_Close(handle) == 0x0000);
    assert(GameFile_Open("ABSENT.DAT", GAME_FILE_READ) == -0x01);

    /* A write works on a copy in the save folder; the install keeps its own. */
    handle = GameFile_Open("a:\\mixed.dat", GAME_FILE_WRITE);
    assert(handle >= 0x0000);
    assert(GameFile_Write(handle, "X", 0x0001) == 0x0001);
    assert(GameFile_Close(handle) == 0x0000);
    Path(path, sizeof(path), install, "Mixed.Dat");
    assert(FileHolds(path, "first"));
    Path(path, sizeof(path), saves, "Mixed.Dat");
    assert(FileHolds(path, "Xirst"));

    /* Reads now see the save folder's copy. */
    handle = GameFile_Open("Mixed.dat", GAME_FILE_READ);
    assert(handle >= 0x0000);
    memset(buffer, 0, sizeof(buffer));
    assert(GameFile_Read(handle, buffer, 0x0005) == 0x0005);
    assert(strcmp(buffer, "Xirst") == 0);
    assert(GameFile_Close(handle) == 0x0000);

    /* A new file is made in the save folder only. */
    handle = GameFile_Open("NEW.DAT", GAME_FILE_CREATE);
    assert(handle >= 0x0000);
    assert(GameFile_Close(handle) == 0x0000);
    Path(path, sizeof(path), saves, "NEW.DAT");
    assert(SDL_GetPathInfo(path, NULL));
    Path(path, sizeof(path), install, "NEW.DAT");
    assert(!SDL_GetPathInfo(path, NULL));

    /* The install's files can't be deleted; the save folder's can. */
    assert(GameFile_Delete("OTHER.DAT") == -0x01);
    Path(path, sizeof(path), install, "Other.dat");
    assert(FileHolds(path, "keep"));
    assert(GameFile_Delete("NEW.DAT") == 0x0000);
    Path(path, sizeof(path), saves, "NEW.DAT");
    assert(!SDL_GetPathInfo(path, NULL));
    return 0;
}
