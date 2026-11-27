#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/media_settings.h"
#include "revenge/platform_backend.h"
#include "game_paths.h"
#include "settings.h"
#include "original_import_for_tests.h"
#include "REVENGE.h"
#include "revenge/original_data.h"

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

/* The player's settings file stands for MEDIA.CFG (src/SDL/settings.c).
 *
 *   first    the first start takes the install's MEDIA.CFG: the text the game
 *            then parses is that file's bytes, and settings.txt is written;
 *            the next start reads settings.txt alone, to the same choices
 *   none     with no MEDIA.CFG, our defaults
 *   own      a settings.txt of the player's own words wins
 *   bad      a choice it doesn't know is a fault naming the file and line
 *   parse    the game's own parse applies every line, the last (the input
 *            device) too, as 2C7F:055F tests its count before the decrement
 */
/* 2C7F's loader; only its own file declares it. */
word16 LoadMediaConfiguration(void);

static char saves[1024];
static char settingsPath[1024];

static void WriteText(const char *path, const char *text)
{
	assert(SDL_SaveFile(path, text, strlen(text)));
}

static void Folders(const char *name)
{
	char *current = SDL_GetCurrentDirectory();
	char empty[1024];

	assert(current != NULL);
	SDL_snprintf(saves, sizeof(saves), "%ssettings-test-%s/", current, name);
	SDL_snprintf(empty, sizeof(empty), "%ssettings-test-empty/", current);
	SDL_free(current);
	SDL_snprintf(settingsPath, sizeof(settingsPath), "%s%s", saves, HOST_SETTINGS_FILE_NAME);
	(void) SDL_RemovePath(settingsPath);
	assert(SDL_CreateDirectory(saves));
	assert(SDL_CreateDirectory(empty));
	assert(SDL_setenv_unsafe("REVENGE_SAVE_DIRECTORY", saves, 1) == 0);
	if (strcmp(name, "first") != 0)
		assert(CHANGE_DIRECTORY(empty) == 0);
	else
		assert(CHANGE_DIRECTORY(getenv("REVENGE_GAME")) == 0);
}

static void Text(char *text, word16 capacity, word16 *length)
{
	*length = MediaSettings_InstallerText(HostSettings_MediaChoices(), text, capacity);
	assert(*length > 0);
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "first";
	char text[0xC8];
	word16 length;

	RequireOriginalImport();
	SDLBackend_InitializeDOSMemoryArena();
	Folders(mode);

	if (strcmp(mode, "first") == 0)
	{
		char install[1024];
		size_t size = 0;
		char *original;
		char *written;

		assert(GamePaths_Resolve("MEDIA.CFG", GAME_PATH_READ, install, sizeof(install)) == 0);
		original = (char *) SDL_LoadFile(install, &size);
		assert(original != NULL);
		assert(HostSettings_Prepare());
		Text(text, sizeof(text), &length);
		assert(length == size && memcmp(text, original, size) == 0);
		written = (char *) SDL_LoadFile(settingsPath, NULL);
		assert(written != NULL && strstr(written, "\nvideo = vga\n") != NULL);
		SDL_free(written);
		assert(HostSettings_Prepare());
		Text(text, sizeof(text), &length);
		assert(length == size && memcmp(text, original, size) == 0);
		SDL_free(original);
	}
	else if (strcmp(mode, "none") == 0)
	{
		const RevengeMediaChoices *choices;

		assert(HostSettings_Prepare());
		choices = HostSettings_MediaChoices();
		assert(choices->choice[MEDIA_OPTION_VIDEO] == 0 && choices->choice[MEDIA_OPTION_MUSIC] == 1);
		assert(choices->choice[MEDIA_OPTION_SOUND] == 0 && choices->choice[MEDIA_OPTION_INPUT] == 1);
		assert(strcmp(choices->soundParameter, "220") == 0);
		assert(SDL_GetPathInfo(settingsPath, NULL));
	}
	else if (strcmp(mode, "own") == 0)
	{
		const RevengeMediaChoices *choices;

		WriteText(settingsPath, "# mine\nVIDEO = VGA\nmusic=adlib\n\nsound = ssi-2001 2C0   # the card\ninput = mouse\n");
		assert(HostSettings_Prepare());
		choices = HostSettings_MediaChoices();
		assert(choices->choice[MEDIA_OPTION_MUSIC] == 3 && choices->choice[MEDIA_OPTION_SOUND] == 2);
		assert(choices->choice[MEDIA_OPTION_INPUT] == 0 && strcmp(choices->soundParameter, "2C0") == 0);
	}
	else if (strcmp(mode, "parse") == 0)
	{
		WriteText(settingsPath, "video = vga\nmusic = pc-speaker\nsound = pc-speaker\ninput = mouse\n");
		assert(HostSettings_Prepare());
		assert(InputConfigurationRecords[0].value != 0x0000);
		InputDeviceType = 0x0000;
		assert(LoadMediaConfiguration() == TRUE);
		assert(InputDeviceType == InputConfigurationRecords[0].value);
	}
	else
	{
		WriteText(settingsPath, "video = vga\n\nmusic = loud\nsound = pc-speaker\ninput = keys\n");
		(void) HostSettings_Prepare();
	}
	return 0;
}
