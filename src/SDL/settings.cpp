#include "settings.h"

#include "game_paths.h"
#include "revenge/debug.h"
#include "revenge/media_settings.h"
#include "revenge/upgrades.h"

#include <SDL3/SDL.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum
{
    SETTINGS_PATH_CAPACITY = 4096,
    SETTINGS_LINE_CAPACITY = 0x80,
    SETTINGS_TEXT_CAPACITY = 0x1000
};

/* Our word for each option, and for each of its records in the records'
 * order (DS:3AD1 video, 3B12 music, 3B87 sound, 3B6D input). */
/* The music's and the sound's words are the rebuild's own: where each goes
 * (MEDIA_MUSIC_*, MEDIA_SOUND_*). The card words, their records' (DS:3B12,
 * 3B87), are still read as old values, once: the speaker's as the speaker,
 * every card's as the soundfont or the samples, and the file is written
 * again in the new words. */
static const char *const OptionWords[MEDIA_OPTION_COUNT] = { "video", "music", "sound", "input" };
/* The video has one word, as the rebuild draws every screen one way: in the
 * 256-colour mode, as the game does on the MCGA record (DS:3AD1's second).
 * On its VGA record the game sets DS:83DC (2C7F:06F7) and runs the battle in
 * the 16-colour mode (0D73:0296, 030E), with the same pixels. The records'
 * words are read as old values, once, and all of them as vga. */
static const char *const VideoWords[] = { "vga" };
static const char *const VideoRecordWords[] = { "vga", "mcga", "ega", "tandy", "cga" };
static const char *const MusicWords[] = { "soundfont", "speaker", "off" };
static const char *const MusicRecordWords[] = {
    "lapc-1", "mt-32", "sound-blaster", "adlib", "ps-1", "tandy", "pc-speaker"
};
static const char *const SoundWords[] = { "samples", "speaker" };
static const char *const SoundRecordWords[] = {
    "sound-blaster", "game-blaster", "ssi-2001", "sound-master", "adlib", "ps-1",
    "tandy-dac", "tandy", "pc-speaker"
};
static const char *const InputWords[] = { "mouse", "keys" };

/* Each option's words; the music's and the sound's are their records', as
 * the game counts them (the player's words are MusicWords and SoundWords). */
static const char *const *const ChoiceWords[MEDIA_OPTION_COUNT] = {
    VideoRecordWords, MusicRecordWords, SoundRecordWords, InputWords
};
static const uint16_t ChoiceWordCounts[MEDIA_OPTION_COUNT] = {
    sizeof(VideoRecordWords) / sizeof(VideoRecordWords[0]), sizeof(MusicRecordWords) / sizeof(MusicRecordWords[0]),
    sizeof(SoundRecordWords) / sizeof(SoundRecordWords[0]), sizeof(InputWords) / sizeof(InputWords[0])
};

/* The record the video's one way drives: the MCGA's. */
constexpr int VIDEO_RECORD_VGA = 1;
constexpr int MUSIC_WORD_COUNT = sizeof(MusicWords) / sizeof(MusicWords[0]);
/* The records the music's ways out drive: the speaker's own driver, and
 * the Roland driver for the soundfont and for none. */
constexpr int MUSIC_RECORD_ROLAND = 1;
constexpr int MUSIC_RECORD_SPEAKER = 6;
constexpr int SOUND_WORD_COUNT = sizeof(SoundWords) / sizeof(SoundWords[0]);
/* The records the sound's ways out drive: the samples through the
 * Sound Blaster's (its parameter always 220, the default's), and the
 * speaker's. */
constexpr int SOUND_RECORD_SAMPLES = 0;
constexpr int SOUND_RECORD_SPEAKER = 8;

/* With no MEDIA.CFG: VGA (the MCGA record), the music on the soundfont, the effects as
 * digital samples, the mouse. The keys work as fully with the mouse chosen
 * (the mouse only adds to them); with the keys chosen the game ignores the
 * mouse (0D73:0006), which a player clicking would take for broken. */
enum
{
    INPUT_RECORD_MOUSE = 0
};

static const RevengeMediaChoices DefaultChoices = {
    { VIDEO_RECORD_VGA, MUSIC_RECORD_ROLAND, SOUND_RECORD_SAMPLES, INPUT_RECORD_MOUSE }, "220",
    MEDIA_MUSIC_SOUNDFONT, MEDIA_SOUND_SAMPLES
};

