// 3621: MIDI sequence
// MIDISequence.cpp
// Standard MIDI sequence parser and selectable music-driver dispatch layer.

#include "revenge/audio.h"
#include "revenge/audio_log.h"
#include "revenge/debug.h"
#include "revenge/host_dispatch.h"
#include "revenge/midi_data.h"
#include "revenge/platform_backend.h"
#include "MIDIInternal.h"

#define MIDI_DRIVER_DISPATCH(driverDescriptor) \
    ((const MIDIDriverHostDispatch *) HostDispatch_Select( \
        (driverDescriptor), MIDIDriverDescriptorIdentities, \
        MIDIDriverHostDispatches, MIDI_DRIVER_HOST_COUNT - 0x01, \
        MIDIDriverHostDispatches[MIDI_DRIVER_HOST_COUNT - 0x01]))

/*
 * DS:62B8 stores a 16-bit near pointer to a driver descriptor. Native C
 * callbacks cannot occupy those DOS words, so this host-only companion holds
 * the callable descriptor while MIDIParser retains the original value.
 */
static MIDIDriverDescriptorData *ActiveMIDIDriverHost =
    &NullMIDIDriverDescriptor;
static uint8_t tracedMIDIEventCount;

// 3621:0035: void SetMIDITimingDivisor(uint16 timerCallsPerMinute)
void SetMIDITimingDivisor(uint16_t timerCallsPerMinute)
{
    if (timerCallsPerMinute != 0x0000)
    {
        /* CLI/STI around the pair (3621:003F, 0048) go. */
        MIDISequence.MIDITimerCallsPerMinute = timerCallsPerMinute;
        MIDISequence.MIDITickRateRemainder = 0x0000;
    }
}

// 3621:004B: void ConfigureMIDITempo(
//     uint16 ticksPerQuarterNote, uint16 beatsPerMinute)
void ConfigureMIDITempo(uint16_t ticksPerQuarterNote, uint16_t beatsPerMinute)
{
    uint16_t ticksPerMinute;

    // V: MIDI meta event FF 51 supplies microseconds per quarter note. The
    // caller converts that value to BPM, then this routine converts BPM and
    // the stream's 60h ticks-per-quarter-note division into ticks per minute.
    MIDISequence.MIDIBeatsPerMinute = beatsPerMinute;
    ticksPerMinute = (uint16_t) (ticksPerQuarterNote * beatsPerMinute);
    /* CLI/STI around the pair (3621:0057, 0060) go. */
    MIDISequence.MIDITicksPerMinute = ticksPerMinute;
    MIDISequence.MIDITickRateRemainder = 0x0000;
}

// 3621:0063: void ConfigureMIDITimeSignature(
//     uint16 numerator, uint16 denominator, uint16 beatsPerMeasure)
void ConfigureMIDITimeSignature(
    uint16_t numerator,
    uint16_t denominator,
    uint16_t beatsPerMeasure)
{
    uint16_t beatDivisor;
    uint32_t scaledNumerator;

    // V: Called for MIDI meta event FF 58. With the game's fixed 60h PPQN
    // stream division, 180h is four quarter notes. The resulting divisors
    // convert elapsed MIDI ticks first into beats and then into measures.
    MIDISequence.MIDIBeatsPerMeasure = beatsPerMeasure;
    beatDivisor = (uint16_t) (denominator * beatsPerMeasure);
    scaledNumerator = (uint32_t) numerator * 0x0180UL;
    MIDISequence.MIDITicksPerBeat =
        (uint16_t) (scaledNumerator / beatDivisor);
}

// 3621:007E: void SendMIDIAllNotesOff(void)
void SendMIDIAllNotesOff(void)
{
    int16_t channel;

    for (channel = 0x000F; channel >= 0x0000; --channel)
    {
        DispatchMIDIByte((uint16_t) (0x00B0 | channel));
        DispatchMIDIByte(0x007B);
        DispatchMIDIByte(0x0000);
    }
}

// 3621:00AA: uint16 ReadMIDIVariableLengthQuantity(
//     const byte **cursor)
uint16_t ReadMIDIVariableLengthQuantity(const uint8_t **cursor)
{
    uint16_t value;
    uint8_t encodedByte;

    value = 0x0000;
    do
    {
        encodedByte = **cursor;
        ++(*cursor);
        value = (uint16_t) ((value << 0x07) | (encodedByte & 0x7F));
    } while ((encodedByte & 0x80) != 0x00);
    return value;
}

