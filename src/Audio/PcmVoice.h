#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace revenge::audio
{

// Samples as the game hands them over, played one after another at the
// output's rate: each held for its share of the output's frames, as many as
// its source rate gives it. The queue is bounded (about 2.7 s); what doesn't
// fit is dropped.
class PcmVoice
{
  public:
    void reset();
    // The source rate of what is queued from now (0 keeps the rate); the
    // share of a frame carried over starts again.
    void setRate(uint32_t sourceRateHz);
    // An unsigned 8-bit sample (80h is silence) at the source rate.
    void queueUnsigned8(uint8_t sample);
    // The next frame, from -1 to 1; 0 when nothing is queued.
    float next();

  private:
    static constexpr std::size_t kCapacity = 0x20000;
    void push(float value);

    std::array<float, kCapacity> queue_{};
    std::size_t read_ = 0, count_ = 0;
    static constexpr uint32_t kDefaultRate = 8000;
    uint32_t rate_ = kDefaultRate;
    uint64_t phase_ = 0;
};

}  // namespace revenge::audio
