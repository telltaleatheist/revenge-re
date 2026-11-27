// REVENGE_4184_TACTICAL_GRAPHICS_DATA.c
// Typed definitions recovered from the shared graphics data segment.

#include "revenge/original_data.h"
#include "revenge/video.h"

/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
byte WeaponCombatRatingByType[0x17];

const byte RelativeAttackDirectionByMask[0x10] =
{
	0x08, 0x06, 0x02, 0x08, 0x04, 0x05, 0x03, 0x08,
	0x00, 0x07, 0x01, 0x08, 0x08, 0x08, 0x08, 0x08
};

/* The line-step views overlap in the DOS data segment. Their typed C views
 * preserve the values and address-derived names used by the verified tracer. */
const int16 LinePrimaryYStep[0x08] =
{
	-0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, -0x01
};

const int16 LinePrimaryXStep[0x08] =
{
	0x00, 0x01, 0x01, 0x00, 0x00, -0x01, -0x01, 0x00
};

/* The row steps (4184:08FB and 4184:091F) start as the image has them, 96
 * wide; LoadTacticalMap writes the map's width into the eight non-zero
 * entries (3A6A:7A9B-7ACB is this memory), so line of sight walks rows of the
 * loaded map. Entry 3 of the primary table (4184:0901) is the width the trace
 * and the terrain builder read. */
int16 LinePrimaryRowOffset[0x08] =
{
	-0x60, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, -0x60
};

const int16 LineSecondaryYStep[0x08] =
{
	0x00, -0x01, 0x01, 0x00, 0x00, 0x01, -0x01, 0x00
};

const int16 LineSecondaryXStep[0x08] =
{
	0x01, 0x00, 0x00, 0x01, -0x01, 0x00, 0x00, -0x01
};

int16 LineSecondaryRowOffset[0x08] =
{
	0x00, -0x60, 0x60, 0x00, 0x00, 0x60, -0x60, 0x00
};

word16 EncodedSpriteClipLeftTile = 0x0000;
word16 SpriteClipRightTile = 0x0028;
word16 SpriteClipTopPixel = 0x0000;
word16 EncodedSpriteClipBottomPixel = 0x00C8;
word16 SpritePreserveMode = FALSE;

byte CgaEvenPixelPhaseLookup[0x0100];
byte CgaOddPixelPhaseLookup[0x0100];
byte CgaPackedPixelSeedTable[0x20];

word16 ActiveVideoAdapterType = VIDEO_ADAPTER_CGA;
