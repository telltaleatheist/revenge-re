#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

/* The sound's way out: an SDL3 stream that the mixer (src/Audio) fills on the
 * game's clock, and the game's sound calls handed to the mixer. No synthesis
 * here, and SDL's thread runs no game code: the stream only plays what the
 * game's thread has put in it, at most a quarter of a second ahead.
 * REVENGE_AUDIO_PCM=FILE also writes every frame there, as 32-bit floats,
 * stereo (left, then right), 48 kHz, for listening to a run without a
 * device. The music's soundfont and instrument table are read from the data
 * folder (data/music) once, when the music is the soundfont. */
#include "revenge/platform_backend.h"
#include "revenge/audio.h"
#include "revenge/debug.h"
#include "revenge/media_settings.h"
#include "audio_mix.h"
#include "game_paths.h"
#include "harness.h"

#include <SDL3/SDL.h>
#include <stdio.h>

enum
{
	/* The most the stream holds before a block is dropped: a quarter of a
	 * second, in bytes. */
	AudioQueueLimitBytes = AUDIO_MIX_SAMPLE_RATE / 4 * AUDIO_MIX_CHANNELS * (int) sizeof(float),
	MusicPathCapacity = 4096,
	/* The speech's wait renders the sound a millisecond at a time. */
	SpeechWaitStepNS = 1000000,
	MusicErrorCapacity = 256
};

static const char MusicSoundfontName[] = "FluidR3_GM-subset.sf2";
static const char MusicTableName[] = "instruments.json";

static SDL_AudioStream *audioStream;
static FILE *pcmFile;
static int musicLoaded;
static int speakerSounding;
static word16 timedDelayScale;
static word16 timedByteCount;

static void PutFrames(const float *frames, uint32_t count)
{
	if (pcmFile != NULL)
		(void) fwrite(frames, sizeof(float) * AUDIO_MIX_CHANNELS, count, pcmFile);
	if (audioStream == NULL || SDL_GetAudioStreamQueued(audioStream) > AudioQueueLimitBytes)
		return;
	(void) SDL_PutAudioStreamData(audioStream, frames, (int) (count * AUDIO_MIX_CHANNELS * sizeof(float)));
}

/* The soundfont and the table, from data/music; a fault naming the file if
 * either is missing or wrong, as they are ours and ship with the program. */
static void LoadMusic(void)
{
	const char *data = GamePaths_DataDirectory();
	char soundfontPath[MusicPathCapacity];
	char tablePath[MusicPathCapacity];
	char error[MusicErrorCapacity];
	size_t soundfontSize = 0;
	size_t tableSize = 0;
	void *soundfont;
	void *table;

	if (data == NULL)
		RevengeFault("no data folder for the music");
	(void) SDL_snprintf(soundfontPath, sizeof(soundfontPath), "%s/music/%s", data, MusicSoundfontName);
	(void) SDL_snprintf(tablePath, sizeof(tablePath), "%s/music/%s", data, MusicTableName);
	soundfont = SDL_LoadFile(soundfontPath, &soundfontSize);
	if (soundfont == NULL)
		RevengeFault("can't read %s", soundfontPath);
	table = SDL_LoadFile(tablePath, &tableSize);
	if (table == NULL)
		RevengeFault("can't read %s", tablePath);
	if (!AudioMix_LoadMusic(soundfont, soundfontSize, (const char *) table, tableSize, error, sizeof(error)))
		RevengeFault("%s, %s: %s", soundfontPath, MusicTableName, error);
	SDL_free(table);
	SDL_free(soundfont);
	musicLoaded = 1;
}

int SDLBackend_OpenAudio(void)
{
	SDL_AudioSpec specification;

	if (audioStream != NULL)
		return TRUE;
	if (HostSettings_MediaChoices()->music == MEDIA_MUSIC_SOUNDFONT && !musicLoaded)
		LoadMusic();
	AudioMix_Reset();
	AudioMix_MusicOn(HostSettings_MediaChoices()->music == MEDIA_MUSIC_SOUNDFONT);
	if (pcmFile == NULL && SDL_getenv("REVENGE_AUDIO_PCM") != NULL)
		pcmFile = fopen(SDL_getenv("REVENGE_AUDIO_PCM"), "wb");
	specification.format = SDL_AUDIO_F32;
	specification.channels = AUDIO_MIX_CHANNELS;
	specification.freq = AUDIO_MIX_SAMPLE_RATE;
	audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification, NULL, NULL);
	if (audioStream == NULL || !SDL_ResumeAudioStreamDevice(audioStream))
	{
		SDLBackend_CloseAudio();
		return FALSE;
	}
	return TRUE;
}

