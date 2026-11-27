#include "settings.h"

#include "game_paths.h"
#include "revenge/debug.h"
#include "revenge/media_settings.h"

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
/* The music's words are the rebuild's own: where it goes (MEDIA_MUSIC_*).
 * The card words, its records' (DS:3B12), are still read as old values,
 * once: the speaker's as the speaker, every card's as the soundfont, and
 * the file is written again in the new words. (The sound's words go the
 * same way in phase 3's effects slice.) */
static const char *const OptionWords[MEDIA_OPTION_COUNT] = { "video", "music", "sound", "input" };
static const char *const VideoWords[] = { "vga", "mcga", "ega", "tandy", "cga" };
static const char *const MusicWords[] = { "soundfont", "speaker", "off" };
static const char *const MusicRecordWords[] = {
	"lapc-1", "mt-32", "sound-blaster", "adlib", "ps-1", "tandy", "pc-speaker"
};
static const char *const SoundWords[] = {
	"sound-blaster", "game-blaster", "ssi-2001", "sound-master", "adlib", "ps-1",
	"tandy-dac", "tandy", "pc-speaker"
};
static const char *const InputWords[] = { "mouse", "keys" };

/* Each option's words; the music's are its records', as the game counts
 * them (the player's words are MusicWords). */
static const char *const *const ChoiceWords[MEDIA_OPTION_COUNT] = {
	VideoWords, MusicRecordWords, SoundWords, InputWords
};
static const word16 ChoiceWordCounts[MEDIA_OPTION_COUNT] = {
	sizeof(VideoWords) / sizeof(VideoWords[0]), sizeof(MusicRecordWords) / sizeof(MusicRecordWords[0]),
	sizeof(SoundWords) / sizeof(SoundWords[0]), sizeof(InputWords) / sizeof(InputWords[0])
};

enum
{
	MUSIC_WORD_COUNT = sizeof(MusicWords) / sizeof(MusicWords[0]),
	/* The records the music's ways out drive: the speaker's own driver, and
	 * the Roland driver for the soundfont and for none. */
	MUSIC_RECORD_ROLAND = 1,
	MUSIC_RECORD_SPEAKER = 6
};

/* With no MEDIA.CFG: VGA, the music on the soundfont, the effects as
 * digital samples (a Sound Blaster at 220h), the keyboard. */
static const RevengeMediaChoices DefaultChoices = {
	{ 0, MUSIC_RECORD_ROLAND, 0, 1 }, "220", MEDIA_MUSIC_SOUNDFONT
};

static RevengeMediaChoices choices;
static int prepared;

static word16 MusicRecord(word16 music)
{
	return music == MEDIA_MUSIC_SPEAKER ? MUSIC_RECORD_SPEAKER : MUSIC_RECORD_ROLAND;
}

/* A card's record read as an old value: the speaker's, or the soundfont. */
static word16 MusicFromRecord(word16 record)
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

static int WordIndex(const char *const *words, word16 count, const char *word)
{
	word16 index;

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

/* Our file: "option = word", the sound's word followed by its parameter
 * when it takes one; '#' starts a comment. 1 if it held an old word. */
static int ReadOurFile(char *text, const char *path, RevengeMediaChoices *out)
{
	int old = 0;
	int seen[MEDIA_OPTION_COUNT] = { 0, 0, 0, 0 };
	char *cursor = text;
	char *line;
	int lineNumber = 0;
	int option;

	*out = DefaultChoices;
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
		option = WordIndex(OptionWords, MEDIA_OPTION_COUNT, words[0]);
		if (option < 0)
			RevengeFault("%s:%d: no option \"%s\"", path, lineNumber, words[0]);
		if (seen[option])
			RevengeFault("%s:%d: %s is set twice", path, lineNumber, OptionWords[option]);
		seen[option] = 1;
		if (option == MEDIA_OPTION_MUSIC && (choice = WordIndex(MusicWords, MUSIC_WORD_COUNT, words[1])) >= 0)
			out->music = (word16) choice;
		else if ((choice = WordIndex(ChoiceWords[option], ChoiceWordCounts[option], words[1])) >= 0
			&& option == MEDIA_OPTION_MUSIC)
		{
			out->music = MusicFromRecord((word16) choice);
			old = 1;
		}
		if (choice < 0)
			RevengeFault("%s:%d: %s can't be \"%s\"", path, lineNumber, OptionWords[option], words[1]);
		out->choice[option] = option == MEDIA_OPTION_MUSIC ? MusicRecord(out->music) : (word16) choice;
		if (option == MEDIA_OPTION_SOUND && MediaSettings_SoundTakesParameter((word16) choice))
		{
			if (count != 3 || !IsHexParameter(words[2]))
				RevengeFault("%s:%d: sound %s takes a hexadecimal parameter (such as 220)",
					path, lineNumber, SoundWords[choice]);
			snprintf(out->soundParameter, sizeof(out->soundParameter), "%s", words[2]);
		}
		else if (count == 3)
			RevengeFault("%s:%d: %s %s takes no parameter", path, lineNumber,
				OptionWords[option], words[1]);
	}
	for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
		if (!seen[option])
			RevengeFault("%s: no %s line", path, OptionWords[option]);
	return old;
}

