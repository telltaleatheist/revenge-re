// 30B2: PC speaker timer
// PCSpeakerTimer.cpp
// Shared music timer and PC-speaker effect driver reconstructed from REVENGE.EXE.

#include "Audio/Audio.h"
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
//   MusicTimer owns all timer and PC-speaker stream state used by the IRQ handler.
// Seg3A6A is not this module's DS. Its selector is merely saved at 426B:0002
// while installing the handler, then restored around callbacks into the game.

// 30B2:0291
uint8_t EnablePCSpeaker(void)
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
void WritePCSpeakerPITDivisor(uint8_t divisorLowByte, uint8_t divisorHighByte)
{
    AudioLog_SpeakerDivisor(
        (uint16_t) divisorLowByte | ((uint16_t) divisorHighByte << 0x08));
    SDLBackend_WritePCSpeakerPITDivisor(
        (uint16_t) divisorLowByte | ((uint16_t) divisorHighByte << 0x08));
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
    uint16_t timerDivisor;

    MusicTimer.GameDataSegment = 0x3A6A;
    MusicTimer.SystemTickReload = MUSIC_SYSTEM_TICK_RELOAD;
    MusicTimer.SystemTickCountdown = 0x0000;
    if (MusicTimer.MusicDriverMode == MUSIC_DRIVER_MODE_1 ||
        MusicTimer.MusicDriverMode == MUSIC_DRIVER_MODE_2)
        MusicTimer.MusicServiceReload = MUSIC_SERVICE_RELOAD;
    else
        MusicTimer.MusicServiceReload = 0x0000;
    MusicTimer.MusicServiceCountdown = 0x0000;
    MusicTimer.MusicServiceBusy = 0x0000;

    if (MusicTimer.MusicDriverMode == MUSIC_DRIVER_NONE)
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
void StartPCSpeakerEffectStream(const uint8_t *effectStream)
{
    MusicTimer.PendingSpeakerEffectStream = effectStream;
    MusicTimer.SpeakerEffectStartRequested = 0x01;
    MusicTimer.SpeakerEffectPlaybackActive = 0x01;
    MusicTimer.SpeakerEffectServiceActive = 0x01;
}

// 30B2:03E1
void SetPCSpeakerEffectStopRequest(void)
{
    MusicTimer.SpeakerEffectStopRequested = 0x01;
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
static void StopPCSpeakerEffect(uint16_t streamCursor)
{
    SilencePCSpeaker();
    MusicTimer.SpeakerEffectStopRequested = 0x00;
    MusicTimer.SpeakerEffectPlaybackActive = 0x00;
    MusicTimer.SpeakerEffectServiceActive = 0x00;
    MusicTimer.SpeakerEffectStreamCursor = streamCursor;
}

// 30B2:00DF's command loop: runs the stream's commands from *cursor to the
// next tone, TRUE with its divisor in *pitDivisor and *cursor on its event;
// or FALSE at the stream's end, an unknown command or a loop jump that
// overflows, *cursor where the stream stopped.
static uint16_t FetchPCSpeakerEffectEvent(
    const uint8_t *effectStream,
    uint16_t *cursor,
    uint16_t *pitDivisor)
{
    uint16_t streamCursor = *cursor;

    while (TRUE)
    {
        uint8_t command = effectStream[streamCursor];
        if (command <= SPEAKER_EFFECT_NOTE_MAX)
        {
            *pitDivisor = MusicTimer.MusicFrequencyDivisorTable[command];
            *cursor = streamCursor;
            return TRUE;
        }

        switch (command)
        {
        case SPEAKER_EFFECT_COMMAND_LOOP1_BEGIN:
            MusicTimer.SpeakerEffectLoop1Remaining =
                effectStream[streamCursor + 0x01];
            if (MusicTimer.SpeakerEffectLoop1Remaining == 0x00)
                MusicTimer.SpeakerEffectLoop1Remaining = 0x01;
            streamCursor += 0x02;
            continue;
        case SPEAKER_EFFECT_COMMAND_LOOP1_END:
            if (--MusicTimer.SpeakerEffectLoop1Remaining != 0x00)
            {
                uint16_t loopOrigin = streamCursor + 0x01;
                uint16_t loopDistance = effectStream[streamCursor + 0x01] |
                    ((uint16_t) effectStream[streamCursor + 0x02] << 0x08);
                uint16_t loopTarget = loopOrigin - loopDistance;

                streamCursor = loopTarget;
                if (((loopOrigin ^ loopDistance) &
                        (loopOrigin ^ loopTarget) & 0x8000) != 0x00)
                {
                    *cursor = streamCursor;
                    return FALSE;
                }
            }
            else
                streamCursor += 0x03;
            continue;
        case SPEAKER_EFFECT_COMMAND_LOOP2_BEGIN:
            MusicTimer.SpeakerEffectLoop2Remaining =
                effectStream[streamCursor + 0x01];
            if (MusicTimer.SpeakerEffectLoop2Remaining == 0x00)
                MusicTimer.SpeakerEffectLoop2Remaining = 0x01;
            streamCursor += 0x02;
            continue;
        case SPEAKER_EFFECT_COMMAND_LOOP2_END:
            if (--MusicTimer.SpeakerEffectLoop2Remaining != 0x00)
            {
                uint16_t loopOrigin = streamCursor + 0x01;
                uint16_t loopDistance = effectStream[streamCursor + 0x01] |
                    ((uint16_t) effectStream[streamCursor + 0x02] << 0x08);
                uint16_t loopTarget = loopOrigin - loopDistance;

                streamCursor = loopTarget;
                if (((loopOrigin ^ loopDistance) &
                        (loopOrigin ^ loopTarget) & 0x8000) != 0x00)
                {
                    *cursor = streamCursor;
                    return FALSE;
                }
            }
            else
                streamCursor += 0x03;
            continue;
        case SPEAKER_EFFECT_COMMAND_FREQUENCY_HZ:
            *pitDivisor = (uint16_t) (PIT_INPUT_FREQUENCY_HZ /
                (effectStream[streamCursor + 0x01] |
                    ((uint16_t) effectStream[streamCursor + 0x02] << 0x08)));
            *cursor = (uint16_t) (streamCursor + 0x02);
            return TRUE;
        case SPEAKER_EFFECT_COMMAND_SET_DURATION_SCALE:
            MusicTimer.SpeakerEffectDurationScale =
                effectStream[streamCursor + 0x01];
            if (MusicTimer.SpeakerEffectDurationScale == 0x00)
                MusicTimer.SpeakerEffectDurationScale = 0x01;
            streamCursor += 0x02;
            continue;
        case SPEAKER_EFFECT_COMMAND_DECREASE_DURATION_SCALE:
        {
            uint8_t oldScale = MusicTimer.SpeakerEffectDurationScale;
            uint8_t decrease = effectStream[streamCursor + 0x01];
            uint8_t result = oldScale - decrease;
            uint8_t signedOverflow = (uint8_t) (((oldScale ^ decrease) & (oldScale ^ result) & 0x80) !=
                0x00);
            MusicTimer.SpeakerEffectDurationScale = result;
            // The original uses JNO followed by JNZ, not an unsigned borrow test.
            // Preserve that unusual behavior exactly; in practice this condition
            // cannot be true for SUB, so the byte subtraction wraps normally.
            if (signedOverflow && result == 0x00)
                MusicTimer.SpeakerEffectDurationScale = 0x01;
            streamCursor += 0x02;
            continue;
        }
        case SPEAKER_EFFECT_COMMAND_INCREASE_DURATION_SCALE:
        {
            uint8_t oldScale = MusicTimer.SpeakerEffectDurationScale;
            uint8_t increase = effectStream[streamCursor + 0x01];
            uint8_t result = oldScale + increase;
            uint8_t signedOverflow = (uint8_t) (((~(oldScale ^ increase) & (oldScale ^ result)) & 0x80) !=
                0x00);
            MusicTimer.SpeakerEffectDurationScale = signedOverflow ? 0xFF : result;
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
// Consume one PC-speaker stream tick. DS is implicit MusicTimer, not an argument.
// Verified against 30B2:00DF-0290.
void DecodePCSpeakerEffectTick(void)
{
    if (MusicTimer.SpeakerEffectStartRequested != 0x00)
    {
        MusicTimer.SpeakerEffectServiceActive = 0x01;
        MusicTimer.SpeakerEffectPlaybackActive = 0x01;
        MusicTimer.SpeakerEffectFetchNextEvent = 0x01;
        MusicTimer.SpeakerEffectStartRequested = 0x00;
        MusicTimer.SpeakerEffectStopRequested = 0x00;
        MusicTimer.ActiveSpeakerEffectStream =
            MusicTimer.PendingSpeakerEffectStream;
        MusicTimer.SpeakerEffectStreamCursor = 0x0000;
    }

    if (MusicTimer.SpeakerEffectPlaybackActive == 0x00)
        return;

    uint16_t streamCursor = MusicTimer.SpeakerEffectStreamCursor;
    if (MusicTimer.SpeakerEffectFetchNextEvent == 0x00)
    {
        --MusicTimer.SpeakerEffectTicksRemaining;
        if ((int16_t) MusicTimer.SpeakerEffectTicksRemaining < 0x00 &&
            MusicTimer.SpeakerEffectTicksRemaining != 0xFFFF)
            MusicTimer.SpeakerEffectFetchNextEvent = 0x01;
        if (MusicTimer.SpeakerEffectStopRequested != 0x00)
        {
            StopPCSpeakerEffect(streamCursor);
            return;
        }
    }
    else
    {
        const uint8_t *effectStream = MusicTimer.ActiveSpeakerEffectStream;
        uint16_t pitDivisor;

        if (FetchPCSpeakerEffectEvent(effectStream, &streamCursor, &pitDivisor) == FALSE)
        {
            StopPCSpeakerEffect(streamCursor);
            return;
        }
        if (pitDivisor != 0x0000)
        {
            WritePCSpeakerPITDivisor(
                (uint8_t) pitDivisor, (uint8_t) (pitDivisor >> 0x08));
            EnablePCSpeaker();
        }
        else
            SilencePCSpeaker();

        MusicTimer.SpeakerEffectTicksRemaining =
            MusicTimer.SpeakerEffectDurationScale *
                (uint8_t) (effectStream[streamCursor + 0x01] + 0x01) -
            0x01;
        MusicTimer.SpeakerEffectFetchNextEvent = 0x00;
        streamCursor += 0x02;
    }

    MusicTimer.SpeakerEffectStreamCursor = streamCursor;
}
