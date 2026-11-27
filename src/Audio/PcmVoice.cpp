#include "PcmVoice.h"

#include "audio_mix.h"

namespace revenge::audio {

namespace {
// The 1-bit stream's two levels, as loud as the speaker's tone.
constexpr float kBitLevel = 0.25f;
}  // namespace

void PcmVoice::reset() {
    read_ = count_ = 0;
    rate_ = kDefaultRate;
    phase_ = 0;
}

void PcmVoice::setRate(uint32_t sourceRateHz) {
    if (sourceRateHz != 0) rate_ = sourceRateHz;
    phase_ = 0;
}

void PcmVoice::push(float value) {
    if (count_ == kCapacity) return;
    queue_[(read_ + count_) % kCapacity] = value;
    ++count_;
}

void PcmVoice::queueUnsigned8(uint8_t sample) {
    // Each sample fills the output frames its time spans.
    phase_ += AUDIO_MIX_SAMPLE_RATE;
    const float value = (static_cast<int>(sample) - 0x80) / 128.0f;
    while (phase_ >= rate_) {
        push(value);
        phase_ -= rate_;
    }
}

void PcmVoice::queueBits(const uint8_t* data, uint16_t byteCount) {
    if (data == nullptr) return;
    const uint32_t bytes = byteCount & 0xFFFEu;
    const uint64_t frames = static_cast<uint64_t>(bytes) * 8 * AUDIO_MIX_SAMPLE_RATE / rate_;
    for (uint64_t frame = 0; frame < frames; ++frame) {
        const uint64_t bit = frame * rate_ / AUDIO_MIX_SAMPLE_RATE;
        push((data[bit >> 3] & (1u << (bit & 7))) != 0 ? kBitLevel : -kBitLevel);
    }
}

float PcmVoice::next() {
    if (count_ == 0) return 0;
    const float value = queue_[read_];
    read_ = (read_ + 1) % kCapacity;
    --count_;
    return value;
}

}  // namespace revenge::audio
