#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/original_data.h"
#include "original_import_for_tests.h"

#include <assert.h>
#include <string.h>

/* Objects the image starts with values, as the game's data segment holds
 * them when DOS has loaded it: the pilots' records (DS:1D4C, 59 of 4 bytes:
 * identity, experience, gunnery, availability) and DS:1B8E come from the
 * player's copy; the selected unit (DS:03DC) starts at FFFFh, no unit. */
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
	return 0;
}
