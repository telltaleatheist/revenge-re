// 3621: MIDI declarations
#ifndef REVENGE_AUDIO_MIDI_INTERNAL_H
#define REVENGE_AUDIO_MIDI_INTERNAL_H

#include "REVENGE.h"

typedef struct MIDIDriverHostDispatch
{
    word16 (*probe)(void);
    void (*activate)(word16 activationArgument);
    void (*deactivate)(void);
    uint16 (*filterByte)(uint16 midiByte);
    void (*startVoice)(word16 voiceRecordOffset,
        word16 channelStateOffset, word16 velocity);
    void (*releaseVoice)(word16 voiceRecordOffset);
    void (*reset)(void);
    void (*loadInstrument)(const byte *packedInstrument,
        byte *decodedInstrument);
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
