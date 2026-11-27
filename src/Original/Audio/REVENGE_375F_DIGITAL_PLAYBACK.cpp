/* REVENGE_375F_DIGITAL_PLAYBACK.cpp -- digital sample queues, timer service and software mixer. */

#include "REVENGE.h"
#include "revenge/digital_bridge.h"
#include "revenge/platform_backend.h"

#include <stdbool.h>

/* Digital descriptor queues, timer service, and software mixer. */

#define ResolveDigitalPlaybackDescriptor \
    DigitalBridge_ResolvePlaybackDescriptor

void WriteNullDigitalSample(byte sample);
void WriteSoundBlasterDigitalSample(byte sample);

DigitalPlaybackWorkingSet DigitalPlaybackBuffers;
static const word16 DigitalMixBiasOffsets[0x04] = {
    0x0AF2, 0x0BF2, 0x0DF2, 0x10F2
};

void InitializeDigitalDriverQueueCore(DigitalDriverQueue *queue);
void InstallDigitalDriverQueueCallback(
    DigitalPlaybackDescriptor *playbackDescriptor,
    DigitalDriverQueue *queue);
void InitializeDigitalSampleDescriptorCore(
    DigitalSampleDescriptor *descriptor,
    word16 sampleLength,
    word16 playbackScale,
    word16 sourceRateHz,
    const byte *sampleData);
void CalculateDigitalSampleStep(
    DigitalSampleDescriptor *descriptor,
    word16 playbackScale);
word16 SetDigitalSampleLoopRange(
    DigitalSampleDescriptor *descriptor,
    word16 loopStart,
    word16 loopEnd);
void SetDigitalSampleCompletionCallback(
    DigitalSampleDescriptor *descriptor,
    DigitalSampleDescriptor *argument,
    DigitalSampleCompletion completion);
void LinkDigitalSampleDescriptor(
    DigitalDriverQueue *queue,
    DigitalSampleDescriptor *descriptor);
void UnlinkDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor);
void ResetDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor);
void FinishDigitalSampleLoopCore(
    DigitalSampleDescriptor *descriptor);
void StopDigitalSamplePlaybackCore(
    DigitalSampleDescriptor *descriptor);
word16 ReadDigitalSamplePlaybackState(
    DigitalSampleDescriptor *descriptor);
uint32 ApplyDigitalMixScale(uint16 timingScale);
uint32 RebuildDigitalMixLookup(int16 mixScale);
void ServiceDigitalAudioTick(void);
word16 ProgramDigitalPlaybackTimerRate(uint16 playbackRateHz);
void HandleDigitalWeaponEffectCompletion(
    DigitalSampleDescriptor *descriptor);
// 375F:063B: void InstallDigitalPlaybackTimer(Stack uint16 wArg04)
// Called from:
//      PlayScenarioDigitalSoundSequence
void InstallDigitalPlaybackTimer(uint16 playbackRateHz)
{
    InitializeDigitalDriverQueueCore(
        &DigitalPlaybackBuffers.PlaybackTimerQueue);
    InstallDigitalDriverQueueCallback(
        ResolveDigitalPlaybackDescriptor(DIGITAL_DESCRIPTOR_NULL),
        &DigitalPlaybackBuffers.PlaybackTimerQueue);
    /* 375F:0659-066F: CLI, then 38AE:0010 swaps the IRQ0 vector for
	 * 375F:0A1D, keeping the old one in SavedSystemTimerVector. Host: the
	 * game's clock calls the sample tick from the PIT's reprogramming on
	 * (below). */
    word16 systemTimerDivisor = ProgramDigitalPlaybackTimerRate(playbackRateHz);
    DigitalPlaybackRuntime.SystemTimerDivisor = systemTimerDivisor;
    DigitalPlaybackRuntime.SystemTimerAccumulator = systemTimerDivisor - 0x01;
    SDLBackend_StartDigitalPlaybackClock(
        DigitalPlaybackRuntime.DigitalTimerReloadMinusOne + 0x01);
}

