#ifndef REVENGE_AUDIO_MIX_H
#define REVENGE_AUDIO_MIX_H

/* Our sound (src/Audio): voices that take tones, samples and notes, mixed on
 * the game's clock into 48 kHz mono frames. No device and no SDL here: the
 * host gives the mixer the clock's time and a sink for the frames
 * (src/SDL/audio.c). Everything runs on the game's thread. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_MIX_SAMPLE_RATE 48000
#define AUDIO_MIX_BLOCK_FRAMES 256

typedef void (*AudioMixSink)(const float *frames, uint32_t count);

/* Silences every voice; the next advance starts the clock afresh. */
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
/* A 1-bit stream (least significant bit first, whole words only), a bit per
 * source sample, played after what is queued. */
void AudioMix_QueueBits(const uint8_t *data, uint16_t byteCount);

/* The music synth until the soundfont replaces it: MIDI bytes as the MPU
 * path sends them, and voices as the AdLib path starts and stops them. */
void AudioMix_MidiByte(uint8_t value);
void AudioMix_StartMusicVoice(uint16_t voiceId, uint8_t note, uint8_t velocity,
	uint8_t program, uint8_t voiceClass);
void AudioMix_StopMusicVoice(uint16_t voiceId);
void AudioMix_ResetMusicVoices(void);

#ifdef __cplusplus
}
#endif

#endif
