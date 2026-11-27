/* The jukebox's game side (tools/music/jukebox.cpp): the game's own MIDI
 * sequencer and Roland driver, set up as InitializeMusicDriver and
 * PlayMusicResource set them up (3014), on the tests' fake backend, whose
 * MIDI bytes go to the jukebox. No game data but the song's bytes. */
#include "fake_backend.h"
#include "revenge/audio.h"
#include "revenge/far_memory.h"

#include <string.h>

/* The game's staging buffer for a song and its bound (3014:0117,
 * LoadMusicResource 3014:0197). The game takes it from far memory; here it
 * is registered whole with the far pointers, as that allocation is, so the
 * sequencer's stream pointers into it resolve. */
enum { JUKEBOX_SONG_CAPACITY = 0x30D4 };

static byte song[JUKEBOX_SONG_CAPACITY];

void JukeboxGame_Begin(void (*sink)(unsigned char))
{
	FakeBackend_Reset();
	fakeBackend.midiSink = sink;
	(void) HostFarPointer_Register(song, JUKEBOX_SONG_CAPACITY);
	PrepareMIDIDriver(&RolandMIDIDriverDescriptor);
	SetMIDITimingDivisor(MIDI_TIMER_CALLS_PER_MINUTE);
	ActivateMIDIDriver(&RolandMIDIDriverDescriptor);
	Seg426B.MusicDriverMode = 0x02;
}

/* 1 if the song fits the game's buffer and starts. */
int JukeboxGame_Start(const unsigned char *bytes, unsigned length)
{
	if (length == 0 || length > JUKEBOX_SONG_CAPACITY)
		return 0;
	StopMIDISequence();
	memset(song, 0, JUKEBOX_SONG_CAPACITY);
	memcpy(song, bytes, length);
	(void) StartMIDISequence(song, 0x0009);
	EnableMusicPlayback();
	return 1;
}

int JukeboxGame_Playing(void)
{
	return MIDISequence.MIDIPlaybackFlags != 0x00;
}

/* The driver's own service, as the music timer ran it on every IRQ0. */
void JukeboxGame_DriverTick(void)
{
	ServiceActiveMIDIDriver();
}

void JukeboxGame_SequenceTick(void)
{
	ServiceMIDISequenceTick();
}

void JukeboxGame_Stop(void)
{
	StopMIDISequence();
}

void JukeboxGame_End(void)
{
	StopMIDISequence();
	DeactivateMIDIDriver();
	HostFarPointer_Unregister(song);
	fakeBackend.midiSink = NULL;
}
