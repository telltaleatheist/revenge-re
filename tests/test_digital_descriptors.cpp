#include "REVENGE.h"
#include "revenge/digital_bridge.h"
#include "revenge/original_data.h"
#include "fake_backend.h"

#include <assert.h>

int main(void)
{
    DigitalPlaybackDescriptor *playback;

    assert(sizeof(DigitalPlaybackDescriptor) == 0x0C);
    assert(DigitalPlaybackRuntime.InterruptNestingGuard ==
        0xFFFF);
    assert(DigitalPlaybackRuntime.RequestedMixScale ==
        0x0100);

    playback = DigitalBridge_ResolvePlaybackDescriptor(0x6EE2);
    assert(playback == &SoundBlasterPlayback);
    assert(playback->HardwareDescriptorOffset == 0x6DC2);
    assert(playback->AcquireCallbackOffset == 0x032C);
    assert(playback->ReleaseCallbackOffset == 0x032D);
    assert(playback->WriteSampleCallbackOffset == 0x032E);
    assert(playback->MixScale == 0x0070);

    assert(DigitalBridge_ResolvePlaybackDescriptor(0x6EB2) == &NullDigitalPlayback);
    assert(DigitalBridge_ResolvePlaybackDescriptor(0x6EEE) == NULL);
    assert(DigitalBridge_ResolvePlaybackDescriptor(0xFFFF) == NULL);

    FakeBackend_Reset();
    InitializeDigitalDriverQueue(
        &DigitalPlaybackBuffers.PlaybackTimerQueue);
    InstallDigitalDriverQueueCallback(&NullDigitalPlayback,
        &DigitalPlaybackBuffers.PlaybackTimerQueue);
    /* An empty queue: its first link is its end, which points back at it
	 * (375F:06D5). */
    assert(DigitalPlaybackBuffers.PlaybackTimerQueue.First ==
        &DigitalPlaybackBuffers.PlaybackTimerQueue.End);
    assert(DigitalPlaybackBuffers.PlaybackTimerQueue.End.Next == NULL);
    assert(DigitalPlaybackBuffers.PlaybackTimerQueue.End.PreviousLink ==
        &DigitalPlaybackBuffers.PlaybackTimerQueue.First);
    assert(DigitalPlaybackRuntime.ActiveDriverQueue ==
        &DigitalPlaybackBuffers.PlaybackTimerQueue);
    /* Two descriptors linked (375F:0868): each goes in front, its previous
	 * link the queue's first; unlinking (375F:08C9) mends the list either
	 * way round. */
    {
        static const byte samples[0x04] = { 0x80, 0x80, 0x80, 0x80 };
        DigitalDriverQueue queue;
        DigitalSampleDescriptor first;
        DigitalSampleDescriptor second;

        InitializeDigitalDriverQueue(&queue);
        InitializeDigitalSampleDescriptor(&first, samples, 0x0004, 0x1F40, 0x6633);
        InitializeDigitalSampleDescriptor(&second, samples, 0x0004, 0x1F40, 0x6633);
        EnqueueDigitalSampleDescriptor(&first, &queue);
        EnqueueDigitalSampleDescriptor(&first, &queue); /* already queued: no change */
        EnqueueDigitalSampleDescriptor(&second, &queue);
        assert(queue.ActiveDescriptorCount == 0x0002);
        assert(queue.MixBias == 0x0BF2);
        assert(queue.First == &second.Link);
        assert(second.Link.PreviousLink == &queue.First);
        assert(second.Link.Next == &first.Link);
        assert(first.Link.PreviousLink == &second.Link.Next);
        assert(first.Link.Next == &queue.End);
        assert(queue.End.PreviousLink == &first.Link.Next);
        assert(first.OwnerQueue == &queue && second.OwnerQueue == &queue);

        RemoveDigitalSampleDescriptor(&first);
        assert(queue.ActiveDescriptorCount == 0x0001);
        assert(queue.MixBias == 0x0AF2);
        assert(second.Link.Next == &queue.End);
        assert(queue.End.PreviousLink == &second.Link.Next);
        assert(first.OwnerQueue == NULL);
        RemoveDigitalSampleDescriptor(&first); /* not queued: no change */
        assert(queue.ActiveDescriptorCount == 0x0001);

        RemoveDigitalSampleDescriptor(&second);
        assert(queue.ActiveDescriptorCount == 0x0000);
        assert(queue.First == &queue.End);
        assert(queue.End.PreviousLink == &queue.First);
        assert(queue.End.Next == NULL);
    }

    /* The IRQ0 swap is now the game clock's sample tick, from the PIT's
	 * divisor (1193182 / 8000 = 149) until the restore. */
    PITCalibration.Channel0Divisor = 0x0400;
    InstallDigitalPlaybackTimer(0x1F40);
    assert(fakeBackend.digitalPlaybackClockDivisor == 0x0095);
    assert(DigitalPlaybackRuntime.DigitalTimerReloadMinusOne == 0x0094);
    assert(PITCalibration.Channel0Divisor == 0x0095);
    RestoreDigitalPlaybackTimer();
    assert(fakeBackend.digitalPlaybackClockDivisor == 0x0000);
    return 0x00;
}
