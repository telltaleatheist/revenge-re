/* REVENGE_375F_AUDIO_HARDWARE.c -- the sampled-sound output the mixer writes to. */

#include "REVENGE.h"
#include "revenge/digital_bridge.h"
#include "revenge/platform_backend.h"

/* The original drove nine outputs from here (375F:0006-04BB): a probe, an
 * acquire and a release for each card's hardware (DS:6D28-6EA3), and an
 * initialize, a release and a sample writer for each card's output
 * (DS:6EB2-6F4B). The rebuild keeps two outputs: the null one, which the
 * playback timer's empty queue is installed on (375F:063B), and the one the
 * sound setting "samples" selects (sound driver 5, 6EE2h), whose writer hands
 * each mixed byte to the host. The rest, with the probes, the hardware
 * descriptors and the ports, are Replaced in the ledgers. */

/* The two kept DOS sampled-output descriptors (DS:6EB2, DS:6EE2). */
DigitalPlaybackDescriptor NullDigitalPlayback =
	{ 0x6D28, 0x02EE, 0x02EF, 0x02F0, 0x0000, 0x0000 };
DigitalPlaybackDescriptor SoundBlasterPlayback =
	{ 0x6DC2, 0x032C, 0x032D, 0x032E, 0x0070, 0x0000 };

// 375F:02EE: the null output's initialize (RET); the host takes the rate.
void InitializeNullDigitalOutput(word16 playbackRateHz)
{
	SDLBackend_SetDigitalOutputRate(playbackRateHz);
}
// 375F:02EF: the null output's release (RET).
void ReleaseNullDigitalOutput(void) {}
// 375F:02F0: the null output's writer (RET).
void WriteNullDigitalSample(byte sample) { (void) sample; }

// 375F:032C: the samples output's initialize (RET); the host takes the rate.
void InitializeSoundBlasterDigitalOutput(word16 playbackRateHz)
{
	SDLBackend_SetDigitalOutputRate(playbackRateHz);
}
// 375F:032D: the samples output's release (RET).
void ReleaseSoundBlasterDigitalOutput(void) {}
// 375F:032E: the samples output's writer. The original sent the byte to the
// Sound Blaster's DSP as a direct-DAC command (10h, then the byte, through
// 375F:00F8 and 0100); the host plays it.
void WriteSoundBlasterDigitalSample(byte sample)
{
	SDLBackend_WriteUnsigned8Sample(sample);
}

#define ResolveDigitalPlaybackDescriptor \
	DigitalBridge_ResolvePlaybackDescriptor

// 375F:02C2: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void AcquireDigitalPlaybackDriver(word16 driverDescriptorOffset,
	uint16 playbackRateHz)
{
	DigitalPlaybackDescriptor *driver;

	driver = ResolveDigitalPlaybackDescriptor(driverDescriptorOffset);
	++driver->ReferenceCount;
	if (driver->ReferenceCount == 0x01)
	{
		switch (driver->AcquireCallbackOffset)
		{
		case 0x02EE: InitializeNullDigitalOutput(playbackRateHz); break;
		case 0x032C: InitializeSoundBlasterDigitalOutput(playbackRateHz); break;
		}
	}
}

// 375F:02D9: reconstructed C signature follows.
// Called from:
//      PlayScenarioDigitalSoundSequence
void ReleaseDigitalPlaybackDriver(word16 driverDescriptorOffset)
{
	DigitalPlaybackDescriptor *driver;

	driver = ResolveDigitalPlaybackDescriptor(driverDescriptorOffset);
	if (driver->ReferenceCount != 0x00)
	{
		word16 referenceCount = --driver->ReferenceCount;
		if (referenceCount == 0x00)
		{
			switch (driver->ReleaseCallbackOffset)
			{
			case 0x02EF: ReleaseNullDigitalOutput(); break;
			case 0x032D: ReleaseSoundBlasterDigitalOutput(); break;
			}
		}
	}
}

DigitalPlaybackRuntimeState DigitalPlaybackRuntime =
{
	.SavedSystemTimerVector = { 0x0000, 0x0000 },
	.SystemTimerDivisor = 0x0000,
	.SystemTimerAccumulator = 0x0000,
	.InterruptNestingGuard = 0xFFFF,
	.DigitalTimerReloadMinusOne = 0x0000,
	.PlaybackRateHz = 0x0000,
	.WriteSampleCallbackOffset = 0x0000,
	.ActiveDriverQueue = { 0x0000, 0x0000 },
	.RequestedMixScale = 0x0100,
	.DriverMixScale = 0x0000,
	.ActiveMixScale = 0x0000
};