// 3621:00C1: uint16 StartMIDISequence(
//     const byte *midiData, uint16 playbackFlags)
// Verified against 3621:00C1-0159.
uint16_t StartMIDISequence(const uint8_t *midiData, uint16_t playbackFlags)
{
    uint16_t headerLength;
    const uint8_t *track;
    const uint8_t *cursor;

    if ((playbackFlags & 0x01) == 0x00)
        return FALSE;
    if (midiData[0x00] != 'M' || midiData[0x01] != 'T' ||
        midiData[0x02] != 'h' || midiData[0x03] != 'd')
        return FALSE;

    headerLength = ((uint16_t) midiData[0x06] << 0x08) | midiData[0x07];
    if ((((uint16_t) midiData[0x08] << 0x08) | midiData[0x09]) != 0x0000 ||
        (((uint16_t) midiData[0x0A] << 0x08) | midiData[0x0B]) != 0x0001 ||
        (((uint16_t) midiData[0x0C] << 0x08) | midiData[0x0D]) != 0x0060)
        return FALSE;

    track = midiData + 0x08 + headerLength;
    if (track[0x00] != 'M' || track[0x01] != 'T' ||
        track[0x02] != 'r' || track[0x03] != 'k')
        return FALSE;

    MIDISequence.MIDIPlaybackStateBusy = TRUE;
    if ((MIDISequence.MIDIPlaybackFlags & 0x01) != 0x00)
        SendMIDIAllNotesOff();
    MIDISequence.MIDIPlaybackFlags = 0x0000;

    cursor = track + 0x08;
    MIDISequence.MIDITicksUntilNextEvent =
        ReadMIDIVariableLengthQuantity(&cursor);
    MIDISequence.MIDIStreamCursor = cursor;
    MIDISequence.MIDIPlaybackFlags = playbackFlags;
    MIDISequence.MIDIBeatTickRemainder = 0x0000;
    MIDISequence.MIDIMeasureBeatRemainder = 0x0000;
    MIDISequence.MIDIMeasureCount = 0x0000;
    MIDISequence.MIDIPlaybackStateBusy = FALSE;
    return playbackFlags;
}

// 3621:0193: void PauseMIDISequence(void)
void PauseMIDISequence(void)
{
    if ((MIDISequence.MIDIPlaybackFlags & 0x05) == 0x01)
    {
        MIDISequence.MIDIPlaybackStateBusy = TRUE;
        MIDISequence.MIDIPlaybackFlags |= 0x04;
        SendMIDIAllNotesOff();
        MIDISequence.MIDIPlaybackStateBusy = FALSE;
    }
}

// 3621:01B1: void ResumeMIDISequence(void)
void ResumeMIDISequence(void)
{
    if ((MIDISequence.MIDIPlaybackFlags & 0x05) == 0x05)
    {
        MIDISequence.MIDIPlaybackStateBusy = TRUE;
        MIDISequence.MIDIPlaybackFlags &= (uint16_t) ~0x04;
        MIDISequence.MIDIPlaybackStateBusy = FALSE;
    }
}

// 3621:01CC: void StopMIDISequence(void)
void StopMIDISequence(void)
{
    if ((MIDISequence.MIDIPlaybackFlags & 0x01) != 0x00)
    {
        MIDISequence.MIDIPlaybackStateBusy = TRUE;
        MIDISequence.MIDIPlaybackFlags = 0x0000;
        SendMIDIAllNotesOff();
        MIDISequence.MIDIPlaybackStateBusy = FALSE;
    }
}

