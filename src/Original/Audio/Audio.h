// 3014, 30AF, 30B2, 375F, 38AE: Audio declarations
#ifndef REVENGE_AUDIO_AUDIO_H
#define REVENGE_AUDIO_AUDIO_H

#include "revenge/types.h"

typedef struct DigitalSampleDescriptor DigitalSampleDescriptor;
typedef struct DigitalDriverQueue DigitalDriverQueue;

/* A queue's list (375F:0868, 08C9): each descriptor starts with its link,
 * and the queue's end is a link of its own. The original kept each as two far
 * pointers: the next descriptor, and the link pointing here (the queue's
 * first, or the previous descriptor's next). */
typedef struct DigitalSampleLink
{
    struct DigitalSampleLink *Next;
    struct DigitalSampleLink **PreviousLink;
} DigitalSampleLink;

/* A descriptor's completion (its +2C far pointer): 31FB:06E9 for a weapon
 * effect, else the driver's own 375F:0AE5, which does nothing (NULL). */
typedef void (*DigitalSampleCompletion)(DigitalSampleDescriptor *descriptor);

/* The original's descriptor is 34h bytes; each far pointer here is a
 * reference, at the offset the comment gives. */
struct DigitalSampleDescriptor
{
    DigitalSampleLink Link;                 /* +00 next, +04 previous link */
    DigitalDriverQueue *OwnerQueue;         /* +08 */
    const uint8_t *SampleData;                 /* +0C */
    uint16_t SampleLength;
    uint16_t LoopStart;
    uint16_t LoopEnd;
    uint16_t LoopLength;
    uint16_t SourceRateHz;
    uint16_t PlaybackScale;
    uint16_t SourceStepFraction;
    uint16_t SourceStepWhole;
    uint16_t PlaybackHandlerOffset;
    uint16_t SourceCursorFraction;
    /* +24/+26: the cursor's offset and segment. Here, bytes from SampleData:
	 * the original's offset began at the sample's own (normalized, under
	 * 10h), so the two wrap alike for any sample under FFF0h bytes. */
    uint16_t SourceCursor;
    uint16_t RemainingFraction;
    uint16_t RemainingWhole;
    DigitalSampleCompletion Completion;     /* +2C */
    DigitalSampleDescriptor *CompletionArgument; /* +30 */
};

/* The original's queue is 12h bytes: its first link at +00, the end link at
 * +04 (the first points there while the queue is empty). */
struct DigitalDriverQueue
{
    DigitalSampleLink *First;
    DigitalSampleLink End;
    uint16_t ActiveDescriptorCount;
    uint16_t MixBias;
    uint8_t OutputSample;
    uint8_t ReservedOutputByte;
};

/* The CS:04BC-05D5 digital playback timer/mixer state. CS:04BC, the IRQ0
 * vector 38AE:0010 saved, has no field: the rebuild never takes the vector
 * (375F:0A1D is Replaced). */
typedef struct DigitalPlaybackRuntimeState
{
    uint16_t SystemTimerDivisor;
    uint16_t SystemTimerAccumulator;
    uint16_t InterruptNestingGuard;
    uint16_t DigitalTimerReloadMinusOne;
    uint16_t PlaybackRateHz;
    uint16_t WriteSampleCallbackOffset;
    DigitalDriverQueue *ActiveDriverQueue;  /* CS:04CE */
    uint16_t RequestedMixScale;
    uint16_t DriverMixScale;
    uint16_t ActiveMixScale;
    uint8_t MixLookup[0x0100];
} DigitalPlaybackRuntimeState;

/* The DS:6F4C-6FA3 queues and calibration descriptor (the second queue at
 * DS:6F5E, the descriptor at DS:6F70). The rebuild has no calibration
 * (375F:05D6): its queue and descriptor stay as places in the original's
 * data, unused. */
typedef struct DigitalPlaybackWorkingSet
{
    DigitalDriverQueue PlaybackTimerQueue;
    DigitalDriverQueue CalibrationQueue;
    DigitalSampleDescriptor CalibrationSample;
} DigitalPlaybackWorkingSet;

