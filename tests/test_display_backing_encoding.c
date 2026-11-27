#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void Check(int condition, const char *message)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		++failures;
	}
}

static void TestTacticalOverviewTranslation(void)
{
	const byte source[] =
	{
		0x00, 0x7F, 0x80, 0x9F, 0xA0,
		0xBF, 0xC0, 0xC5, 0xC6, 0xFF
	};
	const byte expected[] =
	{
		0x77, 0x77, 0xEE, 0xEE, 0x33, 0x33, 0xAA, 0xAA,
		0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77
	};
	byte destination[0x10];

	memset(destination, 0xCC, sizeof(destination));
	BuildTacticalOverviewMapBuffer(
		source, destination, 0x0008, 0x0001);
	BuildTacticalOverviewMapBuffer(
		source + 0x0008, destination + 0x0008, 0x0002, 0x0001);
	Check(memcmp(destination, expected, sizeof(expected)) == 0x00,
		"overview map uses the CS:04C9 colors and pads rows to eight bytes");
}

static void TestTacticalLookupAndOverlayState(void)
{
	memset(TacticalEffectOverlayMap, 0xA5,
		sizeof(TacticalEffectOverlayMap));
	ClearTacticalEffectOverlayMap();
	Check(TacticalEffectOverlayMap[0x0000] == 0x00 &&
		TacticalEffectOverlayMap[0x00E3] == 0x00,
		"overlay clear covers the complete E4h-byte tactical map");

	DamagedTerrainSpriteLookup = NULL;
	TerrainDamageCapabilityTable = NULL;
	InstallDamagedTerrainSpriteLookup();
	Check(DamagedTerrainSpriteLookup != NULL &&
		DamagedTerrainSpriteLookup == TerrainDamageCapabilityTable,
		"damaged-terrain lookup installer publishes the port's tables to both pointers");
}

static void TestPackedAdapterRows(void)
{
	const byte source[] =
	{
		0x10, 0x11, 0x20, 0x21, 0xEE,
		0x30, 0x31, 0x40, 0x41, 0xDD
	};
	byte destination[0x10];
	word16 length;

	memset(destination, 0xCC, sizeof(destination));
	ActiveVideoAdapterType = VIDEO_ADAPTER_CGA;
	length = EncodeDisplayBackingRows(
		source, destination, 0x02, 0x02, 0x01);
	Check(length == 0x04 && destination[0x00] == 0x10 &&
		destination[0x01] == 0x20 && destination[0x02] == 0x30 &&
		destination[0x03] == 0x40,
		"CGA rows retain every other packed source byte");

	memset(destination, 0xCC, sizeof(destination));
	ActiveVideoAdapterType = VIDEO_ADAPTER_TANDY;
	length = EncodeDisplayBackingRows(
		source, destination, 0x02, 0x02, 0x01);
	Check(length == 0x08 &&
		memcmp(destination, "\x10\x11\x20\x21\x30\x31\x40\x41", 0x08) == 0x00,
		"Tandy rows retain both bytes in each source group");
}

static void TestEgaSparseRows(void)
{
	const byte source[] =
	{
		0x12, 0x34, 0x12, 0x34,
		0x00, 0x00, 0x00, 0x00
	};
	const byte expected[] =
	{
		0x88, 0x01,
		0x44, 0x02,
		0x22, 0x03,
		0x11, 0x84,
		0x00, 0xE0
	};
	byte destination[0x20];
	word16 packedPixels = 0xABCD;
	word16 length;

	Check(ExtractHighNibbleAndShiftLeft(&packedPixels) == 0x0A &&
		packedPixels == 0xBCD0,
		"nibble helper returns bits 15..12 and shifts BX left four times");

	memset(destination, 0xCC, sizeof(destination));
	ActiveVideoAdapterType = VIDEO_ADAPTER_EGA;
	length = EncodeDisplayBackingRows(
		source, destination, 0x02, 0x01, 0x00);
	Check(length == sizeof(expected),
		"EGA sparse encoding returns its emitted byte count");
	Check(memcmp(destination, expected, sizeof(expected)) == 0x00,
		"EGA sparse encoding preserves color masks and terminator flags");
}

int main(void)
{
	TestTacticalOverviewTranslation();
	TestTacticalLookupAndOverlayState();
	TestPackedAdapterRows();
	TestEgaSparseRows();
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
