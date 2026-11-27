#pragma once

#include <array>
#include <cstdint>

namespace revenge::audio {

// The music synth the port had: 32 voices of a sine with a little
// modulation (or noise), with a plain attack and release, for MIDI bytes as
// the MPU path sends them and for voices as the AdLib path starts them.
// Moved here as it was; the soundfont replaces it (phase 3's music slice).
class LegacySynth {
public:
    void reset();
    void midiByte(uint8_t value);
    void startVoice(uint16_t voiceId, uint8_t note, uint8_t velocity, uint8_t program, uint8_t voiceClass);
    void stopVoice(uint16_t voiceId);
    void resetVoices();
    // The next frame, as a signed 16-bit sample.
    int32_t next();

private:
    struct Voice {
        uint32_t phase = 0, phaseStep = 0, noiseState = 0;
        uint16_t semanticVoiceId = 0, envelopeLevel = 0;
        uint8_t channel = 0, note = 0, velocity = 0, program = 0, voiceClass = 0;
        bool semanticVoice = false, releasing = false, active = false;
    };

    void stopNote(uint8_t channel, uint8_t note);
    void startNote(uint8_t channel, uint8_t note, uint8_t velocity);
    void applyMessage();

    std::array<Voice, 0x20> voices_{};
    uint8_t status_ = 0, data_[2] = {}, dataCount_ = 0, dataExpected_ = 0;
    int32_t filter_ = 0;
};

}  // namespace revenge::audio