/* The DOS installer's MEDIA.CFG: "KEY=VALUE[ PARAMETER]" lines. What it
 * names that the game knows is taken; anything else keeps our default. */
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
		choice = MediaSettings_ChoiceIndex((word16) option, words[1]);
		if (choice < 0)
			continue;
		if (option == MEDIA_OPTION_SOUND && MediaSettings_SoundTakesParameter((word16) choice))
		{
			if (count != 3 || !IsHexParameter(words[2]))
				continue;
			snprintf(out->soundParameter, sizeof(out->soundParameter), "%s", words[2]);
		}
		out->choice[option] = (word16) choice;
	}
	out->music = MusicFromRecord(out->choice[MEDIA_OPTION_MUSIC]);
	out->choice[MEDIA_OPTION_MUSIC] = MusicRecord(out->music);
}

static void WriteOurFile(const char *path, const RevengeMediaChoices *values)
{
	char text[SETTINGS_TEXT_CAPACITY];
	int length;
	int option;

	length = snprintf(text, sizeof(text),
		"# The game's settings, read at every start. Each line is\n"
		"# \"option = choice\"; the choices are:\n");
	for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
	{
		word16 choice;

		const char *const *words = option == MEDIA_OPTION_MUSIC ? MusicWords : ChoiceWords[option];
		word16 count = option == MEDIA_OPTION_MUSIC ? (word16) MUSIC_WORD_COUNT : ChoiceWordCounts[option];

		length += snprintf(text + length, sizeof(text) - (size_t) length, "#   %s:", OptionWords[option]);
		for (choice = 0; choice < count; ++choice)
			length += snprintf(text + length, sizeof(text) - (size_t) length, " %s", words[choice]);
		length += snprintf(text + length, sizeof(text) - (size_t) length, "\n");
	}
	length += snprintf(text + length, sizeof(text) - (size_t) length,
		"# These sound choices take a hexadecimal number after them (the card's\n"
		"# setting, such as 220):");
	{
		word16 choice;

		for (choice = 0; choice < ChoiceWordCounts[MEDIA_OPTION_SOUND]; ++choice)
			if (MediaSettings_SoundTakesParameter(choice))
				length += snprintf(text + length, sizeof(text) - (size_t) length, " %s",
					SoundWords[choice]);
	}
	length += snprintf(text + length, sizeof(text) - (size_t) length, "\n\n");
	for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
	{
		word16 choice = values->choice[option];

		if (option == MEDIA_OPTION_SOUND && MediaSettings_SoundTakesParameter(choice))
			length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s %s\n",
				OptionWords[option], ChoiceWords[option][choice], values->soundParameter);
		else if (option == MEDIA_OPTION_MUSIC)
			length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
				OptionWords[option], MusicWords[values->music]);
		else
			length += snprintf(text + length, sizeof(text) - (size_t) length, "%s = %s\n",
				OptionWords[option], ChoiceWords[option][choice]);
	}
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

int HostSettings_Prepare(void)
{
	const char *folder = GamePaths_WriteDirectory();
	char path[SETTINGS_PATH_CAPACITY];
	char install[SETTINGS_PATH_CAPACITY];
	char *text;
	int option;

	for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
		if (ChoiceWordCounts[option] != MediaSettings_ChoiceCount((word16) option))
			RevengeFault("the %s option has %u records, and settings.c names %u",
				OptionWords[option], (unsigned) MediaSettings_ChoiceCount((word16) option),
				(unsigned) ChoiceWordCounts[option]);

	path[0] = '\0';
	if (folder != NULL)
	{
		snprintf(path, sizeof(path), "%s%s", folder, HOST_SETTINGS_FILE_NAME);
		text = LoadText(path);
		if (text != NULL)
		{
			int old = ReadOurFile(text, path, &choices);

			SDL_free(text);
			if (old)
				WriteOurFile(path, &choices);
			prepared = 1;
			return 1;
		}
	}

	choices = DefaultChoices;
	if (GamePaths_Resolve("MEDIA.CFG", GAME_PATH_READ, install, sizeof(install)) == 0 &&
		(text = LoadText(install)) != NULL)
	{
		ReadMediaConfiguration(text, &choices);
		SDL_free(text);
	}
	if (path[0] != '\0')
		WriteOurFile(path, &choices);
	prepared = 1;
	return 1;
}