// 3621:01E8: void ServiceMIDISequenceTick(void)
// The music timer's tick (30B2:0010, now the game's clock) with DS restored
// to Seg3A6A. Verified against 3621:01E8-0387; its leading STI goes.
void ServiceMIDISequenceTick(void)
{
    if ((MIDISequence.MIDIPlaybackFlags & 0x03) != 0x01)
        return;

    uint16_t timingAccumulator = (uint16_t) (MIDISequence.MIDITickRateRemainder +
        MIDISequence.MIDITicksPerMinute);
    uint16_t elapsedTicks =
        (uint16_t) (timingAccumulator /
            MIDISequence.MIDITimerCallsPerMinute);
    MIDISequence.MIDITickRateRemainder =
        (uint16_t) (timingAccumulator %
            MIDISequence.MIDITimerCallsPerMinute);
    MIDISequence.MIDITicksUntilNextEvent += elapsedTicks;

    if ((MIDISequence.MIDIPlaybackFlags & 0x08) != 0x00)
    {
        uint16_t firstAccumulator = (uint16_t) (elapsedTicks + MIDISequence.MIDIBeatTickRemainder);
        uint16_t elapsedBeats =
            (uint16_t) (firstAccumulator /
                MIDISequence.MIDITicksPerBeat);
        MIDISequence.MIDIBeatTickRemainder =
            (uint16_t) (firstAccumulator %
                MIDISequence.MIDITicksPerBeat);
        uint16_t measureAccumulator = (uint16_t) (elapsedBeats + MIDISequence.MIDIMeasureBeatRemainder);
        MIDISequence.MIDIMeasureCount +=
            (uint16_t) (measureAccumulator /
                MIDISequence.MIDIBeatsPerMeasure);
        MIDISequence.MIDIMeasureBeatRemainder =
            (uint16_t) (measureAccumulator %
                MIDISequence.MIDIBeatsPerMeasure);
    }

    if (MIDISequence.MIDIPlaybackStateBusy == TRUE)
        return;

    const uint8_t *cursor = MIDISequence.MIDIStreamCursor;
    while ((int16_t) MIDISequence.MIDITicksUntilNextEvent >= 0x00)
    {
        uint8_t status = *cursor++;
        if (tracedMIDIEventCount < 0x20)
        {
            REVENGE_PRESERVATION_TRACE("midi: event status=%02X ticks=%d flags=%04X\n",
                status, (int16_t) MIDISequence.MIDITicksUntilNextEvent,
                MIDISequence.MIDIPlaybackFlags);
            ++tracedMIDIEventCount;
        }
        if (status < 0x80)
        {
            status = MIDISequence.MIDIRunningStatus;
            --cursor;
        }

        if (status < 0xF0)
        {
            MIDISequence.MIDIRunningStatus = status;
            uint8_t messageType = status & 0xF0;
            uint8_t dataByteCount =
                (messageType == 0xC0 || messageType == 0xD0) ? 0x01 : 0x02;

            // The DS:6266 dispatch table sends Note Off, Note On, and
            // Polyphonic Key Pressure to 034Bh, which suppresses those
            // messages while playback flag 04h is set. Other channel messages
            // continue to reach the driver while paused.
            uint8_t suppressWhilePaused =
                messageType == 0x00 || messageType == 0x80 ||
                messageType == 0x90 || messageType == 0xA0;
            if (suppressWhilePaused &&
                (MIDISequence.MIDIPlaybackFlags & 0x04) != 0x00)
            {
                cursor += dataByteCount;
            }
            else
            {
                DispatchMIDIByte(status);
                for (uint8_t index = 0x00; index < dataByteCount; ++index)
                    DispatchMIDIByte(*cursor++);
            }
        }
        else if (status == 0xFF)
        {
            MIDISequence.MIDIRunningStatus = 0x00;
            uint8_t metaType = *cursor++;
            uint16_t metaLength = ReadMIDIVariableLengthQuantity(&cursor);
            const uint8_t *nextEvent = cursor + metaLength;

            switch (metaType)
            {
            case 0x01:
                // This engine repurposes text-event type 01h as a 16-byte
                // logical-to-physical MIDI channel map.
                for (uint8_t channel = 0x00; channel < 0x10; ++channel)
                    MIDIChannelMap[channel] =
                        *cursor++ & 0x0F;
                break;

            case 0x2F:
                StopMIDISequence();
                MIDISequence.MIDIStreamCursor = cursor;
                return;

            case 0x51:
            {
                uint32_t microsecondsPerQuarterNote =
                    ((uint32_t) cursor[0x00] << 0x10) |
                    ((uint32_t) cursor[0x01] << 0x08) |
                    cursor[0x02];
                uint16_t beatsPerMinute = (uint16_t) DivideUnsigned32(
                    0x03938700UL,
                    microsecondsPerQuarterNote);
                ConfigureMIDITempo(0x60, beatsPerMinute);
                break;
            }

            case 0x58:
            {
                uint8_t numeratorByte = cursor[0x00];
                uint8_t exponent = cursor[0x01];
                uint8_t clocksByte = cursor[0x02];
                uint8_t denominatorByte = exponent < 0x08 ? (uint8_t) (0x01 << exponent) : 0x00;
                uint16_t divisor = (uint16_t) (denominatorByte * clocksByte);
                uint16_t dividend = (uint16_t) (0x60 * numeratorByte);
                uint16_t beatsPerMeasure = dividend / divisor;
                uint16_t divisionRemainder = dividend % divisor;
                uint16_t retainedHighByte = divisionRemainder & 0xFF00;
                ConfigureMIDITimeSignature(
                    retainedHighByte | numeratorByte,
                    retainedHighByte | denominatorByte,
                    beatsPerMeasure);
                break;
            }
            }

            cursor = nextEvent;
        }
        else
        {
            // DS:6274 maps system statuses onto the in-function handlers at
            // 0285h, 0290h, 0341h, 0358h, 0369h, and 037Ah.
            if (status < 0xF8)
                MIDISequence.MIDIRunningStatus = 0x00;

            switch (status)
            {
            case 0xF0:
                // F0h dispatches its status before processing the same
                // VLQ-length payload form used by F7h continuation events.
                DispatchMIDIByte(status);
                /* fall through */
            case 0xF7:
            {
                uint16_t payloadLength = ReadMIDIVariableLengthQuantity(&cursor);
                do
                {
                    DispatchMIDIByte(*cursor++);
                    --payloadLength;
                } while (payloadLength != 0x00);
                break;
            }

            case 0xF1:
            case 0xF3:
                DispatchMIDIByte(status);
                DispatchMIDIByte(*cursor++);
                break;

            case 0xF2:
                DispatchMIDIByte(status);
                DispatchMIDIByte(*cursor++);
                DispatchMIDIByte(*cursor++);
                break;

            case 0xF6:
            case 0xF8:
            case 0xFA:
            case 0xFB:
            case 0xFC:
            case 0xFE:
                DispatchMIDIByte(status);
                break;

            case 0xF4:
            case 0xF5:
            case 0xF9:
            case 0xFD:
                break;
            }
        }

        MIDISequence.MIDITicksUntilNextEvent -=
            ReadMIDIVariableLengthQuantity(&cursor);
    }

    MIDISequence.MIDIStreamCursor = cursor;
}