// 375F:0689: Register word16 ProgramDigitalPlaybackTimerRate(Register uint16 ax)
// Called from:
//      InstallDigitalPlaybackTimer
word16 ProgramDigitalPlaybackTimerRate(uint16 playbackRateHz)
{
    word16 pitDivisor = PITFrequencyToDivisor(playbackRateHz);
    DigitalPlaybackRuntime.PlaybackRateHz =
        PITFrequencyToDivisor(pitDivisor);
    DigitalPlaybackRuntime.DigitalTimerReloadMinusOne =
        pitDivisor - 0x01;
    return SetPITChannel0Divisor(pitDivisor);
}

// 375F:06A9: void RestoreDigitalPlaybackTimer()
// Called from:
//      PlayScenarioDigitalSoundSequence
void RestoreDigitalPlaybackTimer(void)
{
    /* 375F:06AA-06BB: CLI, then 38AE:0010 puts SavedSystemTimerVector
	 * back. Host: the game's clock stops the sample tick. */
    SDLBackend_StopDigitalPlaybackClock();
    SetPITChannel0Divisor(
        DigitalPlaybackRuntime.SystemTimerDivisor);
}

// 375F:06C7: reconstructed C signature follows.
// Called from:
//      InitializeDigitalSoundDriver
void InitializeDigitalDriverQueue(DigitalDriverQueue *queue)
{
    InitializeDigitalDriverQueueCore(queue);
}

// 375F:06D5: reconstructed C signature follows.
// Called from:
//      InstallDigitalPlaybackTimer
//      InitializeDigitalDriverQueue
void InitializeDigitalDriverQueueCore(DigitalDriverQueue *queue)
{
    queue->First = &queue->End;
    queue->End.Next = NULL;
    queue->End.PreviousLink = &queue->First;
    queue->ActiveDescriptorCount = 0x00;
}

// 375F:06FA: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void AttachDigitalDriverQueue(word16 driverDescriptorOffset,
    DigitalDriverQueue *queue)
{
    InstallDigitalDriverQueueCallback(
        ResolveDigitalPlaybackDescriptor(driverDescriptorOffset), queue);
}

// 375F:070C: reconstructed C signature follows.
// Called from:
//      InstallDigitalPlaybackTimer
//      AttachDigitalDriverQueue
void InstallDigitalDriverQueueCallback(
    DigitalPlaybackDescriptor *playbackDescriptor,
    DigitalDriverQueue *queue)
{
    /* CLI (375F:070F) goes. */
    DigitalPlaybackRuntime.WriteSampleCallbackOffset =
        playbackDescriptor->WriteSampleCallbackOffset;
    DigitalPlaybackRuntime.ActiveDriverQueue = queue;
    DigitalPlaybackRuntime.DriverMixScale =
        playbackDescriptor->MixScale;
    ApplyDigitalMixScale(
        DigitalPlaybackRuntime.RequestedMixScale);
}

// 375F:0735: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void InitializeDigitalSampleDescriptor(DigitalSampleDescriptor *descriptor,
    const byte *sampleData,
    word16 sampleLength, word16 playbackRateHz, word16 formatScale)
{
    InitializeDigitalSampleDescriptorCore(descriptor, sampleLength, formatScale, playbackRateHz,
        sampleData);
}

// 375F:0757: reconstructed C signature follows.
// Called from:
//      InitializeDigitalSampleDescriptor
void InitializeDigitalSampleDescriptorCore(
    DigitalSampleDescriptor *descriptor,
    word16 sampleLength, word16 playbackScale, word16 sourceRateHz,
    const byte *sampleData)
{
    descriptor->SampleData = sampleData;
    descriptor->SampleLength = sampleLength;
    descriptor->SourceRateHz = sourceRateHz;
    descriptor->PlaybackScale = playbackScale;
    descriptor->OwnerQueue = NULL;
    descriptor->SourceStepWhole = 0x01;
    descriptor->SourceStepFraction = 0x00;
    /* The original passed the loop range's length (0) as the argument's
	 * high word, with a low word of 0: no argument. */
    SetDigitalSampleLoopRange(descriptor, 0x00, 0x00);
    SetDigitalSampleCompletionCallback(descriptor, NULL, NULL);
    descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_IDLE;
}