static RevengeMediaChoices choices;
static int prepared;
/* The player's switches as the file has them (include/revenge/upgrades.h);
 * in test mode the game's are the run's instead, and these are kept for the
 * file. */
static int fileUpgrades[UPGRADE_COUNT];
static char settingsPath[SETTINGS_PATH_CAPACITY];

static uint16_t MusicRecord(uint16_t music)
{
    return music == MEDIA_MUSIC_SPEAKER ? MUSIC_RECORD_SPEAKER : MUSIC_RECORD_ROLAND;
}

static uint16_t SoundRecord(uint16_t sound)
{
    return sound == MEDIA_SOUND_SPEAKER ? SOUND_RECORD_SPEAKER : SOUND_RECORD_SAMPLES;
}

/* A card's record read as an old value: the speaker's, or the samples. */
static uint16_t SoundFromRecord(uint16_t record)
{
    return record == SOUND_RECORD_SPEAKER ? MEDIA_SOUND_SPEAKER : MEDIA_SOUND_SAMPLES;
}

/* A card's record read as an old value: the speaker's, or the soundfont. */
static uint16_t MusicFromRecord(uint16_t record)
{
    return record == MUSIC_RECORD_SPEAKER ? MEDIA_MUSIC_SPEAKER : MEDIA_MUSIC_SOUNDFONT;
}

const RevengeMediaChoices *HostSettings_MediaChoices(void)
{
    return prepared ? &choices : &DefaultChoices;
}

static int SameWord(const char *first, const char *second)
{
    while (*first != '\0' && tolower((unsigned char) *first) == tolower((unsigned char) *second))
    {
        ++first;
        ++second;
    }
    return *first == '\0' && *second == '\0';
}

static int WordIndex(const char *const *words, uint16_t count, const char *word)
{
    uint16_t index;

    for (index = 0; index < count; ++index)
        if (SameWord(words[index], word))
            return index;
    return -1;
}

static int IsHexParameter(const char *text)
{
    size_t length = strlen(text);
    size_t index;

    if (length == 0 || length >= MEDIA_SOUND_PARAMETER_CAPACITY)
        return 0;
    for (index = 0; index < length; ++index)
        if (!isxdigit((unsigned char) text[index]))
            return 0;
    return 1;
}

/* Splits line into up to three words at spaces, tabs and one '='. */
static int SplitLine(char *line, char **words, int capacity, char separator)
{
    int count = 0;
    char *cursor = line;

    while (*cursor != '\0' && count < capacity)
    {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == separator)
            ++cursor;
        if (*cursor == '\0')
            break;
        words[count++] = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' && *cursor != separator)
            ++cursor;
        if (*cursor != '\0')
            *cursor++ = '\0';
    }
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r')
        ++cursor;
    return *cursor == '\0' ? count : capacity + 1;
}

static char *NextLine(char **cursor)
{
    char *line = *cursor;
    char *end;

    if (*line == '\0')
        return NULL;
    end = strchr(line, '\n');
    if (end != NULL)
    {
        *end = '\0';
        *cursor = end + 1;
    }
    else
        *cursor = line + strlen(line);
    return line;
}

/* "upgrade_<key> = on|off" (include/revenge/upgrades.h): 1 if the line is
 * one, its switch set in upgrades. A key this version doesn't know is a later
 * version's switch, passed over, so going back a version still starts. */
static int ReadUpgradeLine(char **words, int count, const char *path, int lineNumber,
    int *upgrades, int *seenUpgrades)
{
    static const char Prefix[] = "upgrade_";
    size_t index;
    int upgrade;
    int on;

    for (index = 0; index < sizeof(Prefix) - 1; ++index)
        if (tolower((unsigned char) words[0][index]) != Prefix[index])
            return 0;
    upgrade = Upgrades_Index(words[0] + sizeof(Prefix) - 1);
    if (upgrade < 0)
        return 1;
    if (count != 2)
        RevengeFault("%s:%d: not \"%s = on\" or \"off\"", path, lineNumber, words[0]);
    if (seenUpgrades[upgrade])
        RevengeFault("%s:%d: %s is set twice", path, lineNumber, words[0]);
    seenUpgrades[upgrade] = 1;
    if (SameWord(words[1], "on"))
        on = 1;
    else if (SameWord(words[1], "off"))
        on = 0;
    else
        RevengeFault("%s:%d: %s can't be \"%s\"", path, lineNumber, words[0], words[1]);
    upgrades[upgrade] = on;
    return 1;
}