// 3621:0388: uint32 DivideUnsigned32(uint32 dividend, uint32 divisor)
uint32_t DivideUnsigned32(uint32_t dividend, uint32_t divisor)
{
    if (dividend == 0x00000000UL ||
        divisor == 0x00000000UL ||
        divisor > dividend)
    {
        return 0x00000000UL;
    }
    return dividend / divisor;
}

// 3621:03EE: void DispatchMIDIByte(uint16 midiByte)
void DispatchMIDIByte(uint16_t midiByte)
{
    uint16_t parserOffset;
    uint8_t value;

    value = (uint8_t) midiByte;
    AudioLog_MidiByte(value);
    if (MIDIDriverHandlesByte(midiByte) != FALSE)
        return;

    if (value < 0x80)
    {
        parserOffset = MIDIParser.DispatchOffset;
        switch (parserOffset)
        {
        case 0x00:
        case 0x04:
        case 0x08:
        case 0x0C:
        case 0x18:
            /* 3621:043E stores AX at DS:62B4 and advances to the
			 * corresponding second-data-byte state. */
            MIDIParser.FirstDataByte = midiByte;
            MIDIParser.DispatchOffset =
                (uint16_t) (parserOffset + 0x02);
            break;

        case 0x02:
            ReleaseMIDINote(
                MIDIParser.CurrentStatus,
                MIDIParser.FirstDataByte);
            break;

        case 0x06:
            if (value == 0x00)
            {
                ReleaseMIDINote(
                    MIDIParser.CurrentStatus,
                    MIDIParser.FirstDataByte);
            }
            else
            {
                StartMIDINote(
                    MIDIParser.CurrentStatus,
                    MIDIParser.FirstDataByte,
                    midiByte);
            }
            break;

        case 0x0A:
            IgnoreMIDIPolyphonicKeyPressure(
                MIDIParser.CurrentStatus,
                MIDIParser.FirstDataByte,
                midiByte);
            break;

        case 0x0E:
            if (MIDIParser.FirstDataByte == 0x0001)
                midiByte = (uint16_t) ((uint16_t) value << 0x07);
            else if (MIDIParser.FirstDataByte != 0x0021)
            {
                if (MIDIParser.FirstDataByte >= 0x007B)
                    ReleaseAllMIDIVoices();
                break;
            }
            midiByte ^= MIDIParser.ControllerValue;
            midiByte &= 0x007F;
            midiByte ^= MIDIParser.ControllerValue;
            MIDIParser.ControllerValue = midiByte;
            IgnoreMIDIControllerChange(
                MIDIParser.CurrentStatus,
                midiByte);
            break;

        case 0x10:
            SetMIDIChannelProgram(
                MIDIParser.CurrentStatus,
                midiByte);
            break;

        case 0x12:
            break;

        case 0x14:
            IgnoreMIDIChannelPressure(
                MIDIParser.CurrentStatus,
                midiByte);
            break;

        case 0x16:
            DispatchMIDISystemByte(midiByte);
            MIDIParser.DispatchOffset = 0x0016;
            break;

        case 0x1A:
            midiByte = (uint16_t) (((uint16_t) value << 0x07) |
                MIDIParser.FirstDataByte);
            IgnoreMIDIPitchBend(
                MIDIParser.CurrentStatus,
                midiByte);
            break;
        }
        return;
    }

    if (value >= 0xF0)
    {
        if (value == 0xF0)
        {
            DispatchMIDISystemByte(midiByte);
            parserOffset = 0x16;
        }
        else if (value == 0xF7)
        {
            DispatchMIDISystemByte(midiByte);
            parserOffset = 0x12;
        }
        else if (value == 0xFF)
        {
            ResetMIDIPlaybackState();
            parserOffset = 0x12;
        }
        else
        {
            if (value >= 0xF8)
                return;
            parserOffset = 0x12;
        }
    }
    else
    {
        MIDIParser.CurrentStatus =
            (uint16_t) ((midiByte & 0xFF00) | (value & 0x0F));
        parserOffset = (uint16_t) ((midiByte & 0xFF00) |
            (value & 0x70));
        parserOffset >>= 0x02;
    }
    MIDIParser.DispatchOffset = parserOffset;
}

// 3621:0514: void PrepareMIDIDriver(MIDIDriverDescriptorData *driverDescriptor)
void PrepareMIDIDriver(MIDIDriverDescriptorData *driverDescriptor)
{
    (void) MIDI_DRIVER_DISPATCH(driverDescriptor)->probe();
}

// 3621:051E: void ActivateMIDIDriver(MIDIDriverDescriptorData *driverDescriptor)
void ActivateMIDIDriver(MIDIDriverDescriptorData *driverDescriptor)
{
    MIDIDriverDescriptorData *previousDriver;

    previousDriver = ActiveMIDIDriverHost;
    MIDI_DRIVER_DISPATCH(previousDriver)->deactivate();
    ActiveMIDIDriverHost = driverDescriptor;
    /* The original's Tandy, PS/1 and AdLib descriptors (68A6h, 6974h,
	 * 6A3Ch) are Replaced with their drivers. */
    if (driverDescriptor == &SpeakerMIDIDriverDescriptor)
        MIDIParser.ActiveDriverOffset = 0x678E;
    else if (driverDescriptor == &RolandMIDIDriverDescriptor)
        MIDIParser.ActiveDriverOffset = 0x6BD8;
    else
        MIDIParser.ActiveDriverOffset = 0x6766;
    MIDI_DRIVER_DISPATCH(driverDescriptor)->activate(driverDescriptor->ActivationArgument);
    driverDescriptor->ActiveVoiceCount =
        driverDescriptor->AvailableVoiceCount;
    ResetMIDIPlaybackState();
}

