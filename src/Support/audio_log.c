#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

/* The audio log (revenge/audio_log.h). */
#include "revenge/audio_log.h"

#include "revenge/sha256.h"

#include <stdio.h>
#include <stdlib.h>

enum
{
	SYSEX_HASH_HEX_DIGITS = 16
};

static FILE *logFile;
static uint64_t (*clockNow)(void);

/* The message being gathered from the sequencer's bytes. */
static uint8_t status;
static uint8_t data[2];
static unsigned dataCount;
static int inSysex;
static unsigned sysexLength;
static Sha256 sysexHash;

/* The speaker: its gate and divisor, and the tone last written. */
static int speakerSounding;
static uint16_t speakerDivisor;
static uint16_t loggedTone;

static unsigned long long Now(void)
{
	return (unsigned long long) (clockNow() / 1000);
}

void AudioLog_Open(const char *path, uint64_t (*nowNanoseconds)(void))
{
	if (path == NULL || path[0] == '\0' || nowNanoseconds == NULL)
		return;
	logFile = fopen(path, "wb");
	if (logFile == NULL)
	{
		fprintf(stderr, "audio log: can't write %s\n", path);
		exit(70);
	}
	clockNow = nowNanoseconds;
}

void AudioLog_Close(void)
{
	if (logFile != NULL)
		(void) fclose(logFile);
	logFile = NULL;
}

static unsigned DataBytesFor(uint8_t message)
{
	switch (message & 0xF0)
	{
	case 0xC0:
	case 0xD0:
		return 1;
	case 0xF0:
		if (message == 0xF2)
			return 2;
		return message == 0xF1 || message == 0xF3 ? 1 : 0;
	default:
		return 2;
	}
}

static void EndSysex(void)
{
	uint8_t digest[SHA256_DIGEST_BYTES];
	unsigned index;

	Sha256_End(&sysexHash, digest);
	fprintf(logFile, "%llu sysex %u ", Now(), sysexLength);
	for (index = 0; index < SYSEX_HASH_HEX_DIGITS / 2; ++index)
		fprintf(logFile, "%02X", digest[index]);
	fputc('\n', logFile);
	inSysex = 0;
}

static void WriteMessage(void)
{
	fprintf(logFile, "%llu midi %02X", Now(), status);
	if (dataCount > 0)
		fprintf(logFile, " %02X", data[0]);
	if (dataCount > 1)
		fprintf(logFile, " %02X", data[1]);
	fputc('\n', logFile);
}

void AudioLog_MidiByte(uint8_t value)
{
	if (logFile == NULL)
		return;
	if (value >= 0xF8)
	{
		/* Real-time bytes stand alone, even inside another message. */
		fprintf(logFile, "%llu midi %02X\n", Now(), value);
		return;
	}
	if (inSysex)
	{
		if (value < 0x80)
		{
			Sha256_Add(&sysexHash, &value, 1);
			++sysexLength;
			return;
		}
		if (value == 0xF7)
		{
			Sha256_Add(&sysexHash, &value, 1);
			++sysexLength;
			EndSysex();
			return;
		}
		EndSysex();
	}
	if (value == 0xF0)
	{
		inSysex = 1;
		sysexLength = 0;
		Sha256_Begin(&sysexHash);
		status = 0;
		return;
	}
	if (value >= 0x80)
	{
		status = value;
		dataCount = 0;
		if (DataBytesFor(status) == 0)
		{
			WriteMessage();
			status = 0;
		}
		return;
	}
	if (status == 0)
	{
		/* A data byte with no message to belong to. */
		fprintf(logFile, "%llu midi -- %02X\n", Now(), value);
		return;
	}
	data[dataCount++] = value;
	if (dataCount == DataBytesFor(status))
	{
		WriteMessage();
		dataCount = 0;
		/* Channel messages keep their status for running data; system
		 * messages don't. */
		if (status >= 0xF0)
			status = 0;
	}
}

void AudioLog_MidiReset(void)
{
	if (logFile == NULL)
		return;
	if (inSysex)
		EndSysex();
	status = 0;
	dataCount = 0;
	fprintf(logFile, "%llu midi reset\n", Now());
}

static void WriteToneIfChanged(void)
{
	uint16_t tone = speakerSounding ? speakerDivisor : 0;

	if (tone == loggedTone)
		return;
	loggedTone = tone;
	fprintf(logFile, "%llu tone %u\n", Now(), (unsigned) tone);
}

void AudioLog_SpeakerGate(int sounding)
{
	if (logFile == NULL)
		return;
	speakerSounding = sounding != 0;
	WriteToneIfChanged();
}

void AudioLog_SpeakerDivisor(uint16_t divisor)
{
	if (logFile == NULL)
		return;
	speakerDivisor = divisor;
	WriteToneIfChanged();
}

void AudioLog_ToneEffect(unsigned index)
{
	if (logFile == NULL)
		return;
	fprintf(logFile, "%llu tone-effect %u\n", Now(), index);
}

void AudioLog_PcmStart(const char *kind, unsigned index, unsigned length)
{
	if (logFile == NULL)
		return;
	fprintf(logFile, "%llu pcm %s %u %u\n", Now(), kind, index, length);
}

void AudioLog_PcmDone(void)
{
	if (logFile == NULL)
		return;
	fprintf(logFile, "%llu pcm done\n", Now());
}

void AudioLog_Snap(const char *name)
{
	if (logFile == NULL)
		return;
	fprintf(logFile, "%llu snap %s\n", Now(), name);
	(void) fflush(logFile);
}
