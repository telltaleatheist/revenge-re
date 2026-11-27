// Reconstructed original PIT and interrupt-vector utilities (segment 38AE).
// The original's playback-rate calibration here (38AE:005C-0109) is gone with
// the digital calibration that used it (375F:05D6): Replaced in the ledgers.

#include "REVENGE.h"
#include "revenge/original_data.h"

// 38AE:0010, the IRQ0 vector exchange under CLI, is Replaced: there is no
// vector table. Its callers (375F:063B, 375F:06A9) start and stop the game
// clock's sample tick instead.

// 38AE:002A: Register word16 SetPITChannel0Divisor(Register word16 ax)
// Called from:
//      ProgramDigitalPlaybackTimerRate
//      RestoreDigitalPlaybackTimer
word16 SetPITChannel0Divisor(word16 divisor)
{
	word16 previousDivisor;

	// The CLI and the OUT 43h,36h with the divisor's two bytes to port 40h
	// (38AE:002B-0039) go: there is no PIT. The game's clock takes the rate
	// from the caller. The XCHG with DS:6FA4 stays.
	previousDivisor = PITCalibration.Channel0Divisor;
	PITCalibration.Channel0Divisor = divisor;
	return previousDivisor;
}

// 38AE:0046: Register uint16 PITFrequencyToDivisor(Register uint16 frequencyHz)
// Called from:
//      ProgramDigitalPlaybackTimerRate
uint16 PITFrequencyToDivisor(uint16 frequencyHz)
{
	if (frequencyHz <= 0x12)
		return 0x00;
	return (uint16) (0x1234DEUL / frequencyHz);
}
