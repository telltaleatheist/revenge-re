#include "fake_backend.h"
#include "revenge/audio.h"
#include "revenge/midi_data.h"

#include <assert.h>

static void TestVariableLengthQuantity(void)
{
    const byte encoded[] = { 0x81, 0x00 };
    const byte *cursor;

    cursor = encoded;
    assert(ReadMIDIVariableLengthQuantity(&cursor) == 0x0080);
    assert(cursor == encoded + sizeof(encoded));
}

/* The sequencer's voice allocation on a driver with voices of its own: the
 * speaker's four (3621:08DF gives the voice the note's divisor). */
static void TestSpeakerUnreservedVoiceAllocation(void)
{
    MIDIChannelState *channelState;
    MIDIVoiceRecord *voiceRecord;
    byte voice;

    FakeBackend_Reset();
    ActivateMIDIDriver(&SpeakerMIDIDriverDescriptor);
    channelState = MIDIChannelStateAt(0x6420);
    channelState->VoiceClass = 0x01;
    channelState->AllocationPriority = 0x64;
    assert(channelState->ReservedVoiceCount == 0x00);
    assert(MIDIChannelStateAt(0x65A0)->AvailableVoiceCount == 0x04);
    assert(StartMIDINote(0x0000, 0x003C, 0x005A) == TRUE);
    assert(channelState->AvailableVoiceCount == 0x01);
    assert(MIDIChannelStateAt(0x65A0)->AvailableVoiceCount == 0x03);
    /* The voice the channel was given (the list's order picks it). */
    voiceRecord = NULL;
    for (voice = 0x00; voice < 0x04; ++voice)
    {
        MIDIVoiceRecord *candidate = MIDIVoiceRecordAt((word16) (SpeakerMIDIDriverDescriptor.VoiceRecordsOffset + voice * 0x20));
        if (candidate->ChannelStateOffset == 0x6420)
        {
            assert(voiceRecord == NULL);
            voiceRecord = candidate;
        }
    }
    assert(voiceRecord != NULL);
    assert(voiceRecord->PlaybackNote == 0x3C);
    assert(voiceRecord->Driver.Speaker.PITDivisor ==
        SpeakerMIDIFrequencyDivisors[0x3C - 0x0E]);
    DeactivateMIDIDriver();
}

static void TestSingleTrackSequence(void)
{
    static const byte sequence[] = {
        'M', 'T', 'h', 'd', 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x60,
        'M', 'T', 'r', 'k', 0x00, 0x00, 0x00, 0x0C,
        0x00, 0x90, 0x3C, 0x40,
        0x01, 0x80, 0x3C, 0x00,
        0x00, 0xFF, 0x2F, 0x00
    };

    uint16 queuedByte;

    FakeBackend_Reset();
    assert(sizeof(MIDIDriverDescriptorData) == 0x1B);
    assert(sizeof(MIDISystemExclusiveState) == 0x28);
    assert(sizeof(SpeakerMIDIRuntimeState) == 0x06);
    assert(sizeof(RolandMPU401.TransmitQueue) == 0x0100);
    assert(SpeakerMIDIFrequencyDivisors[0x00] == 0xFDF1);
    assert(SpeakerMIDIFrequencyDivisors[0x71] == 0x005F);
    assert(SpeakerMIDIDriverDescriptor.ProbeOffset == 0x08B6);
    assert(RolandMIDIDriverDescriptor.ServiceOffset == 0x0F33);
    ActivateMIDIDriver(&RolandMIDIDriverDescriptor);
    assert(MIDIParser.ActiveDriverOffset == 0x6BD8);
    assert(MIDISystemExclusive.HandlerOffset == 0x0760);
    assert(MIDIChannelMap[0x00] == 0x00);
    assert(MIDIChannelMap[0x0F] == 0x0F);
    ServiceActiveMIDIDriver();
    FakeBackend_Reset();
    SetMIDITimingDivisor(0x0001);
    ConfigureMIDITempo(0x0001, 0x0001);
    assert(StartMIDISequence(sequence, 0x0001) == 0x0001);
    ServiceMIDISequenceTick();
    for (queuedByte = 0x0000; queuedByte < 0x0036; ++queuedByte)
        ServiceActiveMIDIDriver();
    assert(fakeBackend.midiByteCount == 0x0036);
    assert(fakeBackend.midiBytes[0x00] == 0x90);
    assert(fakeBackend.midiBytes[0x01] == 0x3C);
    assert(fakeBackend.midiBytes[0x02] == 0x40);
    assert(fakeBackend.midiBytes[0x03] == 0x80);
    assert(fakeBackend.midiBytes[0x04] == 0x3C);
    assert(fakeBackend.midiBytes[0x05] == 0x00);
    assert(RolandMPU401.ByteCount == 0x0000);
    assert(MIDISequence.MIDIPlaybackFlags == 0x0000);
    DeactivateMIDIDriver();
    assert(MIDIParser.ActiveDriverOffset == 0x6766);
}

int main(void)
{
    assert(MIDISequence.MIDIBeatsPerMinute == 0x0078);
    assert(MIDISequence.MIDITimerCallsPerMinute == 0x1110);
    assert(MIDISequence.MIDITicksPerMinute == 0x2D00);
    assert(MIDISequence.MIDITicksPerBeat == 0x0060);
    assert(MIDISequence.MIDIBeatsPerMeasure == 0x0004);
    TestVariableLengthQuantity();
    TestSpeakerUnreservedVoiceAllocation();
    TestSingleTrackSequence();
    return 0x00;
}
