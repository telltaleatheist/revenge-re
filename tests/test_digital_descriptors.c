#include "REVENGE.h"
#include "revenge/digital_bridge.h"
#include "fake_backend.h"

#include <assert.h>

int main(void)
{
	DigitalPlaybackDescriptor *playback;

	assert(sizeof(DigitalPlaybackDescriptor) == 0x0C);
	assert(sizeof(DigitalPlaybackRuntimeState) == 0x011A);
	assert(sizeof(DigitalPlaybackWorkingSet) == 0x58);
	assert(DigitalPlaybackRuntime.InterruptNestingGuard ==
		0xFFFF);
	assert(DigitalPlaybackRuntime.RequestedMixScale ==
		0x0100);
	assert((byte *) &DigitalPlaybackBuffers.CalibrationQueue -
		(byte *) &DigitalPlaybackBuffers == 0x12);
	assert((byte *) &DigitalPlaybackBuffers.CalibrationSample -
		(byte *) &DigitalPlaybackBuffers == 0x24);

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
	DigitalPlaybackRuntime.SavedSystemTimerVector =
		DosAddress_FromValue(0xF000FFF0UL);
	DigitalPlaybackRuntime.SystemTimerDivisor = 0x1234;
	DigitalPlaybackRuntime.SystemTimerAccumulator = 0x0200;
	DigitalPlaybackRuntime.InterruptNestingGuard = 0xFFFF;
	DigitalPlaybackRuntime.DigitalTimerReloadMinusOne = 0x000F;
	ServiceDigitalPlaybackInterrupt();
	assert(fakeBackend.acknowledgedInterrupt == 0x08);
	assert(DigitalPlaybackRuntime.SystemTimerAccumulator ==
		0x01F0);
	assert(DigitalPlaybackRuntime.InterruptNestingGuard ==
		0xFFFF);
	assert(fakeBackend.chainedInterruptVector == 0x00000000UL);

	DigitalPlaybackRuntime.SystemTimerAccumulator = 0x0005;
	ServiceDigitalPlaybackInterrupt();
	assert(DigitalPlaybackRuntime.SystemTimerAccumulator ==
		0x1229);
	assert(fakeBackend.chainedInterruptVector == 0xF000FFF0UL);
	return 0x00;
}
