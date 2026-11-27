#ifndef REVENGE_ORIGINAL_AUDIO_DATA_H
#define REVENGE_ORIGINAL_AUDIO_DATA_H

#include "revenge/types.h"

#include <stddef.h>

/*
 * The shared timer and PC-speaker stream driver's state, segment 426B. The
 * address comments give the original's offsets.
 */
typedef struct RevengeMusicTimerState
{
    uint8_t ReservedTimerPrefix[0x02];
    uint16_t GameDataSegment;
    uint8_t MusicDriverMode;
    uint8_t SpeakerEffectServiceActive;
    uint8_t ReservedTimerControl;
    uint8_t ChainSystemTimerThisTick;
    uint16_t SystemTickReload;
    uint16_t SystemTickCountdown;
    uint16_t MusicServiceCountdown;
    uint16_t MusicServiceReload;
    uint16_t MusicServiceBusy;
    const uint8_t *ActiveSpeakerEffectStream;   /* 426B:0012 */
    uint16_t SpeakerEffectStreamCursor;        /* 426B:0016 */
    const uint8_t *PendingSpeakerEffectStream;  /* 426B:0018 */
    uint16_t ReservedTimerStreamWord;
    uint16_t SpeakerEffectTicksRemaining;
    uint16_t MusicFrequencyDivisorTable[0x60]; /* 426B:0020 */
    uint8_t SpeakerEffectPlaybackActive;
    uint8_t SpeakerEffectStartRequested;
    uint8_t SpeakerEffectStopRequested;
    uint8_t SpeakerEffectFetchNextEvent;
    uint8_t SpeakerEffectDurationScale;
    uint8_t SpeakerEffectLoop1Remaining;
    uint8_t SpeakerEffectLoop2Remaining;
    uint16_t MusicElapsedSubtick;              /* 426B:00E7 */
    uint8_t MusicElapsedUnit;
} RevengeMusicTimerState;

extern RevengeMusicTimerState MusicTimer;

enum
{
    PC_SPEAKER_EFFECT_STREAM_FIRST_OFFSET = 0x5642,
    PC_SPEAKER_EFFECT_STREAM_BYTE_COUNT = 0x0299,
    PC_SPEAKER_EFFECT_COUNT = 0x10
};

extern uint8_t PCSpeakerEffectStreamData[PC_SPEAKER_EFFECT_STREAM_BYTE_COUNT];
/* DS:58DB, each effect's stream: the original's far pointers into DS:5642. */
extern const uint8_t *const PCSpeakerEffectStreamPointerTable[PC_SPEAKER_EFFECT_COUNT];

/* Offset and segment words at DS:B60A/B60C. */
extern uint8_t *MusicStagingBuffer;

/* The standard-MIDI sequence state at DS:624A-6265. */
typedef struct MIDISequenceState
{
    uint16_t MIDIBeatsPerMinute;
    uint16_t MIDITimerCallsPerMinute;
    uint16_t MIDITicksPerMinute;
    uint16_t MIDITickRateRemainder;
    uint16_t MIDITicksPerBeat;
    uint16_t MIDIBeatsPerMeasure;
    const uint8_t *MIDIStreamCursor;  /* DS:6256 */
    uint16_t MIDIPlaybackFlags;      /* DS:625A */
    uint16_t MIDITicksUntilNextEvent;
    uint16_t MIDIBeatTickRemainder;
    uint16_t MIDIMeasureBeatRemainder;
    uint16_t MIDIMeasureCount;
    uint8_t MIDIPlaybackStateBusy;
    uint8_t MIDIRunningStatus;
} MIDISequenceState;

extern MIDISequenceState MIDISequence;

/* MIDIPlaybackFlags: a sequence is playing, paused (its notes held back), or
 * counting beats and measures as it plays. */
inline constexpr uint16_t MIDI_PLAYBACK_ACTIVE = 0x0001;
inline constexpr uint16_t MIDI_PLAYBACK_PAUSED = 0x0004;
inline constexpr uint16_t MIDI_PLAYBACK_COUNT_MEASURES = 0x0008;