/* Our file: "option = word"; '#' starts a comment. An old card word for the
 * sound may carry the card's parameter after it, which is passed over. The
 * switches' lines may follow, or be absent (off). 1 if it held an old word. */
static int ReadOurFile(char *text, const char *path, RevengeMediaChoices *out, int *upgrades)
{
    int old = 0;
    int seen[MEDIA_OPTION_COUNT] = { 0, 0, 0, 0 };
    int seenUpgrades[UPGRADE_COUNT] = { 0 };
    char *cursor = text;
    char *line;
    int lineNumber = 0;
    int option;

    *out = DefaultChoices;
    for (option = 0; option < UPGRADE_COUNT; ++option)
        upgrades[option] = 0;
    while ((line = NextLine(&cursor)) != NULL)
    {
        char *words[3];
        char *comment = strchr(line, '#');
        int count;
        int choice;

        ++lineNumber;
        if (comment != NULL)
            *comment = '\0';
        count = SplitLine(line, words, 3, '=');
        if (count == 0)
            continue;
        if (count < 2 || count > 3)
            RevengeFault("%s:%d: not \"option = choice\"", path, lineNumber);
        if (ReadUpgradeLine(words, count, path, lineNumber, upgrades, seenUpgrades))
            continue;
        option = WordIndex(OptionWords, MEDIA_OPTION_COUNT, words[0]);
        if (option < 0)
            RevengeFault("%s:%d: no option \"%s\"", path, lineNumber, words[0]);
        if (seen[option])
            RevengeFault("%s:%d: %s is set twice", path, lineNumber, OptionWords[option]);
        seen[option] = 1;
        if (option == MEDIA_OPTION_VIDEO && (choice = WordIndex(VideoWords, 1, words[1])) >= 0)
        {
            /* vga: the record is set below. */
        }
        else if (option == MEDIA_OPTION_MUSIC && (choice = WordIndex(MusicWords, MUSIC_WORD_COUNT, words[1])) >= 0)
            out->music = (uint16_t) choice;
        else if (option == MEDIA_OPTION_SOUND && (choice = WordIndex(SoundWords, SOUND_WORD_COUNT, words[1])) >= 0)
            out->sound = (uint16_t) choice;
        else if ((choice = WordIndex(ChoiceWords[option], ChoiceWordCounts[option], words[1])) >= 0 && option != MEDIA_OPTION_INPUT)
        {
            if (option == MEDIA_OPTION_MUSIC)
                out->music = MusicFromRecord((uint16_t) choice);
            else if (option == MEDIA_OPTION_SOUND)
            {
                out->sound = SoundFromRecord((uint16_t) choice);
                if (count == 3 && MediaSettings_SoundTakesParameter((uint16_t) choice) && IsHexParameter(words[2]))
                    count = 2;
            }
            old = 1;
        }
        if (choice < 0)
            RevengeFault("%s:%d: %s can't be \"%s\"", path, lineNumber, OptionWords[option], words[1]);
        if (count == 3)
            RevengeFault("%s:%d: %s %s takes no parameter", path, lineNumber,
                OptionWords[option], words[1]);
        out->choice[option] = option == MEDIA_OPTION_VIDEO ? (uint16_t) VIDEO_RECORD_VGA
            : option == MEDIA_OPTION_MUSIC                 ? MusicRecord(out->music)
            : option == MEDIA_OPTION_SOUND                 ? SoundRecord(out->sound)
                                                           : (uint16_t) choice;
    }
    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
        if (!seen[option])
            RevengeFault("%s: no %s line", path, OptionWords[option]);
    return old;
}

/* The DOS installer's MEDIA.CFG: "KEY=VALUE[ PARAMETER]" lines. What it
 * names that the game knows is taken (a sound card's parameter is not: the
 * samples keep theirs), but the video, which is always the MCGA record,
 * and the input, which starts as the mouse: the installs ship with the
 * keys chosen. Anything else keeps our default. */
