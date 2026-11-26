#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/original_data.h"
#include "revenge/original/audio_data.h"
#include "original_import_for_tests.h"

#include <assert.h>
#include <string.h>

/* Objects the image starts with values, as the game's data segment holds
 * them when DOS has loaded it: the pilots' records (DS:1D4C, 59 of 4 bytes:
 * identity, experience, gunnery, availability) and DS:1B8E come from the
 * player's copy; the selected unit (DS:03DC) starts at FFFFh, no unit. The
 * screens' dismiss timeout (DS:240F) and the MIDI byte parser's state
 * (DS:62B0-62B9, its first state the one for data before any status) come
 * from the player's copy too. */
static const byte *Image(word16 dataOffset)
{
	return (const byte *) &OriginalImage[ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + dataOffset];
}

int main(void)
{
	RequireOriginalImport();
	assert(sizeof(PilotRoster) == 0xEC);
	assert(memcmp(PilotRoster, Image(0x1D4C), sizeof(PilotRoster)) == 0);
	assert(CampaignState1B8E == Image(0x1B8E)[0]);
	assert(TacticalSelectedUnitIndex == 0xFFFF);
	assert(TacticalSelectedUnitIndex == (word16) (Image(0x03DC)[0] | Image(0x03DC)[1] << 8));
	assert(PresentationDismissTimeout == (word16) (Image(0x240F)[0] | Image(0x240F)[1] << 8));
	assert(sizeof(MIDIParser) == 0x0A);
	assert(memcmp(&MIDIParser, Image(0x62B0), sizeof(MIDIParser)) == 0);
	return 0;
}
