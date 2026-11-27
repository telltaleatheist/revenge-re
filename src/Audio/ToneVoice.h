#pragma once

namespace revenge::audio {

// One pitch at a time, as the speaker plays: a square wave made of its odd
// harmonics up to the 31st, none at or above the output's Nyquist frequency,
// so it never aliases (a tone above it is silent). It takes frequencies,
// never ports.
class ToneVoice {
public:
    void reset();
    // Plays frequency Hz from now: a new tone starts at the wave's start, a
    // change of pitch while playing keeps its place. 0 is silence.
    void play(double frequency);
    void stop();
    bool playing() const { return frequency_ > 0; }
    // The wave's mean over the next `seconds`, from -1 to 1, and its place
    // moved on by them.
    double mean(double seconds);

private:
    double frequency_ = 0;
    double phase_ = 0;  // the place in the wave, in its cycles: 0 to 1
};

}  // namespace revenge::audio