// 3621:0542: void DeactivateMIDIDriver(void)
void DeactivateMIDIDriver(void)
{
    MIDIDriverDescriptorData *driverDescriptor;

    driverDescriptor = ActiveMIDIDriverHost;
    MIDI_DRIVER_DISPATCH(driverDescriptor)->deactivate();
    ActiveMIDIDriverHost =
        &NullMIDIDriverDescriptor;
    MIDIParser.ActiveDriverOffset = 0x6766;
}

// 3621:0550: uint16 MIDIDriverHandlesByte(uint16 midiByte)
uint16_t MIDIDriverHandlesByte(uint16_t midiByte)
{
    return MIDI_DRIVER_DISPATCH(ActiveMIDIDriverHost)->filterByte(midiByte);
}

// 3621:055E: uint16 ReleaseMIDINote(uint16 channel, uint16 note)
// Verified against 3621:055E-05E6.
uint16_t ReleaseMIDINote(uint16_t channel, uint16_t note)
{
    MIDIChannelState *channelState;
    MIDIVoiceRecord *voiceRecord;
    uint16_t channelStateOffset;
    uint16_t voiceOffset;
    uint16_t nextVoiceOffset;
    uint16_t previousVoiceOffset;
    uint8_t channelFlags;

    channelStateOffset = (uint16_t) (0x6420 + (channel << 0x03));
    channelState = MIDIChannelStateAt(channelStateOffset);
    channelFlags = channelState->VoiceClass;
    if (channelFlags == 0x00)
        return FALSE;

    if ((channelFlags & 0x40) != 0x00)
    {
        uint8_t mappedNote;

        mappedNote = (uint8_t) note - channelState->NoteTransposeAndFlags;
        if (mappedNote >= 0x20)
            return FALSE;
        mappedNote += 0x10;
        channelStateOffset = (uint16_t) (0x6420 + (mappedNote << 0x03));
        channelState = MIDIChannelStateAt(channelStateOffset);
        note = 0x0000;
    }

    voiceOffset = 0x62BA;
    for (;;)
    {
        voiceOffset = ReadMIDIDataWord(voiceOffset);
        if (ReadMIDIDataWord(voiceOffset) == 0x0000)
            return FALSE;
        voiceRecord = MIDIVoiceRecordAt(voiceOffset);
        if (voiceRecord->ChannelStateOffset == channelStateOffset &&
            voiceRecord->AllocationPriority != 0x00 &&
            voiceRecord->SourceNote == (uint8_t) note)
        {
            break;
        }
    }

    voiceRecord->AllocationPriority = 0x00;
    nextVoiceOffset = voiceRecord->NextOffset;
    previousVoiceOffset = voiceRecord->PreviousOffset;
    WriteMIDIDataWord(previousVoiceOffset, nextVoiceOffset);
    WriteMIDIDataWord(nextVoiceOffset + 0x02, previousVoiceOffset);

    previousVoiceOffset = 0x62BA;
    nextVoiceOffset = ReadMIDIDataWord(previousVoiceOffset);
    WriteMIDIDataWord(previousVoiceOffset, voiceOffset);
    voiceRecord->PreviousOffset = previousVoiceOffset;
    voiceRecord->NextOffset = nextVoiceOffset;
    WriteMIDIDataWord(nextVoiceOffset + 0x02, voiceOffset);

    {
        MIDIDriverDescriptorData *driverDescriptor;

        driverDescriptor = ActiveMIDIDriverHost;
        MIDI_DRIVER_DISPATCH(driverDescriptor)->releaseVoice(voiceOffset);
    }
    return TRUE;
}

