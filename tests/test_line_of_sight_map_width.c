#include "revenge/far_memory.h"
#include "revenge/original_data.h"
#include "revenge/representation.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Line of sight (0800:078C) walks the terrain map by the row steps at
 * 4184:08FB and 4184:091F. The map loader (1A1F:058F-05AC) writes the map's
 * width into eight of those words, at 3A6A:7A9B-7ACB, the same memory, so the
 * trace walks rows of the loaded map's width. No map is 96 wide, the width
 * the image starts the tables at. */
void LoadTacticalMap(word16 mapNumber);
word16 TraceMapLineOfSight(int16 sourceX, int16 sourceY, int16 targetX, int16 targetY,
	const byte *terrainTypeMap, byte specialTerrainMinimum, byte specialTerrainMaximum,
	byte blockingTerrainThreshold, byte maximumSteps);

enum
{
	WIDTH = 55,
	HEIGHT = 70,
	OPEN = 0x00,
	BLOCKING = 0xCC,
	CLEAR_COLUMN = 20,
	CLEAR_ROW = 30
};

static word16 Trace(int16 sourceX, int16 sourceY, int16 targetX, int16 targetY)
{
	/* The first battle's terrain bounds; no step limit (0 wraps to 255). */
	return TraceMapLineOfSight(sourceX, sourceY, targetX, targetY, TerrainTypeMap,
		0xA0, 0xC6, BLOCKING, 0x00);
}

int main(void)
{
	int x;
	int y;

	/* The map as the game allocates it: one block a far pointer addresses. */
	TerrainTypeMap = (byte *) calloc(0x8000, 1);
	assert(TerrainTypeMap != NULL);
	HostFarPointer_Reset();
	HostFarPointer_Register(TerrainTypeMap, 0x8000);
	OriginalData_InitializeResourceDescriptorPointers();
	/* The test backend's reads leave the buffer as it is: the header the
	 * loader reads first is these bytes, and the cells read over it after are
	 * the ones filled in below. */
	TerrainTypeMap[0x00] = WIDTH;
	TerrainTypeMap[0x02] = HEIGHT;
	LoadTacticalMap(0x0000);
	assert(MapWidth == WIDTH && MapHeight == HEIGHT);

	/* Everything blocks but one column and one row. */
	for (y = 0; y < HEIGHT; ++y)
	{
		for (x = 0; x < WIDTH; ++x)
		{
			TerrainTypeMap[y * WIDTH + x] = (x == CLEAR_COLUMN || y == CLEAR_ROW) ? OPEN : BLOCKING;
		}
	}

	/* Along the row: no row step taken, clear either way. */
	assert(Trace(5, CLEAR_ROW, 50, CLEAR_ROW) != 0x0000);
	/* Down and up the column: each step is one row of the map. */
	assert(Trace(CLEAR_COLUMN, 23, CLEAR_COLUMN, 56) != 0x0000);
	assert(Trace(CLEAR_COLUMN, 56, CLEAR_COLUMN, 23) != 0x0000);
	/* Off the clear lines, a blocked cell stops it. */
	assert(Trace(CLEAR_COLUMN, 23, CLEAR_COLUMN + 3, 56) == 0x0000);
	return 0;
}