// 375F:078E: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void ConfigureDigitalSampleTiming(DigitalSampleDescriptor *descriptor,
    word16 formatScale)
{
    CalculateDigitalSampleStep(descriptor, formatScale);
}

// 375F:07A2: reconstructed C signature follows.
// Called from:
//      ConfigureDigitalSampleTiming
void CalculateDigitalSampleStep(DigitalSampleDescriptor *descriptor,
    word16 playbackScale)
{
    uint16 timerScale = DigitalPlaybackRuntime.PlaybackRateHz;
    uint32 sourceRate = descriptor->SourceRateHz;
    uint16 descriptorScale = descriptor->PlaybackScale;
    uint32 sourceRemainder = sourceRate % timerScale;
    uint32 scaleRemainder = playbackScale % descriptorScale;
    uint16 sourceFraction = (uint16) ((sourceRemainder << 0x10) / timerScale);
    uint16 scaleFraction = (uint16) ((scaleRemainder << 0x10) / descriptorScale);
    uint16 sourceWhole = (uint16) (sourceRate / timerScale);
    uint16 scaleWhole = playbackScale / descriptorScale;

    // Multiply the two unsigned 16.16 ratios exactly as the four 16-bit MULs
    // at 375F:07C9..07DE. Keeping the partial products explicit avoids a
    // fictitious 64-bit type in this 16-bit DOS source model.
    uint32 wholeProduct = (uint32) sourceWhole * scaleWhole;
    uint32 crossProducts = (uint32) sourceWhole * scaleFraction +
        (uint32) scaleWhole * sourceFraction;
    uint32 fractionalProduct = (uint32) sourceFraction * scaleFraction;
    uint32 fixedPointStep = (wholeProduct << 0x10) + crossProducts +
        (fractionalProduct >> 0x10);
    /* CLI (375F:07E4) goes. */
    descriptor->SourceStepWhole = HIGH_WORD(fixedPointStep);
    descriptor->SourceStepFraction = LOW_WORD(fixedPointStep);
}

// 375F:07ED: stack-argument wrapper around the register-call loop-range core.
void ConfigureDigitalSampleLoop(DigitalSampleDescriptor *descriptor,
    word16 loopStart, word16 loopEnd)
{
    SetDigitalSampleLoopRange(descriptor, loopStart, loopEnd);
}

// 375F:0805: reconstructed C signature follows.
// Called from:
//      InitializeDigitalSampleDescriptorCore
word16 SetDigitalSampleLoopRange(DigitalSampleDescriptor *descriptor,
    word16 loopStart, word16 loopEnd)
{
    /* CLI (375F:0806) goes. */
    descriptor->LoopStart = loopStart;
    descriptor->LoopEnd = loopEnd;
    word16 loopLength = loopEnd - loopStart;
    descriptor->LoopLength = loopLength;
    return loopLength;
}

// 375F:0832: reconstructed C signature follows.
// Called from:
//      InitializeDigitalSampleDescriptorCore
void SetDigitalSampleCompletionCallback(
    DigitalSampleDescriptor *descriptor,
    DigitalSampleDescriptor *argument,
    DigitalSampleCompletion completion)
{
    /* CLI (375F:0833) goes. No completion is the driver's own, 375F:0AE5,
	 * which returns at once: NULL here. */
    descriptor->CompletionArgument = argument;
    descriptor->Completion = completion;
}

