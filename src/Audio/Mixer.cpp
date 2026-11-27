// The mixer (audio_mix.h): the speaker's tone, the samples and the music
// synth, rendered on the game's clock one 48 kHz frame at a time.
#include "audio_mix.h"

#include "LegacySynth.h"
#include "PcmVoice.h"
#include "ToneVoice.h"

#include <algorithm>
#include <array>

namespace revenge::audio {
namespace {

constexpr double kPitClockHz = 1193182.0;
// The speaker's tone sits at a quarter of full scale, as the port's had.
constexpr float kToneLevel = 0.25f;
// Never more than a second's frames at once, so a long stall doesn't make
// the game wait on a backlog it can't hear anyway.
constexpr uint64_t kMostFramesAtOnce = AUDIO_MIX_SAMPLE_RATE;

struct Mixer {
    ToneVoice tone;
    PcmVoice samples;
    LegacySynth music;
    bool speakerSounding = false;
    uint16_t speakerDivisor = 0;
    bool started = false;
    uint64_t framesDone = 0;     // frames rendered since the clock started
    uint64_t startNanoseconds = 0;
    std::array<float, AUDIO_MIX_BLOCK_FRAMES> block{};
    uint32_t blockFrames = 0;

    void updateTone() {
        if (!speakerSounding) {
            tone.stop();
            return;
        }
        tone.play(kPitClockHz / (speakerDivisor == 0 ? 65536.0 : speakerDivisor));
    }

    float frame() {
        const float value = kToneLevel * static_cast<float>(tone.mean(1.0 / AUDIO_MIX_SAMPLE_RATE)) +
                            samples.next() + static_cast<float>(music.next()) / 32768.0f;
        return std::clamp(value, -1.0f, 1.0f);
    }
};

Mixer mixer;

}  // namespace
}  // namespace revenge::audio

using revenge::audio::mixer;

extern "C" {

void AudioMix_Reset(void) {
    mixer.tone.reset();
    mixer.samples.reset();
    mixer.music.reset();
    mixer.speakerSounding = false;
    mixer.speakerDivisor = 0;
    mixer.started = false;
    mixer.framesDone = 0;
    mixer.blockFrames = 0;
}

void AudioMix_Advance(uint64_t nowNanoseconds, AudioMixSink sink) {
    if (!mixer.started || nowNanoseconds < mixer.startNanoseconds) {
        mixer.started = true;
        mixer.startNanoseconds = nowNanoseconds;
        mixer.framesDone = 0;
        return;
    }
    const uint64_t due = (nowNanoseconds - mixer.startNanoseconds) * AUDIO_MIX_SAMPLE_RATE / 1000000000ULL;
    if (due - mixer.framesDone > revenge::audio::kMostFramesAtOnce)
        mixer.framesDone = due - revenge::audio::kMostFramesAtOnce;
    while (mixer.framesDone < due) {
        mixer.block[mixer.blockFrames++] = mixer.frame();
        ++mixer.framesDone;
        if (mixer.blockFrames == AUDIO_MIX_BLOCK_FRAMES) {
            if (sink != nullptr) sink(mixer.block.data(), mixer.blockFrames);
            mixer.blockFrames = 0;
        }
    }
}

void AudioMix_SpeakerGate(int sounding) {
    mixer.speakerSounding = sounding != 0;
    mixer.updateTone();
}

void AudioMix_SpeakerDivisor(uint16_t divisor) {
    mixer.speakerDivisor = divisor;
    mixer.updateTone();
}

void AudioMix_SampleRate(uint32_t sourceRateHz) { mixer.samples.setRate(sourceRateHz); }
void AudioMix_QueueUnsigned8(uint8_t sample) { mixer.samples.queueUnsigned8(sample); }
void AudioMix_QueueBits(const uint8_t* data, uint16_t byteCount) { mixer.samples.queueBits(data, byteCount); }

void AudioMix_MidiByte(uint8_t value) { mixer.music.midiByte(value); }
void AudioMix_StartMusicVoice(uint16_t voiceId, uint8_t note, uint8_t velocity, uint8_t program, uint8_t voiceClass) {
    mixer.music.startVoice(voiceId, note, velocity, program, voiceClass);
}
void AudioMix_StopMusicVoice(uint16_t voiceId) { mixer.music.stopVoice(voiceId); }
void AudioMix_ResetMusicVoices(void) { mixer.music.resetVoices(); }

}  // extern "C"
