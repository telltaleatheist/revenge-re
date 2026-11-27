// 3A6A: audio data
// AudioData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/audio.h"
#include "revenge/video.h"
#include "REVENGE.h"

/* DS:62B8 initially points at the null driver descriptor in segment 3621.
 * The host materializes that segmented data pointer when it initializes the
 * DOS memory image; keeping this definition independent preserves its use by
 * data-only verification targets. */
/* The initialized words of DS:624A-6265. The
 * sequence service uses the beat and measure divisors before a track's first
 * tempo/time-signature events are dispatched. */
MIDISequenceState MIDISequence = {
    0x0078, 0x1110, 0x2D00, 0x0000,
    0x0060, 0x0004, NULL,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00, 0x00
};
/* DS:62B0-62B9, filled from the player's copy at start (original_import_pieces.txt). */
MIDIParserState MIDIParser;
uint8_t MIDIChannelMap[0x10];
MIDISystemExclusiveState MIDISystemExclusive;
SpeakerMIDIRuntimeState SpeakerMIDIRuntime;
AdLibInstrumentPatch MIDIInstrumentPatches[0x20];
RolandMPU401State RolandMPU401;
const uint16_t SpeakerMIDIFrequencyDivisors[0x72] = {
    0xFDF1, 0xEFB0, 0xE23D, 0xD58A, 0xC98E, 0xBE3E, 0xB390, 0xA97C,
    0x9FF9, 0x96FF, 0x8E85, 0x8685, 0x7EF9, 0x77D8, 0x711E, 0x6AC5,
    0x64C7, 0x5F1F, 0x59C8, 0x54BE, 0x4FFD, 0x4B7F, 0x4743, 0x4343,
    0x3F7C, 0x3BEC, 0x388F, 0x3562, 0x3263, 0x2F8F, 0x2CE4, 0x2A5F,
    0x27FE, 0x25C0, 0x23A1, 0x21A1, 0x1FBE, 0x1DF6, 0x1C48, 0x1AB1,
    0x1932, 0x17C8, 0x1672, 0x1530, 0x13FF, 0x12E0, 0x11D1, 0x10D1,
    0x0FDF, 0x0EFB, 0x0E24, 0x0D59, 0x0C99, 0x0BE4, 0x0B39, 0x0A98,
    0x0A00, 0x0970, 0x08E8, 0x0868, 0x07F0, 0x077E, 0x0712, 0x06AC,
    0x064C, 0x05F2, 0x059D, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434,
    0x03F8, 0x03BF, 0x0389, 0x0356, 0x0326, 0x02F9, 0x02CE, 0x02A6,
    0x0280, 0x025C, 0x023A, 0x021A, 0x01FC, 0x01DF, 0x01C4, 0x01AB,
    0x0193, 0x017C, 0x0167, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D,
    0x00FE, 0x00F0, 0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00A9,
    0x00A0, 0x0097, 0x008F, 0x0087, 0x007F, 0x0078, 0x0071, 0x006B,
    0x0065, 0x005F
};

/* DS:593E-5F6A, the digital sound data typed; the story's media, placed at
 * start (original_import_pieces.txt). */
DigitalSoundDataImage DigitalSoundData;
/* DS:5950, the original's sample descriptors (in the bytes above), typed. */
DigitalSampleDescriptor DigitalSamplePlaybackDescriptorTable[0x0F];

DigitalSampleDescriptor *ActiveDigitalSampleDescriptor;
DigitalDriverQueue ActiveDigitalDriverQueue;
uint16_t DigitalPlaybackRateHz;
uint8_t DigitalWeaponEffectRepeatCount;
uint8_t *DigitalWeaponSampleBuffer;
uint8_t *DigitalSoundBuffer;
uint8_t DigitalSoundDriverMutationGuard;
uint16_t NarrationMode = 0x0002; /* Original data image: full voice. */
uint16_t ActiveNarrationSubject;
uint16_t NarrationEmphasis;

/* DS:5642, the story's media, placed at start (original_import_pieces.txt). */
uint8_t PCSpeakerEffectStreamData[PC_SPEAKER_EFFECT_STREAM_BYTE_COUNT];

/* DS:58DB: each far pointer, DS:5642 plus the offset here. */
const uint8_t *const PCSpeakerEffectStreamPointerTable[PC_SPEAKER_EFFECT_COUNT] = {
    PCSpeakerEffectStreamData + 0x000, PCSpeakerEffectStreamData + 0x027,
    PCSpeakerEffectStreamData + 0x046, PCSpeakerEffectStreamData + 0x063,
    PCSpeakerEffectStreamData + 0x0A2, PCSpeakerEffectStreamData + 0x118,
    PCSpeakerEffectStreamData + 0x15B, PCSpeakerEffectStreamData + 0x16D,
    PCSpeakerEffectStreamData + 0x17C, PCSpeakerEffectStreamData + 0x187,
    PCSpeakerEffectStreamData + 0x19C, PCSpeakerEffectStreamData + 0x1B5,
    PCSpeakerEffectStreamData + 0x208, PCSpeakerEffectStreamData + 0x221,
    PCSpeakerEffectStreamData + 0x250, PCSpeakerEffectStreamData + 0x25B
};

static_assert(sizeof(PCSpeakerEffectStreamData) == 0x0299,
    "The reconstructed DS:5642-58DA speaker stream data must be exact.");
static_assert(sizeof(PCSpeakerEffectStreamPointerTable) /
            sizeof(PCSpeakerEffectStreamPointerTable[0]) ==
        0x10,
    "DS:58DB-591A holds a far pointer for each of the 10h effects.");
