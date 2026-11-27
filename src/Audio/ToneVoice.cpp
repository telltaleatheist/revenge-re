#include "ToneVoice.h"

#include "audio_mix.h"

#include <cmath>

namespace revenge::audio {

void ToneVoice::reset() {
    frequency_ = 0;
    phase_ = 0;
}

void ToneVoice::play(double frequency) {
    if (!(frequency > 0)) {
        stop();
        return;
    }
    if (!playing()) phase_ = 0;
    frequency_ = frequency;
}

void ToneVoice::stop() {
    frequency_ = 0;
    phase_ = 0;
}

double ToneVoice::mean(double seconds) {
    if (!playing() || !(seconds > 0)) return 0;
    constexpr double pi = 3.14159265358979323846;
    const double cycles = seconds * frequency_;
    // The square (+1 for the first half of each cycle, -1 for the second) as
    // its odd harmonics, 4/(pi n) sin(2 pi n x), integrated over the span in
    // cycles of the wave and divided by it.
    double integral = 0;
    for (unsigned harmonic = 1;
         harmonic <= 31 && harmonic * frequency_ < AUDIO_MIX_SAMPLE_RATE / 2.0; harmonic += 2) {
        const double angular = 2 * pi * harmonic;
        const double amplitude = 4 / (pi * harmonic);
        integral += amplitude * (std::cos(angular * phase_) - std::cos(angular * (phase_ + cycles))) / angular;
    }
    phase_ = std::fmod(phase_ + cycles, 1.0);
    return integral / cycles;
}

}  // namespace revenge::audio
