// REVENGE_30B2_PC_SPEAKER_TIMER.cpp
// Shared music timer and PC-speaker effect driver reconstructed from REVENGE.EXE.

#include "REVENGE.h"
#include "revenge/audio.h"
#include "revenge/audio_log.h"
#include "revenge/platform_backend.h"

enum
{
	MUSIC_DRIVER_NONE = 0x00,
	MUSIC_DRIVER_MODE_1 = 0x01,
	MUSIC_DRIVER_MODE_2 = 0x02,
	MUSIC_SYSTEM_TICK_RELOAD = 0x000F,
	MUSIC_SERVICE_RELOAD = 0x0003,
	MUSIC_PIT_DIVISOR_NORMAL_DRIVER = 0x1000,
	MUSIC_PIT_DIVISOR_FAST_DRIVER = 0x0400,
	MUSIC_ELAPSED_SUBTICK_MAX = 0x0123,
	MUSIC_ELAPSED_UNIT_MAX = 0x3B
};

// This module has two distinct data segments:
//   Seg30B2 owns the saved IRQ0 vector at 0004:0006 (addressed through CS).
//   Seg426B owns all timer and PC-speaker stream state used by the IRQ handler.
// Seg3A6A is not this module's DS. Its selector is merely saved at 426B:0002
// while installing the handler, then restored around callbacks into the game.

// 30B2:0291
byte EnablePCSpeaker(void)
{
	AudioLog_SpeakerGate(1);
	return SDLBackend_EnablePCSpeaker();
}

// 30B2:0298
void SilencePCSpeaker(void)
{
	AudioLog_SpeakerGate(0);
	SDLBackend_SilencePCSpeaker();
}

// 30B2:029F
void WritePCSpeakerPITDivisor(byte divisorLowByte, byte divisorHighByte)
{
	AudioLog_SpeakerDivisor(
		(uint16) divisorLowByte | ((uint16) divisorHighByte << 0x08));
	SDLBackend_WritePCSpeakerPITDivisor(
		(uint16) divisorLowByte | ((uint16) divisorHighByte << 0x08));
}

// 30B2:02A6
// Called from StartLoadedMusicTrack.
void StartPCSpeakerFixedTone(void)
{
	WritePCSpeakerPITDivisor(0x0E, 0x00);
	(void) EnablePCSpeaker();
}

// 30B2:02C1
// Called from StopLoadedMusicTrack, InstallMusicTimer and RestoreSystemTimer.
void DisablePCSpeaker(void)
{
	SilencePCSpeaker();
}

// 30B2:02D9
// Called from InitializeGame and PlayScenarioDigitalSoundSequence.
// The original saves the IRQ0 vector, installs its timer handler (30B2:0010)
// and programs the PIT's channel 0 to 1193182 / timerDivisor Hz. Host: the
// game's clock runs the handler's services from now, at the rates the divisor
// gave them (SDLBackend_StartMusicClock).
void InstallMusicTimer(void)
{
	word16 timerDivisor;

	Seg426B.GameDataSegment = 0x3A6A;
	Seg426B.SystemTickReload = MUSIC_SYSTEM_TICK_RELOAD;
	Seg426B.SystemTickCountdown = 0x0000;
	if (Seg426B.MusicDriverMode == MUSIC_DRIVER_MODE_1 ||
		Seg426B.MusicDriverMode == MUSIC_DRIVER_MODE_2)
		Seg426B.MusicServiceReload = MUSIC_SERVICE_RELOAD;
	else
		Seg426B.MusicServiceReload = 0x0000;
	Seg426B.MusicServiceCountdown = 0x0000;
	Seg426B.MusicServiceBusy = 0x0000;

	if (Seg426B.MusicDriverMode == MUSIC_DRIVER_NONE)
		timerDivisor = MUSIC_PIT_DIVISOR_NORMAL_DRIVER;
	else
		timerDivisor = MUSIC_PIT_DIVISOR_FAST_DRIVER;
	SDLBackend_StartMusicClock(timerDivisor);
	DisablePCSpeaker();
}

// 30B2:0382
// Called from ShutdownGame and PlayScenarioDigitalSoundSequence.
// The original puts the saved IRQ0 vector back and the PIT to the BIOS's
// 18.2 Hz. Host: the game's clock stops the handler's services.
void RestoreSystemTimer(void)
{
	SDLBackend_StopMusicClock();
	DisablePCSpeaker();
}

// 30B2:03B6
void StartPCSpeakerEffectStream(const byte *effectStream)
{
	Seg426B.PendingSpeakerEffectStream = effectStream;
	Seg426B.SpeakerEffectStartRequested = 0x01;
	Seg426B.SpeakerEffectPlaybackActive = 0x01;
	Seg426B.SpeakerEffectServiceActive = 0x01;
}