void SDLBackend_CloseAudio(void)
{
	SDL_DestroyAudioStream(audioStream);
	audioStream = NULL;
	if (pcmFile != NULL)
		(void) fclose(pcmFile);
	pcmFile = NULL;
	AudioMix_Reset();
}

void SDLBackend_AdvanceAudio(void)
{
	AudioMix_Advance(HostClock_NowNS(), PutFrames);
}

byte SDLBackend_EnablePCSpeaker(void)
{
	speakerSounding = 1;
	AudioMix_SpeakerGate(1);
	return 0x03;
}

void SDLBackend_SilencePCSpeaker(void)
{
	speakerSounding = 0;
	AudioMix_SpeakerGate(0);
}

void SDLBackend_WritePCSpeakerPITDivisor(uint16 divisor)
{
	AudioMix_SpeakerDivisor(divisor);
}

void SDLBackend_BeginPCSpeakerTimedOutput(word16 delayScale)
{
	/* 373D:000E loads the word argument into DX, but every delay loop uses
	 * DL/DH/AH. The high argument byte is therefore discarded by the DOS
	 * routine before any timed output is produced. */
	timedDelayScale = (word16) (byte) delayScale;
	timedByteCount = 0x0000;
}

void SDLBackend_WritePCSpeakerBitstream(const byte *sampleData, word16 byteCount)
{
	/* 373D:0057-005A converts the byte count to a word count. The original
	 * unrolled loop therefore consumes complete little-endian words only. */
	timedByteCount = (word16) (timedByteCount + (byteCount & 0xFFFE));
	AudioMix_QueueBits(sampleData, byteCount);
}

word16 SDLBackend_EndPCSpeakerTimedOutput(void)
{
	/* The DOS routine measured calibration output with PIT channel zero. The
	 * host has no corresponding busy-loop cost, so report the instruction-
	 * proportional count used by the recovered scale search. */
	return (word16) ((uint32) timedByteCount * 0x08 * timedDelayScale);
}

void SDLBackend_PlaySpeech(const byte *samples, word16 sampleCount, word16 sampleRateHz)
{
	const uint64_t start = HostClock_NowNS();
	uint64_t length;
	uint64_t now;
	word16 index;

	if (sampleRateHz == 0x0000)
		return;
	length = (uint64_t) sampleCount * UINT64_C(1000000000) / sampleRateHz;
	/* The original took the speaker from its timer for the speech
	 * (373D:002E) and gave it back after (373D:020F): a tone sounding
	 * before is silent meanwhile, and sounds again after. */
	AudioMix_SpeakerGate(0);
	AudioMix_SampleRate(sampleRateHz);
	for (index = 0; index < sampleCount; ++index)
		AudioMix_QueueUnsigned8(samples[index]);
	while ((now = HostClock_NowNS()) - start < length)
	{
		const uint64_t left = length - (now - start);

		HostClock_DelayNS(left < SpeechWaitStepNS ? left : SpeechWaitStepNS);
		SDLBackend_AdvanceAudio();
	}
	AudioMix_SpeakerGate(speakerSounding);
	SDLBackend_HoldTimers(HostClock_NowNS() - start);
}

void SDLBackend_WriteMIDIByte(byte midiByte)
{
	AudioMix_MidiByte(midiByte);
}

/* The AdLib driver's voices. The settings no longer choose it (its word
 * reads as the soundfont), so nothing plays them; the card drivers go in
 * phase 3's next slice. */
void SDLBackend_StartMusicVoice(word16 voiceId, byte note, byte velocity, byte program, byte voiceClass)
{
	(void) voiceId;
	(void) note;
	(void) velocity;
	(void) program;
	(void) voiceClass;
}

void SDLBackend_StopMusicVoice(word16 voiceId)
{
	(void) voiceId;
}

void SDLBackend_ResetMusicVoices(void)
{
}

word16 SDLBackend_ProbeLegacyAudioDevice(byte deviceId)
{
	/* Device 03h is the original AdLib selection. Hosted builds intercept its
	 * semantic voice operations before they become OPL register writes. Device
	 * 04h remains the byte-oriented MPU-401 path. */
	return deviceId == 0x03 || deviceId == 0x04 ? TRUE : FALSE;
}

byte SDLBackend_ReadLegacyAudioPort(word16 port)
{
	(void) port;
	return 0x00;
}

void SDLBackend_WriteLegacyAudioPort(word16 port, byte value)
{
	(void) port;
	(void) value;
}

void SDLBackend_SetDigitalOutputRate(word16 sampleRateHz)
{
	AudioMix_SampleRate(sampleRateHz);
}

void SDLBackend_WriteUnsigned8Sample(byte sample)
{
	AudioMix_QueueUnsigned8(sample);
}
