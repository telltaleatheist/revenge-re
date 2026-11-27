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

/* The speaker's and the Roland's, then the null driver's. */
enum
{
    MIDI_DRIVER_HOST_COUNT = 0x03
};
extern const void *MIDIDriverDescriptorIdentities[MIDI_DRIVER_HOST_COUNT];
extern const void *MIDIDriverHostDispatches[MIDI_DRIVER_HOST_COUNT];

#endif
