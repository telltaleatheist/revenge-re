#ifndef REVENGE_AUDIO_MIX_H
#define REVENGE_AUDIO_MIX_H

/* Our sound (src/Audio): voices that take tones, samples and the music's
 * MIDI bytes, mixed on the game's clock into 48 kHz stereo frames. No device
 * and no SDL here: the host gives the mixer the clock's time, a sink for the
 * frames and the music's files (src/SDL/audio.cpp). Everything runs on the
 * game's thread. */

#include <stddef.h>
#include <stdint.h>


#define AUDIO_MIX_SAMPLE_RATE  48000
#define AUDIO_MIX_CHANNELS     2
#define AUDIO_MIX_BLOCK_FRAMES 256

/* count frames, each AUDIO_MIX_CHANNELS floats (left, then right). */
typedef void (*AudioMixSink)(const float *frames, uint32_t count);

/* Silences every voice, and the music's MT-32 is as it starts; the next
 * advance starts the clock afresh. */
void AudioMix_Reset(void);

/* Renders every frame due by nowNanoseconds (the host clock), sending each
 * full block to sink. The first call after a reset only starts the clock. */
void AudioMix_Advance(uint64_t nowNanoseconds, AudioMixSink sink);

/* The speaker's tone: the PIT divisor (0 stands for 65536) and its gate. */
void AudioMix_SpeakerGate(int sounding);
void AudioMix_SpeakerDivisor(uint16_t divisor);

/* The samples' source rate from now (0 keeps it; 8000 to begin with). */
void AudioMix_SampleRate(uint32_t sourceRateHz);
/* One unsigned 8-bit sample, played after those queued. */
void AudioMix_QueueUnsigned8(uint8_t sample);

/* The music (src/Audio/Music.h): the soundfont's bytes and the instrument
 * table's text. 1 if both load; else 0, with why in `error`. Until then, and
 * while it is off, the music is silent. */
int AudioMix_LoadMusic(const void *soundfont, size_t soundfontSize, const char *table, size_t tableSize,
    char *error, size_t errorCapacity);
void AudioMix_MusicOn(int on);
/* A byte the game sends its Roland driver. */
void AudioMix_MidiByte(uint8_t value);


#endif