// 30B2:03E1
void SetPCSpeakerEffectStopRequest(void)
{
	Seg426B.SpeakerEffectStopRequested = 0x01;
}

// 30B2:0010, the IRQ0 handler InstallMusicTimer installed, is Replaced: the
// game's clock calls its services itself (src/SDL/platform.cpp), at the rates
// its countdowns gave them. Every MusicServiceReload + 1 interrupts (one
// without a music driver, four with one: 1193182 / 1000h Hz either way) it
// ran DecodePCSpeakerEffectTick and counted SystemTickCountdown, chaining the
// BIOS's handler (the BIOS tick) once in 16 of those. With a music driver it
// also ran ServiceActiveMIDIDriver on every interrupt and
// ServiceMIDISequenceTick once in 16 (MusicServiceCountdown 1 with
// SystemTickCountdown & 3 == 1), with interrupts enabled (STI at 30B2:00B5).

// The stop at 30B2:00DF's end: the speaker silenced and the effect ended, the
// stream's cursor kept where it stopped.
static void StopPCSpeakerEffect(word16 streamCursor)
{
	SilencePCSpeaker();
	Seg426B.SpeakerEffectStopRequested = 0x00;
	Seg426B.SpeakerEffectPlaybackActive = 0x00;
	Seg426B.SpeakerEffectServiceActive = 0x00;
	Seg426B.SpeakerEffectStreamCursor = streamCursor;
}

// 30B2:00DF's command loop: runs the stream's commands from *cursor to the
// next tone, TRUE with its divisor in *pitDivisor and *cursor on its event;
// or FALSE at the stream's end, an unknown command or a loop jump that
// overflows, *cursor where the stream stopped.
static word16 FetchPCSpeakerEffectEvent(
	const byte *effectStream,
	word16 *cursor,
	word16 *pitDivisor)
{
	word16 streamCursor = *cursor;

	while (TRUE)
	{
		byte command = effectStream[streamCursor];
		if (command <= SPEAKER_EFFECT_NOTE_MAX)
		{
			*pitDivisor = Seg426B.MusicFrequencyDivisorTable[command];
			*cursor = streamCursor;
			return TRUE;
		}

		switch (command)
		{
		case SPEAKER_EFFECT_COMMAND_LOOP1_BEGIN:
			Seg426B.SpeakerEffectLoop1Remaining =
				effectStream[streamCursor + 0x01];
			if (Seg426B.SpeakerEffectLoop1Remaining == 0x00)
				Seg426B.SpeakerEffectLoop1Remaining = 0x01;
			streamCursor += 0x02;
			continue;
		case SPEAKER_EFFECT_COMMAND_LOOP1_END:
			if (--Seg426B.SpeakerEffectLoop1Remaining != 0x00)
			{
				word16 loopOrigin = streamCursor + 0x01;
				word16 loopDistance = effectStream[streamCursor + 0x01]
					| ((word16)effectStream[streamCursor + 0x02] << 0x08);
				word16 loopTarget = loopOrigin - loopDistance;

				streamCursor = loopTarget;
				if (((loopOrigin ^ loopDistance)
					& (loopOrigin ^ loopTarget) & 0x8000) != 0x00)
				{
					*cursor = streamCursor;
					return FALSE;
				}
			}
			else
				streamCursor += 0x03;
			continue;
		case SPEAKER_EFFECT_COMMAND_LOOP2_BEGIN:
			Seg426B.SpeakerEffectLoop2Remaining =
				effectStream[streamCursor + 0x01];
			if (Seg426B.SpeakerEffectLoop2Remaining == 0x00)
				Seg426B.SpeakerEffectLoop2Remaining = 0x01;
			streamCursor += 0x02;
			continue;
		case SPEAKER_EFFECT_COMMAND_LOOP2_END:
			if (--Seg426B.SpeakerEffectLoop2Remaining != 0x00)
			{
				word16 loopOrigin = streamCursor + 0x01;
				word16 loopDistance = effectStream[streamCursor + 0x01]
					| ((word16)effectStream[streamCursor + 0x02] << 0x08);
				word16 loopTarget = loopOrigin - loopDistance;

				streamCursor = loopTarget;
				if (((loopOrigin ^ loopDistance)
					& (loopOrigin ^ loopTarget) & 0x8000) != 0x00)
				{
					*cursor = streamCursor;
					return FALSE;
				}
			}
			else
				streamCursor += 0x03;
			continue;
		case SPEAKER_EFFECT_COMMAND_FREQUENCY_HZ:
			*pitDivisor = (word16) (PIT_INPUT_FREQUENCY_HZ /
				(effectStream[streamCursor + 0x01]
					| ((word16)effectStream[streamCursor + 0x02] << 0x08)));
			*cursor = (word16) (streamCursor + 0x02);
			return TRUE;
		case SPEAKER_EFFECT_COMMAND_SET_DURATION_SCALE:
			Seg426B.SpeakerEffectDurationScale =
				effectStream[streamCursor + 0x01];
			if (Seg426B.SpeakerEffectDurationScale == 0x00)
				Seg426B.SpeakerEffectDurationScale = 0x01;
			streamCursor += 0x02;
			continue;
		case SPEAKER_EFFECT_COMMAND_DECREASE_DURATION_SCALE:
		{
			byte oldScale = Seg426B.SpeakerEffectDurationScale;
			byte decrease = effectStream[streamCursor + 0x01];
			byte result = oldScale - decrease;
			byte signedOverflow = (byte) (
				((oldScale ^ decrease) & (oldScale ^ result) & 0x80)
				!= 0x00);
			Seg426B.SpeakerEffectDurationScale = result;
			// The original uses JNO followed by JNZ, not an unsigned borrow test.
			// Preserve that unusual behavior exactly; in practice this condition
			// cannot be true for SUB, so the byte subtraction wraps normally.
			if (signedOverflow && result == 0x00)
				Seg426B.SpeakerEffectDurationScale = 0x01;
			streamCursor += 0x02;
			continue;
		}
		case SPEAKER_EFFECT_COMMAND_INCREASE_DURATION_SCALE:
		{
			byte oldScale = Seg426B.SpeakerEffectDurationScale;
			byte increase = effectStream[streamCursor + 0x01];
			byte result = oldScale + increase;
			byte signedOverflow = (byte) (
				((~(oldScale ^ increase) & (oldScale ^ result)) & 0x80)
				!= 0x00);
			Seg426B.SpeakerEffectDurationScale = signedOverflow ? 0xFF : result;
			streamCursor += 0x02;
			continue;
		}
		case SPEAKER_EFFECT_COMMAND_END:
			++streamCursor;
			[[fallthrough]];
		default:
			*cursor = streamCursor;
			return FALSE;
		}
	}
}

