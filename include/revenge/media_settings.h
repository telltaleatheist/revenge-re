#ifndef REVENGE_MEDIA_SETTINGS_H
#define REVENGE_MEDIA_SETTINGS_H

#include "revenge/types.h"

/* The game's four media options, in MEDIA.CFG's order. The DOS installer
 * wrote them to MEDIA.CFG; ours are the player's settings file in our write
 * folder (src/SDL/settings.cpp), which takes them from the install's MEDIA.CFG
 * once. */
enum
{
    MEDIA_OPTION_VIDEO = 0,
    MEDIA_OPTION_MUSIC = 1,
    MEDIA_OPTION_SOUND = 2,
    MEDIA_OPTION_INPUT = 3,
    MEDIA_OPTION_COUNT = 4,
    MEDIA_SOUND_PARAMETER_CAPACITY = 8
};

/* Where the music goes: the soundfont (the game drives its Roland driver,
 * whose bytes the soundfont plays), the speaker (its own speaker driver), or
 * nowhere (the Roland driver, unheard). */
inline constexpr int MEDIA_MUSIC_SOUNDFONT = 0;
inline constexpr int MEDIA_MUSIC_SPEAKER = 1;
inline constexpr int MEDIA_MUSIC_OFF = 2;

/* Where the effects and the speech go: as digital samples (the game drives
 * its Sound Blaster path, whose samples the mixer plays), or the speaker
 * (the effects as the speaker's tone streams). */
inline constexpr int MEDIA_SOUND_SAMPLES = 0;
inline constexpr int MEDIA_SOUND_SPEAKER = 1;

/* Each option's choice: an index into that option's records (DS:3AD1,
 * 3B12, 3B87, 3B6D), and the sound card's hexadecimal parameter; and the
 * music's and the sound's ways out (MEDIA_MUSIC_*, MEDIA_SOUND_*), which
 * pick their records. */
typedef struct RevengeMediaChoices
{
    word16 choice[MEDIA_OPTION_COUNT];
    char soundParameter[MEDIA_SOUND_PARAMETER_CAPACITY];
    word16 music;
    word16 sound;
} RevengeMediaChoices;

/* The names of MEDIA.CFG's options, in the option records at DS:3A91
 * (10h bytes apart). */
extern const char *const ConfigOptionNames[MEDIA_OPTION_COUNT];

/* The number of records an option has. */
word16 MediaSettings_ChoiceCount(word16 option);
/* 1 if that record of the sound option takes a parameter. */
int MediaSettings_SoundTakesParameter(word16 choice);
/* The option a MEDIA.CFG key names, or -1 (any case). */
int MediaSettings_OptionIndex(const char *key);
/* The record of `option` a MEDIA.CFG value names, or -1 (any case). */
int MediaSettings_ChoiceIndex(word16 option, const char *value);
/* The choices as the DOS installer wrote MEDIA.CFG, CR LF lines in its
 * order; the length, or 0 if they don't fit. */
word16 MediaSettings_InstallerText(const RevengeMediaChoices *choices, char *text, word16 capacity);

/* The player's choices (the host's, from its settings file). */
const RevengeMediaChoices *HostSettings_MediaChoices(void);

#endif
