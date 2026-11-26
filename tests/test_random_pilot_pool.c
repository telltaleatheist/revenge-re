#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/original_data.h"
#include "revenge/original/lance_editor_data.h"
#include "Units/REVENGE_2507_LANCE_EDITOR_INTERNAL.h"
#include "original_import_for_tests.h"

#include <assert.h>
#include <string.h>

/* The random pilot pool is the replay copy of the pool (DS:1BAD, 15 bytes)
 * and its count the replay copy of the count (DS:1B8E): drawing a pilot
 * takes one byte from that pool and lowers that count, the one the campaign
 * copies forward and back and a save writes. */
static const byte *Image(word16 dataOffset)
{
	return (const byte *) &OriginalImage[ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + dataOffset];
}

static void DrawOnePilot(void)
{
	byte pool[sizeof(ReplayExtendedCampaignState)];
	byte count = CampaignState1B8E;
	int16 pilotId;
	word16 index;
	int found = 0;

	assert(count > 0x00 && count <= sizeof(pool));
	memcpy(pool, ReplayExtendedCampaignState, sizeof(pool));
	pilotId = PopRandomPilotId();
	for (index = 0x0000; index < count; ++index)
		found |= pool[index] == (byte) pilotId;
	assert(found);
	assert(CampaignState1B8E == (byte) (count - 0x01));
	assert(&RandomSelectionPoolCount == &CampaignState1B8E);
	assert(ReplayExtendedCampaignState[count - 0x01] == (byte) REVENGE_PILOT_ID_NONE);
}

int main(void)
{
	RequireOriginalImport();
	memcpy(ReplayExtendedCampaignState, Image(0x1BAD), sizeof(ReplayExtendedCampaignState));

	/* At start: the count the image holds. */
	assert(CampaignState1B8E == Image(0x1B8E)[0]);
	DrawOnePilot();
	return 0;
}