/* Exact MIDI byte-parser state at DS:62B0-62B9. */
#pragma pack(push, 1)
typedef struct MIDIParserState
{
    uint16_t DispatchOffset;
    uint16_t CurrentStatus;
    uint16_t FirstDataByte;
    uint16_t ControllerValue;
    uint16_t ActiveDriverOffset;
} MIDIParserState;
#pragma pack(pop)

static_assert(sizeof(MIDIParserState) == 0x0A,
    "The reconstructed DS:62B0-62B9 MIDI parser state must be exact.");
static_assert(offsetof(MIDIParserState, ActiveDriverOffset) == 0x08,
    "The active MIDI driver offset must remain at DS:62B8.");

extern MIDIParserState MIDIParser;

/* Exact logical-to-physical channel map at DS:6728-6737. */
extern uint8_t MIDIChannelMap[0x10];

/* Exact system-exclusive command parser state at DS:6738-675F. */
#pragma pack(push, 1)
typedef struct MIDISystemExclusiveState
{
    uint8_t CommandBuffer[0x20];
    uint16_t HandlerOffset;
    uint16_t CommandOffset;
    uint8_t CommandByteCount;
    uint8_t RemainingByteCount;
    uint16_t WriteCursorOffset;
} MIDISystemExclusiveState;
#pragma pack(pop)

static_assert(sizeof(MIDISystemExclusiveState) == 0x28,
    "The original MIDI SysEx parser record occupies exactly 28h bytes.");
static_assert(offsetof(MIDISystemExclusiveState, HandlerOffset) == 0x20,
    "The SysEx handler offset must remain at DS:6758.");
static_assert(offsetof(MIDISystemExclusiveState,
                  WriteCursorOffset) == 0x26,
    "The SysEx write cursor must remain at DS:675E.");

extern MIDISystemExclusiveState MIDISystemExclusive;

/* Exact 8-byte logical MIDI channel record at DS:6420. */
#pragma pack(push, 1)
typedef struct MIDIChannelState
{
    uint8_t VoiceClass;
    uint8_t AllocationPriority;
    uint8_t ReservedVoiceCount;
    uint8_t Program;
    uint8_t NoteTransposeAndFlags;
    uint8_t AvailableVoiceCount;
    uint16_t InstrumentOffset;
} MIDIChannelState;
#pragma pack(pop)

static_assert(sizeof(MIDIChannelState) == 0x08,
    "An original MIDI channel record occupies exactly 8 bytes.");
static_assert(offsetof(MIDIChannelState, InstrumentOffset) == 0x06,
    "The instrument near pointer must remain at channel offset 0006h.");

/* Exact 20h-byte voice allocator record at DS:62C0. */
#pragma pack(push, 1)
typedef struct MIDIVoiceRecord
{
    uint16_t NextOffset;
    uint16_t PreviousOffset;
    uint8_t VoiceClass;
    uint8_t AllocationPriority;
    uint16_t ChannelStateOffset;
    uint8_t SourceNote;
    uint8_t PlaybackNote;
    union
    {
        uint8_t Raw[0x16];
        struct
        {
            uint16_t PITDivisor;
            uint8_t SpeakerDriverStatePadding[0x14];
        } Speaker;
        struct
        {
            uint16_t FrequencyLatch;
            uint8_t AttenuationLatch;
            uint8_t VoiceMask;
            uint8_t LegacyPSGDriverStatePadding[0x12];
        } LegacyPSG;
        struct
        {
            uint8_t Channel;
            uint8_t OperatorRegister;
            uint16_t LoadedPatchOffset;
            uint16_t Frequency;
            uint8_t AdLibDriverStatePadding[0x10];
        } AdLib;
    } Driver;
} MIDIVoiceRecord;
#pragma pack(pop)

static_assert(sizeof(MIDIVoiceRecord) == 0x20,
    "An original MIDI voice allocator record occupies exactly 20h bytes.");
static_assert(offsetof(MIDIVoiceRecord, ChannelStateOffset) == 0x06,
    "The channel near pointer must remain at voice offset 0006h.");
inline constexpr uint16_t MIDI_VOICE_RECORD_SIZE = (uint16_t) sizeof(MIDIVoiceRecord);
static_assert(offsetof(MIDIVoiceRecord, PlaybackNote) == 0x09,
    "The transposed note must remain at voice offset 0009h.");
static_assert(offsetof(MIDIVoiceRecord, Driver) == 0x0A,
    "Driver-private voice state must remain at voice offset 000Ah.");

