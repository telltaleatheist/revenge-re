// REVENGE_3621_MIDI_DRIVERS.cpp
// The music drivers the sequencer dispatches to: null, speaker and Roland.

#include "REVENGE.h"
#include "revenge/audio.h"
#include "revenge/audio_log.h"
#include "revenge/host_dispatch.h"
#include "revenge/midi_data.h"
#include "revenge/platform_backend.h"
#include "REVENGE_3621_MIDI_INTERNAL.h"

word16 NullMIDIDriverProbe(void) { return SOUND_HARDWARE_AVAILABLE; }
void ActivateNullMIDIDriver(word16 activationArgument)
{
	(void) activationArgument;
}
void DeactivateNullMIDIDriver(void) {}
word16 FilterNullMIDIByte(word16 midiByte) { (void) midiByte; return 0x0000; }
void NullMIDIVoiceOn(
	word16 voiceRecordOffset,
	word16 channelStateOffset,
	word16 velocity)
{
	(void) voiceRecordOffset;
	(void) channelStateOffset;
	(void) velocity;
}
void NullMIDIVoiceOff(word16 voiceRecordOffset)
{
	(void) voiceRecordOffset;
}
void ResetNullMIDIDriver(void) {}
void LoadNullMIDIInstrument(
	const byte *packedInstrument,
	byte *decodedInstrument)
{
	(void) packedInstrument;
	(void) decodedInstrument;
}
void ServiceNullMIDIDriver(void) {}

MIDIDriverDescriptorData NullMIDIDriverDescriptor =
{
	.Flags = { 0x00, 0x00, 0x00 },
	.AvailableVoiceCount = 0x00,
	.ProbeOffset = 0x08A8,
	.ActivateOffset = 0x08AC,
	.DeactivateOffset = 0x08AD,
	.FilterByteOffset = 0x08AE,
	.StartVoiceOffset = 0x08B1,
	.ReleaseVoiceOffset = 0x08B2,
	.ResetOffset = 0x08B3,
	.LoadInstrumentOffset = 0x08B4,
	.ServiceOffset = 0x08B5,
	.ActivationArgument = 0x0000,
	.VoiceRecordsOffset = 0x0000,
	.ActiveVoiceCount = 0x00
};

static const MIDIDriverHostDispatch NullMIDIDriverHostDispatch =
{
	NullMIDIDriverProbe,
	ActivateNullMIDIDriver,
	DeactivateNullMIDIDriver,
	FilterNullMIDIByte,
	NullMIDIVoiceOn,
	NullMIDIVoiceOff,
	ResetNullMIDIDriver,
	LoadNullMIDIInstrument,
	ServiceNullMIDIDriver
};

word16 SpeakerMIDIDriverProbe(void) { return SOUND_HARDWARE_AVAILABLE; }

void ActivateSpeakerMIDIDriver(word16 activationArgument)
{
	(void) activationArgument;
	/* CLI/STI around this (3621:08BA, 08D1) go. */
	SpeakerMIDIRuntime.ActivePITDivisor = 0x0014;
	AudioLog_SpeakerGate(1);
	SDLBackend_EnablePCSpeaker();
	AudioLog_SpeakerDivisor(0x0014);
	SDLBackend_WritePCSpeakerPITDivisor(0x0014);
}

void DeactivateSpeakerMIDIDriver(void)
{
	/* CLI/STI around this (3621:08D3, 08DA) go. */
	AudioLog_SpeakerGate(0);
	SDLBackend_SilencePCSpeaker();
}

word16 FilterSpeakerMIDIByte(word16 midiByte)
{
	(void) midiByte;
	return 0x0000;
}

void SpeakerMIDIVoiceOn(
	word16 voiceRecordOffset,
	word16 channelStateOffset,
	word16 velocity)
{
	MIDIVoiceRecord *voiceRecord;
	byte note;
	word16 divisor;

	(void) channelStateOffset;
	(void) velocity;
	voiceRecord = MIDIVoiceRecordAt(voiceRecordOffset);
	note = voiceRecord->PlaybackNote;
	divisor = 0x0014;
	if (note >= 0x0E)
		divisor = SpeakerMIDIFrequencyDivisors[note - 0x0E];
	voiceRecord->Driver.Speaker.PITDivisor = divisor;
}

void SpeakerMIDIVoiceOff(word16 voiceRecordOffset)
{
	MIDIVoiceRecordAt(voiceRecordOffset)->
		Driver.Speaker.PITDivisor = 0x0014;
}

