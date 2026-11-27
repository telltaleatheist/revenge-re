#ifndef REVENGE_MIDI_DATA_H
#define REVENGE_MIDI_DATA_H

#include "revenge/types.h"
#include "revenge/original/audio_data.h"

/*
 * Mechanical host representation of the original DS:62BA-6D18 MIDI work
 * area.  These accessors preserve the executable's little-endian near
 * offsets without putting segmented-pointer operations in Original code.
 */
uint8_t *MIDIDataByte(uint16_t dataSegmentOffset);
uint16_t ReadMIDIDataWord(uint16_t dataSegmentOffset);
void WriteMIDIDataWord(uint16_t dataSegmentOffset, uint16_t value);
void ClearMIDIData(uint16_t dataSegmentOffset, uint16_t byteCount);
MIDIChannelState *MIDIChannelStateAt(uint16_t dataSegmentOffset);
MIDIVoiceRecord *MIDIVoiceRecordAt(uint16_t dataSegmentOffset);

#endif
