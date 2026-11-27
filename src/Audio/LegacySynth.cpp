#include "LegacySynth.h"

#include "audio_mix.h"

#include <cmath>

namespace revenge::audio {

namespace {

constexpr uint32_t kEnvelopeMaximum = 0xFFFF;
constexpr uint32_t kEnvelopeAttackStep = 0x0100;
constexpr uint32_t kEnvelopeReleaseStep = 0x0040;
constexpr double kTwoPi = 6.28318530717958647692;

uint32_t PhaseStep(uint8_t note) {
    static constexpr uint16_t kBaseFrequencyHundredths[12] = {
        0x0331, 0x0362, 0x0395, 0x03CC, 0x0406, 0x0443, 0x0484, 0x04C9, 0x0512, 0x055F, 0x05B1, 0x0608};
    const uint64_t hundredths = static_cast<uint64_t>(kBaseFrequencyHundredths[note % 12]) << (note / 12);
    return static_cast<uint32_t>((hundredths << 32) / (AUDIO_MIX_SAMPLE_RATE * 100ULL));
}

}  // namespace

void LegacySynth::reset() {
    voices_ = {};
    status_ = dataCount_ = dataExpected_ = 0;
    filter_ = 0;
}

void LegacySynth::stopNote(uint8_t channel, uint8_t note) {
    for (Voice& voice : voices_)
        if (voice.active && voice.channel == channel && voice.note == note) voice.releasing = true;
}

void LegacySynth::startNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    stopNote(channel, note);
    std::size_t selected = 0;
    for (std::size_t index = 0; index < voices_.size(); ++index) {
        selected = index;
        if (!voices_[index].active) break;
    }
    Voice& voice = voices_[selected];
    voice.phase = 0;
    voice.phaseStep = PhaseStep(note);
    voice.noiseState = 0x9E3779B9u ^ (static_cast<uint32_t>(channel) << 16) ^ note;
    voice.envelopeLevel = 0;
    voice.channel = channel;
    voice.note = note;
    voice.velocity = velocity;
    voice.program = channel;
    voice.voiceClass = 0x20;
    voice.semanticVoice = false;
    voice.releasing = false;
    voice.active = true;
}

void LegacySynth::applyMessage() {
    const uint8_t type = status_ & 0xF0, channel = status_ & 0x0F;
    if (type == 0x80)
        stopNote(channel, data_[0]);
    else if (type == 0x90) {
        if (data_[1] == 0)
            stopNote(channel, data_[0]);
        else
            startNote(channel, data_[0], data_[1]);
    } else if (type == 0xB0 && data_[0] == 0x7B) {
        for (Voice& voice : voices_)
            if (voice.channel == channel) voice.releasing = true;
    }
}

void LegacySynth::midiByte(uint8_t value) {
    if ((value & 0x80) != 0) {
        status_ = value;
        dataCount_ = 0;
        if (value < 0xF0) {
            const uint8_t type = value & 0xF0;
            dataExpected_ = type == 0xC0 || type == 0xD0 ? 1 : 2;
        } else {
            dataExpected_ = 0;
        }
    } else if (dataExpected_ != 0) {
        data_[dataCount_++] = value;
        if (dataCount_ == dataExpected_) {
            applyMessage();
            dataCount_ = 0;
        }
    }
}

void LegacySynth::startVoice(uint16_t voiceId, uint8_t note, uint8_t velocity, uint8_t program, uint8_t voiceClass) {
    std::size_t selected = 0;
    for (std::size_t index = 0; index < voices_.size(); ++index) {
        const Voice& voice = voices_[index];
        if (voice.active && voice.semanticVoice && voice.semanticVoiceId == voiceId) {
            selected = index;
            break;
        }
        selected = index;
        if (!voice.active) break;
    }
    Voice& voice = voices_[selected];
    voice.phase = 0;
    voice.phaseStep = PhaseStep(note);
    voice.noiseState = 0x9E3779B9u ^ (static_cast<uint32_t>(voiceId) << 8) ^ note;
    voice.envelopeLevel = 0;
    voice.semanticVoiceId = voiceId;
    voice.channel = 0;
    voice.note = note;
    voice.velocity = velocity;
    voice.program = program;
    voice.voiceClass = voiceClass;
    voice.semanticVoice = true;
    voice.releasing = false;
    voice.active = true;
}

void LegacySynth::stopVoice(uint16_t voiceId) {
    for (Voice& voice : voices_)
        if (voice.semanticVoice && voice.semanticVoiceId == voiceId) voice.releasing = true;
}

void LegacySynth::resetVoices() {
    for (Voice& voice : voices_)
        if (voice.semanticVoice) {
            voice.active = false;
            voice.releasing = false;
            voice.envelopeLevel = 0;
        }
}

int32_t LegacySynth::next() {
    int32_t mixed = 0;
    for (Voice& voice : voices_) {
        if (!voice.active) continue;
        if (voice.releasing) {
            if (voice.envelopeLevel <= kEnvelopeReleaseStep) {
                voice.envelopeLevel = 0;
                voice.active = false;
                continue;
            }
            voice.envelopeLevel = static_cast<uint16_t>(voice.envelopeLevel - kEnvelopeReleaseStep);
        } else if (voice.envelopeLevel < kEnvelopeMaximum - kEnvelopeAttackStep) {
            voice.envelopeLevel = static_cast<uint16_t>(voice.envelopeLevel + kEnvelopeAttackStep);
        } else {
            voice.envelopeLevel = kEnvelopeMaximum;
        }
        int32_t wave;
        if ((voice.voiceClass & 0x20) == 0) {
            uint32_t noise = voice.noiseState;
            noise ^= noise << 13;
            noise ^= noise >> 17;
            noise ^= noise << 5;
            voice.noiseState = noise;
            wave = (static_cast<int32_t>(noise >> 24) - 0x80) * 2;
        } else {
            const double carrier = static_cast<double>(voice.phase) * kTwoPi / 4294967296.0;
            const double index = 0.15 + static_cast<double>(voice.program & 7) * 0.08;
            wave = static_cast<int32_t>(std::sin(carrier + std::sin(carrier * 2) * index) * 0x200);
        }
        mixed += static_cast<int32_t>(static_cast<int64_t>(wave) * voice.velocity * voice.envelopeLevel /
                                      (0x20LL * kEnvelopeMaximum));
        voice.phase += voice.phaseStep;
    }
    filter_ = (filter_ * 3 + mixed) / 4;
    return filter_;
}

}  // namespace revenge::audio