// 375F:0814: stack-argument wrapper around the register-call callback core.
void ConfigureDigitalSampleCompletion(
    DigitalSampleDescriptor *descriptor,
    DigitalSampleCompletion completion, DigitalSampleDescriptor *argument)
{
    SetDigitalSampleCompletionCallback(descriptor, argument, completion);
}

// 375F:0852: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void EnqueueDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor,
    DigitalDriverQueue *queue)
{
    LinkDigitalSampleDescriptor(queue, descriptor);
}

// 375F:0868: reconstructed C signature follows.
// Called from:
//      EnqueueDigitalSampleDescriptor
void LinkDigitalSampleDescriptor(DigitalDriverQueue *queue,
    DigitalSampleDescriptor *descriptor)
{
    DigitalSampleLink *next;

    if (descriptor->OwnerQueue == NULL)
    {
        descriptor->OwnerQueue = queue;
        /* CLI (375F:0877) goes. */
        word16 previousCount = queue->ActiveDescriptorCount;
        queue->ActiveDescriptorCount = previousCount + 0x01;
        queue->MixBias = DigitalMixBiasOffsets[previousCount];
        /* In front of the queue's first (its end while empty). */
        next = queue->First;
        descriptor->Link.Next = next;
        descriptor->Link.PreviousLink = &queue->First;
        next->PreviousLink = &descriptor->Link.Next;
        queue->First = &descriptor->Link;
    }
}

// 375F:08B9: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void RemoveDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor)
{
    UnlinkDigitalSampleDescriptor(descriptor);
}

// 375F:08C9: reconstructed C signature follows.
// Called from:
//      RemoveDigitalSampleDescriptor
void UnlinkDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor)
{
    if (descriptor->OwnerQueue != NULL)
    {
        DigitalSampleLink *next = descriptor->Link.Next;
        DigitalSampleLink **previousLink = descriptor->Link.PreviousLink;
        DigitalDriverQueue *queue = descriptor->OwnerQueue;
        word16 activeCount;

        /* CLI (375F:08D3) goes. */
        next->PreviousLink = previousLink;
        *previousLink = next;

        activeCount = --queue->ActiveDescriptorCount;
        queue->MixBias = activeCount == 0x0000 ? 0x0AF2 : DigitalMixBiasOffsets[activeCount - 0x01];
        descriptor->OwnerQueue = NULL;
    }
}

// 375F:0917: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void PrepareDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor)
{
    ResetDigitalSamplePlayback(descriptor);
}

// 375F:0927: reconstructed C signature follows.
// Called from:
//      PrepareDigitalSamplePlayback
void ResetDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor)
{
    descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_IDLE;
    descriptor->SourceCursor = 0x0000;
    descriptor->SourceCursorFraction = 0x0000;
    descriptor->RemainingFraction = 0x0000;
    if (descriptor->LoopLength != 0x0000)
    {
        descriptor->RemainingWhole = descriptor->LoopEnd;
        descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_LOOP;
    }
    else
    {
        descriptor->RemainingWhole = descriptor->SampleLength;
        descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_ONESHOT;
    }
}

// 375F:095A: stack-argument wrapper around the register-call loop finisher.
void FinishDigitalSampleLoop(
    DigitalSampleDescriptor *descriptor)
{
    FinishDigitalSampleLoopCore(descriptor);
}

// 375F:096A: when a looping descriptor is active, finish the current pass
// through the source buffer as a one-shot rather than wrapping again.
void FinishDigitalSampleLoopCore(
    DigitalSampleDescriptor *descriptor)
{
    if (descriptor->PlaybackHandlerOffset == DIGITAL_SAMPLE_HANDLER_LOOP)
    {
        /* CLI (375F:0972) goes. */
        descriptor->RemainingWhole = descriptor->SampleLength -
            descriptor->SourceCursor;
        descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_ONESHOT;
    }
}

// 375F:0986: stack-argument wrapper around the register-call stop core.
void StopDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor)
{
    StopDigitalSamplePlaybackCore(descriptor);
}

