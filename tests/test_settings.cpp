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

/* The player's settings file stands for MEDIA.CFG (src/SDL/settings.cpp).
 *
 *   first    the first start takes the install's MEDIA.CFG: the text the game
 *            then parses is that file's bytes but the video's, which is
 *            always the MCGA record (the 256-colour mode on every screen),
 *            and the input's, which starts as the mouse; settings.txt is
 *            written; the next start reads settings.txt alone, to the same
 *            choices
 *   none     with no MEDIA.CFG, our defaults (the mouse among them)
 *   own      a settings.txt of the player's own words wins; an old card
 *            word for the music reads as the soundfont, for the sound (its
 *            parameter passed over) as the samples, any video record's
 *            word as vga, and the file is written again in the new words
 *   words    the music's words (off, speaker) and the sound's (speaker,
 *            samples), each its record; the file's input word (keys) kept
 *   bad      a choice it doesn't know is a fault naming the file and line
 *   extra    so is a parameter after one of the new sound words
 *   parse    the game's own parse applies every line, the last (the input
 *            device) too, as 2C7F:055F tests its count before the decrement
 */
/* 2C7F's loader; only its own file declares it. */
uint16_t LoadMediaConfiguration(void);

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

/* The value on the line of the text that starts "NAME=", replaced. */
static char *ReplaceValue(char *text, size_t *size, const char *name, const char *value)
{
    char key[0x20];
    char *line = text;
    char *valueStart;
    char *valueEnd;
    char *replaced;
    size_t keyLength;
    int written;

    keyLength = (size_t) SDL_snprintf(key, sizeof(key), "%s=", name);
    while (strncmp(line, key, keyLength) != 0)
    {
        line = strstr(line, "\n");
        assert(line != NULL);
        ++line;
    }
    valueStart = line + keyLength;
    valueEnd = valueStart + strcspn(valueStart, "\r\n");
    replaced = (char *) SDL_malloc(*size + 0x20);
    assert(replaced != NULL);
    written = SDL_snprintf(replaced, *size + 0x20, "%.*s%.12s%s",
        (int) (valueStart - text), text, value, valueEnd);
    assert(written > 0);
    SDL_free(text);
    *size = (size_t) written;
    return replaced;
}

/* The install's MEDIA.CFG as the game is handed it: its video names the
 * MCGA record and its input the mouse's (records 1 and 0). */
static char *ExpectedText(char *original, size_t *size)
{
    char *expected = (char *) SDL_malloc(*size + 0x01);

    assert(expected != NULL);
    memcpy(expected, original, *size);
    expected[*size] = '\0';
    SDL_free(original);
    assert(strncmp(expected, ConfigOptionNames[MEDIA_OPTION_VIDEO],
               strlen(ConfigOptionNames[MEDIA_OPTION_VIDEO])) == 0);
    expected = ReplaceValue(expected, size, ConfigOptionNames[MEDIA_OPTION_VIDEO],
        VideoConfigurationRecords[1].name);
    return ReplaceValue(expected, size, ConfigOptionNames[MEDIA_OPTION_INPUT],
        InputConfigurationRecords[0].name);
}

static void Text(char *text, uint16_t capacity, uint16_t *length)
{
    *length = MediaSettings_InstallerText(HostSettings_MediaChoices(), text, capacity);
    assert(*length > 0);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "first";
    char text[0xC8];
    uint16_t length;

    RequireOriginalImport();
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
        original = ExpectedText(original, &size);
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
        assert(choices->choice[MEDIA_OPTION_VIDEO] == 1 && choices->choice[MEDIA_OPTION_MUSIC] == 1);
        assert(choices->choice[MEDIA_OPTION_SOUND] == 0 && choices->choice[MEDIA_OPTION_INPUT] == 0);
        assert(strcmp(choices->soundParameter, "220") == 0);
        assert(choices->music == MEDIA_MUSIC_SOUNDFONT && choices->sound == MEDIA_SOUND_SAMPLES);
        assert(SDL_GetPathInfo(settingsPath, NULL));
    }
    else if (strcmp(mode, "own") == 0)
    {
        const RevengeMediaChoices *choices;
        char *written;

        WriteText(settingsPath, "# mine\nVIDEO = EGA\nmusic=adlib\n\nsound = ssi-2001 2C0   # the card\ninput = mouse\n");
        assert(HostSettings_Prepare());
        choices = HostSettings_MediaChoices();
        assert(choices->music == MEDIA_MUSIC_SOUNDFONT && choices->choice[MEDIA_OPTION_MUSIC] == 1);
        assert(choices->sound == MEDIA_SOUND_SAMPLES && choices->choice[MEDIA_OPTION_SOUND] == 0);
        assert(choices->choice[MEDIA_OPTION_INPUT] == 0 && strcmp(choices->soundParameter, "220") == 0);
        assert(choices->choice[MEDIA_OPTION_VIDEO] == 1);
        written = (char *) SDL_LoadFile(settingsPath, NULL);
        assert(written != NULL && strstr(written, "\nvideo = vga\n") != NULL);
        assert(strstr(written, "EGA") == NULL && strstr(written, "ega\n") == NULL);
        assert(written != NULL && strstr(written, "\nmusic = soundfont\n") != NULL);
        assert(strstr(written, "\nsound = samples\n") != NULL && strstr(written, "adlib\n") == NULL);
        assert(strstr(written, "2C0") == NULL && strstr(written, "ssi-2001") == NULL);
        SDL_free(written);
    }
    else if (strcmp(mode, "words") == 0)
    {
        const RevengeMediaChoices *choices;
        const char *off = "video = vga\nmusic = off\nsound = speaker\ninput = keys\n";
        char *written;

        WriteText(settingsPath, off);
        assert(HostSettings_Prepare());
        choices = HostSettings_MediaChoices();
        assert(choices->music == MEDIA_MUSIC_OFF && choices->choice[MEDIA_OPTION_MUSIC] == 1);
        assert(choices->sound == MEDIA_SOUND_SPEAKER && choices->choice[MEDIA_OPTION_SOUND] == 8);
        /* A file's own input word is kept: the keys stay the keys. */
        assert(choices->choice[MEDIA_OPTION_INPUT] == 1);
        written = (char *) SDL_LoadFile(settingsPath, NULL);
        assert(written != NULL && strcmp(written, off) == 0);
        SDL_free(written);
        WriteText(settingsPath, "video = vga\nmusic = Speaker\nsound = Samples\ninput = keys\n");
        assert(HostSettings_Prepare());
        choices = HostSettings_MediaChoices();
        assert(choices->music == MEDIA_MUSIC_SPEAKER && choices->choice[MEDIA_OPTION_MUSIC] == 6);
        assert(choices->sound == MEDIA_SOUND_SAMPLES && choices->choice[MEDIA_OPTION_SOUND] == 0);
    }
    else if (strcmp(mode, "extra") == 0)
    {
        WriteText(settingsPath, "video = vga\nmusic = off\nsound = samples 220\ninput = keys\n");
        (void) HostSettings_Prepare();
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