// 3621:05E7: uint16 StartMIDINote(
//     uint16 channel, uint16 note, uint16 velocity)
// Verified against 3621:05E7-069C.
uint16_t StartMIDINote(uint16_t channel, uint16_t note, uint16_t velocity)
{
    MIDIChannelState *channelState;
    MIDIVoiceRecord *voiceRecord;
    uint16_t channelStateOffset;
    uint16_t voiceOffset;
    uint16_t nextVoiceOffset;
    uint16_t previousVoiceOffset;
    uint8_t channelFlags;
    uint8_t allocationPriority;

    channelStateOffset = (uint16_t) (0x6420 + (channel << 0x03));
    channelState = MIDIChannelStateAt(channelStateOffset);
    channelFlags = channelState->VoiceClass;
    if (channelFlags == 0x00)
        return FALSE;

    if ((channelFlags & 0x40) != 0x00)
    {
        uint8_t mappedNote;

        mappedNote = (uint8_t) note - channelState->NoteTransposeAndFlags;
        if (mappedNote >= 0x20)
            return FALSE;
        mappedNote += 0x10;
        channelStateOffset = (uint16_t) (0x6420 + (mappedNote << 0x03));
        channelState = MIDIChannelStateAt(channelStateOffset);
        note = 0x0000;
    }

    allocationPriority = channelState->AllocationPriority;
    channelFlags = channelState->VoiceClass;
    voiceOffset = 0x62BA;
    for (;;)
    {
        uint16_t assignedChannelOffset;

        voiceOffset = ReadMIDIDataWord(voiceOffset);
        if (ReadMIDIDataWord(voiceOffset) == 0x0000)
            return FALSE;
        voiceRecord = MIDIVoiceRecordAt(voiceOffset);
        if (voiceRecord->AllocationPriority > allocationPriority)
            return FALSE;
        if (voiceRecord->VoiceClass == channelFlags)
        {
            assignedChannelOffset = voiceRecord->ChannelStateOffset;
            if (assignedChannelOffset == channelStateOffset)
                break;
            if (MIDIChannelStateAt(assignedChannelOffset)->AvailableVoiceCount != 0x00)
            {
                break;
            }
        }
    }

    {
        uint16_t assignedChannelOffset;

        assignedChannelOffset = voiceRecord->ChannelStateOffset;
        --MIDIChannelStateAt(assignedChannelOffset)->AvailableVoiceCount;
        ++channelState->AvailableVoiceCount;
        voiceRecord->ChannelStateOffset = channelStateOffset;
    }

    voiceRecord->AllocationPriority = allocationPriority;
    nextVoiceOffset = voiceRecord->NextOffset;
    previousVoiceOffset = voiceRecord->PreviousOffset;
    WriteMIDIDataWord(previousVoiceOffset, nextVoiceOffset);
    WriteMIDIDataWord(nextVoiceOffset + 0x02, previousVoiceOffset);

    while (ReadMIDIDataWord(nextVoiceOffset) != 0x0000 &&
        MIDIVoiceRecordAt(nextVoiceOffset)->AllocationPriority <
            allocationPriority)
    {
        previousVoiceOffset = nextVoiceOffset;
        nextVoiceOffset = ReadMIDIDataWord(nextVoiceOffset);
    }
    voiceRecord->NextOffset = nextVoiceOffset;
    voiceRecord->PreviousOffset = previousVoiceOffset;
    WriteMIDIDataWord(previousVoiceOffset, voiceOffset);
    WriteMIDIDataWord(nextVoiceOffset + 0x02, voiceOffset);

    voiceRecord->SourceNote = (uint8_t) note;
    voiceRecord->PlaybackNote = (uint8_t) note +
        channelState->NoteTransposeAndFlags;
    {
        MIDIDriverDescriptorData *driverDescriptor;

        driverDescriptor = ActiveMIDIDriverHost;
        /* 3621:0680-0695 calls descriptor +0Ch with SI=voiceOffset,
		 * DI=channelStateOffset, and AX=velocity. */
        MIDI_DRIVER_DISPATCH(driverDescriptor)->startVoice(voiceOffset, channelStateOffset, velocity);
    }
    return TRUE;
}

// 3621:069D and 3621:069E are intentional MIDI-message no-ops.
void IgnoreMIDIPolyphonicKeyPressure(
    uint16_t channel, uint16_t note, uint16_t pressure)
{
    (void) channel;
    (void) note;
    (void) pressure;
}

void IgnoreMIDIControllerChange(uint16_t channel, uint16_t value)
{
    (void) channel;
    (void) value;
}

// 3621:069F: void ReleaseAllMIDIVoices(void)
// Verified against 3621:069F-06C6.
void ReleaseAllMIDIVoices(void)
{
    uint16_t voiceOffset = 0x62BA;

    for (;;)
    {
        voiceOffset = ReadMIDIDataWord(voiceOffset);
        if (ReadMIDIDataWord(voiceOffset) == 0x0000)
            break;
        if (MIDIVoiceRecordAt(voiceOffset)->AllocationPriority != 0x00)
        {
            MIDIDriverDescriptorData *driverDescriptor;

            MIDIVoiceRecordAt(voiceOffset)->AllocationPriority = 0x00;
            driverDescriptor = ActiveMIDIDriverHost;
            MIDI_DRIVER_DISPATCH(driverDescriptor)->releaseVoice(voiceOffset);
        }
    }
}

// 3621:06C7: void SetMIDIChannelProgram(
//     uint16 channel, uint16 program)
// Verified against 3621:06C7-06ED.
void SetMIDIChannelProgram(uint16_t channel, uint16_t program)
{
    uint16_t channelStateOffset = (uint16_t) (0x6420 + (channel << 0x03));
    MIDIChannelState *channelState = MIDIChannelStateAt(channelStateOffset);

    channelState->Program = (uint8_t) program;
    channelState->InstrumentOffset =
        (uint16_t) (0x65A8 + (program * 0x0C));
}