// 375F:0993: immediately mark a descriptor idle.
void StopDigitalSamplePlaybackCore(
    DigitalSampleDescriptor *descriptor)
{
    descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_IDLE;
}

// 375F:0999: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
word16 GetDigitalSamplePlaybackState(
    DigitalSampleDescriptor *descriptor)
{
    return ReadDigitalSamplePlaybackState(descriptor);
}

// 375F:09A9: reconstructed C signature follows.
// Called from:
//      GetDigitalSamplePlaybackState
word16 ReadDigitalSamplePlaybackState(
    DigitalSampleDescriptor *descriptor)
{
    word16 result = 0x00;
    word16 handlerOffset = descriptor->PlaybackHandlerOffset;
    if (handlerOffset != DIGITAL_SAMPLE_HANDLER_IDLE)
    {
        result = 0x01;
        if (handlerOffset != DIGITAL_SAMPLE_HANDLER_LOOP)
        {
            result = 0x02;
            if (handlerOffset != DIGITAL_SAMPLE_HANDLER_ONESHOT)
                result = 0xFFFF;
        }
    }
    return result;
}

// 375F:09C6: Register word16 SetDigitalPlaybackTimingScale(Stack uint16 wArg04)
// Called from:
//      InitializeDigitalSoundDriver
//      PlayScenarioDigitalSoundSequence
uint32 SetDigitalPlaybackTimingScale(uint16 timingScale)
{
    word16 mixScale;

    DigitalPlaybackRuntime.RequestedMixScale = timingScale;
    mixScale = (word16) (((uint32) DigitalPlaybackRuntime.DriverMixScale *
                             timingScale) >>
        0x08);
    return RebuildDigitalMixLookup(mixScale);
}

// 375F:09CC: void ApplyDigitalMixScale(Register uint16 ax)
// Called from:
//      InstallDigitalDriverQueueCallback
uint32 ApplyDigitalMixScale(uint16 timingScale)
{
    word16 mixScale;

    DigitalPlaybackRuntime.RequestedMixScale = timingScale;
    mixScale = (word16) (((uint32) DigitalPlaybackRuntime.DriverMixScale *
                             timingScale) >>
        0x08);
    if (mixScale == DigitalPlaybackRuntime.ActiveMixScale)
        return 0x00000000;
    return RebuildDigitalMixLookup(mixScale);
}

// 375F:09E0: Register uint16 RebuildDigitalMixLookup(Register word16 ax)
// Called from:
//      SetDigitalPlaybackTimingScale
//      ApplyDigitalMixScale
uint32 RebuildDigitalMixLookup(int16 mixScale)
{
    DigitalPlaybackRuntime.ActiveMixScale = mixScale;
    int32 accumulator = ((int32) 0x0100 - mixScale) << 0x07;
    byte outputSample = 0x00;
    for (word16 inputSample = 0x0000; inputSample < 0x0100; ++inputSample)
    {
        int32 scaledSample = accumulator >> 0x08;
        if (scaledSample < 0x0000)
            outputSample = 0x00;
        else if (scaledSample > 0x00FF)
            outputSample = 0xFF;
        else
            outputSample = (byte) scaledSample;
        DigitalPlaybackRuntime.MixLookup[inputSample] = outputSample;
        accumulator += mixScale;
    }

    // The original exposes the final 24-bit AH:DX accumulator and last output
    // byte in DX:AX. The original's set-up (31FB:0049) took DX after a
    // zero-scale rebuild as the source rate of its silent calibration
    // descriptor; the rebuild has no calibration, and no caller reads it.
    word16 axResult = MAKE_WORD(
        (byte) (((uint32) accumulator >> 0x10) & 0x00FF),
        outputSample);
    return MAKE_DWORD(LOW_WORD(accumulator), axResult);
}


