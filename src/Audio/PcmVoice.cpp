#include "PcmVoice.h"

#include "audio_mix.h"

namespace revenge::audio
{

void PcmVoice::reset()
{
    read_ = count_ = 0;
    rate_ = kDefaultRate;
    phase_ = 0;
}

void PcmVoice::setRate(uint32_t sourceRateHz)
{
    if (sourceRateHz != 0) rate_ = sourceRateHz;
    phase_ = 0;
}

void PcmVoice::push(float value)
{
    if (count_ == kCapacity) return;
    queue_[(read_ + count_) % kCapacity] = value;
    ++count_;
}

void PcmVoice::queueUnsigned8(uint8_t sample)
{
    // Each sample fills the output frames its time spans.
    phase_ += AUDIO_MIX_SAMPLE_RATE;
    const float value = (static_cast<int>(sample) - 0x80) / 128.0f;
    while (phase_ >= rate_)
    {
        push(value);
        phase_ -= rate_;
    }
}

float PcmVoice::next()
{
    if (count_ == 0) return 0;
    const float value = queue_[read_];
    read_ = (read_ + 1) % kCapacity;
    --count_;
    return value;
}

}  // namespace revenge::audio
