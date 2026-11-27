// Reconstructed original PIT and interrupt-vector utilities (segment 38AE).
// The original's playback-rate calibration here (38AE:005C-0109) is gone with
// the digital calibration that used it (375F:05D6): Replaced in the ledgers.

#include "REVENGE.h"
#include "revenge/interrupts.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"

// 38AE:0010: reconstructed C signature follows.
// Called from:
//      InstallDigitalPlaybackTimer
//      RestoreDigitalPlaybackTimer
uint32 SwapInterruptVector(uint32 newVector, word16 interruptNumber)
{
	// The assembly exchanges the two words of the IVT entry atomically under
	// CLI/PUSHF/POPF. The host boundary retains the same 16:16 token exchange.
	return SDLBackend_SwapInterruptVector((byte)interruptNumber, newVector);
}

// 38AE:002A: Register word16 SetPITChannel0Divisor(Register word16 ax)
// Called from:
//      ProgramDigitalPlaybackTimerRate
//      RestoreDigitalPlaybackTimer
word16 SetPITChannel0Divisor(word16 divisor)
{
	word16 previousDivisor;

	// OUT 43h,36h followed by the low and high divisor bytes is confined to
	// the host boundary; the original XCHG with DS:6FA4 remains here.
	SDLBackend_ProgramPITChannel0(divisor);
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
