#include "revenge/media_settings.h"

#include "revenge/debug.h"
#include "revenge/original_data.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* The player's settings stand for MEDIA.CFG: these name its options and
 * choices as the game's own tables do (ConfigOptionNames, the option
 * records), and write the text the DOS installer wrote. */
static const char *MediaChoiceName(uint16_t option, uint16_t choice)
{
    switch (option)
    {
    case MEDIA_OPTION_VIDEO: return VideoConfigurationRecords[REVENGE_INDEX(VideoConfigurationRecords, choice)].name;
    case MEDIA_OPTION_MUSIC: return MusicConfigurationRecords[REVENGE_INDEX(MusicConfigurationRecords, choice)].name;
    case MEDIA_OPTION_SOUND: return SoundConfigurationRecords[REVENGE_INDEX(SoundConfigurationRecords, choice)].name;
    default: return InputConfigurationRecords[REVENGE_INDEX(InputConfigurationRecords, choice)].name;
    }
}

static int SameNameAnyCase(const char *first, const char *second, size_t capacity)
{
    size_t index;

    for (index = 0; index < capacity; ++index)
    {
        if (toupper((unsigned char) first[index]) != toupper((unsigned char) second[index]))
            return FALSE;
        if (first[index] == '\0')
            return TRUE;
    }
    /* A full-length name: the other must end there too. */
    return second[capacity] == '\0';
}

uint16_t MediaSettings_ChoiceCount(uint16_t option)
{
    static const uint16_t counts[MEDIA_OPTION_COUNT] = {
        sizeof(VideoConfigurationRecords) / sizeof(VideoConfigurationRecords[0]),
        sizeof(MusicConfigurationRecords) / sizeof(MusicConfigurationRecords[0]),
        sizeof(SoundConfigurationRecords) / sizeof(SoundConfigurationRecords[0]),
        sizeof(InputConfigurationRecords) / sizeof(InputConfigurationRecords[0])
    };

    return counts[REVENGE_INDEX(counts, option)];
}

int MediaSettings_SoundTakesParameter(uint16_t choice)
{
    return SoundConfigurationRecords[REVENGE_INDEX(SoundConfigurationRecords, choice)]
               .hasNumericParameter != FALSE;
}

int MediaSettings_OptionIndex(const char *key)
{
    uint16_t option;

    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
        if (SameNameAnyCase(ConfigOptionNames[option], key, 0x10))
            return option;
    return -1;
}

int MediaSettings_ChoiceIndex(uint16_t option, const char *value)
{
    uint16_t choice;

    for (choice = 0; choice < MediaSettings_ChoiceCount(option); ++choice)
        if (SameNameAnyCase(MediaChoiceName(option, choice), value, 0x0C))
            return choice;
    return -1;
}

uint16_t MediaSettings_InstallerText(const RevengeMediaChoices *choices, char *text, uint16_t capacity)
{
    size_t length = 0;
    uint16_t option;

    for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
    {
        const char *name = MediaChoiceName(option, choices->choice[option]);
        int written;

        if (option == MEDIA_OPTION_SOUND &&
            MediaSettings_SoundTakesParameter(choices->choice[option]))
            written = snprintf(text + length, capacity - length, "%s=%.12s %s\r\n",
                ConfigOptionNames[option], name, choices->soundParameter);
        else
            written = snprintf(text + length, capacity - length, "%s=%.12s\r\n",
                ConfigOptionNames[option], name);
        if (written < 0 || (size_t) written >= capacity - length)
            return 0x0000;
        length += (size_t) written;
    }
    return (uint16_t) length;
}