void ResetSpeakerMIDIDriver(void)
{
	word16 voice;
	byte count;

	voice = SpeakerMIDIDriverDescriptor.VoiceRecordsOffset;
	SpeakerMIDIRuntime.ServiceVoiceOffset = voice;
	SpeakerMIDIRuntime.ServiceVoiceCountdown =
		SpeakerMIDIDriverDescriptor.ActiveVoiceCount;
	for (count = 0x00;
		count < SpeakerMIDIDriverDescriptor.ActiveVoiceCount;
		++count)
	{
		MIDIVoiceRecord *voiceRecord = MIDIVoiceRecordAt(voice);

		voiceRecord->VoiceClass = 0x01;
		voiceRecord->Driver.Speaker.PITDivisor = 0x0014;
		voice += 0x20;
	}
}

void LoadSpeakerMIDIInstrument(
	const byte *packedInstrument,
	byte *decodedInstrument)
{
	(void) packedInstrument;
	(void) decodedInstrument;
}

void ServiceSpeakerMIDIDriver(void)
{
	word16 voice;
	word16 divisor;

	if (SpeakerMIDIDriverDescriptor.ActiveVoiceCount == 0x00)
		return;
	voice = SpeakerMIDIRuntime.ServiceVoiceOffset;
	divisor = MIDIVoiceRecordAt(voice)->
		Driver.Speaker.PITDivisor;
	if (divisor != SpeakerMIDIRuntime.ActivePITDivisor)
	{
		SpeakerMIDIRuntime.ActivePITDivisor = divisor;
		AudioLog_SpeakerDivisor(divisor);
		SDLBackend_WritePCSpeakerPITDivisor(divisor);
	}
	voice += 0x20;
	if (--SpeakerMIDIRuntime.ServiceVoiceCountdown == 0x00)
	{
		SpeakerMIDIRuntime.ServiceVoiceCountdown =
			SpeakerMIDIDriverDescriptor.ActiveVoiceCount;
		voice = SpeakerMIDIDriverDescriptor.VoiceRecordsOffset;
	}
	SpeakerMIDIRuntime.ServiceVoiceOffset = voice;
}

MIDIDriverDescriptorData SpeakerMIDIDriverDescriptor =
{
	.Flags = { 0x00, 0x00, 0x7F },
	.AvailableVoiceCount = 0x04,
	.ProbeOffset = 0x08B6,
	.ActivateOffset = 0x08BA,
	.DeactivateOffset = 0x08D3,
	.FilterByteOffset = 0x08DC,
	.StartVoiceOffset = 0x08DF,
	.ReleaseVoiceOffset = 0x08F8,
	.ResetOffset = 0x08FF,
	.LoadInstrumentOffset = 0x0922,
	.ServiceOffset = 0x0923,
	.ActivationArgument = 0x4000,
	.VoiceRecordsOffset = 0x0000,
	.ActiveVoiceCount = 0x00
};

static const MIDIDriverHostDispatch SpeakerMIDIDriverHostDispatch =
{
	SpeakerMIDIDriverProbe,
	ActivateSpeakerMIDIDriver,
	DeactivateSpeakerMIDIDriver,
	FilterSpeakerMIDIByte,
	SpeakerMIDIVoiceOn,
	SpeakerMIDIVoiceOff,
	ResetSpeakerMIDIDriver,
	LoadSpeakerMIDIInstrument,
	ServiceSpeakerMIDIDriver
};

// The original's Tandy (3621:0970-0A5C), IBM PS/1 (3621:0A90-0B88) and AdLib
// (3621:0BBE-0E5E) drivers came here. The music setting has two values,
// soundfont (the Roland driver) and speaker, so the rebuild has no driver for
// those cards: Replaced in the ledgers, with their data.

// Roland MPU-401 UART MIDI backend ----------------------------------------

void ServiceRolandMIDIDriver(void);

// 3621:0E60. The original reset the MPU-401 through ports 331h/330h and
// succeeded only on its acknowledgement FEh; the rebuild's MIDI is the host's
// music (SDLBackend_WriteMIDIByte), always there.
word16 RolandMIDIDriverProbe(void)
{
	return SOUND_HARDWARE_AVAILABLE;
}

// 3621:0E94. The original entered UART mode (port 331h), installed its input
// handler on IRQ 2 (3621:0F60) and drained ten stale input bytes (port 330h);
// the rebuild has no MPU-401 and takes no MIDI input. What stays is the empty
// 100h-byte transmit queue.
void ActivateRolandMIDIDriver(word16 activationArgument)
{
	(void) activationArgument;
	/* The STI after this (3621:0ED8) goes. */
	RolandMPU401.WriteIndex = 0x0000;
	RolandMPU401.ReadIndex = 0x0000;
	RolandMPU401.ByteCount = 0x0000;
	/* The original then cleared the IRQ 2 vector it saved (DS:6C0E), which
	 * the rebuild never takes. */
}

