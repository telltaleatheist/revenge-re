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
static const word16 ChoiceWordCounts[MEDIA_OPTION_COUNT] = {
	sizeof(VideoRecordWords) / sizeof(VideoRecordWords[0]), sizeof(MusicRecordWords) / sizeof(MusicRecordWords[0]),
	sizeof(SoundRecordWords) / sizeof(SoundRecordWords[0]), sizeof(InputWords) / sizeof(InputWords[0])
};

enum
{
	/* The record the video's one way drives: the MCGA's. */
	VIDEO_RECORD_VGA = 1,
	MUSIC_WORD_COUNT = sizeof(MusicWords) / sizeof(MusicWords[0]),
	/* The records the music's ways out drive: the speaker's own driver, and
	 * the Roland driver for the soundfont and for none. */
	MUSIC_RECORD_ROLAND = 1,
	MUSIC_RECORD_SPEAKER = 6,
	SOUND_WORD_COUNT = sizeof(SoundWords) / sizeof(SoundWords[0]),
	/* The records the sound's ways out drive: the samples through the
	 * Sound Blaster's (its parameter always 220, the default's), and the
	 * speaker's. */
	SOUND_RECORD_SAMPLES = 0,
	SOUND_RECORD_SPEAKER = 8
};

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

static word16 MusicRecord(word16 music)
{
	return music == MEDIA_MUSIC_SPEAKER ? MUSIC_RECORD_SPEAKER : MUSIC_RECORD_ROLAND;
}

static word16 SoundRecord(word16 sound)
{
	return sound == MEDIA_SOUND_SPEAKER ? SOUND_RECORD_SPEAKER : SOUND_RECORD_SAMPLES;
}

/* A card's record read as an old value: the speaker's, or the samples. */
static word16 SoundFromRecord(word16 record)
{
	return record == SOUND_RECORD_SPEAKER ? MEDIA_SOUND_SPEAKER : MEDIA_SOUND_SAMPLES;
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

/* Our file: "option = word"; '#' starts a comment. An old card word for the
 * sound may carry the card's parameter after it, which is passed over. 1 if
 * it held an old word. */
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
		if (option == MEDIA_OPTION_VIDEO && (choice = WordIndex(VideoWords, 1, words[1])) >= 0)
		{
			/* vga: the record is set below. */
		}
		else if (option == MEDIA_OPTION_MUSIC && (choice = WordIndex(MusicWords, MUSIC_WORD_COUNT, words[1])) >= 0)
			out->music = (word16) choice;
		else if (option == MEDIA_OPTION_SOUND && (choice = WordIndex(SoundWords, SOUND_WORD_COUNT, words[1])) >= 0)
			out->sound = (word16) choice;
		else if ((choice = WordIndex(ChoiceWords[option], ChoiceWordCounts[option], words[1])) >= 0
			&& option != MEDIA_OPTION_INPUT)
		{
			if (option == MEDIA_OPTION_MUSIC)
				out->music = MusicFromRecord((word16) choice);
			else if (option == MEDIA_OPTION_SOUND)
			{
				out->sound = SoundFromRecord((word16) choice);
				if (count == 3 && MediaSettings_SoundTakesParameter((word16) choice) && IsHexParameter(words[2]))
					count = 2;
			}
			old = 1;
		}
		if (choice < 0)
			RevengeFault("%s:%d: %s can't be \"%s\"", path, lineNumber, OptionWords[option], words[1]);
		if (count == 3)
			RevengeFault("%s:%d: %s %s takes no parameter", path, lineNumber,
				OptionWords[option], words[1]);
		out->choice[option] = option == MEDIA_OPTION_VIDEO ? (word16) VIDEO_RECORD_VGA
			: option == MEDIA_OPTION_MUSIC ? MusicRecord(out->music)
			: option == MEDIA_OPTION_SOUND ? SoundRecord(out->sound) : (word16) choice;
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
		choice = MediaSettings_ChoiceIndex((word16) option, words[1]);
		if (choice < 0 || option == MEDIA_OPTION_INPUT)
			continue;
		out->choice[option] = (word16) choice;
	}
	out->choice[MEDIA_OPTION_VIDEO] = VIDEO_RECORD_VGA;
	out->music = MusicFromRecord(out->choice[MEDIA_OPTION_MUSIC]);
	out->choice[MEDIA_OPTION_MUSIC] = MusicRecord(out->music);
	out->sound = SoundFromRecord(out->choice[MEDIA_OPTION_SOUND]);
	out->choice[MEDIA_OPTION_SOUND] = SoundRecord(out->sound);
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

		const char *const *words = option == MEDIA_OPTION_VIDEO ? VideoWords
			: option == MEDIA_OPTION_MUSIC ? MusicWords
			: option == MEDIA_OPTION_SOUND ? SoundWords : ChoiceWords[option];
		word16 count = option == MEDIA_OPTION_VIDEO ? (word16) 1
			: option == MEDIA_OPTION_MUSIC ? (word16) MUSIC_WORD_COUNT
			: option == MEDIA_OPTION_SOUND ? (word16) SOUND_WORD_COUNT : ChoiceWordCounts[option];

		length += snprintf(text + length, sizeof(text) - (size_t) length, "#   %s:", OptionWords[option]);
		for (choice = 0; choice < count; ++choice)
			length += snprintf(text + length, sizeof(text) - (size_t) length, " %s", words[choice]);
		length += snprintf(text + length, sizeof(text) - (size_t) length, "\n");
	}
	length += snprintf(text + length, sizeof(text) - (size_t) length, "\n");
	for (option = 0; option < MEDIA_OPTION_COUNT; ++option)
	{
		word16 choice = values->choice[option];

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