/* 375F:0A1D, the IRQ0 handler InstallDigitalPlaybackTimer installed, is
 * Replaced: it acknowledged the PIC, ran ServiceDigitalAudioTick, and chained
 * the saved vector at the BIOS's 18.2 Hz through SystemTimerAccumulator. The
 * game's clock calls ServiceDigitalAudioTick at the playback rate itself, and
 * keeps the BIOS tick on its own. */

// 375F:0A62: mix and advance one sample for every descriptor in the active
// queue. The hardware receives the byte produced on the previous tick.
void ServiceDigitalAudioTick(void)
{
    DigitalDriverQueue *queue = DigitalPlaybackRuntime.ActiveDriverQueue;
    word16 activeDescriptorCount = queue->ActiveDescriptorCount;
    if (activeDescriptorCount == 0x0000)
    {
        /* STI (375F:0AE4) goes. */
        return;
    }

    switch (DigitalPlaybackRuntime.WriteSampleCallbackOffset)
    {
    case 0x02F0: WriteNullDigitalSample(queue->OutputSample); break;
    case 0x032E: WriteSoundBlasterDigitalSample(queue->OutputSample); break;
    }
    word16 mixTableOffset = queue->MixBias;
    DigitalSampleDescriptor *descriptor =
        (DigitalSampleDescriptor *) queue->First;

    for (word16 descriptorIndex = 0x0000;
        descriptorIndex < activeDescriptorCount; ++descriptorIndex)
    {
        word16 handlerOffset = descriptor->PlaybackHandlerOffset;
        if (handlerOffset == DIGITAL_SAMPLE_HANDLER_IDLE)
            mixTableOffset += 0x0080;
        else if (handlerOffset == DIGITAL_SAMPLE_HANDLER_ONESHOT ||
            handlerOffset == DIGITAL_SAMPLE_HANDLER_LOOP)
        {
            word16 sourceOffset = descriptor->SourceCursor;
            const byte *source = descriptor->SampleData + sourceOffset;
            uint32 remaining = MAKE_DWORD(descriptor->RemainingWhole,
                descriptor->RemainingFraction);
            uint32 sourceStep = MAKE_DWORD(descriptor->SourceStepWhole,
                descriptor->SourceStepFraction);
            bool exhausted = remaining < sourceStep;
            remaining -= sourceStep;
            descriptor->RemainingWhole = HIGH_WORD(remaining);
            descriptor->RemainingFraction = LOW_WORD(remaining);

            if (exhausted)
            {
                if (handlerOffset == DIGITAL_SAMPLE_HANDLER_LOOP)
                {
                    sourceOffset -= descriptor->LoopLength;
                    descriptor->RemainingWhole += descriptor->LoopLength;
                }
                else
                {
                    descriptor->PlaybackHandlerOffset = DIGITAL_SAMPLE_HANDLER_IDLE;
                    if (descriptor->Completion != NULL)
                        descriptor->Completion(descriptor->CompletionArgument);
                }
            }

            mixTableOffset += *source;
            uint32 sourceCursor = MAKE_DWORD(sourceOffset,
                                      descriptor->SourceCursorFraction) +
                sourceStep;
            descriptor->SourceCursor = HIGH_WORD(sourceCursor);
            descriptor->SourceCursorFraction = LOW_WORD(sourceCursor);
        }
        else
        {
            // Unknown computed-jump targets are invalid descriptor state.
            return;
        }

        descriptor = (DigitalSampleDescriptor *) descriptor->Link.Next;
    }

    {
        int32 mixedSample;
        word16 sampleSum;

        sampleSum = mixTableOffset - queue->MixBias;
        mixedSample = (int32) sampleSum -
            (int32) (activeDescriptorCount - 0x01) * 0x80;
        if (mixedSample < 0x00)
            mixedSample = 0x00;
        else if (mixedSample > 0xFF)
            mixedSample = 0xFF;
        queue->OutputSample =
            DigitalPlaybackRuntime.MixLookup[(byte) mixedSample];
    }
}