// 3621:06EF and 3621:06F0 are intentional MIDI-message no-ops.
void IgnoreMIDIChannelPressure(uint16_t channel, uint16_t pressure)
{
    (void) channel;
    (void) pressure;
}

void IgnoreMIDIPitchBend(uint16_t channel, uint16_t bend)
{
    (void) channel;
    (void) bend;
}

// 3621:06F1: void DispatchMIDISystemByte(uint16 midiByte)
void DispatchMIDISystemByte(uint16_t midiByte)
{
    uint8_t *buffer;
    uint16_t bufferOffset;
    uint16_t commandOffset;
    uint8_t value;

    value = (uint8_t) midiByte;
    if (value == 0xF0)
    {
        MIDISystemExclusive.HandlerOffset = 0x0708;
        return;
    }
    if (value == 0xF7)
    {
        MIDISystemExclusive.HandlerOffset = 0x0760;
        return;
    }

    switch (MIDISystemExclusive.HandlerOffset)
    {
    case 0x0708:
        if (value == ActiveMIDIDriverHost->Flags[0x02])
            MIDISystemExclusive.HandlerOffset = 0x0716;
        else
            MIDISystemExclusive.HandlerOffset = 0x0760;
        break;
    case 0x0716:
        if (value < 0x03)
        {
            MIDISystemExclusive.CommandOffset =
                (uint16_t) (value << 0x01);
            MIDISystemExclusive.HandlerOffset = 0x0724;
        }
        else
            MIDISystemExclusive.HandlerOffset = 0x0760;
        break;
    case 0x0724:
        MIDISystemExclusive.CommandByteCount = value;
        MIDISystemExclusive.RemainingByteCount = value;
        MIDISystemExclusive.WriteCursorOffset = 0x6738;
        MIDISystemExclusive.HandlerOffset = 0x072E;
        break;
    case 0x072E:
        bufferOffset = MIDISystemExclusive.WriteCursorOffset;
        MIDISystemExclusive.CommandBuffer[bufferOffset - 0x6738] = value;
        MIDISystemExclusive.WriteCursorOffset =
            (uint16_t) (bufferOffset + 0x01);
        if (--MIDISystemExclusive.RemainingByteCount != 0x00)
            break;

        buffer = MIDISystemExclusive.CommandBuffer;
        commandOffset = MIDISystemExclusive.CommandOffset;
        switch (commandOffset)
        {
        case 0x0000:
        {
            MIDIChannelState *channelState;
            uint16_t channelStateOffset;
            uint16_t voiceOffset;
            uint16_t nextOffset;
            uint16_t previousOffset;
            uint16_t instrumentOffset;
            int16_t voiceDelta;
            uint8_t channelFlags;
            uint8_t requestedVoiceCount;

            channelStateOffset = (uint16_t) (0x6420 + (buffer[0x00] << 0x03));
            channelState = MIDIChannelStateAt(channelStateOffset);
            channelState->VoiceClass = buffer[0x01];
            channelState->AllocationPriority = buffer[0x02];
            requestedVoiceCount = buffer[0x03];
            voiceDelta = (int8_t) (requestedVoiceCount -
                channelState->ReservedVoiceCount);
            if (voiceDelta == 0x00)
                break;
            if (voiceDelta < 0x00)
            {
                channelState->ReservedVoiceCount = (uint8_t) (channelState->ReservedVoiceCount + voiceDelta);
                channelState->AvailableVoiceCount = (uint8_t) (channelState->AvailableVoiceCount - voiceDelta);
            }
            else if (voiceDelta != 0x00)
            {
                while (voiceDelta != 0x00 &&
                    channelState->AvailableVoiceCount != 0x00)
                {
                    ++channelState->ReservedVoiceCount;
                    --channelState->AvailableVoiceCount;
                    --voiceDelta;
                }
                channelFlags = channelState->VoiceClass;
                previousOffset = 0x62BA;
                voiceOffset = ReadMIDIDataWord(previousOffset);
                while (voiceDelta != 0x00 &&
                    ReadMIDIDataWord(voiceOffset) != 0x0000)
                {
                    MIDIVoiceRecord *voiceRecord =
                        MIDIVoiceRecordAt(voiceOffset);
                    if (voiceRecord->VoiceClass == channelFlags)
                    {
                        uint16_t oldChannelStateOffset =
                            voiceRecord->ChannelStateOffset;
                        MIDIChannelState *oldChannelState =
                            MIDIChannelStateAt(oldChannelStateOffset);
                        if (oldChannelState->AvailableVoiceCount != 0x00)
                        {
                            --oldChannelState->AvailableVoiceCount;
                            ++channelState->ReservedVoiceCount;
                            voiceRecord->ChannelStateOffset =
                                channelStateOffset;
                            voiceRecord->AllocationPriority = 0x00;
                            nextOffset = voiceRecord->NextOffset;
                            previousOffset = voiceRecord->PreviousOffset;
                            WriteMIDIDataWord(previousOffset, nextOffset);
                            WriteMIDIDataWord(nextOffset + 0x02, previousOffset);
                            previousOffset = 0x62BA;
                            nextOffset = ReadMIDIDataWord(previousOffset);
                            voiceRecord->NextOffset = nextOffset;
                            voiceRecord->PreviousOffset = previousOffset;
                            WriteMIDIDataWord(previousOffset, voiceOffset);
                            WriteMIDIDataWord(nextOffset + 0x02, voiceOffset);
                            --voiceDelta;
                        }
                    }
                    previousOffset = voiceOffset;
                    voiceOffset = ReadMIDIDataWord(voiceOffset);
                }
            }
            channelState->Program = buffer[0x04];
            instrumentOffset = (uint16_t) (0x65A8 + (buffer[0x04] * 0x0C));
            channelState->InstrumentOffset = instrumentOffset;
            channelFlags = buffer[0x05];
            if ((channelFlags & 0x40) != 0x00)
                channelFlags |= 0x80;
            channelState->NoteTransposeAndFlags = channelFlags;
            break;
        }
        case 0x0002:
        {
            MIDI_DRIVER_DISPATCH(ActiveMIDIDriverHost)->loadInstrument(&MIDISystemExclusive.CommandBuffer[0x01], (uint8_t *) &MIDIInstrumentPatches[buffer[0x00]]);
            break;
        }
        case 0x0004:
        {
            MIDIDriverDescriptorData *driverDescriptor =
                ActiveMIDIDriverHost;
            uint16_t previousNodeOffset;
            uint16_t nodeOffset;
            uint8_t voiceCount;
            uint8_t voice;

            driverDescriptor->ActiveVoiceCount = buffer[0x00];
            WriteMIDIDataWord(0x62BA, 0x62BC);
            WriteMIDIDataWord(0x62BE, 0x62BA);
            voiceCount = driverDescriptor->ActiveVoiceCount;
            MIDIChannelStateAt(0x65A0)->AvailableVoiceCount = voiceCount;
            previousNodeOffset = 0x62BC;
            nodeOffset = 0x62C0;
            for (voice = 0x00; voice < voiceCount; ++voice)
            {
                MIDIVoiceRecord *voiceRecord = MIDIVoiceRecordAt(nodeOffset);

                voiceRecord->ChannelStateOffset = 0x65A0;
                voiceRecord->AllocationPriority = 0x00;
                voiceRecord->NextOffset = previousNodeOffset;
                voiceRecord->PreviousOffset = 0x62BA;
                WriteMIDIDataWord(0x62BA, nodeOffset);
                WriteMIDIDataWord(previousNodeOffset + 0x02, nodeOffset);
                previousNodeOffset = nodeOffset;
                nodeOffset += 0x20;
            }
            driverDescriptor->VoiceRecordsOffset = 0x62C0;
            AudioLog_MidiReset();
            MIDI_DRIVER_DISPATCH(driverDescriptor)->reset();
            break;
        }
        }
        MIDISystemExclusive.RemainingByteCount =
            MIDISystemExclusive.CommandByteCount;
        MIDISystemExclusive.WriteCursorOffset = 0x6738;
        break;
    case 0x0760:
    default:
        break;
    }
}

