#include "revenge/midi_data.h"

#include <assert.h>
#include <string.h>

enum
{
    MIDI_DATA_FIRST_OFFSET = 0x62BA,
    MIDI_DATA_END_OFFSET = 0x65A8,
    MIDI_DATA_BYTE_COUNT = MIDI_DATA_END_OFFSET - MIDI_DATA_FIRST_OFFSET
};

/*
 * Exact zero-initialized MIDI allocator workspace at DS:62BA-65A7. The
 * frequency tables, driver records, OPL layouts, and MPU-401 queue that once
 * shared the monolithic image now have exact named definitions in Original.
 */
static uint8_t midiData[MIDI_DATA_BYTE_COUNT];

uint8_t *MIDIDataByte(uint16_t dataSegmentOffset)
{
    assert(dataSegmentOffset >= MIDI_DATA_FIRST_OFFSET);
    assert(dataSegmentOffset < MIDI_DATA_END_OFFSET);
    return &midiData[dataSegmentOffset - MIDI_DATA_FIRST_OFFSET];
}

uint16_t ReadMIDIDataWord(uint16_t dataSegmentOffset)
{
    uint8_t *source;

    assert(dataSegmentOffset + 0x01 < MIDI_DATA_END_OFFSET);
    source = MIDIDataByte(dataSegmentOffset);
    return (uint16_t) (source[0x00] | ((uint16_t) source[0x01] << 0x08));
}

void WriteMIDIDataWord(uint16_t dataSegmentOffset, uint16_t value)
{
    uint8_t *destination;

    assert(dataSegmentOffset + 0x01 < MIDI_DATA_END_OFFSET);
    destination = MIDIDataByte(dataSegmentOffset);
    destination[0x00] = (uint8_t) value;
    destination[0x01] = (uint8_t) (value >> 0x08);
}

void ClearMIDIData(uint16_t dataSegmentOffset, uint16_t byteCount)
{
    assert((uint32_t) dataSegmentOffset + byteCount <= MIDI_DATA_END_OFFSET);
    memset(MIDIDataByte(dataSegmentOffset), 0x00, byteCount);
}

MIDIChannelState *MIDIChannelStateAt(uint16_t dataSegmentOffset)
{
    assert(dataSegmentOffset >= 0x6420);
    assert(dataSegmentOffset + sizeof(MIDIChannelState) <=
        MIDI_DATA_END_OFFSET);
    return (MIDIChannelState *) MIDIDataByte(dataSegmentOffset);
}

MIDIVoiceRecord *MIDIVoiceRecordAt(uint16_t dataSegmentOffset)
{
    assert(dataSegmentOffset >= 0x62C0);
    assert(dataSegmentOffset + sizeof(MIDIVoiceRecord) <=
        MIDI_DATA_END_OFFSET);
    return (MIDIVoiceRecord *) MIDIDataByte(dataSegmentOffset);
}
