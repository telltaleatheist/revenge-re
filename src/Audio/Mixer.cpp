// The mixer (audio_mix.h): the speaker's tone, the samples and the music,
// rendered on the game's clock into 48 kHz stereo frames.
#include "audio_mix.h"

#include "Music.h"
#include "PcmVoice.h"
#include "ToneVoice.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <string>

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
    Music music;
    bool musicOn = false;
    bool speakerSounding = false;
    uint16_t speakerDivisor = 0;
    bool started = false;
    uint64_t framesDone = 0;  // frames rendered since the clock started
    uint64_t startNanoseconds = 0;
    std::array<float, AUDIO_MIX_BLOCK_FRAMES * AUDIO_MIX_CHANNELS> block{};
    std::array<float, AUDIO_MIX_BLOCK_FRAMES * AUDIO_MIX_CHANNELS> musicBlock{};
    uint32_t blockFrames = 0;

    void updateTone() {
        if (!speakerSounding) {
            tone.stop();
            return;
        }
        tone.play(kPitClockHz / (speakerDivisor == 0 ? 65536.0 : speakerDivisor));
    }

    // Renders count frames (no more than fit the block) onto its end.
    void render(uint32_t count) {
        float* notes = musicBlock.data();
        if (musicOn)
            music.render(notes, count);
        else
            std::fill(notes, notes + count * AUDIO_MIX_CHANNELS, 0.0f);
        float* out = &block[blockFrames * AUDIO_MIX_CHANNELS];
        for (uint32_t frame = 0; frame < count; ++frame) {
            const float middle =
                kToneLevel * static_cast<float>(tone.mean(1.0 / AUDIO_MIX_SAMPLE_RATE)) + samples.next();
            out[frame * 2] = std::clamp(middle + notes[frame * 2], -1.0f, 1.0f);
            out[frame * 2 + 1] = std::clamp(middle + notes[frame * 2 + 1], -1.0f, 1.0f);
        }
        blockFrames += count;
    }
};

Mixer mixer;

}  // namespace
}  // namespace revenge::audio

using revenge::audio::mixer;

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
        const uint32_t count = static_cast<uint32_t>(
            std::min<uint64_t>(due - mixer.framesDone, AUDIO_MIX_BLOCK_FRAMES - mixer.blockFrames));
        mixer.render(count);
        mixer.framesDone += count;
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

int AudioMix_LoadMusic(const void* soundfont, size_t soundfontSize, const char* table, size_t tableSize, char* error,
                       size_t errorCapacity) {
    std::string why;
    try {
        if (mixer.music.load(soundfont, soundfontSize,
                             revenge::audio::ParseInstrumentTable(std::string_view(table, tableSize)),
                             AUDIO_MIX_SAMPLE_RATE, why))
            return 1;
    } catch (const std::exception& problem) {
        why = problem.what();
    }
    if (error != nullptr && errorCapacity > 0) std::snprintf(error, errorCapacity, "%s", why.c_str());
    return 0;
}

void AudioMix_MusicOn(int on) { mixer.musicOn = on != 0 && mixer.music.loaded(); }

void AudioMix_MidiByte(uint8_t value) {
    if (mixer.musicOn) mixer.music.midiByte(value);
}

