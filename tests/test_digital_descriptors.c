#include "REVENGE.h"
#include "revenge/digital_bridge.h"
#include "revenge/original_data.h"
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