// 30B2:00DF
// Consume one PC-speaker stream tick. DS is implicit Seg426B, not an argument.
// Verified against 30B2:00DF-0290.
void DecodePCSpeakerEffectTick(void)
{
	if (Seg426B.SpeakerEffectStartRequested != 0x00)
	{
		Seg426B.SpeakerEffectServiceActive = 0x01;
		Seg426B.SpeakerEffectPlaybackActive = 0x01;
		Seg426B.SpeakerEffectFetchNextEvent = 0x01;
		Seg426B.SpeakerEffectStartRequested = 0x00;
		Seg426B.SpeakerEffectStopRequested = 0x00;
		Seg426B.ActiveSpeakerEffectStream =
			Seg426B.PendingSpeakerEffectStream;
		Seg426B.SpeakerEffectStreamCursor = 0x0000;
	}

	if (Seg426B.SpeakerEffectPlaybackActive == 0x00)
		return;

	word16 streamCursor = Seg426B.SpeakerEffectStreamCursor;
	if (Seg426B.SpeakerEffectFetchNextEvent == 0x00)
	{
		--Seg426B.SpeakerEffectTicksRemaining;
		if ((int16) Seg426B.SpeakerEffectTicksRemaining < 0x00
			&& Seg426B.SpeakerEffectTicksRemaining != 0xFFFF)
			Seg426B.SpeakerEffectFetchNextEvent = 0x01;
		if (Seg426B.SpeakerEffectStopRequested != 0x00)
		{
			StopPCSpeakerEffect(streamCursor);
			return;
		}
	}
	else
	{
		const byte *effectStream = Seg426B.ActiveSpeakerEffectStream;
		word16 pitDivisor;

		if (FetchPCSpeakerEffectEvent(effectStream, &streamCursor, &pitDivisor) == FALSE)
		{
			StopPCSpeakerEffect(streamCursor);
			return;
		}
		if (pitDivisor != 0x0000)
		{
			WritePCSpeakerPITDivisor(
				(byte) pitDivisor, (byte) (pitDivisor >> 0x08));
			EnablePCSpeaker();
		}
		else
			SilencePCSpeaker();

		Seg426B.SpeakerEffectTicksRemaining =
			Seg426B.SpeakerEffectDurationScale
			* (byte)(effectStream[streamCursor + 0x01] + 0x01) - 0x01;
		Seg426B.SpeakerEffectFetchNextEvent = 0x00;
		streamCursor += 0x02;
	}

	Seg426B.SpeakerEffectStreamCursor = streamCursor;
}