/* Exact speaker MIDI scheduler state at DS:67BC-67C1. */
#pragma pack(push, 1)
typedef struct SpeakerMIDIRuntimeState
{
    uint16_t ActivePITDivisor;
    uint16_t ServiceVoiceOffset;
    uint8_t ServiceVoiceCountdown;
    uint8_t ReservedSchedulerByte;
} SpeakerMIDIRuntimeState;
#pragma pack(pop)

static_assert(sizeof(SpeakerMIDIRuntimeState) == 0x06,
    "The speaker MIDI scheduler state occupies exactly 6 bytes.");

extern SpeakerMIDIRuntimeState SpeakerMIDIRuntime;
extern const uint16_t SpeakerMIDIFrequencyDivisors[0x72];

#pragma pack(push, 1)
typedef struct PackedAdLibInstrument
{
    uint8_t Data[0x0B];
} PackedAdLibInstrument;

typedef struct AdLibOperatorPatch
{
    uint8_t Characteristics;
    uint8_t ScaleAndOutputLevel;
    uint8_t SustainAndRelease;
    uint8_t AttackAndDecay;
    uint8_t WaveformSelect;
} AdLibOperatorPatch;

typedef struct AdLibInstrumentPatch
{
    AdLibOperatorPatch Modulator;
    uint8_t FeedbackAndConnection;
    AdLibOperatorPatch Carrier;
    uint8_t ReservedInstrumentByte;
} AdLibInstrumentPatch;
#pragma pack(pop)

static_assert(sizeof(PackedAdLibInstrument) == 0x0B,
    "A packed AdLib instrument occupies exactly 0Bh bytes.");
static_assert(sizeof(AdLibInstrumentPatch) == 0x0C,
    "A decoded AdLib instrument occupies exactly 0Ch bytes.");
static_assert(offsetof(AdLibInstrumentPatch, Carrier) == 0x06,
    "The carrier patch must remain at decoded instrument offset 0006h.");

extern AdLibInstrumentPatch MIDIInstrumentPatches[0x20];

/* The MPU-401 transmit queue at DS:6C12-6D17. DS:6C0E, the IRQ 2 vector the
 * original saved, has no field: the rebuild never takes the vector
 * (3621:0EDA). */
inline constexpr uint16_t MPU401_TRANSMIT_QUEUE_SIZE = 0x0100;

#pragma pack(push, 1)
typedef struct RolandMPU401State
{
    uint8_t TransmitQueue[MPU401_TRANSMIT_QUEUE_SIZE];
    uint16_t WriteIndex;
    uint16_t ReadIndex;
    uint16_t ByteCount;
} RolandMPU401State;
#pragma pack(pop)

static_assert(offsetof(RolandMPU401State, WriteIndex) == 0x0100,
    "The MPU-401 write index follows its queue (DS:6D12).");

extern RolandMPU401State RolandMPU401;

/* Exact 1Bh MIDI driver descriptor used by the near-call dispatch layer. */
#pragma pack(push, 1)
typedef struct MIDIDriverDescriptorData
{
    uint8_t Flags[0x03];
    uint8_t AvailableVoiceCount;
    uint16_t ProbeOffset;
    uint16_t ActivateOffset;
    uint16_t DeactivateOffset;
    uint16_t FilterByteOffset;
    uint16_t StartVoiceOffset;
    uint16_t ReleaseVoiceOffset;
    uint16_t ResetOffset;
    uint16_t LoadInstrumentOffset;
    uint16_t ServiceOffset;
    uint16_t ActivationArgument;
    uint16_t VoiceRecordsOffset;
    uint8_t ActiveVoiceCount;
} MIDIDriverDescriptorData;
#pragma pack(pop)

static_assert(sizeof(MIDIDriverDescriptorData) == 0x1B,
    "An original MIDI driver descriptor occupies exactly 1Bh bytes.");
static_assert(offsetof(MIDIDriverDescriptorData, ProbeOffset) == 0x04,
    "The MIDI probe near pointer must remain at descriptor offset 0004h.");
static_assert(offsetof(MIDIDriverDescriptorData,
                  ActiveVoiceCount) == 0x1A,
    "The active voice count must remain at descriptor offset 001Ah.");

#endif