static void ReadMediaConfiguration(char *text, RevengeMediaChoices *out)
{
    char *cursor = text;
    char *line;

    *out = DefaultChoices;
    while ((line = NextLine(&cursor)) != NULL)
    {
        char *words[3];
        int count = SplitLine(line, words, 3, '=');
        int option;
        int choice;

        if (count < 2 || count > 3)
            continue;
        option = MediaSettings_OptionIndex(words[0]);
        if (option < 0)
            continue;
        choice = MediaSettings_ChoiceIndex((uint16_t) option, words[1]);
        if (choice < 0 || option == MEDIA_OPTION_INPUT)
            continue;
        out->choice[option] = (uint16_t) choice;
    }
    out->choice[MEDIA_OPTION_VIDEO] = VIDEO_RECORD_VGA;
    out->music = MusicFromRecord(out->choice[MEDIA_OPTION_MUSIC]);
    out->choice[MEDIA_OPTION_MUSIC] = MusicRecord(out->music);
    out->sound = SoundFromRecord(out->choice[MEDIA_OPTION_SOUND]);
    out->choice[MEDIA_OPTION_SOUND] = SoundRecord(out->sound);
}

static void WriteOurFile(const char *path, const RevengeMediaChoices *values, const int *upgrades)
{
    char text[SETTINGS_TEXT_CAPACITY];
    int length;
    int option;

    length = snprintf(text, sizeof(text),
        "# The game's settings, read at every start. Each line is\n"
        "# \"option = choice\"; the choices are:\n");
    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
    {
        uint16_t choice;

        const char *const *words = option == MEDIA_OPTION_VIDEO ? VideoWords
            : option == MEDIA_OPTION_MUSIC                      ? MusicWords
            : option == MEDIA_OPTION_SOUND                      ? SoundWords
                                                                : ChoiceWords[option];
        uint16_t count = option == MEDIA_OPTION_VIDEO ? (uint16_t) 1
            : option == MEDIA_OPTION_MUSIC            ? (uint16_t) MUSIC_WORD_COUNT
            : option == MEDIA_OPTION_SOUND            ? (uint16_t) SOUND_WORD_COUNT
                                                      : ChoiceWordCounts[option];

        length += snprintf(text + length, sizeof(text) - (size_t) length, "#   %s:", OptionWords[option]);
        for (choice = 0; choice < count; ++choice)
            length += snprintf(text + length, sizeof(text) - (size_t) length, " %s", words[choice]);
        length += snprintf(text + length, sizeof(text) - (size_t) length, "\n");
    }
    length += snprintf(text + length, sizeof(text) - (size_t) length, "\n");
    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
    {
        uint16_t choice = values->choice[option];

        if (option == MEDIA_OPTION_VIDEO)
            length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
                OptionWords[option], VideoWords[0]);
        else if (option == MEDIA_OPTION_SOUND)
            length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
                OptionWords[option], SoundWords[values->sound]);
        else if (option == MEDIA_OPTION_MUSIC)
            length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
                OptionWords[option], MusicWords[values->music]);
        else
            length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
                OptionWords[option], ChoiceWords[option][choice]);
    }
    length += snprintf(text + length, sizeof(text) - (size_t) length,
        "\n# The UPGRADES page's switches, fixes for the original's own bugs:\n"
        "# on or off. Off plays as the original did.\n");
    for (option = 0; option < UPGRADE_COUNT; ++option)
        length += snprintf(text + length, sizeof(text) - (size_t) length, "upgrade_%s = %s\n",
            Upgrades_Key(option), upgrades[option] ? "on" : "off");
    if (length <= 0 || (size_t) length >= sizeof(text) || !SDL_SaveFile(path, text, (size_t) length))
        RevengeFault("can't write %s", path);
}

static char *LoadText(const char *path)
{
    size_t size = 0;
    char *data = (char *) SDL_LoadFile(path, &size);

    if (data != NULL && strlen(data) != size)
    {
        SDL_free(data);
        RevengeFault("%s holds a zero byte", path);
    }
    return data;
}

/* Test mode: the trace harness runs, or the kit's RE_KIT_TEST=1. */
static int InTestMode(void)
{
    const char *script = SDL_getenv("REVENGE_HARNESS_SCRIPT");
    const char *kit = SDL_getenv("RE_KIT_TEST");

    return (script != NULL && script[0] != '\0') || (kit != NULL && strcmp(kit, "1") == 0);
}

/* The game's switches: the file's, or in test mode only those the run names
 * in REVENGE_TEST_UPGRADES (keys apart by spaces or commas; a trace script's
 * "# upgrades:" line, tools/trace/record.py). */
