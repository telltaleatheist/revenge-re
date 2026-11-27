#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

/* The sound's way out: an SDL3 stream that the mixer (src/Audio) fills on the
 * game's clock, and the game's sound calls handed to the mixer. No synthesis
 * here, and SDL's thread runs no game code: the stream only plays what the
 * game's thread has put in it, at most a quarter of a second ahead.
 * REVENGE_AUDIO_PCM=FILE also writes every frame there, as 32-bit floats,
 * mono, 48 kHz, for listening to a run without a device. */
#include "revenge/platform_backend.h"
#include "revenge/audio.h"
#include "audio_mix.h"
#include "harness.h"

#include <SDL3/SDL.h>
#include <stdio.h>

enum
{
	/* The most the stream holds before a block is dropped: a quarter of a
	 * second, in bytes. */
	AudioQueueLimitBytes = AUDIO_MIX_SAMPLE_RATE / 4 * (int) sizeof(float)
};

static SDL_AudioStream *audioStream;
static FILE *pcmFile;
static word16 timedDelayScale;
static word16 timedByteCount;

static void PutFrames(const float *frames, uint32_t count)
{
	if (pcmFile != NULL)
		(void) fwrite(frames, sizeof(float), count, pcmFile);
	if (audioStream == NULL || SDL_GetAudioStreamQueued(audioStream) > AudioQueueLimitBytes)
		return;
	(void) SDL_PutAudioStreamData(audioStream, frames, (int) (count * sizeof(float)));
}

int SDLBackend_OpenAudio(void)
{
	SDL_AudioSpec specification;

	if (audioStream != NULL)
		return TRUE;
	AudioMix_Reset();
	if (pcmFile == NULL && SDL_getenv("REVENGE_AUDIO_PCM") != NULL)
		pcmFile = fopen(SDL_getenv("REVENGE_AUDIO_PCM"), "wb");
	specification.format = SDL_AUDIO_F32;
	specification.channels = 0x01;
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
	AudioMix_SpeakerGate(1);
	return 0x03;
}

void SDLBackend_SilencePCSpeaker(void)
{
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

void SDLBackend_WriteMIDIByte(byte midiByte)
{
	AudioMix_MidiByte(midiByte);
}

void SDLBackend_StartMusicVoice(word16 voiceId, byte note, byte velocity, byte program, byte voiceClass)
{
	AudioMix_StartMusicVoice(voiceId, note, velocity, program, voiceClass);
}

void SDLBackend_StopMusicVoice(word16 voiceId)
{
	AudioMix_StopMusicVoice(voiceId);
}

void SDLBackend_ResetMusicVoices(void)
{
	AudioMix_ResetMusicVoices();
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
