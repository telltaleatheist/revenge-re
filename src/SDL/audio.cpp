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
    MusicErrorCapacity = 256
};

static const char MusicSoundfontName[] = "FluidR3_GM-subset.sf2";
static const char MusicTableName[] = "instruments.json";

static SDL_AudioStream *audioStream;
static FILE *pcmFile;
static int musicLoaded;
static int speakerSounding;

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
    AudioMix_Advance(HostClock_NowMS() * UINT64_C(1000000), PutFrames);
}

uint8_t SDLBackend_EnablePCSpeaker(void)
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

void SDLBackend_WritePCSpeakerPITDivisor(uint16_t divisor)
{
    AudioMix_SpeakerDivisor(divisor);
}

void SDLBackend_PlaySpeech(const uint8_t *samples, uint16_t sampleCount, uint16_t sampleRateHz)
{
    const uint64_t start = HostClock_NowMS();
    uint64_t lengthNS;
    uint16_t index;

    if (sampleRateHz == 0x0000)
        return;
    lengthNS = (uint64_t) sampleCount * UINT64_C(1000000000) / sampleRateHz;
    /* The original took the speaker from its timer for the speech
	 * (373D:002E) and gave it back after (373D:020F): a tone sounding
	 * before is silent meanwhile, and sounds again after. */
    AudioMix_SpeakerGate(0);
    AudioMix_SampleRate(sampleRateHz);
    for (index = 0; index < sampleCount; ++index)
        AudioMix_QueueUnsigned8(samples[index]);
    /* The wait renders the sound a millisecond at a time. */
    while ((HostClock_NowMS() - start) * UINT64_C(1000000) < lengthNS)
    {
        HostClock_DelayMS(1);
        SDLBackend_AdvanceAudio();
    }
    AudioMix_SpeakerGate(speakerSounding);
    SDLBackend_HoldTimers(HostClock_NowMS() - start);
}

void SDLBackend_WriteMIDIByte(uint8_t midiByte)
{
    AudioMix_MidiByte(midiByte);
}

void SDLBackend_SetDigitalOutputRate(uint16_t sampleRateHz)
{
    AudioMix_SampleRate(sampleRateHz);
}

void SDLBackend_WriteUnsigned8Sample(uint8_t sample)
{
    AudioMix_QueueUnsigned8(sample);
}