// 3621:0832: void ResetMIDIPlaybackState(void)
void ResetMIDIPlaybackState(void)
{
    uint8_t channel;
    MIDIDriverDescriptorData *driverDescriptor;
    MIDIVoiceRecord *voiceRecord;
    uint16_t previousNodeOffset;
    uint16_t nodeOffset;
    uint8_t voiceCount;
    uint8_t voice;

    MIDISystemExclusive.HandlerOffset = 0x0760;
    ClearMIDIData(0x6420, 0x0180);
    for (channel = 0x00; channel < 0x10; ++channel)
        MIDIChannelMap[channel] = channel;
    WriteMIDIDataWord(0x62BA, 0x62BC);
    WriteMIDIDataWord(0x62BE, 0x62BA);
    driverDescriptor = ActiveMIDIDriverHost;
    voiceCount = driverDescriptor->ActiveVoiceCount;
    MIDIChannelStateAt(0x65A0)->AvailableVoiceCount = voiceCount;
    previousNodeOffset = 0x62BC;
    nodeOffset = 0x62C0;
    for (voice = 0x00; voice < voiceCount; ++voice)
    {
        voiceRecord = MIDIVoiceRecordAt(nodeOffset);
        voiceRecord->ChannelStateOffset = 0x65A0;
        voiceRecord->AllocationPriority = 0x00;
        voiceRecord->NextOffset = previousNodeOffset;
        voiceRecord->PreviousOffset = 0x62BA;
        WriteMIDIDataWord(0x62BA, nodeOffset);
        WriteMIDIDataWord(previousNodeOffset + 0x02, nodeOffset);
        previousNodeOffset = nodeOffset;
        nodeOffset += 0x20;
    }
    driverDescriptor->VoiceRecordsOffset = 0x62C0;
    AudioLog_MidiReset();
    MIDI_DRIVER_DISPATCH(driverDescriptor)->reset();
}

// 3621:08A1: void ServiceActiveMIDIDriver(void)
void ServiceActiveMIDIDriver(void)
{
    MIDI_DRIVER_DISPATCH(ActiveMIDIDriverHost)->service();
}

// PC-speaker MIDI backend --------------------------------------------------