// 3621:0EDA. The queue is drained as the original drained it; the original
// then gave IRQ 2 its old handler back, which the rebuild never took.
void DeactivateRolandMIDIDriver(void)
{
	while (RolandMPU401.ByteCount != 0x0000)
		ServiceRolandMIDIDriver();
}

word16 FilterRolandMIDIByte(word16 midiByte)
{
	word16 writeIndex;
	byte outputByte;

	outputByte = (byte) midiByte;
	if ((outputByte & 0x80) != 0x00 && outputByte < 0xF0)
	{
		outputByte = (byte) ((outputByte & 0xF0) |
			MIDIChannelMap[outputByte & 0x0F]);
	}
	while (RolandMPU401.ByteCount >= 0x0100)
		ServiceRolandMIDIDriver();
	/* CLI/STI around the queue write (3621:0F0F, 0F29) go. */
	writeIndex = RolandMPU401.WriteIndex;
	RolandMPU401.TransmitQueue[writeIndex] = outputByte;
	++writeIndex;
	if (writeIndex >= 0x0100)
		writeIndex = 0x0000;
	RolandMPU401.WriteIndex = writeIndex;
	++RolandMPU401.ByteCount;
	return TRUE;
}

void ResetRolandMIDIDriver(void)
{
	FilterRolandMIDIByte(0x00FF);
}

// 3621:0F32 is the shared no-op voice-on, voice-off, and instrument-load
// callback. Its arguments are intentionally ignored by the Roland driver.
void RolandMIDIDriverNoOp(void) {}

void ServiceRolandMIDIDriver(void)
{
	word16 readIndex;

	if (RolandMPU401.ByteCount == 0x0000)
		return;
	/* 3621:0F33 also waited for the MPU-401's ready bit (port 331h, 40h
	 * clear); the host takes a byte at any time. */
	readIndex = RolandMPU401.ReadIndex;
	SDLBackend_WriteMIDIByte(
		RolandMPU401.TransmitQueue[readIndex]);
	++readIndex;
	if (readIndex >= 0x0100)
		readIndex = 0x0000;
	RolandMPU401.ReadIndex = readIndex;
	--RolandMPU401.ByteCount;
}

MIDIDriverDescriptorData RolandMIDIDriverDescriptor =
{
	.Flags = { 0x00, 0x00, 0x41 },
	.AvailableVoiceCount = 0x00,
	.ProbeOffset = 0x0E60,
	.ActivateOffset = 0x0E94,
	.DeactivateOffset = 0x0EDA,
	.FilterByteOffset = 0x0EF4,
	.StartVoiceOffset = 0x0F32,
	.ReleaseVoiceOffset = 0x0F32,
	.ResetOffset = 0x0F2E,
	.LoadInstrumentOffset = 0x0F32,
	.ServiceOffset = 0x0F33,
	.ActivationArgument = 0x0200,
	.VoiceRecordsOffset = 0x0000,
	.ActiveVoiceCount = 0x00
};

static const MIDIDriverHostDispatch RolandMIDIDriverHostDispatch =
{
	RolandMIDIDriverProbe,
	ActivateRolandMIDIDriver,
	DeactivateRolandMIDIDriver,
	FilterRolandMIDIByte,
	(void (*)(word16, word16, word16)) RolandMIDIDriverNoOp,
	(void (*)(word16)) RolandMIDIDriverNoOp,
	ResetRolandMIDIDriver,
	(void (*)(const byte *, byte *)) RolandMIDIDriverNoOp,
	ServiceRolandMIDIDriver
};

const void *MIDIDriverDescriptorIdentities[MIDI_DRIVER_HOST_COUNT] =
{
	&SpeakerMIDIDriverDescriptor,
	&RolandMIDIDriverDescriptor,
	NULL
};

/* The last is the null driver's, for any other descriptor. */
const void *MIDIDriverHostDispatches[MIDI_DRIVER_HOST_COUNT] =
{
	&SpeakerMIDIDriverHostDispatch,
	&RolandMIDIDriverHostDispatch,
	&NullMIDIDriverHostDispatch
};

/* The original's MPU-401 input handler (3621:0F60, IRQ 2) read and dropped
 * each byte the MPU-401 sent; the rebuild takes no MIDI input. Replaced. */