static void ApplyUpgrades(void)
{
    int upgrade;

    Upgrades_SetTestMode(InTestMode());
    Upgrades_SetSaver(HostSettings_SaveUpgrades);
    {
        const char *line = SDL_getenv("REVENGE_TEST_UPGRADES_LINE");

        Upgrades_SetLineShown(!Upgrades_TestMode() || (line != NULL && strcmp(line, "1") == 0));
    }
    for (upgrade = 0; upgrade < UPGRADE_COUNT; ++upgrade)
        Upgrades_Set(upgrade, Upgrades_TestMode() ? 0 : fileUpgrades[upgrade]);
    if (Upgrades_TestMode())
    {
        const char *named = SDL_getenv("REVENGE_TEST_UPGRADES");
        char list[SETTINGS_LINE_CAPACITY];
        char *cursor = list;

        if (named == NULL || named[0] == '\0')
            return;
        const size_t length = strlen(named);

        if (length >= sizeof(list))
            RevengeFault("REVENGE_TEST_UPGRADES is too long");
        memcpy(list, named, length + 1);
        while (*cursor != '\0')
        {
            char *word;

            while (*cursor == ' ' || *cursor == ',')
                ++cursor;
            if (*cursor == '\0')
                break;
            word = cursor;
            while (*cursor != '\0' && *cursor != ' ' && *cursor != ',')
                ++cursor;
            if (*cursor != '\0')
                *cursor++ = '\0';
            upgrade = Upgrades_Index(word);
            if (upgrade < 0)
                RevengeFault("REVENGE_TEST_UPGRADES: no switch \"%s\"", word);
            Upgrades_Set(upgrade, 1);
        }
    }
}

void HostSettings_SaveUpgrades(void)
{
    int upgrade;

    if (Upgrades_TestMode())
        return;
    for (upgrade = 0; upgrade < UPGRADE_COUNT; ++upgrade)
        fileUpgrades[upgrade] = Upgrades_IsOn(upgrade);
    if (prepared && settingsPath[0] != '\0')
        WriteOurFile(settingsPath, &choices, fileUpgrades);
}

/* The settings a first start makes: the install's MEDIA.CFG (the DOS
 * installer's choices), else our defaults, every UPGRADES switch off;
 * written to `path` when there is one, and taken in. */
static void MakeFreshSettings(const char *path)
{
    char install[SETTINGS_PATH_CAPACITY];
    char *text;
    int option;

    choices = DefaultChoices;
    for (option = 0; option < UPGRADE_COUNT; ++option)
        fileUpgrades[option] = 0;
    if (GamePaths_Resolve("MEDIA.CFG", GAME_PATH_READ, install, sizeof(install)) == 0 &&
        (text = LoadText(install)) != NULL)
    {
        ReadMediaConfiguration(text, &choices);
        SDL_free(text);
    }
    if (path[0] != '\0')
        WriteOurFile(path, &choices, fileUpgrades);
    prepared = 1;
    ApplyUpgrades();
}

int HostSettings_Prepare(void)
{
    const char *folder = GamePaths_WriteDirectory();
    char *path = settingsPath;
    char *text;
    int option;

    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
        if (ChoiceWordCounts[option] != MediaSettings_ChoiceCount((uint16_t) option))
            RevengeFault("the %s option has %u records, and settings.cpp names %u",
                OptionWords[option], (unsigned) MediaSettings_ChoiceCount((uint16_t) option),
                (unsigned) ChoiceWordCounts[option]);

    path[0] = '\0';
    if (folder != NULL)
    {
        snprintf(path, sizeof(settingsPath), "%s%s", folder, HOST_SETTINGS_FILE_NAME);
        text = LoadText(path);
        if (text != NULL)
        {
            int old = ReadOurFile(text, path, &choices, fileUpgrades);

            SDL_free(text);
            if (old)
                WriteOurFile(path, &choices, fileUpgrades);
            prepared = 1;
            ApplyUpgrades();
            return 1;
        }
    }

    MakeFreshSettings(path);
    return 1;
}

int HostSettings_Reset(void)
{
    if (settingsPath[0] != '\0' && !SDL_RemovePath(settingsPath) && SDL_GetPathInfo(settingsPath, NULL))
        return 0;
    MakeFreshSettings(settingsPath);
    return 1;
}