#pragma pack(push, 1)
typedef struct DigitalPlaybackDescriptor
{
    uint16_t HardwareDescriptorOffset;
    uint16_t AcquireCallbackOffset;
    uint16_t ReleaseCallbackOffset;
    uint16_t WriteSampleCallbackOffset;
    uint16_t MixScale;
    uint16_t ReferenceCount;
} DigitalPlaybackDescriptor;
#pragma pack(pop)

static_assert(sizeof(DigitalPlaybackDescriptor) == 0x0C,
    "DigitalPlaybackDescriptor must retain its DOS layout");

extern DigitalPlaybackDescriptor NullDigitalPlayback;
extern DigitalPlaybackDescriptor SoundBlasterPlayback;
extern DigitalPlaybackRuntimeState DigitalPlaybackRuntime;
extern DigitalPlaybackWorkingSet DigitalPlaybackBuffers;

/* The two kept outputs' callbacks, as their descriptors name them
 * (375F:02EE-02F0 and 032C-032E). */
inline constexpr uint16_t NULL_OUTPUT_INITIALIZE = 0x02EE;
inline constexpr uint16_t NULL_OUTPUT_RELEASE = 0x02EF;
inline constexpr uint16_t NULL_OUTPUT_WRITE_SAMPLE = 0x02F0;
inline constexpr uint16_t SOUND_BLASTER_OUTPUT_INITIALIZE = 0x032C;
inline constexpr uint16_t SOUND_BLASTER_OUTPUT_RELEASE = 0x032D;
inline constexpr uint16_t SOUND_BLASTER_OUTPUT_WRITE_SAMPLE = 0x032E;

enum
{
    DIGITAL_SAMPLE_HANDLER_IDLE = 0x0A7C,
    DIGITAL_SAMPLE_HANDLER_ONESHOT = 0x0A82,
    DIGITAL_SAMPLE_HANDLER_LOOP = 0x0AAB,
    DIGITAL_DESCRIPTOR_NULL = 0x6EB2,
    DIGITAL_DESCRIPTOR_SOUND_BLASTER = 0x6EE2
};

/* Programmable-timer contracts (segment 38AE). */
uint16_t SetPITChannel0Divisor(uint16_t divisor);
uint16_t PITFrequencyToDivisor(uint16_t frequencyHz);
void RequestPCSpeakerEffectStop(void);
void AllocateMusicStagingBuffer(void);
void ReleaseMusicDriverBuffer(void);
void InitializeMusicDriver(void);
void ShutdownMusicDriver(void);
void StartLoadedMusicTrack(void);
void LoadMusicResource(uint16_t resourceIndex);
void PlayMusicResource(uint16_t resourceIndex);
void InstallMusicTimer(void);
void RestoreSystemTimer(void);
void PlayPCSpeakerEffectByIndex(uint16_t effectId);
void AcquireDigitalPlaybackDriver(
    uint16_t driverDescriptorOffset,
    uint16_t playbackRateHz);
void ReleaseDigitalPlaybackDriver(uint16_t driverDescriptorOffset);
void InstallDigitalPlaybackTimer(uint16_t playbackRateHz);
void RestoreDigitalPlaybackTimer(void);
void InitializeDigitalDriverQueue(DigitalDriverQueue *queue);
void InstallDigitalDriverQueueCallback(
    DigitalPlaybackDescriptor *playbackDescriptor,
    DigitalDriverQueue *queue);
void AttachDigitalDriverQueue(
    uint16_t driverDescriptorOffset,
    DigitalDriverQueue *queue);
void InitializeDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor,
    const uint8_t *sampleData,
    uint16_t sampleLength,
    uint16_t playbackRateHz,
    uint16_t playbackScale);
void ConfigureDigitalSampleTiming(
    DigitalSampleDescriptor *descriptor,
    uint16_t playbackScale);
void ConfigureDigitalSampleCompletion(
    DigitalSampleDescriptor *descriptor,
    DigitalSampleCompletion completion,
    DigitalSampleDescriptor *argument);
void EnqueueDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor,
    DigitalDriverQueue *queue);
void RemoveDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor);
void PrepareDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor);
uint16_t GetDigitalSamplePlaybackState(
    DigitalSampleDescriptor *descriptor);
uint32_t SetDigitalPlaybackTimingScale(uint16_t timingScale);
void ServiceDigitalAudioTick(void);

#endif
