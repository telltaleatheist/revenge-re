// 3621: MIDI declarations
#ifndef REVENGE_AUDIO_MIDI_INTERNAL_H
#define REVENGE_AUDIO_MIDI_INTERNAL_H

#include "revenge/types.h"

typedef struct MIDIDriverHostDispatch
{
    uint16_t (*probe)(void);
    void (*activate)(uint16_t activationArgument);
    void (*deactivate)(void);
    uint16_t (*filterByte)(uint16_t midiByte);
    void (*startVoice)(uint16_t voiceRecordOffset,
        uint16_t channelStateOffset, uint16_t velocity);
    void (*releaseVoice)(uint16_t voiceRecordOffset);
    void (*reset)(void);
    void (*loadInstrument)(const uint8_t *packedInstrument,
        uint8_t *decodedInstrument);
    void (*service)(void);
} MIDIDriverHostDispatch;

/* MIDI's own numbers, as the MIDI 1.0 specification names them. */
inline constexpr uint8_t MIDI_STATUS_BIT = 0x80;
inline constexpr uint8_t MIDI_DATA_MASK = 0x7F;
inline constexpr uint8_t MIDI_MESSAGE_TYPE_MASK = 0xF0;
inline constexpr uint8_t MIDI_CHANNEL_MASK = 0x0F;
inline constexpr int MIDI_CHANNEL_COUNT = 0x10;
inline constexpr uint8_t MIDI_NOTE_OFF = 0x80;
inline constexpr uint8_t MIDI_NOTE_ON = 0x90;
inline constexpr uint8_t MIDI_POLYPHONIC_KEY_PRESSURE = 0xA0;
inline constexpr uint8_t MIDI_CONTROL_CHANGE = 0xB0;
inline constexpr uint8_t MIDI_PROGRAM_CHANGE = 0xC0;
inline constexpr uint8_t MIDI_CHANNEL_PRESSURE = 0xD0;
inline constexpr uint8_t MIDI_SYSTEM_EXCLUSIVE = 0xF0;
inline constexpr uint8_t MIDI_TIME_CODE_QUARTER_FRAME = 0xF1;
inline constexpr uint8_t MIDI_SONG_POSITION = 0xF2;
inline constexpr uint8_t MIDI_SONG_SELECT = 0xF3;
inline constexpr uint8_t MIDI_TUNE_REQUEST = 0xF6;
inline constexpr uint8_t MIDI_END_OF_EXCLUSIVE = 0xF7;
inline constexpr uint8_t MIDI_TIMING_CLOCK = 0xF8; /* the first real-time status */
inline constexpr uint8_t MIDI_START = 0xFA;
inline constexpr uint8_t MIDI_CONTINUE = 0xFB;
inline constexpr uint8_t MIDI_STOP = 0xFC;
inline constexpr uint8_t MIDI_ACTIVE_SENSING = 0xFE;
inline constexpr uint8_t MIDI_SYSTEM_RESET = 0xFF; /* in a file, a meta event */
inline constexpr uint8_t MIDI_CONTROLLER_ALL_NOTES_OFF = 0x7B;

/* The speaker's and the Roland's, then the null driver's. */
enum
{
    MIDI_DRIVER_HOST_COUNT = 0x03
};
extern const void *MIDIDriverDescriptorIdentities[MIDI_DRIVER_HOST_COUNT];
extern const void *MIDIDriverHostDispatches[MIDI_DRIVER_HOST_COUNT];

#endif
